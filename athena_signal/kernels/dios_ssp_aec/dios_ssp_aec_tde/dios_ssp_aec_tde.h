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

#ifndef _DIOS_SSP_AEC_TDE_H_
#define _DIOS_SSP_AEC_TDE_H_

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "dios_ssp_aec_tde_delay_estimator_wrapper.h"
#include "../dios_ssp_aec_macros.h"
#include "../../dios_ssp_share/dios_ssp_share_complex_defs.h"

typedef struct
{
	/* TDE */
	AecmCore_t *tde_short;
	AecmCore_t *tde_long;

	int flag_delayfind;

	/* Realign flag */
	unsigned short CalibrateEnable;
	int CalibrateCounter;
		
	int mic_num;
	int ref_num;
	int frm_len;

	/* buffer, loop number and data length definition */
	float *tdeBuf_ref;
	float *tdeBuf_mic;
	float **audioBuf_mic;
	float **audioBuf_ref; 
	int pt_buf_push;
	int pt_output;
	double delay_fixed_sec;
	double delay_varied_sec;
	int tde_long_shift_smpl;
	int tde_short_shift_smpl;
	int act_delay_smpl;
	int act_delay_smpl_old;
	int frame_cnt_nSecond;
	int vadflag_mic;
	int vadflag_ref;
}objTDE;

/**********************************************************************************
Function:      // dios_ssp_aec_tde_init
Description:   // load configure file and allocate memory
Input:         // mic_num: microphone number
				  ref_num: reference number
				  frm_len: frame length
Output:        // none
Return:        // success: return dios speech signal process aec time delay estimation(tde) pointer
	              failure: return NULL
**********************************************************************************/
objTDE* dios_ssp_aec_tde_init(int mic_num, int ref_num, int frm_len);

/**********************************************************************************
Function:      // dios_ssp_aec_tde_reset
Description:   // reset dios speech signal process aec tde module
Input:         // srv: dios speech signal process aec tde pointer
Output:        // none
Return:        // success: return 0, failure: return ERR_AEC
**********************************************************************************/
int dios_ssp_aec_tde_reset(objTDE* srv);

/**********************************************************************************
Function:      // dios_ssp_aec_tde_process
Description:   // run dios speech signal process aec tde module
Input:         // srv: dios speech signal process aec tde pointer
                  refbuf: reference data buffer
	              micbuf: microphone array data buffer	              
Output:        // none
Return:        // success: return 0, failure: return ERR_AEC
**********************************************************************************/
int dios_ssp_aec_tde_process(objTDE* srv, float* refbuf, float* micbuf);

/**********************************************************************************
Function:      // dios_ssp_aec_tde_uninit
Description:   // free dios speech signal process aec tde module
Input:         // srv: dios speech signal process aec tde pointer
Output:        // none
Return:        // success: return 0, failure: return ERR_AEC
**********************************************************************************/
int dios_ssp_aec_tde_uninit(objTDE* srv);

#endif /* _DIOS_SSP_AEC_TDE_H_ */

