# Athena-signal processing open source library

## What is Athena-signal?

Athena-signal is an open-source implementation of speech signal processing algorithms. 
It aims to help reserchers and engineers who want to use speech signal processing algorithms in their own projects. 
Athena-signal is mainly implemented using C, and called by python.

## Introduction of athena-signal modules

Currently athena-signal is composed of such modules as Acoustic Echo Cancellation(AEC), High Pass Filter(HPF), Minimum Variance Distortionless Response(MVDR) beamformer, 
Voice Activity Detection(VAD), Noise Supression(NS), Automatic Gain Control(AGC).

### Detailed description of each module

- AEC: The core of the AEC algorithm includes time delay estimation, linear echo cancellation, double-talk detection, echo return loss estimation
and residual echo suppression. 
- HPF: High-pass filtering is implemented using cascaded-iir-filter. The cut-off frequency is 200Hz in this program. You can rewrite the iir filter
coefficients and gains, with the help of filter design toolbox in MATLAB, to generate high-pass filter with cut-off frequency you set.
- MVDR: This is a Minimum Variance Distortionless Response beamformer. At present you have to set the steering vector(loc_phi) by yourself, which
indicates the distortionless response direction. Rnn matrix is estimated using MCRA noise estimation method. Microphone array could be any shape as long as you
set the coordinates of each microphones(mic_coord) beforehand. In the future edition, the steering vector will be estimated by DOA estimation.
You can set the steering vector by your own DOA estimation method of course.
- VAD: Voice Activity Detection(VAD) function outputs the current frame speech state based on the result of the double-talk detection.
- NS: Noise reduction algorithm is based on MCRA noise estimation method.
Details can be found in "Noise Estimation by Minima Controlled Recursive Averaging for Robust Speech Enhancement" and "Noise Spectrum Estimation in
Adverse Environments: Improved Minima Controlled Recursive Averaging"
- AGC: Automatic Gain Control(AGC) determines the gain factor based on the current frame signal level and the target level so that the gain of the signal is kept within a reasonable range.

Each modules has a separate switch that controls the operating status of the module. In the code, modules are turned off by default except AEC and NS.

## Athena-signal operating instructions

1. Before use, you need to set the switches of each module to determine the modules to be run or not. 
Enter the number of mic and reference, and the coordinates of each microphone. 
In this framework, the function switches AEC_KEY, HPF_KEY, MVDR_KEY, NS_KEY, AGC_KEY exist in the AEC, HPF, MVDR, NS, AGC. 
When the switch is set to 1, it is in the running state, and when it is 0, the module will be skipped.
Generally you have to manually set the number of microphones and reference channels, and the coordinates for each microphone. 
The coordinates MUST be set when MVDR is enabled.
2. You can set the length of the read and write data array_frm_len, and it is 128 by default, 
ptr_input_data and ptr_ref_data store the microphone signal data and reference signal data, respectively. 
The multi-channel microphone signals are stored in the form of parallel input, that is, the data of each channel is sequentially stored in ptr_input_data. 
3. Since MVDR requires the angle of incidence of the sound source, we set it to 90 by default. 
MVDR supports ANY array setups, including circular array and linear array, as long as you set the coordinates of microphones mic_coord beforehand. 
	
## Requirements

+ Python3.x
+ swig
+ numpy
+ setuptools

## Installation

### Supported environments

+ Linux
+ MacOS
+ Windows

### Install from source code

    swig -python athena_signal/dios_signal.i
    python setup.py bdist_wheel sdist
    
    #For Linux or MacOS
    pip install --ignore-installed  dist/athena_signal-0.1.0-*.whl
    
    #For Windows
    for /r dist %%i in (athena_signal-*.whl) do pip install --ignore-installed %%i

## Execution of example

    python examples/athena_signal_test.py
    
## Configures Setting[Options]
    
    config(dictionary):
        --add_AEC         : If True, do AEC on  signal.
                            (bool, default = True)
        --add_NS         : If True, do NS on signal.
                            (bool, default = True)
        --add_AGC         : If True, do AGC on signal.
                            (bool, default = False)
        --add_HPF         : If True, do HPF on signal.
                            (bool, default = False)
        --add_MVDR        : If True, do MVDR on signal.
                            (bool, default = False)
        --mic_num         : Number of microphones.
                            (int, default = 1)
        --ref_num         : Number of reference channel.
                            (int, default = 1)
    mic_coord(array/list): 
        The coordinates of each microphone of the microphone array
        using in MVDR. (A float array/list of size [mic_num, 3] containing
        three-dimensional coordinates of every microphone.)
## Usage

    from athena_signal.dios_ssp_api import athena_signal_process
    
    #Test AEC
    input_file = ["examples/0841-0875_env7_sit1_male_in.pcm"]
    ref_file = ["examples/0841-0875_env7_sit1_male_ref.pcm"]
    out_file = ["examples/0841-0875_env7_sit1_male_out.pcm"]
    config = {'add_AEC': True, 'add_MVDR': False}
    athena_signal_process(input_file, out_file, ref_file, config)
    
    # Test MVDR
    input_file = ["examples/m0f60_5cm_1_mix.pcm",
                  "examples/m0f60_5cm_2_mix.pcm",
                  "examples/m0f60_5cm_3_mix.pcm",
                  "examples/m0f60_5cm_4_mix.pcm",
                  "examples/m0f60_5cm_5_mix.pcm",
                  "examples/m0f60_5cm_6_mix.pcm"]
    out_file = ["examples/m0f60_5cm_mvdr_out.pcm"]
    config = {'add_AEC': False, 'add_MVDR': True, 'mic_num': 6}
    mic_coord = [[0.05, 0.0, 0.0],
                 [0.025, 0.0433, 0.0],
                 [-0.025, 0.0433, 0.0],
                 [-0.05, 0.0, 0.0],
                 [-0.025, -0.0433, 0.0],
                 [0.025, -0.0433, 0.0]]
    athena_signal_process(input_file, out_file, config=config, mic_coord=mic_coord)

## Contributing

Any contribution is welcome. All issues and pull requests are highly appreciated.
If you have any questions when using athena-signal, or any ideas to make it better, 
please feel free to contact us.

## Acknowledgement

Athena-signal is built with the help of some open-source repos such as WebRTC, speex, etc. We thank to them.
