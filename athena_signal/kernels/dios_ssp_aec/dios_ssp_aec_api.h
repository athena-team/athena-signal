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

#ifndef _DIOS_SSP_AEC_API_H_
#define _DIOS_SSP_AEC_API_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dios_ssp_aec_common.h"
#include "dios_ssp_aec_firfilter.h"
#include "dios_ssp_aec_doubletalk.h"
#include "dios_ssp_aec_erl_est.h"
#include "dios_ssp_aec_res.h"
#include "./dios_ssp_aec_tde/dios_ssp_aec_tde.h"
#include "../dios_ssp_share/dios_ssp_share_subband.h"
#include "../dios_ssp_share/dios_ssp_share_complex_defs.h"

/**********************************************************************************
Function:      // dios_ssp_aec_init_api
Description:   // load configure file and allocate memory
Input:         // mic_num: microphone number
	              ref_num: reference number
				  frm_len: frame length
Output:        // none
Return:        // success: return dios speech signal process aec pointer
	              failure: return NULL
**********************************************************************************/
void* dios_ssp_aec_init_api(int mic_num, int ref_num, int frm_len);

/**********************************************************************************
Function:      // dios_ssp_aec_config_api
Description:   // config dios speech signal process aec module
Input:         // ptr: dios speech signal process aec pointer
	              mode: aec mode
Output:        // none
Return:        // success: return 0, failure: return ERR_AEC
**********************************************************************************/
int dios_ssp_aec_config_api(void* ptr, int mode);

/**********************************************************************************
Function:      // dios_ssp_aec_reset_api
Description:   // reset dios speech signal process aec module
Input:         // ptr: dios speech signal process aec pointer
Output:        // none
Return:        // success: return 0, failure: return ERR_AEC
**********************************************************************************/
int dios_ssp_aec_reset_api(void* ptr);

/**********************************************************************************
Function:      // dios_ssp_aec_process_api
Description:   // run dios speech signal process aec module by frames
Input:         // ptr: dios speech signal process aec pointer
	              io_buf: microphone array data input/output buffer, data type is float
					      size is[M][N], M = mic number * data_len
	              ref_buf: reference data buffer, data type is float
					       size is[M][N], M = reference number * data_len
	              dt_st: double talk status
Output:        // io_buf
Return:        // success: return 0, failure: return ERR_AEC
**********************************************************************************/
int dios_ssp_aec_process_api(void* ptr, float* io_buf, float* ref_buf, int* dt_st);

/**********************************************************************************
Function:      // dios_ssp_aec_uninit_api
Description:   // free dios speech signal process aec module
Input:         // ptr: dios speech signal process aec pointer
Output:        // none
Return:        // success: return 0, failure: return ERR_AEC
**********************************************************************************/
int dios_ssp_aec_uninit_api(void* ptr);

#endif /* _DIOS_SSP_AEC_API_H_ */

