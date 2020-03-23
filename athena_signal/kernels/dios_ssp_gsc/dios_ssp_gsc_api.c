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

Description: This is a Generalized Sidelobe Canceller beamformer.
When the DOA module is disabled, you can set the steering vector(loc_phi) by
yourself, which indicates the distortionless response direction.
When the DOA module is enabled, the steering vector will be estimated by DOA
estimation. You can set the steering vector by your own DOA estimation method.
Microphone array could be any shape as long as you set the coordinates of each 
microphones(mic_coord) beforehand.
==============================================================================*/

#include "dios_ssp_gsc_api.h"

void* dios_ssp_gsc_init_api(int mic_num, void* mic_coord)
{
	void* st = NULL;
	st = (void*)calloc(1, sizeof(objGSCProcess));
	objGSCProcess* ptr = (objGSCProcess*)st;

    PlaneCoord* mic_coord_1 = NULL;
	mic_coord_1 = (PlaneCoord*)mic_coord;
	dios_ssp_gsc_init(ptr, mic_num, mic_coord_1);

	return st;
}

int dios_ssp_gsc_reset_api(void* ptr)
{
	if(ptr == NULL)
	{
		printf("gsc handle not init!\n");
		return ERROR_GSC;
	}

	objGSCProcess* ptr_gsc;
	ptr_gsc = (objGSCProcess*)ptr;
	dios_ssp_gsc_reset(ptr_gsc);

	return 0;
}

int dios_ssp_gsc_process_api(void* ptr, float* mic_data, float* out_data, float loc_phi)
{
	if(ptr == NULL)
	{
		printf("gsc handle not init!\n");
		return ERROR_GSC;
	}
	objGSCProcess *ptr_gsc;
	ptr_gsc= (objGSCProcess*)ptr;

	for (int i = 0; i < ptr_gsc->mic_num; i++)
	{
		memcpy(ptr_gsc->ptr_input_data_float[i], mic_data + i * ptr_gsc->frame_len, sizeof(float) * ptr_gsc->frame_len);
	}
	ptr_gsc->source_location.phi = loc_phi * PI / 180.0f;
	doProcess(ptr_gsc);
	memcpy(out_data, ptr_gsc->ptr_output_data_float, sizeof(float) * ptr_gsc->frame_len);

	return 0;
}

int dios_ssp_gsc_uninit_api(void* ptr)
{
	if(ptr == NULL)
	{
		printf("gsc handle not init!\n");
		return ERROR_GSC;
	}
	objGSCProcess *ptr_gsc;
	ptr_gsc= (objGSCProcess*)ptr;
	dios_ssp_gsc_delete(ptr_gsc);
	return 0;
}
