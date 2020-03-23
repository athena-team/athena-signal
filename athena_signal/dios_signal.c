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

Description: Pass in different parameters to start different function
modules. A complete framework includes high-pass filter(HPF),
acoustic echo cancellation(AEC), voice activity detection(VAD), beamforming
using MVDR, noise supression(NS) and automatic gain control(AGC). You can
enable/disable each module according to your needs by setting KEYs to
1/0 in fe_switch array. Parameter mic_coord is the coordinates of each microphone
of the microphone array using in MVDR.
==============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "kernels/dios_ssp_api.h"

const int array_frm_len = 128;

int dios_ssp_v1(int argc, char **argv, int *fe_switch, size_t m, float *mic_coord, size_t n, int mic_num, int ref_num)
{
    void* ptr = NULL;
	ptr = (void*)malloc(sizeof(objSSP_Param));
	objSSP_Param* SSP_PARAM = (objSSP_Param*)ptr;
    SSP_PARAM->AEC_KEY = fe_switch[0];
    SSP_PARAM->NS_KEY = fe_switch[1];
    SSP_PARAM->AGC_KEY = fe_switch[2];
    SSP_PARAM->HPF_KEY = fe_switch[3];
    SSP_PARAM->BF_KEY = fe_switch[4];
    SSP_PARAM->DOA_KEY = fe_switch[5];
    SSP_PARAM->mic_num = mic_num;
    SSP_PARAM->ref_num = ref_num;

    if (SSP_PARAM->AEC_KEY == 1)
    {
        if (SSP_PARAM->ref_num == 0)
        {
            printf("AEC is turned on, ref_num must be greater than 0.  \n");
            exit(-1);
        }
    }

    if(SSP_PARAM->BF_KEY != 0 || SSP_PARAM->DOA_KEY == 1)
    {
        if (SSP_PARAM->mic_num <= 1)
        {
            printf("MVDR is turned on, mic_num must be greater than 1. \n");
            exit(-1);
        }

        if (n != 3 * (unsigned int)SSP_PARAM->mic_num)
        {
            printf("The number of coordinate points must match the number of microphones. \n");
            exit(-1);
        }

        int i;
        for (i = 0; i < SSP_PARAM->mic_num; i++)
        {
            SSP_PARAM->mic_coord[i].x = mic_coord[3 * i];
            SSP_PARAM->mic_coord[i].y = mic_coord[3 * i + 1];
            SSP_PARAM->mic_coord[i].z = mic_coord[3 * i + 2];
        }
    }
    int input_idx = -1;
    int ref_idx = -1;
    int output_idx = -1;
    int ii, jj;
    for(ii = 0; ii < argc; ii++)
    {
        if(strcmp(argv[ii], "-i") == 0)
        {
            input_idx = ii;
        }
        if(strcmp(argv[ii], "-r") == 0)
        {
            ref_idx = ii;
        }
        if(strcmp(argv[ii], "-o") == 0)
        {
            output_idx = ii;
        }
    }
    if((input_idx == -1) || (output_idx == -1))
    {
        printf("usage: %s -i input1.pcm [input2.pcm] [-r ref.pcm] -o output.pcm\r\n", argv[0]);
        return -1;
    }
    int channel_num;
    if(ref_idx == -1)
    {
        channel_num = output_idx - input_idx - 1;
    }
    else
    {
        channel_num = ref_idx - input_idx - 1;
    }

    FILE **fp = (FILE**)calloc(channel_num, sizeof(FILE*));;
    FILE *fpref = NULL;
    FILE *fpout = NULL;
    short *ptr_input_data = (short*)calloc(channel_num * array_frm_len, sizeof(short));
    short *ptr_output_data = (short*)calloc(array_frm_len, sizeof(short));
    short *ptr_temp = (short*)calloc(array_frm_len, sizeof(short));
    short *ptr_ref_data = (short*)calloc(array_frm_len, sizeof(short));

    void* st;
    st = dios_ssp_init_api(SSP_PARAM);
    dios_ssp_reset_api(st, SSP_PARAM);

    /* read data from outline files */
    char sz_file_name[1024];
    strcpy(sz_file_name, argv[input_idx + 1]);
    fp[0] = fopen(sz_file_name, "rb");
    fseek(fp[0], 0, SEEK_END);
    long file_len = ftell(fp[0]);
    file_len /= 2;
    rewind(fp[0]);
    fclose(fp[0]);
    long frame_num = file_len / array_frm_len;
    int sample_res = file_len % array_frm_len;

    for(ii = 0; ii < channel_num; ii++)
    {
        strcpy(sz_file_name, argv[input_idx + 1 + ii]);
        fp[ii] = fopen(sz_file_name, "rb");
        if(fp[ii] == NULL)
        {
            printf("Open input file %s error.\r\n", sz_file_name);
            exit(0);
        }
        printf("Open input file %s success.\r\n", sz_file_name);
    }
    fpout = fopen(argv[output_idx + 1], "wb");
    if(fpout == NULL)
    {
        printf("Open out file %s error.\r\n", argv[output_idx + 1]);
        exit(0);
    }
    if(ref_idx > 0)
    {
        fpref = fopen(argv[ref_idx + 1], "rb");
        if(fpref == NULL)
        {
            printf("Open ref file %s error.\r\n", argv[ref_idx + 1]);
            exit(0);
        }
    }

    /* if you encounter such error: Matrix is singular!
     * try to use these codes to add random disturbances to Rnn matrix
     */
    /*int kk;
    srand((unsigned)time(NULL));*/

    /* signal processing here */
    for(ii = 0; ii < frame_num; ii++)
    {
        /* prepare input signals */
        for(jj = 0; jj < channel_num; jj++)
        {
            fread(ptr_temp, sizeof(short), array_frm_len, fp[jj]);
            memcpy(&ptr_input_data[jj * array_frm_len], ptr_temp, sizeof(short) * array_frm_len);
            /* if you encounter such error: Matrix is singular!
             * try to use these codes to add random disturbances to Rnn matrix
             */
            /*for (kk = 0; kk < array_frm_len; kk++)
            {
                ptr_input_data[jj * array_frm_len + kk] += rand() % 10;
            }*/
        }

        /* prepare ref signal */
        if(ref_idx > 0)
        {
            fread(ptr_temp, sizeof(short), array_frm_len, fpref);
            memcpy(ptr_ref_data, ptr_temp, sizeof(short) * array_frm_len);
        }
        else
        {
            memset(ptr_ref_data, 0, sizeof(short) * array_frm_len);
        }

        /* dios ssp processing here */
        dios_ssp_process_api(st, ptr_input_data, ptr_ref_data, ptr_output_data, SSP_PARAM);

        /* save output file */
        fwrite(ptr_output_data, sizeof(short), array_frm_len, fpout);
    }
    fwrite(ptr_output_data, sizeof(short), sample_res, fpout);

    if(ref_idx > 0)
    {
        fclose(fpref);
    }
    fclose(fpout);
    for(ii = 0; ii < channel_num; ii++)
    {
        fclose(fp[ii]);
    }
    dios_ssp_uninit_api(st, SSP_PARAM);
    free(fp);
    free(ptr_temp);
    free(ptr_input_data);
    free(ptr_output_data);
    free(ptr_ref_data);
    free(SSP_PARAM);
    printf("Process finished.\r\n");
    return 0;
}

