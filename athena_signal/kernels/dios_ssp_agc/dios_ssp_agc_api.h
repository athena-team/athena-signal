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

#ifndef _DIOS_SSP_AGC_API_H_
#define _DIOS_SSP_AGC_API_H_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../dios_ssp_return_defs.h"
#include "../dios_ssp_share/dios_ssp_share_complex_defs.h"

/**********************************************************************************
Function:      // dios_ssp_agc_init_api
Description:   // init agc module
Input:         // frame_len: frame length
                  peak_val: agc peak value
                  mode_type: agc mode type 
Output:        // none
Return:        // success: return agc module pointer
                  failure: return NULL
**********************************************************************************/
void* dios_ssp_agc_init_api(int frame_len, float peak_val, int mode_type);

/**********************************************************************************
Function:      // dios_ssp_agc_reset_api
Description:   // reset agc module
Input:         // ptr: agc module pointer
Output:        // none
Return:        // success: return 0, failure: return ERROR_AGC
**********************************************************************************/
int dios_ssp_agc_reset_api(void* ptr);

/**********************************************************************************
Function:      // dios_ssp_agc_process_api
Description:   // agc process
Input:         // ptr: agc module pointer
                  io_buf: input and output buffer
                  vad_sig:
                  vad_dir:
                  dt_st: doubletalk state, if any
Output:        // none
Return:        // success: return 0, failure: return ERROR_AGC
**********************************************************************************/
int dios_ssp_agc_process_api(void* ptr, float* io_buf, 
            int vad_sig, int vad_dir, int dt_st);

/**********************************************************************************
Function:      // dios_ssp_agc_uninit_api
Description:   // free agc module
Input:         // ptr: agc module pointer
Output:        // none
Return:        // success: return 0, failure: return ERROR_AGC
**********************************************************************************/
int dios_ssp_agc_uninit_api(void* ptr);

#endif  /* _DIOS_SSP_AGC_API_H_ */

