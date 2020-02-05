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

#ifndef _DIOS_SSP_NS_API_H_
#define _DIOS_SSP_NS_API_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "../dios_ssp_share/dios_ssp_share_rfft.h"
#include "dios_ssp_ns_macros.h"

/**********************************************************************************
Function:      // dios_ssp_ns_init_api
Description:   // init ns module
Input:         // frame_len: frame length
Output:        // none
Return:        // success: return ns module pointer
                  failure: return NULL
**********************************************************************************/
void* dios_ssp_ns_init_api(int frame_len);

/**********************************************************************************
Function:      // dios_ssp_ns_reset_api
Description:   // reset ns module
Input:         // ptr: ns module pointer
Output:        // none
Return:        // success: return 0, failure: return ERROR_NS
**********************************************************************************/
int dios_ssp_ns_reset_api(void* ptr);

/**********************************************************************************
Function:      // dios_ssp_ns_process
Description:   // ns process using MCRA
Input:         // ptr: ns module pointer
                  in_data: input and output data
Output:        // 
Return:        // success: return 0, failure: return ERROR_NS
**********************************************************************************/
int dios_ssp_ns_process(void *ptr, float *in_data);

/**********************************************************************************
Function:      // dios_ssp_ns_uninit_api
Description:   // free ns module
Input:         // ptr: ns module pointer
Output:        // none
Return:        // success: return 0, failure: return ERROR_NS
**********************************************************************************/
int dios_ssp_ns_uninit_api(void* ptr);

#endif  /* _DIOS_SSP_NS_API_H_ */

