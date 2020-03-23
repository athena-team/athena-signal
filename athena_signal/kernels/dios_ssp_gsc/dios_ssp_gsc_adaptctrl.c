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

Description: The adaptation control estimates the frequency-dependent 
signal-to-noise ratio (SNR) based on spatial information. A fixed beamformer
is steered into the assumed target direction-of-arrival (DOA) yielding a rough
estimate of the desired signal frequency-dependent energy. A complementary null
beamformer with a spatial null in the assumed desired signal DOA yields a
frequency-dependent estimate of the interference energy. The ratio of both
energy estimates is taken as a frequnecy-dependent SNR estimate. If the SNR is
greater than a predefined threshold value tabm, then the blocking matrix is
adapted. If the SNR is less than another predefined threshold taic, then the
adaptive interference canceller is adapted. Otherwise the adaptation is stalled
in order to minimize target signal cancellation and interference leakage, and
to obtain better robustness in turn.
==============================================================================*/

#include "dios_ssp_gsc_adaptctrl.h"

void dios_ssp_gsc_gscadaptctrl_init(objFGSCadaptctrl *gscadaptctrl, const DWORD dwSampRate, const WORD wNumMic, const WORD wSyncDlyXref, 
            const WORD wSyncDlyYfbf, const WORD wSyncDlyAic, const DWORD dwFftSize, 
            const WORD wFftOverlap, const DWORD dwF0, const DWORD dwF1, const DWORD dwFc, 
            const float corrThresAbm, const float corrThresAic, 
            const int dwNumSubWindowsMinStat, const int dwSizeSubWindowsMinStat)
{
	gscadaptctrl->m_pfBuffer = NULL;
	gscadaptctrl->m_ppXrefDline = NULL;
	gscadaptctrl->m_pXfbfDline = NULL;
	gscadaptctrl->m_ppcfXref = NULL;
	gscadaptctrl->m_pcfXfbf = NULL;
	gscadaptctrl->m_pcfXcfbf = NULL;
	gscadaptctrl->m_pfPref = NULL;
	gscadaptctrl->m_pfPfbf = NULL;
	gscadaptctrl->m_pfPcfbf = NULL;
	gscadaptctrl->m_pfBeta = NULL;
	gscadaptctrl->m_pfBetaC = NULL;
	gscadaptctrl->m_ppfCtrlAicDline = NULL;

	gscadaptctrl->m_dwSampRate = dwSampRate;
	gscadaptctrl->m_wNumMic = wNumMic;
	gscadaptctrl->m_wSyncDlyXref = wSyncDlyXref;
	gscadaptctrl->m_wSyncDlyYfbf = wSyncDlyYfbf;
	gscadaptctrl->m_wSyncDlyCtrlAic = wSyncDlyAic;
	gscadaptctrl->m_dwFftSize = dwFftSize;
	gscadaptctrl->m_wFftOverlap = wFftOverlap;
	gscadaptctrl->m_corrThresAbm = corrThresAbm;
	gscadaptctrl->m_corrThresAic = corrThresAic;
	gscadaptctrl->m_nCCSSize = (int)(gscadaptctrl->m_dwFftSize / 2 + 1);

	gscadaptctrl->m_dwIndF0 = (DWORD)floor(dwF0 * (float)gscadaptctrl->m_dwFftSize / gscadaptctrl->m_dwSampRate);
	gscadaptctrl->m_dwIndF1 = (DWORD)floor(dwF1 * (float)gscadaptctrl->m_dwFftSize / gscadaptctrl->m_dwSampRate);
	gscadaptctrl->m_dwIndFc = (DWORD)floor(dwFc * (float)gscadaptctrl->m_dwFftSize / gscadaptctrl->m_dwSampRate);

	gscadaptctrl->m_delta = 0.001f;

	gscadaptctrl->npsdosms1 =  (objCNPsdOsMs*)calloc(1, sizeof(objCNPsdOsMs));
	dios_ssp_gsc_rmnpsdosms_init(gscadaptctrl->npsdosms1, (float)(gscadaptctrl->m_dwSampRate), gscadaptctrl->m_nCCSSize, (int)(gscadaptctrl->m_dwFftSize / gscadaptctrl->m_wFftOverlap), dwNumSubWindowsMinStat, dwSizeSubWindowsMinStat);
	gscadaptctrl->npsdosms2 =  (objCNPsdOsMs*)calloc(1, sizeof(objCNPsdOsMs));
	dios_ssp_gsc_rmnpsdosms_init(gscadaptctrl->npsdosms2, (float)(gscadaptctrl->m_dwSampRate), gscadaptctrl->m_nCCSSize, (int)(gscadaptctrl->m_dwFftSize / gscadaptctrl->m_wFftOverlap), dwNumSubWindowsMinStat, dwSizeSubWindowsMinStat);

	gscadaptctrl->adapt_FFT = dios_ssp_share_rfft_init((int)gscadaptctrl->m_dwFftSize);
	gscadaptctrl->fft_out = (float*)calloc(gscadaptctrl->m_dwFftSize, sizeof(float));
	gscadaptctrl->m_ppXrefDline = (float**)calloc(gscadaptctrl->m_wNumMic, sizeof(float*));
	for (int i_mic = 0; i_mic < gscadaptctrl->m_wNumMic; i_mic++)
	{
		gscadaptctrl->m_ppXrefDline[i_mic] = (float*)calloc(gscadaptctrl->m_dwFftSize + gscadaptctrl->m_wSyncDlyXref, sizeof(float));
	}
	gscadaptctrl->m_pXfbfDline = (float*)calloc(gscadaptctrl->m_dwFftSize + gscadaptctrl->m_wSyncDlyYfbf, sizeof(float));
	gscadaptctrl->m_ppcfXref = (xcomplex**)calloc(gscadaptctrl->m_wNumMic, sizeof(xcomplex*));
	for (int i_mic = 0; i_mic < gscadaptctrl->m_wNumMic; i_mic++)
	{
		gscadaptctrl->m_ppcfXref[i_mic] = (xcomplex*)calloc(gscadaptctrl->m_nCCSSize, sizeof(xcomplex));
	}
	gscadaptctrl->m_pcfXfbf = (xcomplex*)calloc(gscadaptctrl->m_nCCSSize, sizeof(xcomplex));
	gscadaptctrl->m_pcfXcfbf = (xcomplex*)calloc(gscadaptctrl->m_nCCSSize, sizeof(xcomplex));						
	gscadaptctrl->m_pfPcfbf = (float*)calloc(gscadaptctrl->m_nCCSSize, sizeof(float));
	gscadaptctrl->m_pfPfbf = (float*)calloc(gscadaptctrl->m_nCCSSize, sizeof(float));
	gscadaptctrl->m_pfPref = (float*)calloc(gscadaptctrl->m_nCCSSize, sizeof(float));
	gscadaptctrl->m_pfBeta = (float*)calloc(gscadaptctrl->m_nCCSSize, sizeof(float));
	gscadaptctrl->m_pfBetaC = (float*)calloc(gscadaptctrl->m_nCCSSize, sizeof(float));
	gscadaptctrl->m_pfBuffer = (float*)calloc(gscadaptctrl->m_nCCSSize, sizeof(float));
	gscadaptctrl->m_ppfCtrlAicDline = (float**)calloc(gscadaptctrl->m_wSyncDlyCtrlAic + 1, sizeof(float*));
	for (int i = 0; i < gscadaptctrl->m_wSyncDlyCtrlAic + 1; i++)
	{
		gscadaptctrl->m_ppfCtrlAicDline[i] = (float*)calloc(gscadaptctrl->m_nCCSSize, sizeof(float));
	}
}

int dios_ssp_gsc_gscadaptctrl_reset(objFGSCadaptctrl *gscadaptctrl)
{
	gscadaptctrl->m_delta = 0.001f;
	dios_ssp_gsc_rmnpsdosms_reset(gscadaptctrl->npsdosms1);
	dios_ssp_gsc_rmnpsdosms_reset(gscadaptctrl->npsdosms2);

	for (int m = 0; m < gscadaptctrl->m_wNumMic; m++)
	{
		memset(gscadaptctrl->m_ppXrefDline[m], 0, sizeof(float) * (gscadaptctrl->m_dwFftSize + gscadaptctrl->m_wSyncDlyXref));
		for (int n = 0; n < gscadaptctrl->m_nCCSSize; n++)
		{
			gscadaptctrl->m_ppcfXref[m][n].i = 0.0f;
			gscadaptctrl->m_ppcfXref[m][n].r = 0.0f;	
		}
	}

	for (DWORD i = 0; i < gscadaptctrl->m_dwFftSize; i++)
	{
		gscadaptctrl->fft_out[i] = 0.0f;
	}

	for (int m = 0; m < gscadaptctrl->m_wSyncDlyCtrlAic + 1; m++)
	{
		memset(gscadaptctrl->m_ppfCtrlAicDline[m], 0, sizeof(float) * gscadaptctrl->m_nCCSSize);
	}

	for (int n = 0; n < gscadaptctrl->m_nCCSSize; n++)
	{
		gscadaptctrl->m_pcfXfbf[n].i = 0.0f;
		gscadaptctrl->m_pcfXfbf[n].r = 0.0f;
		gscadaptctrl->m_pcfXcfbf[n].i = 0.0f;
		gscadaptctrl->m_pcfXcfbf[n].r = 0.0f;		
	}

	memset(gscadaptctrl->m_pXfbfDline, 0, sizeof(float) * (gscadaptctrl->m_dwFftSize + gscadaptctrl->m_wSyncDlyYfbf));
	memset(gscadaptctrl->m_pfPcfbf, 0, sizeof(float) * gscadaptctrl->m_nCCSSize);
	memset(gscadaptctrl->m_pfPfbf, 0, sizeof(float) * gscadaptctrl->m_nCCSSize);
	memset(gscadaptctrl->m_pfPref, 0, sizeof(float) * gscadaptctrl->m_nCCSSize);
	memset(gscadaptctrl->m_pfBeta, 0, sizeof(float) * gscadaptctrl->m_nCCSSize);
	memset(gscadaptctrl->m_pfBetaC, 0, sizeof(float) * gscadaptctrl->m_nCCSSize);
	memset(gscadaptctrl->m_pfBuffer, 0, sizeof(float) * gscadaptctrl->m_nCCSSize);

	return 0;
}

int dios_ssp_gsc_gscadaptctrl_process(objFGSCadaptctrl *gscadaptctrl, float *pXfbf, float **ppXref, const DWORD dwIndXref, 
            float *pfCtrlAbm, float *pfCtrlAic)
{
	float meanBeta;  /* value of mean energy ratio for f < f0 */

	/* psd estimation of the fixed beamformer output */
	/* delay line for the fixed beamformer output
     * 128 + 32 - 128 / (2 * 4)
     * FixBeamformer delay 32 samples */
	delayline(pXfbf, gscadaptctrl->m_pXfbfDline, (int)(gscadaptctrl->m_dwFftSize + gscadaptctrl->m_wSyncDlyYfbf - gscadaptctrl->m_dwFftSize / (2 * gscadaptctrl->m_wFftOverlap)), 
				(int)(gscadaptctrl->m_dwFftSize + gscadaptctrl->m_wSyncDlyYfbf));

	/* perform FFT operations */
	dios_ssp_share_rfft_process(gscadaptctrl->adapt_FFT, gscadaptctrl->m_pXfbfDline, gscadaptctrl->fft_out);
	for (DWORD i = 0; i < gscadaptctrl->m_dwFftSize / 2 + 1; i++)
	{
		gscadaptctrl->m_pcfXfbf[i].r = gscadaptctrl->fft_out[i];
	}
	gscadaptctrl->m_pcfXfbf[0].i = gscadaptctrl->m_pcfXfbf[gscadaptctrl->m_dwFftSize / 2].i = 0.0f;
	for (DWORD i = 1; i < gscadaptctrl->m_dwFftSize / 2; i++)
	{
		gscadaptctrl->m_pcfXfbf[i].i = -gscadaptctrl->fft_out[gscadaptctrl->m_dwFftSize - i];
	}

    /* instantaneous power spectrum estimation of output of fbf */
    for (int k = 0; k < gscadaptctrl->m_nCCSSize; k++)
    {
        gscadaptctrl->m_pfPfbf[k] = gscadaptctrl->m_pcfXfbf[k].r * gscadaptctrl->m_pcfXfbf[k].r + gscadaptctrl->m_pcfXfbf[k].i * gscadaptctrl->m_pcfXfbf[k].i;
    }

	memset(gscadaptctrl->m_pfPcfbf, 0, gscadaptctrl->m_nCCSSize * sizeof(float));

    for (int i = 0; i < gscadaptctrl->m_wNumMic; ++i)
    {
        /* synchronization of fbf output with reference mic signals
         * 128 + 32 - 128 / (2 * 4)
         * Input signals: delay 32 samples */
        delayline(&ppXref[i][dwIndXref], gscadaptctrl->m_ppXrefDline[i], 
				(int)(gscadaptctrl->m_dwFftSize + gscadaptctrl->m_wSyncDlyXref - gscadaptctrl->m_dwFftSize / (2 * gscadaptctrl->m_wFftOverlap)), 
				(int)(gscadaptctrl->m_dwFftSize + gscadaptctrl->m_wSyncDlyXref));

        
		dios_ssp_share_rfft_process(gscadaptctrl->adapt_FFT, gscadaptctrl->m_ppXrefDline[i], gscadaptctrl->fft_out);
		for (DWORD j = 0; j < gscadaptctrl->m_dwFftSize / 2 + 1; j++)
		{
			gscadaptctrl->m_ppcfXref[i][j].r = gscadaptctrl->fft_out[j];
		}
		gscadaptctrl->m_ppcfXref[i][0].i = gscadaptctrl->m_ppcfXref[i][gscadaptctrl->m_dwFftSize / 2].i = 0.0;
		for (DWORD j = 1; j < gscadaptctrl->m_dwFftSize / 2; j++)
		{
			gscadaptctrl->m_ppcfXref[i][j].i = -gscadaptctrl->fft_out[gscadaptctrl->m_dwFftSize - j];
		}
		for (int j = 0; j < gscadaptctrl->m_nCCSSize; j++)
        {
			/* 1.power spectrum estimation of reference mic signals */
			gscadaptctrl->m_pfBuffer[j] = gscadaptctrl->m_ppcfXref[i][j].r * gscadaptctrl->m_ppcfXref[i][j].r + gscadaptctrl->m_ppcfXref[i][j].i * gscadaptctrl->m_ppcfXref[i][j].i;

			/* 2.summation over all channels */
			gscadaptctrl->m_pfPref[j] += gscadaptctrl->m_pfBuffer[j];
        }
    }

    /* instantaneous power spectrum estimate of reference mic signals */
	for (int k = 0; k < gscadaptctrl->m_nCCSSize; k++)
    {
		gscadaptctrl->m_pfPref[k] /= (float)gscadaptctrl->m_wNumMic;
    }

    for (int i = 0; i < gscadaptctrl->m_wNumMic; i++)
    {
        /* complementary fbf */
		for (int j = 0; j < gscadaptctrl->m_nCCSSize; j++) 
        {
			gscadaptctrl->m_pcfXcfbf[j] = complex_sub(gscadaptctrl->m_ppcfXref[i][j], gscadaptctrl->m_pcfXfbf[j]);
			gscadaptctrl->m_pfBuffer[j] = gscadaptctrl->m_pcfXcfbf[j].r * gscadaptctrl->m_pcfXcfbf[j].r + gscadaptctrl->m_pcfXcfbf[j].i * gscadaptctrl->m_pcfXcfbf[j].i;
			gscadaptctrl->m_pfPcfbf[j] += gscadaptctrl->m_pfBuffer[j];
		}
    }

	/* average power spectrum estimate of complementary beamformer */
	for (int k = 0; k < gscadaptctrl->m_nCCSSize; k++)
    {
		gscadaptctrl->m_pfPcfbf[k] /= (float)gscadaptctrl->m_wNumMic;
		gscadaptctrl->m_pfPref[k] /= (float)gscadaptctrl->m_wNumMic;

		/* ratio of psd estimate of fbf output and psd estimate of complementary fbf output */
		/* energy ratio in discrete frequency bins (FCR) */
		if (gscadaptctrl->m_pfPcfbf[k] < gscadaptctrl->m_delta)
        {
			gscadaptctrl->m_pfBeta[k] = 1.0f / gscadaptctrl->m_delta;
        }
		else
        {
			gscadaptctrl->m_pfBeta[k] = 1.0f / gscadaptctrl->m_pfPcfbf[k];
        }
		gscadaptctrl->m_pfBeta[k] *= gscadaptctrl->m_pfPfbf[k];
    }

 	/* mean energy ratio for f < fc, determined over the interval f0 < f < f1 */
	meanBeta = 0.0;
	for (DWORD l = gscadaptctrl->m_dwIndF0; l < gscadaptctrl->m_dwIndF1; l++)
    {
		meanBeta += gscadaptctrl->m_pfBeta[l];
    }
	meanBeta /= (gscadaptctrl->m_dwIndF1 - gscadaptctrl->m_dwIndF0);
	for (DWORD l = 0; l < gscadaptctrl->m_dwIndFc; l++)
    {
		gscadaptctrl->m_pfBeta[l] = meanBeta;
    }

	/* determine lower thresholds in decision variables */
	dios_ssp_gsc_rmnpsdosms_process(gscadaptctrl->npsdosms1, gscadaptctrl->m_pfBeta);

	/* energy ratio in discrete frequency bins (CFR) */
	for (int k = 0; k < gscadaptctrl->m_nCCSSize; k++)
	{
		if (gscadaptctrl->m_pfBeta[k] < gscadaptctrl->m_delta)
        {
			gscadaptctrl->m_pfBetaC[k] = 1.f / gscadaptctrl->m_delta;
        }
		else
        {
			gscadaptctrl->m_pfBetaC[k] = 1.f / gscadaptctrl->m_pfBeta[k];
        }
	}

	/* determine lower thresholds in decision variables */
	dios_ssp_gsc_rmnpsdosms_process(gscadaptctrl->npsdosms2, gscadaptctrl->m_pfBetaC);

	/* decision abm <-> aic adaptation */
	memset(pfCtrlAbm, 0, gscadaptctrl->m_nCCSSize * sizeof(float));
	memset(gscadaptctrl->m_pfBuffer, 0, gscadaptctrl->m_nCCSSize * sizeof(float));

    gscadaptctrl->m_corrThresAic = 4.0f;
    gscadaptctrl->m_corrThresAbm = 0.8f;
	for (int k = 0; k < gscadaptctrl->m_nCCSSize; k++)
	{
		if(gscadaptctrl->npsdosms1->m_P[k] - gscadaptctrl->m_corrThresAic * gscadaptctrl->npsdosms1->m_N[k] < 0)
		{
			gscadaptctrl->m_pfBuffer[k] = 1;
		}
		if(gscadaptctrl->npsdosms2->m_P[k] - gscadaptctrl->m_corrThresAbm * gscadaptctrl->npsdosms2->m_N[k] < 0)
		{
			pfCtrlAbm[k] = 1;
		}
		if ((pfCtrlAbm[k] == 1) && (gscadaptctrl->m_pfBuffer[k] == 1))
		{
			gscadaptctrl->m_pfBuffer[k] = 0;
			pfCtrlAbm[k] = 0;
		}
	}

	/* sync delay of aic control signal
     * m_wSyncDlyCtrlAic = 2
     * aic control delay 2 block(2 frames) */
	for(int k = 0; k < gscadaptctrl->m_wSyncDlyCtrlAic; k++)
	{
		memcpy(gscadaptctrl->m_ppfCtrlAicDline[k+1], gscadaptctrl->m_ppfCtrlAicDline[k], gscadaptctrl->m_nCCSSize * sizeof(float));
	}
	memcpy(gscadaptctrl->m_ppfCtrlAicDline[0], gscadaptctrl->m_pfBuffer, gscadaptctrl->m_nCCSSize * sizeof(float));

	memcpy(pfCtrlAic, gscadaptctrl->m_ppfCtrlAicDline[gscadaptctrl->m_wSyncDlyCtrlAic], gscadaptctrl->m_nCCSSize * sizeof(float));

	return 0;
}

int dios_ssp_gsc_gscadaptctrl_delete(objFGSCadaptctrl *gscadaptctrl)
{
	int ret = 0;
	dios_ssp_gsc_rmnpsdosms_delete(gscadaptctrl->npsdosms1);
	free(gscadaptctrl->npsdosms1);
	dios_ssp_gsc_rmnpsdosms_delete(gscadaptctrl->npsdosms2);
	free(gscadaptctrl->npsdosms2);
	free(gscadaptctrl->fft_out);
	ret = dios_ssp_share_rfft_uninit(gscadaptctrl->adapt_FFT);
	if (0 != ret)
	{
		gscadaptctrl->adapt_FFT = NULL;
	}
	for (int i_mic = 0; i_mic < gscadaptctrl->m_wNumMic; i_mic++)
	{
		free(gscadaptctrl->m_ppXrefDline[i_mic]);
	}
	free(gscadaptctrl->m_ppXrefDline);
	free(gscadaptctrl->m_pXfbfDline);
	for (int i_mic = 0; i_mic < gscadaptctrl->m_wNumMic; i_mic++)
	{
		free(gscadaptctrl->m_ppcfXref[i_mic]);
	}
	free(gscadaptctrl->m_ppcfXref);
	free(gscadaptctrl->m_pcfXfbf);
	free(gscadaptctrl->m_pcfXcfbf);
	free(gscadaptctrl->m_pfPref);
	free(gscadaptctrl->m_pfBuffer);
	free(gscadaptctrl->m_pfPfbf);
	free(gscadaptctrl->m_pfPcfbf);
	free(gscadaptctrl->m_pfBeta);
	free(gscadaptctrl->m_pfBetaC);
	for (int i_mic = 0; i_mic < gscadaptctrl->m_wSyncDlyCtrlAic + 1; i_mic++)
	{
		free(gscadaptctrl->m_ppfCtrlAicDline[i_mic]);
	}
	free(gscadaptctrl->m_ppfCtrlAicDline);

	return 0;
}
