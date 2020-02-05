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

#ifndef _DIOS_SSP_SHARE_RFFT_H_
#define _DIOS_SSP_SHARE_RFFT_H_

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "../dios_ssp_aec/dios_ssp_aec_macros.h"
#include "./dios_ssp_share_complex_defs.h"

/**********************************************************************************
Function:      // dios_ssp_share_rfft_init
Description:   // rfft init
Input:         // fft_len: fft length
Output:        // none
Return:        // success: return dios speech signal process rfft pointer
	              failure: return NULL
**********************************************************************************/
void *dios_ssp_share_rfft_init(int fft_len);    

/**********************************************************************************
Function:      // dios_ssp_share_rfft_process
Description:   // run dios speech signal process rfft module by frames
Input:         // rfft_handle: dios speech signal process rfft pointer
	              inbuffer: input data in time domain, data type is float
Output:        // outbuffer: output data in frequency domain, data type is float
Return:        // success: return 0, failure: return -1
**********************************************************************************/
int dios_ssp_share_rfft_process(void *rfft_handle, float *inbuffer, float *outbuffer);

/**********************************************************************************
Function:      // dios_ssp_share_irfft_process
Description:   // run dios speech signal process irfft module by frames
Input:         // rfft_handle: dios speech signal process rfft pointer
	              inbuffer: input data in frequency domain, data type is float
Output:        // outbuffer: output data in time domain, data type is float
Return:        // success: return 0, failure: return -1
**********************************************************************************/
int dios_ssp_share_irfft_process(void *rfft_handle, float *inbuffer, float *outbuffer);

/**********************************************************************************
Function:      // dios_ssp_share_rfft_uninit
Description:   // free dios speech signal process rfft module
Input:         // rfft_handle: dios speech signal process rfft pointer
Output:        // none
Return:        // success: return 0, failure: return -1
**********************************************************************************/
int dios_ssp_share_rfft_uninit(void *rfft_handle);

#endif  /* _DIOS_SSP_SHARE_RFFT_H_ */

