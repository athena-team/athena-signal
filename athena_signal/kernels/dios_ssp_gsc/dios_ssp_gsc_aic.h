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
==============================================================================*/

#ifndef _DIOS_SSP_GSC_AIC_H_
#define _DIOS_SSP_GSC_AIC_H_

#include "dios_ssp_gsc_dsptools.h"
#include "dios_ssp_gsc_abm.h"
#include "dios_ssp_gsc_globaldefs.h"
#include "../dios_ssp_share/dios_ssp_share_rfft.h"
#include "../dios_ssp_share/dios_ssp_share_complex_defs.h"

typedef struct
{
	int nmic;        /* number of microphones */
    int fftsize;     /* FFT length */
    int fftoverlap;  /* FFT overlap */
    int sigsoverlap; /* overlap factor of input signal segments */
    float lambda;    /* forgetting factor power estimation */
    float mu;        /* stepsize */
    float delta_con; /* threshold for constant regularization */
    float delta_dyn; /* threshold for dynamic regularization */
    float s0_dyn;    /* 'lobe' of dynamic regularization function */
    int regularize_dyn;  /* use dynamic regularization or constant regularization */
    int ntaps;       /* number of filter taps */
    int bdlinesize;  /* length of block delay line */
    int pbdlinesize; /* block delayline length for partitioned block adaptive filter input */
    int syncdly;     /* delay for causality of adaptive filters */
    xcomplex nu;     /* forgetting factor for adaptive filter coefficients */
    int count_sigsegments;  /* counter for input signal segments */
    float *xrefdline;       /* delay line reference signal */
    xcomplex ***Xfdline;    /* block delay line for filter inputs in frequency domain */
    xcomplex ***Xfbdline;   /* block delay line for partitioned block adaptive filter input */
    float **Xdline;         /* block delay line for filter inputs in time domain */
    xcomplex **Xffilt;      /* adaptive filter input signal */
    xcomplex *yftmp; /* temporary vector in frequency domain */
    float *ytmp;     /* temporary vector in time domain */
    xcomplex *yhf;   /* adaptive filter output in frequency domain */
    xcomplex ***Hf;  /* adaptive filter coefficients frequency domain */
    float *e;        /* error signal in time domain */
    float *z;        /* time-domain aic output signals of internal processing in processOneDataBlock() */
    xcomplex *ef;    /* error signal in frequency domain */
    float *pXf;      /* instantaneous power estimate of adaptive filter input */
    float *sftmp;    /* temporary variable for recursive power estimation in frequency domain */
    float *sf;       /* power estimate of adaptive filter input in frequency domain */
    xcomplex *muf;   /* normalized stepsize in frequency domain */
    xcomplex *nuf;   /* frequency-domain forgetting factor for adaptive filter coefficients */
    float maxnorm;   /* maximally allowed filter norm, used by norm constraint */

    void *aic_FFT;
    float *fft_out;
    float *fft_in;
	
}objFGSCaic;

/**********************************************************************************
Function:      // dios_ssp_gsc_gscaic_init
Description:   // adaptive interference canceller init
Input:         // dlysync: delay which is required for synchronization of the 
                            adaptive blocking matrix output with the reference 
                            signal xref
                    num_mic: number of microphones
                    fft_size: length of FFT, must be power of 2
                    maxNorm: specifies the maximum filter norm, used for the filter 
                            norm constraint against desired signal cancellation 
                    forgetfactor: forgetting factor for recursive power estimation for 
                                normalizing the stepsize of the adaptation algorithm
                                0 < forgetfactor < 1
                    stepsize: stepsize of the adaptation algorithm, 0 < stepsize < 2
                    thresConDiv0: static threshold value which prevents division by 0 when
                                normalizing the stepsize
                    thresDynDiv0: dynamic threshold value which prevents division by 0
                                if the estimated signal power sf = 0
                    lobeDynDiv0: parameter which specifies the lobe of the dynamic threshold 
                                if sf > 0
                    useDynRegularization: specifies whether static or dynamic 
                                        regularization is used.
                                        0 for static regularization
                                        1 for dynamic regularization
                    num_taps: number of the filter taps for each adaptive filter
                            must be a power of 2
                            if num_taps is greater than fft_size, then the partitioned 
                            block algorithm is used
                    overlap_fft: overlap factor of aic internal signals
                    overlap_sigs: overlap factor of input signals
                    rate: sampling rate
                    tconst_freezing: time constant (in seconds) for decreasing weights 
                                    of adaptive filters, prevents freezing of adaptive 
                                    filters and reduces desired signal cancellation 
                                    after long periods with desired signal presence.
Output:        // none
Return:        // success: return gscaic object pointer
Others:        // none
**********************************************************************************/
void dios_ssp_gsc_gscaic_init(objFGSCaic *gscaic, int dlysync, int num_mic, int fft_size, float maxNorm, 
            float forgetfactor, float stepsize, float thresConDiv0, 
            float thresDynDiv0, float lobeDynDiv0, int useDynRegularization, 
            int num_taps, int overlap_fft, int overlap_sigs, long rate, 
            float tconst_freezing);

/**********************************************************************************
Function:      // dios_ssp_gsc_gscaic_reset
Description:   // adaptive interference canceller reset
Input:         // gscaic: adaptive interference canceller object pointer
Output:        // none
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_gscaic_reset(objFGSCaic *gscaic);

/**********************************************************************************
Function:      // dios_ssp_gsc_gscaic_resetfilterbank
Description:   // adaptive interference canceller reset filter bank
Input:         // gscaic: adaptive interference canceller object pointer
Output:        // none
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_gscaic_resetfilterbank(objFGSCaic *gscaic);

/**********************************************************************************
Function:      // dios_ssp_gsc_gscaic_process
Description:   // processing of adaptive interference canceller
Input:         // xref: vector of time-domain reference signal
                        vector length fft_size / (2 * fft_overlap)
                    X: matrix of time-domain filter input signals 
                        (adaptive blocking matrix output)
                        number of rows: num_mic
                        number of columns: fft_size / (2 * fft_overlap)
                    y: vector with time-domain output signal
                        vector of length fft_size / (2 * fft_overlap)
                    ctrl_abm: adaptation control output signal in frequency domain 
                            for adaptive blocking matrix
                    ctrl_aic: adaptation control output signal in frequency domain
                            for adaptive interference canceller
Output:        // none
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_gscaic_process(objFGSCaic *gscaic, float *xref, float **X, float *y, float *ctrl_abm, float *ctrl_aic);

/**********************************************************************************
Function:      // dios_ssp_gsc_gscaic_processonedatablock
Description:   // adaptive interference canceller process submodule
Input:         // gscaic: adaptive interference canceller object pointer
Output:        // none
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_gscaic_processonedatablock(objFGSCaic *gscaic, float *ctrl_abm, float *ctrl_aic);

/**********************************************************************************
Function:      // dios_ssp_gsc_gscaic_delete
Description:   // adaptive interference canceller delete
Input:         // gscaic: adaptive interference canceller object pointer
Output:        // none
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_gscaic_delete(objFGSCaic *gscaic);

#endif  /* _DIOS_SSP_GSC_AIC_H_ */
