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

#ifndef _DIOS_SSP_AEC_COMMON_H_
#define _DIOS_SSP_AEC_COMMON_H_

#include <stdlib.h>
#include <string.h>
#include "dios_ssp_aec_macros.h"
#include "../dios_ssp_share/dios_ssp_share_complex_defs.h"
	
/**********************************************************************************
Function:         // dios_ssp_aec_average_track
Description:      // calculate current frame average value
Input:            // input_time: the pointer of input signal in time domain
	                 frm_len: frame length
Output:           // ret_avg: the pointer of the input_time average value
Return:           // Success: return 0, Failure : return ERR_AEC
**********************************************************************************/
int dios_ssp_aec_average_track(float *input_time, int frm_len, float *ret_avg);

#endif /* _DIOS_SSP_AEC_COMMON_H_ */

