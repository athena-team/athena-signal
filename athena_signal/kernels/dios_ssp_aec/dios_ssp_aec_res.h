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

#ifndef _DIOS_SSP_AEC_RES_H_
#define _DIOS_SSP_AEC_RES_H_

#include <stdlib.h>
#include "dios_ssp_aec_macros.h"
#include "../dios_ssp_share/dios_ssp_share_complex_defs.h"

typedef struct
{
	xcomplex *Xf_res_echo;
	xcomplex *Xf_echo;
	int   ccsize;      // number of frequency bins
	int   nb_adapt;    // Number of frames used for adaptation
	float *echoPsd;
	float *res1_old_ps;
	float *res2_old_ps;
	float *Eh;         //Error avg
	float *Yh;         //Echo avg
	float *res1_echo_noise;
	float *res2_echo_noise;
	float *res1_zeta; 
	float *res2_zeta;
	float spec_average;
	float beta0;
	float beta_max;
	float Pey_avg;
	float Pyy_avg;
	float res1_echo_noise_factor;
	float res2_echo_noise_factor;
	float res1_echo_suppress_default;
	float res2_st_echo_suppress_default;
	float res2_dt_echo_suppress_default;
	float res1_echo_suppress_active_default;
	float res2_st_echo_suppress_active_default;
	float res2_dt_echo_suppress_active_default;
	float res1_suppress_factor;
	float res2_st_suppress_factor;
	float res2_dt_suppress_factor;

	float *res_echo_noise;
	float *res_zeta;
	float res_echo_noise_factor;
	float *res_echo_psd;
	float *res_old_ps;
}objRES;

/**********************************************************************************
Function:      // dios_ssp_aec_res_get_residual_echo
Description:   // compute spectrum of estimated residual echo for use in an echo post-filter
Input:         // srv: dios speech signal process aec res pointer
                  dt_st: double-talk detection result
				  stage: the stage of residual echo suppression
Output:        // residual_echo: residual echo output
Return:        // success: return 0, failure: return ERR_AEC
**********************************************************************************/
int dios_ssp_aec_res_get_residual_echo(objRES* srv, float* residual_echo, int dt_st, int stage);

/**********************************************************************************
Function:      // dios_ssp_aec_res_init
Description:   // load configure file and allocate memory
Input:         // none
Output:        // none
Return:        // success: return dios speech signal process aec res pointer
	              failure: return NULL
**********************************************************************************/
objRES* dios_ssp_aec_res_init(void);
	
/**********************************************************************************
Function:      // dios_ssp_aec_res_reset
Description:   // reset dios speech signal process aec res module
Input:         // srv: dios speech signal process aec res pointer
Output:        // none
Return:        // success: return 0, failure: return ERR_AEC
**********************************************************************************/
int dios_ssp_aec_res_reset(objRES *srv);

/**********************************************************************************
Function:      // dios_ssp_aec_res_process
Description:   // run dios speech signal process aec res module by frames
Input:         // srv: dios speech signal process aec res pointer
				  dt_st: double-talk detection result
				  stage: the stage of residual echo suppression
Output:        // none
Return:        // success: return 0, failure: return ERR_AEC
**********************************************************************************/
int dios_ssp_aec_res_process(objRES *srv, int dt_st, int stage);

/**********************************************************************************
Function:      // dios_ssp_aec_res_uninit
Description:   // free dios speech signal process aec res module
Input:         // srv: dios speech signal process aec res pointer
Output:        // none
Return:        // success: return 0, failure: return ERR_AEC
**********************************************************************************/
int dios_ssp_aec_res_unit(objRES *srv);

#endif /* _DIOS_SSP_AEC_RES_H_ */
