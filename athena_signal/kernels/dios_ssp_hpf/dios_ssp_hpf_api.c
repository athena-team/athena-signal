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

Description: High-pass filtering is implemented using cascaded-iir-filter.
The cut-off frequency is 200Hz in this program. You can rewrite the iir filter
coefficients and gains, with the help of filter design toolbox in MATLAB,
to generate high-pass filter with cut-off frequency you set.
==============================================================================*/

#include "dios_ssp_hpf_api.h"

#define SECTION_NUM 2

// struct of filter coefficients
typedef struct 
{
    // register
    float reg_x1[SECTION_NUM];
    float reg_x2[SECTION_NUM];
    float reg_y1[SECTION_NUM];
    float reg_y2[SECTION_NUM];
    // filter coefficients
    float num_0[SECTION_NUM];
    float num_1[SECTION_NUM];
    float num_2[SECTION_NUM]; // numerator
    float den_0[SECTION_NUM];
    float den_1[SECTION_NUM];
    float den_2[SECTION_NUM]; // denominator
    // gain
    float hpf_gain[SECTION_NUM];
} objHPF;

// High-pass filtering real implementation
float hpf_calc(objHPF *srv, float x)
{
    float y = 0;
    float center_tap = 0;
    int section;

    for(section = 0; section < SECTION_NUM; section++)
    {
        center_tap = x * srv->num_0[section] + srv->num_1[section] * srv->reg_x1[section] 
            + srv->num_2[section] * srv->reg_x2[section];
        y = srv->den_0[section] * center_tap - srv->den_1[section] * srv->reg_y1[section] 
            - srv->den_2[section] * srv->reg_y2[section];

        srv->reg_x2[section] = srv->reg_x1[section];
        srv->reg_x1[section] = x;
        srv->reg_y2[section] = srv->reg_y1[section];
        srv->reg_y1[section] = y;

        y *= srv->hpf_gain[section];
        x = y;

    }    
    return y;
}

void* dios_ssp_hpf_init_api(void)
{
    void* ptr = NULL;
    objHPF *srv;
	ptr = (void*)calloc(1, sizeof(objHPF));
    srv = (objHPF *)ptr;
    int i;

    for(i = 0; i < SECTION_NUM; i++)
    {
        srv->reg_x1[i] = 0.0;
        srv->reg_x2[i] = 0.0;
        srv->reg_y1[i] = 0.0;
        srv->reg_y2[i] = 0.0;
    }
    
    srv->num_0[0] = 1.0f;
    srv->num_1[0] = -2.0f;
    srv->num_2[0] = 1.0f;
    srv->den_0[0] = 1.0f;
    srv->den_1[0] = -1.9467f;
    srv->den_2[0] = 0.9509f;
    srv->hpf_gain[0] = 0.9744f;
    srv->num_0[1] = 1.0f;
    srv->num_1[1] = -2.0f;
    srv->num_2[1] = 1.0f;
    srv->den_0[1] = 1.0f;
    srv->den_1[1] = -1.8814f;
    srv->den_2[1] = 0.8855f;
    srv->hpf_gain[1] = 0.9417f;

    return ptr;
}

int dios_ssp_hpf_reset_api(void* ptr)
{
	if (NULL == ptr) 
    {
		return -1;
	}
    objHPF *srv;
    srv = (objHPF *) ptr;
    int i;
    
    for(i = 0; i < SECTION_NUM; i++)
    {
        srv->reg_x1[i] = 0.0;
        srv->reg_x2[i] = 0.0;
        srv->reg_y1[i] = 0.0;
        srv->reg_y2[i] = 0.0;
    }

	return 0;
}

int dios_ssp_hpf_process_api(void* ptr, float* io_buf, int siglen)
{
	if (NULL == ptr) 
    {
		return -1;
	}
	objHPF *srv;
	int j = 0;

	srv = (objHPF *)ptr;
	for (j = 0; j < siglen; j++) 
    {
		io_buf[j] = hpf_calc(srv, io_buf[j]);
	}
	return 0;
}

int dios_ssp_hpf_uninit_api(void* ptr)
{
	if (NULL == ptr) 
    {
		return -1;
	}
	objHPF *srv;
    srv = (objHPF *) ptr;
    free(srv);
	return 0;
}

