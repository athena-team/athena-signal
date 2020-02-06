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

Description: Basic operation: FFT and IFFT.
==============================================================================*/

/* include file */
#include "dios_ssp_share_rfft.h"

typedef struct {
    int fft_len;
    int Mq_max;
    float *wr;
    float *wi;
} RFFT_PARAM;

void *dios_ssp_share_rfft_init(int fft_len)
{
    void *rfft_handle = NULL;
    rfft_handle = (void*)calloc(1, sizeof(RFFT_PARAM));
    RFFT_PARAM *rfft_param;
    rfft_param = (RFFT_PARAM*)rfft_handle;
    int i = 0;
    int j = 0;
    rfft_param->fft_len = fft_len;
    for (i = 1; (j =  i << 1) <= rfft_param->fft_len; i = j)
	{
        rfft_param->Mq_max = i >> 1;
    }
    rfft_param->wi = (float *)calloc(rfft_param->Mq_max - 1, sizeof(float));
    rfft_param->wr = (float *)calloc(rfft_param->Mq_max - 1, sizeof(float));
    if (NULL == rfft_param->wi || NULL == rfft_param->wr)
	{
        puts("Memory allocation error.\n");
        return NULL;
    }

    float theta = 0;
    theta = (float)(-2.0f * PI / rfft_param->fft_len);
    for (i = 1; i < rfft_param->Mq_max; i++)
	{
        rfft_param->wr[i - 1] = (float)cos(theta * i);
        rfft_param->wi[i - 1] = (float)sin(theta * i);
    }
    return(rfft_handle);
}

int dios_ssp_share_rfft_process(void *rfft_handle, float *inbuffer, float *outbuffer)
{
    if (NULL == rfft_handle)
	{
        return -1;
    }
    RFFT_PARAM *rfft_param;
    rfft_param = (RFFT_PARAM*)rfft_handle;
    int m = 0;
    int mh = 0;
    int mq = 0;
    int i = 0;
    int j = 0;
    int k = 0;
    int jr = 0;
    int ji = 0;
    int kr = 0;
    int ki = 0;
    int fft_len = rfft_param->fft_len;
    float xr = 0;
    float xi = 0;

    for (i = 0; i < fft_len; i++)
	{
        outbuffer[i] = inbuffer[i];
    }
    i = 0;
    for (j = 1; j < fft_len - 1; j++)
	{
        for (k = fft_len >> 1; k > (i ^= k); k >>= 1)
		{
        }
        if (j < i)
		{
            xr = outbuffer[j];
            outbuffer[j] = outbuffer[i];
            outbuffer[i] = xr;
        }
    }

    for (mh = 1; (m = mh << 1) <= fft_len; mh = m)
	{
        mq = mh >> 1;

        /* ---- real to real butterflies (W == 1) ---- */
        for (jr = 0; jr < fft_len; jr += m)
		{
            kr = jr + mh;
            xr = outbuffer[kr];
            outbuffer[kr] = outbuffer[jr] - xr;
            outbuffer[jr] += xr;
        }

        /* ---- complex to complex butterflies (W != 1) ---- */
        for (i = 1; i < mq; i++)
		{
            for (j = 0; j < fft_len; j += m)
			{
                jr = j + i;
                ji = j + mh - i;
                kr = j + mh + i;
                ki = j + m - i;
                xr = rfft_param->wr[rfft_param->Mq_max / mq * i - 1] * outbuffer[kr] +
                    rfft_param->wi[rfft_param->Mq_max / mq * i - 1] * outbuffer[ki];
                xi = rfft_param->wr[rfft_param->Mq_max / mq * i - 1] * outbuffer[ki] -
                    rfft_param->wi[rfft_param->Mq_max / mq * i - 1] * outbuffer[kr];
                outbuffer[kr] = -outbuffer[ji] + xi;
                outbuffer[ki] = outbuffer[ji] + xi;
                outbuffer[ji] = outbuffer[jr] - xr;
                outbuffer[jr] = outbuffer[jr] + xr;
            }
        }
        /* ---- real to complex butterflies are trivial ---- */
    }
    return 0;
}

int dios_ssp_share_irfft_process(void *rfft_handle, float *inbuffer, float *outbuffer)
{
    if (NULL == rfft_handle)
	{
        return -1;
    }
    RFFT_PARAM *rfft_param;
    rfft_param = (RFFT_PARAM*)rfft_handle;
    int m = 0;
    int mh = 0;
    int mq = 0;
    int i = 0;
    int j = 0;
    int k = 0;
    int jr = 0;
    int ji = 0;
    int kr = 0;
    int ki = 0;
    int fft_len = rfft_param->fft_len;
    float xr = 0;
    float xi = 0;

    for (i = 0; i < fft_len; i++)
	{
        outbuffer[i] = inbuffer[i];
    }
    outbuffer[0] *= 0.5;
    outbuffer[fft_len / 2] *= 0.5;

    for (m = fft_len; (mh = m >> 1) >= 1; m = mh)
	{
        mq = mh >> 1;

        /* ---- real to real butterflies (W == 1) ---- */
        for (jr = 0; jr < fft_len; jr += m)
		{
            kr = jr + mh;
            xr = outbuffer[jr] - outbuffer[kr];
            outbuffer[jr] += outbuffer[kr];
            outbuffer[kr] = xr;
        }

        /* ---- complex to complex butterflies (W != 1) ---- */
        for (i = 1; i < mq; i++)
		{
            for (j = 0; j < fft_len; j += m)
			{
                jr = j + i;
                ji = j + mh - i;
                kr = j + mh + i;
                ki = j + m - i;
                xr = outbuffer[jr] - outbuffer[ji];
                xi = outbuffer[ki] + outbuffer[kr];
                outbuffer[jr] = outbuffer[jr] + outbuffer[ji];
                outbuffer[ji] = outbuffer[ki] - outbuffer[kr];
                outbuffer[kr] = rfft_param->wr[rfft_param->Mq_max / mq * i - 1] * xr -
                    rfft_param->wi[rfft_param->Mq_max / mq * i - 1] * xi;
                outbuffer[ki] = rfft_param->wr[rfft_param->Mq_max / mq * i - 1] * xi + 
                    rfft_param->wi[rfft_param->Mq_max / mq * i - 1] * xr;
            }
        }
        /* ---- complex to real butterflies are trivial ---- */
    }

    /* ---- unscrambler ---- */
    i = 0;
    for (j = 1; j < fft_len - 1; j++)
	{
        for (k = fft_len >> 1; k > (i ^= k); k >>= 1)
		{
        }
        if (j < i)
		{
            xr = outbuffer[j];
            outbuffer[j] = outbuffer[i];
            outbuffer[i] = xr;
        }
    }

    for (j = 0; j <= fft_len - 1; j++)
	{
        /*a[j] *= 2.0 / fft_len;*/    /* <=== Default */
        outbuffer[j] *= 2.0;            /* <=== use for SubBand. */
    }
    return 0;
}

int dios_ssp_share_rfft_uninit(void *rfft_handle)
{
	if (NULL == rfft_handle)
	{
		return -1;
	}
    RFFT_PARAM *rfft_param;
    rfft_param = (RFFT_PARAM*)rfft_handle;
    free(rfft_param->wi);
    free(rfft_param->wr);
    free(rfft_param);

	return 0;
}

