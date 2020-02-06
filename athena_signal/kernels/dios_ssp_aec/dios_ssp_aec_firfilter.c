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

Description: the core part of linear echo cancellation. The filter coefficients 
consist of two groups for stability of filter: adf_coef and fir_coef, the best 
one will be chosen according to the residual error signal level, the adf_coef 
is updated with IPNLMS. 
==============================================================================*/

/* include file */
#include "dios_ssp_aec_firfilter.h"

/* estimate echo and calculate residual */
void dios_ssp_aec_residual(objFirFilter *srv)
{
	int ch;
	int i_ref;
	int i;
	for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
    {
		for (ch = AEC_LOW_CHAN; ch < AEC_HIGH_CHAN; ch++)
		{
			/* get reference vector for fir filter */
			complex_data_push(srv->num_main_subband_adf[ch] + 1, srv->sig_spk_ref[i_ref][ch], srv->stack_sigIn_adf[i_ref][ch]);
			
			/* get echo signal: conv: y = conj(h) * x */
			srv->est_ref_fir[i_ref][ch] = complex_conv(srv->num_main_subband_adf[ch], srv->fir_coef[i_ref][ch],
					srv->stack_sigIn_adf[i_ref][ch]);
			srv->est_ref_adf[i_ref][ch] = complex_conv(srv->num_main_subband_adf[ch], srv->adf_coef[i_ref][ch], 
					srv->stack_sigIn_adf[i_ref][ch]);
			
			/* get power of reference vector */
			srv->power_in_ntaps_smooth[i_ref][ch] = 0.0f;
			for (i = 0; i < srv->num_main_subband_adf[ch]; i++)
			{
			    srv->power_in_ntaps_smooth[i_ref][ch] += complex_abs2(srv->stack_sigIn_adf[i_ref][ch][i]);
			}
		}
    }
	for (ch = AEC_LOW_CHAN; ch < AEC_HIGH_CHAN; ch++)
	{
		/* get total error signal for each reference signal */
        srv->err_fir[ch] = complex_sub(srv->sig_mic_rec[ch], srv->est_ref_fir[0][ch]);
        srv->err_adf[ch] = complex_sub(srv->sig_mic_rec[ch], srv->est_ref_adf[0][ch]);
        for (i_ref = 1; i_ref < srv->ref_num; i_ref++)
        {
            srv->err_fir[ch] = complex_sub(srv->err_fir[ch], srv->est_ref_fir[i_ref][ch]);
            srv->err_adf[ch] = complex_sub(srv->err_adf[ch], srv->est_ref_adf[i_ref][ch]);
        }
		/* estimate the smooth value for err_fir, err_adf and sig_mic_rec signal */
		srv->mse_adpt[ch] = (srv->lambda[ch] * srv->mse_adpt[ch]) + (1 - srv->lambda[ch]) * complex_abs2(srv->err_adf[ch]);
		srv->mse_main[ch] = (srv->lambda[ch] * srv->mse_main[ch]) + (1 - srv->lambda[ch]) * complex_abs2(srv->err_fir[ch]);
		srv->mse_mic_in[ch] = (srv->lambda[ch] * srv->mse_mic_in[ch]) + (1 - srv->lambda[ch]) * complex_abs2(srv->sig_mic_rec[ch]);
	}
}

/* filter convergence detection */
void dios_ssp_aec_firfilter_detect(objFirFilter *srv)
{
	int ch, i_ref, i;
	for (ch = AEC_LOW_CHAN; ch < AEC_HIGH_CHAN; ch++)
    {
		/* filter convergence detection */
		if (srv->mse_adpt[ch] > srv->mse_mic_in[ch] * MSE_RATIO_OUT_IN)
		{
			for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
			{			 
				for (i = 0; i < srv->num_main_subband_adf[ch]; i++)
				{
					srv->adf_coef[i_ref][ch][i].r = 0.0;
					srv->adf_coef[i_ref][ch][i].i = 0.0;
				}
			}
			srv->mse_mic_in[ch] = 0.0;
			srv->mse_adpt[ch] = 0.0;
			srv->mse_main[ch] = 0.0;
		}
		else if ((srv->mse_mic_in[ch] > srv->mse_adpt[ch] * MSE_RATIO_OUT_IN)
				&& (srv->mse_adpt[ch] < FILTER_COPY_FAC * srv->mse_main[ch]))
		{
			for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
			{				
				for (i = 0; i < srv->num_main_subband_adf[ch]; i++)
				{
					srv->fir_coef[i_ref][ch][i] = srv->adf_coef[i_ref][ch][i];
				}
			}
			srv->mse_mic_in[ch] = 0.0;
			srv->mse_adpt[ch] = 0.0;
			srv->mse_main[ch] = 0.0;
		}

		if (srv->mse_main[ch] > srv->mse_mic_in[ch] * MSE_RATIO_OUT_IN)
		{
			for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
			{				
				for (i = 0; i < srv->num_main_subband_adf[ch]; i++)
				{
					srv->fir_coef[i_ref][ch][i].r = 0.0;
					srv->fir_coef[i_ref][ch][i].i = 0.0;
				}
			}
			srv->mse_main[ch] = 0.0;
			srv->mse_adpt[ch] = 0.0;
			srv->mse_mic_in[ch] = 0.0;
		}
		else if ((srv->mse_mic_in[ch] > srv->mse_main[ch] * MSE_RATIO_OUT_IN)
				&& (srv->mse_main[ch] < FILTER_COPY_FAC * srv->mse_adpt[ch]))
		{
			for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
			{
				for (i = 0; i<srv->num_main_subband_adf[ch]; i++)
				{
					srv->adf_coef[i_ref][ch][i] = srv->fir_coef[i_ref][ch][i];
				}
			}
			srv->mse_mic_in[ch] = 0.0;
			srv->mse_adpt[ch] = 0.0;
			srv->mse_main[ch] = 0.0;
			srv->err_adf[ch] = srv->err_fir[ch];
		}
	}
}

/* get linear echo cancellation output */
void dios_ssp_estecho_output(objFirFilter *srv, xcomplex* output_buf, xcomplex* est_echo)
{
	int ch;
	int i_ref;

	for(ch = AEC_LOW_CHAN; ch < AEC_HIGH_CHAN; ch++)
	{
		srv->energy_err_fir[ch] = complex_abs2(srv->err_fir[ch]);
		srv->energy_err_adf[ch] = complex_abs2(srv->err_adf[ch]);
		
		/* get linear echo cancellation output */
		if (srv->energy_err_fir[ch] >= srv->energy_err_adf[ch])
		{   
			//adf_coef
			output_buf[ch] = srv->err_adf[ch];
			est_echo[ch] = srv->est_ref_adf[0][ch];
			for (i_ref = 1; i_ref < srv->ref_num; i_ref++)
			{
				est_echo[ch] = complex_add(est_echo[ch], srv->est_ref_adf[i_ref][ch]);
			}
		}
		else
		{ 
			//fir_coef
			output_buf[ch] = srv->err_fir[ch];
			est_echo[ch] = srv->est_ref_fir[0][ch];
			for (i_ref = 1; i_ref < srv->ref_num; i_ref++)
			{
				est_echo[ch] = complex_add(est_echo[ch], srv->est_ref_fir[i_ref][ch]);
			}
		}
	}
}

int aec_channel_to_band(int **band_table, int ch)
{
    int index;
    if (ch >= band_table[0][0] && ch <= band_table[0][1])
    {
        index = 0;
    }
    else if (ch >= band_table[1][0] && ch <= band_table[1][1])
    {
	    index = 1;
    }
    else if (ch >= band_table[2][0] && ch <= band_table[2][1])
    {
	    index = 2;
    }
    else
    {
	    index = 3;
    }

    return index;
}

/* filter coefficient update */
void ipnlms_complex(int ch, objFirFilter *srv, int i_ref)
{
    int ii_spk;
	int m, M = srv->num_main_subband_adf[ch];
	float aec_ns_alpha = 0;
	float myu = srv->weight[ch * 2];
	xcomplex delta, z;
	float Padf = 0.0;
	float kl[NUM_MAX_BAND];
	float ip_alpha = 0.5;
	float x2_kl = 0.0;
	float norm_aec = 0.0;
	for (m = 0; m < M; m++)
	{
		kl[m] = complex_abs2(srv->adf_coef[i_ref][ch][m]);
		Padf += kl[m];
	}
		
	for (ii_spk = 0; ii_spk < srv->ref_num; ii_spk++)
	{
		x2_kl = 0.0;
		for (m = 0; m < M; m++)
		{
			kl[m] = (1 - ip_alpha) / (2 * M) + (1 + ip_alpha)*kl[m] / (Padf * 2 + 1e-5f);
			x2_kl += complex_abs2(srv->stack_sigIn_adf[ii_spk][ch][m])*kl[m];
		}
		norm_aec += x2_kl;
	}

	aec_ns_alpha = myu / (norm_aec + 0.01f);
	delta = complex_real_complex_mul(aec_ns_alpha, complex_conjg(srv->err_adf[ch]));
	for (m = 0; m < M; m++)
	{
		z = complex_mul(srv->stack_sigIn_adf[i_ref][ch][m], delta);
		z = complex_real_complex_mul(kl[m], z);
		srv->adf_coef[i_ref][ch][m] = complex_add(srv->adf_coef[i_ref][ch][m], z);
	}
}

//aec fir filter init
objFirFilter* dios_ssp_aec_firfilter_init(int ref_num)
{
	int i;
	int i_ref;
	int ret = 0;
    objFirFilter* srv = NULL;
    srv = (objFirFilter*)calloc(1, sizeof(objFirFilter));
	
	srv->ref_num = ref_num;
    srv->myu = 0.5f;
    srv->beta = 1e-008f;
    srv->fir_coef = (xcomplex ***)calloc(srv->ref_num, sizeof(xcomplex**));
    srv->adf_coef = (xcomplex ***)calloc(srv->ref_num, sizeof(xcomplex**));
    srv->stack_sigIn_adf = (xcomplex ***)calloc(srv->ref_num, sizeof(xcomplex**));
    srv->err_adf = (xcomplex *)calloc(AEC_SUBBAND_NUM, sizeof(xcomplex));
    srv->err_fir = (xcomplex *)calloc(AEC_SUBBAND_NUM, sizeof(xcomplex));
    srv->est_ref_adf = (xcomplex **)calloc(srv->ref_num, sizeof(xcomplex*));
    srv->est_ref_fir = (xcomplex **)calloc(srv->ref_num, sizeof(xcomplex*));

    srv->mse_main = (float *)calloc(AEC_SUBBAND_NUM, sizeof(float));
    srv->mse_adpt = (float *)calloc(AEC_SUBBAND_NUM, sizeof(float));
    srv->mse_mic_in = (float *)calloc(AEC_SUBBAND_NUM, sizeof(float));
    srv->power_in_ntaps_smooth = (float **)calloc(srv->ref_num, sizeof(float*));
    srv->mic_rec_psd = (float *)calloc(AEC_SUBBAND_NUM, sizeof(float));
	srv->energy_err_fir = (float *)calloc(AEC_SUBBAND_NUM, sizeof(float));
	srv->energy_err_adf = (float *)calloc(AEC_SUBBAND_NUM, sizeof(float));
	srv->ref_psd = (float **)calloc(srv->ref_num, sizeof(float*));
    srv->power_mic_send_smooth = (float *)calloc(AEC_SUBBAND_NUM, sizeof(float));
    srv->power_echo_rtn_smooth = (float **)calloc(srv->ref_num, sizeof(float*));
    srv->mic_rec_part_band_energy = (float *)calloc(ERL_BAND_NUM, sizeof(float));

    srv->echo_return_band_energy = (float *)calloc(ERL_BAND_NUM, sizeof(float));
    srv->mic_send_part_band_energy = (float *)calloc(ERL_BAND_NUM, sizeof(float));
    srv->mic_peak = (float **)calloc(srv->ref_num, sizeof(float*));
    srv->erl_ratio = (float **)calloc(srv->ref_num, sizeof(float*));
    srv->power_echo_rtn_fir = (float **)calloc(srv->ref_num, sizeof(float*));
    srv->power_echo_rtn_adpt = (float **)calloc(srv->ref_num, sizeof(float*));
    for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
    {
		srv->ref_psd[i_ref] = (float *)calloc(AEC_SUBBAND_NUM, sizeof(float));
		srv->power_in_ntaps_smooth[i_ref] = (float *)calloc(AEC_SUBBAND_NUM, sizeof(float));
		srv->adf_coef[i_ref] = (xcomplex **)calloc(AEC_SUBBAND_NUM, sizeof(xcomplex*));
		srv->fir_coef[i_ref] = (xcomplex **)calloc(AEC_SUBBAND_NUM, sizeof(xcomplex*));
		srv->stack_sigIn_adf[i_ref] = (xcomplex **)calloc(AEC_SUBBAND_NUM, sizeof(xcomplex*));
		srv->power_echo_rtn_fir[i_ref] = (float *)calloc(AEC_SUBBAND_NUM, sizeof(float));
		srv->power_echo_rtn_adpt[i_ref] = (float *)calloc(AEC_SUBBAND_NUM, sizeof(float));
		for (i = 0; i < AEC_SUBBAND_NUM; i++)
		{
			srv->adf_coef[i_ref][i] = (xcomplex *)calloc((NUM_MAX_BAND + 1), sizeof(xcomplex));
			srv->fir_coef[i_ref][i] = (xcomplex *)calloc((NUM_MAX_BAND + 1), sizeof(xcomplex));
			srv->stack_sigIn_adf[i_ref][i] = (xcomplex *)calloc((NUM_MAX_BAND + 1), sizeof(xcomplex));
		}

		srv->est_ref_adf[i_ref] = (xcomplex *)calloc(AEC_SUBBAND_NUM, sizeof(xcomplex));
		srv->est_ref_fir[i_ref] = (xcomplex *)calloc(AEC_SUBBAND_NUM, sizeof(xcomplex));
		srv->power_echo_rtn_smooth[i_ref] = (float *)calloc(AEC_SUBBAND_NUM, sizeof(float));
    }

    srv->num_main_subband_adf = (int *)calloc(AEC_SUBBAND_NUM, sizeof(int));
    srv->lambda = (float *)calloc(AEC_SUBBAND_NUM, sizeof(float));
    srv->weight = (float *)calloc(AEC_SUBBAND_NUM * 2, sizeof(float));
    
	for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
    {
		srv->mic_peak[i_ref] = (float *)calloc(ERL_BAND_NUM, sizeof(float));
		srv->erl_ratio[i_ref] = (float *)calloc(ERL_BAND_NUM, sizeof(float));
    }
    srv->noise_est_mic_chan = (objNoiseLevel **)calloc(AEC_SUBBAND_NUM, sizeof(objNoiseLevel*));
    for (i = 0; i < AEC_SUBBAND_NUM; i++)
    {
		srv->noise_est_mic_chan[i] = (objNoiseLevel *)calloc(1, sizeof(objNoiseLevel));
    }

	ret = dios_ssp_aec_firfilter_reset(srv);
	if(0 != ret)
	{
		return NULL;
	}
	
    return srv;
}

int dios_ssp_aec_firfilter_reset(objFirFilter* srv)
{
	int i;
	int n;
	int i_ref;
	int ret = 0;

    if (NULL == srv) 
    {
		return ERR_AEC;
    }

	for (i = 0; i < AEC_SUBBAND_NUM; i++)
	{
		srv->weight[2 * i] = srv->myu;
		srv->weight[2 * i + 1] = srv->beta;
		if (i < AEC_MID_CHAN + 1)
		{
			srv->lambda[i] = ALPHA_MSE_FILT_COPY_LOW;
			srv->num_main_subband_adf[i] = NTAPS_LOW_BAND;
		}
		else
		{
			srv->lambda[i] = ALPHA_MSE_FILT_COPY_HIGH;
			srv->num_main_subband_adf[i] = NTAPS_HIGH_BAND;
		}
	}

	for (i = 0; i < AEC_SUBBAND_NUM; i++)
	{
		srv->err_fir[i].r = 0.0;
		srv->err_fir[i].i = 0.0;
		srv->err_adf[i].r = 0.0;
		srv->err_adf[i].i = 0.0;

		/* smoothed vector */
		srv->power_mic_send_smooth[i] = 0.0f;
		srv->mic_rec_psd[i] = 0.0f;

		srv->energy_err_fir[i] = 0.0f;
		srv->energy_err_adf[i] = 0.0f;

		for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
		{
			srv->ref_psd[i_ref][i] = 0.0f;
			srv->power_echo_rtn_smooth[i_ref][i] = 0.0f;
			srv->power_in_ntaps_smooth[i_ref][i] = 0.0f;
		}

		for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
		{
			for (n = 0; n < NUM_MAX_BAND + 1; n++)
			{
				srv->adf_coef[i_ref][i][n].r = 0.0f;
				srv->adf_coef[i_ref][i][n].i = 0.0f;
				srv->fir_coef[i_ref][i][n].r = 0.0f;
				srv->fir_coef[i_ref][i][n].i = 0.0f;
				srv->stack_sigIn_adf[i_ref][i][n].r = 0.0f;
				srv->stack_sigIn_adf[i_ref][i][n].i = 0.0f;
			}
		}
		srv->mse_adpt[i] = 0.0f;
		srv->mse_main[i] = 0.0f;
		srv->mse_mic_in[i] = 0.0f;
	}

	for (i = 0; i < ERL_BAND_NUM; i++)
	{
		srv->mic_rec_part_band_energy[i] = 0.0f;
		srv->mic_send_part_band_energy[i] = 0.0f;
		srv->echo_return_band_energy[i] = 0.0f;
		for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
		{
			srv->mic_peak[i_ref][i] = 0.0f;
			srv->erl_ratio[i_ref][i] = 4.0f;
		}
	}

	// noise level estimate
	for (i = 0; i < AEC_SUBBAND_NUM; i++)
	{
		ret = dios_ssp_share_noiselevel_init(srv->noise_est_mic_chan[i], MIC_CHAN_MAX_NOISE, MIC_CHAN_MIN_NOISE, NL_RUN_MIN_LEN); 
		if (ret != 0)
		{
			return ERR_AEC;
		}
	}
	srv->far_end_talk_holdtime = 1;
	srv->adjust_flag = 0;
	return 0;
}

/* linear filtert main function */
int dios_ssp_aec_firfilter_process(objFirFilter* srv, xcomplex* output_buf, xcomplex* est_echo)
{
    int ch;
	int i_ref;
    float update_thr;
	float update_thr_final;

	if (NULL == srv)
	{
		return ERR_AEC;
	}

	/* get filter update threshold */
    if (srv->dt_status[0] == DOUBLE_TALK_STATUS)
    { 
		update_thr = FILTER_UPDATE_FAC_DT;
    }
    else
    {
	    update_thr = FILTER_UPDATE_FAC_NON_DT;
    }
	update_thr_final = update_thr;

	/* estimate echo and calculate residual */
	dios_ssp_aec_residual(srv);
	

	/* filter convergence detect */
	dios_ssp_aec_firfilter_detect(srv);


	/* filter update */
	for (ch = AEC_LOW_CHAN; ch < AEC_HIGH_CHAN; ch++)
    {
		if (AEC_SAMPLE_RATE == 16000)
		{
			if (ch >= FILTER_UPDATE_FAC_BIN_THR1)
			{
				update_thr_final = update_thr * FILTER_UPDATE_FAC_PARA1;
			}
			else if ((ch >= FILTER_UPDATE_FAC_BIN_THR2)&& (ch < FILTER_UPDATE_FAC_BIN_THR1))
			{
				update_thr_final = update_thr * FILTER_UPDATE_FAC_PARA2;
			}
		}
		
		for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
		{
			// achieve filter update control variable
			int iBand = aec_channel_to_band(srv->band_table, ch);

			if (srv->power_in_ntaps_smooth[i_ref][ch] * srv->erl_ratio[i_ref][iBand] > update_thr_final * srv->noise_est_mic_chan[ch]->noise_level_first)
			{
				srv->adjust_flag = 1;
			}
			else
			{
				srv->adjust_flag = 0;
			}

			if (srv->adjust_flag == 1) 
			{
				ipnlms_complex(ch, srv, i_ref);
			}

		}

		
    }

	/* get linear echo cancellation output */
    for (ch = 0; ch < AEC_LOW_CHAN; ch++)
    {
		output_buf[ch].r = 0.0f;
		output_buf[ch].i = 0.0f;
    }
    for (ch = AEC_HIGH_CHAN; ch < AEC_SUBBAND_NUM; ch++)
    {
		output_buf[ch].r = 0.0f;
		output_buf[ch].i = 0.0f;
    }
	/* get linear echo cancellation output */
	dios_ssp_estecho_output(srv, output_buf, est_echo);
    return 0;
}

int dios_ssp_aec_firfilter_uninit(objFirFilter* srv)
{
	int i;
	int i_ref;
    if (NULL == srv) 
    {
		return ERR_AEC;
    }
    for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
    {
		for (i = 0; i < AEC_SUBBAND_NUM; i++)
		{
			free(srv->adf_coef[i_ref][i]);
			free(srv->fir_coef[i_ref][i]);
			free(srv->stack_sigIn_adf[i_ref][i]);
		}
		
		free(srv->adf_coef[i_ref]);
		free(srv->fir_coef[i_ref]);
		free(srv->stack_sigIn_adf[i_ref]);
	
		free(srv->power_in_ntaps_smooth[i_ref]);
		free(srv->est_ref_adf[i_ref]);
		free(srv->est_ref_fir[i_ref]);
		free(srv->ref_psd[i_ref]);
		free(srv->power_echo_rtn_smooth[i_ref]);
		free(srv->power_echo_rtn_fir[i_ref]);
		free(srv->power_echo_rtn_adpt[i_ref]);
    }
    free(srv->power_echo_rtn_fir);
    free(srv->power_echo_rtn_adpt);
    free(srv->fir_coef);
    free(srv->adf_coef);
    free(srv->stack_sigIn_adf);
    free(srv->err_adf);
    free(srv->err_fir);
    free(srv->est_ref_adf);
    free(srv->est_ref_fir);

    free(srv->mse_main);
    free(srv->mse_adpt);
    free(srv->mse_mic_in);
    free(srv->power_in_ntaps_smooth);
    free(srv->mic_rec_psd);
	free(srv->energy_err_fir);
	free(srv->energy_err_adf);
    free(srv->power_mic_send_smooth);
	free(srv->ref_psd);
    free(srv->power_echo_rtn_smooth);

    free(srv->echo_return_band_energy);
    free(srv->mic_rec_part_band_energy);
    free(srv->mic_send_part_band_energy);

    for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
    {
		free(srv->mic_peak[i_ref]);
		free(srv->erl_ratio[i_ref]);
    }
    free(srv->mic_peak);
    free(srv->erl_ratio);
    free(srv->num_main_subband_adf);
    free(srv->lambda);
    free(srv->weight);
	
    for (i = 0; i < AEC_SUBBAND_NUM; i++)
    {
		free(srv->noise_est_mic_chan[i]);
    }
    free(srv->noise_est_mic_chan);
    free(srv);
    return 0;
}

