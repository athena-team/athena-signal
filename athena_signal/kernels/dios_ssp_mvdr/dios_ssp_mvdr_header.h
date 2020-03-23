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

#ifndef _DIOS_SSP_MVDR_HEADER_H_
#define _DIOS_SSP_MVDR_HEADER_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "dios_ssp_mvdr_macros.h"
#include "dios_ssp_mvdr_win.h"
#include "../dios_ssp_share/dios_ssp_share_typedefs.h"
#include "../dios_ssp_share/dios_ssp_share_rfft.h"
#include "../dios_ssp_share/dios_ssp_share_cinv.h"

typedef struct
{
	int		m_fs;
	int		m_channels;
	int		m_fft_size;
	int		m_shift_size;
	int		m_delta_angle;
	float	m_sd_factor;
	float	m_sd_eps;
    float	m_rnn_eps;
	float	m_alpha_rnn;
    float   m_alpha_s;
    float   m_alpha_p;
    float   m_alpha_d;
    int     m_L;
    float   m_delta_thres;
	float	m_deta_fs;
	int		m_sp_size;
	int		m_angle_num;
	float	m_beta_rnn;

	// frame index
    int		m_frame_sum;

	// buffer
	float	**m_mch_buffer;
	float	*m_win_data;
	float	*m_re;
	float	*m_im;
	float	*m_re_temp;
	float	*m_im_temp;
	
	// steering
	int		m_gstv_dim;
	float	*m_gstv_re;
	float	*m_gstv_im;
	float	*m_stv_re;
	float	*m_stv_im;
	
	// rxx  
	int		m_rxx_size;
	float	*m_rxx_in;
	float	*m_irxx_out;

	// rnn
	float	*m_rnn_re;
	float	*m_rnn_im;
	float	*m_irnn_re;
	float	*m_irnn_im;
	
	// capon spectrum
	int		m_angle_pre;

	// sd rnn 	
	float	*m_sd_rnn_re;		
	float	*m_sd_irnn_re;	
	float	*m_sd_irnn_im;
	
	// mvdr
	float	*m_weight_sd_group_re;
	float   *m_weight_sd_group_im;

	// mcra
	float	*m_ns_ps_cur_mic;
	float	*m_ns_theta_cur_mic;
	float	*m_ns_ps;
	float	*m_P;
	float	*m_Ptmp;
	float	*m_Pmin;
	float	*m_pk;
	float	*m_xn_re;
	float	*m_xn_im;

	float	*m_weight_re;	
	float	*m_weight_im;
	
	float	*m_mvdr_out_re;
	float	*m_mvdr_out_im;	
	float	*m_out_bf;	

	PlaneCoord *cood;
	objMVDRCwin *mvdrwin;
	void *mvdrinv;
	void *mvdr_fft;
	float *fft_in;
	float *fft_out;
	float* dist;
}objMVDR;

/**********************************************************************************
Function:      // dios_ssp_mvdr_init
Description:   // mvdr init
Input:         // ptr_mvdr:
				  sensor_num: microphone number
				  cood: micphone coordinate
Output:        // none
Return:        // success: return mvdr object pointer (void*)ptr_mvdr
				  failure: return NULL
**********************************************************************************/
void dios_ssp_mvdr_init(objMVDR *ptr_mvdr, int sensor_num, PlaneCoord* cood);

/**********************************************************************************
Function:      // dios_ssp_mvdr_reset
Description:   // mvdr reset
Input:         // ptr_mvdr:
Output:        // none
Return:        // success: return mvdr object pointer (void*)ptr_mvdr
				  failure: return NULL
**********************************************************************************/
void dios_ssp_mvdr_reset(objMVDR *ptr_mvdr);

/**********************************************************************************
Function:      // dios_ssp_mvdr_process
Description:   // mvdr process
Input:         // ptr_mvdr:
				  in: microphone data
				  angle: micphone sound source angle
Output:        // out: mvdr process result
**********************************************************************************/
int dios_ssp_mvdr_process(objMVDR *ptr_mvdr, float* in, float* out, int angle);

/**********************************************************************************
Function:      // dios_ssp_mvdr_mcra
Description:   // mcra process
Input:         // ptr_mvdr:
Output:        // none
Return:        // success: return 0
				  failure: return NULL
**********************************************************************************/
int dios_ssp_mvdr_mcra(objMVDR *ptr_mvdr);

/**********************************************************************************
Function:      // dios_ssp_mvdr_cal_rxx
Description:   // calculate Rxx
Input:         // ptr_mvdr:
Output:        // none
Return:        // success: return 0
				  failure: return NULL
**********************************************************************************/
int dios_ssp_mvdr_cal_rxx(objMVDR *ptr_mvdr);

/**********************************************************************************
Function:      // dios_ssp_mvdr_cal_weights_adpmvdr
Description:   // calculate mvdr adaptive weight
Input:         // ptr_mvdr:
Output:        // none
Return:        // success: return 0
				  failure: return NULL
**********************************************************************************/
int dios_ssp_mvdr_cal_weights_adpmvdr(objMVDR *ptr_mvdr);

/**********************************************************************************
Function:      // dios_ssp_mvdr_delete
Description:   // free ptr_mvdr
Input:         // ptr_mvdr
Output:        // none
Return:        // success: return 0
**********************************************************************************/
void dios_ssp_mvdr_delete(objMVDR *ptr_mvdr);

#endif /* _DIOS_SSP_MVDR_HEADER_H_ */

