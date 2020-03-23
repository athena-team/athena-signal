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

#ifndef _DIOS_SSP_GSC_FILTSUMBEAMFORMER_H_
#define _DIOS_SSP_GSC_FILTSUMBEAMFORMER_H_

#include "dios_ssp_gsc_dsptools.h"
#include "../dios_ssp_share/dios_ssp_share_complex_defs.h"
#include "../dios_ssp_share/dios_ssp_share_rfft.h"

typedef struct
{
	int nmic;        /* number of microphones */
	int fftlength;   /* FFT size */
	int fftoverlap;  /* overlap factor of FFT */
	int filtord;     /* filter order */
	float **Xdline;  /* delayline for FBF inputs in time domain */
	xcomplex *xftmp; /* temporary buffer in frequency domain */
	float *ytmp;     /* temporary buffer in time domain */
	xcomplex *yftmp; /* temporary buffer in frequency domain */

	void *filt_FFT;
	float *fft_out;
	float *fft_in;
	
}objFGSCfiltsumbeamformer;

/**********************************************************************************
Function:      // dios_ssp_gsc_gscfiltsumbeamformer_init
Description:   // fixed beamformer init
Input:         // gscfiltsumbeamformer: fixed beamformer object pointer
Output:        // none
Return:        // success: return gscfiltsumbeamformer
Others:        // none
**********************************************************************************/
void dios_ssp_gsc_gscfiltsumbeamformer_init(objFGSCfiltsumbeamformer* gscfiltsumbeamformer, int num_mic, int fft_size, int fft_overlap);

/**********************************************************************************
Function:      // dios_ssp_gsc_gscfiltsumbeamformer_reset
Description:   // fixed beamformer reset
Input:         // gscfiltsumbeamformer: fixed beamformer object pointer
Output:        // none
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_gscfiltsumbeamformer_reset(objFGSCfiltsumbeamformer* gscfiltsumbeamformer);

/**********************************************************************************
Function:      // dios_ssp_gsc_gscfiltsumbeamformer_process
Description:   // do filter-and-sum processing
Input:         // X: matrix of time-domain filter input signals
				  columns: num_mic
				  rows: fftlength / (2 * fftoverlap)
				  y: vector of time-domain output signal
				  index: row index of input signal
Output:        // none
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_gscfiltsumbeamformer_process(objFGSCfiltsumbeamformer* gscfiltsumbeamformer, float **X, float *y, int index);

/**********************************************************************************
Function:      // dios_ssp_gsc_gscfiltsumbeamformer_delete
Description:   // fixed beamformer delete
Input:         // gscfiltsumbeamformer: fixed beamformer object pointer
Output:        // none
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_gscfiltsumbeamformer_delete(objFGSCfiltsumbeamformer* gscfiltsumbeamformer);

#endif /* _DIOS_SSP_GSC_FILTSUMBEAMFORMER_H_ */

