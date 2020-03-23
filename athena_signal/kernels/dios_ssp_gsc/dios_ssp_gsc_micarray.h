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

#ifndef  _DIOS_SSP_GSC_MICARRAY_H_
#define  _DIOS_SSP_GSC_MICARRAY_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "dios_ssp_gsc_beamformer.h"
#include "dios_ssp_gsc_globaldefs.h"
#include "dios_ssp_gsc_multigscbeamformer.h"

typedef struct
{
	int nfs;
	int frame_len;
	int mic_num;
	int outbeam_num;
	int loca_use_flag;
	float wakeup_loca;
	PlaneCoord* ptr_mic_coord;
	
	/* input parameter and signal */
	PolarCoord source_location;
	float** ptr_input_data_float;

	/* output signal */
	short* ptr_output_data_short;
	float* ptr_output_data_float;
	
	General_ArrayGeometric type;
	objCMultiGSCbeamformer *multigscbeamformer;

}objGSCProcess;

/**********************************************************************************
Function:      // dios_ssp_gsc_init
Description:   // gsc init
Input:         // mic_num: microphone number
                  mic_coord: each microphone coordinate (PlaneCoord*)mic_coord
Output:        // none
Return:        // success: return gsc object pointer (void*)ptr_gsc
                  failure: return NULL
Others:        // none
**********************************************************************************/
void dios_ssp_gsc_init(objGSCProcess *ptr_gsc, int mic_num, PlaneCoord* mic_coord);

/**********************************************************************************
Function:      // dios_ssp_gsc_reset
Description:   // gsc reset
Input:         // ptr: gsc object pointer
Output:        // none
Return:        // success: return gsc object pointer (void*)ptr_gsc
Others:        // none
**********************************************************************************/
void dios_ssp_gsc_reset(objGSCProcess *ptr_gsc);

/**********************************************************************************
Function:      // doProcess
Description:   // gsc process
Input:         // ptr: gsc object pointer
Return:        // success: return gsc object pointer (void*)ptr_gsc
Others:        // none
**********************************************************************************/
void doProcess(objGSCProcess *ptr_gsc);

/**********************************************************************************
Function:      // dios_ssp_gsc_delete
Description:   // gsc delete
Input:         // ptr: gsc object pointer
Output:        // none
Return:        // success: return gsc object pointer (void*)ptr_gsc
Others:        // none
**********************************************************************************/
void dios_ssp_gsc_delete(objGSCProcess *ptr_gsc);

#endif /* _DIOS_SSP_GSC_MICARRAY_H_ */
