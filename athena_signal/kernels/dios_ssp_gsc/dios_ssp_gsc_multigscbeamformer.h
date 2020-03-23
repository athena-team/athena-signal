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

#ifndef _DIOS_SSP_GSC_MULTIGSCBEAMFORMER_H_
#define _DIOS_SSP_GSC_MULTIGSCBEAMFORMER_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dios_ssp_gsc_globaldefs.h"
#include "dios_ssp_gsc_beamformer.h"
#include "dios_ssp_gsc_dsptools.h"
#include "dios_ssp_gsc_beamsteering.h"
#include "dios_ssp_gsc_filtsumbeamformer.h"
#include "dios_ssp_gsc_abm.h"
#include "dios_ssp_gsc_aic.h"
#include "dios_ssp_gsc_adaptctrl.h"
#include "../dios_ssp_share/dios_ssp_share_typedefs.h"

typedef struct
{
	DWORD m_nBeam;
	float **m_pOutput;
	objCGSCbeamformer *gscbeamformer;
	
}objCMultiGSCbeamformer;

/**********************************************************************************
Function:      // dios_ssp_gsc_multibeamformer_init
Description:   // multibeamformer init
Input:         // multigscbeamformer: multigscbeamformer object pointer
                  nMic: microphone number
                  mic_coord: each microphone coordinate (PlaneCoord*)mic_coord
Output:        // none
Return:        // success: return multigscbeamformer object pointer
Others:        // none
**********************************************************************************/
void dios_ssp_gsc_multibeamformer_init(objCMultiGSCbeamformer* multigscbeamformer, DWORD nMic, DWORD nBeam, DWORD dwSampRate, DWORD dwBlockSize, General_ArrayGeometric type, void *coord);

/**********************************************************************************
Function:      // dios_ssp_gsc_multibeamformer_reset
Description:   // multibeamformer reset
Input:         // multigscbeamformer: multigscbeamformer object pointer
Output:        // none
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_multibeamformer_reset(objCMultiGSCbeamformer* multigscbeamformer);

/**********************************************************************************
Function:      // dios_ssp_gsc_multibeamformer_arraysteer
Description:   // multibeamformer steering
Input:         // prototype: pSrcLoc, polar coordinates of source locations
Output:        // none
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_multibeamformer_arraysteer(objCMultiGSCbeamformer* multigscbeamformer, PolarCoord *pSrcLoc);

/**********************************************************************************
Function:      // dios_ssp_gsc_multibeamformer_process
Description:   // process data of one block
Input:         // prototype: pInput, float data, multi-channel, continuous memory
Output:        // none
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_multibeamformer_process(objCMultiGSCbeamformer* multigscbeamformer, float** ppInput);

/**********************************************************************************
Function:      // dios_ssp_gsc_multibeamformer_delete
Description:   // multibeamformer delete
Input:         // multigscbeamformer: multigscbeamformer object pointer
Output:        // none
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_multibeamformer_delete(objCMultiGSCbeamformer* multigscbeamformer);

#endif /* _DIOS_SSP_GSC_MULTIGSCBEAMFORMER_H_ */
