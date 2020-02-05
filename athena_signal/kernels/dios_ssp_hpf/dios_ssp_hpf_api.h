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

#ifndef _DIOS_SSP_HPF_API_H_
#define _DIOS_SSP_HPF_API_H_

#include <stdlib.h>

/**********************************************************************************
Function:      // dios_ssp_hpf_init_api
Description:   // init hpf module
Input:         // none
Output:        // none
Return:        // success: return hpf module pointer
                  failure: return NULL
**********************************************************************************/
void* dios_ssp_hpf_init_api(void);

/**********************************************************************************
Function:      // dios_ssp_hpf_reset_api
Description:   // reset hpf module
Input:         // ptr: hpf module pointer
Output:        // none
Return:        // success: return 0, failure: return ERROR_HPF
**********************************************************************************/
int dios_ssp_hpf_reset_api(void* ptr);

/**********************************************************************************
Function:      // dios_ssp_hpf_process_api
Description:   // hpf process
Input:         // ptr: hpf module pointer
                  io_buf: input and output buffer
                  siglen: input signal length
Output:        // none
Return:        // success: return 0, failure: return ERROR_HPF
**********************************************************************************/
int dios_ssp_hpf_process_api(void* ptr, float* io_buf, int siglen);

/**********************************************************************************
Function:      // dios_ssp_hpf_uninit_api
Description:   // free hpf module
Input:         // ptr: hpf module pointer
Output:        // none
Return:        // success: return 0, failure: return ERROR_HPF
**********************************************************************************/
int dios_ssp_hpf_uninit_api(void* ptr);

#endif  /* _DIOS_SSP_HPF_API_H_ */

