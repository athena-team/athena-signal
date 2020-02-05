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

#ifndef _DIOS_SSP_VAD_ENERGY_H_
#define _DIOS_SSP_VAD_ENERGY_H_

enum TALK_STATE 
{
	doubletalk_state,
	singletalk_state,
	nearend_state
};

/**********************************************************************************
Function:      // dios_ssp_energy_vad_init
Description:   // init energy vad module
Input:         // ptr: vad_type
Output:        // none
Return:        // success: return energy vad module pointer
                  failure: return NULL
**********************************************************************************/
void* dios_ssp_energy_vad_init(int vad_type);

/**********************************************************************************
Function:      // dios_ssp_energy_vad_reset
Description:   // reset energy vad module
Input:         // ptr: vad_handle
Output:        // none
Return:        // success: return energy vad module pointer
                  failure: return NULL
**********************************************************************************/
void dios_ssp_energy_vad_reset(void* vad_handle);

/**********************************************************************************
Function:      // dios_ssp_energy_vad_para_set
Description:   // set energy vad param
Input:         // ptr: vad_handle
				  talk_state
				  state
Output:        // none
Return:        // success: return energy vad module pointer
                  failure: return NULL
**********************************************************************************/
void dios_ssp_energy_vad_para_set(void* vad_handle, enum TALK_STATE talk_state, int state);

/**********************************************************************************
Function:      // dios_ssp_energy_vad_process
Description:   // energy vad process
Input:         // ptr: vad_handle
				  inbuf
Output:        // none
Return:        // success: return energy vad flag
                  failure: return NULL
**********************************************************************************/
int dios_ssp_energy_vad_process(void* vad_handle, float *inbuf);

/**********************************************************************************
Function:      // dios_ssp_energy_vad_stric_result_get
Description:   // stric energy vad result
Input:         // ptr: energyvad_ptr
Output:        // none
Return:        // success: return stric energy vad result
                  failure: return NULL
**********************************************************************************/
int dios_ssp_energy_vad_stric_result_get(void* energyvad_ptr);

/**********************************************************************************
Function:      // dios_ssp_energy_vad_para_get
Description:   // stric energy vad result
Input:         // ptr: energyvad_ptr
				  noisedev
				  noiselevel_second
				  noiselevel_first
				  vmean
Output:        // none
Return:        // success: return NULL
                  failure: return NULL
**********************************************************************************/
void dios_ssp_energy_vad_para_get(void* energyvad_ptr, float *noisedev, float *noiselevel_second, float *noiselevel_first, float *vmean);

/**********************************************************************************
Function:      // dios_ssp_energy_vad_uninit
Description:   // free energy vad module
Input:         // ptr: vad_handle
Output:        // none
Return:        // success: return NULL
                  failure: return NULL
**********************************************************************************/
void dios_ssp_energy_vad_uninit(void* vad_handle);

#endif

