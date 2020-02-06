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
==============================================================================*/

#ifndef _DIOS_SSP_SHARE_NOISELEVEL_H_
#define _DIOS_SSP_SHARE_NOISELEVEL_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

typedef struct {

	// aec_ns_first noise estimator variables
	float smoothfactor_fisrt;     // final noise smooth rate
	float min_noise_energy;       // Min Noise in_energy
	float max_noise_energy;       // Max Noise in_energy
	float min_energy;      // Running minimun along a shifting window
	float temp_min;        // Temporary minimum
	int min_win_len;      // Running minimun window length
	int min_hold_frame;         // min age counter used to schedule min update
	float noise_level_first;     // Noise Level (return value)

	// second noise estimator variables
	float smoothfactor_second;     // final noise smooth rate
	float min_noise_energy_second;       // Min Noise in_energy
	float max_noise_energy_second;       // Max Noise in_energy
	float min_energy_second;      // Running minimun along a shifting window
	float temp_min_second;        // Temporary minimum
	int min_win_len_second;      // Running minimun window length
	int min_hold_frame_second;         // min age counter used to schedule min update
	float noise_level_second;     // Noise Level (return value)

	float min_energy_delay;
	int noise_change_frame;
	int noise_change_flag;						/* the frame jump flag*/
	int noise_change_counter;						/* count the jump frame number*/
	int noise_change_update_flag;
} objNoiseLevel;

/**********************************************************************************
Function:      // dios_ssp_share_noiselevel_init
Description:   // load configure file and allocate memory
Input:         // srv: dios speech signal process noise level pointer
	              max_noise_energy: max value of noise energy
				  min_noise_energy: min value of noise energy
				  min_win_len:min window length for noise level
Output:        // none
Return:        // success: return 0, failure: return -1
**********************************************************************************/
int dios_ssp_share_noiselevel_init(objNoiseLevel* srv, float max_noise_energy, float min_noise_energy, int min_win_len);

/**********************************************************************************
Function:      // dios_ssp_share_noiselevel_process
Description:   // run dios speech signal process noise level module
Input:         // srv: dios speech signal process noise level pointer
	              input_energy: input signal energy
Output:        // none
Return:        // vad result
**********************************************************************************/
int dios_ssp_share_noiselevel_process(objNoiseLevel* srv, float input_energy);

#endif /* _DIOS_SSP_SHARE_NOISELEVEL_H_ */

