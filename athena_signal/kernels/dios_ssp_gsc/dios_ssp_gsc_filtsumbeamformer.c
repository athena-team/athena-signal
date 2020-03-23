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

Description: The FBF which is usually a delay&sum beamformer enhances desired
signal components, and is used as reference for the adaptation of the 
adaptivesidelobe cancelling path. 
The fractional time-delays that are required in the discretized time domainfor
the steering into the assumed target direction-of-arrival(DOA) are
usuallrealized by short fractional delay filters.
==============================================================================*/

#include "dios_ssp_gsc_filtsumbeamformer.h"

void dios_ssp_gsc_gscfiltsumbeamformer_init(objFGSCfiltsumbeamformer* gscfiltsumbeamformer, int num_mic, int fft_size, int fft_overlap)
{
	/* reset all pointers to NULL */
	gscfiltsumbeamformer->Xdline = NULL;
	gscfiltsumbeamformer->xftmp = NULL;
	gscfiltsumbeamformer->ytmp = NULL;
	gscfiltsumbeamformer->yftmp = NULL;
	// gscfiltsumbeamformer->m_pFFT = NULL;

	gscfiltsumbeamformer->nmic = num_mic;
	gscfiltsumbeamformer->fftlength = fft_size;
	gscfiltsumbeamformer->fftoverlap = fft_overlap;
	gscfiltsumbeamformer->filtord = 0;  /* 0 for delay-and-sum beamformer */
	
	/* initialize delayline for fbf inputs in time domain [old|new] */
	gscfiltsumbeamformer->Xdline = (float**)calloc(gscfiltsumbeamformer->nmic, sizeof(float*));
	for (int i_mic = 0; i_mic < gscfiltsumbeamformer->nmic; i_mic++)
	{
		gscfiltsumbeamformer->Xdline[i_mic] = (float*)calloc(gscfiltsumbeamformer->fftlength, sizeof(float));
	}
	gscfiltsumbeamformer->xftmp = (xcomplex*)calloc(gscfiltsumbeamformer->fftlength / 2 + 1, sizeof(xcomplex));
	gscfiltsumbeamformer->ytmp = (float*)calloc(gscfiltsumbeamformer->fftlength, sizeof(float));
	gscfiltsumbeamformer->yftmp = (xcomplex*)calloc(gscfiltsumbeamformer->fftlength / 2 + 1, sizeof(xcomplex));
	
	gscfiltsumbeamformer->filt_FFT = dios_ssp_share_rfft_init(gscfiltsumbeamformer->fftlength);
	gscfiltsumbeamformer->fft_out = (float*)calloc(gscfiltsumbeamformer->fftlength, sizeof(float));
	gscfiltsumbeamformer->fft_in = (float*)calloc(gscfiltsumbeamformer->fftlength, sizeof(float));
}

int dios_ssp_gsc_gscfiltsumbeamformer_reset(objFGSCfiltsumbeamformer* gscfiltsumbeamformer)
{
	for (int m = 0; m < gscfiltsumbeamformer->nmic; m++)
	{
		memset(gscfiltsumbeamformer->Xdline[m], 0, sizeof(float) * gscfiltsumbeamformer->fftlength);
		for (int n = 0; n < gscfiltsumbeamformer->fftlength / 2 + 1; n++)
		{
			gscfiltsumbeamformer->xftmp[n].i = 0.0f;
			gscfiltsumbeamformer->xftmp[n].r = 0.0f;
			gscfiltsumbeamformer->yftmp[n].i = 0.0f;
			gscfiltsumbeamformer->yftmp[n].r = 0.0f;
		}
	}

	for(int i = 0; i < gscfiltsumbeamformer->fftlength; i++)
	{
		gscfiltsumbeamformer->fft_out[i] = 0.0;
		gscfiltsumbeamformer->fft_in[i] = 0.0;
	}

	memset(gscfiltsumbeamformer->ytmp, 0, sizeof(float) * gscfiltsumbeamformer->fftlength);

	return 0;
}

int dios_ssp_gsc_gscfiltsumbeamformer_process(objFGSCfiltsumbeamformer* gscfiltsumbeamformer, float **X, float *y, int index)
{
	int i;
	int ind_newblock = gscfiltsumbeamformer->fftlength - gscfiltsumbeamformer->fftlength / (2 * gscfiltsumbeamformer->fftoverlap);
	
	memset(gscfiltsumbeamformer->yftmp, 0, (gscfiltsumbeamformer->fftlength / 2 + 1) * sizeof(xcomplex));
	
	float vol = 1.0f / (float)gscfiltsumbeamformer->nmic;
	
	for (int k = 0; k < gscfiltsumbeamformer->nmic; k++) 
	{
		/* move new samples into the buffers */
		delayline(&X[k][index], gscfiltsumbeamformer->Xdline[k], ind_newblock, gscfiltsumbeamformer->fftlength);

		dios_ssp_share_rfft_process(gscfiltsumbeamformer->filt_FFT, gscfiltsumbeamformer->Xdline[k], gscfiltsumbeamformer->fft_out);
		for (int i = 0; i < gscfiltsumbeamformer->fftlength / 2 + 1; i++)
		{
			gscfiltsumbeamformer->xftmp[i].r = gscfiltsumbeamformer->fft_out[i];
		}
		gscfiltsumbeamformer->xftmp[0].i = gscfiltsumbeamformer->xftmp[gscfiltsumbeamformer->fftlength / 2].i = 0.0;
		for (int i = 1; i < gscfiltsumbeamformer->fftlength / 2; i++)
		{
			gscfiltsumbeamformer->xftmp[i].i = -gscfiltsumbeamformer->fft_out[gscfiltsumbeamformer->fftlength - i];
		}

        for (i = 0; i < gscfiltsumbeamformer->fftlength / 2 + 1; i++)
        {
            gscfiltsumbeamformer->xftmp[i].r *= vol;
            gscfiltsumbeamformer->xftmp[i].i *= vol;
            gscfiltsumbeamformer->yftmp[i].r = gscfiltsumbeamformer->xftmp[i].r + gscfiltsumbeamformer->yftmp[i].r;
            gscfiltsumbeamformer->yftmp[i].i = gscfiltsumbeamformer->xftmp[i].i + gscfiltsumbeamformer->yftmp[i].i;
        }
	}

	gscfiltsumbeamformer->fft_in[0] = gscfiltsumbeamformer->yftmp[0].r;
	gscfiltsumbeamformer->fft_in[gscfiltsumbeamformer->fftlength / 2] = gscfiltsumbeamformer->yftmp[gscfiltsumbeamformer->fftlength / 2].r;
	for(int j = 1; j < gscfiltsumbeamformer->fftlength / 2; j++)
	{
		gscfiltsumbeamformer->fft_in[j] = gscfiltsumbeamformer->yftmp[j].r;
		gscfiltsumbeamformer->fft_in[gscfiltsumbeamformer->fftlength - j] = -gscfiltsumbeamformer->yftmp[j].i;
	}
	dios_ssp_share_irfft_process(gscfiltsumbeamformer->filt_FFT, gscfiltsumbeamformer->fft_in, gscfiltsumbeamformer->fft_out);
	for(int j = 0; j < gscfiltsumbeamformer->fftlength; j++)
	{
		gscfiltsumbeamformer->ytmp[j] = gscfiltsumbeamformer->fft_out[j] / gscfiltsumbeamformer->fftlength;
	}

	/* block of data in time domain */
	memcpy(y, &(gscfiltsumbeamformer->ytmp[ind_newblock]), (gscfiltsumbeamformer->fftlength / (2 * gscfiltsumbeamformer->fftoverlap)) * sizeof(float));

	return 0;
}

int dios_ssp_gsc_gscfiltsumbeamformer_delete(objFGSCfiltsumbeamformer* gscfiltsumbeamformer)
{
	int ret = 0;
	for (int i_mic = 0; i_mic < gscfiltsumbeamformer->nmic; i_mic++)
	{
		free(gscfiltsumbeamformer->Xdline[i_mic]);
	}
	free(gscfiltsumbeamformer->Xdline);
	
	free(gscfiltsumbeamformer->xftmp);
	free(gscfiltsumbeamformer->ytmp);
	free(gscfiltsumbeamformer->yftmp);
	free(gscfiltsumbeamformer->fft_out);
	free(gscfiltsumbeamformer->fft_in);
	ret = dios_ssp_share_rfft_uninit(gscfiltsumbeamformer->filt_FFT);
	if (0 != ret)
	{
		gscfiltsumbeamformer->filt_FFT = NULL;
	}

	return 0;
}
