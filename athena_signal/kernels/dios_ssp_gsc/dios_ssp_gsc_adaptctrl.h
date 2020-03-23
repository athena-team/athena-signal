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

#ifndef _DIOS_SSP_GSC_ADAPTCTRL_H_
#define _DIOS_SSP_GSC_ADAPTCTRL_H_

#include "dios_ssp_gsc_globaldefs.h"
#include "dios_ssp_gsc_rmNPsdOsMs.h"
#include "dios_ssp_gsc_dsptools.h"
#include "../dios_ssp_share/dios_ssp_share_rfft.h"
#include "../dios_ssp_share/dios_ssp_share_complex_defs.h"

typedef struct
{
	WORD m_wNumMic;        /* number of microphones */
	DWORD m_dwSampRate;    /* sampling rate */
	DWORD m_dwFftSize;     /* FFT length */
	WORD m_wFftOverlap;    /* overlap factor of FFT */
	WORD m_wSyncDlyXref;   /* sync delay for reference signal */
	WORD m_wSyncDlyYfbf;   /* sync delay for FBF output */
	WORD m_wSyncDlyCtrlAic;  /* sync delay for AIC control signal */
	DWORD m_dwIndF0;       /* frequency bin number for f0 */
	DWORD m_dwIndF1;       /* frequency bin number for f1 */
	DWORD m_dwIndFc;       /* bin number correspoinding to fc */

	int m_nCCSSize;

	float m_corrThresAbm;    /* threshold correction for abm adaptation */
	float m_corrThresAic;    /* threshold correction for aic adaptation */
	float m_delta;           /* prevents divisions by zero */

	float *m_pfFftIn;        /* input buffer for fft */
	float **m_ppXrefDline;   /* delayline for reference signal synchronization im time domain */
	float *m_pXfbfDline;     /* delayline for fixed beamformer output */
	xcomplex **m_ppcfXref;   /* reference signals in the frequency domain */
	xcomplex *m_pcfXcfbf;    /* complementary fbf output in frequency domain */
	xcomplex *m_pcfXfbf;     /* FBF output in frequency domain */
	
	float *m_pfPref;  /* instantaneous power estimate of reference mic signals in frequency domain */
	float *m_pfPfbf;  /* instantaneous power estimate fixed beamformer output in frequency domain */
	float *m_pfPcfbf; /* instantaneous power estimate complementary beamformer in frequency domain */
	float *m_pfBeta;  /* decision variable for coherence update */
	float *m_pfBetaC; /* decision variable for coherence update */
	float **m_ppfCtrlAicDline;  /* delayline for synchronization of aic control signal */
	float *m_pfBuffer;

	objCNPsdOsMs *npsdosms1;
	objCNPsdOsMs *npsdosms2;

	void *adapt_FFT;
	float *fft_out;
	
}objFGSCadaptctrl;

/**********************************************************************************
Function:      // dios_ssp_gsc_gscadaptctrl_init
Description:   // initialize ABM/AIC control, set variables and allocate memory
Input:         // dwSampRate: sampling rate
					wNumMic: number of microphones
					wSyncDly****: delay which is required for synchronization of the 
								adaptation control output signal with the blocking
								matrix and adaptive interference canceller
					dwFftSize: length of FFT (must be power of 2)
					wFftOverlap: overlap factor of input signals (must be power of 2)
					dWF0: minimum frequency for energy ratio averaging (300Hz)
					dWF1: maximum frequency for energy ratio averaging (600Hz)
					dwFc: cutoff frequency for mean power estimation (300Hz)
					corrThres***: correction factor for ABM (0.8) and AIC (4.0)
					dwNumSubWindowsMinStat: number of sub-windows for minimum 
											statistics (8)
					dwSizeSubWindowsMinStat: size of sub-windows for minimum 
											statistics (18)
Output:        // none
Return:        // success: return adaptation control object pointer
Others:        // none
**********************************************************************************/
void dios_ssp_gsc_gscadaptctrl_init(objFGSCadaptctrl *gscadaptctrl, const DWORD dwSampRate, const WORD wNumMic, const WORD wSyncDlyXref, 
            const WORD wSyncDlyYfbf, const WORD wSyncDlyAic, const DWORD dwFftSize, 
            const WORD wFftOverlap, const DWORD dwF0, const DWORD dwF1, const DWORD dwFc, 
            const float corrThresAbm, const float corrThresAic, 
            const int dwNumSubWindowsMinStat, const int dwSizeSubWindowsMinStat);

/**********************************************************************************
Function:      // dios_ssp_gsc_gscadaptctrl_reset
Description:   // adaptation control reset
Input:         // gscadaptctrl: adaptation control object pointer
Output:        // none
Return:        // success: return 0 
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_gscadaptctrl_reset(objFGSCadaptctrl *gscadaptctrl);

/**********************************************************************************
Function:      // dios_ssp_gsc_gscadaptctrl_process
Description:   // processing of adaptation control
Input:         // pXfbf: time-domain output signal of FBF
							vector length dwFftSize / (2 * wFftOverlap)
					ppXref: time-domain reference signals for complementary FBF
							vector length dwFftSize / (2 * wFftOverlap)
Output:        // pfCtrlAbm: adaptation control output in frequency domain for ABM
								vector length dwFftSize / 2 + 1 with element '0' or '1'
								'0' means adaptation stalled
								'1' means adaptation performed
					pfCtrlAic: adaptation control output in frequency domain for AIC
								vector length dwFftSize / 2 + 1 with element '0' or '1'
								'0' means adaptation stalled
								'1' means adaptation performed
Return:        // success: return 0 
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_gscadaptctrl_process(objFGSCadaptctrl *gscadaptctrl, float *pXfbf, float **ppXref, const DWORD dwIndXref, 
            float *pfCtrlAbm, float *pfCtrlAic);

/**********************************************************************************
Function:      // dios_ssp_gsc_gscadaptctrl_delete
Description:   // adaptation control delete
Input:         // gscadaptctrl: adaptation control object pointer
Output:        // none
Return:        // success: return 0 
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_gscadaptctrl_delete(objFGSCadaptctrl *gscadaptctrl);

#endif /* _DIOS_SSP_GSC_ADAPTCTRL_H_ */

