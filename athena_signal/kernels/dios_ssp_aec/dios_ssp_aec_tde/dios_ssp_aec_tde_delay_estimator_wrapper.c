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

Description: some codes of this file refers to Webrtc (https://webrtc.org/)
which is an open source.This file is the core module of delay detection function
==============================================================================*/

/* include file */
#include "dios_ssp_aec_tde_delay_estimator_wrapper.h"

#define BACKGROUND_MUSIC

const float pi = (3.1415926f);  
// Only bit |kBandFirst| through bit |kBandLast| are processed and
// |kBandFirst| - |kBandLast| must be < 32.
enum { kBandFirst = 12 };
enum { kBandLast = 43 };

/* third level function begin */
short dios_ssp_aec_tde_maxabsvalueW16C(float* vector, int length)
{
	int i = 0, absolute = 0, maximum = 0;

	if (vector == NULL || length <= 0)
	{
		return -1;
	}

	for (i = 0; i < length; i++)
	{
		absolute = abs((int)vector[i]);

		if (absolute > maximum)
		{
			maximum = absolute;
		}
	}

	// Guard the case for abs(-32768).
	if (maximum > DIOS_SSP_WORD16_MAX)
	{
		maximum = DIOS_SSP_WORD16_MAX;
	}

	return (short)maximum;
}

/* Computes the binary spectrum by comparing the input |spectrum| with a |threshold_spectrum|. */
// Input: 
// spectrum: Spectrum of which the binary spectrum should be calculated.
// threshold_spectrum: Threshold spectrum with which the input spectrum is compared.
// Output: 
// Binary spectrum.
static unsigned int BinarySpectrum(float* spectrum, SpectrumType* threshold_spectrum, int q_domain, int* threshold_initialized)
{
    int i = kBandFirst;
    unsigned int out = 0;
    float kScale = 1 / 64.0; // adjustable
    
    if(q_domain >= 16)
    {
        return -1;
    }

    if (!(*threshold_initialized))
    {
        for (i = kBandFirst; i <= kBandLast; i++)
		{
            if (spectrum[i] > 0.0)
	        {
                threshold_spectrum[i].float_ = spectrum[i] / 2;
                *threshold_initialized = 1;
            }
        }
    }
    for (i = kBandFirst; i <= kBandLast; i++)
	{
        // Update the |threshold_spectrum|.
        threshold_spectrum[i].float_ += (spectrum[i] - threshold_spectrum[i].float_) * kScale;
        // Convert |spectrum| at current frequency bin to a binary value.
        if ((int)(spectrum[i]) > (int)(threshold_spectrum[i].float_))
	    {
            out = out | (1<<(i -kBandFirst ));
        }
    }

    return out;
}
/* third level function end */

/* second level function begin*/
/* Description: Transforms a time domain signal into the frequency domain, outputting the complex valued signal, 
 * absolute value and sum of absolute values. 
 * Input:
 * time_signal:  Pointer to time domain signal
 * Output:
 * freq_signal_real: Pointer to real part of frequency domain array freq_signal_imag: Pointer to
 * imaginary part of frequency domain array
 * freq_signal_abs: Pointer to absolute value of frequency domain array
 * Return:
 * The Q-domain of current frequency values
 */

static int TimeToFrequencyDomain(AecmCore_t* srv, float* time_signal, float* freq_signal_abs)
{
    int i = 0;
    int time_signal_scaling = 0;
    short tmp16no1;
    float fft[PART_LEN2];

    tmp16no1 = dios_ssp_aec_tde_maxabsvalueW16C(time_signal, PART_LEN2);
    time_signal_scaling = NormW16(tmp16no1);

    for (i = 0; i < PART_LEN2; i++)
    {
        fft[i] = time_signal[i] * srv->tde_ana_win[i];
    }
    dios_ssp_share_rfft_process(srv->rfft_param, fft, srv->fft_out);
    
    freq_signal_abs[0] = (float)sqrt(srv->fft_out[0] * srv->fft_out[0]);
    freq_signal_abs[PART_LEN] = (float)sqrt(srv->fft_out[PART_LEN] * srv->fft_out[PART_LEN]);
    for (i = 1; i < PART_LEN; i++)
    {
	    freq_signal_abs[i] = (float)sqrt(srv->fft_out[i] * srv->fft_out[i] + srv->fft_out[PART_LEN2 - i] * srv->fft_out[PART_LEN2 - i]);
    }

    return time_signal_scaling;
}

int dios_ssp_aec_tde_addfarspectrum(void* handle, float* far_spectrum, int spectrum_size, int far_q)
{
    DelayEstimatorFarend* self = (DelayEstimatorFarend*) handle;
    unsigned int binary_spectrum = 0;

    if (self == NULL)
    {
        return -1;
    }
    if (far_spectrum == NULL)
    {
        // Empty far end spectrum.
        return -1;
    }
    if (spectrum_size != self->spectrum_size)
    {
        // Data sizes don't match.
        return -1;
    }
    if (far_q > 15)
    {
        // If |far_q| is larger than 15 we cannot guarantee no wrap around.
        return -1;
    }

    // Get binary spectrum.
    binary_spectrum = BinarySpectrum(far_spectrum, self->mean_far_spectrum,
                                      far_q, &(self->far_spectrum_initialized));
    dios_ssp_aec_tde_addbinaryfarspectrum(self->binary_farend, binary_spectrum);

    return 0;
}

int dios_ssp_aec_tde_delayestimateprocess(void* handle, float* near_spectrum, int spectrum_size, int near_q)
{
    DelayEstimator* self = (DelayEstimator*) handle;
    unsigned int binary_spectrum = 0;

    if (self == NULL)
    {
        return -1;
    }
    if (near_spectrum == NULL)
    {
        // Empty near end spectrum.
        return -1;
    }
    if (spectrum_size != self->spectrum_size)
    {
        // Data sizes don't match.
        return -1;
    }
    if (near_q > 15)
    {
        // If |near_q| is larger than 15 we cannot guarantee no wrap around.
        return -1;
    }

    // Get binary spectra.
    binary_spectrum = BinarySpectrum(near_spectrum, self->mean_near_spectrum, near_q, &(self->near_spectrum_initialized));

    return dios_ssp_aec_tde_processbinaryspectrum(self->binary_handle, binary_spectrum);
}
/* second level function end*/

/* first level function begin */
int dios_ssp_aec_tde_ProcessBlock(AecmCore_t * srv,
                                  float * farend,
                                  float * nearendNoisy)
{
    int i;
    int flag_delayfind = 0;
    int max_v, max_i;  /* find maximal value and index of histogram */
    float xfa[PART_LEN1];    /* farend signal frequency domain amplitude */
    float dfaNoisy[PART_LEN1];   /* near end signal frequency domain amplitude */
    int delay;
    short zerosDBufNoisy;
    int far_q;

    memcpy(srv->xBuf, srv->xBuf + PART_LEN, sizeof(float) * PART_LEN);
    memcpy(srv->dBufNoisy,srv->dBufNoisy + PART_LEN, sizeof(float) * PART_LEN);
    // Buffer near and far end signals
    memcpy(srv->xBuf + PART_LEN, farend, sizeof(float) * PART_LEN);
    memcpy(srv->dBufNoisy + PART_LEN, nearendNoisy, sizeof(float) * PART_LEN);

    /* hanning window FFT. multiply 128points with a 128-point hanning window, then FFT*/
    // Transform far end signal from time domain to frequency domain.
    far_q = TimeToFrequencyDomain(srv,
                                  srv->xBuf,
                                  xfa);

    // Transform noisy near end signal from time domain to frequency domain.
    zerosDBufNoisy = TimeToFrequencyDomain(srv,
                                           srv->dBufNoisy,
                                           dfaNoisy);

    // Get the delay
    // Save far-end history and estimate delay
    // Get new buffer position
    srv->far_history_pos++;
    if (srv->far_history_pos >= srv->max_delay_history_size) 
    {
        srv->far_history_pos = 0;
    }
    // Update Q-domain buffer
    srv->far_q_domains[srv->far_history_pos] = far_q;
    // Update far end spectrum buffer
    memcpy(&(srv->far_history[srv->far_history_pos * PART_LEN1]), xfa, sizeof(float) * PART_LEN1);

    if (dios_ssp_aec_tde_addfarspectrum(srv->delay_estimator_farend, xfa, PART_LEN1, far_q) == -1)
    {
        return -1;
    }
    delay = dios_ssp_aec_tde_delayestimateprocess(srv->delay_estimator, dfaNoisy, PART_LEN1, zerosDBufNoisy);
    
    if (delay == -1)
    {
        return -1;
    }
    else if (delay == -2)
    {
        // If the delay is unknown, we assume zero.
        // NOTE: this will have to be adjusted if we ever add lookahead.
        delay = 0;
    }

    if (srv->fixedDelay >= 0)
    {
        // Use fixed delay
        delay = srv->fixedDelay;
    }

    // histogram
    memmove(srv->delayN+1, srv->delayN, (srv->win_slide-1) * sizeof(int)); // DELAY_WIN_SLIDE
    srv->delayN[0] = delay; //delay_t0;
    memset(srv->delayHistVect,0, srv->max_delay_size*sizeof(int));
    for (i = 0; i < srv->win_slide; i++) // DELAY_WIN_SLIDE
    {
		if (srv->delayN[i] >= 0 && srv->delayN[i] < srv->max_delay_size)
		{
			srv->delayHistVect[srv->delayN[i]]++;
		}
		else
		{
			printf("Delay exceed the estimate range!");
		}
    }

    srv->delayHistVect[0] = 0; 
    max_v = srv->delayHistVect[0]; 
    max_i = 0; 
    for (i = 1; i < srv->max_delay_size; i++)
    {
        if (srv->delayHistVect[i] > max_v)
        {
            max_v = srv->delayHistVect[i]; 
            max_i = i; 
        }
    }
    if ((max_v > srv->win_slide * 0.8f) && (max_i!=0) && (max_i > srv->delay_nframe + 2 || max_i < srv->delay_nframe - 2)) 
    { 
        srv->delay_nframe = max_i;
        srv->delay_nsample = max_i * PART_LEN;
		flag_delayfind = 1;
    }

    return (flag_delayfind);
}

int get_tde_final(AecmCore_t * srv)
{
	return (srv->delay_nsample);
}
/* first level function end */

void dios_ssp_aec_tde_freedelayestimator(void* handle)
{
    DelayEstimator* self = (DelayEstimator*) handle;

    if (handle == NULL)
    {
        return;
    }

    free(self->mean_near_spectrum);
    self->mean_near_spectrum = NULL;

    dios_ssp_aec_tde_freebinarydelayestimator(self->binary_handle);
    self->binary_handle = NULL;

    free(self);
}

void dios_ssp_aec_tde_freedelayestimatorfarend(void* handle)
{
    DelayEstimatorFarend* self = (DelayEstimatorFarend*) handle;

    if (handle == NULL)
    {
        return;
    }

    free(self->mean_far_spectrum);
    self->mean_far_spectrum = NULL;

    dios_ssp_aec_tde_freebinarydelayestimatorfarend(self->binary_farend);
    self->binary_farend = NULL;

    free(self);
}

int dios_ssp_aec_tde_initdelayestimatorfarend(void* handle)
{
    DelayEstimatorFarend* self = (DelayEstimatorFarend*) handle;

    if (self == NULL)
    {
        return -1;
    }

    // Initialize far-end part of binary delay estimator.
    dios_ssp_aec_tde_initbinarydelayestimatorfarend(self->binary_farend);

    // Set averaged far and near end spectra to zero.
    memset(self->mean_far_spectrum, 0, sizeof(SpectrumType) * self->spectrum_size);
    // Reset initialization indicators.
    self->far_spectrum_initialized = 0;

    return 0;
}

int dios_ssp_aec_tde_initdelayestimator(void* handle)
{
    DelayEstimator* self = (DelayEstimator*) handle;

    if (self == NULL)
    {
        return -1;
    }

    // Initialize binary delay estimator.
    dios_ssp_aec_tde_initbinarydelayestimator(self->binary_handle);

    // Set averaged far and near end spectra to zero.
    memset(self->mean_near_spectrum, 0, sizeof(SpectrumType) * self->spectrum_size);
    // Reset initialization indicators.
    self->near_spectrum_initialized = 0;

    return 0;
}

void* dios_ssp_aec_tde_creatdelayestimatorfarend(int spectrum_size, int history_size)
{
    DelayEstimatorFarend* self = NULL;

    // Check if the sub band used in the delay estimation is small enough to fit
    // the binary spectra in a unsigned int.
    //COMPILE_ASSERT(kBandLast - kBandFirst < 32);

    if (spectrum_size >= kBandLast)
    {
        self = (DelayEstimatorFarend*)calloc(1, sizeof(DelayEstimator));
    }

    if (self != NULL)
    {
        int memory_fail = 0;

        // Allocate memory for the binary far-end spectrum handling.
        self->binary_farend = dios_ssp_aec_tde_creatbinarydelayestimatorfarend(history_size);
        memory_fail |= (self->binary_farend == NULL);

        // Allocate memory for spectrum buffers.
        self->mean_far_spectrum = (SpectrumType*)calloc(spectrum_size, sizeof(SpectrumType));
        memory_fail |= (self->mean_far_spectrum == NULL);

        self->spectrum_size = spectrum_size;

        if (memory_fail)
	    {
            dios_ssp_aec_tde_freedelayestimatorfarend(self);
            self = NULL;
        }
    }

    return self;
}

void* dios_ssp_aec_tde_creatdelayestimator(void* farend_handle, int max_lookahead)
{
    DelayEstimator* self = NULL;
    DelayEstimatorFarend* farend = (DelayEstimatorFarend*) farend_handle;

    if (farend_handle != NULL)
    {
        self = (DelayEstimator*)calloc(1, sizeof(DelayEstimator));
    }

    if (self != NULL)
    {
        int memory_fail = 0;

        // Allocate memory for the farend spectrum handling.
        self->binary_handle = dios_ssp_aec_tde_creatbinarydelayestimator(farend->binary_farend, max_lookahead);
        memory_fail |= (self->binary_handle == NULL);

        // Allocate memory for spectrum buffers.
        self->mean_near_spectrum = (SpectrumType*)calloc(farend->spectrum_size, sizeof(SpectrumType));
        memory_fail |= (self->mean_near_spectrum == NULL);

        self->spectrum_size = farend->spectrum_size;

        if (memory_fail)
	    {
            dios_ssp_aec_tde_freedelayestimator(self);
            self = NULL;
        }
    }

    return self;
}

int dios_ssp_aec_tde_robust_validation(void* handle, int enable)
{
    DelayEstimator* self = (DelayEstimator*) handle;

    if (self == NULL)
    {
        return -1;
    }
    if ((enable < 0) || (enable > 1))
    {
        return -1;
    }
    if(self->binary_handle == NULL)
    {
        return -1;
    }
    self->binary_handle->robust_validation_enabled = enable;
    return 0;
}

/* initialization */
int dios_ssp_aec_tde_creatcore(AecmCore_t **aecmInst, int max_delay_size, int win_slide)
{
    AecmCore_t *srv = (AecmCore_t*)calloc(1, sizeof(AecmCore_t));
    *aecmInst = srv;
    if (srv == NULL)
    {
        return -1;
    }

    srv->farFrameBuf = dios_ssp_aec_tde_creatbuffer(FRAME_LEN + PART_LEN, sizeof(short));
    if (!srv->farFrameBuf)
    {
        dios_ssp_aec_tde_freecore(srv);
        srv = NULL;
        return -1;
    }

    srv->nearNoisyFrameBuf = dios_ssp_aec_tde_creatbuffer(FRAME_LEN + PART_LEN, sizeof(short));
    if (!srv->nearNoisyFrameBuf)
    {
        dios_ssp_aec_tde_freecore(srv);
        srv = NULL;
        return -1;
    }

    srv->nearCleanFrameBuf = dios_ssp_aec_tde_creatbuffer(FRAME_LEN + PART_LEN, sizeof(short));
    if (!srv->nearCleanFrameBuf)
    {
        dios_ssp_aec_tde_freecore(srv);
        srv = NULL;
        return -1;
    }

    srv->outFrameBuf = dios_ssp_aec_tde_creatbuffer(FRAME_LEN + PART_LEN, sizeof(short));
    if (!srv->outFrameBuf)
    {
        dios_ssp_aec_tde_freecore(srv);
        srv = NULL;
        return -1;
    }

    srv->max_delay_history_size = max_delay_size;
    srv->delay_estimator_farend = dios_ssp_aec_tde_creatdelayestimatorfarend(PART_LEN1, srv->max_delay_history_size);                               
                                                                     
    if (srv->delay_estimator_farend == NULL)
	{
		dios_ssp_aec_tde_freecore(srv);
		srv = NULL;
		return -1;
    }
    srv->delay_estimator = dios_ssp_aec_tde_creatdelayestimator(srv->delay_estimator_farend, 0);
    if (srv->delay_estimator == NULL)
	{
		dios_ssp_aec_tde_freecore(srv);
		srv = NULL;
		return -1;
    }
    dios_ssp_aec_tde_robust_validation(srv->delay_estimator, 1); 
    // Init some srv pointers. 16 and 32 byte alignment is only necessary
    // for Neon code currently.
    //srv->xBuf = (float*) (((unsigned long)srv->xBuf_buf + 31) & ~ 31);
    //srv->dBufClean = (short*) (((unsigned long)srv->dBufClean_buf + 31) & ~ 31);
    //srv->dBufNoisy = (float*) (((unsigned long)srv->dBufNoisy_buf + 31) & ~ 31);
    //srv->outBuf = (short*) (((unsigned long)srv->outBuf_buf + 15) & ~ 15);
    //srv->channelStored = (short*) (((unsigned long)srv->channelStored_buf + 15) & ~ 15);
    //srv->channelAdapt16 = (short*) (((unsigned long)srv->channelAdapt16_buf + 15) & ~ 15);
    //srv->channelAdapt32 = (int*) (((unsigned long)srv->channelAdapt32_buf + 31) & ~ 31);

    // Init some srv pointers.
	srv->xBuf = srv->xBuf_buf;
    srv->dBufClean = srv->dBufClean_buf;
    srv->dBufNoisy = srv->dBufNoisy_buf;
    srv->outBuf = srv->outBuf_buf;
    srv->channelStored = srv->channelStored_buf;
    srv->channelAdapt16 = srv->channelAdapt16_buf;
    srv->channelAdapt32 = srv->channelAdapt32_buf;

    srv->win_slide = win_slide; 
	srv->max_delay_size = max_delay_size; // 100 
	//srv->max_long_delay_size = MAX_DELAY_LONG; // 750
    srv->delayHistVect    = NULL; 
    srv->delayN    = NULL; 
    srv->delayHistVect = (int *)calloc(srv->max_delay_size, sizeof(int)); 
	srv->delayN = (int *)calloc(srv->win_slide, sizeof(int)); //

	return 0;
}

int dios_ssp_aec_tde_initcore(AecmCore_t * const srv)
{
    int i = 0;
    int tmp32 = PART_LEN1 * PART_LEN1;
    short tmp16 = PART_LEN1;

    // sanity check of sampling frequency
    //srv->mult = (short)samplingFreq / 8000;

    dios_ssp_aec_tde_initbuffer(srv->farFrameBuf);
    dios_ssp_aec_tde_initbuffer(srv->nearNoisyFrameBuf);
    dios_ssp_aec_tde_initbuffer(srv->nearCleanFrameBuf);
    dios_ssp_aec_tde_initbuffer(srv->outFrameBuf);

    memset(srv->xBuf_buf, 0, sizeof(srv->xBuf_buf));
    memset(srv->dBufClean_buf, 0, sizeof(srv->dBufClean_buf));
    memset(srv->dBufNoisy_buf, 0, sizeof(srv->dBufNoisy_buf));
    memset(srv->outBuf_buf, 0, sizeof(srv->outBuf_buf));

    srv->totCount = 0;

    if (dios_ssp_aec_tde_initdelayestimatorfarend(srv->delay_estimator_farend) != 0)
	{
        return -1;
    }
    if (dios_ssp_aec_tde_initdelayestimator(srv->delay_estimator) != 0)
	{
        return -1;
    }
    // Set far end histories to zero
    memset(srv->far_history, 0, sizeof(float) * PART_LEN1 * MAX_DELAY_LONG);
    memset(srv->far_q_domains, 0, sizeof(int) * MAX_DELAY_LONG);
	srv->far_history_pos = srv->max_delay_history_size; // increase est. delay range

    srv->fixedDelay = -1;

    // Shape the initial noise level to an approximate pink noise.
    for (i = 0; i < (PART_LEN1 >> 1) - 1; i++)
    {
        srv->noiseEst[i] = (tmp32 << 8);
        tmp16--;
        tmp32 -= (int)((tmp16 << 1) + 1);
    }
    for (; i < PART_LEN1; i++)
    {
        srv->noiseEst[i] = (tmp32 << 8);
    }

    srv->farEnergyVAD = FAR_ENERGY_MIN; // This prevents false speech detection at the
                                         // beginning.
    srv->farEnergyMSE = 0;
    srv->currentVADValue = 0;
    srv->vadUpdateCount = 0;

    srv->delay_nframe    = 0;
    srv->delay_nsample    = 0;
    memset(srv->delayHistVect, 0, srv->max_delay_size * sizeof(int)); 
	memset(srv->delayN, 0, srv->win_slide * sizeof(int)); 

    srv->rfft_param = dios_ssp_share_rfft_init(PART_LEN2);
    memset(srv->fft_out, 0, sizeof(srv->fft_out));
    for (i=0; i < PART_LEN2; i++)
    {
        srv->tde_ana_win[i] = (float)sqrt(0.5 * (1-cos(2*PI*i/PART_LEN2)));
    }
    return 0;
}

int dios_ssp_aec_tde_freecore(AecmCore_t *srv)
{
    if (srv == NULL)
    {
        return -1;
    }

    dios_ssp_aec_tde_freebuffer(srv->farFrameBuf);
    dios_ssp_aec_tde_freebuffer(srv->nearNoisyFrameBuf);
    dios_ssp_aec_tde_freebuffer(srv->nearCleanFrameBuf);
    dios_ssp_aec_tde_freebuffer(srv->outFrameBuf);

    dios_ssp_aec_tde_freedelayestimator(srv->delay_estimator);
    dios_ssp_aec_tde_freedelayestimatorfarend(srv->delay_estimator_farend);
    dios_ssp_share_rfft_uninit(srv->rfft_param);

    if (srv->delayHistVect != NULL)
    {
        free(srv->delayHistVect);
        srv->delayHistVect = NULL;
    }
    if (srv->delayN != NULL)
    {
        free(srv->delayN);
        srv->delayN = NULL;
    }

    free(srv);

    return 0;
}
