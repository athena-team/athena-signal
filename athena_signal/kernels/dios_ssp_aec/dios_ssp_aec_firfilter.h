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

#ifndef _DIOS_SSP_AEC_FIRFILTER_H_
#define _DIOS_SSP_AEC_FIRFILTER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dios_ssp_aec_macros.h"
#include "../dios_ssp_share/dios_ssp_share_complex_defs.h"
#include "../dios_ssp_share/dios_ssp_share_noiselevel.h"

/* fir filter struct define */
typedef struct {
	int ref_num;
	int far_end_talk_holdtime;
	float myu;
	float beta;
	xcomplex** sig_spk_ref;
	xcomplex* sig_mic_rec; // data input for filter x, y
	float* err_out;        // filter result error output
	int* num_main_subband_adf;
	float* lambda;
	float* weight;
	xcomplex*** fir_coef;
	xcomplex*** adf_coef;
	xcomplex*** stack_sigIn_adf;
	xcomplex* err_adf;
	xcomplex* err_fir;
	xcomplex** est_ref_adf;
	xcomplex** est_ref_fir;
	float* mse_main;  //err_fir mse
	float* mse_adpt;  //err_adf mse
	float* mse_mic_in; // mse of mic input singal
	float** power_in_ntaps_smooth;
	float* mic_rec_psd;
	float *energy_err_fir;
	float *energy_err_adf;
	float* power_mic_send_smooth;
	float** power_echo_rtn_smooth;
	float** ref_psd;
	float** power_echo_rtn_fir;
	float** power_echo_rtn_adpt;
	// ERL estimate
	int** band_table;
	float* spk_part_band_energy;
	float* echo_return_band_energy;
	float* mic_rec_part_band_energy;
	float* mic_send_part_band_energy;
	float** spk_peak;
	float** mic_peak;
	float** erl_ratio;
	// noise level estimate
	objNoiseLevel** noise_est_spk_t; // noise level for reference signal in time domain
	objNoiseLevel*** noise_est_spk_part; // partial group band
	objNoiseLevel** noise_est_mic_chan; // subband
	int adjust_flag;
	// double talk detection
	int* dt_status;
}objFirFilter;

/**********************************************************************************
Function:      // dios_ssp_aec_firfilter_init
Description:   // load configure file and allocate memory
Input:         // ref_num: reference number
Output:        // none
Return:        // success: return dios speech signal process aec firfilter pointer
	              failure: return NULL
**********************************************************************************/
objFirFilter* dios_ssp_aec_firfilter_init(int ref_num);

/**********************************************************************************
Function:      // dios_ssp_aec_firfilter_reset
Description:   // reset dios speech signal process aec firfilter module
Input:         // ptr: dios speech signal process aec firfilter pointer
Output:        // none
Return:        // success: return 0, failure: return ERR_AEC
**********************************************************************************/
int dios_ssp_aec_firfilter_reset(objFirFilter* ptr);
	
/**********************************************************************************
Function:      // dios_ssp_aec_firfilter_process
Description:   // run dios speech signal process aec firfilter module by frames
Input:         // ptr: dios speech signal process aec firfilter pointer
Output:        // output_buf: error signal output
	              est_echo: estimated echo signal output
Return:        // success: return 0, failure: return ERR_AEC
**********************************************************************************/
int dios_ssp_aec_firfilter_process(objFirFilter* ptr, xcomplex* output_buf, xcomplex* est_echo);

/**********************************************************************************
Function:      // dios_ssp_aec_firfilter_uninit
Description:   // free dios speech signal process aec firfilter module
Input:         // srv: dios speech signal process aec firfilter pointer
Output:        // none
Return:        // success: return 0, failure: return ERR_AEC
**********************************************************************************/
int dios_ssp_aec_firfilter_uninit(objFirFilter* ptr);

#endif /* _DIOS_SSP_AEC_FIRFILTER_H_ */

