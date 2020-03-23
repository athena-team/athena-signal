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

#ifndef	 _DIOS_SSP_GSC_FIRFILTERDESIGN_H_
#define	 _DIOS_SSP_GSC_FIRFILTERDESIGN_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dios_ssp_gsc_globaldefs.h"
#include "../dios_ssp_share/dios_ssp_share_complex_defs.h"
#include "../dios_ssp_share/dios_ssp_share_rfft.h"

typedef struct
{
	int	m_nFIRLen;
	int m_nFFTLen;
	int m_nFFTOrder;
	int m_nCSize;

	float *m_pTapsBuf;
	xcomplex *m_pTapsFreq;
	float *m_mag;
	float *m_phase;
	float *m_win;

	void *gengralfir_FFT;
	float *fft_out;
	float *fft_in;
	
}objCGeneralFIRDesigner;

/**********************************************************************************
Function:      // dios_ssp_gscfirfilterdesign_init
Description:   // fir filter design init
Input:         // generalfirdesign: generalfirdesign object pointer
Output:        // none
Return:        // success: return generalfirdesign object pointer
Others:        // none
**********************************************************************************/
void dios_ssp_gscfirfilterdesign_init(objCGeneralFIRDesigner *generalfirdesign, int len, General_WindowType winType);

/**********************************************************************************
Function:      // dios_ssp_gscfirfilterdesign_fractionaldelay
Description:   // do FIR filtering directly, not using FIRState struct
Input:         // pIn: input signal
                  dwBaseSize: length of input signal
                  FIRTaps: FIR filter
                  dwFIRTaps: length of FIR filter
                  pDlyLine: signal data buffer including old and new samples
                  DlyLineIndex: the beginning or endding position
Output:        // pOut: filtered output signal 
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gscfirfilterdesign_fractionaldelay(objCGeneralFIRDesigner *generalfirdesign, float fcLow, float fcHigh, float delay, float *pTaps);

/**********************************************************************************
Function:      // dios_ssp_gscfirfilterdesign_delete
Description:   // fir filter design delete
Input:         // generalfirdesign: generalfirdesign object pointer
Output:        // none
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gscfirfilterdesign_delete(objCGeneralFIRDesigner *generalfirdesign);

#endif /* _DIOS_SSP_GSC_FIRFILTERDESIGN_H_ */
