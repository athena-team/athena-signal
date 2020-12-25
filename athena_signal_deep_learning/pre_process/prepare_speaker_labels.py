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

import numpy as np
import os
import pickle
import torch
import json

def handle_json(json_path):
    with open(json_path, 'r') as f:
        aux_infos = json.load(f)
    speakers = {}
    for aux in aux_infos:
        speaker_id = aux[0].split('/')[-1][0:3]
        if speaker_id not in speakers:
            speakers[speaker_id] = 1
        else:
            speakers[speaker_id] += 1
    speakers_new = sorted(speakers.keys())
    num_class = len(speakers_new)
    speakers_labels = {}
    for i in range(len(speakers_new)):
        label = np.zeros(num_class)
        label[i] = 1
        speakers_labels[speakers_new[i]] = label.astype(np.float32)
    export_name = os.path.join('/nfs/cold_project/dengchengyun/AEC_Separation/IRA/dprnn_extraction_original/data/tt', 'tt_speaker')
    with open(export_name, 'wb') as fid:
        pickle.dump(speakers_labels, fid)

if __name__ == "__main__":

    handle_json('/nfs/cold_project/dengchengyun/AEC_Separation/IRA/dprnn_extraction_original/data/tt/aux.json')

    with open('/nfs/cold_project/dengchengyun/AEC_Separation/IRA/dprnn_extraction_original/data/tt/tt_speaker', 'rb') as fid:
        data = pickle.load(fid)
    for d in data:
        max_index = int(np.argmax(data[d]))
        print(data[d], max_index, type(max_index), torch.tensor(max_index).long())