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

#ifndef _DIOS_SSP_API_H_
#define _DIOS_SSP_API_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./dios_ssp_hpf/dios_ssp_hpf_api.h"
#include "./dios_ssp_vad/dios_ssp_vad_api.h"
#include "./dios_ssp_aec/dios_ssp_aec_api.h"
#include "./dios_ssp_ns/dios_ssp_ns_api.h"
#include "./dios_ssp_agc/dios_ssp_agc_api.h"
#include "./dios_ssp_mvdr/dios_ssp_mvdr_api.h"
#include "./dios_ssp_doa/dios_ssp_doa_api.h"
#include "./dios_ssp_gsc/dios_ssp_gsc_api.h"

typedef struct 
{
	short AEC_KEY;
    short NS_KEY;
    short AGC_KEY;
    short HPF_KEY;
    short BF_KEY;
    short DOA_KEY;

    int mic_num;
    int ref_num;
    PlaneCoord mic_coord[16];
} objSSP_Param;
    
/**********************************************************************************
Function:      // dios_ssp_init_api
Description:   // init with SSP_PARAM and allocate memory
Input:         // SSP_PARAM: object of SSP with necessary parameters
Output:        // none
Return:        // success: return dios speech signal process pointer
                  failure: return NULL
**********************************************************************************/
void* dios_ssp_init_api(objSSP_Param *SSP_PARAM);
	
/**********************************************************************************
Function:      // dios_ssp_reset_api
Description:   // reset dios speech signal process module
Input:         // ptr: dios speech signal process pointer
                  SSP_PARAM:
Output:        // none
Return:        // success: return OK_AUDIO_PROCESS, failure: return others
**********************************************************************************/
int dios_ssp_reset_api(void* ptr, objSSP_Param *SSP_PARAM);

/**********************************************************************************
Function:      // dios_ssp_process_api
Description:   // run dios speech signal process module by frames 
Input:         // ptr: dios speech signal process pointer
                  mic_buf: microphone array data buffer
                  ref_buf: reference data buffer
                           for mono-channel, the length of ref_buf is 128
                           for stereo-channel, the length of ref_buf is 128 * 2,
                           [0 ~ 127] is from left channel
                           [128 ~ 255] is from right channel
                  SSP_PARAM:
Output:        // out_buf: processed data
Return:        // success: return OK_AUDIO_PROCESS, failure: return others
**********************************************************************************/
int dios_ssp_process_api(void* ptr, short* mic_buf, short* ref_buf, 
            short* out_buf, objSSP_Param *SSP_PARAM);

/**********************************************************************************
Function:      // dios_ssp_uninit_api
Description:   // free dios speech signal process module
Input:         // ptr: dios speech signal process pointer
                  SSP_PARAM:
Output:        // none
Return:        // success: return OK_AUDIO_PROCESS, failure: return others
**********************************************************************************/
int dios_ssp_uninit_api(void* ptr, objSSP_Param *SSP_PARAM);

#endif  /* _DIOS_SSP_API_H_ */

