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

#ifndef _DIOS_SSP_DOA_API_H_
#define _DIOS_SSP_DOA_API_H_

#include <stdio.h>
#include "../dios_ssp_return_defs.h"
#include "dios_ssp_doa_macros.h"
#include "dios_ssp_doa_win.h" 
#include "../dios_ssp_share/dios_ssp_share_typedefs.h"
#include "../dios_ssp_share/dios_ssp_share_rfft.h"
#include "../dios_ssp_share/dios_ssp_share_cinv.h"

typedef struct
{
	int		m_fs;
	float	m_eps;
	int		m_channels;
	int		m_delta_angle;
	float	m_deta_fs;
	int		m_fft_size;
	int		m_sp_size;
	int		m_shift_size;
	int		m_angle_num;
    float	m_angle_smooth;
    int		m_frq_bin_num;
    float	m_low_frq;
	float	m_high_frq;
	int		m_frq_sp;
    float	*m_capon_spectrum;
    int     *m_doa_fid;
    int		m_low_fid;
    float	*m_irxx_re;
	float	*m_irxx_im;
    float	*m_vec_re;
	float	*m_vec_im;
    float	*m_gstv_re;
	float	*m_gstv_im;
    int		m_frq_bin_width;
    // rxx  
	int		m_rxx_size;
    float	*m_rxx_in;
    float	*m_irxx_out;
    float	*m_rxx_re;
	float	*m_rxx_im;
	float	*m_re;
	float	*m_im;
	float	*m_win_data;
	float	*fft_out;	
    short	m_first_frame_flag;
	float	m_beta_rxx;	
	float	m_alpha_rxx;
	int		m_gstv_dim;
	PlaneCoord *cood;
	void *doainv;
    objDOACwin *doawin;
	void *doa_fft;
	float	**m_mch_buffer;	
}objDOA;

/**********************************************************************************
Function:      // dios_ssp_doa_init_api
Description:   // doa init
Input:         // sensor_num: microphone number
				  cood: micphone coordinate
Output:        // none
Return:        // success: return doa object pointer (void*)ptr_doa
				  failure: return NULL
**********************************************************************************/
void* dios_ssp_doa_init_api(int mic_num, PlaneCoord* mic_coord);

/**********************************************************************************
Function:      // dios_ssp_doa_reset_api
Description:   // doa reset
Input:         // ptr
Output:        // none
Return:        // success: return 0
**********************************************************************************/
int dios_ssp_doa_reset_api(void *ptr);

/**********************************************************************************
Function:      // dios_ssp_doa_process_api
Description:   // doa process
Input:         // ptr
			   // in: microphone data
			   // vad_result: vad result
			   // dt_st: double talk result
Output:        // none
Return:        // success: return doa result
**********************************************************************************/
float dios_ssp_doa_process_api(void* ptr, float* in, int vad_result, int dt_st);

/**********************************************************************************
Function:      // dios_ssp_doa_uninit_api
Description:   // doa free
Input:         // ptr
Output:        // none
Return:        // success: return 0
**********************************************************************************/
int dios_ssp_doa_uninit_api(void *ptr);

#endif /* _DIOS_SSP_DOA_API_H_ */
