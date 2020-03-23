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

Description: Automatic Gain Control(AGC) determines the gain factor based on 
current frame signal level and the target level so that the gain of the signal 
is kept within a reasonable range.
Function framework:
1) calaulate AGC gain which is determined by the ratio of the target level to 
   the maximum level of the current frame, and the ratio of the target energy
   to the maximum energy.
2) sort the energy gain coefficients of the last ten frames.
3) ten frames are smoothed once, and the smaller values of energy gain 
   coefficients(final_gain) and level gain coefficients(noclip_gain) 
   are selected.
==============================================================================*/

#include "dios_ssp_agc_api.h"

typedef struct {
    int frame_len;
	float peak_val;        // the peak of the current frame
	int peak_hold_cnt;     // hold the peak value for a few frames before decaying
	int peak_hold_time;    // hold the peak value a few frames
	float peak_smooth_fac; // peak smoothing rate
	float gain_smooth_fac;
	short first_flag;
	float def_max_peak_val;
	float defs_max_rms_enrg;
	float defs_clip_val;
} objGainCalc;

typedef struct {
	short hold_frame_cnt;
	short min_gain_track_len;
	float gain_min;
	float gain_max;
	float agc_gain;
	float gain_min_delay;
	float gain_smooth_fac;
	float gain_decay_fac;
	float gain_min_tmp;
	short first_flag;
	int no_sig_cnt;
	float gain_min_global;
	float kws_gain_min;
	float kws_gain_max;

	int mode_type;
	volatile int vol_kws_agc_enable;
	volatile float vol_kws_gain;
	volatile float vol_kws_agc_gain;
	volatile float vol_kws_gain_min;
	volatile float vol_kws_gain_max;
	volatile float vol_kws_gain_min_delay;
	volatile float vol_kws_gain_min_tmp;
	volatile short vol_kws_hold_frame_cnt;
}objGainSmooth;

typedef struct {
	int cache_len;
	int max_ushort;
	int max_kws_word_num;

	float* kws_gain_buffer;
	float* kws_gain_buffer_sort;  //max kws num * 1
	float** each_kws_gain_buffer;
	
	int seq_num;
	int tick_dis_now;
	int tick_kws_now;
	int tick_kws_last;
	
	int* tick_kws_start;
	int* tick_kws_end;
	int* tick_kws_len;

	int median_filter_len;
	float** kws_gain_min_filter;
	float** kws_gain_max_filter;

	float* kws_gain;
	float* kws_gain_max;
	float* kws_gain_min;
}objWakeupGainInfo;

typedef struct {
	int block_len;
	int frame_len;
	float gain_agc_delay;
	float gain_agc;
	int *vad_buffer;
	float *gain_agc_buffer;
	float *gain_sort;
	int frame_index;
	int each_block_frame_num;
	float final_smooth_gain;
	float final_smooth_gain_fac;
	float max_gain;
	short beginning_silence_checking_flag;
	float silence_signal_thr;
	short beginning_proc_flag;
	int beginning_smooth_frm_cnt;
	float small_signal_thr;
	int useful_gain_sort_count;

	//direction vad use
	float gain_in_dir;
	float gain_decay;
	float gain_amp;
	float dir_gain_smooth_fac;
	int no_dir_vad_cnt;
	float gain_dir_return;

	objGainCalc *st_gc;
	objGainSmooth *st_gs;
	objWakeupGainInfo* st_wake;
	int first_flag;

	//use volatile type to store the set val	
	volatile float vol_kws_agc_gain_delay;
	volatile float vol_kws_agc_gain;
	volatile int vol_kws_enable_flag;
	int mode_type;
} objAGC;

// calculate the gain of each frame
int gain_calc(float *input_time, objGainCalc *st_gc, float* gain_buffer, float* noclipgain)
{
	float energy = 0;        // Sum(RxInput[]^2) over the current frame
	float maxabs_level = 0;
	float rms_energy = 0;
	float noclip_gain = 0;
	float rms_gain = 0;
	float peak_gain = 0.0f;
	float final_gain = 0;    // Max(|RxInput[]|) over the current frame
	float sample_value0 = 0;
	float sample_value1 = 0;
	float sample_value2 = 0;
	float sample_value3 = 0;
	int sample_index = 0;

	for (sample_index = 0; sample_index < st_gc->frame_len; sample_index += 4) 
    {
		sample_value0 = (float)input_time[sample_index];
		sample_value1 = (float)input_time[sample_index + 1];
		sample_value2 = (float)input_time[sample_index + 2];
		sample_value3 = (float)input_time[sample_index + 3];
		maxabs_level = xmax(maxabs_level, xabs(sample_value0));
		maxabs_level = xmax(maxabs_level, xabs(sample_value1));
		maxabs_level = xmax(maxabs_level, xabs(sample_value2));
		maxabs_level = xmax(maxabs_level, xabs(sample_value3));
		energy += ((sample_value0 * sample_value0) + (sample_value1 * sample_value1)) +
			((sample_value2 * sample_value2) + (sample_value3 * sample_value3));
	}
	rms_energy = xsqrt(energy / st_gc->frame_len);
	if (st_gc->first_flag == 1) 
    {
		st_gc->peak_val = maxabs_level;
		st_gc->first_flag = 0;
	}
	if (maxabs_level > st_gc->peak_val) 
    {
		st_gc->peak_val = maxabs_level;
		st_gc->peak_hold_cnt = 0;
	}
	else if (st_gc->peak_hold_cnt++ > st_gc->peak_hold_time) 
    {
		st_gc->peak_val = xsmooth_proc(st_gc->peak_val, st_gc->peak_smooth_fac, maxabs_level);
	}

	noclip_gain = st_gc->defs_clip_val / (1.0f + st_gc->peak_val);
	rms_gain = st_gc->defs_max_rms_enrg / (1.0f + rms_energy);
	peak_gain = st_gc->def_max_peak_val / (1.0f + st_gc->peak_val);
	final_gain = xmin(peak_gain, rms_gain);
	gain_buffer[0] = final_gain;
	noclipgain[0] = noclip_gain;
	return 0;
}

void gain_sort(float *a, int i, int j)
{
	int m = 0;
	int n = 0;
	float temp = 0;
	float k = 0;
	m = i;
	n = j;
	k = a[(i + j) / 2];
	do {
		while (a[m] < k && m < j) {
			m++;
		}
		while (a[n] > k && n > i) {
			n--;
		}
		if (m <= n) {
			temp = a[m];
			a[m] = a[n];
			a[n] = temp;
			m++;
			n--;
		}
	} while (m <= n);
	if (m < j) {
		gain_sort(a, m, j);
	}
	if (n > i) {
		gain_sort(a, i, n);
	}
}

float gain_smooth(float gain, objGainSmooth *st_gs, float gain_delay, float vadsum, int each_block_frame_num)
{
	float gain_final = 1.0;
	if (vadsum > (float)(2.0f * (float)each_block_frame_num) / 3.0f) 
    {
		if (gain < st_gs->gain_min / 5.0f && st_gs->first_flag == 0) 
        {
			gain = st_gs->gain_smooth_fac * st_gs->gain_min + (1 - st_gs->gain_smooth_fac) * gain;
		}
		if ((gain < st_gs->gain_min)) 
        {
			st_gs->gain_min = gain;
			st_gs->hold_frame_cnt = 0;
			st_gs->gain_min_tmp = st_gs->gain_max;
		}
		else
        {
			st_gs->hold_frame_cnt++;
		}

		if ((st_gs->hold_frame_cnt >(st_gs->min_gain_track_len >> 1)) && (gain < st_gs->gain_min_tmp)) 
        {   
			st_gs->gain_min_tmp = gain;
		}

		if ((st_gs->hold_frame_cnt > ((3 * st_gs->min_gain_track_len) >> 1))) 
        {
			st_gs->gain_min = st_gs->gain_min_tmp;
			st_gs->hold_frame_cnt = (st_gs->min_gain_track_len >> 1);
			st_gs->gain_min_tmp = st_gs->gain_max;
		}
		
		if (st_gs->first_flag == 1 && st_gs->mode_type != 2) 
        {
			st_gs->agc_gain = st_gs->gain_min;
			st_gs->first_flag = 0;
		}

		if (st_gs->mode_type == 2 && st_gs->gain_min <= st_gs->agc_gain) 
        {
			st_gs->agc_gain = st_gs->gain_decay_fac * st_gs->agc_gain + (1 - st_gs->gain_decay_fac) * st_gs->gain_min;
		}
		else 
        {
			st_gs->agc_gain = st_gs->gain_smooth_fac * st_gs->agc_gain + (1 - st_gs->gain_smooth_fac) * st_gs->gain_min;
		}
		gain_final = st_gs->agc_gain;
		st_gs->no_sig_cnt = 0;
	}
	else 
    { 
		gain_final = gain_delay;
	}

	return(gain_final);
}

void* dios_ssp_agc_init_api(int frame_len, float peak_val, int mode_type)
{
    int i,k;

	void* ptr = NULL;
	ptr = (void*)calloc(1,sizeof(objAGC));
	objAGC *srv;
	srv = (objAGC *)ptr;
	srv->mode_type = mode_type;
	srv->vol_kws_enable_flag = 0;
	srv->block_len = 1280;
	srv->frame_len = frame_len;
	srv->dir_gain_smooth_fac = 0.9f;  //80ms 
	srv->no_dir_vad_cnt = 0;
	srv->gain_dir_return = 1.0f;
	srv->gain_agc_delay = 1.0f;
	srv->gain_agc = 1.0f;
	srv->each_block_frame_num = srv->block_len / (srv->frame_len);
	srv->final_smooth_gain = 1.0f;
	srv->final_smooth_gain_fac = 0.8f;
	srv->max_gain = 100.0f;
	srv->beginning_silence_checking_flag = 1;
	srv->silence_signal_thr = 10.0f;
	srv->beginning_proc_flag = 1;
	srv->beginning_smooth_frm_cnt = 3;
	srv->small_signal_thr = 100.0f;
	srv->useful_gain_sort_count = 0;


	srv->vad_buffer = (int *)calloc(srv->each_block_frame_num, sizeof(int));
	srv->gain_agc_buffer = (float *)calloc(srv->each_block_frame_num, sizeof(float));
	srv->gain_sort = (float *)calloc(srv->each_block_frame_num, sizeof(float));
	for (i = 0; i < srv->each_block_frame_num; i++)
	{
		srv->vad_buffer[i] = 0;
		srv->gain_agc_buffer[i] = 1.0f;
		srv->gain_sort[i] = 1.0f;
	}
	srv->frame_index = 0;
	srv->first_flag = 1;

	srv->st_gc = (objGainCalc *)calloc(1, sizeof(objGainCalc));
	srv->st_gc->def_max_peak_val = peak_val;
	srv->st_gc->defs_max_rms_enrg = 0.70710678118655f * 32767.0f * 0.7f;
	srv->st_gc->defs_clip_val = 26000.0f;
	srv->st_gc->frame_len = srv->frame_len;
	srv->st_gc->peak_val = 0;
	srv->st_gc->peak_hold_cnt = 0;
	srv->st_gc->peak_hold_time = (int)(640 / srv->st_gc->frame_len);
	srv->st_gc->peak_smooth_fac = xsmooth_factor(320.0f / (float)srv->st_gc->frame_len);
	srv->st_gc->gain_smooth_fac = xsmooth_factor(8.0f);
	srv->st_gc->first_flag = 1;
	
	srv->st_gs = (objGainSmooth *)calloc(1, sizeof(objGainSmooth));
	srv->st_gs->gain_min = 3000.0f;
	srv->st_gs->gain_max = 3000.0f;
	srv->st_gs->gain_min_tmp = 3000.0f;
	srv->st_gs->hold_frame_cnt = 0;
	srv->st_gs->agc_gain = 1.0f;
	srv->st_gs->gain_min_delay = srv->st_gs->gain_min;
	srv->st_gs->first_flag = 1;
	srv->st_gs->no_sig_cnt = 0;
	srv->st_gs->gain_min_global = 3000.0f;
	
	srv->st_gs->min_gain_track_len = 100;
	srv->st_gs->gain_smooth_fac = 0.9115f;
	srv->st_gs->mode_type = srv->mode_type;
	if (srv->mode_type == 2) 
    {
		srv->st_gs->min_gain_track_len = 120;    //960ms
		srv->st_gs->gain_smooth_fac = 0.99601f;  //2s
		srv->st_gs->gain_decay_fac = 0.9115f;    //80ms
	}
	//wakeup gain in gain smooth
	srv->st_gs->kws_gain_max = 1.0f;
	srv->st_gs->kws_gain_min = 1.0f;
	srv->st_gs->vol_kws_agc_enable = 0;
	//wakeup drive agc 
	srv->st_wake = NULL;
	
	//init wakeup buffer gain and var.
	srv->st_wake = (objWakeupGainInfo*)calloc(1, sizeof(objWakeupGainInfo));
	srv->st_wake->cache_len = 1000;      //buffer the nearest 1000 frames gain = 8s
	srv->st_wake->max_ushort = 65535;
	srv->st_wake->max_kws_word_num = 10;
	srv->st_wake->seq_num = 0;
	srv->st_wake->tick_dis_now = 0;
	srv->st_wake->tick_kws_now = 0;
	srv->st_wake->tick_kws_last = 0;
	srv->st_wake->median_filter_len = 5; //median filter len
	srv->st_wake->kws_gain_buffer = (float*)calloc(srv->st_wake->cache_len, sizeof(float));
	srv->st_wake->each_kws_gain_buffer = (float**)calloc(srv->st_wake->max_kws_word_num, sizeof(float*));
	for (k = 0; k < srv->st_wake->max_kws_word_num; k++) 
    {
		srv->st_wake->each_kws_gain_buffer[k] = (float*)calloc(srv->st_wake->cache_len, sizeof(float));
	}

	srv->st_wake->kws_gain_buffer_sort = (float*)calloc(srv->st_wake->max_kws_word_num, sizeof(float));

	srv->st_wake->tick_kws_start = (int*)calloc(srv->st_wake->max_kws_word_num, sizeof(int));
	srv->st_wake->tick_kws_end = (int*)calloc(srv->st_wake->max_kws_word_num, sizeof(int));
	srv->st_wake->tick_kws_len = (int*)calloc(srv->st_wake->max_kws_word_num, sizeof(int));

	srv->st_wake->kws_gain = (float*)calloc(srv->st_wake->max_kws_word_num, sizeof(float));
	srv->st_wake->kws_gain_max = (float*)calloc(srv->st_wake->max_kws_word_num, sizeof(float));
	srv->st_wake->kws_gain_min = (float*)calloc(srv->st_wake->max_kws_word_num, sizeof(float));
	
	srv->st_wake->kws_gain_min_filter = (float**)calloc(srv->st_wake->max_kws_word_num, sizeof(float*));
	srv->st_wake->kws_gain_max_filter = (float**)calloc(srv->st_wake->max_kws_word_num, sizeof(float*));
	for (k = 0; k < srv->st_wake->max_kws_word_num; k++) 
    {
		srv->st_wake->kws_gain_min_filter[k] = (float*)calloc(srv->st_wake->median_filter_len, sizeof(float));
		srv->st_wake->kws_gain_max_filter[k] = (float*)calloc(srv->st_wake->median_filter_len, sizeof(float));
	}
	
    return(ptr);
}

int dios_ssp_agc_reset_api(void* ptr)
{
	if (NULL == ptr) 
    {
		return ERROR_AGC;
	}

    int i, k;

	objAGC *srv = (objAGC *)ptr;
	srv->vol_kws_enable_flag = 0;
	srv->no_dir_vad_cnt = 0;
	srv->gain_dir_return = 1.0f;
	srv->gain_agc = 1.0;
	srv->gain_agc_delay = 1.0;
	srv->frame_index = 0;
	srv->first_flag = 1;

	srv->final_smooth_gain = 1.0f;
	srv->final_smooth_gain_fac = 0.8f;
	srv->max_gain = 100.0f;
	srv->beginning_silence_checking_flag = 1;
	srv->silence_signal_thr = 10.0f;
	srv->beginning_proc_flag = 1;
	srv->beginning_smooth_frm_cnt = 3;
	srv->small_signal_thr = 100.0f;
	srv->useful_gain_sort_count = 0;

	srv->st_gc->peak_val = 0;
	srv->st_gc->peak_hold_cnt = 0;
	srv->st_gc->first_flag = 1;
	srv->st_gs->gain_min = 3000.0;
	srv->st_gs->gain_max = 3000.0;
	srv->st_gs->gain_min_tmp = 3000.0;
	srv->st_gs->hold_frame_cnt = 0;
	srv->st_gs->agc_gain = 1.0;
	srv->st_gs->gain_min_delay = 3000.0;
	srv->st_gs->first_flag = 1;
	srv->st_gs->no_sig_cnt = 0;
	srv->st_gs->gain_min_global = 3000.0;
	srv->st_gs->kws_gain_max = 1.0f;
	srv->st_gs->kws_gain_min = 1.0f;
	srv->st_gs->vol_kws_agc_enable = 0;
	for (i = 0; i < srv->each_block_frame_num; i++) 
    {
		srv->vad_buffer[i] = 0;
		srv->gain_agc_buffer[i] = 1.0f;
		srv->gain_sort[i] = 1.0f;
	}

	//wakeup control agc reset
	srv->st_wake->max_kws_word_num = 10;
	srv->st_wake->seq_num = 0;
	srv->st_wake->tick_dis_now = 0;

	srv->st_wake->tick_kws_now = 0;
	srv->st_wake->tick_kws_last = 0;
	srv->st_wake->median_filter_len = 5;
	memset(srv->st_wake->kws_gain_buffer, 0, srv->st_wake->cache_len * sizeof(float));
	memset(srv->st_wake->kws_gain_buffer_sort, 0, srv->st_wake->max_kws_word_num * sizeof(float));
	for (k = 0; k < srv->st_wake->max_kws_word_num; k++) 
    {
		memset(srv->st_wake->each_kws_gain_buffer[k], 0, srv->st_wake->cache_len * sizeof(float));
	}
	memset(srv->st_wake->tick_kws_start, 0, srv->st_wake->max_kws_word_num * sizeof(int));
	memset(srv->st_wake->tick_kws_end, 0, srv->st_wake->max_kws_word_num * sizeof(int));
	memset(srv->st_wake->tick_kws_len, 0, srv->st_wake->max_kws_word_num * sizeof(int));

	memset(srv->st_wake->kws_gain, 0, srv->st_wake->max_kws_word_num * sizeof(float));
	memset(srv->st_wake->kws_gain_max, 0, srv->st_wake->max_kws_word_num * sizeof(float));
	memset(srv->st_wake->kws_gain_min, 0, srv->st_wake->max_kws_word_num * sizeof(float));
	
	for (k = 0; k < srv->st_wake->max_kws_word_num; k++) 
    {
		memset(srv->st_wake->kws_gain_min_filter[k], 0, srv->st_wake->median_filter_len * sizeof(float));
		memset(srv->st_wake->kws_gain_max_filter[k], 0, srv->st_wake->median_filter_len * sizeof(float));
	}
	return 0;
}

int dios_ssp_agc_process_api(void* ptr, float* io_buf, int vad_sig, int vad_dir, int dt_st)
{
	objAGC *srv = (objAGC *)ptr;
	float final_gain = 0.0f;
	float noclip_gain = 0.0f;
	float gain_curr = 0.0f;
	int i, k = 0;
	float vadsum = 0.0f;
	float env = 0;
	float max_env = 0;
	int sort_idx;
	
	if (NULL == ptr)
    {
		return ERROR_AGC;
	}	

	//max envelope calculation for beginning process
	if ((srv->beginning_silence_checking_flag == 1) || (srv->beginning_proc_flag == 1))
	{
		for (i = 0; i < srv->frame_len; i++)
		{
			env = (float)fabs(io_buf[i]);
			if (env > max_env)
			{
				max_env = env;
			}
		}
	}

	//silence signal process directly return and not used for any other calculation
	if ((vad_sig == 0) && (srv->beginning_silence_checking_flag == 1) && (max_env < srv->silence_signal_thr))
	{
		return 0;
	}
	else
	{
		srv->beginning_silence_checking_flag = 0;
	}	

	srv->vad_buffer[srv->frame_index] = vad_sig;
	gain_calc(io_buf, srv->st_gc, &gain_curr, &noclip_gain);
	srv->gain_agc_buffer[srv->frame_index] = gain_curr;
	if (srv->frame_index == 0 && srv->first_flag == 1)
	{
		srv->first_flag = 0;
	}
	srv->frame_index++;
    srv->frame_index = (srv->frame_index) % (srv->each_block_frame_num);
	srv->useful_gain_sort_count++;
	if (srv->useful_gain_sort_count > srv->each_block_frame_num)
	{
		srv->useful_gain_sort_count = srv->each_block_frame_num;
	}
    memcpy(srv->gain_sort, srv->gain_agc_buffer, sizeof(float) * srv->each_block_frame_num);
	gain_sort(srv->gain_sort, 0, srv->each_block_frame_num - 1);

	// small signal process
	if ((srv->beginning_proc_flag == 1) && (max_env < srv->small_signal_thr))
	{
		return 0;
	}
	else
	{
		srv->beginning_proc_flag = 0;
	}

	if ((srv->first_flag == 0) && (vad_sig != 0))
    {
		for (k = 0; k < srv->each_block_frame_num; k++)
        {
			vadsum += (float)srv->vad_buffer[k];
		}

		sort_idx = srv->each_block_frame_num - srv->useful_gain_sort_count + 1;			
		if (sort_idx > srv->each_block_frame_num - 1)
		{
			sort_idx = srv->each_block_frame_num - 1;
		}
		final_gain = gain_smooth(srv->gain_sort[sort_idx], srv->st_gs, srv->gain_agc_delay, vadsum, srv->each_block_frame_num);

		if (srv->beginning_smooth_frm_cnt > 0)
		{
			sort_idx = srv->each_block_frame_num - srv->useful_gain_sort_count;
			if (sort_idx > srv->each_block_frame_num - 1)
			{
				sort_idx = srv->each_block_frame_num - 1;
			}
			srv->final_smooth_gain = srv->final_smooth_gain_fac * srv->final_smooth_gain + (1.0f - srv->final_smooth_gain_fac) * srv->gain_sort[sort_idx];
			srv->beginning_smooth_frm_cnt--;
		}
		else
		{
			srv->final_smooth_gain = srv->final_smooth_gain_fac * srv->final_smooth_gain + (1.0f - srv->final_smooth_gain_fac) * final_gain;			
		}
		
		srv->final_smooth_gain = xmin(srv->final_smooth_gain, noclip_gain);
		srv->final_smooth_gain = xmin(srv->final_smooth_gain, srv->max_gain);
		srv->gain_agc_delay = srv->final_smooth_gain;

		for (i = 0; i < srv->frame_len; i++)
		{
			io_buf[i] *= srv->final_smooth_gain;
		}
	}
	else 
    {
		final_gain = xmin(noclip_gain, srv->gain_agc_delay);

		if (srv->mode_type == 2 && dt_st == 0)
        {
			for (i = 0; i < srv->frame_len; i++) 
            {
				io_buf[i] *= 1.0f;
			}
		}
		else 
        {
			for (i = 0; i < srv->frame_len; i++) 
            {
				io_buf[i] *= final_gain;
			}
		}
	}
	
	return 0;
}

int dios_ssp_agc_uninit_api(void* ptr)
{
	if (NULL == ptr) 
    {
		return ERROR_AGC;
	}

    int k;

	objAGC *srv = (objAGC *)ptr;
	free(srv->st_gc);
	free(srv->st_gs);
	free(srv->vad_buffer);
	free(srv->gain_agc_buffer);
	free(srv->gain_sort);
	
    //wakeup control agc free
	for (k = 0; k < srv->st_wake->max_kws_word_num; k++) 
    {
		free(srv->st_wake->kws_gain_min_filter[k]);
		free(srv->st_wake->kws_gain_max_filter[k]);
	}
	free(srv->st_wake->tick_kws_start);
	free(srv->st_wake->tick_kws_end);
	free(srv->st_wake->tick_kws_len);
	free(srv->st_wake->kws_gain);
	free(srv->st_wake->kws_gain_max);
	free(srv->st_wake->kws_gain_min);
	free(srv->st_wake->kws_gain_min_filter);
	free(srv->st_wake->kws_gain_max_filter);
	free(srv->st_wake->kws_gain_buffer);
	free(srv->st_wake->kws_gain_buffer_sort);
	for (k = 0; k < srv->st_wake->max_kws_word_num; k++) 
    {
		free(srv->st_wake->each_kws_gain_buffer[k]);
	}
	free(srv->st_wake->each_kws_gain_buffer);
	free(srv->st_wake);
	free(srv);
	return 0;
}

