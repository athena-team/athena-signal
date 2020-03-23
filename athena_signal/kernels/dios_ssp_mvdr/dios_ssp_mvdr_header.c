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

Description: MVDR beamformer includes steering vector calculation, noise
estimation using MCRA, and the autocorrelation matrix Rnn estimation.
==============================================================================*/

#include "dios_ssp_mvdr_header.h"

int dios_ssp_mvdr_init_diffuse_rnn(objMVDR *ptr_mvdr)
{
    int i, j, k;
	float f = 0, temp = 0;
	for (k = 1; k < ptr_mvdr->m_sp_size; k++)
	{
		f = ptr_mvdr->m_deta_fs*k;
		for (i = 0; i < ptr_mvdr->m_channels; i++)
		{
			ptr_mvdr->m_sd_rnn_re[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+i] = 1.0f*(1+ptr_mvdr->m_sd_factor)+ptr_mvdr->m_sd_eps;
			for (j = i+1; j < ptr_mvdr->m_channels; j++)
			{				
				temp = 2 * PI * f * ptr_mvdr->dist[i * ptr_mvdr->m_channels + j] / VELOCITY;
				ptr_mvdr->m_sd_rnn_re[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+j] = (float)(sin(temp)/temp);
				ptr_mvdr->m_sd_rnn_re[k*ptr_mvdr->m_rxx_size+j*ptr_mvdr->m_channels+i] = ptr_mvdr->m_sd_rnn_re[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+j];
			}
		}
		for (i = 0; i < ptr_mvdr->m_channels; i++)
		{
			for (j = 0; j < ptr_mvdr->m_channels; j++)
			{
				ptr_mvdr->m_rxx_in[i*ptr_mvdr->m_channels*2+2*j] = ptr_mvdr->m_sd_rnn_re[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+j];
				ptr_mvdr->m_rxx_in[i*ptr_mvdr->m_channels*2+2*j+1] = 0;
			}
		}
		dios_ssp_matrix_inv_process(ptr_mvdr->mvdrinv, ptr_mvdr->m_rxx_in, ptr_mvdr->m_irxx_out);	
		for (i = 0; i < ptr_mvdr->m_channels; ++i)
		{
			for (j = 0; j < ptr_mvdr->m_channels; ++j)
			{
				ptr_mvdr->m_sd_irnn_re[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+j] = ptr_mvdr->m_irxx_out[i*ptr_mvdr->m_channels*2+2*j];
				ptr_mvdr->m_sd_irnn_im[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+j] = ptr_mvdr->m_irxx_out[i*ptr_mvdr->m_channels*2+2*j+1];
			}
		}
	}

	return 0;
}

int dios_ssp_mvdr_alloc_mem(objMVDR *ptr_mvdr)
{
    int i;
	ptr_mvdr->m_mch_buffer = (float**)calloc(ptr_mvdr->m_channels, sizeof(float*));
	for (i = 0; i < ptr_mvdr->m_channels; ++i )
	{
		ptr_mvdr->m_mch_buffer[i] = (float*)calloc(ptr_mvdr->m_fft_size, sizeof(float));
	}
	
	ptr_mvdr->m_win_data = (float*)calloc(ptr_mvdr->m_fft_size*ptr_mvdr->m_channels, sizeof(float));
	ptr_mvdr->m_re = (float*)calloc(ptr_mvdr->m_fft_size*ptr_mvdr->m_channels, sizeof(float));
	ptr_mvdr->m_im = (float*)calloc(ptr_mvdr->m_fft_size*ptr_mvdr->m_channels, sizeof(float));
	ptr_mvdr->m_re_temp = (float*)calloc(ptr_mvdr->m_fft_size*ptr_mvdr->m_channels, sizeof(float));	
	ptr_mvdr->m_im_temp = (float*)calloc(ptr_mvdr->m_fft_size*ptr_mvdr->m_channels, sizeof(float));
	
	ptr_mvdr->m_rnn_re = (float*)calloc(ptr_mvdr->m_sp_size*ptr_mvdr->m_rxx_size, sizeof(float));
	ptr_mvdr->m_rnn_im = (float*)calloc(ptr_mvdr->m_sp_size*ptr_mvdr->m_rxx_size, sizeof(float));	
	ptr_mvdr->m_irnn_re = (float*)calloc(ptr_mvdr->m_sp_size*ptr_mvdr->m_rxx_size, sizeof(float));
	ptr_mvdr->m_irnn_im = (float*)calloc(ptr_mvdr->m_sp_size*ptr_mvdr->m_rxx_size, sizeof(float));
	
	ptr_mvdr->m_rxx_in = (float*)calloc(2*ptr_mvdr->m_rxx_size, sizeof(float));
	ptr_mvdr->m_irxx_out = (float*)calloc(2*ptr_mvdr->m_rxx_size, sizeof(float));
	
	ptr_mvdr->m_sd_rnn_re = (float*)calloc(ptr_mvdr->m_sp_size*ptr_mvdr->m_rxx_size, sizeof(float));
	ptr_mvdr->m_sd_irnn_re = (float*)calloc(ptr_mvdr->m_sp_size*ptr_mvdr->m_rxx_size, sizeof(float));
	ptr_mvdr->m_sd_irnn_im = (float*)calloc(ptr_mvdr->m_sp_size*ptr_mvdr->m_rxx_size, sizeof(float));
	
	ptr_mvdr->m_weight_sd_group_re = (float*)calloc(ptr_mvdr->m_angle_num*ptr_mvdr->m_sp_size*ptr_mvdr->m_channels, sizeof(float));
	ptr_mvdr->m_weight_sd_group_im = (float*)calloc(ptr_mvdr->m_angle_num*ptr_mvdr->m_sp_size*ptr_mvdr->m_channels, sizeof(float));

	ptr_mvdr->m_weight_re = (float*)calloc(ptr_mvdr->m_sp_size*ptr_mvdr->m_channels, sizeof(float));
	ptr_mvdr->m_weight_im = (float*)calloc(ptr_mvdr->m_sp_size*ptr_mvdr->m_channels, sizeof(float));

	ptr_mvdr->m_mvdr_out_re = (float*)calloc(ptr_mvdr->m_fft_size, sizeof(float));
	ptr_mvdr->m_mvdr_out_im = (float*)calloc(ptr_mvdr->m_fft_size, sizeof(float));
	ptr_mvdr->m_out_bf = (float*)calloc(ptr_mvdr->m_fft_size, sizeof(float));

    // grid steering vectors
	ptr_mvdr->m_gstv_re = (float*)calloc(ptr_mvdr->m_sp_size*ptr_mvdr->m_angle_num*ptr_mvdr->m_channels, sizeof(float));
	ptr_mvdr->m_gstv_im = (float*)calloc(ptr_mvdr->m_sp_size*ptr_mvdr->m_angle_num*ptr_mvdr->m_channels, sizeof(float));
	ptr_mvdr->m_stv_re = (float*)calloc(ptr_mvdr->m_sp_size*ptr_mvdr->m_channels, sizeof(float));
	ptr_mvdr->m_stv_im = (float*)calloc(ptr_mvdr->m_sp_size*ptr_mvdr->m_channels, sizeof(float));

	// mcra 
	ptr_mvdr->m_ns_ps_cur_mic = (float*)calloc(ptr_mvdr->m_fft_size, sizeof(float));
    ptr_mvdr->m_ns_theta_cur_mic = (float*)calloc(ptr_mvdr->m_fft_size, sizeof(float));
	ptr_mvdr->m_ns_ps = (float*)calloc(ptr_mvdr->m_channels*ptr_mvdr->m_fft_size, sizeof(float));
	ptr_mvdr->m_P = (float*)calloc(ptr_mvdr->m_channels*ptr_mvdr->m_fft_size, sizeof(float));
	ptr_mvdr->m_Ptmp = (float*)calloc(ptr_mvdr->m_channels*ptr_mvdr->m_fft_size, sizeof(float));
	ptr_mvdr->m_Pmin = (float*)calloc(ptr_mvdr->m_channels*ptr_mvdr->m_fft_size, sizeof(float));
	ptr_mvdr->m_pk = (float*)calloc(ptr_mvdr->m_channels*ptr_mvdr->m_fft_size, sizeof(float));
	ptr_mvdr->m_xn_re = (float*)calloc(ptr_mvdr->m_channels*ptr_mvdr->m_fft_size, sizeof(float));
	ptr_mvdr->m_xn_im = (float*)calloc(ptr_mvdr->m_channels*ptr_mvdr->m_fft_size, sizeof(float));
	
	dios_ssp_mvdr_reset(ptr_mvdr);

	return 0;
}

int dios_ssp_mvdr_init_steering_vectors_g(objMVDR *ptr_mvdr)
{
	float deta = 0.0f;
	float theta = 0.0f;
	float phi = PI * 0.5f;
	float omega = 0.0f;
    int i, j, k;

	for (i = 0; i < ptr_mvdr->m_angle_num; ++i )
	{
		theta = (float)i * (float)ptr_mvdr->m_delta_angle * PI / 180.0f;
		for (k = 0; k < ptr_mvdr->m_sp_size; ++k )
		{
			omega = 2.0f * PI * ptr_mvdr->m_deta_fs * (float)k;
			for (j = 0; j < ptr_mvdr->m_channels; ++j )
			{
				deta = (float)(omega  * (ptr_mvdr->cood[j].x * cos(theta) * sin(phi) + ptr_mvdr->cood[j].y * sin(theta) * sin(phi) + ptr_mvdr->cood[j].z * cos(phi))/ VELOCITY);
				ptr_mvdr->m_gstv_re[i*ptr_mvdr->m_gstv_dim+k*ptr_mvdr->m_channels+j] = (float)cos(deta);
				ptr_mvdr->m_gstv_im[i*ptr_mvdr->m_gstv_dim+k*ptr_mvdr->m_channels+j] = (float)sin(deta);
			}
		}
	}

	return 0;
}

int dios_ssp_mvdr_init_sd_weights(objMVDR *ptr_mvdr)
{
    int i, j, k, m;
	dios_ssp_mvdr_init_diffuse_rnn(ptr_mvdr);
	
	float re_temp, im_temp, power, re_temp2, im_temp2;	
	
	for (m = 0; m < ptr_mvdr->m_angle_num; m++)
	{
		for (k = 1; k < ptr_mvdr->m_fft_size/2; k++ )	
		{		
			for (i = 0; i < ptr_mvdr->m_channels; i++)		
			{			
				re_temp = im_temp = 0;			
				for (j = 0; j < ptr_mvdr->m_channels; j++)			
				{				
					re_temp += ptr_mvdr->m_sd_irnn_re[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+j]*ptr_mvdr->m_gstv_re[m*ptr_mvdr->m_gstv_dim+k*ptr_mvdr->m_channels+j] 
								- ptr_mvdr->m_sd_irnn_im[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+j]*ptr_mvdr->m_gstv_im[m*ptr_mvdr->m_gstv_dim+k*ptr_mvdr->m_channels+j];
					im_temp += ptr_mvdr->m_sd_irnn_re[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+j]*ptr_mvdr->m_gstv_im[m*ptr_mvdr->m_gstv_dim+k*ptr_mvdr->m_channels+j] 
							+ ptr_mvdr->m_sd_irnn_im[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+j]*ptr_mvdr->m_gstv_re[m*ptr_mvdr->m_gstv_dim+k*ptr_mvdr->m_channels+j];
				}						
				ptr_mvdr->m_weight_sd_group_re[m*ptr_mvdr->m_sp_size*ptr_mvdr->m_channels+k*ptr_mvdr->m_channels+i] = re_temp;		
				ptr_mvdr->m_weight_sd_group_im[m*ptr_mvdr->m_sp_size*ptr_mvdr->m_channels+k*ptr_mvdr->m_channels+i] = im_temp;		
			}					
			re_temp = im_temp = 0;		
			for (i = 0; i < ptr_mvdr->m_channels; i++ )		
			{			
				re_temp += ptr_mvdr->m_gstv_re[m*ptr_mvdr->m_gstv_dim+k*ptr_mvdr->m_channels+i]*ptr_mvdr->m_weight_sd_group_re[m*ptr_mvdr->m_sp_size*ptr_mvdr->m_channels+k*ptr_mvdr->m_channels+i] 
						+ ptr_mvdr->m_gstv_im[m*ptr_mvdr->m_gstv_dim+k*ptr_mvdr->m_channels+i]*ptr_mvdr->m_weight_sd_group_im[m*ptr_mvdr->m_sp_size*ptr_mvdr->m_channels+k*ptr_mvdr->m_channels+i];
				im_temp += ptr_mvdr->m_gstv_re[m*ptr_mvdr->m_gstv_dim+k*ptr_mvdr->m_channels+i]*ptr_mvdr->m_weight_sd_group_im[m*ptr_mvdr->m_sp_size*ptr_mvdr->m_channels+k*ptr_mvdr->m_channels+i] 
						- ptr_mvdr->m_gstv_im[m*ptr_mvdr->m_gstv_dim+k*ptr_mvdr->m_channels+i]*ptr_mvdr->m_weight_sd_group_re[m*ptr_mvdr->m_sp_size*ptr_mvdr->m_channels+k*ptr_mvdr->m_channels+i];
			}				
			power = re_temp*re_temp + im_temp*im_temp;
			for (i = 0; i < ptr_mvdr->m_channels; i++)		
			{	
				re_temp2 = (re_temp*ptr_mvdr->m_weight_sd_group_re[m*ptr_mvdr->m_sp_size*ptr_mvdr->m_channels+k*ptr_mvdr->m_channels+i]
						+im_temp*ptr_mvdr->m_weight_sd_group_im[m*ptr_mvdr->m_sp_size*ptr_mvdr->m_channels+k*ptr_mvdr->m_channels+i])/power;
				im_temp2 = (re_temp*ptr_mvdr->m_weight_sd_group_im[m*ptr_mvdr->m_sp_size*ptr_mvdr->m_channels+k*ptr_mvdr->m_channels+i]
						-im_temp*ptr_mvdr->m_weight_sd_group_re[m*ptr_mvdr->m_sp_size*ptr_mvdr->m_channels+k*ptr_mvdr->m_channels+i])/power; 

				ptr_mvdr->m_weight_sd_group_re[m*ptr_mvdr->m_sp_size*ptr_mvdr->m_channels+k*ptr_mvdr->m_channels+i] = re_temp2;			
				ptr_mvdr->m_weight_sd_group_im[m*ptr_mvdr->m_sp_size*ptr_mvdr->m_channels+k*ptr_mvdr->m_channels+i] = im_temp2;
			}
		}
	}
	
	int ang_region = ptr_mvdr->m_angle_pre/ptr_mvdr->m_delta_angle;
	memcpy(ptr_mvdr->m_weight_re, ptr_mvdr->m_weight_sd_group_re+ang_region*ptr_mvdr->m_sp_size*ptr_mvdr->m_channels, sizeof(float)*ptr_mvdr->m_sp_size*ptr_mvdr->m_channels);
	memcpy(ptr_mvdr->m_weight_im, ptr_mvdr->m_weight_sd_group_im+ang_region*ptr_mvdr->m_sp_size*ptr_mvdr->m_channels, sizeof(float)*ptr_mvdr->m_sp_size*ptr_mvdr->m_channels);

	return 0;
}

int dios_ssp_mvdr_free_mem(objMVDR *ptr_mvdr)
{
    int i;

	for (i = 0; i < ptr_mvdr->m_channels; ++i )
	{
		free(ptr_mvdr->m_mch_buffer[i]);
	}
	free(ptr_mvdr->m_mch_buffer);
	free(ptr_mvdr->m_win_data);
	free(ptr_mvdr->m_re);
	free(ptr_mvdr->m_im);	
	free(ptr_mvdr->m_re_temp);
	free(ptr_mvdr->m_im_temp);
	
	free(ptr_mvdr->m_rnn_re);
	free(ptr_mvdr->m_rnn_im);
	free(ptr_mvdr->m_irnn_re);
	free(ptr_mvdr->m_irnn_im);
	
	free(ptr_mvdr->m_rxx_in);
	free(ptr_mvdr->m_irxx_out);

	free(ptr_mvdr->m_sd_rnn_re);
	free(ptr_mvdr->m_sd_irnn_re);
	free(ptr_mvdr->m_sd_irnn_im);
	
	free(ptr_mvdr->m_weight_sd_group_re);
	free(ptr_mvdr->m_weight_sd_group_im);

	free(ptr_mvdr->m_weight_re);
	free(ptr_mvdr->m_weight_im);

	free(ptr_mvdr->m_mvdr_out_re);
	free(ptr_mvdr->m_mvdr_out_im);
	free(ptr_mvdr->m_out_bf);
	
	free(ptr_mvdr->m_gstv_re);
	free(ptr_mvdr->m_gstv_im);
	free(ptr_mvdr->m_stv_re);
	free(ptr_mvdr->m_stv_im);
	
    // mcra
	free(ptr_mvdr->m_ns_ps_cur_mic);
	free(ptr_mvdr->m_ns_theta_cur_mic);
	free(ptr_mvdr->m_ns_ps);
	free(ptr_mvdr->m_P);
	free(ptr_mvdr->m_Ptmp);
	free(ptr_mvdr->m_Pmin);
	free(ptr_mvdr->m_pk);
	free(ptr_mvdr->m_xn_re);
	free(ptr_mvdr->m_xn_im);

	return 0;
}

void dios_ssp_mvdr_init(objMVDR *ptr_mvdr, int sensor_num, PlaneCoord* cood)
{
    int i, j;

    ptr_mvdr->m_fs = DEFAULT_MVDR_SAMPLING_FRQ;
	
	ptr_mvdr->m_channels = sensor_num;

	ptr_mvdr->m_fft_size = DEFAULT_MVDR_WIN_SIZE;
	ptr_mvdr->m_shift_size = DEFAULT_MVDR_SHIFT_SIZE;
	
	ptr_mvdr->m_delta_angle = DEFAULT_MVDR_DELTA_ANGLE;
	
	ptr_mvdr->m_sd_factor = DEFAULT_MVDR_SD_FACTOR;
	ptr_mvdr->m_sd_eps = DEFAULT_MVDR_SD_EPS;
	
	ptr_mvdr->m_rnn_eps = DEFAULT_MVDR_RNN_EPS;
	ptr_mvdr->m_alpha_rnn = DEFAULT_MVDR_ALPHA_RNN;

    ptr_mvdr->m_alpha_s = DEFAULT_MVDR_ALPHA_S;
    ptr_mvdr->m_alpha_p = DEFAULT_MVDR_ALPHA_P;
    ptr_mvdr->m_alpha_d = DEFAULT_MVDR_ALPHA_D;
    ptr_mvdr->m_L = DEFAULT_MVDR_L;
    ptr_mvdr->m_delta_thres = DEFAULT_MVDR_DELTA_THRES;

    ptr_mvdr->m_deta_fs = ptr_mvdr->m_fs / (float)ptr_mvdr->m_fft_size;
	ptr_mvdr->m_sp_size = ptr_mvdr->m_fft_size/2+1;	

	ptr_mvdr->m_rxx_size = ptr_mvdr->m_channels * ptr_mvdr->m_channels;
	ptr_mvdr->m_gstv_dim = ptr_mvdr->m_sp_size*ptr_mvdr->m_channels;
	
	ptr_mvdr->m_beta_rnn = 1-ptr_mvdr->m_alpha_rnn;

	ptr_mvdr->cood = cood;

	ptr_mvdr->dist = (float*)calloc(ptr_mvdr->m_channels * ptr_mvdr->m_channels, sizeof(float));
	for (i = 0; i < ptr_mvdr->m_channels; i++)
	{
		for (j = i+1; j < ptr_mvdr->m_channels; j++)
		{
			ptr_mvdr->dist[i* ptr_mvdr->m_channels + j] = (float)sqrt(pow(ptr_mvdr->cood[i].x - ptr_mvdr->cood[j].x, 2) + pow(ptr_mvdr->cood[i].y * ptr_mvdr->cood[i].y, 2) + pow(ptr_mvdr->cood[i].z * ptr_mvdr->cood[i].z, 2));
		}
	}
	ptr_mvdr->mvdrinv = dios_ssp_matrix_inv_init(ptr_mvdr->m_channels);
	ptr_mvdr->m_angle_num = (int)((360.0-0.0)/ ptr_mvdr->m_delta_angle);
	ptr_mvdr->mvdrwin = (objMVDRCwin*)calloc(1, sizeof(objMVDRCwin));
	dios_ssp_mvdr_win_init(ptr_mvdr->mvdrwin, ptr_mvdr->m_fft_size, ptr_mvdr->m_shift_size);
	
	ptr_mvdr->mvdr_fft = dios_ssp_share_rfft_init(ptr_mvdr->m_fft_size);
	ptr_mvdr->fft_in = (float *)calloc(ptr_mvdr->m_fft_size, sizeof(float));
	ptr_mvdr->fft_out = (float *)calloc(ptr_mvdr->m_fft_size, sizeof(float));

	dios_ssp_mvdr_alloc_mem(ptr_mvdr);
	
	dios_ssp_mvdr_init_steering_vectors_g(ptr_mvdr);
	
	dios_ssp_mvdr_init_sd_weights(ptr_mvdr);
}

void dios_ssp_mvdr_reset(objMVDR *ptr_mvdr)
{
    int i, k;
	ptr_mvdr->m_frame_sum = 0;
	ptr_mvdr->m_angle_pre = 89;
	for (i = 0; i < ptr_mvdr->m_channels; ++i )
	{
		memset( ptr_mvdr->m_mch_buffer[i], 0, sizeof(float)*ptr_mvdr->m_fft_size );
	}
	memset( ptr_mvdr->m_win_data, 0, sizeof(float)*ptr_mvdr->m_channels*ptr_mvdr->m_fft_size );
	memset( ptr_mvdr->m_re, 0, sizeof(float)*ptr_mvdr->m_channels*ptr_mvdr->m_fft_size );
	memset( ptr_mvdr->m_im, 0, sizeof(float)*ptr_mvdr->m_channels*ptr_mvdr->m_fft_size );
	memset( ptr_mvdr->m_re_temp, 0, sizeof(float)*ptr_mvdr->m_channels*ptr_mvdr->m_fft_size );
	memset( ptr_mvdr->m_im_temp, 0, sizeof(float)*ptr_mvdr->m_channels*ptr_mvdr->m_fft_size );
	memset( ptr_mvdr->m_rnn_re, 0, sizeof(float)*ptr_mvdr->m_sp_size*ptr_mvdr->m_rxx_size );
	memset( ptr_mvdr->m_rnn_im, 0, sizeof(float)*ptr_mvdr->m_sp_size*ptr_mvdr->m_rxx_size );
	memset( ptr_mvdr->m_irnn_re, 0, sizeof(float)*ptr_mvdr->m_sp_size*ptr_mvdr->m_rxx_size );
	memset( ptr_mvdr->m_irnn_im, 0, sizeof(float)*ptr_mvdr->m_sp_size*ptr_mvdr->m_rxx_size );
	memset( ptr_mvdr->m_rxx_in, 0, sizeof(float)*2*ptr_mvdr->m_rxx_size );
	memset( ptr_mvdr->m_irxx_out, 0, sizeof(float)*2*ptr_mvdr->m_rxx_size );
	memset( ptr_mvdr->m_mvdr_out_re, 0, sizeof(float)*ptr_mvdr->m_fft_size );
	memset( ptr_mvdr->m_mvdr_out_im, 0, sizeof(float)*ptr_mvdr->m_fft_size );
	memset( ptr_mvdr->m_out_bf, 0, sizeof(float)*ptr_mvdr->m_fft_size );
	for(k = 0; k < ptr_mvdr->m_sp_size; ++k)
	{
		for(i = 0; i < ptr_mvdr->m_channels; ++i)
		{
			ptr_mvdr->m_stv_re[k*ptr_mvdr->m_channels+i] = 1;
			ptr_mvdr->m_stv_im[k*ptr_mvdr->m_channels+i] = 0;
		}
	}
	// mcra
	memset( ptr_mvdr->m_ns_ps_cur_mic, 0, sizeof(float)*ptr_mvdr->m_fft_size );
	memset( ptr_mvdr->m_ns_theta_cur_mic, 0, sizeof(float)*ptr_mvdr->m_fft_size );
	memset( ptr_mvdr->m_ns_ps, 0, sizeof(float)*ptr_mvdr->m_channels*ptr_mvdr->m_fft_size );
	memset( ptr_mvdr->m_P, 0, sizeof(float)*ptr_mvdr->m_channels*ptr_mvdr->m_fft_size );
	memset( ptr_mvdr->m_Ptmp, 0, sizeof(float)*ptr_mvdr->m_channels*ptr_mvdr->m_fft_size );
	memset( ptr_mvdr->m_Pmin, 0, sizeof(float)*ptr_mvdr->m_channels*ptr_mvdr->m_fft_size );
	memset( ptr_mvdr->m_pk, 0, sizeof(float)*ptr_mvdr->m_channels*ptr_mvdr->m_fft_size );
	memset( ptr_mvdr->m_xn_re, 0, sizeof(float)*ptr_mvdr->m_channels*ptr_mvdr->m_fft_size );
	memset( ptr_mvdr->m_xn_im, 0, sizeof(float)*ptr_mvdr->m_channels*ptr_mvdr->m_fft_size );

	//fft
	for (i = 0; i < ptr_mvdr->m_fft_size; ++i )
    {
		ptr_mvdr->fft_in[i] = 0.0;
		ptr_mvdr->fft_out[i] = 0.0;
    }
	
}

int dios_ssp_mvdr_process(objMVDR *ptr_mvdr, float* in, float* out, int angle)
{
	int i, k, ch_idx = 0;
	for ( ch_idx = 0; ch_idx < ptr_mvdr->m_channels; ++ch_idx )
	{
		for (i = 0; i < ptr_mvdr->m_shift_size; ++i )
		{
			ptr_mvdr->m_mch_buffer[ch_idx][ptr_mvdr->m_fft_size - ptr_mvdr->m_shift_size + i] = in[ch_idx * ptr_mvdr->m_shift_size + i];
		}
	}
	
	if( angle != ptr_mvdr->m_angle_pre )
	{
		int ang_region = angle/ptr_mvdr->m_delta_angle;
		memcpy(ptr_mvdr->m_stv_re, ptr_mvdr->m_gstv_re+ang_region*ptr_mvdr->m_gstv_dim, sizeof(float)*ptr_mvdr->m_sp_size*ptr_mvdr->m_channels);
		memcpy(ptr_mvdr->m_stv_im, ptr_mvdr->m_gstv_im+ang_region*ptr_mvdr->m_gstv_dim, sizeof(float)*ptr_mvdr->m_sp_size*ptr_mvdr->m_channels);
		ptr_mvdr->m_angle_pre = angle;
	}
	
	ptr_mvdr->m_frame_sum++;
	// add ana window		
	for ( ch_idx = 0; ch_idx < ptr_mvdr->m_channels; ++ch_idx )
	{
		dios_ssp_mvdr_win_add_ana_win(ptr_mvdr->mvdrwin, ptr_mvdr->m_mch_buffer[ch_idx], ptr_mvdr->m_win_data+ch_idx*ptr_mvdr->m_fft_size);
	}
	// stft 
	for ( ch_idx = 0; ch_idx < ptr_mvdr->m_channels; ++ch_idx )
	{
		dios_ssp_share_rfft_process(ptr_mvdr->mvdr_fft, ptr_mvdr->m_win_data+ch_idx*ptr_mvdr->m_fft_size, ptr_mvdr->fft_out);
		for (i = 0; i < ptr_mvdr->m_fft_size / 2 + 1; i++)
		{
			ptr_mvdr->m_re[i + ch_idx * ptr_mvdr->m_fft_size] = ptr_mvdr->fft_out[i];
		}
		ptr_mvdr->m_im[0] = ptr_mvdr->m_im[ptr_mvdr->m_fft_size / 2] = 0.0;
		for (i = 1; i < ptr_mvdr->m_fft_size / 2; i++)
		{
			ptr_mvdr->m_im[i + ch_idx * ptr_mvdr->m_fft_size] = -ptr_mvdr->fft_out[ptr_mvdr->m_fft_size - i];
		}
	}
	
	dios_ssp_mvdr_mcra(ptr_mvdr);

	dios_ssp_mvdr_cal_rxx(ptr_mvdr);

	dios_ssp_mvdr_cal_weights_adpmvdr(ptr_mvdr);
		
	for (k = 1; k < ptr_mvdr->m_fft_size/2; k++ )
	{
		ptr_mvdr->m_mvdr_out_re[k] = ptr_mvdr->m_mvdr_out_im[k] = 0;
		for (i = 0; i < ptr_mvdr->m_channels; ++i )
		{
			ptr_mvdr->m_mvdr_out_re[k] += ptr_mvdr->m_weight_re[k*ptr_mvdr->m_channels+i]*ptr_mvdr->m_re[i*ptr_mvdr->m_fft_size+k] + ptr_mvdr->m_weight_im[k*ptr_mvdr->m_channels+i]*ptr_mvdr->m_im[i*ptr_mvdr->m_fft_size+k];
			ptr_mvdr->m_mvdr_out_im[k] += ptr_mvdr->m_weight_re[k*ptr_mvdr->m_channels+i]*ptr_mvdr->m_im[i*ptr_mvdr->m_fft_size+k] - ptr_mvdr->m_weight_im[k*ptr_mvdr->m_channels+i]*ptr_mvdr->m_re[i*ptr_mvdr->m_fft_size+k];
		}
		ptr_mvdr->m_mvdr_out_re[ptr_mvdr->m_fft_size-k] = ptr_mvdr->m_mvdr_out_re[k];
		ptr_mvdr->m_mvdr_out_im[ptr_mvdr->m_fft_size-k] = -ptr_mvdr->m_mvdr_out_im[k];
	}
	
	ptr_mvdr->fft_in[0] = ptr_mvdr->m_mvdr_out_re[0];
	ptr_mvdr->fft_in[ptr_mvdr->m_fft_size / 2] = ptr_mvdr->m_mvdr_out_re[ptr_mvdr->m_fft_size / 2];
	for (i = 1; i < ptr_mvdr->m_fft_size / 2; i++) 
	{
		ptr_mvdr->fft_in[i] = ptr_mvdr->m_mvdr_out_re[i];
		ptr_mvdr->fft_in[ptr_mvdr->m_fft_size - i] = -ptr_mvdr->m_mvdr_out_im[i];
	}
	dios_ssp_share_irfft_process(ptr_mvdr->mvdr_fft, ptr_mvdr->fft_in, ptr_mvdr->m_win_data);
	for (i = 0; i < ptr_mvdr->m_fft_size; ++i)
	{
		ptr_mvdr->m_win_data[i] = ptr_mvdr->m_win_data[i] / ptr_mvdr->m_fft_size;
	}
	// add syn window 
	dios_ssp_mvdr_win_add_syn_win(ptr_mvdr->mvdrwin, ptr_mvdr->m_win_data, ptr_mvdr->m_re_temp);
	
	// ola		
	for (i = 0; i < ptr_mvdr->m_fft_size; ++i )
	{
		ptr_mvdr->m_out_bf[i] += ptr_mvdr->m_re_temp[i];
	}
	
	for (i = 0; i < ptr_mvdr->m_shift_size; ++i )
	{
		out[i] = ptr_mvdr->m_out_bf[i];
	}

	for (ch_idx = 0; ch_idx < ptr_mvdr->m_channels; ++ch_idx)
	{
		memmove(ptr_mvdr->m_mch_buffer[ch_idx], ptr_mvdr->m_mch_buffer[ch_idx] + ptr_mvdr->m_shift_size, sizeof(float) * (ptr_mvdr->m_fft_size - ptr_mvdr->m_shift_size));
	}
	memmove( ptr_mvdr->m_out_bf, ptr_mvdr->m_out_bf + ptr_mvdr->m_shift_size, sizeof(float) * (ptr_mvdr->m_fft_size - ptr_mvdr->m_shift_size) );
	memset( ptr_mvdr->m_out_bf + ptr_mvdr->m_fft_size - ptr_mvdr->m_shift_size, 0, sizeof(float) * ptr_mvdr->m_shift_size);

	return 0;
}

int dios_ssp_mvdr_mcra(objMVDR *ptr_mvdr)
{
    int i, k;
	for(i = 0; i < ptr_mvdr->m_channels; ++i)
	{
	
		float Srk = 0, ik = 0, adk = 0, xn_amp = 0;
	
		for(k = 0; k < ptr_mvdr->m_fft_size; ++k)
		{
			ptr_mvdr->m_ns_ps_cur_mic[k] = ptr_mvdr->m_re[i*ptr_mvdr->m_fft_size+k]*ptr_mvdr->m_re[i*ptr_mvdr->m_fft_size+k] + ptr_mvdr->m_im[i*ptr_mvdr->m_fft_size+k]*ptr_mvdr->m_im[i*ptr_mvdr->m_fft_size+k];
			ptr_mvdr->m_ns_theta_cur_mic[k] = (float)atan2(ptr_mvdr->m_im[i*ptr_mvdr->m_fft_size+k], ptr_mvdr->m_re[i*ptr_mvdr->m_fft_size+k]);
		}

		if( ptr_mvdr->m_frame_sum == 1 )
		{
			memcpy(ptr_mvdr->m_ns_ps+i*ptr_mvdr->m_fft_size, ptr_mvdr->m_ns_ps_cur_mic, sizeof(float)*ptr_mvdr->m_fft_size);
			memcpy(ptr_mvdr->m_P+i*ptr_mvdr->m_fft_size, ptr_mvdr->m_ns_ps_cur_mic, sizeof(float)*ptr_mvdr->m_fft_size);
			memcpy(ptr_mvdr->m_Ptmp+i*ptr_mvdr->m_fft_size, ptr_mvdr->m_ns_ps_cur_mic, sizeof(float)*ptr_mvdr->m_fft_size);
			memcpy(ptr_mvdr->m_Pmin+i*ptr_mvdr->m_fft_size, ptr_mvdr->m_ns_ps_cur_mic, sizeof(float)*ptr_mvdr->m_fft_size);
		}
		else
		{
			for(k = 0; k < ptr_mvdr->m_sp_size; ++k)
			{
				ptr_mvdr->m_P[i*ptr_mvdr->m_fft_size+k] = ptr_mvdr->m_alpha_s*ptr_mvdr->m_P[i*ptr_mvdr->m_fft_size+k] + (1 - ptr_mvdr->m_alpha_s)*ptr_mvdr->m_ns_ps_cur_mic[k];
			}
		}

		if( ptr_mvdr->m_frame_sum % ptr_mvdr->m_L == 0 )
		{
			for(k = 0; k < ptr_mvdr->m_sp_size; ++k)
			{
				ptr_mvdr->m_Pmin[i*ptr_mvdr->m_fft_size+k] = (ptr_mvdr->m_Ptmp[i*ptr_mvdr->m_fft_size+k] < ptr_mvdr->m_P[i*ptr_mvdr->m_fft_size+k])?ptr_mvdr->m_Ptmp[i*ptr_mvdr->m_fft_size+k]:ptr_mvdr->m_P[i*ptr_mvdr->m_fft_size+k];
				ptr_mvdr->m_Ptmp[i*ptr_mvdr->m_fft_size+k] = ptr_mvdr->m_P[i*ptr_mvdr->m_fft_size+k];
			}
		}
		else
		{
			for(k = 0; k < ptr_mvdr->m_sp_size; ++k)
			{
				ptr_mvdr->m_Pmin[i*ptr_mvdr->m_fft_size+k] = (ptr_mvdr->m_Pmin[i*ptr_mvdr->m_fft_size+k] < ptr_mvdr->m_P[i*ptr_mvdr->m_fft_size+k])?ptr_mvdr->m_Pmin[i*ptr_mvdr->m_fft_size+k]:ptr_mvdr->m_P[i*ptr_mvdr->m_fft_size+k];
				ptr_mvdr->m_Ptmp[i*ptr_mvdr->m_fft_size+k] = (ptr_mvdr->m_Ptmp[i*ptr_mvdr->m_fft_size+k] < ptr_mvdr->m_P[i*ptr_mvdr->m_fft_size+k])?ptr_mvdr->m_Ptmp[i*ptr_mvdr->m_fft_size+k]:ptr_mvdr->m_P[i*ptr_mvdr->m_fft_size+k];
			}
		}

		for(k = 0; k < ptr_mvdr->m_sp_size; ++k)
		{
			Srk = ptr_mvdr->m_P[i*ptr_mvdr->m_fft_size+k]/ptr_mvdr->m_Pmin[i*ptr_mvdr->m_fft_size+k];
			ik = (Srk > ptr_mvdr->m_delta_thres)?1.0f:0.0f;
			ptr_mvdr->m_pk[i*ptr_mvdr->m_fft_size+k] = ptr_mvdr->m_alpha_p*ptr_mvdr->m_pk[i*ptr_mvdr->m_fft_size+k] + (1 - ptr_mvdr->m_alpha_p)*ik;
			adk = ptr_mvdr->m_alpha_d + (1 - ptr_mvdr->m_alpha_d)*ptr_mvdr->m_pk[i*ptr_mvdr->m_fft_size+k];
			ptr_mvdr->m_ns_ps[i*ptr_mvdr->m_fft_size+k] = adk*ptr_mvdr->m_ns_ps[i*ptr_mvdr->m_fft_size+k] + (1 - adk)*ptr_mvdr->m_ns_ps_cur_mic[k];
		}

		for(k = 1; k < ptr_mvdr->m_fft_size/2; ++k)
		{
			ptr_mvdr->m_ns_ps[i*ptr_mvdr->m_fft_size+ptr_mvdr->m_fft_size-k] = ptr_mvdr->m_ns_ps[i*ptr_mvdr->m_fft_size+k];
		}

		for(k = 0; k < ptr_mvdr->m_fft_size; ++k)
		{
			xn_amp = (float)sqrt(ptr_mvdr->m_ns_ps[i*ptr_mvdr->m_fft_size+k]);
			ptr_mvdr->m_xn_re[i*ptr_mvdr->m_fft_size+k] = (float)(xn_amp*cos(ptr_mvdr->m_ns_theta_cur_mic[k]));
			ptr_mvdr->m_xn_im[i*ptr_mvdr->m_fft_size+k] = (float)(xn_amp*sin(ptr_mvdr->m_ns_theta_cur_mic[k]));
		}
	}

	return 0;
}

int dios_ssp_mvdr_cal_rxx(objMVDR *ptr_mvdr)
{
    int i, j, k;
	float rnn_re = 0, rnn_im = 0;
	
	if ( ptr_mvdr->m_frame_sum == 1 )
	{
		for (k = 0; k < ptr_mvdr->m_sp_size; ++k )
		{
			for (i = 0; i < ptr_mvdr->m_channels; ++i )
			{				
				rnn_re = ptr_mvdr->m_xn_re[i*ptr_mvdr->m_fft_size+k]*ptr_mvdr->m_xn_re[i*ptr_mvdr->m_fft_size+k] 
						+ptr_mvdr-> m_xn_im[i*ptr_mvdr->m_fft_size+k]*ptr_mvdr->m_xn_im[i*ptr_mvdr->m_fft_size+k] + ptr_mvdr->m_rnn_eps;
				ptr_mvdr->m_rnn_re[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+i] = rnn_re;
				
				for (j = i+1; j < ptr_mvdr->m_channels; ++j )
				{					
					rnn_re = ptr_mvdr->m_xn_re[i*ptr_mvdr->m_fft_size+k]*ptr_mvdr->m_xn_re[j*ptr_mvdr->m_fft_size+k] 
							+ ptr_mvdr->m_xn_im[i*ptr_mvdr->m_fft_size+k]*ptr_mvdr->m_xn_im[j*ptr_mvdr->m_fft_size+k];
					rnn_im = -ptr_mvdr->m_xn_re[i*ptr_mvdr->m_fft_size+k]*ptr_mvdr->m_xn_im[j*ptr_mvdr->m_fft_size+k] 
							+ ptr_mvdr->m_xn_re[j*ptr_mvdr->m_fft_size+k]*ptr_mvdr->m_xn_im[i*ptr_mvdr->m_fft_size+k];
					ptr_mvdr->m_rnn_re[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+j] = rnn_re;
					ptr_mvdr->m_rnn_im[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+j] = rnn_im;
				}
			}
		}
	}
	else
	{
		for (k = 0; k < ptr_mvdr->m_sp_size; ++k )
		{
			for (i = 0; i < ptr_mvdr->m_channels; ++i )
			{											
				rnn_re = ptr_mvdr->m_xn_re[i*ptr_mvdr->m_fft_size+k]*ptr_mvdr->m_xn_re[i*ptr_mvdr->m_fft_size+k] 
						+ ptr_mvdr->m_xn_im[i*ptr_mvdr->m_fft_size+k]*ptr_mvdr->m_xn_im[i*ptr_mvdr->m_fft_size+k] + ptr_mvdr->m_rnn_eps;
				ptr_mvdr->m_rnn_re[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+i] = ptr_mvdr->m_alpha_rnn*ptr_mvdr->m_rnn_re[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+i] 
						+ ptr_mvdr->m_beta_rnn*rnn_re;
				
				for (j = i+1; j < ptr_mvdr->m_channels; ++j )
				{						
					rnn_re = ptr_mvdr->m_xn_re[i*ptr_mvdr->m_fft_size+k]*ptr_mvdr->m_xn_re[j*ptr_mvdr->m_fft_size+k] 
							+ ptr_mvdr->m_xn_im[i*ptr_mvdr->m_fft_size+k]*ptr_mvdr->m_xn_im[j*ptr_mvdr->m_fft_size+k];
					rnn_im = -ptr_mvdr->m_xn_re[i*ptr_mvdr->m_fft_size+k]*ptr_mvdr->m_xn_im[j*ptr_mvdr->m_fft_size+k] 
							+ ptr_mvdr->m_xn_re[j*ptr_mvdr->m_fft_size+k]*ptr_mvdr->m_xn_im[i*ptr_mvdr->m_fft_size+k];
					ptr_mvdr->m_rnn_re[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+j] = ptr_mvdr->m_alpha_rnn*ptr_mvdr->m_rnn_re[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+j] 
							+ ptr_mvdr->m_beta_rnn*rnn_re;
					ptr_mvdr->m_rnn_im[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+j] = ptr_mvdr->m_alpha_rnn*ptr_mvdr->m_rnn_im[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+j] 
							+ ptr_mvdr->m_beta_rnn*rnn_im;
				}	
			}
		}
	}

	return 0;
}

int dios_ssp_mvdr_cal_weights_adpmvdr(objMVDR *ptr_mvdr)
{
    int i, j, k;
	for(k = 1; k < ptr_mvdr->m_sp_size; ++k)
	{
		for (i = 0; i < ptr_mvdr->m_channels; i++)
		{
			ptr_mvdr->m_rxx_in[i*ptr_mvdr->m_channels*2+2*i] = ptr_mvdr->m_rnn_re[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+i];
			ptr_mvdr->m_rxx_in[i*ptr_mvdr->m_channels*2+2*i+1] = ptr_mvdr->m_rnn_im[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+i];
			for (j = i+1; j < ptr_mvdr->m_channels; j++)
			{
				ptr_mvdr->m_rxx_in[i*ptr_mvdr->m_channels*2+2*j] = ptr_mvdr->m_rnn_re[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+j];
				ptr_mvdr->m_rxx_in[i*ptr_mvdr->m_channels*2+2*j+1] = ptr_mvdr->m_rnn_im[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+j];
				ptr_mvdr->m_rxx_in[j*ptr_mvdr->m_channels*2+2*i] = ptr_mvdr->m_rxx_in[i*ptr_mvdr->m_channels*2+2*j];
				ptr_mvdr->m_rxx_in[j*ptr_mvdr->m_channels*2+2*i+1] = -ptr_mvdr->m_rxx_in[i*ptr_mvdr->m_channels*2+2*j+1];
			}
		}
		dios_ssp_matrix_inv_process(ptr_mvdr->mvdrinv, ptr_mvdr->m_rxx_in, ptr_mvdr->m_irxx_out);
		for (i = 0; i < ptr_mvdr->m_channels; ++i)
		{
			for (j = 0; j < ptr_mvdr->m_channels; ++j)
			{
				ptr_mvdr->m_irnn_re[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+j] = ptr_mvdr->m_irxx_out[i*ptr_mvdr->m_channels*2+2*j];
				ptr_mvdr->m_irnn_im[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+j] = ptr_mvdr->m_irxx_out[i*ptr_mvdr->m_channels*2+2*j+1];
			}
		}
	}
	
	float re_temp = 0, im_temp = 0, power = 0, re_temp2 = 0, im_temp2 = 0;
	for (k = 1; k < ptr_mvdr->m_sp_size; k++ )	
	{		
		for (i = 0; i < ptr_mvdr->m_channels; i++)		
		{			
			re_temp = im_temp = 0;			
			for (j = 0; j < ptr_mvdr->m_channels; j++)			
			{				
				re_temp += ptr_mvdr->m_irnn_re[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+j]*ptr_mvdr->m_stv_re[k*ptr_mvdr->m_channels+j] 
						- ptr_mvdr->m_irnn_im[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+j]*ptr_mvdr->m_stv_im[k*ptr_mvdr->m_channels+j];
				im_temp += ptr_mvdr->m_irnn_re[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+j]*ptr_mvdr->m_stv_im[k*ptr_mvdr->m_channels+j] 
						+ ptr_mvdr->m_irnn_im[k*ptr_mvdr->m_rxx_size+i*ptr_mvdr->m_channels+j]*ptr_mvdr->m_stv_re[k*ptr_mvdr->m_channels+j];
			}						
			ptr_mvdr->m_weight_re[k*ptr_mvdr->m_channels+i] = re_temp;		
			ptr_mvdr->m_weight_im[k*ptr_mvdr->m_channels+i] = im_temp;		
		}					
		re_temp = im_temp = 0;		
		for (i = 0; i < ptr_mvdr->m_channels; i++ )		
		{			
			re_temp += ptr_mvdr->m_stv_re[k*ptr_mvdr->m_channels+i]*ptr_mvdr->m_weight_re[k*ptr_mvdr->m_channels+i] 
					+ ptr_mvdr->m_stv_im[k*ptr_mvdr->m_channels+i]*ptr_mvdr->m_weight_im[k*ptr_mvdr->m_channels+i];
			im_temp += ptr_mvdr->m_stv_re[k*ptr_mvdr->m_channels+i]*ptr_mvdr->m_weight_im[k*ptr_mvdr->m_channels+i] 
					- ptr_mvdr->m_stv_im[k*ptr_mvdr->m_channels+i]*ptr_mvdr->m_weight_re[k*ptr_mvdr->m_channels+i];
		}				
		power = re_temp*re_temp + im_temp*im_temp;
		for (i = 0; i < ptr_mvdr->m_channels; i++)		
		{	
			re_temp2 = (re_temp*ptr_mvdr->m_weight_re[k*ptr_mvdr->m_channels+i]+im_temp*ptr_mvdr->m_weight_im[k*ptr_mvdr->m_channels+i])/power;
			im_temp2 = (re_temp*ptr_mvdr->m_weight_im[k*ptr_mvdr->m_channels+i]-im_temp*ptr_mvdr->m_weight_re[k*ptr_mvdr->m_channels+i])/power; 

			ptr_mvdr->m_weight_re[k*ptr_mvdr->m_channels+i] = re_temp2;			
			ptr_mvdr->m_weight_im[k*ptr_mvdr->m_channels+i] = im_temp2;		
		}
	}

	return 0;
}

void dios_ssp_mvdr_delete(objMVDR *ptr_mvdr)
{
	int ret = 0;
	dios_ssp_mvdr_win_delete(ptr_mvdr->mvdrwin);
	free(ptr_mvdr->fft_out);
	free(ptr_mvdr->fft_in);
	ret = dios_ssp_share_rfft_uninit(ptr_mvdr->mvdr_fft);
	if (0 != ret)
	{
		ptr_mvdr->mvdr_fft = NULL;
	}
	ret = dios_ssp_matrix_inv_delete(ptr_mvdr->mvdrinv);
	if (0 != ret)
	{
		ptr_mvdr->mvdrinv = NULL;
	}
	dios_ssp_mvdr_free_mem(ptr_mvdr);
}

