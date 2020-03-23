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

Description: This is a DOA (Direction Of Arrival) module which uses the Capon 
algorithm to get the direction of the sound source. The main function of the 
Capon algorithm is the Capon beamformer, also called MVDR. The Capon spectrum 
is estimated by using Rxx matrix and steering vector in frequency domain.
==============================================================================*/

#include "dios_ssp_doa_api.h"

int dios_ssp_doa_cal_rxx(objDOA *ptr_doa)
{
	float rxx_re = 0, rxx_im = 0;
	if ( ptr_doa->m_first_frame_flag == 1 )
	{
		ptr_doa->m_first_frame_flag = 0;
		for ( int k = 0; k < ptr_doa->m_sp_size; ++k )
		{
			for ( int i = 0; i < ptr_doa->m_channels; ++i )
			{
				rxx_re = ptr_doa->m_re[i*ptr_doa->m_fft_size+k]*ptr_doa->m_re[i*ptr_doa->m_fft_size+k] + ptr_doa->m_im[i*ptr_doa->m_fft_size+k]*ptr_doa->m_im[i*ptr_doa->m_fft_size+k] + ptr_doa->m_eps;
				ptr_doa->m_rxx_re[k*ptr_doa->m_rxx_size+i*ptr_doa->m_channels+i] = rxx_re;
				
				for ( int j = i+1; j < ptr_doa->m_channels; ++j )
				{
					rxx_re = ptr_doa->m_re[i*ptr_doa->m_fft_size+k]*ptr_doa->m_re[j*ptr_doa->m_fft_size+k] + ptr_doa->m_im[i*ptr_doa->m_fft_size+k]*ptr_doa->m_im[j*ptr_doa->m_fft_size+k];
					rxx_im = -ptr_doa->m_re[i*ptr_doa->m_fft_size+k]*ptr_doa->m_im[j*ptr_doa->m_fft_size+k] + ptr_doa->m_re[j*ptr_doa->m_fft_size+k]*ptr_doa->m_im[i*ptr_doa->m_fft_size+k];
					ptr_doa->m_rxx_re[k*ptr_doa->m_rxx_size+i*ptr_doa->m_channels+j] = rxx_re;
					ptr_doa->m_rxx_im[k*ptr_doa->m_rxx_size+i*ptr_doa->m_channels+j] = rxx_im;
				}
			}
		}
	}
	else
	{
		for ( int k = 0; k < ptr_doa->m_sp_size; ++k )
		{
			for ( int i = 0; i < ptr_doa->m_channels; ++i )
			{
				rxx_re = ptr_doa->m_re[i*ptr_doa->m_fft_size+k]*ptr_doa->m_re[i*ptr_doa->m_fft_size+k] + ptr_doa->m_im[i*ptr_doa->m_fft_size+k]*ptr_doa->m_im[i*ptr_doa->m_fft_size+k] + ptr_doa->m_eps;
				ptr_doa->m_rxx_re[k*ptr_doa->m_rxx_size+i*ptr_doa->m_channels+i] = ptr_doa->m_alpha_rxx*ptr_doa->m_rxx_re[k*ptr_doa->m_rxx_size+i*ptr_doa->m_channels+i] + ptr_doa->m_beta_rxx*rxx_re;
								
				for ( int j = i+1; j < ptr_doa->m_channels; ++j )
				{
					rxx_re = ptr_doa->m_re[i*ptr_doa->m_fft_size+k]*ptr_doa->m_re[j*ptr_doa->m_fft_size+k] + ptr_doa->m_im[i*ptr_doa->m_fft_size+k]*ptr_doa->m_im[j*ptr_doa->m_fft_size+k];
					rxx_im = -ptr_doa->m_re[i*ptr_doa->m_fft_size+k]*ptr_doa->m_im[j*ptr_doa->m_fft_size+k] + ptr_doa->m_re[j*ptr_doa->m_fft_size+k]*ptr_doa->m_im[i*ptr_doa->m_fft_size+k];
					ptr_doa->m_rxx_re[k*ptr_doa->m_rxx_size+i*ptr_doa->m_channels+j] = ptr_doa->m_alpha_rxx*ptr_doa->m_rxx_re[k*ptr_doa->m_rxx_size+i*ptr_doa->m_channels+j] + ptr_doa->m_beta_rxx*rxx_re;
					ptr_doa->m_rxx_im[k*ptr_doa->m_rxx_size+i*ptr_doa->m_channels+j] = ptr_doa->m_alpha_rxx*ptr_doa->m_rxx_im[k*ptr_doa->m_rxx_size+i*ptr_doa->m_channels+j] + ptr_doa->m_beta_rxx*rxx_im;										
				}	
			}
		}
	}

	return 0;
}

int dios_ssp_doa_init_steering_vectors_g(objDOA *ptr_doa)
{
	float deta = 0.0f;
	float theta = 0.0f;
	float phi = PI * 0.5f;
	float omega = 0.0f;
    int i, j, k;

	for (i = 0; i < ptr_doa->m_angle_num; ++i )
	{
		theta = (float)i * (float)ptr_doa->m_delta_angle * PI / 180.0f;
		for (k = 0; k < ptr_doa->m_sp_size; ++k )
		{
			omega = 2.0f * PI * ptr_doa->m_deta_fs * (float)k;
			for (j = 0; j < ptr_doa->m_channels; ++j )
			{
				deta = (float)(omega * (ptr_doa->cood[j].x * cos(theta) * sin(phi) + ptr_doa->cood[j].y * sin(theta) * sin(phi) + ptr_doa->cood[j].z * cos(phi))/ VELOCITY);
				ptr_doa->m_gstv_re[i*ptr_doa->m_gstv_dim+k*ptr_doa->m_channels+j] = (float)cos(deta);
				ptr_doa->m_gstv_im[i*ptr_doa->m_gstv_dim+k*ptr_doa->m_channels+j] = (float)sin(deta);
			}
		}
	}

	return 0;
}

void* dios_ssp_doa_init_api(int mic_num, PlaneCoord* mic_coord)
{
	void* st = NULL;
	st = (void*)calloc(1, sizeof(objDOA));
	objDOA* ptr_doa = (objDOA*)st;

    int i;
    ptr_doa->m_channels = mic_num;
	ptr_doa->cood = mic_coord;
	ptr_doa->m_fs = DEFAULT_DOA_SAMPLING_FRQ;
	ptr_doa->m_fft_size = DEFAULT_DOA_WIN_SIZE;
	ptr_doa->m_shift_size = DEFAULT_DOA_SHIFT_SIZE;
    ptr_doa->m_delta_angle = DEFAULT_DOA_DELTA_ANGLE;
    ptr_doa->m_low_frq = DEFAULT_DOA_LOW_FRQ;
	ptr_doa->m_high_frq = DEFAULT_DOA_HIGH_FRQ;
	ptr_doa->m_frq_sp = DEFAULT_DOA_FRQ_SP;
	ptr_doa->m_eps = DEFAULT_DOA_EPS;
	ptr_doa->m_alpha_rxx = DEFAULT_DOA_ALPHA_RXX;
	ptr_doa->m_beta_rxx = 1-ptr_doa->m_alpha_rxx;
	ptr_doa->m_deta_fs = ptr_doa->m_fs / (float)ptr_doa->m_fft_size;
    ptr_doa->m_angle_num = (int)((360.0-0.0)/ ptr_doa->m_delta_angle);
    ptr_doa->m_angle_smooth = 90.0f;
    ptr_doa->m_frq_bin_num = (int)((ptr_doa->m_high_frq - ptr_doa->m_low_frq)/ptr_doa->m_frq_sp + 1);
    ptr_doa->m_low_fid = (int)(ptr_doa->m_low_frq*ptr_doa->m_fft_size/ptr_doa->m_fs);
    ptr_doa->m_sp_size = ptr_doa->m_fft_size/2+1;
    ptr_doa->m_rxx_size = ptr_doa->m_channels * ptr_doa->m_channels;
    ptr_doa->m_frq_bin_width = (int)(ptr_doa->m_frq_sp/ptr_doa->m_deta_fs);

    ptr_doa->m_capon_spectrum = (float*)calloc(ptr_doa->m_angle_num, sizeof(float));
    ptr_doa->m_doa_fid = (int*)calloc(ptr_doa->m_angle_num, sizeof(int));
    ptr_doa->m_irxx_re = (float*)calloc(ptr_doa->m_sp_size*ptr_doa->m_rxx_size, sizeof(float));
	ptr_doa->m_irxx_im = (float*)calloc(ptr_doa->m_sp_size*ptr_doa->m_rxx_size, sizeof(float));
    ptr_doa->m_vec_re = (float*)calloc(ptr_doa->m_channels, sizeof(float));
	ptr_doa->m_vec_im = (float*)calloc(ptr_doa->m_channels, sizeof(float));
    ptr_doa->m_gstv_re = (float*)calloc(ptr_doa->m_sp_size*ptr_doa->m_angle_num*ptr_doa->m_channels, sizeof(float));
	ptr_doa->m_gstv_im = (float*)calloc(ptr_doa->m_sp_size*ptr_doa->m_angle_num*ptr_doa->m_channels, sizeof(float));
    ptr_doa->m_rxx_in = (float*)calloc(2*ptr_doa->m_rxx_size, sizeof(float));
    ptr_doa->m_rxx_re = (float*)calloc(ptr_doa->m_sp_size*ptr_doa->m_rxx_size, sizeof(float));
    ptr_doa->m_rxx_im = (float*)calloc(ptr_doa->m_sp_size*ptr_doa->m_rxx_size, sizeof(float));
	ptr_doa->m_mch_buffer = (float**)calloc(ptr_doa->m_channels, sizeof(float*));
	for (i = 0; i < ptr_doa->m_channels; ++i )
	{
		ptr_doa->m_mch_buffer[i] = (float*)calloc(ptr_doa->m_fft_size, sizeof(float));
	}
	ptr_doa->doa_fft = dios_ssp_share_rfft_init(ptr_doa->m_fft_size);
	ptr_doa->fft_out = (float *)calloc(ptr_doa->m_fft_size, sizeof(float));
	ptr_doa->m_win_data = (float*)calloc(ptr_doa->m_fft_size*ptr_doa->m_channels, sizeof(float));
	ptr_doa->m_re = (float*)calloc(ptr_doa->m_fft_size*ptr_doa->m_channels, sizeof(float));
	ptr_doa->m_im = (float*)calloc(ptr_doa->m_fft_size*ptr_doa->m_channels, sizeof(float));	
	ptr_doa->m_irxx_out = (float*)calloc(2*ptr_doa->m_rxx_size, sizeof(float));
	ptr_doa->m_gstv_dim = ptr_doa->m_sp_size*ptr_doa->m_channels;	
	ptr_doa->doainv = dios_ssp_matrix_inv_init(ptr_doa->m_channels);
	ptr_doa->doawin = (objDOACwin*)calloc(1, sizeof(objDOACwin));
	dios_ssp_doa_win_init(ptr_doa->doawin, ptr_doa->m_fft_size, ptr_doa->m_shift_size);

	dios_ssp_doa_init_steering_vectors_g(ptr_doa);
	
	return st;	
}

int dios_ssp_doa_reset_api(void *ptr)
{
    int i;
    objDOA* ptr_doa;
	ptr_doa = (objDOA*)ptr;
	
	ptr_doa->m_first_frame_flag = 1;
	//fft
	for (i = 0; i < ptr_doa->m_fft_size; ++i )
    {
		ptr_doa->fft_out[i] = 0.0;
    }

    for(i = 0; i < ptr_doa->m_frq_bin_num; ++i)
	{
		ptr_doa->m_doa_fid[i] = ptr_doa->m_low_fid + (i*ptr_doa->m_frq_sp*ptr_doa->m_fft_size)/ptr_doa->m_fs;
	}
	for (i = 0; i < ptr_doa->m_channels; ++i )
	{
		memset( ptr_doa->m_mch_buffer[i], 0, sizeof(float)*ptr_doa->m_fft_size );
	}
	memset( ptr_doa->m_win_data, 0, sizeof(float)*ptr_doa->m_channels*ptr_doa->m_fft_size );
	memset( ptr_doa->m_re, 0, sizeof(float)*ptr_doa->m_channels*ptr_doa->m_fft_size );
	memset( ptr_doa->m_im, 0, sizeof(float)*ptr_doa->m_channels*ptr_doa->m_fft_size );
    memset( ptr_doa->m_capon_spectrum, 0, sizeof(float)*ptr_doa->m_angle_num );
    memset( ptr_doa->m_irxx_re, 0, sizeof(float)*ptr_doa->m_sp_size*ptr_doa->m_rxx_size );
	memset( ptr_doa->m_irxx_im, 0, sizeof(float)*ptr_doa->m_sp_size*ptr_doa->m_rxx_size );
    memset( ptr_doa->m_vec_re, 0, sizeof(float)*ptr_doa->m_channels );
	memset( ptr_doa->m_vec_im, 0, sizeof(float)*ptr_doa->m_channels );
    memset( ptr_doa->m_rxx_in, 0, sizeof(float)*2*ptr_doa->m_rxx_size );
    memset( ptr_doa->m_rxx_re, 0, sizeof(float)*ptr_doa->m_sp_size*ptr_doa->m_rxx_size );
	memset( ptr_doa->m_rxx_im, 0, sizeof(float)*ptr_doa->m_sp_size*ptr_doa->m_rxx_size );	
	memset( ptr_doa->m_irxx_out, 0, sizeof(float)*2*ptr_doa->m_rxx_size );
	    
    return 0;
}

float dios_ssp_doa_process_api(void* ptr, float* in, int vad_result, int dt_st)
{
    int   max_ind = 0;
	float max_spectrum = 0;
	float re_temp, im_temp;

    objDOA* ptr_doa;
	ptr_doa = (objDOA*)ptr;

	for (int ch_idx = 0; ch_idx < ptr_doa->m_channels; ++ch_idx)
	{
		for (int i = 0; i < ptr_doa->m_shift_size; ++i)
		{
			ptr_doa->m_mch_buffer[ch_idx][ptr_doa->m_fft_size - ptr_doa->m_shift_size + i] = in[ch_idx * ptr_doa->m_shift_size + i];
		}
	}	

	// add ana window		
	for (int ch_idx = 0; ch_idx < ptr_doa->m_channels; ++ch_idx )
	{
		dios_ssp_doa_win_add_ana_win(ptr_doa->doawin, ptr_doa->m_mch_buffer[ch_idx], ptr_doa->m_win_data+ch_idx*ptr_doa->m_fft_size);
	}
	// stft 
	for (int ch_idx = 0; ch_idx < ptr_doa->m_channels; ++ch_idx )
	{
		dios_ssp_share_rfft_process(ptr_doa->doa_fft, ptr_doa->m_win_data+ch_idx*ptr_doa->m_fft_size, ptr_doa->fft_out);
		for (int i = 0; i < ptr_doa->m_fft_size / 2 + 1; i++)
		{
			ptr_doa->m_re[i + ch_idx * ptr_doa->m_fft_size] = ptr_doa->fft_out[i];
		}
		ptr_doa->m_im[0 + ch_idx * ptr_doa->m_fft_size] = ptr_doa->m_im[ptr_doa->m_fft_size / 2 + ch_idx * ptr_doa->m_fft_size] = 0.0;
		for (int i = 1; i < ptr_doa->m_fft_size / 2; i++)
		{
			ptr_doa->m_im[i + ch_idx * ptr_doa->m_fft_size] = -ptr_doa->fft_out[ptr_doa->m_fft_size - i];
		}
	}

	dios_ssp_doa_cal_rxx(ptr_doa);

	for ( int n = 0; n < ptr_doa->m_frq_bin_num; ++n)
	{
		int k = ptr_doa->m_doa_fid[n];

		memset( ptr_doa->m_rxx_in, 0, sizeof(float)*2*ptr_doa->m_rxx_size );
			
		for ( int i = 0; i < ptr_doa->m_channels; ++i)
		{
			for(int m = 0; m < ptr_doa->m_frq_bin_width; ++m)
			{	
				ptr_doa->m_rxx_in[i*ptr_doa->m_channels*2+2*i] += ptr_doa->m_rxx_re[(k-m+ptr_doa->m_frq_bin_width/2-1)*ptr_doa->m_rxx_size+i*ptr_doa->m_channels+i];		
				ptr_doa->m_rxx_in[i*ptr_doa->m_channels*2+2*i+1] += ptr_doa->m_rxx_im[(k-m+ptr_doa->m_frq_bin_width/2-1)*ptr_doa->m_rxx_size+i*ptr_doa->m_channels+i];
			}
			ptr_doa->m_rxx_in[i*ptr_doa->m_channels*2+2*i] /= ptr_doa->m_frq_bin_width;
			ptr_doa->m_rxx_in[i*ptr_doa->m_channels*2+2*i+1] /= ptr_doa->m_frq_bin_width;
			for ( int j = i+1; j < ptr_doa->m_channels; ++j)
			{
				for( int m = 0; m < ptr_doa->m_frq_bin_width; ++m)
				{
					ptr_doa->m_rxx_in[i*ptr_doa->m_channels*2+2*j] += ptr_doa->m_rxx_re[(k-m+ptr_doa->m_frq_bin_width/2-1)*ptr_doa->m_rxx_size+i*ptr_doa->m_channels+j];
					ptr_doa->m_rxx_in[i*ptr_doa->m_channels*2+2*j+1] += ptr_doa->m_rxx_im[(k-m+ptr_doa->m_frq_bin_width/2-1)*ptr_doa->m_rxx_size+i*ptr_doa->m_channels+j];				
				}
				ptr_doa->m_rxx_in[i*ptr_doa->m_channels*2+2*j] /= ptr_doa->m_frq_bin_width;
				ptr_doa->m_rxx_in[i*ptr_doa->m_channels*2+2*j+1] /= ptr_doa->m_frq_bin_width;
				ptr_doa->m_rxx_in[j*ptr_doa->m_channels*2+2*i] = ptr_doa->m_rxx_in[i*ptr_doa->m_channels*2+2*j];
				ptr_doa->m_rxx_in[j*ptr_doa->m_channels*2+2*i+1] = -ptr_doa->m_rxx_in[i*ptr_doa->m_channels*2+2*j+1];
			}
		}
		dios_ssp_matrix_inv_process(ptr_doa->doainv, ptr_doa->m_rxx_in, ptr_doa->m_irxx_out);
		for ( int i = 0; i < ptr_doa->m_channels; ++i)
		{
			for ( int j = 0; j < ptr_doa->m_channels; ++j)
			{
				ptr_doa->m_irxx_re[k*ptr_doa->m_rxx_size+i*ptr_doa->m_channels+j] =  ptr_doa->m_irxx_out[i*ptr_doa->m_channels*2+2*j];
				ptr_doa->m_irxx_im[k*ptr_doa->m_rxx_size+i*ptr_doa->m_channels+j] =  ptr_doa->m_irxx_out[i*ptr_doa->m_channels*2+2*j+1];
			}
		}
	}

	memset(ptr_doa->m_capon_spectrum, 0, sizeof(float)*ptr_doa->m_angle_num);
	for ( int n = 0; n < ptr_doa->m_frq_bin_num; ++n)
	{
		int k = ptr_doa->m_doa_fid[n];
		for ( int m = 0; m < ptr_doa->m_angle_num; ++m )
		{
			for (int i = 0; i < ptr_doa->m_channels; i++)
			{
				re_temp = im_temp = 0;
				for (int j = 0; j < ptr_doa->m_channels; j++)
				{
					re_temp += ptr_doa->m_irxx_re[k*ptr_doa->m_rxx_size+i*ptr_doa->m_channels+j] * ptr_doa->m_gstv_re[m*ptr_doa->m_gstv_dim+k*ptr_doa->m_channels+j] 
							- ptr_doa->m_irxx_im[k*ptr_doa->m_rxx_size+i*ptr_doa->m_channels+j] * ptr_doa->m_gstv_im[m*ptr_doa->m_gstv_dim+k*ptr_doa->m_channels+j];
					im_temp += ptr_doa->m_irxx_re[k*ptr_doa->m_rxx_size+i*ptr_doa->m_channels+j] * ptr_doa->m_gstv_im[m*ptr_doa->m_gstv_dim+k*ptr_doa->m_channels+j] 
							+ ptr_doa->m_irxx_im[k*ptr_doa->m_rxx_size+i*ptr_doa->m_channels+j] * ptr_doa->m_gstv_re[m*ptr_doa->m_gstv_dim+k*ptr_doa->m_channels+j];
				}
				ptr_doa->m_vec_re[i] = re_temp;
				ptr_doa->m_vec_im[i] = im_temp;
			}
			re_temp = im_temp = 0;
			for (int i = 0; i < ptr_doa->m_channels; i++)
			{
				re_temp += ptr_doa->m_gstv_re[m*ptr_doa->m_gstv_dim+k*ptr_doa->m_channels+i]*ptr_doa->m_vec_re[i] + ptr_doa->m_gstv_im[m*ptr_doa->m_gstv_dim+k*ptr_doa->m_channels+i]*ptr_doa->m_vec_im[i];
				im_temp += ptr_doa->m_gstv_re[m*ptr_doa->m_gstv_dim+k*ptr_doa->m_channels+i]*ptr_doa->m_vec_im[i] - ptr_doa->m_gstv_im[m*ptr_doa->m_gstv_dim+k*ptr_doa->m_channels+i]*ptr_doa->m_vec_re[i];
			}
			ptr_doa->m_capon_spectrum[m] += ptr_doa->m_channels / re_temp;
		}
	}

	max_ind = 0;
	max_spectrum = ptr_doa->m_capon_spectrum[0];
	for ( int m = 1; m < ptr_doa->m_angle_num; ++m )
	{
		if ( ptr_doa->m_capon_spectrum[m] > max_spectrum )
		{
			max_ind = m;
			max_spectrum = ptr_doa->m_capon_spectrum[m];
		}
	}

	if(vad_result == 1 || dt_st != 1)
	{
		ptr_doa->m_angle_smooth = (float)(max_ind*ptr_doa->m_delta_angle);
	}	

	for (int ch_idx = 0; ch_idx < ptr_doa->m_channels; ++ch_idx)
	{
		memmove(ptr_doa->m_mch_buffer[ch_idx], ptr_doa->m_mch_buffer[ch_idx] + ptr_doa->m_shift_size, sizeof(float) * (ptr_doa->m_fft_size - ptr_doa->m_shift_size));
	}

    return ptr_doa->m_angle_smooth;
}

int dios_ssp_doa_uninit_api(void *ptr)
{
    objDOA* ptr_doa;
	ptr_doa = (objDOA*)ptr;
	int ret = 0;

	dios_ssp_doa_win_delete(ptr_doa->doawin);
	ret = dios_ssp_share_rfft_uninit(ptr_doa->doa_fft);
	if (0 != ret)
	{
		ptr_doa->doa_fft = NULL;
	}

	ret = dios_ssp_matrix_inv_delete(ptr_doa->doainv);
	if (0 != ret)
	{
		ptr_doa->doainv = NULL;
	}
	for (int i = 0; i < ptr_doa->m_channels; ++i )
	{
		free(ptr_doa->m_mch_buffer[i]);
	}
	free(ptr_doa->fft_out);
	free(ptr_doa->m_win_data);
	free(ptr_doa->m_re);
	free(ptr_doa->m_im);
	free(ptr_doa->m_mch_buffer);
    free(ptr_doa->m_capon_spectrum);
    free(ptr_doa->m_doa_fid);
    free(ptr_doa->m_irxx_re);
    free(ptr_doa->m_irxx_im);
    free(ptr_doa->m_vec_re);
    free(ptr_doa->m_vec_im);
    free(ptr_doa->m_gstv_re);
	free(ptr_doa->m_gstv_im);
    free(ptr_doa->m_rxx_in);
    free(ptr_doa->m_rxx_re);
    free(ptr_doa->m_rxx_im);	
	free(ptr_doa->m_irxx_out);	

    return 0;
}
