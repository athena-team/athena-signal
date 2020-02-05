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

#ifndef _DIOS_SSP_MVDR_API_H_
#define _DIOS_SSP_MVDR_API_H_

#include "../dios_ssp_return_defs.h"
#include "dios_ssp_mvdr_header.h"

/**********************************************************************************
Function:      // dios_ssp_mvdr_init_api
Description:   // mvdr init
Input:         // mic_num: microphone number
				  mic_coord: each microphone coordinate (PlaneCoord*)mic_coord
Output:        // none
Return:        // success: return mvdr object pointer (void*)ptr_mvdr
                  failure: return NULL
**********************************************************************************/
void* dios_ssp_mvdr_init_api(int mic_num, void* mic_coord);

/**********************************************************************************
Function:      // dios_ssp_mvdr_reset_api
Description:   // mvdr reset
Input:         // ptr: mvdr object pointer
Output:        // none
Return:        // success: return 0, failure: return ERROR_MVDR
**********************************************************************************/
int dios_ssp_mvdr_reset_api(void *ptr);

/**********************************************************************************
Function:      // dios_ssp_mvdr_process_api
Description:   // mvdr process
Input:         // ptr: mvdr object pointer
                  mic_data: mvdr input data, data type is float
                  loc_phi: direction of wakeup
Output:        // out_data: mvdr output signal
Return:        // success: return 0, failure: return ERROR_MVDR
**********************************************************************************/
int dios_ssp_mvdr_process_api(void* ptr, float* mic_data, float* out_data, float loc_phi);

/**********************************************************************************
Function:      // dios_ssp_mvdr_uninit_api
Description:   // mvdr delete
Input:         // ptr: mvdr object pointer
Output:        // none
Return:        // success: return 0, failure: return ERROR_MVDR
**********************************************************************************/
int dios_ssp_mvdr_uninit_api(void *ptr);

#endif /* _DIOS_SSP_MVDR_API_H_ */


