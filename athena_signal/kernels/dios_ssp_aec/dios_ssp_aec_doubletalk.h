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

#ifndef _DIOS_SSP_AEC_DOUBLETALK_H_
#define _DIOS_SSP_AEC_DOUBLETALK_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dios_ssp_aec_macros.h"
#include "../dios_ssp_share/dios_ssp_share_complex_defs.h"
#include "../dios_ssp_share/dios_ssp_share_noiselevel.h"

typedef struct {
	xcomplex** sig_spk_ref;
	xcomplex* sig_mic_rec;
	int ref_num;
	float* res1_psd;
	float* res1_sum;
	float res1_eng_avg;
	float* res1_eng_avg_buf;
	float* res1_min_avg_buf;
	float* mic_noiselevel_sum;
	float** erl_ratio;
	int dt_num_hangover;
	int dt_cnt;
	int dt_frame_cnt;
	int dt_num_bands;
	int** doubletalk_band_table;
	int far_end_talk_holdtime;
	int dt_st;
	float dt_thr_factor;
	float dt_min_thr;
	objNoiseLevel** mic_noise_bin; // subband
}objDoubleTalk;

/**********************************************************************************
Function:      // dios_ssp_aec_doubletalk_init
Description:   // load configure file and allocate memory
Input:         // ref_num: reference number
Output:        // none
Return:        // success: return dios speech signal process aec doubletalk pointer
	              failure: return NULL
**********************************************************************************/
objDoubleTalk* dios_ssp_aec_doubletalk_init(int ref_num);

/**********************************************************************************
Function:      // dios_ssp_aec_doubletalk_reset
Description:   // reset dios speech signal process aec doubletalk module
Input:         // srv: dios speech signal process aec doubletalk pointer
Output:        // none
Return:        // success: return 0, failure: return ERR_AEC
**********************************************************************************/
int dios_ssp_aec_doubletalk_reset(objDoubleTalk* srv);

/**********************************************************************************
Function:      // dios_ssp_aec_doubletalk_process
Description:   // run dios speech signal process aec doubletalk module by frames
Input:         // srv: dios speech signal process aec doubletalk pointer
Output:        // dt_st: doubletalk status
Return:        // success: return 0, failure: return ERR_AEC
**********************************************************************************/
int dios_ssp_aec_doubletalk_process(objDoubleTalk* srv, int* dt_st);

/**********************************************************************************
Function:      // dios_ssp_aec_doubletalk_uninit
Description:   // free dios speech signal process aec doubletalk module
Input:         // srv: dios speech signal process aec doubletalk pointer
Output:        // none
Return:        // success: return 0, failure: return ERR_AEC
**********************************************************************************/
int dios_ssp_aec_doubletalk_uninit(objDoubleTalk* srv);

#endif /* _DIOS_SSP_AEC_DOUBLETALK_H_ */

