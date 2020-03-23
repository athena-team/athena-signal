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

Description: GSC beamforming includes two functions, steering vector and
processing function.
==============================================================================*/

#include "dios_ssp_gsc_micarray.h"

static const int gsc_sampling_rate = 16000; /* sampling rate */
static const int gsc_block_len = 128;       /* frame len */

void dios_ssp_gsc_init(objGSCProcess *ptr_gsc, int mic_num, PlaneCoord* mic_coord)
{
	/* parameter initialization */
	ptr_gsc->nfs = gsc_sampling_rate;
	ptr_gsc->frame_len = gsc_block_len;
	ptr_gsc->mic_num = mic_num;
	ptr_gsc->outbeam_num = 1;
	ptr_gsc->loca_use_flag = 1;
	ptr_gsc->type = General_ArrayArbitrary;
	ptr_gsc->source_location.rho = 4.0f;
	ptr_gsc->source_location.theta = 0.0f;
	ptr_gsc->source_location.phi = 0.0f;
	ptr_gsc->ptr_input_data_float = (float**)calloc(ptr_gsc->mic_num, sizeof(float*));
	for (int i_mic = 0; i_mic < ptr_gsc->mic_num; i_mic++)
	{
		ptr_gsc->ptr_input_data_float[i_mic] = (float*)calloc(ptr_gsc->frame_len, sizeof(float));
	}

	/* input parameter and signal buffer init */
	ptr_gsc->ptr_mic_coord = (PlaneCoord*)calloc(mic_num, sizeof(PlaneCoord));
	for (int i = 0; i < mic_num; i++)
	{
		ptr_gsc->ptr_mic_coord[i].x = mic_coord[i].x;
		ptr_gsc->ptr_mic_coord[i].y = mic_coord[i].y;
		ptr_gsc->ptr_mic_coord[i].z = mic_coord[i].z;
	}

	/* output signal buffer init */
	ptr_gsc->ptr_output_data_float = (float*)calloc(ptr_gsc->frame_len, sizeof(float));
	ptr_gsc->ptr_output_data_short = (short*)calloc(ptr_gsc->frame_len, sizeof(short));

	ptr_gsc->multigscbeamformer = (objCMultiGSCbeamformer*)calloc(1, sizeof(objCMultiGSCbeamformer));
	dios_ssp_gsc_multibeamformer_init(ptr_gsc->multigscbeamformer, ptr_gsc->mic_num, ptr_gsc->outbeam_num, ptr_gsc->nfs, ptr_gsc->frame_len, ptr_gsc->type, ptr_gsc->ptr_mic_coord);

}

void dios_ssp_gsc_reset(objGSCProcess *ptr_gsc)
{
	dios_ssp_gsc_multibeamformer_reset(ptr_gsc->multigscbeamformer);
}

void doProcess(objGSCProcess *ptr_gsc)
{	
	dios_ssp_gsc_multibeamformer_arraysteer(ptr_gsc->multigscbeamformer, &(ptr_gsc->source_location));
	dios_ssp_gsc_multibeamformer_process(ptr_gsc->multigscbeamformer, ptr_gsc->ptr_input_data_float);
	float** gsc_bfout = ptr_gsc->multigscbeamformer->m_pOutput;
	memcpy(ptr_gsc->ptr_output_data_float, gsc_bfout[0], ptr_gsc->frame_len * sizeof(float));
}

void dios_ssp_gsc_delete(objGSCProcess *ptr_gsc)
{
	free((void*)ptr_gsc->ptr_mic_coord);
	free((void*)ptr_gsc->ptr_output_data_short);
	free((void*)ptr_gsc->ptr_output_data_float);
	for (int i_mic = 0; i_mic < ptr_gsc->mic_num; i_mic++)
	{
		free(ptr_gsc->ptr_input_data_float[i_mic]);
	}
	free(ptr_gsc->ptr_input_data_float);

	if(ptr_gsc->multigscbeamformer != NULL)
	{
		dios_ssp_gsc_multibeamformer_delete(ptr_gsc->multigscbeamformer);
	}
	free(ptr_gsc->multigscbeamformer);

}
