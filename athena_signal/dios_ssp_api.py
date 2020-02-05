# /* Copyright (C) 2017 Beijing Didi Infinity Technology and Development Co.,Ltd.
# All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================*/
"""This model implements the upper-level encapsulation of dios_signal for
easy invocation."""

from athena_signal.dios_signal import dios_ssp_v1
import numpy as np

class AthenaSignal:
    """
    This model does AEC/NS/AGC/HPF/MVDR on input wav.
    """
    def __init__(self, feature_switch=[1, 1, 0, 0, 0], mic_num=1, ref_num=1):
        self.feature_switch=feature_switch
        self.mic_num=mic_num
        self.ref_num=ref_num

    def set_params(self, config=None, mic_coord=None):
        """
        Set params.
        :param config: Contains seven optional parametres:
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
        :param mic_coord: The coordinates of each microphone of the microphone array
                using in MVDR. (A float array/list of size [mic_num, 3] containing
                three-dimensional coordinates of every microphone.)
        :return: None
        """
        if 'add_AEC' in config:
            if config['add_AEC']:
                self.feature_switch[0] = 1
            else:
                self.feature_switch[0] = 0
        if 'add_NS' in config:
            if config['add_NS']:
                self.feature_switch[1] = 1
            else:
                self.feature_switch[1] = 0
        if 'add_AGC' in config:
            if config['add_AGC']:
                self.feature_switch[2] = 1
            else:
                self.feature_switch[2] = 0
        if 'add_HPF' in config:
            if config['add_HPF']:
                self.feature_switch[3] = 1
            else:
                self.feature_switch[3] = 0
        if 'add_MVDR' in config:
            if config['add_MVDR']:
                self.feature_switch[4] = 1
            else:
                self.feature_switch[4] = 0
        if 'mic_num' in config:
            self.mic_num = config['mic_num']
        if 'ref_num' in config:
            self.ref_num = config['ref_num']

        conf = []
        for s in self.feature_switch:
            if s == 1:
                conf.append('True')
            else:
                conf.append('False')


        print('#################################################')
        print('The configurations are: add_AEC: {}, add_NS: {}, add_AGC: {}, add_HPF: {},'
              ' add_MVDR: {}'.format(conf[0], conf[1], conf[2], conf[3], conf[4]))
        print('The number of microphones is: ', self.mic_num)
        print('The number of reference channels is: ', self.ref_num)

        self.mic_coord = np.zeros(3 * self.mic_num, dtype=float)
        if self.feature_switch[4] == 1 and mic_coord is not None:
            for i in range(self.mic_num):
                self.mic_coord[i * 3] = mic_coord[i][0]
                self.mic_coord[i * 3 + 1] = mic_coord[i][1]
                self.mic_coord[i * 3 + 2] = mic_coord[i][2]
            print('The coordinates are: ', mic_coord)
        print('#################################################')

    def process(self, input_file, out_file, ref_file=None):
        argc = []
        if input_file:
            argc.append('-i'.encode())
            for file in input_file:
                argc.append(file.encode())
        if ref_file:
            argc.append('-r'.encode())
            for file in ref_file:
                argc.append(file.encode())
        if out_file:
            argc.append('-o'.encode())
            for file in out_file:
                argc.append(file.encode())
        dios_ssp_v1(argc, self.feature_switch, list(self.mic_coord), self.mic_num, self.ref_num)

def athena_signal_process(input_file, out_file, ref_file=None, config=None, mic_coord=None):
    """
    Do signal processing.
    :param input_file: List of input wav files.
    :param out_file: List of output wav files.
    :param ref_file: List of reference wav files.
    :param config: Parameters for signal processing.
    :return: None
    """
    athena_siganl_test = AthenaSignal()
    athena_siganl_test.set_params(config, mic_coord)
    athena_siganl_test.process(input_file, out_file, ref_file)