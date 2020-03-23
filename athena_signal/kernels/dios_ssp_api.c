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

Description: This is an illustration of how you can use the modules of 
athena-signal to create your own signal processing algorithms. A complete 
framework includes high-pass filter(HPF), acoustic echo cancellation(AEC),
voice activity detection(VAD), beamforming using MVDR, noise supression(NS)
and automatic gain control(AGC). You can enable/disable each module according
to your needs by setting KEYs to 1/0 in SSP_PARAM struct.

Other modules such as source localization, blind source separation, 
de-reverberation will be added soon. We are working on them!
==============================================================================*/

#include "dios_ssp_api.h"

typedef struct {
    /* handle of each module */
	void* ptr_aec;
	void* ptr_vad;
	void* ptr_hpf;
    void* ptr_ns;
	void* ptr_agc;
    void* ptr_mvdr;
    void* ptr_gsc;
    void* ptr_doa;

    /* necessary buffer definition */
	float* ptr_mic_buf;
	float* ptr_ref_buf;
	float* ptr_data_buf;

    /* necessary variables */
    int cfg_frame_len;
    int cfg_mic_num;
    int cfg_ref_num;
    PlaneCoord cfg_mic_coord[16];  // maximum mic num, you can change it
    float cfg_wakeup_loc_phi;

    /* necessary variables */
    int dt_st;
    int vad_result;
    PolarCoord *loc_result;  // save source localization result
} objDios_ssp;

void* dios_ssp_init_api(objSSP_Param *SSP_PARAM)
{
    int i;
	void* ptr = NULL;
	ptr = (void*)calloc(1, sizeof(objDios_ssp));
	objDios_ssp* srv = (objDios_ssp*)ptr;
    
    // params init
    srv->cfg_frame_len = 128;
    srv->cfg_mic_num = SSP_PARAM->mic_num;
    srv->cfg_ref_num = SSP_PARAM->ref_num;;
    for(i=0; i<srv->cfg_mic_num; i++)
    {
        srv->cfg_mic_coord[i].x = SSP_PARAM->mic_coord[i].x;
        srv->cfg_mic_coord[i].y = SSP_PARAM->mic_coord[i].y;
        srv->cfg_mic_coord[i].z = SSP_PARAM->mic_coord[i].z;
    }

    // signal process modules init
    if(SSP_PARAM->HPF_KEY == 1)
    {
        srv->ptr_hpf = dios_ssp_hpf_init_api();
    }
    if(SSP_PARAM->AEC_KEY == 1)
    {
        srv->ptr_aec = dios_ssp_aec_init_api(srv->cfg_mic_num, srv->cfg_ref_num, srv->cfg_frame_len);
    }
    if(SSP_PARAM->DOA_KEY == 1)
    {
        srv->ptr_doa = dios_ssp_doa_init_api(srv->cfg_mic_num, (PlaneCoord*)srv->cfg_mic_coord);
    }
    if(SSP_PARAM->BF_KEY == 1)
    {
        srv->ptr_mvdr = dios_ssp_mvdr_init_api(srv->cfg_mic_num, (void*)srv->cfg_mic_coord);
    }
    if(SSP_PARAM->BF_KEY == 2)
    {
        srv->ptr_gsc = dios_ssp_gsc_init_api(srv->cfg_mic_num, (void*)srv->cfg_mic_coord);
    }
    //dios_ssp_aec_config_api(srv->ptr_aec, 0);  // 0: communication mode; 1: asr mode
    srv->ptr_vad = dios_ssp_vad_init_api();
    if(SSP_PARAM->NS_KEY == 1)
    {
        srv->ptr_ns = dios_ssp_ns_init_api(srv->cfg_frame_len);
    }
    if(SSP_PARAM->AGC_KEY == 1)
    {
        srv->ptr_agc = dios_ssp_agc_init_api(srv->cfg_frame_len, 26000.0, 0);
    }

    // allocate memory
    srv->ptr_mic_buf = (float*)calloc(srv->cfg_mic_num * srv->cfg_frame_len, sizeof(float));
    srv->ptr_ref_buf = (float*)calloc(srv->cfg_ref_num * srv->cfg_frame_len, sizeof(float));
    srv->ptr_data_buf = (float*)calloc(srv->cfg_frame_len, sizeof(float));
    srv->loc_result = (PolarCoord*)calloc(1, sizeof(PolarCoord));

    // variables init 
    srv->dt_st = 1;
    srv->vad_result = 1;
    srv->cfg_wakeup_loc_phi = 90;

	return ptr;
}

int dios_ssp_reset_api(void* ptr, objSSP_Param *SSP_PARAM)
{
    if(ptr == NULL)
    {
        return ERROR_AUDIO_PROCESS;
    }

    objDios_ssp* srv = (objDios_ssp*)ptr;
    int ret;

    // variables reset
    srv->dt_st = 1;
    srv->vad_result = 1;

    if(SSP_PARAM->HPF_KEY == 1)
    {
        ret = dios_ssp_hpf_reset_api(srv->ptr_hpf);
        if(ret != 0)
        {
            return ERROR_HPF;
        }
    }

    if(SSP_PARAM->AEC_KEY == 1)
    {
        ret = dios_ssp_aec_reset_api(srv->ptr_aec);
        if(ret != 0)
        {
            return ERROR_AEC;
        }
    }

    if(SSP_PARAM->DOA_KEY == 1)
    {
        ret = dios_ssp_doa_reset_api(srv->ptr_doa);
        if(ret != 0)
        {
            return ERROR_DOA;
        }
    }
    
    if(SSP_PARAM->BF_KEY == 1)
    {
        ret = dios_ssp_mvdr_reset_api(srv->ptr_mvdr);
        if(ret != 0)
        {
            return ERROR_MVDR;
        }
    }

    if(SSP_PARAM->BF_KEY == 2)
    {
        ret = dios_ssp_gsc_reset_api(srv->ptr_gsc);
        if(ret != 0)
        {
            return ERROR_GSC;
        }
    }

    ret = dios_ssp_vad_reset_api(srv->ptr_vad);
    if(ret != 0)
    {
        return ERROR_VAD;
    }

    if(SSP_PARAM->NS_KEY == 1)
    {
        ret = dios_ssp_ns_reset_api(srv->ptr_ns);
        if(ret != 0)
        {
            return ERROR_NS;
        }
    }

    if(SSP_PARAM->AGC_KEY == 1)
    {
        ret = dios_ssp_agc_reset_api(srv->ptr_agc);
        if(ret != 0)
        {
            return ERROR_AGC;
        }
    }

	return 0;
}

int dios_ssp_process_api(void* ptr, short* mic_buf, short* ref_buf, short* out_buf, objSSP_Param *SSP_PARAM)
{
    if(ptr == NULL)
    {
        return ERROR_AUDIO_PROCESS;
    }

    objDios_ssp* srv = (objDios_ssp*)ptr;
    int ret;
    int i, j;
   
    // get input data, single-channel or multi-channel, with or without reference
    for(i = 0; i < srv->cfg_mic_num; i++)
    {
        for(j = 0; j < srv->cfg_frame_len; j++)
        {
            srv->ptr_mic_buf[i * srv->cfg_frame_len + j] = (float)(mic_buf[i * srv->cfg_frame_len + j]);
        }
    }
    if(ref_buf != NULL)
    {
        for(i = 0; i < srv->cfg_ref_num; i++)
        {
            for(j = 0; j < srv->cfg_frame_len; j++)
            {
                srv->ptr_ref_buf[i * srv->cfg_frame_len + j] = (float)(ref_buf[i * srv->cfg_frame_len + j]);
            }
        }
    }

    // begin signal processing: HPF + AEC + Beamforming + VAD + NS + AGC
    // hpf process
    if(SSP_PARAM->HPF_KEY == 1)
    {
        ret = dios_ssp_hpf_process_api(srv->ptr_hpf, srv->ptr_mic_buf, srv->cfg_frame_len);
        if(ret != 0)
        {
            return ERROR_HPF;
        }
    }

    // aec process
    if(ref_buf != NULL && SSP_PARAM->AEC_KEY == 1)
    {

        ret = dios_ssp_aec_process_api(srv->ptr_aec, srv->ptr_mic_buf, srv->ptr_ref_buf, &srv->dt_st);
        if(ret != 0)
        {
            return ERROR_AEC;
        }
    }

    // save mic1
    memcpy(srv->ptr_data_buf, &srv->ptr_mic_buf[0], srv->cfg_frame_len * sizeof(float));

    if(SSP_PARAM->DOA_KEY == 1)
    {
        srv->cfg_wakeup_loc_phi = dios_ssp_doa_process_api(srv->ptr_doa, srv->ptr_mic_buf, srv->vad_result, srv->dt_st);
    }
    
    // MVDR process
    if(SSP_PARAM->BF_KEY == 1)
    {
        ret = dios_ssp_mvdr_process_api(srv->ptr_mvdr, srv->ptr_mic_buf, srv->ptr_data_buf, srv->cfg_wakeup_loc_phi);
        if(ret != 0)
        {
            return ERROR_MVDR;
        }
    }

    //GSC process
    if(SSP_PARAM->BF_KEY == 2)
    {
        ret = dios_ssp_gsc_process_api(srv->ptr_gsc, srv->ptr_mic_buf, srv->ptr_data_buf, srv->cfg_wakeup_loc_phi);
        if(ret != 0)
        {
            return ERROR_GSC;
        }
    }
    
    // vad process
    ret = dios_ssp_vad_process_api(srv->ptr_vad, srv->ptr_data_buf, srv->dt_st);
    if(ret != 0)
    {
        return ERROR_VAD;
    }
    srv->vad_result = dios_ssp_vad_result_get(srv->ptr_vad);

    // ns process
    if(SSP_PARAM->NS_KEY == 1)
    {
        ret = dios_ssp_ns_process(srv->ptr_ns, srv->ptr_data_buf);
        if(ret != 0)
        {
            return ERROR_NS;
        }
    }

    // agc process
    if(SSP_PARAM->AGC_KEY == 1)
    {
        ret = dios_ssp_agc_process_api(srv->ptr_agc, srv->ptr_data_buf, srv->vad_result, 1, srv->dt_st);
        if(ret != 0)
        {
            return ERROR_AGC;
        }
    }
    // end of wakeup signal process

    // get output data for wakeup
    for(j = 0; j < srv->cfg_frame_len; j++)
    {
        out_buf[j] = (short)(srv->ptr_data_buf[j]);
    }

    return 0;
}

int dios_ssp_uninit_api(void* ptr, objSSP_Param *SSP_PARAM)
{
    if(ptr == NULL)
    {
        return ERROR_AUDIO_PROCESS;
    }

    objDios_ssp* srv = (objDios_ssp*)ptr;
    int ret;

    /* free memory */
    if(srv->ptr_mic_buf != NULL)
    {
        free(srv->ptr_mic_buf);
        srv->ptr_mic_buf = NULL;
    }
    if(srv->ptr_ref_buf != NULL)
    {
        free(srv->ptr_ref_buf);
        srv->ptr_ref_buf = NULL;
    }
    if(srv->ptr_data_buf != NULL)
    {
        free(srv->ptr_data_buf);
        srv->ptr_data_buf = NULL;
    }

    if(SSP_PARAM->HPF_KEY == 1)
    {
        ret = dios_ssp_hpf_uninit_api(srv->ptr_hpf);
        if(ret != 0)
        {
            return ERROR_HPF;
        }
    }
    
    if(SSP_PARAM->AEC_KEY == 1)
    {
        ret = dios_ssp_aec_uninit_api(srv->ptr_aec);
        if(ret != 0)
        {
            return ERROR_AEC;
        }
    }

    if(SSP_PARAM->DOA_KEY == 1)
    {
        ret = dios_ssp_doa_uninit_api(srv->ptr_doa);
        if(ret != 0)
        {
            return ERROR_DOA;
        }
    }
    
    if(SSP_PARAM->BF_KEY == 1)
    {
        ret = dios_ssp_mvdr_uninit_api(srv->ptr_mvdr);
        if(ret != 0)
        {
            return ERROR_MVDR;
        }
    }

    if(SSP_PARAM->BF_KEY == 2)
    {
        ret = dios_ssp_gsc_uninit_api(srv->ptr_gsc);
        if(ret != 0)
        {
            return ERROR_GSC;
        }
    }

    ret = dios_ssp_vad_uninit_api(srv->ptr_vad);
    if(ret != 0)
    {
        return ERROR_VAD;
    }

    if(SSP_PARAM->NS_KEY == 1)
    {
        ret = dios_ssp_ns_uninit_api(srv->ptr_ns);
        if(ret != 0)
        {
            return ERROR_NS;
        }
    }

    if(SSP_PARAM->AGC_KEY == 1)
    {
        ret = dios_ssp_agc_uninit_api(srv->ptr_agc);
        if(ret != 0)
        {
            return ERROR_AGC;
        }
    }

    free(srv->loc_result);
    free(srv);
	return OK_AUDIO_PROCESS;
}

