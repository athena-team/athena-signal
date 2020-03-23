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

Description: Noise reduction algorithm is based on MCRA noise estimation method, 
which includes the following steps:
1. The data goes through the analysis window and FFT.
2. MCRA estimation of noise.
3. Calculate gain factor according to MMSE criterion.
4. Noise reduction based on gain and IFFT.
Details can be found in "Noise Estimation by Minima Controlled Recursive
Averaging for Robust Speech Enhancement" and "Noise Spectrum Estimation in
Adverse Environments: Improved Minima Controlled Recursive Averaging"
==============================================================================*/

#include "dios_ssp_ns_api.h"

#define PI (3.141592653589793f)

static float a[7] = { 1.0f, 3.5156229f, 3.0899424f, 1.2067492f, 0.2659732f, 0.0360768f, 0.0045813f };
static float b[7] = { 0.5f, 0.87890594f, 0.51498869f, 0.15084934f,0.02658773f,0.00301532f,0.00032411f}; 
static float c[9] = { 0.39894228f, 0.01328592f, 0.00225319f, -0.00157565f, 0.00916281f, 
						-0.02057706f, 0.02635537f, -0.01647633f, 0.00392377f };
static float d[9] = { 0.39894228f, -0.03988024f, -0.00362018f, 0.00163801f, -0.01031555f, 
						0.02282967f, -0.02895312f, 0.01787654f, -0.00420059f };                       
static float first_modified_Bessel( int n, float x )    
{    
	int i;
	float t = (float)fabs( x ); 
	float y;
	float p = 0;
	if ( n == 0 )
	{
		if ( t < 3.75 )    
		{     
			y = ( x / 3.75f ) * ( x / 3.75f ); 
			p=a[6];    
			for ( i = 5; i >= 0; i-- )    
			{
				p = p * y + a[i];    
			}
		}    
		else     
		{     
			y = 3.75f / t; 
			p = c[8];    
			for ( i = 7; i >= 0; i-- )    
			{
				p = p*y + c[i];    
			}
			p = p * (float)exp(t) / (float)sqrt(t);    
		}  
	} // n = 0
	else if ( n == 1 )
	{
		if ( t < 3.75 )    
		{     
			y = ( x/3.75f ) * ( x/3.75f ); 
			p = b[6];    
			for ( i = 5; i >= 0; i-- ) 
			{
				p=p*y+b[i];    
			}
			p = p*t;    
		}    
		else    
		{     
			y = 3.75f / t;
			p = d[8];    
			for ( i = 7; i >= 0; i-- ) 
			{
				p = p * y + d[i];
			}
			p = p * (float)exp(t) / (float)sqrt(t);   
			if ( x < 0 )
			{
				p *= -1;
			}
		}    
	} // n = 1
	return p;
}

typedef struct {
    int frame_len;
    int m_wav_len2;
	int m_buffer_len;
	int m_max_pack_len;
    float *m_out_mmse_data;

    //mmse_process
    int m_shift_size;
    int m_fft_size;
    int m_frame_sum;
    float *m_wav_buffer;
    float *m_out_buffer;
    float *m_win_wav;
    float* m_re;
    float* m_im;

    //anaylsis window & synthesis window
    float *m_ana_win;
	float *m_syn_win;
	float *m_norm_win;

    //stft_process & istft_process
    int m_log_fft_size;
    int *m_rev;
    float *m_sin_fft;
	float *m_cos_fft;
	float *m_tmp;
	float *fftin_buffer;
	float *fft_out;
	void *rfft_param;

    //mcra2
	int	m_ind_2k;
	int	m_ind_4k;
    int	m_ind_6k;
    float m_thres_02;
    float m_thres_24;
    float m_thres_46;
    float m_thres_68;
    int m_sp_size;
    int m_freq_win_len;
    float *m_sp_smooth;
    float *m_sp;
    float *m_freq_win;
    int	m_ini_frame_num;
    float *m_sp_ff;
    float *m_sp_sf;
    float *m_sp_ff_pre;
    float *m_sp_noise;
    float m_alfa_ff;
    float m_alfa_sf;
    float m_beta_sf;
    float *m_ratio;
    float m_eps;
    float *m_thres;
    char *m_I;
    float m_global_prob;
    float m_alfa_fb_prob;
    float *m_prob;
    float m_alfa_prob;
    float m_alfa_noise;

    //mmse_gain
    float *m_gammak;
    float m_gammak_min;
    float *m_sp_snr;
    float m_alfa_snr;
    float *m_gain;
     float m_min_gain;
} objNSMMSE;

void* dios_ssp_ns_init_api(int frame_len)
{
	void* ptr = NULL;
	ptr = (void*)calloc(1, sizeof(objNSMMSE));
	objNSMMSE* srv = (objNSMMSE*)ptr;

	srv->frame_len = frame_len;
	srv->m_wav_len2 = 0;
	srv->m_buffer_len = 5120;
	srv->m_max_pack_len = NS_SAMPLE_RATE + 512;
    srv->m_out_mmse_data = (float *)calloc(2 * srv->frame_len, sizeof(float));
	
	srv->m_shift_size = NS_FFT_LEN / 2;
    srv->m_fft_size  = NS_FFT_LEN;
    srv->m_frame_sum = 0;
    srv->m_wav_buffer = (float *)calloc(srv->m_max_pack_len, sizeof(float));
    srv->m_out_buffer = (float *)calloc(srv->m_max_pack_len, sizeof(float));
    srv->m_win_wav = (float *)calloc(srv->m_fft_size, sizeof(float));
    srv->m_re = (float *)calloc(srv->m_fft_size, sizeof(float));
    srv->m_im = (float *)calloc(srv->m_fft_size, sizeof(float));

	srv->m_ana_win = (float *)calloc(srv->m_fft_size, sizeof(float));
	srv->m_syn_win = (float *)calloc(srv->m_fft_size, sizeof(float));
	srv->m_norm_win = (float *)calloc(srv->m_fft_size, sizeof(float));

	srv->m_log_fft_size = 0;
    srv->m_rev = (int *)calloc(srv->m_fft_size, sizeof(int));
    srv->m_sin_fft = (float *)calloc(srv->m_fft_size, sizeof(float));
	srv->m_cos_fft = (float *)calloc(srv->m_fft_size, sizeof(float));
	srv->m_tmp = (float *)calloc(srv->m_fft_size, sizeof(float));
	srv->fftin_buffer = (float*)calloc(NS_FFT_LEN, sizeof(float));
	srv->fft_out = (float *)calloc(srv->m_fft_size, sizeof(float));
	srv->m_ind_2k = 2000 * srv->m_fft_size / NS_SAMPLE_RATE;//m_fs-sample
	srv->m_ind_4k = 4000 * srv->m_fft_size / NS_SAMPLE_RATE;
    srv->m_ind_6k = 6000 * srv->m_fft_size / NS_SAMPLE_RATE;
    srv->m_thres_02 = 2.0f;
    srv->m_thres_24 = 2.0f;
    srv->m_thres_46 = 2.0f;
    srv->m_thres_68 = 2.0f;
	srv->m_sp_size = srv->m_fft_size / 2 +1;
    srv->m_freq_win_len = 8;//DEFAULT_MMSE_HALF_FRQ_WIN_LEN
    srv->m_sp_smooth = (float *)calloc(srv->m_fft_size / 2 + 1, sizeof(float));
    srv->m_sp = (float *)calloc(srv->m_fft_size / 2 + 1, sizeof(float));
    srv->m_freq_win = (float *)calloc(2 * srv->m_freq_win_len + 1, sizeof(float));
    srv->m_ini_frame_num = 20;
    srv->m_sp_ff = (float *)calloc(srv->m_fft_size / 2 + 1, sizeof(float));
    srv->m_sp_sf = (float *)calloc(srv->m_fft_size / 2 + 1, sizeof(float));
    srv->m_sp_ff_pre = (float *)calloc(srv->m_fft_size / 2 + 1, sizeof(float));
    srv->m_sp_noise = (float *)calloc(srv->m_fft_size / 2 + 1, sizeof(float));
    srv->m_alfa_ff = 0.9f;
    srv->m_alfa_sf = 0.99f;
    srv->m_beta_sf = 0.96f;
    srv->m_ratio = (float *)calloc(srv->m_fft_size / 2 + 1, sizeof(float));
    srv->m_eps = 1073.0f;
    srv->m_thres = (float *)calloc(srv->m_fft_size / 2 + 1, sizeof(float));
    srv->m_I = (char *)calloc(srv->m_fft_size / 2 + 1, sizeof(char));
    srv->m_global_prob = 1.0f;
    srv->m_alfa_fb_prob = 0.0;
    srv->m_prob = (float *)calloc(srv->m_fft_size / 2 + 1, sizeof(float));
    srv->m_alfa_prob = 0.99f;
    srv->m_alfa_noise = 0.999f;

	srv->m_gammak = (float *)calloc(srv->m_fft_size / 2 + 1, sizeof(float));
    srv->m_gammak_min = 40.0f;
    srv->m_sp_snr = (float *)calloc(srv->m_fft_size / 2 + 1, sizeof(float));
    srv->m_alfa_snr = 0.9f;
    srv->m_gain = (float *)calloc(srv->m_fft_size / 2 + 1, sizeof(float));
    srv->m_min_gain = 0.25f;

	//FFT initialization
    srv->rfft_param = dios_ssp_share_rfft_init(NS_FFT_LEN);
	
	return ptr;
}    

int dios_ssp_ns_reset_api(void* ptr)
{
	if (NULL == ptr) {
		return -1;
	}
	objNSMMSE* srv = (objNSMMSE*)ptr;
	int i, j;
	float temp;
	int m_block_num;

	m_block_num = srv->m_fft_size / srv->m_shift_size;

	srv->m_wav_len2 = 0;
	
	for (j = 0; j <2 * srv->frame_len; j++)	{srv->m_out_mmse_data[j] = 0.0;}

	srv->m_frame_sum = 0;
	for (j = 0; j <srv->m_max_pack_len; j++)
	{
		srv->m_wav_buffer[j] = 0.0;
		srv->m_out_buffer[j] = 0.0;
	}
	for (j = 0; j <srv->m_fft_size; j++)
	{
		srv->m_win_wav[j] = 0.0;
		srv->m_re[j] = 0.0;
		srv->m_im[j] = 0.0;
		srv->m_ana_win[j] = 0.54f - 0.46f * (float)cos( (2*j)*PI/(srv->m_fft_size-1));
		
	}
	for (j = 0; j <srv->m_fft_size; j++)
	{
		srv->m_norm_win[j] = srv->m_ana_win[j] * srv->m_ana_win[j];
	}
	for (i = 0; i <srv->m_fft_size; i++)
	{
        temp = 0;
        for (j = 0; j < m_block_num; ++j )
        {
            temp += srv->m_norm_win[i+j*srv->m_shift_size];
        }
        srv->m_norm_win[i] = 1.0f / temp;
    }
	for (i = 0; i < srv->m_shift_size; ++i )
    {
        for (j = 1; j < m_block_num; ++j )
        {
            srv->m_norm_win[i+j*srv->m_shift_size] = srv->m_norm_win[i];
        }
    }
    for (i = 0; i < srv->m_fft_size; ++i )
    {
        srv->m_syn_win[i] = srv->m_norm_win[i] * srv->m_ana_win[i]; 
    }

	srv->m_log_fft_size = 0;
	int tmp = 1;
	while ( tmp < srv->m_fft_size )
	{
		srv->m_log_fft_size += 1;
		tmp *= 2;
	}
	for (i = 0; i < srv->m_fft_size; i++)
    {
        srv->m_rev[i] = 0;
        tmp = i;
        for (j = 0; j < srv->m_log_fft_size; ++j)
        {
            srv->m_rev[i] = (srv->m_rev[i]<<1) | (tmp&1); // rev_i = rev_i*2+tmp%2
            tmp = tmp>>1;
        }
    }
	for (i = 0; i < srv->m_fft_size/2; ++i )
    {
        srv->m_sin_fft[i] = (float)sin( 2*PI*i/srv->m_fft_size ); 
        srv->m_cos_fft[i] = (float)cos( 2*PI*i/srv->m_fft_size );
    }
	for (i = 0; i < srv->m_fft_size; ++i )
    {
        srv->m_tmp[i] = 0.0;
		srv->fft_out[i] = 0.0;
    }

	for (i = 0; i <srv->m_fft_size/2 + 1; ++i)
	{
		srv->m_sp_smooth[i] = 0.0;
		srv->m_sp[i] = 0.0;
		srv->m_sp_ff[i] = 0.0;
		srv->m_sp_sf[i] = 0.0;
		srv->m_sp_ff_pre[i] = 0.0;
		srv->m_sp_noise[i] = 0.0;
		srv->m_ratio[i] = 0.0;
		
		srv->m_prob[i] = 0.0;
		srv->m_gammak[i] = 0.0;
		srv->m_gain[i] = 0.0;
		srv->m_sp_snr[i] = 40.0;
	}
	temp = 1.0f / ( srv->m_freq_win_len + 1 );
    for (i = 0; i < srv->m_freq_win_len; ++i )
    {
        srv->m_freq_win[i] = (i+1) * temp;
        srv->m_freq_win[2*srv->m_freq_win_len-i] = (i+1) * temp;
    }
    srv->m_freq_win[srv->m_freq_win_len] = 1.0;
	for (i = 0; i < srv->m_ind_2k; ++i ) // 0Hz ~ 1kHz
	{
		srv->m_thres[i] = srv->m_thres_02;
	}
	for (i = srv->m_ind_2k; i < srv->m_ind_4k; ++i ) // 1kHz ~ 2kHz
	{
		srv->m_thres[i] = srv->m_thres_24;
	}
	for (i = srv->m_ind_4k; i < srv->m_ind_6k; ++i ) // 2kHz ~ 3kHz
	{
		srv->m_thres[i] = srv->m_thres_46;
	}	
	for (i = srv->m_ind_6k; i < srv->m_sp_size; ++i ) // 2kHz ~ 3kHz
	{
		srv->m_thres[i] = srv->m_thres_68;
	}
	return 0;
}

void add_ana_win(objNSMMSE* srv, float *x, float *x_win )
{
    int i;
	for (i = 0; i < srv->m_fft_size; ++i )
	{ 
		x_win[i] = x[i] * srv->m_ana_win[i];
	}
}

void add_syn_win(objNSMMSE* srv, float *x, float *x_win )
{
    int i;
	for (i = 0; i < srv->m_fft_size; ++i )
	{ 
		x_win[i] = x[i] * srv->m_syn_win[i];
	}
}

int mcra2(objNSMMSE* srv)
{
    int i, j;
	float tmp;
	// sp
	for ( i = 0; i < srv->m_sp_size; ++i )
	{
		srv->m_sp[i] = srv->m_re[i]*srv->m_re[i] + srv->m_im[i] * srv->m_im[i];
	}
	// smooth in frequency domain
    for ( i = 1; i < srv->m_freq_win_len; ++i )
    {
        srv->m_sp_smooth[i] = 0;
        tmp = 0;
        for ( j = 0; j <= i+srv->m_freq_win_len; ++j  )
        {
            srv->m_sp_smooth[i] += srv->m_sp[j] * srv->m_freq_win[j-i+srv->m_freq_win_len]; 
            tmp += srv->m_freq_win[j-i+srv->m_freq_win_len];
        }
        srv->m_sp_smooth[i] /= tmp;
    }
    for ( i = srv->m_freq_win_len; i < srv->m_sp_size-1-srv->m_freq_win_len; ++i )
    {
        srv->m_sp_smooth[i] = 0;
        tmp = 0;
        for ( j = i-srv->m_freq_win_len; j <= i+srv->m_freq_win_len; ++j )
        {
            srv->m_sp_smooth[i] += srv->m_sp[j] * srv->m_freq_win[j-i+srv->m_freq_win_len]; 
            tmp += srv->m_freq_win[j-i+srv->m_freq_win_len];
        }
        srv->m_sp_smooth[i] /= tmp;
    }
    for ( i = srv->m_sp_size-1-srv->m_freq_win_len; i < srv->m_sp_size-1; ++i )
    {
        srv->m_sp_smooth[i] = 0;
        tmp = 0;
        for ( j = i-srv->m_freq_win_len; j < srv->m_sp_size; ++j )
        {
            srv->m_sp_smooth[i] += srv->m_sp[j] * srv->m_freq_win[j-i+srv->m_freq_win_len]; 
            tmp += srv->m_freq_win[j-i+srv->m_freq_win_len];
        }
        srv->m_sp_smooth[i] /= tmp;
    }
	if ( srv->m_frame_sum <= srv->m_ini_frame_num )
	{ 
		for ( i = 0; i < srv->m_sp_size; ++i )
        {
            srv->m_sp_ff[i] += srv->m_sp_smooth[i] / srv->m_ini_frame_num;
            srv->m_sp_sf[i] += srv->m_sp_smooth[i] / srv->m_ini_frame_num;
			srv->m_sp_ff_pre[i] += srv->m_sp_smooth[i] / srv->m_ini_frame_num;
			
			srv->m_sp_noise[i] = 0;
		}
		return 0;
	}
	// ff smooth
	for ( i = 0; i < srv->m_sp_size; ++i )
    {
        srv->m_sp_ff[i] = srv->m_alfa_ff * srv->m_sp_ff[i] + (1-srv->m_alfa_ff) * srv->m_sp_smooth[i];
    }
    // sf smooth
    for ( i = 0; i < srv->m_sp_size; ++i )
    {
        if ( srv->m_sp_sf[i] < srv->m_sp_ff[i] )
        {
            srv->m_sp_sf[i] = srv->m_alfa_sf* srv->m_sp_sf[i] + (1 - srv->m_alfa_sf) * (srv->m_sp_ff[i]-srv->m_beta_sf* srv->m_sp_ff_pre[i]) / ( 1-srv->m_beta_sf );
        }
        else 
        {
            srv->m_sp_sf[i] = srv->m_sp_ff[i];
        }
    }
	// ratio
	for ( i = 0; i < srv->m_sp_size; ++i )
	{
		srv->m_ratio[i] = srv->m_sp_ff[i] / (srv->m_sp_sf[i] + srv->m_eps);
	}
	// hard decision
	for ( i = 0; i < srv->m_sp_size; ++i )
	{
		if ( srv->m_ratio[i] > srv->m_thres[i] )
		{
			srv->m_I[i] = 1;
		} 
		else
		{
			srv->m_I[i] = 0;
		}
	}
	tmp = 0;
	for ( i = 0; i < srv->m_sp_size; ++i )
	{
		tmp += srv->m_I[i];
	}
	tmp /= srv->m_sp_size;
	srv->m_global_prob = srv->m_alfa_fb_prob*srv->m_global_prob + (1-srv->m_alfa_fb_prob)*tmp;
	// speech present probability
	for ( i = 0; i < srv->m_sp_size; ++i )
	{
		srv->m_prob[i] = ( 1-srv->m_alfa_prob )*srv->m_prob[i] + srv->m_alfa_prob*srv->m_I[i];
	}
	// noise estimation
	float alfa_noise;
	for ( i = 0; i < srv->m_sp_size; ++i )
	{
		alfa_noise = srv->m_alfa_noise + (1-srv->m_alfa_noise)*srv->m_prob[i]; 
		srv->m_sp_noise[i] = alfa_noise* srv->m_sp_noise[i] + (1-alfa_noise)*srv->m_sp[i];
	}
	// prepare for next frame  
	for ( i = 0; i < srv->m_sp_size; ++i )
	{
		srv->m_sp_ff_pre[i] = srv->m_sp_ff[i];
	}
	return 0;
}

int mmse_gain(objNSMMSE* srv)
{
    float tmp;
    int i;
	for (i = 0; i < srv->m_sp_size; ++i )
	{
		tmp = srv->m_sp[i]/(srv->m_sp_noise[i]+srv->m_eps);
		srv->m_gammak[i] = tmp < srv->m_gammak_min ? tmp : srv->m_gammak_min;
	}
	for (i = 0; i < srv->m_sp_size; ++i )
	{
		tmp = (srv->m_gammak[i]-1) > 0 ? (srv->m_gammak[i]-1) : 0;
		//ksi[i] = ( ksi[i] > ksi_min ? ksi[i] : ksi_min );
		srv->m_sp_snr[i] = srv->m_alfa_snr*srv->m_sp_snr[i]+(1-srv->m_alfa_snr)*tmp;
	}
	
	float vk, j00, j11, tmpA, tmpB, tmpC; 
	float evk, Lambda, pSAP;
	for (i = 0; i < srv->m_sp_size; ++i )
	{
		vk = srv->m_sp_snr[i]*srv->m_gammak[i]/(1+srv->m_sp_snr[i]);
		j00 = first_modified_Bessel( 0, vk/2 );
		j11 = first_modified_Bessel( 1, vk/2 );
		tmpC = (float)exp( -0.5*vk );
        if (srv->m_gammak[i] < 1.0e-3)
        {
            tmpA = 0; // Limitation 
        }   
        else
        {
            tmpA = (float)sqrt(PI) / 2 * (float)pow( vk, 0.5 ) * tmpC / srv->m_gammak[i] ;
        }
		tmpB = (1+vk)*j00+vk*j11;
		evk = (float)exp( vk );
		Lambda = (1-0.3f) / 0.3f * evk / ( 1+srv->m_sp_snr[i] );
		pSAP = Lambda/(Lambda+1 );
		
		tmp = tmpA*tmpB*pSAP;
		srv->m_gain[i] = tmp;
	}
	for (i = 0; i < srv->m_sp_size; ++i )
	{	
		if (srv->m_gain[i] < srv->m_min_gain )
		{
			srv->m_gain[i] = srv->m_min_gain;
		}
		else if ( srv->m_gain[i] >= 1 )
		{
			srv->m_gain[i] = 1;
		}
	}
	for (i = 1; i < srv->m_fft_size/2; ++i )
	{
		srv->m_re[i] *= srv->m_gain[i];
		srv->m_im[i] *= srv->m_gain[i];
		srv->m_re[srv->m_fft_size-i] = srv->m_re[i];
		srv->m_im[srv->m_fft_size-i] = -srv->m_im[i];
	}

	return 0;
}

int mmse_process(objNSMMSE* srv, float *in_data, float *out_data)
{
    int i;
	// input
	for (i = 0 ; i < srv->frame_len; ++i )
	{
		srv->m_wav_buffer[i+srv->m_wav_len2] = in_data[i];
	}
	srv->m_wav_len2 += srv->frame_len;
	// mmse gain
	int sta;
	for ( sta = 0; sta + srv->m_fft_size <= srv->m_wav_len2; sta += srv->m_shift_size )
	{
		srv->m_frame_sum ++;
		// 1. add anaylsis window
		add_ana_win(srv, srv->m_wav_buffer+sta, srv->m_win_wav); 
		// 2. stft
		dios_ssp_share_rfft_process(srv->rfft_param, srv->m_win_wav, srv->fft_out);
		for (i = 0; i < NS_SUBBAND_NUM; i++)
		{
			srv->m_re[i] = srv->fft_out[i];
		}
		srv->m_im[0] = srv->m_im[NS_FFT_LEN / 2] = 0.0;
		for (i = 1; i < NS_SUBBAND_NUM - 1; i++)
		{
			srv->m_im[i] = -srv->fft_out[NS_FFT_LEN - i];
		}
		// 3. noise estimation
		mcra2(srv);
		// 4. mmse gain
		mmse_gain(srv);
		// 5. istft
		srv->fftin_buffer[0] = srv->m_re[0];
		srv->fftin_buffer[srv->frame_len] = srv->m_re[srv->frame_len];
		for (i = 1; i < srv->frame_len; i++) 
		{
			srv->fftin_buffer[i] = srv->m_re[i];
			srv->fftin_buffer[NS_FFT_LEN - i] = -srv->m_im[i];
		}
		dios_ssp_share_irfft_process(srv->rfft_param, srv->fftin_buffer, srv->m_win_wav);
		for (i = 0; i < srv->m_fft_size; ++i)
		{
			srv->m_win_wav[i] = srv->m_win_wav[i] / srv->m_fft_size;//FFT coefficient 1/N
		}

		// 6. add synthesis window
		add_syn_win(srv, srv->m_win_wav, srv->m_re); 
		// 7. ola
		for (i = 0; i < srv->m_fft_size; ++i )
		{
			srv->m_out_buffer[i+sta] += srv->m_re[i];
		}
	}
	
	for (i = 0; i < sta; ++i )
	{
		if ( srv->m_out_buffer[i] > 32767 )
		{
			out_data[i] = 32767;
		}
		else if ( srv->m_out_buffer[i] < -32768 ) 
		{
			out_data[i] = -32768;
		}
		else
		{
			out_data[i] = srv->m_out_buffer[i];
		}
	}
	
	memmove( srv->m_out_buffer, srv->m_out_buffer+sta, sizeof(float)*(srv->m_fft_size-srv->m_shift_size) );
	memset( srv->m_out_buffer+srv->m_fft_size-srv->m_shift_size, 0, sizeof(float)*sta ); 
	memmove( srv->m_wav_buffer, srv->m_wav_buffer+sta, sizeof(float)*(srv->m_wav_len2-sta) );	
	srv->m_wav_len2 -= sta;

	return 0;
}

int dios_ssp_ns_process(void *ptr, float *in_data)
{
    if (NULL == ptr) {
		return -1;
	}

    int i;
    objNSMMSE* srv = (objNSMMSE*)ptr;
    
	mmse_process(srv, in_data, srv->m_out_mmse_data);

	for(i = 0; i<srv->frame_len; i++)
	{
		in_data[i] = srv->m_out_mmse_data[i];
	}
	return 0;
}

int dios_ssp_ns_uninit_api(void* ptr)
{
	int ret;
	if (NULL == ptr) {
		return -1;
	}
	objNSMMSE *srv = (objNSMMSE *)ptr;

	free(srv->m_out_mmse_data);
	free(srv->m_wav_buffer);
	free(srv->m_out_buffer);
	free(srv->m_win_wav);
	free(srv->m_re);
	free(srv->m_im);
	free(srv->m_ana_win);
	free(srv->m_syn_win);
	free(srv->m_norm_win);

	free(srv->m_rev);
	free(srv->m_sin_fft);
	free(srv->m_cos_fft);
	free(srv->m_tmp);
	free(srv->fftin_buffer);
	free(srv->fft_out);
	free(srv->m_sp_smooth);
	free(srv->m_sp);
	free(srv->m_freq_win);
	free(srv->m_sp_ff);
	free(srv->m_sp_sf);
	free(srv->m_sp_ff_pre);
	free(srv->m_sp_noise);
	free(srv->m_ratio);
	free(srv->m_thres);

	free(srv->m_I);
	free(srv->m_prob);
	free(srv->m_gammak);
	free(srv->m_sp_snr);
	free(srv->m_gain);
	
	ret = dios_ssp_share_rfft_uninit(srv->rfft_param);
	if (0 != ret)
	{
		srv->rfft_param = NULL;
	}

	free(srv);
	return 0;
}
