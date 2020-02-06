/* Copyright (C) 2017 Beijing Didi Infinity Technology and Development Co.,Ltd.
All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Description: To estimate the background noise level the following step are 
applied:
 1. Clipping the input energy into [max_noise_energy, min_noise_energy]
 2. Running minimum (an approximation of the minimum over a shifting window of 
 min_win_len values)
 3. A linear smoothing as the output of the running min is staircase shaped
==============================================================================*/

#include "dios_ssp_share_noiselevel.h"
   
int dios_ssp_share_noiselevel_init(objNoiseLevel* srv, float max_noise_energy, float min_noise_energy, int min_win_len)
{
    float smooth_factor = 0.2f;
	if (NULL == srv)
	{
		return -1;
	}

    srv->smoothfactor_fisrt = smooth_factor;
    srv->min_noise_energy = min_noise_energy;
    srv->max_noise_energy = max_noise_energy;
    srv->min_energy = max_noise_energy;
    srv->temp_min = max_noise_energy;
    srv->min_win_len = min_win_len * 2; 
    srv->min_hold_frame = 0;
    srv->noise_level_first = min_noise_energy;

    srv->smoothfactor_second = smooth_factor;
    srv->min_noise_energy_second = min_noise_energy;
    srv->max_noise_energy_second = max_noise_energy;
    srv->min_energy_second = max_noise_energy;
    srv->temp_min_second = max_noise_energy;
    srv->min_win_len_second  = min_win_len;
    srv->min_hold_frame_second = 0;
    srv->noise_level_second = min_noise_energy;

    srv->min_energy_delay = max_noise_energy;
    srv->noise_change_frame = 0;
    srv->noise_change_flag = 0;
    srv->noise_change_counter = 0;
    srv->noise_change_update_flag = 0;

	return 0;
}

int dios_ssp_share_noiselevel_process(objNoiseLevel* srv, float in_energy)
{
    int vad = 0;

    if (in_energy < srv->min_energy_second) 
    {
        srv->min_energy_delay = srv->min_energy_second;
        srv->min_energy_second = in_energy;
        srv->min_hold_frame_second = 0;
        srv->temp_min_second = srv->max_noise_energy_second;
    } 
    else 
    {
        srv->min_hold_frame_second++;
    }

    if (srv->min_hold_frame_second > (srv->min_win_len_second >> 1) && in_energy < srv->temp_min_second) 
    {
        srv->temp_min_second = in_energy;
    }

    if (srv->min_hold_frame_second > ((3 * srv->min_win_len_second) >> 1)) 
    {
        srv->min_energy_delay = srv->min_energy_second;
        srv->min_energy_second = srv->temp_min_second;
        srv->temp_min_second = srv->max_noise_energy_second;
        srv->min_hold_frame_second = (srv->min_win_len_second >> 1);
    }

	srv->noise_level_second += srv->smoothfactor_second * (srv->min_energy_second - srv->noise_level_second);	

    if ((srv->min_energy_second > 2 * srv->min_energy_delay || srv->min_energy_delay > 2 * srv->min_energy_second) 
		    && srv->noise_change_flag == 0) 
    {
        srv->noise_change_flag = 1;
        srv->noise_change_frame = 0;
    } 
    if (srv->noise_change_flag == 1 && in_energy < 10 * srv->min_energy_second) 
    {
        srv->noise_change_counter++;
        srv->noise_change_update_flag = 1;
    } 
    else 
    {
        srv->noise_change_update_flag = 0;
    }
    if (srv->noise_change_counter >= 9) 
    {
        srv->noise_change_update_flag = 0;
    }
    srv->noise_change_frame++;
    if (srv->noise_change_frame > srv->min_win_len_second) 
    {
        srv->noise_change_counter = 0;
        srv->noise_change_flag = 0;
        srv->noise_change_frame = 0;
        srv->noise_change_update_flag = 0;
    }
    if (in_energy < 10.F * srv->noise_level_second)  /* If low enough energy,update second noise estimator */
    {
        if (in_energy < srv->min_noise_energy_second) 
		{
            in_energy = srv->min_noise_energy;
        }

        if (in_energy < srv->min_energy) 
		{
            srv->min_energy = in_energy;
            srv->min_hold_frame = 0;
            srv->temp_min = srv->max_noise_energy;
        } 
		else 
		{
            srv->min_hold_frame++;
        }

        if (srv->min_hold_frame > (srv->min_win_len >> 1) && in_energy < srv->temp_min) 
		{
            srv->temp_min = in_energy;
        }

        if (srv->min_hold_frame > ((3 * srv->min_win_len) >> 1)) 
		{
            srv->min_energy = srv->temp_min;
            srv->temp_min = srv->max_noise_energy;
            srv->min_hold_frame = (srv->min_win_len >> 1);
        }

	    srv->noise_level_first += srv->smoothfactor_fisrt * (srv->min_energy - srv->noise_level_first);
    }
    if ((in_energy > 20.0f * srv->noise_level_second) && in_energy > 20.0f * srv->noise_level_first) 
    {
        vad = 1;
    } 
    else 
    {
        vad = 0;
    }
    return(vad);
}

