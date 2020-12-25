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

import torch
import torch.nn as nn
import math
import os

def check_parameters(net):
    '''
        Returns module parameters. Mb
    '''
    parameters = sum(param.numel() for param in net.parameters())
    return parameters / 10**6
    
def save_checkpoint(epoch, checkpoint_path, mymodel, optimizer, day_time):
    '''
       save model
       best: the best model
    '''
    os.makedirs(checkpoint_path, exist_ok=True)
    file_path = os.path.join(checkpoint_path, '{}_epoch{}.pth.tar'.format(day_time, epoch))

    torch.save({
        'epoch': epoch,
        'model_state_dict': mymodel.state_dict(),
        'optim_state_dict': optimizer.state_dict()
    },
        file_path)
    print('Saving checkpoint model to %s' % file_path)

def make_optimizer(params, opt):
    optimizer = getattr(torch.optim, opt['optim']['name'])
    if opt['optim']['name'] == 'Adam':
        optimizer = optimizer(params, lr=opt['optim']['lr'], weight_decay=opt['optim']['weight_decay'])
    else:
        optimizer = optimizer(params, lr=opt['optim']['lr'], weight_decay=opt['optim']
        ['weight_decay'], momentum=opt['optim']['momentum'])

    return optimizer
