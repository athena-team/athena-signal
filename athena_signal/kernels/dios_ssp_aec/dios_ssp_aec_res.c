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

Description: some codes of the residual echo supression processing refers to 
Speex (https://www.speex.org/) which is an open source. It is devided into two 
stages, the output of the 1st stage is the input of the double-talk detection,
and the output of the 2nd stage is the really nonlinear output. 
Function framework:
1) the output of the linear result is used as input to estimate the residual echo
2) calculate prior SNR and posteriori SNR
3) calculate residual echo processing gain cofficient based on a posteriori SNR
   and prior SNR
4) output the final result

==============================================================================*/

/* include file */
#include "dios_ssp_aec_res.h"

int dios_ssp_aec_res_get_residual_echo(objRES* srv, float *residual_echo, int dt_st, int stage)
{
	int i;
	float resPsd[AEC_SUBBAND_NUM];
	float Pey = 1.0, Pyy = 1.0;
	float Eh, Yh;
	float Syy = 0.0, See = 0.0;
	float tmp32;
	float alpha;
	float leak_estimate;

	if (NULL == srv) {
		return ERR_AEC;
	}
	if (stage == 1)
	{
		/* Compute power spectrum of the echo */
		for (i = AEC_LOW_CHAN; i < AEC_HIGH_CHAN; i++)
		{
			srv->echoPsd[i] = complex_abs2(srv->Xf_echo[i]);
			resPsd[i] = complex_abs2(srv->Xf_res_echo[i]);

			/* Compute filtered spectra and (cross-)correlations */
			Eh = resPsd[i] - srv->Eh[i];
			Yh = srv->echoPsd[i] - srv->Yh[i];
			Pey += (Eh * Yh);
			Pyy += (Yh * Yh);
			srv->Eh[i] = (1 - srv->spec_average)*srv->Eh[i] + srv->spec_average * resPsd[i];
			srv->Yh[i] = (1 - srv->spec_average)*srv->Yh[i] + srv->spec_average * srv->echoPsd[i];
			Syy += srv->echoPsd[i];
			See += resPsd[i];

		}
		if (See < 25600)
			See = 25600;

		Pyy = (float)sqrt(Pyy);
		Pey = Pey / Pyy;

		/* Compute correlation updatete rate */
		tmp32 = srv->beta0 * Syy;
		if (tmp32 > srv->beta_max * See)
			tmp32 = srv->beta_max * See;
		alpha = tmp32 / See;

		//update average pey pyy
		srv->Pey_avg = (1 - alpha) * srv->Pey_avg + alpha * Pey;
		srv->Pyy_avg = (1 - alpha) * srv->Pyy_avg + alpha * Pyy;

		if (srv->Pyy_avg < 1.0)
			srv->Pyy_avg = 1.0;
		if (srv->Pey_avg < srv->Pyy_avg * MIN_LEAK)
			srv->Pey_avg = srv->Pyy_avg * MIN_LEAK;
		if (srv->Pey_avg > srv->Pyy_avg)
			srv->Pey_avg = srv->Pyy_avg;
	}

	leak_estimate = srv->Pey_avg /srv->Pyy_avg;

	if (leak_estimate>.5)
		leak_estimate = 1;
	else
		leak_estimate = 2 * leak_estimate;

	/* stage 1: use default res to get the data for dtd */
	/* stage 2: protect doubletalk and make more suppression for single talk */
	if (stage == 1)
	{
		leak_estimate = leak_estimate * srv->res1_suppress_factor;
	}
	else
	{
		if (dt_st == SINGLE_TALK_STATUS)
		{
			leak_estimate = leak_estimate * srv->res2_st_suppress_factor;
		}
		else
		{
			leak_estimate = leak_estimate * srv->res2_dt_suppress_factor;
		}
	}

	/* Estimate residual echo */
	for (i = AEC_LOW_CHAN; i < AEC_HIGH_CHAN; i++)
	{
		residual_echo[i] = leak_estimate * srv->echoPsd[i];
	}
	return 0;
}

float dios_ssp_aec_res_prioriser(objRES* srv, float *ps, float *postSer, float *prioriSer)
{
	int i;
	float gamma;
	float Zframe = 0;
	
	/* Special case for first frame */
	if (srv->nb_adapt == 1)
		for (i = AEC_LOW_CHAN; i < AEC_HIGH_CHAN; i++)
			srv->res_old_ps[i] = ps[i];
	for (i = AEC_LOW_CHAN; i < AEC_HIGH_CHAN; i++)
	{
		srv->res_echo_noise[i] = xmax(srv->res_echo_noise_factor * srv->res_echo_noise[i], srv->res_echo_psd[i]);
		srv->res_echo_noise[i] = xmax(srv->res_echo_noise[i], 1e-10f);

		/* A posteriori SNR = ps/noise - 1*/
		postSer[i] = ps[i] / srv->res_echo_noise[i] - 1.0f;
		postSer[i] = xmin(postSer[i], POSTSER_THR);

		/* Computing update gamma = .1 + .9*(old/(old+noise))^2 */
		gamma = (float)(0.1f + 0.9f * pow(srv->res_old_ps[i] / (srv->res_old_ps[i] + srv->res_echo_noise[i]), 2));

		/* A priori SNR update = gamma*max(0,post) + (1-gamma)*old/noise */
		prioriSer[i] = gamma * xmax(0, postSer[i]) + (1.0f - gamma) * (srv->res_old_ps[i] / srv->res_echo_noise[i]);
		prioriSer[i] = xmin(prioriSer[i], PRIORISER_THR);
	}

	/* Recursive average of the a priori SNR. A bit smoothed for the psd components */
	for (i = AEC_LOW_CHAN; i < AEC_HIGH_CHAN; i++)
	{
		if (i == AEC_LOW_CHAN || i == (AEC_HIGH_CHAN - 1))
			srv->res_zeta[i] = 0.7f * srv->res_zeta[i] + 0.3f * prioriSer[i];
		else
			srv->res_zeta[i] = 0.7f*srv->res_zeta[i] + 0.15f*prioriSer[i] + 0.075f*prioriSer[i - 1] + 0.075f*prioriSer[i + 1];
			
		/* Speech probability of presence for the entire frame is based on the average a priori SNR */
		Zframe += srv->res_zeta[i];
	}
	return Zframe;
}

static float dios_ssp_aec_res_qcurve(float x)
{
	return 1.f / (1.f + .15f / x);
}

static void dios_ssp_aec_res_compute_gain_floor(float effective_echo_suppress, float *echo, float *gain_floor)
{
	int i;
	float echo_floor;

	echo_floor = (float)exp(.2302585f*effective_echo_suppress);

	/* Compute the gain floor based on different floors for the background noise and residual echo */
	for (i = AEC_LOW_CHAN; i < AEC_HIGH_CHAN; i++)
		gain_floor[i] = (float)(sqrt(echo_floor * echo[i]) / sqrt(1 + echo[i]));
}

/* This function approximates the gain function y = gamma(1.25)^2 * M(-.25;1;-x) / sqrt(x)
 * which multiplied by xi/(1+xi) is the optimal gain in the loudness domain ( sqrt[amplitude] )
 */
static float dios_ssp_aec_res_hypergeom_gain(float xx)
{
	int ind;
	float integer, frac;
	static const float table[21] = {
		0.82157f, 1.02017f, 1.20461f, 1.37534f, 1.53363f, 1.68092f, 1.81865f,
		1.94811f, 2.07038f, 2.18638f, 2.29688f, 2.40255f, 2.50391f, 2.60144f,
		2.69551f, 2.78647f, 2.87458f, 2.96015f, 3.04333f, 3.12431f, 3.20326f };
	
	integer = (float)floor(2 * xx);
	ind = (int)integer;
	if (ind<0)
		return 1.0f;
	if (ind>19)
		return (float)(1 + .1296 / xx);
	frac = 2 * xx - integer;
	return (float)(((1 - frac)*table[ind] + frac * table[ind + 1]) / sqrt(xx + .0001f));
}

objRES* dios_ssp_aec_res_init(void)
{
	int ret = 0;
	objRES* srv = NULL;
	srv = (objRES *)calloc(1, sizeof(objRES));

    srv->ccsize = AEC_FFT_LEN/2 + 1;
	srv->echoPsd = (float *)calloc(srv->ccsize, sizeof(float));
	srv->res1_old_ps = (float *)calloc(srv->ccsize, sizeof(float));
	srv->res2_old_ps = (float *)calloc(srv->ccsize, sizeof(float));
	srv->Eh = (float *)calloc(srv->ccsize, sizeof(float));
	srv->Yh = (float *)calloc(srv->ccsize, sizeof(float));
	srv->res1_echo_noise = (float *)calloc(srv->ccsize, sizeof(float));
	srv->res2_echo_noise = (float *)calloc(srv->ccsize, sizeof(float));
	srv->res1_zeta = (float *)calloc(srv->ccsize, sizeof(float));
	srv->res2_zeta = (float *)calloc(srv->ccsize, sizeof(float));

	ret = dios_ssp_aec_res_reset(srv);
	if (0 != ret)
	{
		return NULL;
	}
	return srv;
}

int dios_ssp_aec_res_reset(objRES *srv)
{
	int j;
	if (NULL == srv)
	{
		return ERR_AEC;
	}
	srv->spec_average = 128.0f / 16000.0f; 
	srv->beta0 = 2.0f * 128.0f / 16000.0f;//(2.0f*st->frame_size) / st->sampling_rate;
	srv->beta_max = srv->beta0 / 4.0f; //(.5f*st->frame_size) / st->sampling_rate;
	srv->Pey_avg = 1.0;
	srv->Pyy_avg = 1.0;
	srv->nb_adapt = 0;

    srv->ccsize = AEC_FFT_LEN/2 + 1;
    for (j = 0; j < srv->ccsize; j++)
    {
		srv->echoPsd[j] = 0.0f;
		srv->res1_old_ps[j] = 16384.0f;
		srv->res2_old_ps[j] = 16384.0f;
		srv->Eh[j] = 0.0f;
		srv->Yh[j] = 0.0f;
		srv->res1_echo_noise[j] = 0.0f;
		srv->res2_echo_noise[j] = 0.0f;
		srv->res1_zeta[j] = 0.0f;
		srv->res2_zeta[j] = 0.0f;
    }
	return 0;
}

/* perform the non-linear processing */
/* input buffer and output buffer are the same buffer */
int dios_ssp_aec_res_process(objRES *srv, int dt_st, int stage)
{
	int i;
	int ret = 0;
    float ps[AEC_SUBBAND_NUM];
	float resEchoPsd[AEC_SUBBAND_NUM];
	float postSer[AEC_SUBBAND_NUM];
	float prioriSer[AEC_SUBBAND_NUM];
	float gain[AEC_SUBBAND_NUM];
	float gain_floor[AEC_SUBBAND_NUM];
	float Zframe = 0;
	float Pframe;
	float effective_echo_suppress;

	if (NULL == srv)
	{
		return ERR_AEC;
	}

	ret = dios_ssp_aec_res_get_residual_echo(srv, resEchoPsd, dt_st, stage);
	if (ret != 0)
	{
		return ERR_AEC;
	}

	srv->res_echo_psd = resEchoPsd;

	for (i = AEC_LOW_CHAN; i < AEC_HIGH_CHAN; i++)
    {      
        ps[i] = complex_abs2(srv->Xf_res_echo[i]);
    }
	
	/* Cal post ser and priori ser */
	if (stage == 1)
	{
		srv->res_echo_noise = srv->res1_echo_noise;
		srv->res_echo_noise_factor = srv->res1_echo_noise_factor;
		srv->res_zeta = srv->res1_zeta;
		srv->res_old_ps = srv->res1_old_ps;

		srv->nb_adapt++;
		if (srv->nb_adapt > 20000)
			srv->nb_adapt = 20000;
	}
	else
	{
		srv->res_echo_noise = srv->res2_echo_noise;
		srv->res_echo_noise_factor = srv->res2_echo_noise_factor;
		srv->res_zeta = srv->res2_zeta;
		srv->res_old_ps = srv->res2_old_ps;
	}
	
	Zframe = dios_ssp_aec_res_prioriser(srv, ps, postSer, prioriSer);
	
	Zframe = xmax(Zframe, 1e-10f);
	Pframe = 0.1f + 0.9f * dios_ssp_aec_res_qcurve(Zframe / (AEC_HIGH_CHAN - AEC_LOW_CHAN));
	
	
	if (stage == 1)
	{
		effective_echo_suppress = (1.0f - Pframe) * srv->res1_echo_suppress_default + Pframe * srv->res1_echo_suppress_active_default;
		dios_ssp_aec_res_compute_gain_floor(effective_echo_suppress, srv->res1_echo_noise, gain_floor);
	}
	else
	{
		if (dt_st == 1)
		{
			effective_echo_suppress = (1.0f - Pframe) * srv->res2_st_echo_suppress_default + Pframe * srv->res2_st_echo_suppress_active_default;
		}
		else
		{
			effective_echo_suppress = (1.0f - Pframe) * srv->res2_dt_echo_suppress_default + Pframe * srv->res2_dt_echo_suppress_active_default;
		}		
		dios_ssp_aec_res_compute_gain_floor(effective_echo_suppress, srv->res2_echo_noise, gain_floor);
	}


	
	/* See EM and Cohen papers*/
	float theta;
	/* Gain from hypergeometric function */
	float MM;
	/* Weiner filter gain */
	float prior_ratio;
	/* a priority probability of speech presence*/
	float P1;
	/* Speech absence a priori probability */
	float q;
	/* Speech presence a priori probability */
	float p;
	
	/* Calculate the final residual echo output*/
	for (i = AEC_LOW_CHAN; i < AEC_HIGH_CHAN; i++)
	{
		/* Wiener filter gain */
		prior_ratio = prioriSer[i] / (prioriSer[i] + 1);
		theta = prior_ratio * (1.0f + postSer[i]);

		/* Optimal estimator for loudness domain */
		MM = dios_ssp_aec_res_hypergeom_gain(theta);

		/* EM Gain with bound */
		gain[i] = xmin(1.0f, prior_ratio * MM);
		
		/* get speech probability of presence */
		if (stage == 1)
		{
			P1 = 0.199f + 0.8f * dios_ssp_aec_res_qcurve(srv->res1_zeta[i]);
		}
		else
		{
			P1 = 0.199f + 0.8f * dios_ssp_aec_res_qcurve(srv->res2_zeta[i]);
		}
		q = 1.0f - Pframe * P1;
		p = (float)(1.0f / (1.0f + (q / (1.0f - q))*(1.0f + prioriSer[i])*exp(-theta)));

		/* Save old power spectrum */
		if (stage == 1)
		{			
			srv->res1_old_ps[i] = 0.2f * srv->res1_old_ps[i] + 0.8f * (gain[i] * gain[i]) * ps[i];
		}
		else
		{
			srv->res2_old_ps[i] = 0.2f * srv->res2_old_ps[i] + 0.8f * (gain[i] * gain[i]) * ps[i];
		}

		/*Apply gain floor*/
		gain[i] = xmax(gain[i], gain_floor[i]);

#if 1
		/* Take into account speech probability of presence (loudness domain MMSE estimator) */
		/* gain2 = [p*sqrt(gain)+(1-p)*sqrt(gain _floor) ]^2 */
		gain[i] = (float)(pow(p * sqrt(gain[i]) + (1.0f - p) * sqrt(gain_floor[i]), 2));
#else
		/* Use this if you want a log-domain MMSE estimator instead */
		gain[i] = pow(gain[i], p) * pow(gain_floor[i],1.f-p);
#endif

		// get res output and save the outpsd for next priori SER
		srv->Xf_res_echo[i].r *= gain[i];
		srv->Xf_res_echo[i].i *= gain[i];
	}

	// get res output and save the outpsd for next priori SER
	for (i = 0; i < AEC_LOW_CHAN; i++)
	{
		srv->Xf_res_echo[i].r = 0;
		srv->Xf_res_echo[i].i = 0;
	}
	for (i = AEC_HIGH_CHAN; i < AEC_SUBBAND_NUM; i++)
	{
		srv->Xf_res_echo[i].r = 0;
		srv->Xf_res_echo[i].i = 0;
	}
	return 0;
}

/* destroy and free space */
int dios_ssp_aec_res_unit(objRES *srv)
{
	if (NULL == srv)
	{
		return ERR_AEC;
	}
	free(srv->echoPsd);
	free(srv->res1_old_ps);
	free(srv->res2_old_ps);
	free(srv->res1_echo_noise);
	free(srv->res2_echo_noise);
	free(srv->res1_zeta);
	free(srv->res2_zeta);
	free(srv->Eh);
	free(srv->Yh);
	free(srv);
	return 0;
}
