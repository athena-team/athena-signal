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
import torch
from torch.utils.data import Dataset
import json
import numpy as np
import soundfile as sf
import torch.nn.functional as F
import pickle
import random
from scipy.signal import resample_poly
from torch.utils.data import DataLoader as Loader
import pickle

def make_dataloader(opt):
    
    # make train's dataloader
    data_dir = 'datasets'
    train_dataset = Datasets_Extractor(
        [opt[data_dir]['train']['dataroot_mix'][0]],
        [opt[data_dir]['train']['dataroot_aux']],
        [opt[data_dir]['train']['dataroot_labels']],
        [opt[data_dir]['train']['dataroot_targets'][0], opt[data_dir]['train']['dataroot_targets'][1]],
        opt['model']['MODEL'],
        **opt[data_dir]['audio_setting'])
    train_dataloader = Loader(train_dataset,
                              batch_size=opt[data_dir]['dataloader_setting']['batch_size'],
                              num_workers=opt[data_dir]['dataloader_setting']['num_workers'],
                              shuffle=opt[data_dir]['dataloader_setting']['shuffle'])

    # make validation dataloader
    val_dataset = Datasets_Extractor(
        [opt[data_dir]['val']['dataroot_mix'][0]],
        [opt[data_dir]['val']['dataroot_aux']],
        [opt[data_dir]['train']['dataroot_labels']],
        [opt[data_dir]['val']['dataroot_targets'][0], opt[data_dir]['val']['dataroot_targets'][1]],
        opt['model']['MODEL'],
        **opt[data_dir]['audio_setting'])
    val_dataloader = Loader(val_dataset,
                            batch_size=opt[data_dir]['dataloader_setting']['batch_size'],
                            num_workers=opt[data_dir]['dataloader_setting']['num_workers'],
                            shuffle=False)

    return train_dataloader, val_dataloader
    
class Datasets_Extractor(Dataset):
    '''
       Load audio data
       mix_scp: file path of mix audio (type: str)
       ref_scp: file path of ground truth audio (type: list[spk1,spk2])
       chunk_size (int, optional): split audio size (default: 32000(4 s))
       least_size (int, optional): Minimum split size (default: 16000(2 s))
    '''

    def __init__(self, mix_json=None, auxjson=None, speaker_label=None, ref_json=None, model='DPRNN_Speaker_Suppression', sample_rate=8000, chunk_size=32000, least_size=16000):
        super(Datasets_Extractor, self).__init__()
        self.sample_rate = sample_rate
        self.chunk_size = chunk_size
        self.least_size = least_size
        self.model = model

        mix_infos = []
        if len(mix_json[0]) != 0:
            for src_json in mix_json:
                with open(src_json, 'r') as f:
                    mix_infos.append(json.load(f)) # control data numnber

        sources_infos = []
        if len(ref_json[0]) != 0:
            for src_json in ref_json:
                with open(src_json, 'r') as f:
                    sources_infos.append(json.load(f)) # control data numnber

        aux_infos = []
        if len(auxjson[0]) != 0:
            for src_json in auxjson:
                with open(src_json, 'r') as f:
                    aux_infos.append(json.load(f))  # control data numnber

        if len(speaker_label[0]) != 0:
            for src_label in speaker_label:
                with open(src_label, 'rb') as fid:
                    self.label = pickle.load(fid)

        drop_utt, drop_len = 0, 0
        mix = []
        sources = []
        aux_signal = []
        self.scp_dict = dict()
        if self.model == 'DPRNN_Speech_Separation':
            for mix_inf in mix_infos:
                orig_len = len(mix_inf)
                for i in range(orig_len - 1, -1, -1):  # Go backward
                    if mix_inf[i][1] < self.chunk_size:
                        drop_utt += 1
                        drop_len += mix_inf[i][1]
                        del mix_inf[i]
                        for src_inf in sources_infos:
                            del src_inf[i]
            for i in range(len(mix_infos)):
                mix = mix + mix_infos[i]
            self.mix = mix
            self.sources = sources_infos

        elif self.model == 'DPRNN_Speaker_Extraction':
            for mix_inf, src_inf, aux_inf in zip(mix_infos, sources_infos, aux_infos):
                orig_len = len(mix_inf)
                for i in range(orig_len - 1, -1, -1):  # Go backward
                    if mix_inf[i][1] < self.chunk_size:
                        drop_utt += 1
                        drop_len += mix_inf[i][1]
                        del mix_inf[i]
                        del src_inf[i]
                        del aux_inf[i]
            for i in range(len(mix_infos)):
                mix = mix + mix_infos[i]
                sources = sources + sources_infos[i]
                aux_signal = aux_signal + aux_infos[i]
                
            # txt_path = './config/uniq_target_ref_dur.txt'
            # self.__read_line(txt_path)
            self.mix = mix
            self.aux = aux_signal
            self.sources = sources

        elif self.model == 'DPRNN_Speaker_Suppression':
            for mix_inf, src_inf, aux_inf in zip(mix_infos, sources_infos, aux_infos):
                orig_len = len(mix_inf)
                for i in range(orig_len - 1, -1, -1):  # Go backward
                    if mix_inf[i][1] < self.chunk_size:
                        drop_utt += 1
                        drop_len += mix_inf[i][1]
                        del mix_inf[i]
                        del src_inf[i]
                        del aux_inf[i]
            for i in range(len(mix_infos)):
                mix = mix + mix_infos[i]
                sources = sources + sources_infos[i]
                aux_signal = aux_signal + aux_infos[i]

            self.mix = mix
            self.aux = aux_signal
            self.sources = sources
                    
        
        print("Drop {} utts({:.4f} h) from {} (shorter than {} samples)".format(drop_utt, drop_len / sample_rate / 36000, orig_len, self.chunk_size))

    def __len__(self):
        return len(self.mix)

    def __getitem__(self, index):
        # read data
        if self.mix[index][1] <= self.chunk_size+1:
                rand_start = 0
        else:
            rand_start = np.random.randint(0, self.mix[index][1] - self.chunk_size - 1)
            # rand_start = 0
        stop = rand_start + self.chunk_size
        x, _ = sf.read(self.mix[index][0], start=rand_start, stop=stop, dtype='float32')
        mixture = torch.from_numpy(x)

        if self.model == 'DPRNN_Speech_Separation':
            sources = []
            for src in self.sources:
                s, _ = sf.read(src[index][0], start=rand_start, stop=stop, dtype='float32')
                source = torch.from_numpy(s)
                sources.append(source)
        if self.model != 'DPRNN_Speech_Separation':
            s, _ = sf.read(self.sources[index][0], start=rand_start, stop=stop, dtype='float32')
            source = torch.from_numpy(s)

            # sample rate modify
            rand_start_aux = 0
            stop_aux = self.aux[index][1]
            aux, fs = sf.read(self.aux[index][0], start=rand_start_aux, stop=stop_aux, dtype='float32')
            if fs == 16000:
                aux = resample_poly(aux, 8000, fs)
                aux = np.float32(aux)

            aux_len = len(aux)
            if aux_len > self.chunk_size:
                rand_start_aux = np.random.randint(0, aux_len - self.chunk_size)
                gap = 0
                aux_len_back = torch.tensor(self.chunk_size).long()
            else:
                rand_start_aux = 0
                gap = self.chunk_size - aux_len
                aux_len_back = torch.tensor(aux_len).long()

            stop_aux = rand_start_aux + self.chunk_size

            if stop_aux > aux_len:
                stop_aux = aux_len

            aux = aux[rand_start_aux:stop_aux]
            if gap > 0:
                aux_audio = F.pad(torch.from_numpy(aux), [0, gap], mode='constant')
            else:
                aux_audio = torch.from_numpy(aux)

        if self.model == 'DPRNN_Speech_Separation':
            return mixture, sources
        if self.model == 'DPRNN_Speaker_Suppression':
            return [mixture, aux_audio], [source, aux_len_back]
        if self.model == 'DPRNN_Speaker_Extraction':
            spe_key = self.aux[index][0].split('/')[-1][0:3]
            label_index = int(self.label[spe_key])
            aux_label = torch.tensor(label_index).long()
            return [mixture, aux_audio], [source, aux_len_back, aux_label]

class Datasets_Test(Dataset):
    '''
       Load audio data
       mix_scp: file path of mix audio (type: str)
       ref_scp: file path of ground truth audio (type: list[spk1,spk2])
       chunk_size (int, optional): split audio size (default: 32000(4 s))
       least_size (int, optional): Minimum split size (default: 16000(2 s))
    '''

    def __init__(self, mix_json=None, aux_json=None, ref_json=None, sample_rate=8000, model='DPRNN_Speech_Separation', chunk_size=32000, least_size=32000):
        super(Datasets_Test, self).__init__()
        self.sample_rate = sample_rate
        self.chunk_size = chunk_size
        self.least_size = least_size
        self.model = model
        with open(mix_json, 'r') as f:
            mix_infos = json.load(f)
        
        if aux_json != None:
            with open(aux_json, 'r') as f:
                aux_infos = json.load(f)
            self.aux = aux_infos

        sources_infos = []
        for src_json in ref_json:
            with open(src_json, 'r') as f:
                sources_infos.append(json.load(f))

        self.mix = mix_infos
        self.sources = sources_infos

    def __len__(self):
        return len(self.mix)

    def __getitem__(self, index):

        x, _ = sf.read(self.mix[index][0], start=0, dtype='float32')
        mixture = torch.from_numpy(x)

        file_name = self.mix[index][0].split('/')[-1]

        sources = []
        for src in self.sources:
            s, _ = sf.read(src[index][0], start=0, dtype='float32')
            source = torch.from_numpy(s)
            sources.append(source)

        if self.model != 'DPRNN_Speech_Separation':
            aux, _ = sf.read(self.aux[index][0], start=0, dtype='float32')
            aux_audio = torch.from_numpy(aux)
            aux_len = torch.tensor(self.aux[0][1]).long()

            return mixture, aux_audio, sources, aux_len, file_name
        else:
            return mixture, sources, file_name