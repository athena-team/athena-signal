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

import argparse
import json
import os
import soundfile as sf
from tqdm import tqdm


def preprocess_one_dir(in_dir, out_dir, out_filename):
    """ Create .json file for one condition."""
    file_infos = []
    in_dir = os.path.abspath(in_dir)
    wav_list = os.listdir(in_dir)
    wav_list.sort()
    for wav_file in tqdm(wav_list):
        if not wav_file.endswith('.wav'):
            continue
        wav_path = os.path.join(in_dir, wav_file)
        samples = sf.SoundFile(wav_path)
        file_infos.append((wav_path, len(samples)))
    if not os.path.exists(out_dir):
        os.makedirs(out_dir)
    with open(os.path.join(out_dir, out_filename + '.json'), 'w') as f:
        json.dump(file_infos, f, indent=4)


def preprocess(inp_args):
    """ Create .json files for all conditions."""
    speaker_list = ['tts_16k_10s']
    # for data_type in ['tr', 'cv']:
        # for spk in speaker_list:
        #     preprocess_one_dir(os.path.join(inp_args.in_dir, data_type, spk),
        #                        os.path.join(inp_args.out_dir, data_type),
        #                        spk)

    for spk in speaker_list:
        preprocess_one_dir(os.path.join(inp_args.in_dir, spk),
                            os.path.join(inp_args.out_dir),
                            spk)


if __name__ == "__main__":
    parser = argparse.ArgumentParser("WHAM data preprocessing")
    parser.add_argument('--in_dir', type=str, default=None,
                        help='Directory path of wham including tr, cv and tt')
    parser.add_argument('--out_dir', type=str, default=None,
                        help='Directory path to put output files')
    args = parser.parse_args()
    print(args)
    preprocess(args)
