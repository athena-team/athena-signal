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

#ifndef _DIOS_SSP_VAD_API_H_
#define _DIOS_SSP_VAD_API_H_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "dios_ssp_vad_energy.h"
#include "dios_ssp_vad_counter.h"
#include "../dios_ssp_return_defs.h"

/**********************************************************************************
Function:      // dios_ssp_vad_init_api
Description:   // init vad module
Input:         // none
Output:        // none
Return:        // success: return vad module pointer
                  failure: return NULL
**********************************************************************************/
void* dios_ssp_vad_init_api(void);

/**********************************************************************************
Function:      // dios_ssp_vad_reset_api
Description:   // reset vad module
Input:         // ptr: vad module pointer
Output:        // none
Return:        // success: return 0, failure: return ERROR_VAD
**********************************************************************************/
int dios_ssp_vad_reset_api(void* ptr);

/**********************************************************************************
Function:      // dios_ssp_vad_process_api
Description:   // vad process
Input:         // ptr: vad module pointer
                  vad_data: vad input data
                  dt_st: doubletalk status, if any
Output:        // none 
Return:        // success: return 0, failure: return ERROR_VAD
**********************************************************************************/
int dios_ssp_vad_process_api(void* ptr, float* vad_data, int dt_st);

/**********************************************************************************
Function:      // dios_ssp_vad_uninit_api
Description:   // free vad module
Input:         // ptr: vad module pointer
Output:        // none
Return:        // success: return 0, failure: return ERROR_VAD
**********************************************************************************/
int dios_ssp_vad_uninit_api(void* ptr);

/**********************************************************************************
Function:      // dios_ssp_vad_result_get
Description:   // get vad result
Input:         // ptr: vad module pointer
Output:        // none
Return:        // success: return vad result, failure: return ERROR_VAD
**********************************************************************************/
int dios_ssp_vad_result_get(void* ptr);

#endif  /* _DIOS_SSP_VAD_API_H_ */

