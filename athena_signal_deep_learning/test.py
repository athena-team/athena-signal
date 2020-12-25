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

import os
import torch
from data_loader.wham_dataset import Datasets_Test
import argparse
from torch.nn.parallel import data_parallel
from model import model_function
from logger.set_logger import setup_logger
import logging
from config.option import parse
import tqdm
import numpy as np
from pypesq import pesq
from pystoi.stoi import stoi
from mir_eval.separation import bss_eval_sources
import math
import librosa
import datetime

# setting debug
os.makedirs('./log', exist_ok=True)
day_time = datetime.date.today().strftime('%y%m%d')
log = logging.getLogger('Extarctor-{}'.format(day_time))
log.setLevel(logging.DEBUG)
fh = logging.FileHandler(os.path.join('log', '{}_wsjo-2mix-ext-result.log'.format(day_time)))
fh.setLevel(logging.DEBUG)
ch = logging.StreamHandler()
ch.setLevel(logging.INFO)
formatter = logging.Formatter(
        '%(asctime)s - %(name)s - %(levelname)s - %(message)s')
fh.setFormatter(formatter)
ch.setFormatter(formatter)
log.addHandler(fh)
log.addHandler(ch)


def cal_SDRi(src_ref, src_est, mix):
    """Calculate Source-to-Distortion Ratio improvement (SDRi).
    NOTE: bss_eval_sources is very very slow.
    Args:
        src_ref: numpy.ndarray, [C, T]
        src_est: numpy.ndarray, [C, T], reordered by best PIT permutation
        mix: numpy.ndarray, [T]
    Returns:
        average_SDRi
    """
    num = 0
    new_sdr = 0
    orig_sdr = 0
    avg_SDRi = 0
    for ref, est in zip (src_ref, src_est):
        num = num + 1
        src_anchor = mix
        sdr, sir, sar, popt = bss_eval_sources(ref, est)
        new_sdr = new_sdr + sdr
        sdr0, sir0, sar0, popt0 = bss_eval_sources(ref, src_anchor)
        orig_sdr = orig_sdr + sdr0
        avg_SDRi = avg_SDRi + (sdr - sdr0)
    
    return new_sdr/num, orig_sdr/num, avg_SDRi/num

def cal_STOIi(src_ref, src_est, mix):
    """Calculate Source-to-Distortion Ratio improvement (SDRi).
    NOTE: bss_eval_sources is very very slow.
    Args:
        src_ref: numpy.ndarray, [C, T]
        src_est: numpy.ndarray, [C, T], reordered by best PIT permutation
        mix: numpy.ndarray, [T]
    Returns:
        average_SDRi
    """
    num = 0
    new_stoi = 0
    orig_stoi = 0
    avg_STOIi = 0
    for ref, est in zip (src_ref, src_est):
        num = num + 1
        new_stoi_out = stoi(ref, est, 8000)
        new_stoi = new_stoi + new_stoi_out
        orig_stoi_out = stoi(ref, mix, 8000)
        orig_stoi = orig_stoi + orig_stoi_out
        avg_STOIi = avg_STOIi + (new_stoi_out - orig_stoi_out)

    return new_stoi / num, orig_stoi / num, avg_STOIi / num

def cal_PESQi(src_ref, src_est, mix):
    """Calculate Source-to-Distortion Ratio improvement (SDRi).
    NOTE: bss_eval_sources is very very slow.
    Args:
        src_ref: numpy.ndarray, [C, T]
        src_est: numpy.ndarray, [C, T], reordered by best PIT permutation
        mix: numpy.ndarray, [T]
    Returns:
        average_SDRi
    """
    num = 0
    new_pesq = 0
    orig_pesq = 0
    avg_PESQi = 0
    for ref, est in zip (src_ref, src_est):
        num = num + 1
        new_pesq_out = pesq(ref, est, 8000)
        new_pesq = new_pesq + new_pesq_out
        orig_pesq_out = pesq(ref, mix, 8000)
        orig_pesq = orig_pesq + orig_pesq_out
        avg_PESQi = avg_PESQi + (new_pesq_out - orig_pesq_out)
    
    return new_pesq / num, orig_pesq / num, avg_PESQi / num


def cal_SISNRi(src_ref, src_est, mix):
    """Calculate Scale-Invariant Source-to-Noise Ratio improvement (SI-SNRi)
    Args:
        src_ref: numpy.ndarray, [C, T]
        src_est: numpy.ndarray, [C, T], reordered by best PIT permutation
        mix: numpy.ndarray, [T]
    Returns:
        average_SISNRi
    """
    num = 0
    new_si_snr = 0
    ori_si_snr = 0
    avg_SISNRi = 0
    for ref, est in zip (src_ref, src_est):
        num = num + 1
        new_si_snr_out = cal_SISNR(ref, est)
        new_si_snr = new_si_snr + new_si_snr_out
        ori_si_snr_out = cal_SISNR(ref, mix)
        ori_si_snr = ori_si_snr + ori_si_snr_out
        avg_SISNRi = avg_SISNRi + (new_si_snr_out - ori_si_snr_out)

    return new_si_snr/num, ori_si_snr/num, avg_SISNRi/num


def cal_SISNR(ref_sig, out_sig, eps=1e-8):
    """Calcuate Scale-Invariant Source-to-Noise Ratio (SI-SNR)
    Args:
        ref_sig: numpy.ndarray, [T]
        out_sig: numpy.ndarray, [T]
    Returns:
        SISNR
    """
    assert len(ref_sig) == len(out_sig)
    ref_sig = ref_sig - np.mean(ref_sig)
    out_sig = out_sig - np.mean(out_sig)
    ref_energy = np.sum(ref_sig ** 2) + eps
    proj = np.sum(ref_sig * out_sig) * ref_sig / ref_energy
    noise = out_sig - proj
    ratio = np.sum(proj ** 2) / (np.sum(noise ** 2) + eps)
    sisnr = 10 * np.log(ratio + eps) / np.log(10.0)
    return sisnr

class Separation():
    def __init__(self, mix_json, aux_json, ref_json, yaml_path, model, gpuid):
        super(Separation, self).__init__()
        opt = parse(yaml_path)
        self.data_loader = Datasets_Test(mix_json, aux_json, ref_json, sample_rate=opt['datasets']['audio_setting']['sample_rate'], model=opt['model']['MODEL'])
        self.mix_infos = self.data_loader.mix
        self.total_wavs = len(self.mix_infos)
        self.model = opt['model']['MODEL']
        # Extraction and Suppression model
        if self.model == 'DPRNN_Speaker_Extraction' or self.model == 'DPRNN_Speaker_Suppression':
            net = model_function.Extractin_Suppression_Model(**opt['Dual_Path_Aux_Speaker'])
        # Separation model
        if self.model == 'DPRNN_Speech_Separation':
            net = model_function.Speech_Serapation_Model(**opt['Dual_Path_Aux_Speaker'])
        net = torch.nn.DataParallel(net)
        dicts = torch.load(model, map_location='cpu')
        net.load_state_dict(dicts["model_state_dict"])
        setup_logger(opt['logger']['name'], opt['logger']['path'],
                            screen=opt['logger']['screen'], tofile=opt['logger']['tofile'])
        self.logger = logging.getLogger(opt['logger']['name'])
        self.logger.info('Load checkpoint from {}, epoch {: d}'.format(model, dicts["epoch"]))
        self.net=net.cuda()
    
        self.device=torch.device('cuda:{}'.format(
            gpuid[0]) if len(gpuid) > 0 else 'cpu')
        self.gpuid=tuple(gpuid)

        self.Output_wav = False

    def inference(self):
        low_count = 0
        with torch.no_grad():
            total_SISNRi = 0
            total_SDRi = 0
            total_STOIi = 0
            total_PESQi = 0
            total_SISNR = 0.0
            total_SDR = 0.0
            total_PESQ = 0
            total_STOI = 0
            total_cnt = 0
            for i in tqdm.tqdm(range(self.total_wavs)):
                if self.model == 'DPRNN_Speaker_Extraction' or self.model == 'DPRNN_Speaker_Suppression':
                    egs, aux, sources, aux_len, key = self.data_loader[i]
                    egs = egs.to(self.device)
                    aux = aux.to(self.device)
                    s1 = sources[0]
                    s1 = s1.to(self.device)
                    aux_len = aux_len.to(self.device)
                    
                    if egs.dim() == 1:
                        egs = torch.unsqueeze(egs, 0)
                        aux = torch.unsqueeze(aux, 0)
                    pad = torch.zeros((1, 10))
                    pad = pad.to(self.device)
                    pad_in = torch.cat((egs, pad), dim=1)
                    pad_in = pad_in.to(self.device)
                    aux_len = torch.unsqueeze(aux_len, dim=0)
                    ests = self.net([pad_in, aux, aux_len])
                    spks = torch.squeeze(ests[0].detach().cpu())
                if self.model == 'DPRNN_Speech_Separation':
                    egs, sources, key = self.data_loader[i]
                    egs = egs.to(self.device)
                    s1 = sources[0]
                    s1 = s1.to(self.device)
                    
                    if egs.dim() == 1:
                        egs = torch.unsqueeze(egs, 0)
                    pad = torch.zeros((1, 10))
                    pad = pad.to(self.device)
                    pad_in = torch.cat((egs, pad), dim=1)
                    pad_in = pad_in.to(self.device)
                    ests = self.net(pad_in)
                    spks = [torch.squeeze(s.detach().cpu()) for s in ests]

                if self.Output_wav == True:
                    s = spks
                    s = s - torch.mean(s)
                    s = s / torch.max(torch.abs(s))
                    low_count += 1
                    file_name = './data/test_wavs/SDRout_' + key
                    librosa.output.write_wav(file_name, s.numpy(), sr=opt['datasets']['audio_setting']['sample_rate'])
                    print('\n', low_count, key)
                
                else:
                    src_ref = np.array([]).reshape(0, s1.shape[0])
                    src_est = np.array([]).reshape(0, s1.shape[0])
                    if len(spks) != 2:
                        s = spks[0: s1.shape[0]]
                        s = s - torch.mean(s)
                        s = s / torch.max(torch.abs(s))
                        src_ref = np.concatenate((src_ref, np.expand_dims(s1.cpu().numpy(), axis=0)), axis=0)
                        src_est = np.concatenate((src_est, np.expand_dims(s.numpy(), axis=0)), axis=0)
                    else:
                        for s in spks:
                            s = s[: s1.shape[0]]
                            s = s - torch.mean(s)
                            s = s / torch.max(torch.abs(s))
                            s2 = sources[1]
                            si_snr1 = cal_SISNR(s1.cpu().numpy(), s.numpy())
                            si_snr2 = cal_SISNR(s2.cpu().numpy(), s.numpy())
                            if si_snr1 > si_snr2:
                                src_ref = np.concatenate((src_ref, np.expand_dims(s1.cpu().numpy(), axis=0)), axis=0)
                                src_est = np.concatenate((src_est, np.expand_dims(s.numpy(), axis=0)), axis=0)
                            else:
                                src_ref = np.concatenate((src_ref, np.expand_dims(s2.cpu().numpy(), axis=0)), axis=0)
                                src_est = np.concatenate((src_est, np.expand_dims(s.numpy(), axis=0)), axis=0)

                    avg_SDRi = cal_SDRi(src_ref, src_est, egs.squeeze(0).cpu().numpy())
                    avg_SISNRi = cal_SISNRi(src_ref, src_est, egs.squeeze(0).cpu().numpy())
                    avg_STOIi = cal_STOIi(src_ref, src_est, egs.squeeze(0).cpu().numpy())
                    avg_PESQi = cal_PESQi(src_ref, src_est, egs.squeeze(0).cpu().numpy())
                    total_SISNRi += avg_SISNRi[2]
                    total_PESQi += avg_PESQi[2]
                    total_STOIi += avg_STOIi[2]
                    total_PESQ += avg_PESQi[0]
                    total_STOI += avg_STOIi[0]
                    total_SISNR += avg_SISNRi[0]
                    total_SDR += avg_SDRi[0]
                    total_cnt += 1
                    total_SDRi += avg_SDRi[2]

                    if total_cnt % 20 == 0 or total_cnt == self.total_wavs:
                        log.info("Average SDR improvement: {} of the {} indexs,  avgrage SI-SNR is {}".format(
                            total_SDRi / total_cnt, i, total_SDR / total_cnt))
                        log.info("Average SISNR improvement: {} of the {} indexs, avgrage SDR is {} ".format(
                            total_SISNRi / total_cnt, i, total_SISNR / total_cnt))
                        log.info("Average PESQ improvement: {} of the {} indexs, avgrage PESQ is {}.".format(
                            total_PESQi / total_cnt,
                            i, total_PESQ / total_cnt))
                        log.info("Average STOI improvement: {} of the {} indexs, avgrage STOI is {}.".format(
                            total_STOIi / total_cnt,
                            i, total_STOI / total_cnt))
            
            log.info('\n @@@@@@@@@@ We done it!!!! @@@@@@@@ \n')
            log.info("Average SDR improvement: {}".format(total_SDRi / total_cnt))
            log.info("Average SISNR improvement: {}".format(total_SISNRi / total_cnt))
            log.info("Average PESQ improvement: {}".format(total_PESQi / total_cnt))
            log.info("Average STOI improvement: {}".format(total_STOIi / total_cnt))

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument(
        '-mix_json', type=str, default='/nfs/volume-225-15/dengchengyun/dengchengyun/trainData/wsj0_mix/2speakers/wav8k/min/tt/mix.json',
        help='Path to mix scp file.')
    parser.add_argument(
        '-s1_json', type=str,
        default=['/nfs/volume-225-15/dengchengyun/dengchengyun/trainData/wsj0_mix/2speakers/wav8k/min/tt/s1.json', '/nfs/volume-225-15/dengchengyun/dengchengyun/trainData/wsj0_mix/2speakers/wav8k/min/tt/s1.json'],
        help='Path to s1 scp file.')
    parser.add_argument(
        '-aux_json', type=str, default=None,
        help='Path to aux scp file.')
    parser.add_argument(
        '-yaml', type=str, default='./config/train_rnn.yml', help='Path to yaml file.')
    parser.add_argument(
        '-model', type=str, default='./data/checkpoint/201216_epoch120.pth.tar', help="Path to model file.")
    parser.add_argument(
        '-gpuid', type=str, default='0', help='Enter GPU id number')
    parser.add_argument(
        '-save_path', type=str, default='./result/conv_tasnet/', help='save result path')
    args=parser.parse_args()
    gpuid=[int(i) for i in args.gpuid.split(',')]
    separation=Separation(args.mix_json, args.aux_json, args.s1_json, args.yaml, args.model, gpuid)
    separation.inference()


if __name__ == "__main__":
    main()