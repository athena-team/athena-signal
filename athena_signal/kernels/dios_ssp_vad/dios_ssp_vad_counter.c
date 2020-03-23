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

Description: subfunctions of the VAD module
==============================================================================*/

#include "dios_ssp_vad_counter.h"

const int VAD_CERTAINTY_COUNTER_LENGTH = 10;
const float CERTAINTY_SMOOTH_RATE = 0.9857f;
const float CERTAINTY_SUM_THRESH = 16;
const float CERTAINTY_SUM_THRESH_LOW = 13;
const int VAD_FALSE_COUNTER_LENGTH = 20;
const float FALSE_ALARM_CNT_THLD = 10;
const int SMOOTH_0_CNT_THLD = 16;
const int SMOOTH_1_CNT_THLD = 4;

typedef struct {
	float* certainty_buff;
	float* false_alarm_buff;
	int cnt_index_cert;
	int cnt_index_fals;
	int vad_certainty_counter_length;
	float vad_certainty_cnt;
	int vad_false_counter_length;
	float false_alarm_cnt;
	float vad_certainty_cnt_thld;
	// max min filter counter
	int continuous_0_cnt;
	int continuous_1_cnt;
}VAD_COUNTER;

void dios_ssp_vad_mix_process_subfunc(VAD_COUNTER *srv, float signaldev, float history_certainty, float certainty_level)
{
	if (signaldev > 80 && signaldev <= 150)
        {
			certainty_level = 0.005f * (signaldev - 80) + 2;
		}
        else if (signaldev > 150 && signaldev <= 300)
        {
			certainty_level = 0.015f * (signaldev - 150) + 2.35f;
		}
        else if (signaldev > 300)
        {
			certainty_level = 0.005f * (signaldev - 300) + 4.6f;
		}
		certainty_level = ((certainty_level > 5.35f) ? 5.35f :certainty_level);

		srv->vad_certainty_cnt += certainty_level - history_certainty;
		srv->vad_certainty_cnt = ((srv->vad_certainty_cnt < 0) ? 0 : srv->vad_certainty_cnt);
		srv->certainty_buff[srv->cnt_index_cert] = certainty_level;
		srv->cnt_index_cert++;
}

void* dios_ssp_vad_counter_init(void)
{
	void* vad_counter_handle = NULL;
	VAD_COUNTER *srv;
	vad_counter_handle = (void*)calloc(1, sizeof(VAD_COUNTER));
	srv = (VAD_COUNTER *)vad_counter_handle;
	srv->vad_certainty_counter_length = VAD_CERTAINTY_COUNTER_LENGTH;
	srv->certainty_buff = NULL;
	srv->certainty_buff = (float*)calloc(srv->vad_certainty_counter_length, sizeof(float));
	srv->cnt_index_cert = 0;
	srv->vad_certainty_cnt = 0;
	srv->vad_certainty_cnt_thld = CERTAINTY_SUM_THRESH;

	srv->vad_false_counter_length = VAD_FALSE_COUNTER_LENGTH;
	srv->false_alarm_buff = NULL;
	srv->false_alarm_buff = (float*)calloc(srv->vad_false_counter_length, sizeof(float));
	srv->cnt_index_fals = 0;
	srv->false_alarm_cnt = 0;

	// max min filter counter
	srv->continuous_0_cnt = 25;
	srv->continuous_1_cnt = 0;

	return vad_counter_handle;
}

int dios_ssp_vad_counter_reset(void* vad_counter_handle)
{
	if (vad_counter_handle == NULL) 
    {
		return -1;
	}

	VAD_COUNTER *srv;
	srv = (VAD_COUNTER *) vad_counter_handle;

	memset(srv->certainty_buff, 0, srv->vad_certainty_counter_length * sizeof(float));
	srv->cnt_index_cert = 0;
	srv->vad_certainty_cnt = 0;
	srv->vad_certainty_cnt_thld = CERTAINTY_SUM_THRESH;
	memset(srv->false_alarm_buff, 0, srv->vad_false_counter_length * sizeof(float));
	srv->cnt_index_fals = 0;
	srv->false_alarm_cnt = 0;
	srv->continuous_0_cnt = SMOOTH_0_CNT_THLD;
	srv->continuous_1_cnt = 0;

	return 0;
}

int dios_ssp_vad_mix_process(int apm_flag, int dt_flag, void* vad_counter_handle, float signaldev, int* vad_flag, int state)
{
	VAD_COUNTER *srv;
	srv = (VAD_COUNTER *)vad_counter_handle;
	float history_certainty = 0;
	float history_false = 0;
	*vad_flag = 0;
	if (state == 1)
    {
		srv->vad_certainty_cnt_thld *= CERTAINTY_SMOOTH_RATE;
		srv->vad_certainty_cnt_thld = ((srv->vad_certainty_cnt_thld<CERTAINTY_SUM_THRESH_LOW)?CERTAINTY_SUM_THRESH_LOW:srv->vad_certainty_cnt_thld);
	}
    else 
    {
        srv->vad_certainty_cnt_thld = CERTAINTY_SUM_THRESH;
    }
	srv->cnt_index_cert = (srv->cnt_index_cert % srv->vad_certainty_counter_length);
	history_certainty = srv->certainty_buff[(srv->cnt_index_cert + 1) % srv->vad_certainty_counter_length];
	srv->cnt_index_fals = (srv->cnt_index_fals % srv->vad_false_counter_length);
	history_false = srv->false_alarm_buff[(srv->cnt_index_fals + 1) % srv->vad_false_counter_length];

	if (dt_flag==0 && apm_flag==0)
    {
		float certainty_level = 0;

		dios_ssp_vad_mix_process_subfunc(srv, signaldev, history_certainty, certainty_level);

		srv->false_alarm_cnt -= history_false;
		srv->false_alarm_buff[srv->cnt_index_fals] = 0;
		srv->cnt_index_fals++;
	}
	else if (dt_flag == 0 && apm_flag == 1)
    {
		float certainty_level = 0;
		
		dios_ssp_vad_mix_process_subfunc(srv, signaldev, history_certainty, certainty_level);

		if (signaldev > 50 && signaldev < 80)
        {
			srv->false_alarm_cnt += 1 - history_false;
			srv->false_alarm_buff[srv->cnt_index_fals] = 1;
			srv->cnt_index_fals++;
		}
        else
        {
			srv->false_alarm_cnt -= history_false;
			srv->false_alarm_buff[srv->cnt_index_fals] = 0;
			srv->cnt_index_fals++;
		}

		if (srv->vad_certainty_cnt > 25)
        {
			*vad_flag = 1;
		}
		if (srv->false_alarm_cnt >= FALSE_ALARM_CNT_THLD)
        {
			*vad_flag = 0;
		}
	}
	else if (dt_flag == 2 && apm_flag == 1)
    {
		float certainty_level = 2;

		if (signaldev < 50)
        {
			certainty_level = 0.5;
		}
        
		dios_ssp_vad_mix_process_subfunc(srv, signaldev, history_certainty, certainty_level);

		if (signaldev > 50 && signaldev < 80)
        {
			srv->false_alarm_cnt += 1 - history_false;
			srv->false_alarm_buff[srv->cnt_index_fals] = 1;
			srv->cnt_index_fals++;
		}
        else
        {
			srv->false_alarm_cnt -= history_false;
			srv->false_alarm_buff[srv->cnt_index_fals] = 0;
			srv->cnt_index_fals++;
		}

		if (srv->vad_certainty_cnt > srv->vad_certainty_cnt_thld)
        {
			*vad_flag = 1;
		}
        if (srv->false_alarm_cnt >= FALSE_ALARM_CNT_THLD)
        {
			*vad_flag = 0;
		}
	}
	else if (dt_flag == 1 && apm_flag == 1)
    {
		float certainty_level = 2;
		if(signaldev < 50)
        {
			certainty_level = 0.5;
		}
        
		dios_ssp_vad_mix_process_subfunc(srv, signaldev, history_certainty, certainty_level);

		if (srv->vad_certainty_cnt > srv->vad_certainty_cnt_thld)
        {
			*vad_flag = 1;
		}

		srv->false_alarm_cnt -= history_false;
		srv->false_alarm_buff[srv->cnt_index_fals] = 0;
		srv->cnt_index_fals++;
	}
	else if (dt_flag == 1 && apm_flag == 0)
    {
		float certainty_level = 0;
		srv->vad_certainty_cnt += certainty_level - history_certainty;
		srv->vad_certainty_cnt = ((srv->vad_certainty_cnt < 0) ? 0 : srv->vad_certainty_cnt);
		srv->certainty_buff[srv->cnt_index_cert] = certainty_level;
		srv->cnt_index_cert++;

		if (srv->vad_certainty_cnt > CERTAINTY_SUM_THRESH)
        {
			*vad_flag = 1;
		}

		srv->false_alarm_cnt -= history_false;
		srv->false_alarm_buff[srv->cnt_index_fals] = 0;
		srv->cnt_index_fals++;
	}
	else if (dt_flag == 2 && apm_flag == 0)
    {
		float certainty_level = 0;
		
		dios_ssp_vad_mix_process_subfunc(srv, signaldev, history_certainty, certainty_level);

		srv->false_alarm_cnt -= history_false;
		srv->false_alarm_buff[srv->cnt_index_fals] = 0;
		srv->cnt_index_fals++;

		if (srv->vad_certainty_cnt > 25)
        {
			*vad_flag = 1;
		}
		if (srv->false_alarm_cnt >= FALSE_ALARM_CNT_THLD)
        {
			*vad_flag = 0;
		}
		else
        {
			return -1;
		}
	}
	return 0;
}

int dios_ssp_vad_mix_stric_process(int apm_flag, int dt_flag, void* vad_counter_handle, float signaldev, int* vad_flag, int state)
{
	VAD_COUNTER *srv;
	srv = (VAD_COUNTER *) vad_counter_handle;
	float history_certainty = 0;
	float history_false = 0;
	*vad_flag = 0;
	if (state == 1)
    {
		srv->vad_certainty_cnt_thld *= CERTAINTY_SMOOTH_RATE;
		srv->vad_certainty_cnt_thld = ((srv->vad_certainty_cnt_thld < CERTAINTY_SUM_THRESH_LOW) ? CERTAINTY_SUM_THRESH_LOW : srv->vad_certainty_cnt_thld);
	}
    else 
    {
        srv->vad_certainty_cnt_thld = CERTAINTY_SUM_THRESH;
    }
	srv->cnt_index_cert = (srv->cnt_index_cert % srv->vad_certainty_counter_length);
	history_certainty = srv->certainty_buff[(srv->cnt_index_cert + 1) % srv->vad_certainty_counter_length];
	srv->cnt_index_fals = (srv->cnt_index_fals % srv->vad_false_counter_length);
	history_false = srv->false_alarm_buff[(srv->cnt_index_fals + 1) % srv->vad_false_counter_length];

	if (dt_flag == 0 && apm_flag == 0)
    {
		float certainty_level = 0;

		dios_ssp_vad_mix_process_subfunc(srv, signaldev, history_certainty, certainty_level);

		srv->false_alarm_cnt -= history_false;
		srv->false_alarm_buff[srv->cnt_index_fals] = 0;
		srv->cnt_index_fals++;
	}
	else if (dt_flag == 0 && apm_flag == 1)
    {
		float certainty_level = 0;

		dios_ssp_vad_mix_process_subfunc(srv, signaldev, history_certainty, certainty_level);

		if (signaldev > 50 && signaldev < 80)
        {
			srv->false_alarm_cnt += 1 - history_false;
			srv->false_alarm_buff[srv->cnt_index_fals] = 1;
			srv->cnt_index_fals++;
		}
        else
        {
			srv->false_alarm_cnt -= history_false;
			srv->false_alarm_buff[srv->cnt_index_fals] = 0;
			srv->cnt_index_fals++;
		}
	}
	else if (dt_flag == 2 && apm_flag == 1)
    {
		float certainty_level = 2;

		if (signaldev < 50)
        {
			certainty_level = 0.5;
		}
        
		dios_ssp_vad_mix_process_subfunc(srv, signaldev, history_certainty, certainty_level);

		if (signaldev > 50 && signaldev < 80)
        {
			srv->false_alarm_cnt += 1 - history_false;
			srv->false_alarm_buff[srv->cnt_index_fals] = 1;
			srv->cnt_index_fals++;
		}
        else
        {
			srv->false_alarm_cnt -= history_false;
			srv->false_alarm_buff[srv->cnt_index_fals] = 0;
			srv->cnt_index_fals++;
		}

		if (srv->vad_certainty_cnt > srv->vad_certainty_cnt_thld)
        {
			*vad_flag = 1;
		}
        if (srv->false_alarm_cnt >= FALSE_ALARM_CNT_THLD)
        {
			*vad_flag = 0;
		}
	}
	else if (dt_flag == 1 && apm_flag == 1)
    {
		float certainty_level = 2;
		if(signaldev < 50)
        {
			certainty_level = 0.5;
		}
        
		dios_ssp_vad_mix_process_subfunc(srv, signaldev, history_certainty, certainty_level);

		if (srv->vad_certainty_cnt > srv->vad_certainty_cnt_thld)
        {
			*vad_flag = 1;
		}

		srv->false_alarm_cnt -= history_false;
		srv->false_alarm_buff[srv->cnt_index_fals] = 0;
		srv->cnt_index_fals++;
	}
	else if (dt_flag == 1 && apm_flag==0)
    {
		float certainty_level = 0;
		srv->vad_certainty_cnt += certainty_level - history_certainty;
		srv->vad_certainty_cnt = ((srv->vad_certainty_cnt < 0) ? 0 : srv->vad_certainty_cnt);
		srv->certainty_buff[srv->cnt_index_cert] = certainty_level;
		srv->cnt_index_cert++;
		srv->false_alarm_cnt -= history_false;
		srv->false_alarm_buff[srv->cnt_index_fals] = 0;
		srv->cnt_index_fals++;
	}
	else if (dt_flag == 2 && apm_flag == 0)
    {
		float certainty_level = 0;
		
		dios_ssp_vad_mix_process_subfunc(srv, signaldev, history_certainty, certainty_level);

		srv->false_alarm_cnt -= history_false;
		srv->false_alarm_buff[srv->cnt_index_fals] = 0;
		srv->cnt_index_fals++;

		if (srv->false_alarm_cnt >= FALSE_ALARM_CNT_THLD)
        {
			*vad_flag = 0;
		}
		else
        {
			return -1;
		}
	}
	return 0;
}

void dios_ssp_vad_smooth(int* vad_flag, void* vad_counter_handle, int* vad_state)
{
	VAD_COUNTER *srv;
	srv = (VAD_COUNTER *) vad_counter_handle;

	int vad_temp = *vad_flag;
	if (*vad_flag == 1) 
    {
        srv->continuous_0_cnt = 0;
    }
	else 
    {
        srv->continuous_0_cnt++;
    }

	if (srv->continuous_0_cnt < SMOOTH_0_CNT_THLD && *vad_flag == 0 && *vad_state == 1) 
    {
        vad_temp = 1;
    }
	else if (srv->continuous_0_cnt == SMOOTH_0_CNT_THLD) 
    {
        srv->continuous_1_cnt = 0;
    }
	else if (srv->continuous_0_cnt > SMOOTH_0_CNT_THLD) 
    {
        *vad_state = 0;
    }

	if (vad_temp == 1) 
    {
        srv->continuous_1_cnt++;
    }
	else 
    {
        srv->continuous_1_cnt = 0;
    }

	if (srv->continuous_1_cnt >= SMOOTH_1_CNT_THLD) 
    {
        *vad_state = 1;
    }
	
	*vad_flag = vad_temp;      // to reduce speech destroyed under voice state

	return;
}

void dios_ssp_vad_counter_uinit(void* vad_counter_handle)
{
	if (vad_counter_handle == NULL) 
    {
		return;
	}
	VAD_COUNTER *srv;
	srv = (VAD_COUNTER *)vad_counter_handle;

	free(srv->certainty_buff);
	free(srv->false_alarm_buff);
	free(srv);
}

