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

Description: Noise power spectral Density estimation based on optimal
smoothing and minimum statistics.
==============================================================================*/

#include "dios_ssp_gsc_rmNPsdOsMs.h"

float dios_ssp_gsc_rmnpsdosms_calculateM(int D)
{	
	float MTab[14] = {0.f, 0.26f, 0.48f, 0.58f, 0.61f, 0.668f, 0.705f, 
        0.762f, 0.8f, 0.841f, 0.865f, 0.89f, 0.9f, 0.91f};
	int DTab[14] = {1, 2, 5, 8, 10, 15, 20, 30, 40, 60, 80, 120, 140, 160};		
	float tempM = 0;
	int index = 0;
	
	if (D == 1)
    {
		return MTab[0];
    }
	
	while (D > DTab[index])
	{
		index = index + 1;
		if (D == DTab[index])
		{
			return(MTab[index]);
		}
		else if (D < DTab[index])
		{
			tempM = MTab[index - 1] + (D - DTab[index - 1]) * 
                (MTab[index] - MTab[index - 1]) / (DTab[index] - DTab[index - 1]);
			return tempM;
		}
	}

	return tempM;
}

int dios_ssp_gsc_rmnpsdosms_computesmoothingparameter(objCNPsdOsMs *npsdosms1, float *pPSDInput)
{
	int i;
	float ALPHAcs = 0.f;
	float temp = 0.f;
	float param = 0.f;
	
	for (int i = 0; i < npsdosms1->m_L; i++)
    {
		/* temp = sum(|Y(k)|^2) */
		temp += pPSDInput[i];
		/* ALPHAcs = sum(P) */
		ALPHAcs += npsdosms1->m_P[i];
    }
	/* ALPHAcs = sum(P)/(sum(|Y(k)|^2)+epsilon) */	
	ALPHAcs = ALPHAcs / (temp + epsilon);
	/* ALPHAcs = [sum(P)/(sum(|Y(k)|^2)+epsilon)]-1 */
	ALPHAcs = ALPHAcs - 1;
	/* ALPHAcs = {[sum(P)/(sum(|Y(k)|^2)+epsilon)]-1}^2 */
	ALPHAcs = ALPHAcs * ALPHAcs;
	/* ALPHAcs = 1 +  {[sum(P)/(sum(|Y(k)|^2)+epsilon)]-1}^2 */
	ALPHAcs = 1 + ALPHAcs;
	/* ALPHAcs = 1/(1 +  {[sum(P)/(sum(|Y(k)|^2)+epsilon)]-1}^2) */
	ALPHAcs = 1 / ALPHAcs;								

	/* Eq.(10) */
	param = ALPHAcs > 0.7f ? ALPHAcs : 0.7f;
	npsdosms1->m_ALPHAc = 0.7f * npsdosms1->m_ALPHAc + 0.3f * param; 

	/* Eq.(12) */
	float temp_alpha_min = (float)pow(npsdosms1->m_SNR, (-npsdosms1->m_R / (0.064f * npsdosms1->m_fs)));
	temp_alpha_min = npsdosms1->m_alpha_min < temp_alpha_min?npsdosms1->m_alpha_min : temp_alpha_min;

	/* Eq.(11) */
	for (i = 0; i < npsdosms1->m_L; i++)
	{
		/* 1.temp(k) = 1/N(k) */
		if (npsdosms1->m_N[i] < epsilon)
        {
			npsdosms1->m_temp[i] = 1.f / epsilon;
        }
		else
        {
			npsdosms1->m_temp[i] = 1.f / npsdosms1->m_N[i];
        }
		/* 2.temp(k) = P(k)/N(k) */
		npsdosms1->m_temp[i] *= npsdosms1->m_P[i];

		/* 3.temp(k) = P(k)/N(k)-1 */
		npsdosms1->m_temp[i] -= 1.f;

		/* 4.temp(k) = [P(k)/N(k)-1]^2 */
		npsdosms1->m_temp[i] *= npsdosms1->m_temp[i];

		/* 5.temp(k) = 1+[P(k)/N(k)-1]^2 */
		npsdosms1->m_temp[i] += 1.f;

		/* 6.temp(k) = 1/{1+[P(k)/N(k)-1]^2} */
		if (npsdosms1->m_temp[i] < epsilon)
        {
			npsdosms1->m_temp[i] = 1.f / epsilon;
        }
		else
        {
			npsdosms1->m_temp[i] = 1.f / npsdosms1->m_temp[i];
        }

		/* 7.temp(k) *= alpha_max*alpha_c */
		npsdosms1->m_temp[i] *= npsdosms1->m_alpha_max * npsdosms1->m_ALPHAc;

		npsdosms1->m_alpha[i] = temp_alpha_min > npsdosms1->m_temp[i]?temp_alpha_min : npsdosms1->m_temp[i];
	}

	return 0;
}

/* computation of the Bias introduced because of using 
 * subwindows and short time estimation. */
int dios_ssp_gsc_rmnpsdosms_computebiascorrection(objCNPsdOsMs *npsdosms1)
{
	int i;
	float avrg_q_inv = 0.f;
	float param = 0.f;
	
	for (i = 0; i < npsdosms1->m_L; i++)
    {
		/* 1.beta(k)=min(alpha(k)^2,0.8) */
		param = npsdosms1->m_alpha[i] * npsdosms1->m_alpha[i];
		npsdosms1->m_beta[i] = 0.8f < param ? 0.8f : param;
		npsdosms1->m_P1m[i] = npsdosms1->m_beta[i] * npsdosms1->m_P1m[i] + (1.0f - npsdosms1->m_beta[i]) * npsdosms1->m_P[i];
		npsdosms1->m_P2m[i] = npsdosms1->m_beta[i] * npsdosms1->m_P2m[i] + (1.0f - npsdosms1->m_beta[i]) * npsdosms1->m_P[i] * npsdosms1->m_P[i];

		/* 2.var{P} = P2m - P1m^2 */
		npsdosms1->m_varP[i] = npsdosms1->m_P2m[i] - npsdosms1->m_P1m[i] * npsdosms1->m_P1m[i];

		/* 3.Qeq = (2*SIGMAn^4)/var{P} */
		param = 2.0f * npsdosms1->m_N[i] * npsdosms1->m_N[i] / (npsdosms1->m_varP[i] + epsilon);
		npsdosms1->m_Qeq[i] = 2.0f > param ? 2.0f : param;

		/* 4.QeqS = (Qeq - 2*M(D))/(1-M(D)) */
		npsdosms1->m_sQeq[i] = (npsdosms1->m_Qeq[i] - 2.0f * npsdosms1->m_M) / (1.0f - npsdosms1->m_M);

		/* 5.Bmin = 1+(D-1)*2/sQeq */
		npsdosms1->m_Bmin[i] = 1.0f + 2.0f * (npsdosms1->m_D - 1.0f) / npsdosms1->m_sQeq[i];

		/* 6.QeqS_sub = (Qeq - 2*M(V))/(1-M(V)) */
		npsdosms1->m_sQeqSub[i] = (npsdosms1->m_Qeq[i] - 2.0f * npsdosms1->m_MSub) / (1.0f - npsdosms1->m_MSub);

		/* 7.Bmin_sub = 1+(V-1)*2/sQeq */
		npsdosms1->m_Bmin_sub[i] = 1.0f + 2.0f * (npsdosms1->m_V - 1.0f) / npsdosms1->m_sQeqSub[i];

		/* 8.Bc = 1 + av * sqrt(Q^(-1)) */
		if (npsdosms1->m_Qeq[i] < epsilon)
        {
			npsdosms1->m_temp[i] = 1.0f / epsilon;
        }
		else
        {
			npsdosms1->m_temp[i] = 1.0f / npsdosms1->m_Qeq[i];
        }

		avrg_q_inv += npsdosms1->m_temp[i];
    }

	avrg_q_inv = avrg_q_inv / npsdosms1->m_L;					
	npsdosms1->m_Bc = 1.0f + npsdosms1->m_av * (float)sqrt(avrg_q_inv);
	
	/* noise_slope_max (in Fig.5) */
	if (avrg_q_inv < 0.03)
    {
		npsdosms1->m_noise_slope_max = 8.0f;
    }
	else if (avrg_q_inv < 0.05)
    {
		npsdosms1->m_noise_slope_max = 4.0f;
    }
	else if (avrg_q_inv < 0.06)
    {
		npsdosms1->m_noise_slope_max = 2.0f;
    }
	else
    {
		npsdosms1->m_noise_slope_max = 1.2f;
    }

	return 0;
}

/* search of the minimum of the smoothed PSD P and 
 * update of the estimation of the noise N */
int dios_ssp_gsc_rmnpsdosms_findminimum(objCNPsdOsMs *npsdosms1)
{
	int u, k;

	for (k = 0; k < npsdosms1->m_L; k++)
	{
		npsdosms1->m_k_mod[k] = 0;
		if (npsdosms1->m_P[k]*npsdosms1->m_Bmin[k]*npsdosms1->m_Bc < npsdosms1->m_actmin[k])
		{
			npsdosms1->m_actmin[k] = npsdosms1->m_P[k] * npsdosms1->m_Bmin[k] * npsdosms1->m_Bc;
			npsdosms1->m_actmin_sub[k] = npsdosms1->m_P[k] * npsdosms1->m_Bmin_sub[k] * npsdosms1->m_Bc;
			npsdosms1->m_k_mod[k] = 1;
		}
	}
	if (npsdosms1->m_subwc == npsdosms1->m_V)
	{
		npsdosms1->m_subwc = 1;
		for (k = 0; k < npsdosms1->m_L; k++)
		{
			if (npsdosms1->m_k_mod[k])
			{
				npsdosms1->m_lmin_flag[k] = 0;
			}
			/* store actmin */
			npsdosms1->m_store[k][npsdosms1->m_Ucount - 1] = npsdosms1->m_actmin[k];
			/* find minimum of last U stored actmin(k) */
			npsdosms1->m_Pmin_u[k] = npsdosms1->m_store[k][0];
			for (u = 1; u < npsdosms1->m_U; u++)
            {
				npsdosms1->m_Pmin_u[k] = npsdosms1->m_store[k][u] < npsdosms1->m_Pmin_u[k]?npsdosms1->m_store[k][u] : npsdosms1->m_Pmin_u[k];
            }
			
			if ((npsdosms1->m_lmin_flag[k]) && (npsdosms1->m_actmin_sub[k] < npsdosms1->m_noise_slope_max * npsdosms1->m_Pmin_u[k]) 
                    && (npsdosms1->m_actmin_sub[k] > npsdosms1->m_Pmin_u[k]))
			{
				npsdosms1->m_Pmin_u[k] = npsdosms1->m_actmin_sub[k];
				/* replace all previous stored values of actmin(k) by actmin_sub(k) */
				for (u = 0; u < npsdosms1->m_U; u++)
                {
					npsdosms1->m_store[k][u] = npsdosms1->m_actmin_sub[k];
                }
			}	
			npsdosms1->m_lmin_flag[k] = 0;
		}

		/* set actmin(k) and actmin_sub(k) to their maximum values */
		for (k = 0; k < npsdosms1->m_L; k++)
		{
			npsdosms1->m_actmin[k] = actmin_max;
			npsdosms1->m_actmin_sub[k] = actmin_max;
		}
		
		if (npsdosms1->m_Ucount == npsdosms1->m_U)
        {
			npsdosms1->m_Ucount = 1;
        }
		else
        {
			npsdosms1->m_Ucount = npsdosms1->m_Ucount + 1;
        }
	}
	else 
	{
		for (k = 0; k < npsdosms1->m_L; k++)
		{
			if(npsdosms1->m_k_mod[k])
			{
				npsdosms1->m_lmin_flag[k] = 1;
			}
			npsdosms1->m_Pmin_u[k] = npsdosms1->m_actmin_sub[k] < npsdosms1->m_Pmin_u[k]?npsdosms1->m_actmin_sub[k] : npsdosms1->m_Pmin_u[k];
			npsdosms1->m_N[k] = npsdosms1->m_Pmin_u[k];
		}
		npsdosms1->m_subwc++;
	}

	return 0;
}

void dios_ssp_gsc_rmnpsdosms_init(objCNPsdOsMs *npsdosms1, float fs, int L, int R, int U, int V)
{
	int k;
	float alpha_max = 0.96f;
	float alpha_min = 0.30f;
	float av = 2.12f;
	npsdosms1->m_P = NULL;
	npsdosms1->m_N = NULL;
	npsdosms1->m_alpha = NULL;
	npsdosms1->m_temp = NULL;
	npsdosms1->m_beta = NULL;					
	npsdosms1->m_P1m = NULL;						
	npsdosms1->m_P2m = NULL;
	npsdosms1->m_varP = NULL;				
	npsdosms1->m_Qeq = NULL;					
	npsdosms1->m_sQeq = NULL;					
	npsdosms1->m_sQeqSub = NULL;				
	npsdosms1->m_Bmin = NULL;					
	npsdosms1->m_Bmin_sub = NULL;
	npsdosms1->m_k_mod = NULL;					
	npsdosms1->m_lmin_flag = NULL;				
	npsdosms1->m_actmin = NULL;					
	npsdosms1->m_actmin_sub = NULL;				
	npsdosms1->m_Pmin_u = NULL;					
	npsdosms1->m_store = NULL;

	npsdosms1->m_fs = fs;
	npsdosms1->m_L = L;	
	npsdosms1->m_R = R;
	npsdosms1->m_U = U;
	npsdosms1->m_V = V;
	npsdosms1->m_D = npsdosms1->m_U * npsdosms1->m_V;
	npsdosms1->m_alpha_max = alpha_max;
	npsdosms1->m_alpha_min = alpha_min;
	npsdosms1->m_av = av;

	npsdosms1->m_M = dios_ssp_gsc_rmnpsdosms_calculateM(npsdosms1->m_D);		/* default: M = 0.875 */
	npsdosms1->m_MSub = dios_ssp_gsc_rmnpsdosms_calculateM(npsdosms1->m_V);	/* default: MSub = 0.6332 */

	npsdosms1->m_ALPHAc = 1.0f;
	npsdosms1->m_Bc = 0.f;
	npsdosms1->m_noise_slope_max = 0.f;
	npsdosms1->m_subwc = npsdosms1->m_V;
	npsdosms1->m_Ucount = 1;	
	npsdosms1->m_SNRcount = 1;
	npsdosms1->m_sumP = 0.f;
	npsdosms1->m_sumN = 0.f;
	npsdosms1->m_SNR = 100.f;

	npsdosms1->m_P = (float*)calloc(npsdosms1->m_L, sizeof(float));
	npsdosms1->m_N = (float*)calloc(npsdosms1->m_L, sizeof(float));
	npsdosms1->m_alpha = (float*)calloc(npsdosms1->m_L, sizeof(float));
	npsdosms1->m_temp = (float*)calloc(npsdosms1->m_L, sizeof(float));
	npsdosms1->m_beta = (float*)calloc(npsdosms1->m_L, sizeof(float));
	npsdosms1->m_P1m = (float*)calloc(npsdosms1->m_L, sizeof(float));
	npsdosms1->m_P2m = (float*)calloc(npsdosms1->m_L, sizeof(float));
	npsdosms1->m_varP = (float*)calloc(npsdosms1->m_L, sizeof(float));
	npsdosms1->m_Qeq = (float*)calloc(npsdosms1->m_L, sizeof(float));	
	npsdosms1->m_sQeq = (float*)calloc(npsdosms1->m_L, sizeof(float));
	npsdosms1->m_sQeqSub = (float*)calloc(npsdosms1->m_L, sizeof(float));	
	npsdosms1->m_Bmin = (float*)calloc(npsdosms1->m_L, sizeof(float));
	npsdosms1->m_Bmin_sub = (float*)calloc(npsdosms1->m_L, sizeof(float));
	npsdosms1->m_k_mod = (float*)calloc(npsdosms1->m_L, sizeof(float));	
	npsdosms1->m_lmin_flag = (float*)calloc(npsdosms1->m_L, sizeof(float));
	npsdosms1->m_actmin = (float*)calloc(npsdosms1->m_L, sizeof(float));
	npsdosms1->m_actmin_sub = (float*)calloc(npsdosms1->m_L, sizeof(float));
	npsdosms1->m_Pmin_u = (float*)calloc(npsdosms1->m_L, sizeof(float));
	npsdosms1->m_store = (float**)calloc(npsdosms1->m_L, sizeof(float*));
	for (int l = 0; l < npsdosms1->m_L; l++)
	{
		npsdosms1->m_store[l] = (float*)calloc(npsdosms1->m_U, sizeof(float));
	}

    memset(npsdosms1->m_k_mod, 0, npsdosms1->m_L);

	for (int k = 0; k < npsdosms1->m_L; k++)
    {
		for (int j = 0; j < npsdosms1->m_U; j++)
        {
			npsdosms1->m_store[k][j] = actmin_max;
        }
    }
	
	for (k = 0; k < npsdosms1->m_L; k++)
    {
		npsdosms1->m_Pmin_u[k] = actmin_max;
    }
	for (k = 0; k < npsdosms1->m_L; k++)
    {
		npsdosms1->m_actmin[k] = actmin_max;
    }
	for (k = 0; k < npsdosms1->m_L; k++)
    {
		npsdosms1->m_actmin_sub[k] = actmin_max;
    }
}

int dios_ssp_gsc_rmnpsdosms_reset(objCNPsdOsMs *npsdosms1)
{
	int k;
	npsdosms1->m_M = dios_ssp_gsc_rmnpsdosms_calculateM(npsdosms1->m_D);		/* default: M = 0.875 */
	npsdosms1->m_MSub = dios_ssp_gsc_rmnpsdosms_calculateM(npsdosms1->m_V);	/* default: MSub = 0.6332 */

	npsdosms1->m_ALPHAc = 1.0f;

	npsdosms1->m_Bc = 0.f;
	npsdosms1->m_noise_slope_max = 0.f;

	npsdosms1->m_subwc = npsdosms1->m_V;
	npsdosms1->m_Ucount = 1;	

	npsdosms1->m_SNRcount = 1;
	npsdosms1->m_sumP = 0.f;
	npsdosms1->m_sumN = 0.f;
	npsdosms1->m_SNR = 100.f;

	memset(npsdosms1->m_P, 0, sizeof(float) * npsdosms1->m_L);
	memset(npsdosms1->m_N, 0, sizeof(float) * npsdosms1->m_L);
	memset(npsdosms1->m_alpha, 0, sizeof(float) * npsdosms1->m_L);
	memset(npsdosms1->m_temp, 0, sizeof(float) * npsdosms1->m_L);
	memset(npsdosms1->m_beta, 0, sizeof(float) * npsdosms1->m_L);
	memset(npsdosms1->m_P1m, 0, sizeof(float) * npsdosms1->m_L);
	memset(npsdosms1->m_P2m, 0, sizeof(float) * npsdosms1->m_L);
	memset(npsdosms1->m_varP, 0, sizeof(float) * npsdosms1->m_L);
	memset(npsdosms1->m_Qeq, 0, sizeof(float) * npsdosms1->m_L);
	memset(npsdosms1->m_sQeq, 0, sizeof(float) * npsdosms1->m_L);
	memset(npsdosms1->m_sQeqSub, 0, sizeof(float) * npsdosms1->m_L);
	memset(npsdosms1->m_Bmin, 0, sizeof(float) * npsdosms1->m_L);
	memset(npsdosms1->m_Bmin_sub, 0, sizeof(float) * npsdosms1->m_L);
	memset(npsdosms1->m_k_mod, 0, sizeof(float) * npsdosms1->m_L);
	memset(npsdosms1->m_lmin_flag, 0, sizeof(float) * npsdosms1->m_L);
	memset(npsdosms1->m_actmin, 0, sizeof(float) * npsdosms1->m_L);
	memset(npsdosms1->m_actmin_sub, 0, sizeof(float) * npsdosms1->m_L);
	memset(npsdosms1->m_Pmin_u, 0, sizeof(float) * npsdosms1->m_L);

	for (int m = 0; m < npsdosms1->m_L; m++)
	{
		memset(npsdosms1->m_store[m], 0, sizeof(float) * npsdosms1->m_U);
	}
	
	memset(npsdosms1->m_k_mod, 0, npsdosms1->m_L);

	for (int k = 0; k < npsdosms1->m_L; k++)
    {
		for (int j = 0; j < npsdosms1->m_U; j++)
        {
			npsdosms1->m_store[k][j] = actmin_max;
        }
    }

	for (k = 0; k < npsdosms1->m_L; k++)
    {
		npsdosms1->m_Pmin_u[k] = actmin_max;
    }
	for (k = 0; k < npsdosms1->m_L; k++)
    {
		npsdosms1->m_actmin[k] = actmin_max;
    }
	for (k = 0; k < npsdosms1->m_L; k++)
    {
		npsdosms1->m_actmin_sub[k] = actmin_max;
    }

	return 0;
}

int dios_ssp_gsc_rmnpsdosms_process(objCNPsdOsMs *npsdosms1, float *pPSDInput)
{
	float param = 0.f;
	dios_ssp_gsc_rmnpsdosms_computesmoothingparameter(npsdosms1, pPSDInput);
			
	/* Smoothing PSD */
	/* P(k) = alpha(k)*P(k) + (1-alpha(k))*|Y(k)|^2 */
	for (int i = 0; i < npsdosms1->m_L; i++)
    {
		npsdosms1->m_P[i] = npsdosms1->m_alpha[i] * npsdosms1->m_P[i] + (1.0f - npsdosms1->m_alpha[i]) * pPSDInput[i];
    }
	
	dios_ssp_gsc_rmnpsdosms_computebiascorrection(npsdosms1);
	dios_ssp_gsc_rmnpsdosms_findminimum(npsdosms1);

	/* after every D samples the overall SNR is updated */
	for (int i = 0; i < npsdosms1->m_L; i++)
    {
		npsdosms1->m_sumP += npsdosms1->m_P[i];
		npsdosms1->m_sumN += npsdosms1->m_N[i];
    }

	if (npsdosms1->m_SNRcount == npsdosms1->m_D)
	{
		param = npsdosms1->m_sumP < npsdosms1->m_sumN?npsdosms1->m_sumP : npsdosms1->m_sumN;
		npsdosms1->m_SNR = (npsdosms1->m_sumP - param) / (npsdosms1->m_sumN + epsilon);
		npsdosms1->m_SNRcount = 1;
	}
	else
    {
		npsdosms1->m_SNRcount++;
    }

	return 0;
}

int dios_ssp_gsc_rmnpsdosms_delete(objCNPsdOsMs *npsdosms1)
{
	free(npsdosms1->m_P);
	free(npsdosms1->m_N);
	free(npsdosms1->m_alpha);
	free(npsdosms1->m_temp);
	free(npsdosms1->m_beta);
	free(npsdosms1->m_P1m);
	free(npsdosms1->m_P2m);
	free(npsdosms1->m_varP);
	free(npsdosms1->m_Qeq);
	free(npsdosms1->m_sQeq);
	free(npsdosms1->m_sQeqSub);
	free(npsdosms1->m_Bmin);
	free(npsdosms1->m_Bmin_sub);
	free(npsdosms1->m_k_mod);
	free(npsdosms1->m_lmin_flag);
	free(npsdosms1->m_actmin);
	free(npsdosms1->m_actmin_sub);
	free(npsdosms1->m_Pmin_u);
	for (int l = 0; l < npsdosms1->m_L; l++)
	{
		free(npsdosms1->m_store[l]);
	}
	free(npsdosms1->m_store);

	return 0;
}
