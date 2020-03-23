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

Description: The ABM consists of adaptive filters between the FBF output and
the sensor channels: The signal of interest is adaptively subtracted from the
sidelobe cancelling path in order to prevent target signal cancellation by the
AIC. The time delay ensures causality of the adaptive filters.
==============================================================================*/

#include "dios_ssp_gsc_abm.h"

void dios_ssp_gsc_gscabm_init(objFGSCabm *gscabm, int num_mic, int fft_size, int overlap_sigs, int overlap_fft, int dlysync, float forgetfactor, float stepsize, 
            float threshdiv0, long rate, float tconst_freezing)
{
	gscabm->Xdline = NULL;
	gscabm->xrefdline = NULL;
	gscabm->xfref = NULL;
	gscabm->ytmp = NULL;
	gscabm->yf = NULL;
	gscabm->e = NULL;
	gscabm->E = NULL;
	gscabm->ef = NULL;
	gscabm->muf = NULL;
	gscabm->nuf = NULL;
	gscabm->yftmp = NULL;
	gscabm->pxfref = NULL;
	gscabm->sf = NULL;
	gscabm->pftmp = NULL;
	gscabm->hf = NULL;
	gscabm->m_upper_bound = NULL;
	gscabm->m_lower_bound = NULL;
	gscabm->nmic = num_mic;
	gscabm->fftsize = fft_size;
	gscabm->fftoverlap = overlap_fft;
	gscabm->sigsoverlap = overlap_sigs;   
	gscabm->lambda = forgetfactor * (float)pow(1.0 - 1.0 / (3.0 * (float)gscabm->fftsize), 
            gscabm->fftsize / (2 * gscabm->fftoverlap));
	gscabm->mu = 2 * stepsize * (1 - gscabm->lambda);
	gscabm->delta = threshdiv0;
	gscabm->nu.r = 1.f - expf(-gscabm->fftsize / (2 * gscabm->fftoverlap * tconst_freezing * rate));
	gscabm->nu.i = 0.f;
	gscabm->syncdly = dlysync;
	gscabm->count_sigsegments = 0;
	gscabm->Xdline = (float**)calloc(gscabm->nmic, sizeof(float*));
	for (int i_mic = 0; i_mic < gscabm->nmic; i_mic++)
	{
		gscabm->Xdline[i_mic] = (float*)calloc(gscabm->fftsize, sizeof(float));
	}
	gscabm->xrefdline = (float*)calloc(gscabm->fftsize / 2 + gscabm->syncdly, sizeof(float));
	gscabm->xfref = (xcomplex*)calloc(gscabm->fftsize / 2 + 1, sizeof(xcomplex));
	gscabm->ytmp = (float*)calloc(gscabm->fftsize, sizeof(float));
	gscabm->yf = (xcomplex*)calloc(gscabm->fftsize / 2 + 1, sizeof(xcomplex));
	gscabm->e = (float*)calloc(gscabm->fftsize, sizeof(float));
	gscabm->E = (float**)calloc(gscabm->nmic, sizeof(float*));
	for (int i_mic = 0; i_mic < gscabm->nmic; i_mic++)
	{
		gscabm->E[i_mic] = (float*)calloc(gscabm->fftsize / (2 * gscabm->fftoverlap), sizeof(float));
	}
	gscabm->ef = (xcomplex*)calloc(gscabm->fftsize / 2 + 1, sizeof(xcomplex));
	gscabm->muf = (xcomplex*)calloc(gscabm->fftsize / 2 + 1, sizeof(xcomplex));
	gscabm->nuf = (xcomplex*)calloc(gscabm->fftsize / 2 + 1, sizeof(xcomplex));
	gscabm->yftmp = (xcomplex*)calloc(gscabm->fftsize / 2 + 1, sizeof(xcomplex));
	gscabm->pxfref = (float*)calloc(gscabm->fftsize / 2 + 1, sizeof(float));
	gscabm->sf = (float**)calloc(gscabm->nmic, sizeof(float*));
	for (int i_mic = 0; i_mic < gscabm->nmic; i_mic++)
	{
		gscabm->sf[i_mic] = (float*)calloc(gscabm->fftsize / 2 + 1, sizeof(float));
	}
	gscabm->pftmp = (float*)calloc(gscabm->fftsize / 2 + 1, sizeof(float));
	gscabm->hf = (xcomplex**)calloc(gscabm->nmic, sizeof(xcomplex*));
	for (int i_mic = 0; i_mic < gscabm->nmic; i_mic++)
	{
		gscabm->hf[i_mic] = (xcomplex*)calloc(gscabm->fftsize / 2 + 1, sizeof(xcomplex));
	}

	gscabm->m_upper_bound = (float*)calloc(gscabm->fftsize / 2, sizeof(float));
	gscabm->m_lower_bound = (float*)calloc(gscabm->fftsize / 2, sizeof(float));

    float deltax = 0.001f;
	for (int i = 0; i < gscabm->fftsize / 2; i++)
	{
		gscabm->m_upper_bound[i] = deltax;
		gscabm->m_lower_bound[i] = -deltax;
	}
	gscabm->m_upper_bound[gscabm->fftsize / 4] = 1.3f;
	if (gscabm->nmic > 2)
	{
		gscabm->m_upper_bound[gscabm->fftsize / 4 + 1] = 0.6f; 
        gscabm->m_upper_bound[gscabm->fftsize / 4 - 1] = 0.6f;
		gscabm->m_upper_bound[gscabm->fftsize / 4 + 2] = 0.15f;
        gscabm->m_upper_bound[gscabm->fftsize / 4 - 2] = 0.15f;
	} 
	else if (gscabm->nmic == 2)
	{
		gscabm->m_upper_bound[gscabm->fftsize / 4] = 1.1f;
		gscabm->m_upper_bound[gscabm->fftsize / 4 + 1] = 0.7f;
        gscabm->m_upper_bound[gscabm->fftsize / 4 - 1] = 0.7f;
		gscabm->m_upper_bound[gscabm->fftsize / 4 + 2] = 0.3f;
        gscabm->m_upper_bound[gscabm->fftsize / 4 - 2] = 0.3f;
		gscabm->m_upper_bound[gscabm->fftsize / 4 + 3] = 0.1f;
        gscabm->m_upper_bound[gscabm->fftsize / 4 - 3] = 0.1f;
	}
	gscabm->abm_FFT =dios_ssp_share_rfft_init(gscabm->fftsize);
	gscabm->fft_out = (float*)calloc(gscabm->fftsize, sizeof(float));
	gscabm->fft_in = (float*)calloc(gscabm->fftsize, sizeof(float));

	/* initialize ABM with coefficients for desired signal from steering 
     * direction and acoustic free-field condition */
	dios_ssp_gsc_gscabm_initabmfreefield(gscabm);
	
}

int dios_ssp_gsc_gscabm_reset(objFGSCabm *gscabm)
{
	/* count variable for filling up abm input signal buffers */
	gscabm->count_sigsegments = 0;

	for (int m = 0; m < gscabm->nmic; m++)
	{
		memset(gscabm->Xdline[m], 0, sizeof(float) * gscabm->fftsize);
	}
	memset(gscabm->xrefdline, 0, sizeof(float) * (gscabm->fftsize / 2 + gscabm->syncdly));

	for (int n = 0; n < gscabm->fftsize / 2 + 1; n++)
	{
		/* init adaptive filter input in the frequency domain */
		gscabm->xfref[n].i = 0.0f;
		gscabm->xfref[n].r = 0.0f;
		/* adaptive filter output in frequency domain */
		gscabm->yf[n].i = 0.0f;
		gscabm->yf[n].r = 0.0f;
		/* frequency-domain error signal */
		gscabm->ef[n].i = 0.0f;
		gscabm->ef[n].r = 0.0f;
		/* normalized stepsize in frequency domain */
		gscabm->muf[n].i = 0.0f;
		gscabm->muf[n].r = 0.0f;
		/* frequency-domain forgetting factor for adaptive filter coefficients */
		gscabm->nuf[n].i = 0.0f;
		gscabm->nuf[n].r = 0.0f;
		/* temporary signal buffer in frequency domain */
		gscabm->yftmp[n].i = 0.0f;
		gscabm->yftmp[n].r = 0.0f;
	}
	/* temporary signal buffer in time domain */
	memset(gscabm->ytmp, 0, sizeof(float) * gscabm->fftsize);
	/* time-domain error signal */
	memset(gscabm->e, 0, sizeof(float) * gscabm->fftsize);
	/* instantaneous power estimate of adaptive filter input */
	memset(gscabm->pxfref, 0, sizeof(float) * (gscabm->fftsize / 2 + 1));
	/* time-domain error signal for abm output */
	for (int m = 0; m < gscabm->nmic; m++)
	{
		memset(gscabm->E[m], 0, sizeof(float) * gscabm->fftsize / (2 * gscabm->fftoverlap));
	}

	/* power estimate after recursive filtering */
	for (int m = 0; m < gscabm->nmic; m++)
	{
		memset(gscabm->sf[m], 0, sizeof(float) * (gscabm->fftsize / 2 + 1));
	}
	memset(gscabm->pftmp, 0, sizeof(float) * (gscabm->fftsize / 2 + 1));
	/* adaptive filters in frequency domain */
	for (int m = 0; m < gscabm->nmic; m++)
	{
		for (int n = 0; n < gscabm->fftsize / 2 + 1; n++)
		{
			gscabm->hf[m][n].i = 0.0f;
			gscabm->hf[m][n].r = 0.0f;
		}
	}

	for (int i = 0; i < gscabm->fftsize; i++)
	{
		gscabm->fft_out[i] = 0.0;
		gscabm->fft_in[i] = 0.0;
	}

	float deltax = 0.001f;
	for (int i = 0; i < gscabm->fftsize / 2; i++)
	{
		gscabm->m_upper_bound[i] = deltax;
		gscabm->m_lower_bound[i] = -deltax;
	}
	gscabm->m_upper_bound[gscabm->fftsize / 4] = 1.3f;
	if (gscabm->nmic > 2)
	{
		gscabm->m_upper_bound[gscabm->fftsize / 4 + 1] = 0.6f;
        gscabm->m_upper_bound[gscabm->fftsize / 4 - 1] = 0.6f;
		gscabm->m_upper_bound[gscabm->fftsize / 4 + 2] = 0.15f;
       gscabm-> m_upper_bound[gscabm->fftsize / 4 - 2] = 0.15f;
	} 
	else if (gscabm->nmic == 2)
	{
		gscabm->m_upper_bound[gscabm->fftsize / 4] = 1.1f;
		gscabm->m_upper_bound[gscabm->fftsize / 4 + 1] = 0.7f;
        gscabm->m_upper_bound[gscabm->fftsize / 4 - 1] = 0.7f;
		gscabm->m_upper_bound[gscabm->fftsize / 4 + 2] = 0.3f;
        gscabm->m_upper_bound[gscabm->fftsize / 4 - 2] = 0.3f;
		gscabm->m_upper_bound[gscabm->fftsize / 4 + 3] = 0.1f;
        gscabm->m_upper_bound[gscabm->fftsize / 4 - 3] = 0.1f;
	}

	dios_ssp_gsc_gscabm_initabmfreefield(gscabm);

	return 0;
}

void dios_ssp_gsc_gscabm_initabmfreefield(objFGSCabm *gscabm)
{
	memset(gscabm->ytmp, 0, gscabm->fftsize * sizeof(float));
	gscabm->ytmp[gscabm->syncdly] = 1;
	dios_ssp_share_rfft_process(gscabm->abm_FFT, gscabm->ytmp, gscabm->fft_out);
    for (int i = 0; i < gscabm->fftsize / 2 + 1; i++)
    {
	    gscabm->hf[0][i].r = gscabm->fft_out[i];
    }
    gscabm->hf[0][0].i = gscabm->hf[0][gscabm->fftsize / 2].i = 0.0;
    for (int i = 1; i < gscabm->fftsize / 2; i++)
    {
	    gscabm->hf[0][i].i = -gscabm->fft_out[gscabm->fftsize - i];
    }
	
	for (int k = 1; k < gscabm->nmic; k++)
    {
		memcpy(gscabm->hf[k], gscabm->hf[0], (gscabm->fftsize / 2 + 1) * sizeof(xcomplex));
    }
}

int dios_ssp_gsc_gscabm_process(objFGSCabm *gscabm, float **X, float *xref, float **Y, float *ctrl_abm, float *ctrl_aic, int index)
{
	int i;
	/* buffer input signal segments, input signal segments are shorter than 
     * or equal to the processing data blocks
     * adaptive filter input signal = [old | new] */
	delayline(xref, gscabm->xrefdline, gscabm->fftsize / 2 + gscabm->syncdly - gscabm->fftsize / (2 * gscabm->sigsoverlap), gscabm->fftsize / 2 + gscabm->syncdly);
	for (int i = 0; i < gscabm->nmic; i++)
    {
		delayline(&X[i][index], gscabm->Xdline[i], gscabm->fftsize - gscabm->fftsize / (2 * gscabm->sigsoverlap), gscabm->fftsize);
    }

	if (gscabm->count_sigsegments == (gscabm->sigsoverlap / gscabm->fftoverlap - 1))  /* 4 / 2 - 1 */
	{
		/* process when input signal buffers are filled */
		dios_ssp_gsc_gscabm_processonedatablock(gscabm, ctrl_abm, ctrl_aic);
		gscabm->count_sigsegments = 0;
	} 
	else
	{
		/* fill input signal block until enough blocks are available */
		gscabm->count_sigsegments++;
	}

	/* write processed data to the abm output */
	for (i = 0; i < gscabm->nmic; i++)
    {
		memcpy(Y[i], &gscabm->E[i][gscabm->count_sigsegments * gscabm->fftsize / (2 * gscabm->sigsoverlap)], (gscabm->fftsize / (2 * gscabm->sigsoverlap)) * sizeof(float));
    }

	return 0;
}

int dios_ssp_gsc_gscabm_processonedatablock(objFGSCabm *gscabm, float *ctrl_abm, float *ctrl_aic)
{
	int i;
	for (int ch = 0; ch < gscabm->nmic; ch++) 
	{
		dios_ssp_share_rfft_process(gscabm->abm_FFT, gscabm->Xdline[ch], gscabm->fft_out);
		for (i = 0; i < gscabm->fftsize / 2 + 1; i++)
		{
			gscabm->xfref[i].r = gscabm->fft_out[i];
		}
		gscabm->xfref[0].i = gscabm->xfref[gscabm->fftsize / 2].i = 0.0;
		for (i = 1; i < gscabm->fftsize / 2; i++)
		{
			gscabm->xfref[i].i = -gscabm->fft_out[gscabm->fftsize - i];
		}
		
		for (i = 0; i < gscabm->fftsize / 2 + 1; i++)
        {
			gscabm->pxfref[i] = gscabm->xfref[i].r * gscabm->xfref[i].r + gscabm->xfref[i].i * gscabm->xfref[i].i;		
			gscabm->sf[ch][i] = gscabm->lambda * gscabm->sf[ch][i] + (1.f - gscabm->lambda) * gscabm->pxfref[i];

			/* 1.normalization term of FLMS -> muf */
			if (gscabm->sf[ch][i] < gscabm->delta)
            {
				gscabm->pftmp[i] = 1.0f / gscabm->delta;
            }
			else
            {
				gscabm->pftmp[i] = 1.0f / gscabm->sf[ch][i];
            }

			gscabm->pftmp[i] *= gscabm->mu;

			/* 2.introduction of control signal */
			gscabm->pftmp[i] *= ctrl_abm[i];

			/* 3.conversion from float to xcomplex format */
			gscabm->muf[i].r = gscabm->pftmp[i];
			gscabm->muf[i].i = 0.0;
			/* 4.prevents freezing of adaptive filters */
			gscabm->nuf[i].r = ctrl_aic[i];
			gscabm->nuf[i].i = 0.0;
			gscabm->nuf[i] = complex_mul(gscabm->nuf[i], gscabm->nu);
			/* 5.compute adaptive filter output */
			gscabm->yf[i] = complex_mul(gscabm->xfref[i], gscabm->hf[ch][i]);
        }

		/* ifft of adaptive filter output: y is then constrained to be y = [0 | new] */
		// gscabm->m_pFFT->FFTInv_CToR(gscabm->yf, gscabm->ytmp);
		gscabm->fft_in[0] = gscabm->yf[0].r;
		gscabm->fft_in[gscabm->fftsize / 2] = gscabm->yf[gscabm->fftsize / 2].r;
		for(int k = 1; k < gscabm->fftsize / 2; k++)
		{
			gscabm->fft_in[k] = gscabm->yf[k].r;
			gscabm->fft_in[gscabm->fftsize - k] = -gscabm->yf[k].i;
		}
		dios_ssp_share_irfft_process(gscabm->abm_FFT, gscabm->fft_in, gscabm->fft_out);
		for(int k = 0; k < gscabm->fftsize; k++)
		{
			gscabm->ytmp[k] = gscabm->fft_out[k] / gscabm->fftsize;
		}

		/* compute error signal in time-domain with circular convolution constraint e = [0 | new] */
		for (i = 0; i < gscabm->fftsize / 2; i++)
        {
			gscabm->e[gscabm->fftsize / 2 + i] = gscabm->xrefdline[i] - gscabm->ytmp[gscabm->fftsize / 2 + i];
        }

		/* last block of error signal -> abm output signal in time domain */
		memcpy(gscabm->E[ch], &(gscabm->e[gscabm->fftsize - gscabm->fftsize / (2 * gscabm->fftoverlap)]), (gscabm->fftsize / (2 * gscabm->fftoverlap)) * sizeof(float));

		dios_ssp_share_rfft_process(gscabm->abm_FFT, gscabm->e, gscabm->fft_out);
		for (i = 0; i < gscabm->fftsize / 2 + 1; i++)
		{
			gscabm->ef[i].r = gscabm->fft_out[i];
		}
		gscabm->ef[0].i = gscabm->ef[gscabm->fftsize / 2].i = 0.0;
		for (i = 1; i < gscabm->fftsize / 2; i++)
		{
			gscabm->ef[i].i = -gscabm->fft_out[gscabm->fftsize - i];
		}
        /* update of adaptive filter */
        for (i = 0; i < gscabm->fftsize / 2 + 1; i++)
        {
			/* 1.conjugate of reference signal */
            gscabm->yftmp[i] = complex_conjg(gscabm->xfref[i]);

			/* 2.enovation term */
			gscabm->yftmp[i] = complex_mul(gscabm->yftmp[i], gscabm->ef[i]);

			/* 3.stepsize term */
			gscabm->yftmp[i] = complex_mul(gscabm->yftmp[i], gscabm->muf[i]);

			/* 4.update */
			gscabm->hf[ch][i] = complex_add(gscabm->hf[ch][i], gscabm->yftmp[i]);
        }
        /* against freezing of the adaptive filter coefficients */
        for (i = 0; i < gscabm->fftsize / 2 + 1; i++)
        {
            gscabm->hf[ch][i] = complex_sub(gscabm->hf[ch][i], complex_mul(gscabm->hf[ch][i], gscabm->nuf[i]));
        }

		/* circular correlation constraint (hf = [new | 0]) -> hf */
		// gscabm->m_pFFT->FFTInv_CToR(gscabm->hf[ch], gscabm->ytmp);
		gscabm->fft_in[0] = gscabm->hf[ch][0].r;
		gscabm->fft_in[gscabm->fftsize / 2] = gscabm->hf[ch][gscabm->fftsize / 2].r;
		for(int k = 1; k < gscabm->fftsize / 2; k++)
		{
			gscabm->fft_in[k] = gscabm->hf[ch][k].r;
			gscabm->fft_in[gscabm->fftsize - k] = -gscabm->hf[ch][k].i;
		}
		dios_ssp_share_irfft_process(gscabm->abm_FFT, gscabm->fft_in, gscabm->fft_out);
		for(int k = 0; k < gscabm->fftsize; k++)
		{
			gscabm->ytmp[k] = gscabm->fft_out[k] / gscabm->fftsize;
		}

		memset(&(gscabm->ytmp[gscabm->fftsize / 2]), 0, (gscabm->fftsize / 2) * sizeof(float));

		int limit_ind = gscabm->fftsize / 4 - 3;
		for (i = limit_ind; i > 0 ; i--)
		{
			gscabm->ytmp[i] = gscabm->ytmp[i] < gscabm->m_upper_bound[i]?gscabm->ytmp[i] : gscabm->m_upper_bound[i];
			gscabm->ytmp[i] = gscabm->ytmp[i] > gscabm->m_lower_bound[i] ? gscabm->ytmp[i] : gscabm->m_lower_bound[i];
			gscabm->ytmp[gscabm->fftsize / 2 - i] = 
			gscabm->ytmp[gscabm->fftsize / 2 - i] < gscabm->m_upper_bound[gscabm->fftsize / 2 - i]?gscabm->ytmp[gscabm->fftsize / 2 - i] : gscabm->m_upper_bound[gscabm->fftsize / 2 - i];
			gscabm->ytmp[gscabm->fftsize / 2 - i] = 
			gscabm->ytmp[gscabm->fftsize / 2 - i] > gscabm->m_lower_bound[gscabm->fftsize / 2 - i]?gscabm->ytmp[gscabm->fftsize / 2 - i] : gscabm->m_lower_bound[gscabm->fftsize / 2 - i];
		}
		gscabm->ytmp[0] = gscabm->ytmp[0] < gscabm->m_upper_bound[0]?gscabm->ytmp[0] : gscabm->m_upper_bound[0];
		gscabm->ytmp[0] = gscabm->ytmp[0] > gscabm->m_lower_bound[0]?gscabm->ytmp[0] : gscabm->m_lower_bound[0];

		dios_ssp_share_rfft_process(gscabm->abm_FFT, gscabm->ytmp, gscabm->fft_out);
		for (i = 0; i < gscabm->fftsize / 2 + 1; i++)
		{
			gscabm->hf[ch][i].r = gscabm->fft_out[i];
		}
		gscabm->hf[ch][0].i = gscabm->hf[ch][gscabm->fftsize / 2].i = 0.0;
		for (i = 1; i < gscabm->fftsize / 2; i++)
		{
			gscabm->hf[ch][i].i = -gscabm->fft_out[gscabm->fftsize - i];
		}
	}

	return 0;
}

int dios_ssp_gsc_gscabm_delete(objFGSCabm *gscabm)
{
	int ret = 0;
	for (int i_mic = 0; i_mic < gscabm->nmic; i_mic++)
	{
		free(gscabm->Xdline[i_mic]);
	}
	free(gscabm->Xdline);
	free(gscabm->xrefdline);
	free(gscabm->xfref);
	free(gscabm->ytmp);
	free(gscabm->yf);
	free(gscabm->e);
	for (int i_mic = 0; i_mic < gscabm->nmic; i_mic++)
	{
		free(gscabm->E[i_mic]);
	}
	free(gscabm->E);
	free(gscabm->ef);
	free(gscabm->muf);
	free(gscabm->nuf);
	free(gscabm->yftmp);
	free(gscabm->pxfref);
	for (int i_mic = 0; i_mic < gscabm->nmic; i_mic++)
	{
		free(gscabm->sf[i_mic]);
	}
	free(gscabm->sf);
	free(gscabm->pftmp);
	for (int i_mic = 0; i_mic < gscabm->nmic; i_mic++)
	{
		free(gscabm->hf[i_mic]);
	}
	free(gscabm->hf);
	free(gscabm->m_upper_bound);
	free(gscabm->m_lower_bound);
	free(gscabm->fft_out);
	free(gscabm->fft_in);
	ret = dios_ssp_share_rfft_uninit(gscabm->abm_FFT);
	if (0 != ret)
	{
		gscabm->abm_FFT = NULL;
	}

	return 0;
}
