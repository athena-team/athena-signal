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

Description: Voice Activity Detection(VAD) function outputs the current frame
speech state based on the result of the double-talk detection.
1. Set initial parameters based on double-talk results
2. Calculate input signal energy
3. Calculate the number of speech frames based on double-talk result and energy
4. Smooth VAD results with hangpover
==============================================================================*/

#include "dios_ssp_vad_api.h"

typedef struct {
	void* energy_vad_ptr;
	void* vad_counter_ptr;
	void* vad_counter_stric_ptr;
	int voice_state;
	int vad_temp_result;
	int vad_result;
	int voice_stric_state;
	int vad_temp_stric_result;
	int vad_stric_result;
} objVadProcess;

void* dios_ssp_vad_init_api(void)
{
	void* ptr_vad = NULL;
	objVadProcess *srv;
	ptr_vad = (void*)calloc(1, sizeof(objVadProcess));
	srv = (objVadProcess *)ptr_vad;

    srv->energy_vad_ptr = dios_ssp_energy_vad_init(0);
	if(srv->energy_vad_ptr == NULL)
    {
		return NULL;
	}
	srv->vad_counter_ptr = dios_ssp_vad_counter_init();
	if(srv->vad_counter_ptr == NULL)
    {
		return NULL;
	}
	srv->vad_counter_stric_ptr = dios_ssp_vad_counter_init();
	if(srv->vad_counter_stric_ptr == NULL)
    {
		return NULL;
	}

	srv->voice_state = 0;
	srv->vad_temp_result = 0;
	srv->vad_result = 0;
	srv->voice_stric_state = 0;
	srv->vad_temp_stric_result = 0;
	srv->vad_stric_result = 0;

	return ptr_vad;
}

int dios_ssp_vad_reset_api(void* ptr_vad)
{
	if (ptr_vad == NULL) 
    {
		return ERROR_VAD;
	}
	objVadProcess *srv;
	srv = (objVadProcess *)ptr_vad;
	
	dios_ssp_energy_vad_reset(srv->energy_vad_ptr);
	dios_ssp_vad_counter_reset(srv->vad_counter_ptr);
	dios_ssp_vad_counter_reset(srv->vad_counter_stric_ptr);
	srv->voice_state = 0;
	srv->vad_temp_result = 0;
	srv->vad_result = 0;
	srv->voice_stric_state = 0;
	srv->vad_temp_stric_result = 0;
	srv->vad_stric_result = 0;

	return 0;
}

int dios_ssp_vad_process_api(void* ptr_vad, float* vad_data, int dt_st)
{
	if (ptr_vad == NULL || vad_data == NULL) 
    {
		return ERROR_VAD;
	}
	objVadProcess *srv;
	srv = (objVadProcess *)ptr_vad;

    // different param according to doubletalk status
	if(dt_st == 0)
    {
		dios_ssp_energy_vad_para_set(srv->energy_vad_ptr, singletalk_state, srv->voice_state);
	}
    else if(dt_st == 1)
    {
		dios_ssp_energy_vad_para_set(srv->energy_vad_ptr, nearend_state, srv->voice_state);
	}
    else
    {
		dios_ssp_energy_vad_para_set(srv->energy_vad_ptr, doubletalk_state, srv->voice_state);
	}

	// energy vad processing
	int energy_vad_result = 0;
	int energy_vad_stric_result = 0;
	energy_vad_result = dios_ssp_energy_vad_process(srv->energy_vad_ptr, vad_data);
	energy_vad_stric_result = dios_ssp_energy_vad_stric_result_get(srv->energy_vad_ptr);
	float signal_dev_value;
	float noiselevel_second;
	float noiselevel_first;
	float vmmean;
	dios_ssp_energy_vad_para_get(srv->energy_vad_ptr, &signal_dev_value, &noiselevel_second, &noiselevel_first, &vmmean);

	// mix vad processing
	dios_ssp_vad_mix_process(energy_vad_result, dt_st, srv->vad_counter_ptr, signal_dev_value, &srv->vad_temp_result, srv->voice_state);  
	dios_ssp_vad_mix_stric_process(energy_vad_stric_result, dt_st, srv->vad_counter_stric_ptr, signal_dev_value, &srv->vad_temp_stric_result, srv->voice_stric_state);  

	// smooth the vad result
	srv->vad_result = srv->vad_temp_result;
	dios_ssp_vad_smooth(&srv->vad_result, srv->vad_counter_ptr, &srv->voice_state);
	srv->vad_stric_result = srv->vad_temp_stric_result;
	dios_ssp_vad_smooth(&srv->vad_stric_result, srv->vad_counter_stric_ptr, &srv->voice_stric_state);

	return 0;
}

int dios_ssp_vad_result_get(void* ptr_vad)
{
	if (ptr_vad == NULL) 
    {
		return ERROR_VAD;
	}
	objVadProcess *srv;
	srv = (objVadProcess *)ptr_vad;

	return srv->vad_result;
}

int dios_ssp_vad_uninit_api(void* ptr_vad)
{
	if (ptr_vad == NULL) 
    {
		return ERROR_VAD;
	}
	objVadProcess *srv;
	srv = (objVadProcess *)ptr_vad;

	dios_ssp_energy_vad_uninit(srv->energy_vad_ptr);
	dios_ssp_vad_counter_uinit(srv->vad_counter_ptr);
	dios_ssp_vad_counter_uinit(srv->vad_counter_stric_ptr);
	free(srv);

	return 0;
}

