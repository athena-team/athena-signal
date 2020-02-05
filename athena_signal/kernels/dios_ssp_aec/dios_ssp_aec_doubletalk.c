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

Description: Output the result of double-talking according to the result of 
the 1st-stage residual echo suppression.
==============================================================================*/

/* include file */
#include "dios_ssp_aec_doubletalk.h"

objDoubleTalk* dios_ssp_aec_doubletalk_init(int ref_num)
{
	int i;
	int j;
	int ret = 0;
	objDoubleTalk* srv = NULL;

	srv = (objDoubleTalk*)calloc(1, sizeof(objDoubleTalk));

	srv->ref_num = ref_num;

	srv->dt_num_bands = (int)(((float)DT_FREQ_HI - DT_FREQ_LO) / DT_FREQ_DELTA + 0.5);
	
	srv->doubletalk_band_table = (int **)calloc(srv->dt_num_bands, sizeof(int*));
	for (i = 0; i<srv->dt_num_bands; i++)
	{
		srv->doubletalk_band_table[i] = (int *)calloc(2, sizeof(int));
	}
	
	srv->res1_psd = (float*)calloc(AEC_SUBBAND_NUM, sizeof(float));
	srv->res1_sum = (float *)calloc(srv->dt_num_bands, sizeof(float));
	srv->res1_eng_avg_buf = (float *)calloc(DT_RES1_ENG_BUF_LEN, sizeof(float));
	srv->res1_min_avg_buf = (float *)calloc(DT_RES1_MIN_BUF_LEN, sizeof(float));
	
	srv->mic_noiselevel_sum = (float *)calloc(srv->dt_num_bands, sizeof(float));

	/* double talk band table init */
	srv->doubletalk_band_table[0][0] = (int)((float)DT_FREQ_LO * AEC_FFT_LEN / AEC_SAMPLE_RATE);
	for (i = DT_FREQ_LO + DT_FREQ_DELTA, j = 1; i < DT_FREQ_HI; i += DT_FREQ_DELTA, j++)
	{
		srv->doubletalk_band_table[j][0] = (int)((float)i * AEC_FFT_LEN / AEC_SAMPLE_RATE);
		srv->doubletalk_band_table[j - 1][1] = srv->doubletalk_band_table[j][0] - 1;
	}
	srv->doubletalk_band_table[j - 1][1] = (int)((float)DT_FREQ_HI * AEC_FFT_LEN / AEC_SAMPLE_RATE);
	
	ret = dios_ssp_aec_doubletalk_reset(srv);
	if (0 != ret)
	{
		return NULL;
	}
	
	return srv;
}

int dios_ssp_aec_doubletalk_reset(objDoubleTalk* srv)
{
	int i;
	if (NULL == srv)
	{
		return ERR_AEC;
	}

	srv->dt_num_hangover = 10; // frame number
	srv->res1_eng_avg = 0.0f;

	for (i = 0; i<srv->dt_num_bands; i++)
	{
		srv->res1_sum[i] = 0.0f;
		srv->mic_noiselevel_sum[i] = 0.0f;
	}
	memset(srv->res1_eng_avg_buf, 0, DT_RES1_ENG_BUF_LEN * sizeof(float));
	memset(srv->res1_min_avg_buf, 0, DT_RES1_MIN_BUF_LEN * sizeof(float));

	srv->dt_cnt = 0;
	srv->dt_frame_cnt = 0;
	srv->dt_st = SINGLE_TALK_STATUS;	

	return 0;
}

int dios_ssp_aec_doubletalk_process(objDoubleTalk* srv, int* dt_st)
{
	int i;
	int ch;

	if (NULL == srv)
	{
		return ERR_AEC;
	}

	float res1_eng_tmp = 0.0f;
	float res1_eng = 0.0f;
	int dtd_band_used = srv->dt_num_bands/2;


	for (i = 0; i < dtd_band_used; i++)
	{
		srv->res1_sum[i] = 0.0f;
		srv->mic_noiselevel_sum[i] = 0.0f;

		for (ch = srv->doubletalk_band_table[i][0]; ch <= srv->doubletalk_band_table[i][1]; ch++)
		{
			srv->res1_sum[i] += srv->res1_psd[ch];  // the 1st-stage res output psd
			srv->mic_noiselevel_sum[i] += srv->mic_noise_bin[ch]->noise_level_first;
		}

		res1_eng_tmp = srv->res1_sum[i] - 1.0f * srv->mic_noiselevel_sum[i];
		res1_eng_tmp = (res1_eng_tmp > 0) ? res1_eng_tmp : 0;
		res1_eng += res1_eng_tmp;
	}
	res1_eng /= dtd_band_used;

	srv->res1_eng_avg = DT_RES1_ENG_ALPHA * srv->res1_eng_avg + (1.0f - DT_RES1_ENG_ALPHA) * res1_eng;

	/* find the wined min value for the 1st-stage res average value */
	memmove(srv->res1_min_avg_buf, srv->res1_min_avg_buf + 1, (DT_RES1_MIN_BUF_LEN - 1) * sizeof(float));
	srv->res1_min_avg_buf[DT_RES1_MIN_BUF_LEN - 1] = srv->res1_eng_avg;

	float min_res1_tmp = 0.0f;
	for (i = 0; i < DT_RES1_MIN_BUF_LEN; i++)
	{
		min_res1_tmp += srv->res1_min_avg_buf[i];
	}
	min_res1_tmp /= DT_RES1_MIN_BUF_LEN;

	memmove(srv->res1_eng_avg_buf, srv->res1_eng_avg_buf + 1, (DT_RES1_ENG_BUF_LEN - 1) * sizeof(float));
	srv->res1_eng_avg_buf[DT_RES1_ENG_BUF_LEN - 1] = min_res1_tmp;

	float min_res1 = srv->res1_eng_avg_buf[0];
	for (i = 1; i < DT_RES1_ENG_BUF_LEN; i++)
	{
		if (min_res1 > srv->res1_eng_avg_buf[i])
		{
			min_res1 = srv->res1_eng_avg_buf[i];
		}
	}
	
	float dtd_thr = srv->dt_thr_factor * min_res1;
	dtd_thr = dtd_thr > srv->dt_min_thr ? dtd_thr : srv->dt_min_thr;

	/* hangover for doubletalk status, starting frames as single talk */
	if (srv->dt_frame_cnt < AEC_SINGLE_TALK_FRAMES)
	{
		srv->dt_frame_cnt++;
		srv->dt_st = SINGLE_TALK_STATUS;
	}
	else
	{
		if (srv->res1_eng_avg > dtd_thr)
		{
			srv->dt_cnt = srv->dt_num_hangover;
		}
		else
		{
			if (srv->dt_cnt > 0)
			{
				srv->dt_cnt--;
			}			
		}

		if (srv->dt_cnt > 0 && srv->far_end_talk_holdtime != 0)
		{
			srv->dt_st = DOUBLE_TALK_STATUS; // near end + far end
		}
		else
		{
			if (srv->far_end_talk_holdtime == 0)
			{
				srv->dt_st = NEAREND_TALK_STATUS; // near end only
			}
			else
			{
				srv->dt_st = SINGLE_TALK_STATUS; // far end only
			}
		}		
	}

	dt_st[0] = srv->dt_st;
	return 0;
}

int dios_ssp_aec_doubletalk_uninit(objDoubleTalk* srv)
{
	int i;

	if (NULL == srv)
	{
		return ERR_AEC;
	}
	for (i = 0; i<srv->dt_num_bands; i++)
	{
		free(srv->doubletalk_band_table[i]);
	}
	free(srv->doubletalk_band_table);

	free(srv->res1_psd);	
	free(srv->res1_sum);
	free(srv->res1_eng_avg_buf);
	free(srv->res1_min_avg_buf);
	free(srv->mic_noiselevel_sum);
	free(srv);
	return 0;
}
