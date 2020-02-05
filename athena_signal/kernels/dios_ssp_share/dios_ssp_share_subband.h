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

#ifndef _DIOS_SSP_SHARE_SUBBAND_H_
#define _DIOS_SSP_SHARE_SUBBAND_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dios_ssp_share_rfft.h"

typedef struct {
	int frm_len;
	int Ppf_tap; // WIN_LEN/FFT_LEN
	int Ppf_decm;
	int *p_in;
	int *p_h0;
	float scale;
	float *ana_xin;  // time domain data input for analyze
	xcomplex *ana_cxout;  // frequency domain complex output for analyze
	float *ana_xout;  // time domain output for analyze

	float *comp_in;  // time domain data input for compose
	float *comp_out; // data output for compose
	float* lpf_coef;
	void *rfft_param;
	float *fftout_buffer;
	float *fftin_buffer;
}objSubBand;

/**********************************************************************************
Function:      // dios_ssp_share_subband_init
Description:   // subband init
Input:         // frm_len: frame length
Output:        // none
Return:        // success: return dios speech signal process subband pointer
	              failure: return NULL
**********************************************************************************/
objSubBand* dios_ssp_share_subband_init(int frm_len);
	
/**********************************************************************************
Function:      // dios_ssp_share_subband_reset
Description:   // reset dios speech signal process subband module
Input:         // ptr: dios speech signal process subband pointer
Output:        // none
Return:        // success: return 0, failure: return -1
**********************************************************************************/
int dios_ssp_share_subband_reset(objSubBand* ptr);

/**********************************************************************************
Function:      // dios_ssp_share_subband_analyse
Description:   // run dios speech signal process subband analyse module by frames
Input:         // ptr: dios speech signal process subband pointer
	              in_buf: input data in time domain, data type is float
Output:        // out_buf: output data in frequency domain, data type is complex
Return:        // success: return 0, failure: return -1
**********************************************************************************/
int dios_ssp_share_subband_analyse(objSubBand* ptr, float* in_buf, xcomplex* out_buf);

/**********************************************************************************
Function:      // dios_ssp_share_subband_compose
Description:   // run dios speech signal process subband compose module by frames
Input:         // ptr: dios speech signal process subband pointer
	              in_buf: input data in frequency domain, data type is complex
Output:        // out_buf: output data in time domain, data type is float
Return:        // success: return 0, failure: return -1
**********************************************************************************/
int dios_ssp_share_subband_compose(objSubBand* ptr, xcomplex* in_buf, float* out_buf);

/**********************************************************************************
Function:      // dios_ssp_share_subband_uninit
Description:   // free dios speech signal process subband module
Input:         // srv: dios speech signal process subband pointer
Output:        // none
Return:        // success: return 0, failure: return -1
**********************************************************************************/
int dios_ssp_share_subband_uninit(objSubBand* ptr);

#endif /* _DIOS_SSP_SHARE_SUBBAND_H_ */

