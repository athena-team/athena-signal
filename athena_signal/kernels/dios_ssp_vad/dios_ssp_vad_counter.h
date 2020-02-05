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

#ifndef _DIOS_SSP_VAD_COUNTER_H_
#define _DIOS_SSP_VAD_COUNTER_H_

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**********************************************************************************
Function:      // dios_ssp_vad_counter_init
Description:   // init vad counter module
Input:         // none
Output:        // none
Return:        // success: return vad counter module pointer
                  failure: return NULL
**********************************************************************************/
void* dios_ssp_vad_counter_init(void);

/**********************************************************************************
Function:      // dios_ssp_vad_counter_reset
Description:   // reset vad counter module
Input:         // ptr: vad_counter_handle
Output:        // none
Return:        // success: return 0, failure: return -1
**********************************************************************************/
int dios_ssp_vad_counter_reset(void* vad_counter_handle);

/**********************************************************************************
Function:      // dios_ssp_vad_mix_process
Description:   // vad mix process
Input:         // ptr: apm_flag
				  dt_flag
				  vad_counter_handle
				  signaldev
				  vad_flag
				  state
Output:        // none
Return:        // success: return 0
                  failure: return NULL
**********************************************************************************/
int dios_ssp_vad_mix_process(int apm_flag, int dt_flag, void* vad_counter_handle, float signaldev, int* vad_flag, int state);

/**********************************************************************************
Function:      // dios_ssp_vad_mix_stric_process
Description:   // vad mix stric process
Input:         // ptr: apm_flag
				  dt_flag
				  vad_counter_handle
				  signaldev
				  vad_flag
				  state
Output:        // none
Return:        // success: return 0
                  failure: return NULL
**********************************************************************************/
int dios_ssp_vad_mix_stric_process(int apm_flag, int dt_flag, void* vad_counter_handle, float signaldev, int* vad_flag, int state);

/**********************************************************************************
Function:      // dios_ssp_vad_smooth
Description:   // vad smooth
Input:         // ptr: vad_flag
				  vad_counter_handle
				  vad_state
Output:        // none
Return:        // success: return NULL
                  failure: return NULL
**********************************************************************************/
void dios_ssp_vad_smooth(int* vad_flag, void* vad_counter_handle, int* vad_state);

/**********************************************************************************
Function:      // dios_ssp_vad_para_get_debug
Description:   // vad param get
Input:         // ptr: vad_counter_handle
				  vad_cnt
				  false_cnt
Output:        // none
Return:        // success: return NULL
                  failure: return NULL
**********************************************************************************/
void dios_ssp_vad_para_get_debug(void* vad_counter_handle, float *vad_cnt, float *false_cnt);

/**********************************************************************************
Function:      // dios_ssp_vad_counter_uinit
Description:   // free vad counter
Input:         // ptr: vad_counter_handle
Output:        // none
Return:        // success: return NULL
                  failure: return NULL
**********************************************************************************/
void dios_ssp_vad_counter_uinit(void* vad_counter_handle);

#endif

