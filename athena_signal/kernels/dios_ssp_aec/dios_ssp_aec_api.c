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

Description: The core of the AEC algorithm includes time delay estimation, 
linear echo cancellation, double-talk detection, echo return loss estimation
and residual echo suppression. 
==============================================================================*/

/* include file */
#include "dios_ssp_aec_api.h"

/* aec struct define */
typedef struct {
	/* module structure definition */
	objTDE* st_tde;
	objSubBand** st_subband_mic;
	objSubBand** st_subband_ref;
	objFirFilter** st_firfilter;
	objRES*** st_res;
	objDoubleTalk** st_doubletalk;
	objNoiseLevel** st_noise_est_spk_t;
	objNoiseLevel*** st_noise_est_spk_subband;

	/* buffer definition */
	float** input_mic_time;
	float** input_ref_time;
	xcomplex** input_mic_subband;
	xcomplex** input_ref_subband;
	xcomplex** firfilter_out;
	xcomplex** final_out;
	xcomplex** est_echo;
	int** band_table;
	float** ref_psd;
	float* spk_part_band_energy;
	float freq_div_table[5];
	float** spk_peak;
	int mic_num;
	int ref_num;
	int frm_len;
	float *abs_ref_avg;
	float *mic_tde;
	float *ref_tde;
	int ref_buffer_len;//for ref fix delay
	float *ref_buffer; //for ref fix delay

	/* some variable definition */
	int far_end_talk_holdtime;
	int* doubletalk_result;
}objAEC;

void* dios_ssp_aec_init_api(int mic_num, int ref_num, int frm_len)
{
	int i;
	int i_mic;
	int i_ref;
	int ret = 0;
	void* ptr = NULL;

	if (mic_num <= 0 || ref_num <= 0 || frm_len != 128)
	{
		return NULL;
	}

	ptr = (void*)calloc(1,sizeof(objAEC));
	objAEC* srv = (objAEC*)ptr;
	
	srv->mic_num = mic_num;
	srv->ref_num = ref_num;
	srv->frm_len = frm_len;

	srv->ref_buffer_len = AEC_REF_FIX_DELAY;

	/* buffer memory allocate */
	/* mic number related */
	srv->mic_tde = (float*)calloc(srv->mic_num * srv->frm_len, sizeof(float));
	srv->doubletalk_result = (int *)calloc(srv->mic_num, sizeof(int));
	srv->input_mic_time = (float**)calloc(srv->mic_num, sizeof(float*));
	srv->input_mic_subband = (xcomplex**)calloc(srv->mic_num, sizeof(xcomplex*));
	srv->firfilter_out = (xcomplex**)calloc(srv->mic_num, sizeof(xcomplex*));
	srv->final_out = (xcomplex**)calloc(srv->mic_num, sizeof(xcomplex*));
	srv->est_echo = (xcomplex**)calloc(srv->mic_num, sizeof(xcomplex*));
	srv->st_subband_mic = (objSubBand**)calloc(srv->mic_num, sizeof(objSubBand*));
	srv->st_firfilter = (objFirFilter**)calloc(srv->mic_num, sizeof(objFirFilter*));
	srv->st_doubletalk = (objDoubleTalk**)calloc(srv->mic_num, sizeof(objDoubleTalk*));
	srv->st_res = (objRES***)calloc(srv->mic_num, sizeof(objRES**));

	srv->st_tde = dios_ssp_aec_tde_init(srv->mic_num, srv->ref_num, srv->frm_len);

	for (i_mic = 0; i_mic < srv->mic_num; i_mic++)
	{
		srv->input_mic_time[i_mic] = (float*)calloc(srv->frm_len, sizeof(float));
		srv->input_mic_subband[i_mic] = (xcomplex*)calloc(AEC_SUBBAND_NUM, sizeof(xcomplex));
		srv->firfilter_out[i_mic] = (xcomplex*)calloc(AEC_SUBBAND_NUM, sizeof(xcomplex));
		srv->final_out[i_mic] = (xcomplex*)calloc(AEC_SUBBAND_NUM, sizeof(xcomplex));
		srv->est_echo[i_mic] = (xcomplex*)calloc(AEC_SUBBAND_NUM, sizeof(xcomplex));
		srv->st_res[i_mic] = (objRES**)calloc(srv->ref_num, sizeof(objRES*));
		
		/* sub module init */
		srv->st_subband_mic[i_mic] = dios_ssp_share_subband_init(srv->frm_len);
		srv->st_firfilter[i_mic] = dios_ssp_aec_firfilter_init(srv->ref_num);
		srv->st_doubletalk[i_mic] = dios_ssp_aec_doubletalk_init(srv->ref_num);		
		for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
		{
			srv->st_res[i_mic][i_ref] = dios_ssp_aec_res_init();
		}

		/* module share memory and varvariable */
		srv->st_firfilter[i_mic]->sig_mic_rec = srv->input_mic_subband[i_mic];
		srv->st_firfilter[i_mic]->sig_spk_ref = srv->input_ref_subband;
		srv->st_firfilter[i_mic]->noise_est_spk_part = srv->st_noise_est_spk_subband;
		srv->st_firfilter[i_mic]->noise_est_spk_t = srv->st_noise_est_spk_t;
		srv->st_firfilter[i_mic]->band_table = srv->band_table;
		srv->st_firfilter[i_mic]->spk_part_band_energy = srv->spk_part_band_energy;
		srv->st_firfilter[i_mic]->spk_peak = srv->spk_peak;
	}

	/* reference number related */
	srv->ref_buffer = (float*)calloc(srv->ref_num * (srv->ref_buffer_len + srv->frm_len), sizeof(float));
	srv->ref_tde = (float*)calloc(srv->ref_num * srv->frm_len, sizeof(float));
	srv->abs_ref_avg = (float*)calloc(srv->ref_num, sizeof(float));
	srv->ref_psd = (float**)calloc(srv->ref_num, sizeof(float*));
	srv->input_ref_subband = (xcomplex**)calloc(srv->ref_num, sizeof(xcomplex*));
	srv->spk_peak = (float**)calloc(srv->ref_num, sizeof(float*));
	srv->input_ref_time = (float**)calloc(srv->ref_num, sizeof(float*));
	srv->st_subband_ref = (objSubBand**)calloc(srv->ref_num, sizeof(objSubBand*));
	srv->st_noise_est_spk_t = (objNoiseLevel**)calloc(srv->ref_num, sizeof(objNoiseLevel*));
	for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
	{
		srv->input_ref_time[i_ref] = (float*)calloc(srv->frm_len, sizeof(float));
		srv->input_ref_subband[i_ref] = (xcomplex*)calloc(AEC_SUBBAND_NUM, sizeof(xcomplex));
		srv->ref_psd[i_ref] = (float*)calloc(AEC_SUBBAND_NUM, sizeof(float));
		srv->spk_peak[i_ref] = (float*)calloc(ERL_BAND_NUM, sizeof(float));
	}
	for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
	{
		/* ref signal sunbband init */
		srv->st_subband_ref[i_ref] = dios_ssp_share_subband_init(srv->frm_len);
		srv->st_noise_est_spk_t[i_ref] = (objNoiseLevel*)calloc(1, sizeof(objNoiseLevel));
	}

	srv->st_noise_est_spk_subband = (objNoiseLevel***)calloc(srv->ref_num, sizeof(objNoiseLevel**));
	for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
	{
		srv->st_noise_est_spk_subband[i_ref] = (objNoiseLevel**)calloc(ERL_BAND_NUM, sizeof(objNoiseLevel*));
		for (i = 0; i < ERL_BAND_NUM; i++)
		{
			srv->st_noise_est_spk_subband[i_ref][i] = (objNoiseLevel *)calloc(1, sizeof(objNoiseLevel));
		}
	}

	/* erl number related */
	srv->spk_part_band_energy = (float*)calloc(ERL_BAND_NUM, sizeof(float));
	srv->band_table = (int**)calloc(ERL_BAND_NUM, sizeof(int*));
	for (i = 0; i < ERL_BAND_NUM; i++)
	{
		srv->band_table[i] = (int *)calloc(2, sizeof(int));
	}
	
	/* variable init */
	srv->freq_div_table[0] = 0;
	srv->freq_div_table[1] = 600;
	srv->freq_div_table[2] = 1200;
	srv->freq_div_table[3] = 3000;
	srv->freq_div_table[4] = 8000;

	/* struct init */
	srv->band_table[0][0] = AEC_LOW_CHAN;
	for (i = 1; i < ERL_BAND_NUM; i++)
	{
		srv->band_table[i][0] = (int)(srv->freq_div_table[i] / AEC_SAMPLE_RATE * AEC_FFT_LEN);
		srv->band_table[i - 1][1] = srv->band_table[i][0] - 1;
	}
	srv->band_table[ERL_BAND_NUM - 1][1] = AEC_HIGH_CHAN - 1;

	ret = dios_ssp_aec_reset_api(srv);
	if (0 != ret)
	{
		return NULL;
	}

	ret = dios_ssp_aec_config_api(srv, 1);
	if (0 != ret)
	{
		return NULL;
	}

	return (ptr);
}

int dios_ssp_aec_config_api(void* ptr, int mode)
{
	int i_mic;
	int i_ref;
	objAEC* srv = (objAEC*)ptr;
	if (mode == 0) //com mode
	{		
		for (i_mic = 0; i_mic < srv->mic_num; i_mic++)
		{
			/* double talk detection para set */
			srv->st_doubletalk[i_mic]->dt_thr_factor = COM_DT_THR_FACTOR;
			srv->st_doubletalk[i_mic]->dt_min_thr = COM_DT_MIN_THR;
			for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
			{
				/* residual echo suppression para set */
				srv->st_res[i_mic][i_ref]->res1_echo_noise_factor = COM_RES1_ECHO_NOISE_FACTOR;
				srv->st_res[i_mic][i_ref]->res2_echo_noise_factor = COM_RES2_ECHO_NOISE_FACTOR;
				srv->st_res[i_mic][i_ref]->res1_echo_suppress_default = COM_RES1_ECHO_SUPPRESS_DEFAULT;
				srv->st_res[i_mic][i_ref]->res2_st_echo_suppress_default = COM_RES2_ST_ECHO_SUPPRESS_DEFAULT;
				srv->st_res[i_mic][i_ref]->res2_dt_echo_suppress_default = COM_RES2_DT_ECHO_SUPPRESS_DEFAULT;
				srv->st_res[i_mic][i_ref]->res1_echo_suppress_active_default = COM_RES1_ECHO_SUPPRESS_ACTIVE_DEFAULT;
				srv->st_res[i_mic][i_ref]->res2_st_echo_suppress_active_default = COM_RES2_ST_ECHO_SUPPRESS_ACTIVE_DEFAULT;
				srv->st_res[i_mic][i_ref]->res2_dt_echo_suppress_active_default = COM_RES2_DT_ECHO_SUPPRESS_ACTIVE_DEFAULT;
				srv->st_res[i_mic][i_ref]->res1_suppress_factor = COM_RES1_SUPPRESS_FACTOR;
				srv->st_res[i_mic][i_ref]->res2_st_suppress_factor = COM_RES2_ST_SUPPRESS_FACTOR;
				srv->st_res[i_mic][i_ref]->res2_dt_suppress_factor = COM_RES2_DT_SUPPRESS_FACTOR;
			}
		}
		
	}
	else //asr mode
	{
		for (i_mic = 0; i_mic < srv->mic_num; i_mic++)
		{
			/* double talk detection para set */
			srv->st_doubletalk[i_mic]->dt_thr_factor = ASR_DT_THR_FACTOR;
			srv->st_doubletalk[i_mic]->dt_min_thr = ASR_DT_MIN_THR;
			for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
			{
				/* residual echo suppression para set */
				srv->st_res[i_mic][i_ref]->res1_echo_noise_factor = ASR_RES1_ECHO_NOISE_FACTOR;
				srv->st_res[i_mic][i_ref]->res2_echo_noise_factor = ASR_RES2_ECHO_NOISE_FACTOR;
				srv->st_res[i_mic][i_ref]->res1_echo_suppress_default = ASR_RES1_ECHO_SUPPRESS_DEFAULT;
				srv->st_res[i_mic][i_ref]->res2_st_echo_suppress_default = ASR_RES2_ST_ECHO_SUPPRESS_DEFAULT;
				srv->st_res[i_mic][i_ref]->res2_dt_echo_suppress_default = ASR_RES2_DT_ECHO_SUPPRESS_DEFAULT;
				srv->st_res[i_mic][i_ref]->res1_echo_suppress_active_default = ASR_RES1_ECHO_SUPPRESS_ACTIVE_DEFAULT;
				srv->st_res[i_mic][i_ref]->res2_st_echo_suppress_active_default = ASR_RES2_ST_ECHO_SUPPRESS_ACTIVE_DEFAULT;
				srv->st_res[i_mic][i_ref]->res2_dt_echo_suppress_active_default = ASR_RES2_DT_ECHO_SUPPRESS_ACTIVE_DEFAULT;
				srv->st_res[i_mic][i_ref]->res1_suppress_factor = ASR_RES1_SUPPRESS_FACTOR;
				srv->st_res[i_mic][i_ref]->res2_st_suppress_factor = ASR_RES2_ST_SUPPRESS_FACTOR;
				srv->st_res[i_mic][i_ref]->res2_dt_suppress_factor = ASR_RES2_DT_SUPPRESS_FACTOR;
			}
		}
	}
	return 0;
}

int dios_ssp_aec_reset_api(void* ptr)
{
	int ret = 0;
	int i;
	int i_mic;
	int i_ref;
	objAEC* srv = (objAEC*)ptr;
	
	if (NULL == ptr)
	{
		return ERR_AEC;
	}

	srv->far_end_talk_holdtime = 1;

	memset(srv->ref_buffer, 0, srv->ref_num * (srv->ref_buffer_len + srv->frm_len) * sizeof(float));

	ret = dios_ssp_aec_tde_reset(srv->st_tde);
	if (0 != ret)
	{
		return ERR_AEC;
	}

	for (i_mic = 0; i_mic < srv->mic_num; i_mic++)
	{
		srv->doubletalk_result[i_mic] = SINGLE_TALK_STATUS;

		/* subband module reset */
		ret = dios_ssp_share_subband_reset(srv->st_subband_mic[i_mic]);
		if (0 != ret)
		{
			return ERR_AEC;
		}
		ret = dios_ssp_aec_firfilter_reset(srv->st_firfilter[i_mic]);
		if (0 != ret)
		{
			return ERR_AEC;
		}
		ret = dios_ssp_aec_doubletalk_reset(srv->st_doubletalk[i_mic]);
		if (0 != ret)
		{
			return ERR_AEC;
		}

		for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
		{
			ret = dios_ssp_aec_res_reset(srv->st_res[i_mic][i_ref]);
			if (0 != ret)
			{
				return ERR_AEC;
			}
		}
	}

	for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
	{
		srv->abs_ref_avg[i_ref] = 0;
		memset(srv->ref_psd[i_ref], 0, AEC_SUBBAND_NUM * sizeof(float));

		ret = dios_ssp_share_subband_reset(srv->st_subband_ref[i_ref]);
		if (0 != ret)
		{
			return ERR_AEC;
		}

		ret = dios_ssp_share_noiselevel_init(srv->st_noise_est_spk_t[i_ref], SPK_MAX_NOISE, SPK_MIN_NOISE, NL_RUN_MIN_LEN);
		if (0 != ret)
		{
			return ERR_AEC;
		}

		for (i = 0; i < ERL_BAND_NUM; i++)
		{
			ret = dios_ssp_share_noiselevel_init(srv->st_noise_est_spk_subband[i_ref][i], SPK_PART_MAX_NOISE, SPK_PART_MIN_NOISE, NL_RUN_MIN_LEN);
			if (0 != ret)
			{
				return ERR_AEC;
			}
		}
	}
	
	return 0;
}

// main function processed by AEC
int dios_ssp_aec_process_api(void* ptr, float* io_buf, float* ref_buf, int* dt_st)
{
	objAEC* srv = (objAEC*)ptr;
	int ret_process = 0;
	int i_mic;
	int i_ref;
	int i, ii;
	int ch;
	int far_end_talk_flag = 0;

    if (NULL == srv)
    {
		return ERR_AEC;
    }
	memcpy(srv->mic_tde, io_buf, srv->mic_num * srv->frm_len * sizeof(float));
	memcpy(srv->ref_tde, ref_buf, srv->ref_num * srv->frm_len * sizeof(float));

	/* fixed delay process */
	memcpy(srv->ref_buffer + srv->ref_num * srv->ref_buffer_len, srv->ref_tde, srv->ref_num * srv->frm_len * sizeof(float));
	memcpy(srv->ref_tde, srv->ref_buffer, srv->ref_num * srv->frm_len * sizeof(float));
	memmove(srv->ref_buffer, srv->ref_buffer + srv->ref_num * srv->frm_len, srv->ref_num * srv->ref_buffer_len * sizeof(float));
	memset(srv->ref_buffer + srv->ref_num * srv->ref_buffer_len, 0, srv->ref_num * srv->frm_len * sizeof(float));
	
	/* delay the processing function and align the data with the calculated delay */
	ret_process = dios_ssp_aec_tde_process(srv->st_tde, srv->ref_tde, srv->mic_tde);
	if (0 != ret_process)
	{
		return ERR_AEC;
	}

	/* get mic data and mic peak */
	for (i_mic = 0; i_mic < srv->mic_num; i_mic++)
    {
		/* get mic data */
		for (i = 0; i < srv->frm_len; i++)
		{
			srv->input_mic_time[i_mic][i] = (float)srv->mic_tde[i_mic * srv->frm_len + i];
		}
    }
	/* get ref data and ref average value */
	for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
	{
		/* get ref data */
		for (i = 0; i < srv->frm_len; i++)
		{
			srv->input_ref_time[i_ref][i] = (float)srv->ref_tde[i_ref * srv->frm_len + i];
		}
		/* get ref average value */
		ret_process = dios_ssp_aec_average_track(srv->input_ref_time[i_ref], srv->frm_len, &(srv->abs_ref_avg[i_ref]));
		if (0 != ret_process)
		{
			return ERR_AEC;
		}
	}
	/* check farend talk status */
	for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
	{
		/* speaker noise track */
		dios_ssp_share_noiselevel_process(srv->st_noise_est_spk_t[i_ref], srv->abs_ref_avg[i_ref]);

		/* check whether is farend talk status */
		if ((srv->abs_ref_avg[i_ref] > 2.0f * srv->st_noise_est_spk_t[i_ref]->noise_level_first)
			&& (srv->abs_ref_avg[i_ref] > SPK_MAX_NOISE))
		{
			far_end_talk_flag = 1;
			break;
		}
	}
	if (far_end_talk_flag == 1)
	{
		srv->far_end_talk_holdtime = FAREND_TALK_CNT;
	}
	else if (srv->far_end_talk_holdtime > 0)
	{
		srv->far_end_talk_holdtime--;
	}

    /* reference subband analyse */
    for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
    {
		ret_process = dios_ssp_share_subband_analyse(srv->st_subband_ref[i_ref], srv->input_ref_time[i_ref], srv->input_ref_subband[i_ref]);
		if (0 != ret_process) 
		{
			return ERR_AEC;
		}		

		/* get reference signal psd */
		for(ch = 0; ch < AEC_SUBBAND_NUM; ch++)
		{
			srv->ref_psd[i_ref][ch] = complex_abs2(srv->input_ref_subband[i_ref][ch]);
		}

		/* get erl band energy */
		for(i = 0; i < ERL_BAND_NUM; i++)
		{
			srv->spk_part_band_energy[i] = 0.0f;
			for (ch = srv->band_table[i][0]; ch <= srv->band_table[i][1]; ch++)
			{
				srv->spk_part_band_energy[i] += srv->ref_psd[i_ref][ch];
			}
			/* speaker noise tracking */
			dios_ssp_share_noiselevel_process(srv->st_noise_est_spk_subband[i_ref][i], srv->spk_part_band_energy[i]);

			/* speaker peak value tracking */
			if (srv->spk_part_band_energy[i] > srv->spk_peak[i_ref][i])
			{
				srv->spk_peak[i_ref][i] = srv->spk_part_band_energy[i];
			}
			else
			{
				srv->spk_peak[i_ref][i] = AEC_PEAK_ALPHA * srv->spk_peak[i_ref][i] + (1 - AEC_PEAK_ALPHA) * srv->spk_part_band_energy[i];
			}
		}
    }
		
    for (i_mic = 0; i_mic < srv->mic_num; i_mic++)
    {
		/* mic subband analyse */
		ret_process = dios_ssp_share_subband_analyse(srv->st_subband_mic[i_mic], srv->input_mic_time[i_mic], srv->input_mic_subband[i_mic]);
		if (0 != ret_process) 
		{
			return ERR_AEC;
		}
		/* fir filter process */
		srv->st_firfilter[i_mic]->far_end_talk_holdtime = srv->far_end_talk_holdtime;
		srv->st_firfilter[i_mic]->dt_status = &srv->doubletalk_result[i_mic];

		srv->st_firfilter[i_mic]->sig_mic_rec = srv->input_mic_subband[i_mic];
		srv->st_firfilter[i_mic]->sig_spk_ref = srv->input_ref_subband;

		srv->st_firfilter[i_mic]->noise_est_spk_part = srv->st_noise_est_spk_subband;
		srv->st_firfilter[i_mic]->noise_est_spk_t = srv->st_noise_est_spk_t;

		srv->st_firfilter[i_mic]->band_table = srv->band_table;
		srv->st_firfilter[i_mic]->spk_part_band_energy = srv->spk_part_band_energy;
		srv->st_firfilter[i_mic]->spk_peak = srv->spk_peak;
		ret_process = dios_ssp_aec_firfilter_process(srv->st_firfilter[i_mic], srv->firfilter_out[i_mic], srv->est_echo[i_mic]);
		if (0 != ret_process) 
		{
			return ERR_AEC;
		}

		/* save for 2nd stage res */
		/* The 1st stage residual echo processing is to judge the double-talk state */
		memcpy(srv->final_out[i_mic], srv->firfilter_out[i_mic], sizeof(xcomplex) * AEC_SUBBAND_NUM);
		
		/* ERL estimate */
		ret_process = dios_ssp_aec_erl_est_process(srv->st_firfilter[i_mic]);
		if (ret_process != 0)
		{
			return ERR_AEC;
		}
		
		/* aec output noise tracking */
		for (ch = AEC_LOW_CHAN; ch < AEC_HIGH_CHAN; ch++)
		{
			if (srv->far_end_talk_holdtime == 0)
			{
				dios_ssp_share_noiselevel_process(srv->st_firfilter[i_mic]->noise_est_mic_chan[ch], srv->st_firfilter[i_mic]->power_mic_send_smooth[ch]);
			}
		}

		/* 1st stage residual echo suppression to improve dtd result */
		for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
		{
			srv->st_res[i_mic][i_ref]->Xf_res_echo = srv->firfilter_out[i_mic];
			srv->st_res[i_mic][i_ref]->Xf_echo = srv->est_echo[i_mic];
			/* dtd result input(the 2nd parameter) is only for stage two, so this dtd result is useless */
			ret_process = dios_ssp_aec_res_process(srv->st_res[i_mic][i_ref], srv->doubletalk_result[i_mic], 1);
			if (0 != ret_process)
			{
				return ERR_AEC;
			}
		}		

		/* double talk process */
		for (ii = 0; ii< AEC_SUBBAND_NUM; ii++)
		{
			srv->st_doubletalk[i_mic]->res1_psd[ii] = complex_abs2(srv->firfilter_out[i_mic][ii]);
		}
		srv->st_doubletalk[i_mic]->mic_noise_bin = srv->st_firfilter[i_mic]->noise_est_mic_chan;
		srv->st_doubletalk[i_mic]->erl_ratio = srv->st_firfilter[i_mic]->erl_ratio;
		srv->st_doubletalk[i_mic]->far_end_talk_holdtime = srv->far_end_talk_holdtime;
		ret_process = dios_ssp_aec_doubletalk_process(srv->st_doubletalk[i_mic], &srv->doubletalk_result[i_mic]);
		if (0 != ret_process)
		{
			return ERR_AEC;
		}
		
		/* 2nd stage residual echo suppression */
		for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
		{
			srv->st_res[i_mic][i_ref]->Xf_res_echo = srv->final_out[i_mic];
			srv->st_res[i_mic][i_ref]->Xf_echo = srv->est_echo[i_mic];
			ret_process = dios_ssp_aec_res_process(srv->st_res[i_mic][i_ref], srv->doubletalk_result[i_mic], 2);
			if (0 != ret_process)
			{
				return ERR_AEC;
			}
		}

		/* subband compose */
		ret_process = dios_ssp_share_subband_compose(srv->st_subband_mic[i_mic], srv->final_out[i_mic], &io_buf[i_mic * srv->frm_len]);
		
	}
    dt_st[0] = srv->doubletalk_result[0];
    return 0;
}

int dios_ssp_aec_uninit_api(void* ptr)
{
	int i;
	int i_mic;
	int i_ref;
	int ret = 0;
	objAEC* srv = (objAEC*)ptr;

	if (NULL == ptr)
	{
		return ERR_AEC;
	}

	/* buffer memory free */
	for (i_mic = 0; i_mic < srv->mic_num; i_mic++)
	{
		free(srv->input_mic_time[i_mic]);
		free(srv->input_mic_subband[i_mic]);
		free(srv->firfilter_out[i_mic]);
		free(srv->final_out[i_mic]);
		free(srv->est_echo[i_mic]);		
	}
	free(srv->mic_tde);
	free(srv->doubletalk_result);
	free(srv->input_mic_time);
	free(srv->input_mic_subband);
	free(srv->firfilter_out);
	free(srv->final_out);
	free(srv->est_echo);
	
	for (i = 0; i < ERL_BAND_NUM; i++)
	{
		free(srv->band_table[i]);
	}
	free(srv->band_table);
	for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
	{
		free(srv->input_ref_time[i_ref]);
		free(srv->input_ref_subband[i_ref]);
		free(srv->st_noise_est_spk_t[i_ref]);
		free(srv->ref_psd[i_ref]);
		free(srv->spk_peak[i_ref]);
		for (i = 0; i < ERL_BAND_NUM; i++)
		{
			free(srv->st_noise_est_spk_subband[i_ref][i]);
		}
		free(srv->st_noise_est_spk_subband[i_ref]);
	}
	free(srv->st_noise_est_spk_t);
	free(srv->st_noise_est_spk_subband);
	free(srv->spk_peak);
	free(srv->ref_psd);
	free(srv->abs_ref_avg);
	free(srv->ref_buffer);
	free(srv->ref_tde);
	free(srv->input_ref_time);
	free(srv->input_ref_subband);
	free(srv->spk_part_band_energy);

	ret = dios_ssp_aec_tde_uninit(srv->st_tde);
	if (0 != ret)
	{
		return ERR_AEC;
	}

	/* reference number related uninit */
	for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
	{
		ret = dios_ssp_share_subband_uninit(srv->st_subband_ref[i_ref]);
		if (0 != ret)
		{
			return ERR_AEC;
		}
	}
	free(srv->st_subband_ref);

	/* microphone number related uninit */
	for (i_mic = 0; i_mic < srv->mic_num; i_mic++)
	{
		/* subband uninit */
		ret = dios_ssp_share_subband_uninit(srv->st_subband_mic[i_mic]);
		if (0 != ret)
		{
			return ERR_AEC;
		}
		/* fir filter uninit */
		ret = dios_ssp_aec_firfilter_uninit(srv->st_firfilter[i_mic]);
		if (0 != ret)
		{
			return ERR_AEC;
		}
		/* res uninit */
		for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
		{
			ret = dios_ssp_aec_res_unit(srv->st_res[i_mic][i_ref]);
			if (0 != ret)
			{
				return ERR_AEC;
			}
		}
		free(srv->st_res[i_mic]);

		/* double talk uninit */
		ret = dios_ssp_aec_doubletalk_uninit(srv->st_doubletalk[i_mic]);
		if (0 != ret)
		{
			return ERR_AEC;
		}		
	}
	free(srv->st_subband_mic);
	free(srv->st_firfilter);
	free(srv->st_res);
	free(srv->st_doubletalk);
	free(srv);

	return 0;
}
