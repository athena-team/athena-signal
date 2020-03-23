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

Description: The adaptive interference canceller adaptively subtracts all
signal components from fixed reference path (fixed beamformer output) which
are not suppressed by the adaptive blocking matrix. 
The adaptive interference canceller is realized by adaptive filters between
each of the adaptive blocking matrix outputs and the reference path.    
The adaptive interference caneller is only updated when the SNR is low.  
==============================================================================*/

#include "dios_ssp_gsc_aic.h"

void dios_ssp_gsc_gscaic_init(objFGSCaic *gscaic, int dlysync, int num_mic, int fft_size, float maxNorm, 
            float forgetfactor, float stepsize, float thresConDiv0, 
            float thresDynDiv0, float lobeDynDiv0, int useDynRegularization, 
            int num_taps, int overlap_fft, int overlap_sigs, long rate, 
            float tconst_freezing)
{
	gscaic->xrefdline = NULL;
	gscaic->Xfdline = NULL;
	gscaic->Xdline = NULL;
	gscaic->Xfbdline = NULL;
	gscaic->Xffilt = NULL;
	gscaic->yftmp = NULL;
	gscaic->ytmp = NULL;
	gscaic->yhf = NULL;
	gscaic->Hf = NULL;
	gscaic->e = NULL;
	gscaic->z = NULL;
	gscaic->ef = NULL;
	gscaic->sftmp = NULL;
	gscaic->sf = NULL;
	gscaic->muf = NULL;
	gscaic->nuf = NULL;

	gscaic->nmic = num_mic;
	gscaic->fftsize = fft_size;
	gscaic->fftoverlap = overlap_fft;
	gscaic->sigsoverlap = overlap_sigs;
	gscaic->maxnorm = maxNorm;
	gscaic->lambda = forgetfactor * powf(1.f - 1.f / (3.f * (float)gscaic->fftsize), (float)(num_taps / gscaic->fftoverlap));
	gscaic->mu = 2 * stepsize * (1 - gscaic->lambda);
	gscaic->delta_con = thresConDiv0;
	gscaic->delta_dyn = thresDynDiv0;
	gscaic->s0_dyn = lobeDynDiv0;
	gscaic->regularize_dyn = useDynRegularization;
	gscaic->ntaps = num_taps;
	gscaic->bdlinesize = 2;
	gscaic->pbdlinesize = 2 * gscaic->ntaps / gscaic->fftsize;
	gscaic->syncdly = dlysync; 
	gscaic->nu.r = 1.f - expf(-gscaic->fftsize / (2 * gscaic->fftoverlap * tconst_freezing * rate));
	gscaic->nu.i = 0.f;
	gscaic->count_sigsegments = 0;

	gscaic->xrefdline = (float*)calloc(gscaic->fftsize / 2 + gscaic->syncdly, sizeof(float));
	gscaic->Xfdline = (xcomplex***)calloc(gscaic->nmic, sizeof(xcomplex**));
	for(int i = 0; i < gscaic->nmic; i++)
	{
		gscaic->Xfdline[i] = (xcomplex**)calloc(gscaic->bdlinesize, sizeof(xcomplex*));
		for(int k = 0; k < gscaic->bdlinesize; k++)
		{
			gscaic->Xfdline[i][k] = (xcomplex*)calloc(gscaic->fftsize / 2 + 1, sizeof(xcomplex));
		}
		
	}
	gscaic->Xdline = (float**)calloc(gscaic->nmic, sizeof(float*));
	for (int i_mic = 0; i_mic < gscaic->nmic; i_mic++)
	{
		gscaic->Xdline[i_mic] = (float*)calloc(gscaic->fftsize, sizeof(float));
	}
	gscaic->Xfbdline = (xcomplex***)calloc(gscaic->nmic, sizeof(xcomplex**));
	for(int i = 0; i < gscaic->nmic; i++)
	{
		gscaic->Xfbdline[i] = (xcomplex**)calloc(gscaic->pbdlinesize, sizeof(xcomplex*));
		for(int k = 0; k < gscaic->pbdlinesize; k++)
		{
			gscaic->Xfbdline[i][k] = (xcomplex*)calloc(gscaic->fftsize / 2 + 1, sizeof(xcomplex));
		}
		
	}
	gscaic->Xffilt = (xcomplex**)calloc(gscaic->nmic, sizeof(xcomplex*));
	for (int i_mic = 0; i_mic < gscaic->nmic; i_mic++)
	{
		gscaic->Xffilt[i_mic] = (xcomplex*)calloc(gscaic->fftsize / 2 + 1, sizeof(xcomplex));
	}
	gscaic->yftmp = (xcomplex*)calloc(gscaic->fftsize / 2 + 1, sizeof(xcomplex));
	gscaic->ytmp = (float*)calloc(gscaic->fftsize, sizeof(float));
	gscaic->yhf = (xcomplex*)calloc(gscaic->fftsize / 2 + 1, sizeof(xcomplex));
	gscaic->Hf = (xcomplex***)calloc(gscaic->nmic, sizeof(xcomplex**));
	for(int i = 0; i < gscaic->nmic; i++)
	{
		gscaic->Hf[i] = (xcomplex**)calloc(gscaic->pbdlinesize, sizeof(xcomplex*));
		for(int k = 0; k < gscaic->pbdlinesize; k++)
		{
			gscaic->Hf[i][k] = (xcomplex*)calloc(gscaic->fftsize / 2 + 1, sizeof(xcomplex));
		}
		
	}
	gscaic->e = (float*)calloc(gscaic->fftsize, sizeof(float));
	gscaic->z = (float*)calloc(gscaic->fftsize / (2 * gscaic->fftoverlap), sizeof(float));
	gscaic->ef = (xcomplex*)calloc(gscaic->fftsize / 2 + 1, sizeof(xcomplex));
	gscaic->pXf = (float*)calloc(gscaic->fftsize / 2 + 1, sizeof(float));
	gscaic->sftmp = (float*)calloc(gscaic->fftsize / 2 + 1, sizeof(float));
	gscaic->sf = (float*)calloc(gscaic->fftsize / 2 + 1, sizeof(float));
	gscaic->muf = (xcomplex*)calloc(gscaic->fftsize / 2 + 1, sizeof(xcomplex));
	gscaic->nuf = (xcomplex*)calloc(gscaic->fftsize / 2 + 1, sizeof(xcomplex));

	gscaic->aic_FFT = dios_ssp_share_rfft_init(gscaic->fftsize);
	gscaic->fft_out = (float*)calloc(gscaic->fftsize, sizeof(float));
	gscaic->fft_in = (float*)calloc(gscaic->fftsize, sizeof(float));
}

int dios_ssp_gsc_gscaic_reset(objFGSCaic *gscaic)
{
	gscaic->count_sigsegments = 0;

	memset(gscaic->xrefdline, 0, sizeof(float) * (gscaic->fftsize / 2 + gscaic->syncdly));
	for (int m = 0; m < gscaic->nmic; m++)
	{
		memset(gscaic->Xdline[m], 0, sizeof(float) * gscaic->fftsize);
		for (int ll = 0; ll < gscaic->bdlinesize; ll++)
		{
			for (int n = 0; n < gscaic->fftsize / 2 + 1; n++)
			{
				gscaic->Xfdline[m][ll][n].i = 0.0f;
				gscaic->Xfdline[m][ll][n].r = 0.0f;
			}
		}
		for (int ll = 0; ll < gscaic->pbdlinesize; ll++)
		{
			for (int n = 0; n < gscaic->fftsize / 2 + 1; n++)
			{
				gscaic->Xfbdline[m][ll][n].i = 0.0f;
				gscaic->Xfbdline[m][ll][n].r = 0.0f;
				gscaic->Hf[m][ll][n].i = 0.0f;
				gscaic->Hf[m][ll][n].r = 0.0f;
			}
		}
		for (int n = 0; n < gscaic->fftsize / 2 + 1; n++)
		{
			gscaic->Xffilt[m][n].i = 0.0f;
			gscaic->Xffilt[m][n].r = 0.0f;
		}
	}

	for (int i = 0; i < gscaic->fftsize; i++)
	{
		gscaic->fft_out[i] = 0.0;
		gscaic->fft_in[i] = 0.0;
	}
	
    memset(gscaic->pXf, 0, sizeof(float) * (gscaic->fftsize / 2 + 1));
	memset(gscaic->sftmp, 0, sizeof(float) * (gscaic->fftsize / 2 + 1));
	memset(gscaic->sf, 0, sizeof(float) * (gscaic->fftsize / 2 + 1));
	memset(gscaic->ytmp, 0, sizeof(float) * gscaic->fftsize);
	memset(gscaic->e, 0, sizeof(float) * gscaic->fftsize);
	memset(gscaic->z, 0, sizeof(float) * (gscaic->fftsize / (2 * gscaic->fftoverlap)));

	for (int n = 0; n < gscaic->fftsize / 2 + 1; n++)
	{
		/* error signal in frequency domain */
		gscaic->ef[n].i = 0.0f;
		gscaic->ef[n].r = 0.0f;
		/* temporay vector in frequency domain */
		gscaic->yftmp[n].i = 0.0f;
		gscaic->yftmp[n].r = 0.0f;
		/* adaptive filter output in frequency domain */
		gscaic->yhf[n].i = 0.0f;
		gscaic->yhf[n].r = 0.0f;
		/* normalized stepsize in frequency domain */
		gscaic->muf[n].i = 0.0f;
		gscaic->muf[n].r = 0.0f;
		/* frequency-domain forgetting factor for adaptive filter coefficients */
		gscaic->nuf[n].i = 0.0f;
		gscaic->nuf[n].r = 0.0f;
	}

	return 0;
}

int dios_ssp_gsc_gscaic_resetfilterbank(objFGSCaic *gscaic)
{
	for (int k = 0; k < gscaic->nmic; k++) 
	{
		for (int i = 0; i < gscaic->pbdlinesize; i++)
        {
			memset(gscaic->Hf[k][i], 0, (gscaic->fftsize / 2 + 1) * sizeof(xcomplex));
        }
	}
	return 0;
}

int dios_ssp_gsc_gscaic_process(objFGSCaic *gscaic, float *xref, float **X, float *y, float *ctrl_abm, float *ctrl_aic)
{
	/* buffer input signal segments, input signal segments are shorter than or equal to 
     * the processing data blocks, adaptive filter input signal = [old | new] */
	for (int k = 0; k < gscaic->nmic; k++)
    {
		delayline(X[k], gscaic->Xdline[k], gscaic->fftsize - gscaic->fftsize / (2 * gscaic->sigsoverlap), gscaic->fftsize);
    }

	/* delay time-domain fixed beamformer output, syncdly = 72 samples */
	delayline(xref, gscaic->xrefdline, gscaic->fftsize / 2 + gscaic->syncdly - gscaic->fftsize / (2 * gscaic->sigsoverlap), gscaic->fftsize / 2 + gscaic->syncdly);
	if (gscaic->count_sigsegments == (gscaic->sigsoverlap / gscaic->fftoverlap - 1)) 
	{
		/* process when input signal buffers are filled */
		dios_ssp_gsc_gscaic_processonedatablock(gscaic, ctrl_abm, ctrl_aic);
		gscaic->count_sigsegments = 0;
	} 
	else 
	{
		/* fill input signal block until enough blocks are available */
		gscaic->count_sigsegments++;
	}

	/* write processed data to the aic output */
	memcpy(y, &(gscaic->z[gscaic->count_sigsegments * gscaic->fftsize / (2 * gscaic->sigsoverlap)]), (gscaic->fftsize / (2 * gscaic->sigsoverlap)) * sizeof(float));

	return 0;
}

int dios_ssp_gsc_gscaic_processonedatablock(objFGSCaic *gscaic, float *ctrl_abm, float *ctrl_aic)
{
	int i, j, k;

	/* reset filter output and power estimate to zero */
	memset(gscaic->yhf, 0, (gscaic->fftsize / 2 + 1) * sizeof(xcomplex));
	memset(gscaic->pXf, 0, (gscaic->fftsize / 2 + 1) * sizeof(float));

	for (int k = 0; k < gscaic->nmic; k++)
	{
        /* fft of filter inputs */
		dios_ssp_share_rfft_process(gscaic->aic_FFT, gscaic->Xdline[k], gscaic->fft_out);
		for (int i = 0; i < gscaic->fftsize / 2 + 1; i++)
		{
			gscaic->Xffilt[k][i].r = gscaic->fft_out[i];
		}
		gscaic->Xffilt[k][0].i = gscaic->Xffilt[k][gscaic->fftsize / 2].i = 0.0;
		for (int i = 1; i < gscaic->fftsize / 2; i++)
		{
			gscaic->Xffilt[k][i].i = -gscaic->fft_out[gscaic->fftsize - i];
		}

		/* block delay line for partitioned block adaptive filters
         * pbdlinesize = 1, no delay */
		for(i = 0; i < gscaic->pbdlinesize-1; i++)
		{
			memcpy(gscaic->Xfbdline[k][i+1], gscaic->Xfbdline[k][i], (gscaic->fftsize / 2 + 1) * sizeof(xcomplex));
		}
		memcpy(gscaic->Xfbdline[k][0], gscaic->Xffilt[k], (gscaic->fftsize / 2 + 1) * sizeof(xcomplex));

		for (i = 0; i < 1; i++)
		{
			for (j = 0; j < gscaic->fftsize / 2 + 1; j++)
            {
				/* 1.power estimate of adaptive filter inputs	for later recursion */
				gscaic->sftmp[j] = gscaic->Xfbdline[k][i][j].r * gscaic->Xfbdline[k][i][j].r + gscaic->Xfbdline[k][i][j].i * gscaic->Xfbdline[k][i][j].i;
				/* 2.summing up power estimate */
				gscaic->pXf[j] += gscaic->sftmp[j];
				/* 3.filter with adaptive filters */
				gscaic->yftmp[j] = complex_mul(gscaic->Hf[k][i][j], gscaic->Xfbdline[k][i][j]);
				/* 4.summing up filter outputs */
				gscaic->yhf[j] = complex_add(gscaic->yhf[j], gscaic->yftmp[j]);
            }
		}
	}

	/* ifft of adaptive filter output */
	// gscaic->m_pFFT->FFTInv_CToR(gscaic->yhf, gscaic->ytmp);
	gscaic->fft_in[0] = gscaic->yhf[0].r;
	gscaic->fft_in[gscaic->fftsize / 2] = gscaic->yhf[gscaic->fftsize / 2].r;
	for(int k = 1; k < gscaic->fftsize / 2; k++)
	{
		gscaic->fft_in[k] = gscaic->yhf[k].r;
		gscaic->fft_in[gscaic->fftsize - k] = -gscaic->yhf[k].i;
	}
	dios_ssp_share_irfft_process(gscaic->aic_FFT, gscaic->fft_in, gscaic->fft_out);
	for(int k = 0; k < gscaic->fftsize; k++)
	{
		gscaic->ytmp[k] = gscaic->fft_out[k] / gscaic->fftsize;
	}

	/* compute error signal in time-domain with circular convolution constraint e = [0 | new] */
	for (k = 0; k < gscaic->fftsize / 2; k++)
    {
		gscaic->e[gscaic->fftsize / 2 + k] = gscaic->xrefdline[k] - gscaic->ytmp[gscaic->fftsize / 2 + k];
    }

	/* copy output samples */
	memcpy(gscaic->z, &(gscaic->e[gscaic->fftsize - gscaic->fftsize / (2 * gscaic->fftoverlap)]), (gscaic->fftsize / (2 * gscaic->fftoverlap)) * sizeof(float));

	/* fourier transform of aic error signal */
	dios_ssp_share_rfft_process(gscaic->aic_FFT, gscaic->e, gscaic->fft_out);
	for (int i = 0; i < gscaic->fftsize / 2 + 1; i++)
	{
		gscaic->ef[i].r = gscaic->fft_out[i];
	}
	gscaic->ef[0].i = gscaic->ef[gscaic->fftsize / 2].i = 0.0;
	for (int i = 1; i < gscaic->fftsize / 2; i++)
	{
		gscaic->ef[i].i = -gscaic->fft_out[gscaic->fftsize - i];
	}

	/* adaptation
     * recursive power estimate of adaptive filter input all the computations
     * are done with real signals, for later usage, they are transformed to the 
     * complex xcomplex format instantaneous energy estimation */
	for (int i = 0; i < gscaic->fftsize / 2 + 1; i++)
	{
		gscaic->sf[i] = gscaic->lambda * gscaic->sf[i] + (1.f - gscaic->lambda) * gscaic->pXf[i];
	}
	
    /* normalization term of FLMS -> muf */
	/* regularization: dynamical or constant */
	if (gscaic->regularize_dyn == 1)
	{
		for(i = 0; i < gscaic->fftsize / 2 + 1; i++)
		{
			gscaic->sftmp[i] = gscaic->sf[i] + gscaic->delta_dyn * (float)exp(-gscaic->sf[i] / gscaic->s0_dyn);
		}

		for (k = 0; k < gscaic->fftsize / 2 + 1; k++)
		{
			if (gscaic->sftmp[k] < 10e-6f)
            {
				gscaic->sftmp[k] = 1.f / (10e-6f);
            }
			else
            {
				gscaic->sftmp[k] = 1.f / gscaic->sftmp[k];
            }
		}
	} 
	else 
	{
		for (k = 0; k < gscaic->fftsize / 2 + 1; k++)
		{
			if (gscaic->sf[k] < gscaic->delta_con)
            {
				gscaic->sftmp[k] = 1.f / gscaic->delta_con;
            }
			else
            {
				gscaic->sftmp[k] = 1.f / gscaic->sf[k];
            }
		}
	}

	
	for (k = 0; k < gscaic->fftsize / 2 + 1; k++)
    {
		/* 1.introduction of stepsize */
		gscaic->sftmp[k] *= (gscaic->mu);

		/* 2.introduction of control signal */
		gscaic->sftmp[k] *= ctrl_aic[k];

		/* 3.conversion from float to xcomplex format */
		gscaic->muf[k].r = gscaic->sftmp[k];
		gscaic->muf[k].i = 0.f;
		/* 4.prevents freezing of adaptive filters */
		gscaic->nuf[k].r = ctrl_abm[k];
		gscaic->nuf[k].i = 0.f;
		gscaic->nuf[k] = complex_mul(gscaic->nuf[k], gscaic->nu);
    }

	float norm = 0.f;

	for (k = 0; k < gscaic->nmic; k++)
	{
        for (i = 0; i < 1; i++)
		{
            
			for (j = 0; j < gscaic->fftsize / 2 + 1; j++)
            {
				/* 1.conjugate of reference signal */
				gscaic->yftmp[j] = complex_conjg(gscaic->Xfbdline[k][i][j]);

				/* 2.enovation term */
				gscaic->yftmp[j] = complex_mul(gscaic->yftmp[j], gscaic->ef[j]);

				/* 3.stepsize term */
				gscaic->yftmp[j] = complex_mul(gscaic->yftmp[j], gscaic->muf[j]);

				/* 4.update */
				gscaic->Hf[k][i][j] = complex_add(gscaic->Hf[k][i][j], gscaic->yftmp[j]);

				/* 5.norm constraint */
				norm += gscaic->Hf[k][i][j].r * gscaic->Hf[k][i][j].r + gscaic->Hf[k][i][j].i * gscaic->Hf[k][i][j].i;
            }
		}
	}

	/* norm constraint */
	norm /= (gscaic->fftsize * gscaic->fftsize);
	if (norm > gscaic->maxnorm)
    {
		norm = sqrtf(gscaic->maxnorm / norm);
    }
	else
    {
		norm = 1.f;
    }

	for (k = 0; k < gscaic->nmic; k++)
	{
		for (int i = 0; i < 1; i++)
		{
			/* against freezing of the adaptive filter coefficients */
			for (int j = 0; j < gscaic->fftsize / 2 + 1; j++)
            {
				gscaic->Hf[k][i][j] = complex_sub(gscaic->Hf[k][i][j], complex_mul(gscaic->Hf[k][i][j], gscaic->nuf[j]));
            }

			/* circular correlation constraint (Hf[k][i] = [new | 0]) -> Hf[k][i] */
			// gscaic->m_pFFT->FFTInv_CToR(gscaic->Hf[k][i], gscaic->ytmp);
			gscaic->fft_in[0] = gscaic->Hf[k][i][0].r;
			gscaic->fft_in[gscaic->fftsize / 2] = gscaic->Hf[k][i][gscaic->fftsize / 2].r;
			for(int j = 1; j < gscaic->fftsize / 2; j++)
			{
				gscaic->fft_in[j] = gscaic->Hf[k][i][j].r;
				gscaic->fft_in[gscaic->fftsize - j] = -gscaic->Hf[k][i][j].i;
			}
			dios_ssp_share_irfft_process(gscaic->aic_FFT, gscaic->fft_in, gscaic->fft_out);
			for(int j = 0; j < gscaic->fftsize; j++)
			{
				gscaic->ytmp[j] = gscaic->fft_out[j] / gscaic->fftsize;
			}

			memset(&gscaic->ytmp[gscaic->fftsize / 2], 0, (gscaic->fftsize / 2) * sizeof(float));

            /* norm constraint */
			for (int l = 0; l < gscaic->fftsize / 2; l++)
            {
				gscaic->ytmp[l] *= norm;
            }

			dios_ssp_share_rfft_process(gscaic->aic_FFT, gscaic->ytmp, gscaic->fft_out);
			for (int j = 0; j < gscaic->fftsize / 2 + 1; j++)
			{
				gscaic->Hf[k][i][j].r = gscaic->fft_out[j];
			}
			gscaic->Hf[k][i][0].i = gscaic->Hf[k][i][gscaic->fftsize / 2].i = 0.0;
			for (int j = 1; j < gscaic->fftsize / 2; j++)
			{
				gscaic->Hf[k][i][j].i = -gscaic->fft_out[gscaic->fftsize - j];
			}
		}
	}

	return 0;
}

int dios_ssp_gsc_gscaic_delete(objFGSCaic *gscaic)
{
	int i, k;
	int ret = 0;
	free(gscaic->xrefdline);
	for(i = 0; i < gscaic->nmic; i++)
	{
		for(k = 0; k < gscaic->bdlinesize; k++)
		{
			free(gscaic->Xfdline[i][k]);
		}
		free(gscaic->Xfdline[i]);
	}
	free(gscaic->Xfdline);
	for (int i_mic = 0; i_mic < gscaic->nmic; i_mic++)
	{
		free(gscaic->Xdline[i_mic]);
	}
	free(gscaic->Xdline);
	for(i = 0; i < gscaic->nmic; i++)
	{
		for(k = 0; k < gscaic->pbdlinesize; k++)
		{
			free(gscaic->Xfbdline[i][k]);
		}
		free(gscaic->Xfbdline[i]);
	}
	free(gscaic->Xfbdline);
	for (int i_mic = 0; i_mic < gscaic->nmic; i_mic++)
	{
		free(gscaic->Xffilt[i_mic]);
	}
	free(gscaic->Xffilt);
	free(gscaic->yftmp);
	free(gscaic->ytmp);
	free(gscaic->yhf);
	for(i = 0; i < gscaic->nmic; i++)
	{
		for(k = 0; k < gscaic->pbdlinesize; k++)
		{
			free(gscaic->Hf[i][k]);
		}
		free(gscaic->Hf[i]);
	}
	free(gscaic->Hf);
	free(gscaic->e);
	free(gscaic->z);
	free(gscaic->ef);
	free(gscaic->pXf);
	free(gscaic->sftmp);
	free(gscaic->sf);
	free(gscaic->muf);
	free(gscaic->nuf);

	free(gscaic->fft_out);
	free(gscaic->fft_in);
	ret = dios_ssp_share_rfft_uninit(gscaic->aic_FFT);
	if (0 != ret)
	{
		gscaic->aic_FFT = NULL;
	}

	return 0;
}
