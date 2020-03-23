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

Description: This is a Minimum Variance Distortionless Response beamformer.
When the DOA module is disabled, you can set the steering vector(loc_phi) by
yourself, which indicates the distortionless response direction.
When the DOA module is enabled, the steering vector will be estimated by DOA
estimation. You can set the steering vector by your own DOA estimation method.
Rnn matrix is estimated using MCRA noise estimation method. 
Microphone array could be any shape as long as you set the coordinates of each 
microphones(mic_coord) beforehand.
==============================================================================*/

#include "dios_ssp_mvdr_api.h"

void* dios_ssp_mvdr_init_api(int mic_num, void* mic_coord)
{
	void* st = NULL;
	st = (void*)calloc(1, sizeof(objMVDR));
	objMVDR* ptr = (objMVDR*)st;
	dios_ssp_mvdr_init(ptr, mic_num, (PlaneCoord*)mic_coord);

	return st;	
}

int dios_ssp_mvdr_reset_api(void *ptr)
{
	if(ptr == NULL)
	{
		printf("mvdr handle not init!\n");
		return ERROR_MVDR;
	}

	objMVDR* ptr_mvdr;
	ptr_mvdr = (objMVDR*)ptr;
	dios_ssp_mvdr_reset(ptr_mvdr);
	
	return 0;
}

int dios_ssp_mvdr_process_api(void* ptr, float* mic_data, float* out_data, float loc_phi)
{
	int angle;
	angle = (int)(loc_phi + 0.5);
	objMVDR *ptr_mvdr;
	ptr_mvdr= (objMVDR*)ptr;
	dios_ssp_mvdr_process(ptr_mvdr, mic_data, out_data, angle);
	
	return 0;
}

int dios_ssp_mvdr_uninit_api(void *ptr)
{
	objMVDR *ptr_mvdr;
	ptr_mvdr= (objMVDR*)ptr;
	dios_ssp_mvdr_delete(ptr_mvdr);
	
	return 0;
}
