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

#ifndef _DIOS_SSP_GSC_BEAMSTEERING_H_
#define _DIOS_SSP_GSC_BEAMSTEERING_H_

#include "dios_ssp_gsc_firfilterdesign.h"
#include "dios_ssp_gsc_dsptools.h"

typedef struct
{
	int m_nMic;            /* number of microphones */
	DWORD m_dwKernelRate;  /* sampling rate */
	int m_nTaps;           /* number of taps to use */
	int m_nBlockSize;      /* size of one block in samples */
	float *m_delays;       /* vector with intersensor delays */
	float **m_pDlyLine;
	float **m_pTaps;
	int *m_nDlyLineIndex;

	objCGeneralFIRDesigner *generalfirdesign;
	
}objCGSCbeamsteer;

/**********************************************************************************
Function:      // dios_ssp_gsc_gscbeamsteer_init
Description:   // beamsteering process init
Input:         // gscbeamsteer: gscbeamsteer object pointer
				  nMic: number of microphones
				  nBlockSize: size of one block in samples
				  dwKernelRate: sampling rate
				  nTaps: number of taps to use
Return:        // success: return gscbeamsteer object pointer
Others:        // none
**********************************************************************************/
void dios_ssp_gsc_gscbeamsteer_init(objCGSCbeamsteer* gscbeamsteer, int nMic, int nBlockSize, DWORD dwKernelRate, int nTaps);

/**********************************************************************************
Function:      // dios_ssp_gsc_gscbeamsteer_reset
Description:   // beamsteering process reset
Input:         // gscbeamsteer: gscbeamsteer object pointer
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_gscbeamsteer_reset(objCGSCbeamsteer* gscbeamsteer);

/**********************************************************************************
Function:      // dios_ssp_gsc_gscbeamsteering
Description:   // prepare delay filter according to TDOA, stored in m_pTaps 
Input:         // gscbeamsteer: gscbeamsteer object pointer
				  delay_sample: TDOA
				  dwInputRate: sampling rate of input signal
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_gscbeamsteering(objCGSCbeamsteer* gscbeamsteer, float *delay_sample, DWORD dwInputRate);

/**********************************************************************************
Function:      // dios_ssp_gsc_gscbeamsteer_process
Description:   // do delay-filtering processing
Input:         // X: input signals
Output:        // Y: output delayed signals
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_gscbeamsteer_process(objCGSCbeamsteer* gscbeamsteer, float **X, float **Y);

/**********************************************************************************
Function:      // dios_ssp_gsc_gscbeamsteer_delete
Description:   // free the memory of doProcessing
Input:         // gscbeamsteer: gscbeamsteer object pointer
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_gscbeamsteer_delete(objCGSCbeamsteer* gscbeamsteer);

#endif /* _DIOS_SSP_GSC_BEAMSTEERING_H_ */
