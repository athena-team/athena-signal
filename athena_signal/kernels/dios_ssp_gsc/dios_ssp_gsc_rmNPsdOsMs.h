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

#ifndef _DIOS_SSP_GSC_RMNPSDOSMS_H_
#define _DIOS_SSP_GSC_RMNPSDOSMS_H_

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "dios_ssp_gsc_globaldefs.h"

static const float actmin_max = 10.0f;
static const float epsilon = 10e-10f;

typedef struct
{
    float m_fs;			/* sampling rate of the input signal */
	int	m_L;			/* size of spectrum: FFT size or complex spectral size */
	int	m_R;			/* length of overlap of the input signals */
	int	m_U;			/* number of subwindows */
	int	m_V;			/* length of one subwindow */
	int	m_D;			/* length of the estimation window */
	float m_alpha_max;	/* upper limit for alpha(k) */
	float m_alpha_min;	/* lower limit for alpha(k) */
	float m_av;			/* parameter directly affecting Bc and therefore Bias */

	float m_M;			/* value taken from Rainer Martin to calculate Bias */
	float m_MSub;		
	
	float m_ALPHAc;		/* some constaints used to determine Aplha */
	
	float m_Bc;			/* value that enhances the Bias */
	float m_noise_slope_max;  /* value by which the maximum rise of the overall 
                                 noise level is determined */

	int	m_subwc;		/* counter for the samples in one subwindows */
	int	m_Ucount;		/* counter of the subwindows */

	int	m_SNRcount;		/* counter for a new SNR estimation */
	float m_sumP;		/* sums up P over all frequency bins to estimate SNR */
	float m_sumN;		/* sums up N over all frequency bins to estimate SNR */
	float m_SNR;		/* estimation of the overall signal to noise ratio */

	/* output */
	float* m_P;			/* smoothed power P(k) */
	float* m_N;			/* noise estimation */
	float* m_alpha;		/* smoothing parameter */

	float* m_temp;

	/* for Bias correction */
	float* m_beta;		/* smoothing parameter needed for Bias Compensation */
	float* m_P1m;		/* mean of the first moment of P */
	float* m_P2m;		/* mean of the second moment of P^2 */
	float* m_varP;		/* estimation of the variance needed for Bias Compensation */
	float* m_Qeq;		/* needed for Bias Compensation */
	float* m_sQeq;		/* needed for Bias Compensation */
	float* m_sQeqSub;	/* needed for Bias Compensation */
	float* m_Bmin;		/* Bias for D samples */
	float* m_Bmin_sub;	/* Bias for a subwindow of length V */

	/* for finding minimum */
	float* m_k_mod;		/* set, if Actmin is changed (SHOULD BE BOOLEAN) */
	float* m_lmin_flag;	/* set, if a possible local minimum is found (SHOULD BE BOOLEAN) */
	float* m_actmin;	/* actual minimum in D samples for each frequency bin */
	float* m_actmin_sub;  /* actual minimum in a subwindow for each frequency bin */
	float* m_Pmin_u;	/* minimum of the last U subwindows for each frequency bin */
	float** m_store;	/* matrix that stores the last U minima of Actmin for all frequency bins */	

}objCNPsdOsMs;

/**********************************************************************************
Function:      // dios_ssp_gsc_rmnpsdosms_init
Description:   // noise power spectrum density estimation initialization
Input:         // fs: sampling rate
					L: FFT size or complex spectral size
					R: FFT size/overlap
					U: Number of subwindows
					V: Length of one subwindow; U * V < 160
					alpha_max: Upper limit for alpha(k)
					alpha_min: Maximum lower limit for alpha(k)
					av: factor for calculation of Bc(lamdba)
Output:        // none
Return:        // success return npsdosms1 object pointer 
Others:        // none
**********************************************************************************/
void dios_ssp_gsc_rmnpsdosms_init(objCNPsdOsMs *npsdosms1, float fs, int L, int R,	int U, int V);

/**********************************************************************************
Function:      // dios_ssp_gsc_rmnpsdosms_reset
Description:   // reset
Input:         // npsdosms1: npsdosms1 object pointer
Output:        // none
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_rmnpsdosms_reset(objCNPsdOsMs *npsdosms1);

/**********************************************************************************
Function:      // dios_ssp_gsc_rmnpsdosms_process
Description:   // process one blcok of input PSD
Input:         // pPSDInput: input PSD |Y(k)|^2 
Output:        // none
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_rmnpsdosms_process(objCNPsdOsMs *npsdosms1, float *pPSDInput);

/**********************************************************************************
Function:      // dios_ssp_gsc_rmnpsdosms_delete
Description:   // delete
Input:         // npsdosms1: npsdosms1 object pointer
Output:        // none
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_rmnpsdosms_delete(objCNPsdOsMs *npsdosms1);

#endif  /* _DIOS_SSP_GSC_RMNPSDOSMS_H_ */
