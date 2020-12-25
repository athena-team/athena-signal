# Copyright (C) 2017 Beijing Didi Infinity Technology and Development Co.,Ltd.
# All rights reserved.

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#     http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================

import sys

sys.path.append('../')

from torch import nn
import torch
from trainer import check_parameters
import warnings
from model.norm import select_norm
from model.en_decoder import Encoder
from model.en_decoder import Decoder
from model.dprnn_net import Speaker_TasNet
from model.dprnn_net import Dual_Path_RNN
import time

warnings.filterwarnings('ignore')

# speech separation main function
class Speech_Serapation_Model(nn.Module):
    '''
       model of Dual Path RNN
       input:
            in_channels: The number of expected features in the input x
            out_channels: The number of features in the hidden state h
            hidden_channels: The hidden size of RNN
            kernel_size: Encoder and Decoder Kernel size
            rnn_type: RNN, LSTM, GRU
            norm: gln = "Global Norm", cln = "Cumulative Norm", ln = "Layer Norm"
            dropout: If non-zero, introduces a Dropout layer on the outputs
                     of each LSTM layer except the last layer,
                     with dropout probability equal to dropout. Default: 0
            bidirectional: If True, becomes a bidirectional LSTM. Default: False
            num_layers: number of Dual-Path-Block
            K: the length of chunk
            num_spks: the number of speakers
    '''

    def __init__(self, 
                 in_channels, 
                 out_channels, 
                 hidden_channels,
                 S_R,
                 num_speakers,
                 sp_channels,
                 P,
                 kernel_size=2, 
                 rnn_type='LSTM', 
                 norm='ln', 
                 dropout=0,
                 bidirectional=False, 
                 num_layers=4, 
                 K=200, 
                 num_spks=2):
        super(Speech_Serapation_Model, self).__init__()
        self.encoder = Encoder(kernel_size=kernel_size, out_channels=in_channels)
        self.separation = Dual_Path_RNN(in_channels, 
                                        out_channels, 
                                        hidden_channels,
                                        rnn_type=rnn_type, 
                                        norm=norm, 
                                        dropout=dropout,
                                        bidirectional=bidirectional, 
                                        num_layers=num_layers, 
                                        K=K, 
                                        num_spks=num_spks)
        self.decoder = Decoder(in_channels=in_channels, out_channels=1, kernel_size=kernel_size,
                               stride=kernel_size // 2, bias=False)
        self.num_spks = num_spks

    def forward(self, wavs):
        '''
           x: [B, L]
        '''
        speech_wav = wavs
        e = self.encoder(speech_wav)
        s = self.separation([e])
        out = [s[i] * e for i in range(self.num_spks)]
        audio = [self.decoder(out[i]) for i in range(self.num_spks)]
        return audio


# speaker extraction and suppression main function
class Extractin_Suppression_Model(nn.Module):
    '''
       model of Dual Path RNN
       input:
            in_channels: The number of expected features in the input x
            out_channels: The number of features in the hidden state h
            hidden_channels: The hidden size of RNN
            kernel_size: Encoder and Decoder Kernel size
            rnn_type: RNN, LSTM, GRU
            norm: gln = "Global Norm", cln = "Cumulative Norm", ln = "Layer Norm"
            dropout: If non-zero, introduces a Dropout layer on the outputs
                     of each LSTM layer except the last layer,
                     with dropout probability equal to dropout. Default: 0
            bidirectional: If True, becomes a bidirectional LSTM. Default: False
            num_layers: number of Dual-Path-Block
            K: the length of chunk
            num_spks: the number of speakers
    '''

    def __init__(self, 
                 in_channels,
                 out_channels,
                 hidden_channels,
                 S_R=3,
                 num_speakers=101,
                 sp_channels=128,
                 P=3,
                 kernel_size=2,
                 rnn_type='LSTM',
                 norm='ln',
                 dropout=0,
                 bidirectional=False,
                 num_layers=4,
                 K=100,
                 num_spks=1):
        super(Extractin_Suppression_Model, self).__init__()
        self.encoder = Encoder(kernel_size=kernel_size, out_channels=in_channels)
        self.suppression = Dual_Path_RNN(in_channels, 
                                         out_channels, 
                                         hidden_channels, 
                                         sp_channels=sp_channels,
                                         rnn_type=rnn_type, 
                                         norm=norm, 
                                         dropout=dropout,
                                         bidirectional=bidirectional, 
                                         num_layers=num_layers, 
                                         K=K, 
                                         num_spks=num_spks)
        self.speaker = Speaker_TasNet(num_repeats=S_R, in_channels=in_channels, out_channels=sp_channels, kernel_size=P)
        self.sp_linear = nn.Linear(sp_channels, num_speakers)
        self.decoder = Decoder(in_channels=in_channels, out_channels=1, kernel_size=kernel_size, stride=kernel_size // 2, bias=False)
        self.kernel_size = kernel_size
        self.num_spks = num_spks
        self.S_R = S_R

    def forward(self, wavs):
        '''
           x: [B, L]
        '''
        # [B, N, L]
        speech_wav = wavs[0]
        assist_wav = wavs[1]
        speaker_len = wavs[2]
        speaker_len = (speaker_len - self.kernel_size) // (self.kernel_size // 2) + 1
        for i in range(self.S_R):
            speaker_len = speaker_len // 3
        e = self.encoder(speech_wav)
        sp_e = self.encoder(assist_wav)
        spe_embedding = self.speaker([sp_e, speaker_len])
        spe_predict = self.sp_linear(torch.squeeze(spe_embedding, dim=-1))
        spe_embedding_norm = spe_embedding / torch.norm(spe_embedding, dim=1, keepdim=True)
        s = self.suppression([e, spe_embedding_norm])
        out = [s[i] * e for i in range(self.num_spks)]
        audio = [self.decoder(out[i]) for i in range(self.num_spks)]
        return audio[0], spe_predict




if __name__ == "__main__":
    rnn = Extractin_Suppression_Model(256, 64, 128, bidirectional=True, norm='ln', num_layers=6,
                               num_spks=1, kernel_size=16, K=100)

    lens = 32000
    batch_size = 1
    x = torch.ones(batch_size, 32000)
    speaker_in = torch.randn(batch_size, 32000)
    speaker_lens = torch.ones(batch_size, 1) * 32000
    out = rnn([x, speaker_in, speaker_lens])
    print(out[0].size())
    print("The total params is {:.3f}".format(check_parameters(rnn)))

    # start_time = time.time()
    # rnn = Extractin_Suppression_Model(256, 64, 128, S_R=3, num_speakers=101, sp_channels=0, P=3, bidirectional=True, norm='ln', num_layers=6, num_spks=1)
    # encoder = Encoder(16, 512)
    # lens = 32000
    # batch_size = 3
    # x = torch.ones(batch_size, 32000)
    # speaker_in = torch.randn(batch_size, 32000)
    # out = rnn(x)
    # print(out.size())
    # print(out[1].size())
    # print(out[0].size())
    # print("The total params is {:.3f}".format(check_parameters(rnn)))
    # end_time = time.time()
    # rtf = (end_time - start_time) / (lens / 8000.0)
    # print('The RTF is: ', rtf)