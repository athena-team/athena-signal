# Copyright (C) 2017 Beijing Didi Infinity Technology and Development Co.,Ltd.
# All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# 	http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================
"""This model tests API of athena_signal."""


from athena_signal.dios_ssp_api import athena_signal_process

if __name__ == "__main__":

    # Test AEC
    input_file = ["examples/0841-0875_env7_sit1_male_in.pcm"]
    ref_file = ["examples/0841-0875_env7_sit1_male_ref.pcm"]
    out_file = ["examples/0841-0875_env7_sit1_male_out.pcm"]
    config = {'add_AEC': 1, 'add_BF': 0}
    athena_signal_process(input_file, out_file, ref_file, config)

    # Test BF
    input_file = ["examples/m0f60_5cm_1_mix.pcm",
                  "examples/m0f60_5cm_2_mix.pcm",
                  "examples/m0f60_5cm_3_mix.pcm",
                  "examples/m0f60_5cm_4_mix.pcm",
                  "examples/m0f60_5cm_5_mix.pcm",
                  "examples/m0f60_5cm_6_mix.pcm"]
    out_file = ["examples/m0f60_5cm_bf_out.pcm"]
    config = {'add_AEC': 0, 'add_BF': 2, 'add_DOA': 0, 'mic_num': 6, 'ref_num': 0}
    mic_coord = [[0.05, 0.0, 0.0],
                 [0.025, 0.0433, 0.0],
                 [-0.025, 0.0433, 0.0],
                 [-0.05, 0.0, 0.0],
                 [-0.025, -0.0433, 0.0],
                 [0.025, -0.0433, 0.0]]
    athena_signal_process(input_file, out_file, config=config, mic_coord=mic_coord)
