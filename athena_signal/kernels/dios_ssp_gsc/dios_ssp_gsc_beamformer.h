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

#ifndef _DIOS_SSP_GSC_BEAMFORMER_H_
#define _DIOS_SSP_GSC_BEAMFORMER_H_

#include "dios_ssp_gsc_dsptools.h"
#include "dios_ssp_gsc_globaldefs.h"
#include "dios_ssp_gsc_beamsteering.h"
#include "dios_ssp_gsc_filtsumbeamformer.h"
#include "dios_ssp_gsc_abm.h"
#include "dios_ssp_gsc_aic.h"
#include "dios_ssp_gsc_adaptctrl.h"
#include "../dios_ssp_share/dios_ssp_share_typedefs.h"

typedef struct
{
	float **m_input;
	float **m_outSteering;
	float *m_outFBF;
	float *m_ctrlABM;
	float *m_ctrlAIC;
	float **m_outABM;
	float *m_outAIC;
	float *m_output;

	int m_nMic;
	int m_nIOBlockSize;
	DWORD m_dwSampRate;
	float m_time;
	int m_nInputChannels;
	int m_nGSCUpdateSize;
	int m_nCCSSize;

	PlaneCoord *m_locMic;
	float *m_tdoa;
	float m_current_phi;
	float m_phi_thr;
	float m_current_abm_phi;
	float m_abm_phi_thr;

	float m_soft_vol;
	float m_alpha_mute;
	float m_alpha_active;

	GSCFDAF m_paramGSC;        /* parameters for gsc frequency-domain adaptive filters */
	ADAPTCTRL m_paramAC;       /* parameters for adaptation control */
	ABMPARAMETERS m_paramABM;  /* parameters for adaptive blocking matrix */
	AICPARAMETERS m_paramAIC;  /* parameters for adaptive interference canceller */
 	SYNCDELAYS m_paramSync;    /* parameters for synchronization delays */

	objCGSCbeamsteer *gscbeamsteer;						/* declaration of beamsteering */
	objFGSCfiltsumbeamformer *gscfiltsumbeamformer;		/* declaration of fixed beamformer */
	objFGSCabm *gscabm;									/* declaration of adaptive blocking matrix */
	objFGSCaic *gscaic;									/* declaration of adaptive interference canceller */
	objFGSCadaptctrl *gscadaptctrl;						/* declaration of gsc adaptation control */
	
}objCGSCbeamformer;

/**********************************************************************************
Function:      // dios_ssp_gsc_gscbeamformer_init
Description:   // beamformer init
Input:         // gscbeamformer: gscbeamformer object pointer
                  nMic: microphone number
                  coord: each microphone coordinate (PlaneCoord*)mic_coord
Output:        // none
Return:        // success: return gscbeamformer object pointer
Others:        // none
**********************************************************************************/
void dios_ssp_gsc_gscbeamformer_init(objCGSCbeamformer* gscbeamformer, DWORD nMic, DWORD dwSampRate, DWORD dwBlockSize, General_ArrayGeometric type, void *coord);

/**********************************************************************************
Function:      // dios_ssp_gsc_gscbeamformer_reset
Description:   // beamformer reset
Input:         // gscbeamformer: gscbeamformer object pointer
Output:        // none
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_gscbeamformer_reset(objCGSCbeamformer* gscbeamformer);

/**********************************************************************************
Function:      // dios_ssp_gsc_gscbeamformer_arraysteer
Description:   // beamformer steering
Input:         // prototype: pSrcLoc, polar coordinates of source locations
Output:        // none
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_gscbeamformer_arraysteer(objCGSCbeamformer* gscbeamformer, PolarCoord loc);

/**********************************************************************************
Function:      // dios_ssp_gsc_gscbeamformer_process
Description:   // process data of one block
Input:         // prototype: pInput, float data, multi-channel, continuous memory
Output:        // none
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_gscbeamformer_process(objCGSCbeamformer* gscbeamformer, float** ppInput);

/**********************************************************************************
Function:      // dios_ssp_gsc_gscbeamformer_delete
Description:   // beamformer delete
Input:         // gscbeamformer: gscbeamformer object pointer
Output:        // none
Return:        // success: return 0
Others:        // none
**********************************************************************************/
int dios_ssp_gsc_gscbeamformer_delete(objCGSCbeamformer* gscbeamformer);

#endif /* _DIOS_SSP_GSC_BEAMFORMER_H_ */
