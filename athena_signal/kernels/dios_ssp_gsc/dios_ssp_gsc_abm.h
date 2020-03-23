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

#ifndef _DIOS_SSP_GSC_ABM_H_
#define _DIOS_SSP_GSC_ABM_H_

#include "dios_ssp_gsc_dsptools.h"
#include "dios_ssp_gsc_globaldefs.h"
#include "../dios_ssp_share/dios_ssp_share_rfft.h"
#include "../dios_ssp_share/dios_ssp_share_complex_defs.h"

typedef struct
{
	int nmic;        /* number of microphones */
	int fftsize;     /* length of FFT */
	int fftoverlap;  /* overlap factor of FFT */
	int sigsoverlap; /* overlap factor of input signal segments */
	int syncdly;     /* synchronization delay */
	float lambda;    /* forgetting factor power estimation */
	float delta;     /* threshold factor for preventing division by zero */
	float mu;        /* stepsize adaptation algorithm */
	xcomplex nu;     /* forgetting factor for adaptive filter coefficients */
	int count_sigsegments;  /* counter for input signal segments */
	float **Xdline;  /* delayline of beamsteering output for causality */
	float *xrefdline;  /* delayline for adaptive filter input */
	xcomplex *xfref; /* frequency-domain adaptive filter input */
	xcomplex **hf;   /* adaptive filter transfer functions */
	float *ytmp;     /* temporary signal buffer in time domain */
	xcomplex *yftmp; /* temporary signal buffer in frequency domain */
	xcomplex *yf;    /* adaptive filter output in frequency domain */
	float *e;        /* time-domain error signal */
	float **E;  /* abm output signals of internal processing in processOneDataBlock() */
	xcomplex *ef;    /* frequency-domain error signal */
	xcomplex *muf;   /* normalized step size in frequency domain */
	xcomplex *nuf;   /* frequency-domain forgetting factor for adaptive filter coefficients */
	float *pxfref;   /* instantaneous power estimate of adaptive filter input */
	float **sf;
	float *pftmp;    /* temporary variable for calculation of normalized stepsize */
	float *m_upper_bound;
	float *m_lower_bound;
	void *abm_FFT;
	float *fft_out;
	float *fft_in;
	
}objFGSCabm;

/**********************************************************************************
Function:      // dios_ssp_gsc_gscabm_init
Description:   // constructor function
Input:         // num_mic: number of sensors
					fft_size: length of FFT, must be power of 2
					overlap_sigs: overlap factor of input signals 
					overlap_fft: overlap factor of abm internal signals
					dlysync: delay which is required for synchronization of the 
							adaptive blocking matrix inputs with the reference 
							signal xfref
					forgetfactor: forgetting factor for recursive power estimation for 
								normalizing the stepsize of the adaptation algorithm
								0 < forgetfactor < 1
					stepsize: stepsize of the adaptation algorithm, 0 < stepsize < 2
					threshdiv0: threshold value which prevents division by 0 when 
								normalizing the stepsize
					rate: sampling rate
					tconst_freezing: time constant (in seconds) for decreasing weights 
									of adaptive filters, prevents freezing of adaptive 
									filters and improves interference suppression 
									during periods without desired signal activity 
Output:        // none
Return:        // success: return adaptive blocking matrix object pointer
Others:        // none
**********************************************************************************/
void dios_ssp_gsc_gscabm_init(objFGSCabm *gscabm, int num_mic, int fft_size, int overlap_sigs, int overlap_fft, 
            int dlysync, float forgetfactor, float stepsize, 
            float threshdiv0, long rate, float tconst_freezing);

/**********************************************************************************
Function:      // dios_ssp_gsc_gscabm_initabmfreefield
Description:   // adaptive blocking matrix init
Input:         // gscabm: adaptive blocking matrix object pointer
Output:        // none
Return:        // success: return adaptive blocking matrix object pointer
Others:        // none
**********************************************************************************/			
void dios_ssp_gsc_gscabm_initabmfreefield(objFGSCabm *gscabm);

/**********************************************************************************
Function:      // dios_ssp_gsc_gscbeamformer_reset
Description:   // adaptive blocking matrix reset
Input:         // gscabm: adaptive blocking matrix object pointer
Output:        // none
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_gscabm_reset(objFGSCabm *gscabm);

/**********************************************************************************
    Function:      // dios_ssp_gsc_gscabm_process
    Description:   // processing of adaptive blocking matrix
    Input:         // X: matrix of time-domain sensor signals, number of rows: num_mic
                         number of columns: >= fft_size / (2 * fft_overlap)
                         see also index
                      xref: vector of time-domain reference signal
                            vector length fft_size / (2 * fft_overlap)
                      Y: matrix with time-domain output signals, matrix of dimension
                         num_mic * fft_size / (2 * fft_overlap)
                      ctrl_abm: adaptation control output signal in the frequency domain 
                                for ABM 
                      ctrl_aic: adaptation control output signal in the frequency domain 
                                for AIC
                      index: index into the matrix X if the number of columns of X is 
                             greater than fft_size / (2 * fft_overlap)
    Output:        // none
    Return:        // success: return 0
    Others:        // none
    **********************************************************************************/
int dios_ssp_gsc_gscabm_process(objFGSCabm *gscabm, float **X, float *xref, float **Y, float *ctrl_abm, float *ctrl_aic, int index);

/**********************************************************************************
Function:      // dios_ssp_gsc_gscabm_processonedatablock
Description:   // adaptive blocking matrix process submodule
Input:         // gscabm: adaptive blocking matrix object pointer
Output:        // none
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_gscabm_processonedatablock(objFGSCabm *gscabm, float *ctrl_abm, float *ctrl_aic);

/**********************************************************************************
Function:      // dios_ssp_gsc_gscabm_delete
Description:   // adaptive blocking matrix delete
Input:         // gscabm: adaptive blocking matrix object pointer
Output:        // none
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_gscabm_delete(objFGSCabm *gscabm);

#endif  /* _DIOS_SSP_GSC_ABM_H_ */

