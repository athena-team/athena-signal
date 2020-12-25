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

sys.path.append('./')

import argparse
import torch
import os
import time
import numpy as np
import datetime
from torch.optim.lr_scheduler import ReduceLROnPlateau
import logging

from logger import set_logger
from config import option
from trainer import check_parameters
from trainer import save_checkpoint
from trainer import make_optimizer
from data_loader.wham_dataset import make_dataloader
from model import model_function
from model.loss import Loss_SI_SDR, accuracy_speaker, Loss

def train():
    parser = argparse.ArgumentParser(description='Parameters for training Model')
    # configuration fiule
    parser.add_argument('--opt', type=str, help='Path to option YAML file.')
    args = parser.parse_args()
    opt = option.parse(args.opt)

    set_logger.setup_logger(opt['logger']['name'], 
                            opt['logger']['path'],
                            screen=opt['logger']['screen'], 
                            tofile=opt['logger']['tofile'])
    logger = logging.getLogger(opt['logger']['name'])
    day_time = datetime.date.today().strftime('%y%m%d')

    # build model
    model = opt['model']['MODEL']
    logger.info("Building the model of {}".format(model))
    # Extraction and Suppression model
    if opt['model']['MODEL'] == 'DPRNN_Speaker_Extraction' or opt['model']['MODEL'] == 'DPRNN_Speaker_Suppression':
        net = model_function.Extractin_Suppression_Model(**opt['Dual_Path_Aux_Speaker'])
    # Separation model
    if opt['model']['MODEL'] == 'DPRNN_Speech_Separation':
        net = model_function.Speech_Serapation_Model(**opt['Dual_Path_Aux_Speaker'])
    if opt['train']['gpuid']:
        if len(opt['train']['gpuid']) > 1:
            logger.info('We use GPUs : {}'.format(opt['train']['gpuid']))
        else:
            logger.info('We use GPUs : {}'.format(opt['train']['gpuid']))

        device = torch.device('cuda:{}'.format(opt['train']['gpuid'][0]))
        gpuids = opt['train']['gpuid']
        if len(gpuids) > 1:
            net = torch.nn.DataParallel(net, device_ids=gpuids)
        net = net.to(device)
    logger.info('Loading {} parameters: {:.3f} Mb'.format(model, check_parameters(net)))

    # build optimizer
    logger.info("Building the optimizer of {}".format(model))
    Optimizer = make_optimizer(net.parameters(), opt)

    Scheduler = ReduceLROnPlateau(
        Optimizer, 
        mode='min',
        factor=opt['scheduler']['factor'],
        patience=opt['scheduler']['patience'],
        verbose=True, 
        min_lr=opt['scheduler']['min_lr'])

    # build dataloader
    logger.info('Building the dataloader of {}'.format(model))
    train_dataloader, val_dataloader= make_dataloader(opt)
    logger.info('Train Datasets Length: {}, Val Datasets Length: {}'.format(len(train_dataloader), len(val_dataloader)))

    # build trainer
    logger.info('............. Training ................')

    total_epoch = opt['train']['epoch']
    num_spks = opt['num_spks']
    print_freq = opt['logger']['print_freq']
    checkpoint_path = opt['train']['path']
    early_stop = opt['train']['early_stop']
    max_norm = opt['optim']['clip_norm']
    best_loss = np.inf
    no_improve = 0
    ce_loss = torch.nn.CrossEntropyLoss()
    weight = 0.1

    epoch = 0
    # Resume training settings
    if opt['resume']['state']:
        opt['resume']['path'] = opt['resume']['path']+'/'+'200722_epoch{}.pth.tar'.format(opt['resume']['epoch'])
        ckp = torch.load(opt['resume']['path'], map_location='cpu')
        epoch = ckp['epoch']
        logger.info("Resume from checkpoint {}: epoch {:.3f}".format(opt['resume']['path'], epoch))
        net.load_state_dict(ckp['model_state_dict'])
        net.to(device)
        Optimizer.load_state_dict(ckp['optim_state_dict'])

    while epoch < total_epoch:

        epoch += 1
        logger.info('Start training from epoch: {:d}, iter: {:d}'.format(epoch, 0))
        num_steps = len(train_dataloader)

        # trainning process
        total_SNRloss = 0.0
        total_CEloss = 0.0
        num_index = 1
        start_time = time.time()
        for inputs, targets in train_dataloader:
            # Separation train
            if opt['model']['MODEL'] == 'DPRNN_Speech_Separation':
                mix = inputs
                ref = targets
                net.train()

                mix = mix.to(device)
                ref = [ref[i].to(device) for i in range(num_spks)]

                net.zero_grad()
                train_out = net(mix)
                SNR_loss = Loss(train_out, ref)
                loss = SNR_loss

            # Extraction train
            if opt['model']['MODEL'] == 'DPRNN_Speaker_Extraction':
                mix, aux = inputs
                ref, aux_len, sp_label = targets
                net.train()

                mix = mix.to(device)
                aux = aux.to(device)
                ref = ref.to(device)
                aux_len = aux_len.to(device)
                sp_label = sp_label.to(device)

                net.zero_grad()
                train_out = net([mix, aux, aux_len])
                SNR_loss = Loss_SI_SDR(train_out[0], ref)
                CE_loss = torch.mean(ce_loss(train_out[1], sp_label))
                loss = SNR_loss + weight * CE_loss
                total_CEloss += CE_loss.item()

            # Suppression train
            if opt['model']['MODEL'] == 'DPRNN_Speaker_Suppression':
                mix, aux = inputs
                ref, aux_len = targets
                net.train()

                mix = mix.to(device)
                aux = aux.to(device)
                ref = ref.to(device)
                aux_len = aux_len.to(device)

                net.zero_grad()
                train_out = net([mix, aux, aux_len])
                SNR_loss = Loss_SI_SDR(train_out[0], ref)
                loss = SNR_loss

            # BP processs
            loss.backward()
            torch.nn.utils.clip_grad_norm_(net.parameters(), max_norm)
            Optimizer.step()

            total_SNRloss += SNR_loss.item()

            if num_index % print_freq == 0:
                message = '<Training epoch:{:d} / {:d} , iter:{:d} / {:d}, lr:{:.3e}, SI-SNR_loss:{:.3f}, CE loss:{:.3f}>'.format(
                    epoch, total_epoch, num_index, num_steps,
                    Optimizer.param_groups[0]['lr'],
                    total_SNRloss/num_index,
                    total_CEloss /num_index)
                logger.info(message)

            num_index += 1

        end_time = time.time()
        mean_SNRLoss = total_SNRloss / num_index
        mean_CELoss = total_CEloss / num_index

        message = 'Finished Training *** <epoch:{:d} / {:d}, iter:{:d}, lr:{:.3e}, ' \
                  'SNR loss:{:.3f}, CE loss:{:.3f}, Total time:{:.3f} min> '.format(
            epoch, total_epoch, num_index, Optimizer.param_groups[0]['lr'], mean_SNRLoss, mean_CELoss, (end_time - start_time) / 60)
        logger.info(message)


        # development processs
        val_num_index = 1
        val_total_loss = 0.0
        val_CE_loss = 0.0
        val_acc_total = 0.0
        val_acc = 0.0
        val_start_time = time.time()
        val_num_steps = len(val_dataloader)
        for inputs, targets in val_dataloader:
            net.eval()
            with torch.no_grad():
                # Separation development
                if opt['model']['MODEL'] == 'DPRNN_Speech_Separation':
                    mix = inputs
                    ref = targets
                    mix = mix.to(device)
                    ref = [ref[i].to(device) for i in range(num_spks)]
                    Optimizer.zero_grad()
                    val_out = net(mix)
                    val_loss = Loss(val_out, ref)
                    val_total_loss += val_loss.item()

                # Extraction development
                if opt['model']['MODEL'] == 'DPRNN_Speaker_Extraction':
                    mix, aux = inputs
                    ref, aux_len, label = targets
                    mix = mix.to(device)
                    aux = aux.to(device)
                    ref = ref.to(device)
                    aux_len = aux_len.to(device)
                    label = label.to(device)
                    Optimizer.zero_grad()
                    val_out = net([mix, aux, aux_len])
                    val_loss = Loss_SI_SDR(val_out[0], ref)
                    val_ce = torch.mean(ce_loss(val_out[1], label))
                    val_acc = accuracy_speaker(val_out[1], label)
                    val_acc_total += val_acc
                    val_total_loss += val_loss.item()
                    val_CE_loss += val_ce.item()

                # suppression development
                if opt['model']['MODEL'] == 'DPRNN_Speaker_Suppression':
                    mix, aux = inputs
                    ref, aux_len = targets
                    mix = mix.to(device)
                    aux = aux.to(device)
                    ref = ref.to(device)
                    aux_len = aux_len.to(device)
                    Optimizer.zero_grad()
                    val_out = net([mix, aux, aux_len])
                    val_loss = Loss_SI_SDR(val_out[0], ref)
                    val_total_loss += val_loss.item()

                if val_num_index % print_freq == 0:
                    message = '<Valid-Epoch:{:d} / {:d}, iter:{:d} / {:d}, lr:{:.3e}, ' \
                              'val_SISNR_loss:{:.3f}, val_CE_loss:{:.3f}, val_acc :{:.3f}>' .format(
                        epoch, total_epoch, val_num_index, val_num_steps, Optimizer.param_groups[0]['lr'],
                        val_total_loss / val_num_index,
                        val_CE_loss / val_num_index,
                        val_acc_total / val_num_index)
                    logger.info(message)
            val_num_index += 1

        val_end_time = time.time()
        mean_val_total_loss = val_total_loss / val_num_index
        mean_val_CE_loss = val_CE_loss / val_num_index
        mean_acc = val_acc_total / val_num_index
        message = 'Finished *** <epoch:{:d}, iter:{:d}, lr:{:.3e}, val SI-SNR loss:{:.3f}, val_CE_loss:{:.3f}, val_acc:{:.3f}' \
                  ' Total time:{:.3f} min> '.format(epoch, val_num_index, Optimizer.param_groups[0]['lr'],
                                                    mean_val_total_loss, mean_val_CE_loss, mean_acc,
                                                    (val_end_time - val_start_time) / 60)
        logger.info(message)

        Scheduler.step(mean_val_total_loss)

        if mean_val_total_loss >= best_loss:
            no_improve += 1
            logger.info('No improvement, Best SI-SNR Loss: {:.4f}'.format(best_loss))

        if mean_val_total_loss < best_loss:
            best_loss = mean_val_total_loss
            no_improve = 0
            save_checkpoint(epoch, checkpoint_path, net, Optimizer, day_time)
            logger.info('Epoch: {:d}, Now Best SI-SNR Loss Change: {:.4f}'.format(epoch, best_loss))

        if no_improve == early_stop:
            save_checkpoint(epoch, checkpoint_path, net, Optimizer, day_time)
            logger.info(
                "Stop training cause no impr for {:d} epochs".format(
                    no_improve))
            break


if __name__ == "__main__":
    train()
