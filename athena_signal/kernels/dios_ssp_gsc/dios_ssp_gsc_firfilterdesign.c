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

Description: define and implement FIR filter designing
==============================================================================*/

#include "dios_ssp_gsc_firfilterdesign.h"

void WinBlackman(float *win, int len)
{
	int i;

	for (i = 0; i < len; i++)
    {
		win[i] = 0.42f - 0.5f * (float)cos(2.0f * PI * (float)i / (float)(len - 1)) + 0.08f * (float)cos(4.0f * PI * (float)i / (float)(len - 1));
    }
}

void WinHamming(float *win, int len)
{
	int i;

	for (i = 0; i < len; i++)
    {
		win[i] = 0.54f - 0.46f * (float)cos(2.0f * PI * (float)i / (float)(len - 1));
    }
}

void WinHanning(float *win, int len)
{
	int i;

	for (i = 0; i < len; i++)
    {
		win[i] = 0.5f - 0.5f * (float)cos(2.0f * PI * (float)i / (float)(len - 1));
    }
}

void dios_ssp_gscfirfilterdesign_init(objCGeneralFIRDesigner *generalfirdesign, int len, General_WindowType winType)
{
	/* calculate FFT_len and make FFT_len >= FIR_len */
	generalfirdesign->m_nFIRLen = len;
	generalfirdesign->m_nFFTOrder = 0;
	generalfirdesign->m_nFFTLen = 1;
	while (generalfirdesign->m_nFFTLen < len)
	{	
		generalfirdesign->m_nFFTLen <<= 1;
		generalfirdesign->m_nFFTOrder++;
	}
	generalfirdesign->m_nCSize = (generalfirdesign->m_nFFTLen >> 1) + 1;

	/* if FFT_len > FIR_len, use local taps buffer */
	if (generalfirdesign->m_nFFTLen > generalfirdesign->m_nFIRLen)
    {
		generalfirdesign->m_pTapsBuf = (float*)calloc(generalfirdesign->m_nFFTLen, sizeof(float));
    }
	else
    {
		generalfirdesign->m_pTapsBuf = NULL;
    }

	generalfirdesign->gengralfir_FFT = dios_ssp_share_rfft_init(generalfirdesign->m_nFFTLen);
	generalfirdesign->fft_out = (float*)calloc(generalfirdesign->m_nFFTLen, sizeof(float));
	generalfirdesign->fft_in = (float*)calloc(generalfirdesign->m_nFFTLen, sizeof(float));
	generalfirdesign->m_pTapsFreq = (xcomplex*)calloc(generalfirdesign->m_nCSize, sizeof(xcomplex));
	generalfirdesign->m_mag = (float*)calloc(generalfirdesign->m_nCSize, sizeof(float));
	generalfirdesign->m_phase = (float*)calloc(generalfirdesign->m_nCSize, sizeof(float));

	if (winType == General_WinRect)
    {
		generalfirdesign->m_win = NULL;
    }
	else
	{
		generalfirdesign->m_win = (float*)calloc(generalfirdesign->m_nFIRLen, sizeof(float));
		switch (winType)
		{
        case General_WinBlackman:
            WinBlackman(generalfirdesign->m_win, generalfirdesign->m_nFIRLen);
			break;
		case General_WinHamming:
			WinHamming(generalfirdesign->m_win, generalfirdesign->m_nFIRLen);
			break;
		case General_WinHanning:
			WinHanning(generalfirdesign->m_win, generalfirdesign->m_nFIRLen);
			break;
        default:
			break;
		}
	}
}

int dios_ssp_gscfirfilterdesign_fractionaldelay(objCGeneralFIRDesigner *generalfirdesign, float fcLow, float fcHigh, float delay, float *pTaps)
{
	int i;
	int length;

	length = 1 << (generalfirdesign->m_nFFTOrder);
	/* if FFT_len > FIR_len, use local taps buffer */
	if (generalfirdesign->m_nFFTLen == generalfirdesign->m_nFIRLen)
    {
		generalfirdesign->m_pTapsBuf = pTaps;
    }

	memset(generalfirdesign->m_pTapsBuf, 0, sizeof(float) * generalfirdesign->m_nFFTLen);  /* clear taps */

	int	delay_int = (int)delay;
	int shift = delay_int - generalfirdesign->m_nFIRLen / 2;
	generalfirdesign->m_pTapsBuf[generalfirdesign->m_nFIRLen / 2] = 1.0;  /* initial taps with integer delay */

	float frac = delay - (float)delay_int;  /* fractional part of delay */
	short bHighpass = (fcLow > EPSILON) ? 1 : 0;
	short bLowpass = (fcHigh < 0.5) ? 1 : 0;
	short bTFMapping = bHighpass || bLowpass || (frac >= EPSILON);

	/* FFT of delay taps */
	if (bTFMapping)
    {
		dios_ssp_share_rfft_process(generalfirdesign->gengralfir_FFT, generalfirdesign->m_pTapsBuf, generalfirdesign->fft_out);
		for (int i = 0; i < generalfirdesign->m_nFFTLen / 2 + 1; i++)
		{
			generalfirdesign->m_pTapsFreq[i].r = generalfirdesign->fft_out[i];
		}
		generalfirdesign->m_pTapsFreq[0].i = generalfirdesign->m_pTapsFreq[generalfirdesign->m_nFFTLen / 2].i = 0.0;
		for (int i = 1; i < generalfirdesign->m_nFFTLen / 2; i++)
		{
			generalfirdesign->m_pTapsFreq[i].i = -generalfirdesign->fft_out[generalfirdesign->m_nFFTLen - i];
		}
    }
	
	/* fractional delay */
	if (frac >= EPSILON)
	{
		/* convert to magnitude and phase vectors */
		for (i = 0; i < length / 2 + 1; i++)
		{
			generalfirdesign->m_mag[i] = (float)sqrt(generalfirdesign->m_pTapsFreq[i].r * generalfirdesign->m_pTapsFreq[i].r + generalfirdesign->m_pTapsFreq[i].i * generalfirdesign->m_pTapsFreq[i].i);
			if (generalfirdesign->m_pTapsFreq[i].r == 0.0)
			{
				if (generalfirdesign->m_pTapsFreq[i].i > 0.0)
					generalfirdesign->m_phase[i] = PI / 2;
				if (generalfirdesign->m_pTapsFreq[i].i == 0.0)
					generalfirdesign->m_phase[i] = 0.0;
				if (generalfirdesign->m_pTapsFreq[i].i < 0.0)
					generalfirdesign->m_phase[i] = -PI / 2;
			}
			else
			{
				/* between -PI and PI */
				generalfirdesign->m_phase[i] = (float)atan(generalfirdesign->m_pTapsFreq[i].i / generalfirdesign->m_pTapsFreq[i].r);
				if (generalfirdesign->m_pTapsFreq[i].r < 0.0 && generalfirdesign->m_pTapsFreq[i].i >= 0.0)
					generalfirdesign->m_phase[i] += PI;
				if (generalfirdesign->m_pTapsFreq[i].r < 0.0 && generalfirdesign->m_pTapsFreq[i].i < 0.0)
					generalfirdesign->m_phase[i] -= PI;
			}
			
		}
		
		/* phase shift: delta_phi(w) = w * d * (-2 * PI / N) */
		float delta = -2 * PI * frac / generalfirdesign->m_nFFTLen;		
		
		for  (int i = 0; i < generalfirdesign->m_nCSize; i++)
        {
			generalfirdesign->m_phase[i] += i * delta;
        }

		/* convert back to complex */
		for (i = 0; i < length / 2 + 1; i++)
		{
			generalfirdesign->m_pTapsFreq[i].r = generalfirdesign->m_mag[i] * (float)cos(generalfirdesign->m_phase[i]);
			generalfirdesign->m_pTapsFreq[i].i = generalfirdesign->m_mag[i] * (float)sin(generalfirdesign->m_phase[i]);
		}
		generalfirdesign->m_pTapsFreq[generalfirdesign->m_nCSize - 1].i = 0;  /* X(fs/2) = real */
	}

	/* highpass filtering */
	if (bHighpass)
	{
		float wLow = fcLow * generalfirdesign->m_nFFTLen + 0.5f;
		int nwLow = (int)wLow;  /* nint(fc * N): cut-off frequency bin */
		float scale_wc = (float)nwLow - wLow + 1.0f;  /* 1 - (fc * N) - 0.5 + nint(fc * N) */
		for (int i = 0; i < nwLow; i++)
        {
			generalfirdesign->m_pTapsFreq[i].r = 0.0;
            generalfirdesign->m_pTapsFreq[i].i = 0.0;  /* suppress low frequency */
        }
		generalfirdesign->m_pTapsFreq[nwLow].r *= scale_wc;
		generalfirdesign->m_pTapsFreq[nwLow].i *= scale_wc;
	}

	/* lowpass filtering */
	if (bLowpass)
	{
		float wHigh = fcHigh * generalfirdesign->m_nFFTLen + 0.5f;
		int	nwHigh = (int)wHigh;  /* nint(fc * N): cut-off frequency bin */
		float scale_wc = wHigh - (float)nwHigh;  /* (fc * N) + 0.5 - nint(fc * N) */
		for (int i = nwHigh + 1; i < generalfirdesign->m_nCSize; i++)  /* suppress high frequency */
        {
			generalfirdesign->m_pTapsFreq[i].r = 0.0;
            generalfirdesign->m_pTapsFreq[i].i = 0.0;
        }
		generalfirdesign->m_pTapsFreq[nwHigh].r *= scale_wc;
		generalfirdesign->m_pTapsFreq[nwHigh].i *= scale_wc;
	}

	/* Inverse FFT to time domain */
	if (bTFMapping)
	{
		generalfirdesign->fft_in[0] = generalfirdesign->m_pTapsFreq[0].r;
		generalfirdesign->fft_in[generalfirdesign->m_nFFTLen / 2] = generalfirdesign->m_pTapsFreq[generalfirdesign->m_nFFTLen / 2].r;
		for(int j = 1; j < generalfirdesign->m_nFFTLen / 2; j++)
		{
			generalfirdesign->fft_in[j] = generalfirdesign->m_pTapsFreq[j].r;
			generalfirdesign->fft_in[generalfirdesign->m_nFFTLen - j] = -generalfirdesign->m_pTapsFreq[j].i;
		}
		dios_ssp_share_irfft_process(generalfirdesign->gengralfir_FFT, generalfirdesign->fft_in, generalfirdesign->fft_out);
		for(int j = 0; j < generalfirdesign->m_nFFTLen; j++)
		{
			generalfirdesign->m_pTapsBuf[j] = generalfirdesign->fft_out[j] / generalfirdesign->m_nFFTLen;
		}
	
		/* windowing */
		if (generalfirdesign->m_win != NULL)
        {
			for (int i = 0; i < generalfirdesign->m_nFIRLen; i++)
            {
				generalfirdesign->m_pTapsBuf[i] *= generalfirdesign->m_win[i];
            }
        }
	}

	/* right shift */
	if (shift > 0)
	{
		for (i = generalfirdesign->m_nFIRLen - 1; i >= shift; i--)
        {
			generalfirdesign->m_pTapsBuf[i] = generalfirdesign->m_pTapsBuf[i - shift];
        }
		for (i = 0; i < shift; i++)
        {
			generalfirdesign->m_pTapsBuf[i] = 0.0;
        }
	}
	/* left shift */
	else if (shift < 0)
	{
		for (i = 0; i < (generalfirdesign->m_nFIRLen + shift); i++)
        {
			generalfirdesign->m_pTapsBuf[i] = generalfirdesign->m_pTapsBuf[i - shift];
        }
		for (; i < generalfirdesign->m_nFIRLen; i++)
        {
			generalfirdesign->m_pTapsBuf[i] = 0.0;
        }
	}

	if (generalfirdesign->m_nFFTLen > generalfirdesign->m_nFIRLen)
    {
		memcpy(pTaps, generalfirdesign->m_pTapsBuf, sizeof(float) * generalfirdesign->m_nFIRLen);
    }
	else
    {
		generalfirdesign->m_pTapsBuf = NULL;
    }

	return 0;
}

int dios_ssp_gscfirfilterdesign_delete(objCGeneralFIRDesigner *generalfirdesign)
{
	int ret = 0;
	free(generalfirdesign->m_pTapsFreq);
	free(generalfirdesign->m_mag);
	free(generalfirdesign->m_phase);
	free(generalfirdesign->m_win);
	free(generalfirdesign->m_pTapsBuf);
	free(generalfirdesign->fft_out);
	free(generalfirdesign->fft_in);
	ret = dios_ssp_share_rfft_uninit(generalfirdesign->gengralfir_FFT);
	if (0 != ret)
	{
		generalfirdesign->gengralfir_FFT = NULL;
	}

	return 0;
}
