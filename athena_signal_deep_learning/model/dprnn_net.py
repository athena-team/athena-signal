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

from torch import nn
import torch
from model.norm import select_norm

class ResNet(nn.Module):
    '''
        ResNet is defined by GeMeng.
    '''
    def __init__(self, in_channels=256, out_channels=512, kernel_size=3, padding=0, stride=3, norm='bn'):

        super(ResNet, self).__init__()
        self.in_channels = in_channels
        self.out_channels = out_channels

        self.bn1 = select_norm(norm, out_channels, 3)
        self.bn2 = select_norm(norm, in_channels, 3)

        self.prelu1 = nn.PReLU()
        self.prelu2 = nn.PReLU()

        self.conv1 = nn.Conv1d(in_channels, out_channels, 1)
        self.conv2 = nn.Conv1d(out_channels, in_channels, 1)

        self.maxpool = nn.MaxPool1d(kernel_size, stride=stride, padding=padding)

    def forward(self, x):

        x_out = self.conv1(x)
        x_out = self.bn1(x_out)
        x_out = self.prelu1(x_out)
        x_out = self.conv2(x_out)
        x_out = self.bn2(x_out)
        x_out = x + x_out
        x_out = self.prelu2(x_out)
        max_out = self.maxpool(x_out)

        return max_out

class Speaker_TasNet(nn.Module):

    def __init__(self, num_repeats=3, in_channels=512, out_channels=128, kernel_size=3, stride=3, norm='ln'):

        super(Speaker_TasNet, self).__init__()

        self.norm1 = select_norm(norm, in_channels, 3)
        self.conv_in = nn.Conv1d(in_channels, out_channels, 1)

        self.RESNET = nn.ModuleList([])
        for i in range(num_repeats):
            self.RESNET.append(ResNet(out_channels, in_channels, kernel_size=kernel_size, stride=stride, padding=1))

        self.conv_out = nn.Conv1d(out_channels, out_channels, 1)

    def forward(self, x_in):

        x = x_in[0]
        x_len = x_in[1]
        y = self.norm1(x)
        y = self.conv_in(y)
        for i in range(len(self.RESNET)):
            y = self.RESNET[i](y)
        y = self.conv_out(y)
        res = torch.sum(y, dim=-1) / x_len.view(-1, 1).float()
        return torch.unsqueeze(res, dim=2)

class Dual_RNN_Block(nn.Module):
    '''
       Implementation of the intra-RNN and the inter-RNN
       input:
            in_channels: The number of expected features in the input x
            out_channels: The number of features in the hidden state h
            rnn_type: RNN, LSTM, GRU
            norm: gln = "Global Norm", cln = "Cumulative Norm", ln = "Layer Norm"
            dropout: If non-zero, introduces a Dropout layer on the outputs
                     of each LSTM layer except the last layer,
                     with dropout probability equal to dropout. Default: 0
            bidirectional: If True, becomes a bidirectional LSTM. Default: False
    '''

    def __init__(self, out_channels,
                 hidden_channels, rnn_type='LSTM', norm='ln',
                 dropout=0, bidirectional=False, num_spks=2):
        super(Dual_RNN_Block, self).__init__()
        # RNN model
        self.inter_rnn_sp = getattr(nn, rnn_type)(
            out_channels, hidden_channels, 1, batch_first=True, dropout=dropout, bidirectional=bidirectional)
        self.inter_rnn = getattr(nn, rnn_type)(
            out_channels, hidden_channels, 1, batch_first=True, dropout=dropout, bidirectional=bidirectional)
        # Norm
        self.intra_norm = select_norm(norm, out_channels, 4)
        self.inter_norm = select_norm(norm, out_channels, 4)
        # Linear
        self.intra_linear = nn.Linear(
            hidden_channels * 2 if bidirectional else hidden_channels, out_channels)
        self.inter_linear = nn.Linear(
            hidden_channels * 2 if bidirectional else hidden_channels, out_channels)

    def forward(self, x):
        '''
           x: [B, N, K, S]
           out: [Spks, B, N, K, S]
        '''
        # intra RNN
        # [BS, K, N]
        speech_in = x
        x_in = speech_in
        # speaker_in = x[1]
        B, N, K, S = speech_in.shape
        # x_in = torch.cat((speech_in, speaker_in), dim=1)
        intra_rnn = x_in.permute(0, 3, 2, 1).contiguous().view(B * S, K, N)
        # [BS, K, H]
        intra_rnn, _ = self.inter_rnn_sp(intra_rnn)
        # [BS, K, N]
        intra_rnn = self.intra_linear(intra_rnn.contiguous().view(B * S * K, -1)).view(B * S, K, -1)
        # [B, S, K, N]
        intra_rnn = intra_rnn.view(B, S, K, N)
        # [B, N, K, S]
        intra_rnn = intra_rnn.permute(0, 3, 2, 1).contiguous()
        intra_rnn = self.intra_norm(intra_rnn)

        # [B, N, K, S]
        intra_rnn = intra_rnn + speech_in

        # inter RNN
        # [BK, S, N]
        inter_rnn = intra_rnn.permute(0, 2, 3, 1).contiguous().view(B * K, S, N)
        # [BK, S, H]
        inter_rnn, _ = self.inter_rnn(inter_rnn)
        # [BK, S, N]
        inter_rnn = self.inter_linear(inter_rnn.contiguous().view(B * S * K, -1)).view(B * K, S, -1)
        # [B, K, S, N]
        inter_rnn = inter_rnn.view(B, K, S, N)
        # [B, N, K, S]
        inter_rnn = inter_rnn.permute(0, 3, 1, 2).contiguous()
        inter_rnn = self.inter_norm(inter_rnn)
        # [B, N, K, S]
        out = inter_rnn + intra_rnn

        return out


class Dual_Path_RNN(nn.Module):
    '''
       Implementation of the Dual-Path-RNN model
       input:
            in_channels: The number of expected features in the input x
            out_channels: The number of features in the hidden state h
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

    def __init__(self, in_channels, out_channels, hidden_channels,
                 rnn_type='LSTM', norm='ln', dropout=0, sp_channels=0,
                 bidirectional=False, num_layers=4, K=200, num_spks=2):
        super(Dual_Path_RNN, self).__init__()
        self.K = K
        self.num_spks = num_spks
        self.sp_channels = sp_channels
        self.num_layers = num_layers
        self.norm = select_norm(norm, in_channels, 3)
        self.conv1d = nn.Conv1d(in_channels + self.sp_channels, out_channels, 1, bias=False)

        self.dual_rnn = nn.ModuleList([])
        for i in range(num_layers):
            self.dual_rnn.append(Dual_RNN_Block(out_channels, hidden_channels,
                                                rnn_type=rnn_type, norm=norm, dropout=dropout,
                                                bidirectional=bidirectional))
        self.conv2d = nn.Conv2d(
            out_channels, out_channels * num_spks, kernel_size=1)
        self.end_conv1x1 = nn.Conv1d(out_channels, in_channels, 1, bias=False)
        self.prelu = nn.PReLU()
        self.activation = nn.ReLU()
        # gated output layer
        self.output = nn.Sequential(nn.Conv1d(out_channels, out_channels, 1),
                                    nn.Tanh()
                                    )
        self.output_gate = nn.Sequential(nn.Conv1d(out_channels, out_channels, 1),
                                         nn.Sigmoid()
                                         )

    def forward(self, x_in):
        '''
           x: [B, N, L]

        '''
        if len(x_in) == 1:
            # [B, N, L]
            x = x_in[0]
            x = self.norm(x)
            # [B, N, L]
            x = self.conv1d(x)
        if len(x_in) == 2:
            # [B, N, L]
            x = x_in[0]
            sp_in = x_in[1]
            x = self.norm(x)
            # [B, N, L]
            B0, _, T0 = x.size()
            sp_in = sp_in.expand((B0, self.sp_channels, T0))
            sp_x_in = torch.cat((x, sp_in), dim=1)
            x = self.conv1d(sp_x_in)
        # [B, N, K, S]
        x, gap = self._Segmentation(x, self.K)
        # [B, N*spks, K, S]
        for i in range(self.num_layers):
            x = self.dual_rnn[i](x)
        x = self.prelu(x)
        x = self.conv2d(x)
        # [B*spks, N, K, S]
        B, _, K, S = x.shape
        x = x.view(B * self.num_spks, -1, K, S)
        # [B*spks, N, L]
        x = self._over_add(x, gap)
        x = self.output(x) * self.output_gate(x)
        # [spks*B, N, L]
        x = self.end_conv1x1(x)
        # [B*spks, N, L] -> [B, spks, N, L]
        _, N, L = x.shape
        x = x.view(B, self.num_spks, N, L)
        x = self.activation(x)
        # [spks, B, N, L]
        x = x.transpose(0, 1)

        return x

    def _padding(self, input, K):
        '''
           padding the audio times
           K: chunks of length
           P: hop size
           input: [B, N, L]
        '''
        B, N, L = input.shape
        P = K // 2
        gap = K - (P + L % K) % K
        if gap > 0:
            pad = torch.Tensor(torch.zeros(B, N, gap)).type(input.type())
            input = torch.cat([input, pad], dim=2)

        _pad = torch.Tensor(torch.zeros(B, N, P)).type(input.type())
        input = torch.cat([_pad, input, _pad], dim=2)

        return input, gap

    def _Segmentation(self, input, K):
        '''
           the segmentation stage splits
           K: chunks of length
           P: hop size
           input: [B, N, L]
           output: [B, N, K, S]
        '''
        B, N, L = input.shape
        P = K // 2
        input, gap = self._padding(input, K)
        # [B, N, K, S]
        input1 = input[:, :, :-P].contiguous().view(B, N, -1, K)
        input2 = input[:, :, P:].contiguous().view(B, N, -1, K)
        input = torch.cat([input1, input2], dim=3).view(B, N, -1, K).transpose(2, 3)

        return input.contiguous(), gap

    def _over_add(self, input, gap):
        '''
           Merge sequence
           input: [B, N, K, S]
           gap: padding length
           output: [B, N, L]
        '''
        B, N, K, S = input.shape
        P = K // 2
        # [B, N, S, K]
        input = input.transpose(2, 3).contiguous().view(B, N, -1, K * 2)

        input1 = input[:, :, :, :K].contiguous().view(B, N, -1)[:, :, P:]
        input2 = input[:, :, :, K:].contiguous().view(B, N, -1)[:, :, :-P]
        input = input1 + input2
        # [B, N, L]
        if gap > 0:
            input = input[:, :, :-gap]

        return input