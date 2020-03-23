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

Description: the main operation of the delay processing function. The function 
consists of two parts: long-term delay estimation and short-term delay estimation,
the sum of two parts is the real delay. Using delays to align the data of the 
reference and mic signal.
==============================================================================*/

/* include file */
#include "dios_ssp_aec_tde.h"

objTDE* dios_ssp_aec_tde_init(int mic_num, int ref_num, int frm_len)
{
	int i;
	int ret;
	objTDE *srv = NULL;
    srv = (objTDE *)calloc(1, sizeof(objTDE));

	srv->mic_num = mic_num;
	srv->ref_num = ref_num;
	srv->frm_len = frm_len;	

    srv->tde_short = NULL;
    srv->tde_long = NULL;

    srv->tdeBuf_ref    = NULL;
    srv->tdeBuf_mic    = NULL;

    srv->audioBuf_mic  = NULL;
    srv->audioBuf_ref  = NULL;
  
    srv->audioBuf_mic = (float **)calloc(srv->mic_num, sizeof(float*));
    for (i = 0; i < srv->mic_num; i++)
    {
	    srv->audioBuf_mic[i] = (float*)calloc(DELAY_BUFFER_SIZE, sizeof(float));
    }
    
    srv->audioBuf_ref = (float **)calloc(srv->ref_num, sizeof(float*));
    for(i = 0; i < srv->ref_num; i++)
    {
	    srv->audioBuf_ref[i] = (float *)calloc(DELAY_BUFFER_SIZE, sizeof(float));
    }
    
    srv->tdeBuf_ref = (float *)calloc(PART_LEN, sizeof(float));	
    srv->tdeBuf_mic = (float *)calloc(PART_LEN, sizeof(float));
    
    /* long-term tde */ 
    ret = dios_ssp_aec_tde_creatcore(&srv->tde_long, MAX_DELAY_LONG, DELAY_WIN_SLIDE_TDE); 
    if (ret != 0)
    {
	    printf("dios_ssp_aec_tde_creatcore Error!\n");
    }    

    /* short-term tde */
    ret = dios_ssp_aec_tde_creatcore(&srv->tde_short, MAX_DELAY_SHORT, DELAY_WIN_SLIDE); 
    if (ret != 0)
    {
	    printf("dios_ssp_aec_tde_creatcore Error!\n");
    }

	dios_ssp_aec_tde_reset(srv);

    return srv;
}

int dios_ssp_aec_tde_reset(objTDE *srv)
{
	int i;
	int ret;
	if (NULL == srv)
	{
		return ERR_AEC;
	}

	srv->CalibrateEnable = 1;
	srv->CalibrateCounter = 1;
	srv->pt_buf_push = 0;
	srv->pt_output = 0;

	srv->delay_fixed_sec = 0.0f;
	srv->delay_varied_sec = 0.0f;
	srv->frame_cnt_nSecond = 0;
	srv->flag_delayfind = 0;

    for(i = 0; i < srv->mic_num; i++)
    {
		memset(srv->audioBuf_mic[i], 0, DELAY_BUFFER_SIZE * sizeof(float));
    }
    
    for(i = 0; i < srv->ref_num; i++)
    {
		memset(srv->audioBuf_ref[i], 0, DELAY_BUFFER_SIZE * sizeof(float));
    }
    memset(srv->tdeBuf_ref, 0, PART_LEN * sizeof(float));
    memset(srv->tdeBuf_mic, 0, PART_LEN * sizeof(float));

	srv->tde_short_shift_smpl = 0;
	srv->tde_long_shift_smpl = 0;
	srv->act_delay_smpl = 0;
	srv->act_delay_smpl_old = 0;

	ret = dios_ssp_aec_tde_initcore(srv->tde_long);
	if (ret != 0)
	{
		printf("dios_ssp_aec_tde_initcore Error!\n");
	}

	ret = dios_ssp_aec_tde_initcore(srv->tde_short);
	if (ret != 0)
	{
		printf("dios_ssp_aec_tde_initcore Error!\n");
	}

    return 0 ;
}

int dios_ssp_aec_tde_process(objTDE *srv, float* refbuf, float* micbuf)
{
	int i;
	int i_mic;
	int i_ref;
	int i_tde;
    int look_ahead = 100;
	int pp;

	if (NULL == srv)
	{
		return ERR_AEC;
	}

	for (i_mic = 0; i_mic < srv->mic_num; i_mic++)
	{
		for (i = 0; i < srv->frm_len; i++)
		{
			srv->audioBuf_mic[i_mic][srv->pt_buf_push + i] = micbuf[i_mic * srv->frm_len + i];
		}
	}
	for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
	{
		for (i = 0; i < srv->frm_len; i++)
		{
			srv->audioBuf_ref[i_ref][i + srv->pt_buf_push] = refbuf[i_ref * srv->frm_len + i];
		}
	}
    
#if (DIOS_SSP_AEC_TDE_ON == 1)
    int idx;
    /* long-term delay estimation */
    srv->flag_delayfind = 0;
    for (i_tde = 0; i_tde < 2; i_tde++)
    {
		for (i = 0; i < PART_LEN; i++)
		{
			idx = srv->pt_buf_push + i + i_tde * PART_LEN - look_ahead;
			if (idx < 0) 
			{
				idx += DELAY_BUFFER_SIZE;
			}
			srv->tdeBuf_mic[i] =  srv->audioBuf_mic[0][idx];
			srv->tdeBuf_ref[i] =  srv->audioBuf_ref[0][srv->pt_buf_push + i + i_tde * PART_LEN];
		}
		int flag1 = dios_ssp_aec_tde_ProcessBlock(srv->tde_long, srv->tdeBuf_ref, srv->tdeBuf_mic);
		srv->CalibrateCounter--;
		if (srv->CalibrateCounter == 0)
		{
			srv->CalibrateEnable = 1;
			srv->CalibrateCounter = 1000;
		}
		if (srv->CalibrateEnable == 1 && flag1)
		{
			srv->tde_long_shift_smpl = get_tde_final(srv->tde_long);
			srv->tde_long_shift_smpl -= 3200;
			if (srv->tde_long_shift_smpl < 0)
			{
				srv->tde_long_shift_smpl = 0;
			}
			srv->CalibrateEnable = 0;
			srv->CalibrateCounter = 1000;
		}
		if (flag1 == 1)
		{
			srv->flag_delayfind = 1;
		}
    }

    /* short-term delay estimation */
    for (i_tde = 0; i_tde < 2; i_tde++)
    {
		int j1, j2;
		for (i = 0; i < PART_LEN; i++)
		{
			j1 =  srv->pt_buf_push + i + i_tde * PART_LEN - look_ahead;
			if (j1 < 0) 
			{
				j1 += DELAY_BUFFER_SIZE;
			}
			srv->tdeBuf_mic[i] =  srv->audioBuf_mic[0][j1];
			j2 = srv->pt_buf_push + i + i_tde * PART_LEN - srv->tde_long_shift_smpl;
			if (j2 < 0) 
			{
				j2 += DELAY_BUFFER_SIZE;
			}
			srv->tdeBuf_ref[i] =  srv->audioBuf_ref[0][j2];
		}
		int flag2 = dios_ssp_aec_tde_ProcessBlock(srv->tde_short, srv->tdeBuf_ref, srv->tdeBuf_mic);
		if (flag2)
		{
			srv->tde_short_shift_smpl = get_tde_final(srv->tde_short);
		}
		else
		{
			if (srv->flag_delayfind)
			{
				if (srv->tde_long_shift_smpl > 0)
				{
					srv->tde_short_shift_smpl = 3200;
				}
				else if (srv->tde_long_shift_smpl == 0)
				{
					srv->tde_short_shift_smpl = get_tde_final(srv->tde_long);
				}
				srv->flag_delayfind = 0;
			}
		}
    }

    /* delay estimation result */
    srv->act_delay_smpl = srv->tde_long_shift_smpl + srv->tde_short_shift_smpl;
	srv->act_delay_smpl = (int)(0.9 * srv->act_delay_smpl + 0.1 * srv->act_delay_smpl_old);
	srv->act_delay_smpl_old = srv->act_delay_smpl;
	/* make sure causality, back two frames */
	int act_delay_temp = srv->act_delay_smpl - srv->frm_len * 2;
	srv->act_delay_smpl = (act_delay_temp > 0)? act_delay_temp : 0;
#else
	srv->act_delay_smpl = 0;
#endif
    int pt_pop;
    /* mic signal */
    pt_pop = srv->pt_buf_push - look_ahead;
    if (pt_pop < 0)
    {
		pt_pop += DELAY_BUFFER_SIZE;
    }
	for (i_mic = 0; i_mic < srv->mic_num; i_mic++)
	{
		if (pt_pop < DELAY_BUFFER_SIZE && (pt_pop + srv->frm_len > DELAY_BUFFER_SIZE))
		{
			int len = DELAY_BUFFER_SIZE - pt_pop;
			memcpy(micbuf + i_mic * srv->frm_len, srv->audioBuf_mic[i_mic] + pt_pop, len * sizeof(float));
			memcpy(micbuf + i_mic * srv->frm_len + len, srv->audioBuf_mic[i_mic], (srv->frm_len - len) * sizeof(float));
		}
		else
		{
			memcpy(micbuf + i_mic * srv->frm_len, srv->audioBuf_mic[i_mic] + pt_pop, srv->frm_len * sizeof(float));
		}
	}

	/* ref signal */
	if (srv->pt_buf_push - srv->act_delay_smpl < 0)
	{
		pp = DELAY_BUFFER_SIZE + (srv->pt_buf_push - srv->act_delay_smpl);
	}
	else
	{
		pp = srv->pt_buf_push - srv->act_delay_smpl;
	}
	for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
	{
		if (pp < DELAY_BUFFER_SIZE && (pp + srv->frm_len > DELAY_BUFFER_SIZE))
		{
			int len = DELAY_BUFFER_SIZE - pp;
			memcpy(refbuf + i_ref * srv->frm_len, srv->audioBuf_ref[i_ref] + pp, len * sizeof(float));
			memcpy(refbuf + i_ref * srv->frm_len + len, srv->audioBuf_ref[i_ref], (srv->frm_len - len) * sizeof(float));
		}
		else
		{
			memcpy(refbuf + i_ref * srv->frm_len, srv->audioBuf_ref[i_ref] + pp, srv->frm_len * sizeof(float));
		}			
	}

    srv->pt_buf_push = (srv->pt_buf_push + srv->frm_len) % DELAY_BUFFER_SIZE;
	
    return 0;
}

int dios_ssp_aec_tde_uninit(objTDE* srv)
{
	int i_mic;
	int i_ref;
    if (NULL == srv)
    {
		return ERR_AEC;
    }

    if (srv->audioBuf_mic != NULL)
    {
        for (i_mic = 0; i_mic < srv->mic_num; i_mic++)
		{
            free(srv->audioBuf_mic[i_mic]);
        }
        free(srv->audioBuf_mic);
        srv->audioBuf_mic = NULL;
    }

    if (srv->audioBuf_ref != NULL)
    {
        for (i_ref = 0; i_ref < srv->ref_num; i_ref++)
		{
            free(srv->audioBuf_ref[i_ref]);
        }
        free(srv->audioBuf_ref);
        srv->audioBuf_ref = NULL;
    }
    if (srv->tdeBuf_ref != NULL)
    {
        free(srv->tdeBuf_ref);
        srv->tdeBuf_ref = NULL;
    }
    if (srv->tdeBuf_mic != NULL)
    {
        free(srv->tdeBuf_mic);
        srv->tdeBuf_mic = NULL;
    }

    dios_ssp_aec_tde_freecore(srv->tde_long);
    dios_ssp_aec_tde_freecore(srv->tde_short);

    if (srv != NULL)
    {
        free(srv);
        srv = NULL;
    }

    return 0;
}
