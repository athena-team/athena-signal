# Athena-signal deep learning open source library

## What is Athena-signal deep learning?

Athena-signal deep learning is an open-source implementation of speech signal processing algorithms, including speech separation, speaker extraction and speaker suppression. 
It aims to help reserchers and engineers who want to use deep learning speech signal processing algorithms in their own projects. 
Athena-signal deep learning is mainly implemented using python based on pytorch.

## Introduction of athena-signal modules

Currently athena_signal deep learning is composed of such module as speech separation, speaker extraction and speaker suppression.

### Detailed description of each module

- Speech Separation: The current speech separation model mainly separates the mixed sources of two people, and the outpit results includes s1 and s2 respectively, and the PIT criterion is used to determine the output order of the smallest loss to reduce redundancy.
- Speaker Extraction: The speaker extraction model can extract the voice of a specfic speaker. During the model training, the specific speaker voice must be provided to the model as an auxiliary signal, output the target speaker speech, and provide the speaker label when calculating the loss, and calculate the cross entropy. Croos entropy can improve the performance of speaker extraction model.
- Speaker Suppression: The speaker suppression model is mainly used to suppress the speech of a specific speaker. The current model mainly supports the suppression of the speech of one speaker. The experiment of suppressing the speech of multiple speakers is under further study. The training process of the speaker suppression model also needs to provide the speech of the suppression target as an auxiliary signal. And the output of the model is the speech of the non-suppression target. In the training process of the speaker suppression model, the speaker label can also be provided to calculate the cross entropy. Verifying cross entropy can also improve the performance of the speaker suppression model.

Each modules has a separate switch that controls the operating status of the module.

## Athena-signal deep learning operating instructions

1. You need to set the name of the running model. The model name is set in the configuration file. Three different model names, DPRNN_Speech_Separation, DPRNN_Speaker_Extraction, DPRNN_Speaker_Suppression represent speech separation, speaker extraction and speaker suppression.
2. You can set various parameters in the configuration file, such as sample rate. At present, the setting of each parameter are the default seeting in the scenarior of sampling rate 8000.
3. The data loading function will select the corresponding module according to different model states. Speech separation does not need auxiliary signals and speaker labels, so the configurationfiles dataroot_aux and dataroot_labels are set to [], and [opt[data_dir]['train']['dataroot_aux'][0]] and [opt[data_dir]['train']['dataroot_labels'][0]] in the wham_datset.py file is changed to [opt[data_dir]['train']['dataroot_aux']] and [opt[data_dir]['train']['dataroot_labels']]. The current speaker extraction and speaker suppression models do not need to output speaker labels, and the configuration files dataroot_labels is set to [].
4. Speaker extraction and speaker suppression model parameters are basically the same. The main difference in speech separation model parameters is reflected in the parameter sp_channels. In speaker extraction and speaker suppression, sp_channels=128, and in speech separation sp_channels=0.

## Requirements

+ Python3.x
+ torch
+ numpy
+ pypesq
+ pystoi
+ librosa
+ time
+ datetime

## Execution of example

    python train.py --opt config/train_rnn.yml

## Contributing

The open source code draws on the related programs of other open source workers. We would like to express our deep gratitude to other open source workers and welcome everyone to point out the problems.

Any contribution is welcome. All issues and pull requests are highly appreciated.
If you have any questions when using athena-signal, or any ideas to make it better, 
please feel free to contact us.