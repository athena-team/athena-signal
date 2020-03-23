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

Description: Subfunctions of the VAD module. Judging VAD through calculation of
energy.
==============================================================================*/

#include "dios_ssp_vad_energy.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../dios_ssp_share/dios_ssp_share_noiselevel.h"
#include "../dios_ssp_share/dios_ssp_share_rfft.h"
#include "dios_ssp_vad_macros.h"

#define FRM_LEN 128  
#define NUM_CHAN 20
#define VM_SIZE 90
#define TRUE    1
#define FALSE   0

typedef struct {
	int delay;                // 128 the overlap (or delay) in samples
	int fft_len;              // FFT length
	int lo_chan;              // the index of low channel
	int mid_chan;             // the index of middle channel
	int hi_chan;              // the index of high channel
	int metric_thld;          // the metric threshold
	int index_thld;           // the index threshold (4.5dB) - 0.375dB
	int setback_thld;         // the setback threshold (4.5dB) - 0.375dB
	int snr_thld;             // the SNR threshold (2.25dB)
	int index_cnt_thld;       // the index counter threshold
	int hyster_cnt_thld;      // the hyster counter threshold
	int update_cnt_thld;      // the update counter threshold
	float noise_floor_enrg;   // the noise floor energy
	float min_chan_enrg;      // the minimum allowable channel energy
	float min_noise_enrg;     // the minimum allowable noise enregy
	float ine_chan;           // the minimum allowable channel initialization energy
	float ine_noise;          // the minimum allowable channel noise initialization energy
	float high_tce_db;        // the high energy endpoint of total channel energy in db. background noise, low update
	float low_tce_db;         // the low energy endpoint of total channel energy in db. background noise, fast update
	float tce_range;
	float high_alpha;         // the high exponetial windowing factor
	float low_alpha;          // the low exponetial windowing factor
	float alpha_range;
	float alpha_ran_div_tce_ran; 
	float pre_emp_fac;        // pre_emphasis factor
	float de_emp_fac;         // de_emphasis factor
	float cee_sm_fac;         // the channel energy smoothing factor
	float cne_sm_fac;         // the channel noise energy smoothing factor
	float min_gain_db;        // the minimum overall gain in db
	float gain_slope;         // the gain slope
	float snr_const;          // the snr constant
	int max_ind_snr;          // the maximum index of snr
	int min_ind_snr;          // the minimum index of snr
	float speech_factor;      // the ratio of sub channel energy to full channel energy
	int max_speech_count;
	int max_no_sig_count;
	int speech_count_num;
	int speech_detect_num;
	int low_speech_ch;
	int high_speech_ch;
	int low_full_ch;
	int hpf_filter_len;
	float update_thld_smooth_rate1;    // the smooth rate for update_thld in 'single talk' case, decade from 4 to 2.5 in 400 msec
    float update_thld_smooth_rate2;    // the smooth rate for update_thld in 'single talk' case, decade from 4 to 2.5 in 400 msec
	float update_thld_smooth_rate3;    // the smooth rate for update_thld in 'double talk' case, decade from 4 to 2.1 in 400 msec
    int first_frame;          // the first_frame frame flag
    float pre_emp_data;
    float de_emp_data;        // used for pre / de_emphasis operations
    float *window_overlap;    // used for the overlap (or delay) in samples
    float *ch_enrg_long_db;   // the channel long-term energy in dB
    int update_count;
    int hyster_count;
    int last_update_count;    // some counters
    int frame_count;          // the frame counter
    float *ch_energy;         // the channel energy
    int voice_detect_state;
    int speech_detect_count;
    int no_voice_count;
    int voice_detect_state_count;
    float *ch_energy_db;      // the channel energy in dB
    float ch_enrg_dev;        // the energy (or spectral) deviation
    float *ch_noise;          // the channel noise energy
    int *ch_snr;              // the channel signal to noise ratio
    int vm_sum;               // the voice metric sum
    float vm_mean;	          // the voice metric mean
    float energy;
    float tne;
    float tce;
    float gain;
    float *in_buffer;
    float *out_buffer;        // the input / output buffers (floatpoints format)
    float alpha;              // the exponetial windowing factor
    int update_flag;
    int modify_flag;
    int index_count;
    float *window_coef;
    float *data_buffer;
    int	v_t;
    int nl_run_min_len;
    float update_thld;
	float dev_thld;                              
    objNoiseLevel *energyvad_noise_est;

    void *rfft_param;
	float *vad_fftbuf_in;
    float *vad_fft_out;
	xcomplex *vad_fftbuf_out;
	int ch_table[NUM_CHAN][2];
	int vm_table[VM_SIZE];

	// more stric VAD for communication case
	int update_flag_stric;
	float update_thld_stric;
	int update_count_stric;
	int last_update_count_stric;
	int hyster_count_stric;
	int voice_detect_state_stric;
	int voice_detect_state_count_stric;
	int speech_detect_count_stric;
	int no_voice_count_stric;
	int vad_result_stric;
} objENERGYVAD;

void* dios_ssp_energy_vad_init(int vad_type)
{
    void* energyvad_ptr = NULL;
    objENERGYVAD *vad_param;
    int j = 0;
    energyvad_ptr = (void*)calloc(1, sizeof(objENERGYVAD));
    vad_param = (objENERGYVAD *)energyvad_ptr;
    if (vad_type == 0) 
    {
        vad_param->update_thld = 2.3f;
        vad_param->nl_run_min_len = 200;
    } 
    else 
    {
        vad_param->update_thld = 1.5f;
        vad_param->nl_run_min_len = 200;
	}

	vad_param->update_flag_stric = FALSE;
	vad_param->update_thld_stric = 6.0f;
	vad_param->update_count_stric = 0;
	vad_param->last_update_count_stric = 0;
	vad_param->hyster_count_stric = 0;
	vad_param->voice_detect_state_stric = FALSE;
	vad_param->voice_detect_state_count_stric = 0;
	vad_param->speech_detect_count_stric = 0;
	vad_param->no_voice_count_stric = 0;
	vad_param->vad_result_stric = 0;

	vad_param->delay = 128;
	vad_param->fft_len = 256;
	vad_param->lo_chan = 0;
	vad_param->mid_chan = 5;
	vad_param->hi_chan = 19;
	vad_param->metric_thld = 3;
	vad_param->index_thld = 5;
	vad_param->setback_thld = 12;
	vad_param->snr_thld = 6;
	vad_param->index_cnt_thld = 5;
	vad_param->hyster_cnt_thld = 6;
	vad_param->update_cnt_thld = 50;
	vad_param->noise_floor_enrg = 1.0f;
	vad_param->min_chan_enrg = 0.0625f;
	vad_param->min_noise_enrg = 0.0625f;
	vad_param->ine_chan = 16.0f;
	vad_param->ine_noise = 16.0f;
	vad_param->high_tce_db = 50.0f;
	vad_param->low_tce_db = 30.0f;
	vad_param->tce_range = (vad_param->high_tce_db - vad_param->low_tce_db);
	vad_param->high_alpha = 0.99f;
	vad_param->low_alpha = 0.50f;
	vad_param->alpha_range = (vad_param->high_alpha - vad_param->low_alpha);
	vad_param->alpha_ran_div_tce_ran = (vad_param->alpha_range / vad_param->tce_range);
	vad_param->pre_emp_fac = -0.8f;
	vad_param->de_emp_fac = 0.8f;
	vad_param->cee_sm_fac = 0.45f;
	vad_param->cne_sm_fac = 0.9f;
	vad_param->min_gain_db = -8.0f;
	vad_param->gain_slope = 0.45f;
	vad_param->snr_const= 0.375f;
	vad_param->max_ind_snr = 89;
	vad_param->min_ind_snr = 0;
	vad_param->speech_factor = 0.5f;
	vad_param->max_speech_count = 50;
	vad_param->max_no_sig_count = 20;
	vad_param->speech_count_num = 10;
	vad_param->speech_detect_num = 5;
	vad_param->low_speech_ch = 3;
	vad_param->high_speech_ch = 16;
	vad_param->low_full_ch = 1;
	vad_param->hpf_filter_len = 4;
	vad_param->update_thld_smooth_rate1 = 0.9906f;
	vad_param->update_thld_smooth_rate2 = 0.9881f;        
	vad_param->update_thld_smooth_rate3 = 0.9872f;  

	int tmp_ch_tbl[NUM_CHAN][2] = {
		{2, 3},
		{4, 5},
		{6, 7},
		{8, 9}, //500hz
		{10, 11},
		{12, 13},
		{14, 16},
		{17, 19},
		{20, 22},
		{23, 26},
		{27, 30},
		{31, 34}, // 4
		{35, 39}, // 5
		{40, 45}, // 6
		{46, 52}, // 7--3250Hz
		{53, 61}, // 9
		{62, 72}, // 11
		{73, 87}, // 15
		{88, 104}, // 17
		{105, 127} // 23
	};
	memcpy(vad_param->ch_table, tmp_ch_tbl, NUM_CHAN * 2 * sizeof(int));
	int tmp_vm_tbl[VM_SIZE] = {
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		3, 3, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 7, 7, 7,
		8, 8, 9, 9, 10, 10, 11, 12, 12, 13, 13, 14, 15,
		15, 16, 17, 17, 18, 19, 20, 20, 21, 22, 23, 24,
		24, 25, 26, 27, 28, 28, 29, 30, 31, 32, 33, 34,
		35, 36, 37, 37, 38, 39, 40, 41, 42, 43, 44, 45,
		46, 47, 48, 49, 50, 50, 50, 50, 50, 50, 50, 50,
		50, 50
	};
	memcpy(vad_param->vm_table, tmp_vm_tbl, VM_SIZE * sizeof(int));

	// FFT initialization
	vad_param->vad_fftbuf_in = (float *)calloc(vad_param->fft_len, sizeof(float));
	vad_param->vad_fftbuf_out = (xcomplex *)calloc(vad_param->fft_len / 2 + 1, sizeof(xcomplex));
    vad_param->vad_fft_out = (float*)calloc(vad_param->fft_len, sizeof(float));
    vad_param->rfft_param = dios_ssp_share_rfft_init(VAD_FFT_LEN);

    // allocate memory space and initialize to zeros
    vad_param->window_overlap = (float *)calloc(vad_param->delay, sizeof(float));
    vad_param->ch_energy = (float *)calloc(NUM_CHAN, sizeof(float));
    vad_param->ch_noise = (float *)calloc(NUM_CHAN, sizeof(float));
    vad_param->ch_energy_db = (float *)calloc(NUM_CHAN, sizeof(float));
    vad_param->ch_enrg_long_db = (float *)calloc(NUM_CHAN, sizeof(float));
    vad_param->data_buffer = (float *)calloc(FRM_LEN + vad_param->delay, sizeof(float));
    vad_param->ch_snr = (int *)calloc(NUM_CHAN, sizeof(int));
    vad_param->window_coef = (float *)calloc(FRM_LEN + vad_param->delay, sizeof(float));
    vad_param->in_buffer = (float *)calloc(FRM_LEN, sizeof(float));
    vad_param->out_buffer = (float *)calloc(FRM_LEN, sizeof(float));

    vad_param->energyvad_noise_est = (objNoiseLevel *)calloc(1, sizeof(objNoiseLevel));
    dios_ssp_share_noiselevel_init(vad_param->energyvad_noise_est, 90000.0f, 0.00001f, vad_param->nl_run_min_len); 

    vad_param->voice_detect_state = FALSE;
    vad_param->speech_detect_count = 0;
    vad_param->no_voice_count = 0;
    vad_param->voice_detect_state_count = 0;
    vad_param->v_t = vad_type;

    if (vad_param->window_overlap == NULL || vad_param->ch_enrg_long_db == NULL || 
            vad_param->ch_energy == NULL || vad_param->ch_noise == NULL || 
            vad_param->data_buffer == NULL || vad_param->ch_snr == NULL ||
            vad_param->vad_fftbuf_in == NULL || vad_param->vad_fftbuf_out == NULL ||
            vad_param->ch_energy_db == NULL || vad_param->window_coef == NULL || 
            vad_param->in_buffer == NULL || vad_param->out_buffer == NULL) 
    {
        puts("Memory allocation error.\n");
        return NULL;
    }

    // compute the smoothed trapezoidal window_coef coefficients
    for (j = 0; j < vad_param->delay; j++) 
    {
        vad_param->window_coef[j] = (float)sin(PI * (j + 0.5) / (2 * vad_param->delay)) * 
            (float)sin(PI * (j + 0.5) / (2 * vad_param->delay));
    }
    for (j = vad_param->delay; j < FRM_LEN; j++) 
    {
        vad_param->window_coef[j] = 1.0;
    }
    for (j = FRM_LEN; j < FRM_LEN + vad_param->delay; j++) 
    {
        vad_param->window_coef[j] = (float)sin(PI * (j - FRM_LEN + vad_param->delay + 0.5) / (2 * vad_param->delay)) * 
            (float)sin(PI * (j - FRM_LEN + vad_param->delay + 0.5) / (2 * vad_param->delay));
    }
    for (j = 0; j < FRM_LEN + vad_param->delay; j++) 
    {
        vad_param->data_buffer[j] = 0.0;
    }
    vad_param->first_frame = TRUE;    // set the first_frame frame flag
    
    return energyvad_ptr;
}

void dios_ssp_energy_vad_reset(void* energyvad_ptr)
{
    objENERGYVAD *vad_param;
    int j = 0;

    vad_param = (objENERGYVAD *)energyvad_ptr;

    for (j = 0; j < FRM_LEN + vad_param->delay; j++) 
    {
        vad_param->data_buffer[j] = 0.0;
    }
    for (j = 0; j < vad_param->delay; j++) 
    {
        vad_param->window_overlap[j] = 0.0;
    }
    for (j = 0; j < NUM_CHAN; j++) 
    {
        vad_param->ch_energy[j] = 0.0;
        vad_param->ch_noise[j] = 0.0;
        vad_param->ch_energy_db[j] = 0.0;
        vad_param->ch_enrg_long_db[j] = 0.0;
        vad_param->ch_snr[j] = 0;
    }
    for (j = 0; j < vad_param->fft_len; j++) 
    {
		vad_param->vad_fftbuf_in[j] = 0.0;
        vad_param->vad_fft_out[j] = 0.0;
    }
	for (j = 0; j < vad_param->fft_len / 2 + 1; j++) 
    {
		vad_param->vad_fftbuf_out[j].r = 0.0;
		vad_param->vad_fftbuf_out[j].i = 0.0;
	}
    for (j = 0; j < FRM_LEN; j++) 
    {
        vad_param->in_buffer[j] = 0.0;
        vad_param->out_buffer[j] = 0.0;
    }
    vad_param->voice_detect_state = FALSE;
    vad_param->no_voice_count = 0;
    vad_param->voice_detect_state_count = 0;
    dios_ssp_share_noiselevel_init(vad_param->energyvad_noise_est, 90000.f, 0.00001f, vad_param->nl_run_min_len);              
    vad_param->first_frame = TRUE;
	vad_param->update_thld = 2.3f;           
	vad_param->update_flag = TRUE;
	vad_param->speech_detect_count = 0;
	vad_param->update_thld_stric = 6;
	vad_param->update_flag_stric = TRUE;
	vad_param->update_count_stric = 0;
	vad_param->last_update_count_stric = 0;
	vad_param->hyster_count_stric = 0;
	vad_param->voice_detect_state_stric = FALSE;
	vad_param->voice_detect_state_count_stric = 0;
	vad_param->speech_detect_count_stric = 0;
	vad_param->no_voice_count_stric = 0;
	vad_param->vad_result_stric = 0;
}

void dios_ssp_energy_vad_para_set(void* energyvad_ptr, enum TALK_STATE talk_state, int state)
{
	objENERGYVAD *vad_param;
    vad_param = (objENERGYVAD *)energyvad_ptr;
	switch (talk_state)
    {
	case nearend_state:
		vad_param->dev_thld = 40.0;              // signal deviation
		if(state == 1)
        {
			vad_param->update_thld *= vad_param->update_thld_smooth_rate2;            // compare to vm_mean
			if(vad_param->update_thld <= 2.2) 
            {
                vad_param->update_thld = 2.2f;
            }
		}
        else 
        {
            vad_param->update_thld = 2.4f;
        }
		break;
	case doubletalk_state:
		vad_param->dev_thld = 40.0;              // signal deviation
		if(state == 1)
        {
			vad_param->update_thld *= vad_param->update_thld_smooth_rate3;            // compare to vm_mean
			if(vad_param->update_thld <= 2.1) 
            {
                vad_param->update_thld = 2.1f;
            }
		}
        else 
        {
            vad_param->update_thld = 2.3f;
        }
		break;
	case singletalk_state:
		vad_param->dev_thld = 40.0;              // signal deviation       60
		if(state == 1)
        {
			vad_param->update_thld *= vad_param->update_thld_smooth_rate1;            // compare to vm_mean
			if(vad_param->update_thld <= 2.5) 
            {
                vad_param->update_thld = 2.5f;
            }
		}
        else 
        {
            vad_param->update_thld = 2.6f;
        }
		break;	
	}
}

int dios_ssp_energy_vad_process(void* energyvad_ptr, float *inbuf)
{
    objENERGYVAD *vad_param;
    int i = 0;
    int j = 0;
    int j1 = 0;
    int j2 = 0;
    int tmpi = 0;
    float tmpf = 0;
    float energ = 0.0f;
    float *nl_data = NULL;
    nl_data = (float *)calloc(FRM_LEN, sizeof(float));
    energ = 0;
    for (j = 0; j < FRM_LEN; j++)
    {
        nl_data[j] = (float)inbuf[j];
        energ += nl_data[j] * nl_data[j];
    }
    energ /= FRM_LEN;
    free(nl_data);
    vad_param = (objENERGYVAD *)energyvad_ptr;

    int nlvad = dios_ssp_share_noiselevel_process(vad_param->energyvad_noise_est, energ);
    
    // init the channel gains one time
    if (vad_param->first_frame == TRUE) 
    {
        for (i = vad_param->lo_chan; i <= vad_param->hi_chan; i++) 
        {
            vad_param->ch_energy[i] = 0.0;
        }
        vad_param->pre_emp_data = 0.0;
        vad_param->de_emp_data = 0.0;
        vad_param->update_count = 0;
        vad_param->frame_count = 0;
        vad_param->hyster_count = 0;
        vad_param->last_update_count = 0;
    }
    vad_param->frame_count++;

    for (i = 0; i < FRM_LEN; i++) 
    {
        vad_param->in_buffer[i] = (float)inbuf[i];
    }
    //iir_proc(vad_param->iir_notch_vad, vad_param->in_buffer, FRM_LEN);

    /*--------------------------------------*/
    /*	Frame & Window						*/
    /*--------------------------------------*/
    /*  		Frame 1	  Frame 2			*/
    /*      __|_________|__       |			*/
    /*     |___Window 1____|      |			*/
    /*	      |         |         |			*/
    /*    	  |       __|_________|__		*/
    /*	      |      |___Window 2____|		*/
    /*	      |         |         |			*/
    /*										*/
    /*--------------------------------------*/
    
    // preemphasize the input data and store in the data buffer with appropriate delay
    for (i = 0; i < vad_param->delay; i++) 
    {
        vad_param->data_buffer[i] = vad_param->window_overlap[i];
    }

    vad_param->data_buffer[vad_param->delay] = vad_param->in_buffer[0] + vad_param->pre_emp_fac * vad_param->pre_emp_data;

    for (i = vad_param->delay + 1, j = 1; i < vad_param->delay + FRM_LEN; i++, j++) 
    {
        vad_param->data_buffer[i] = vad_param->in_buffer[j] + vad_param->pre_emp_fac * vad_param->in_buffer[j - 1];
    }
    vad_param->pre_emp_data = vad_param->in_buffer[FRM_LEN - 1];

    // update window_overlap buffer
    for (i = 0, j = FRM_LEN; i < vad_param->delay; i++, j++) 
    {
        vad_param->window_overlap[i] = vad_param->data_buffer[j];
    }

    // apply window_coef to frame prior to FFT
    for (i = 0; i < FRM_LEN + vad_param->delay; i++)
    {
		vad_param->vad_fftbuf_in[i] = vad_param->data_buffer[i] * vad_param->window_coef[i];
    }

    for (i = FRM_LEN + vad_param->delay; i < vad_param->fft_len; i++) 
    {
		vad_param->vad_fftbuf_in[i] = 0.0;
    }

    // perform FFT on the 'tbuf', results in 'spec'
    dios_ssp_share_rfft_process(vad_param->rfft_param, vad_param->vad_fftbuf_in, vad_param->vad_fft_out);
    for (i = 0; i < VAD_SUBBAND_NUM; i++)
    {
	    vad_param->vad_fftbuf_out[i].r = vad_param->vad_fft_out[i];
    }
    vad_param->vad_fftbuf_out[0].i = vad_param->vad_fftbuf_out[VAD_SUBBAND_NUM - 1].i = 0.0;
    for (i = 1; i < VAD_SUBBAND_NUM - 1; i++)
    {
	    vad_param->vad_fftbuf_out[i].i = -vad_param->vad_fft_out[VAD_FFT_LEN - i];
    }
    
    // estimate the energy in each channel
    for (i = vad_param->lo_chan; i <= vad_param->hi_chan; i++) 
    {
        //vad_param->speech_enrg = 0.0;
        vad_param->energy = 0.0;
        j1 = vad_param->ch_table[i][0];
        j2 = vad_param->ch_table[i][1];

        for (j = j1; j <= j2; j++) 
        {
			vad_param->energy += vad_param->vad_fftbuf_out[j].r * vad_param->vad_fftbuf_out[j].r + vad_param->vad_fftbuf_out[j].i * vad_param->vad_fftbuf_out[j].i;
        }

        vad_param->energy /= (j2 - j1 + 1);

        if (vad_param->first_frame == TRUE) 
        {
            vad_param->ch_energy[i] = vad_param->energy;
        } 
        else 
        {
            vad_param->ch_energy[i] = vad_param->cee_sm_fac * vad_param->ch_energy[i] + 
                (1 - vad_param->cee_sm_fac) * vad_param->energy;
        }

        if (vad_param->ch_energy[i] < vad_param->min_chan_enrg) 
        {
            vad_param->ch_energy[i] = vad_param->min_chan_enrg;
        }
    }
    // initialize channel noise esitmate to channel energy of the first_frame four frames
    if (vad_param->frame_count <= 5) 
    {
        for (i = vad_param->lo_chan; i <= vad_param->hi_chan; i++) 
        {
            if (vad_param->ch_energy[i] < vad_param->ine_chan) 
            {
                vad_param->ch_noise[i] = vad_param->ine_noise;
            } 
            else 
            {
                vad_param->ch_noise[i] = vad_param->ch_energy[i];
            }
        }
    }
    // compute the channel SNR indices
    for (i = vad_param->lo_chan; i <= vad_param->hi_chan; i++) 
    {
        float ftmp = 0;
        ftmp = 10.0f * (float)log10(vad_param->ch_energy[i] / vad_param->ch_noise[i]);
        if (ftmp < vad_param->min_ind_snr) 
        {
            ftmp = (float)vad_param->min_ind_snr;
        }

        tmpi = (int)((ftmp + vad_param->snr_const / 2) / vad_param->snr_const);
        if (tmpi > vad_param->max_ind_snr) 
        {
            tmpi = vad_param->max_ind_snr;
        }

        vad_param->ch_snr[i] = tmpi;
    }

    // compute the sum of voice metrics
    vad_param->vm_sum = 0;
    for (i = vad_param->lo_chan; i <= vad_param->hi_chan; i++) 
    {
        j = vad_param->ch_snr[i];
        vad_param->vm_sum += vad_param->vm_table[j];
    }
    vad_param->vm_mean = (float)vad_param->vm_sum / (vad_param->hi_chan - vad_param->lo_chan + 1);

    // compute the total noise estimate (tne) and total channel energy estimate (tce)
    vad_param->tne = vad_param->tce = 0.0;
    for (i = vad_param->lo_chan; i <= vad_param->hi_chan; i++) 
    {
        vad_param->tne += vad_param->ch_noise[i];
        vad_param->tce += vad_param->ch_energy[i];
    }

    // calculate log spectral deviation
    for (i = vad_param->lo_chan; i <= vad_param->hi_chan; i++) 
    {
        vad_param->ch_energy_db[i] = 10.0f * (float)log10(vad_param->ch_energy[i]);
    }

    if (vad_param->first_frame == TRUE) 
    {
        for (i = vad_param->lo_chan; i <= vad_param->hi_chan; i++) 
        {
            vad_param->ch_enrg_long_db[i] = vad_param->ch_energy_db[i];
        }
    }

    vad_param->ch_enrg_dev = 0.0;
    for (i = vad_param->lo_chan; i <= vad_param->hi_chan; i++) 
    {
        vad_param->ch_enrg_dev += (float)fabs(vad_param->ch_energy_db[i] - vad_param->ch_enrg_long_db[i]);
    }

    /* calculate the exponential windowing factor as a function of total channel energy (tce) */
    /* i.e.   high tce (50 dB) -> slow integration (alpha = 0.99)                             */
    /*        low  tce (30 dB) -> fast integration (alpha = 0.50)                             */
    tmpf = 10.0f * (float)log10(vad_param->tce);
    vad_param->alpha = vad_param->high_alpha - vad_param->alpha_ran_div_tce_ran * (vad_param->high_tce_db - tmpf);

    if (vad_param->alpha > vad_param->high_alpha) 
    {
        vad_param->alpha = vad_param->high_alpha;
    } 
    else if (vad_param->alpha < vad_param->low_alpha) 
    {
        vad_param->alpha = vad_param->low_alpha;
    }

    // calculate the long term log spectral energy
    for (i = vad_param->lo_chan; i <= vad_param->hi_chan; i++) 
    {
        vad_param->ch_enrg_long_db[i] = vad_param->alpha * vad_param->ch_enrg_long_db[i] + 
            (1 - vad_param->alpha) * vad_param->ch_energy_db[i];
    }

    // set or reset the update flag
    // normal update logic
    vad_param->update_flag = FALSE;
    if (vad_param->vm_mean <= vad_param->update_thld) 
    {
        // update_thld: higher for more noise suppresion
        vad_param->update_flag = TRUE;
        vad_param->update_count = 0;
    }
    // forced update logic
    else if (vad_param->tce > vad_param->noise_floor_enrg && vad_param->ch_enrg_dev < vad_param->dev_thld) 
    {
        // dev_thld: the distinction of speech and background noise
        vad_param->update_count++;
        if (vad_param->update_count >= vad_param->update_cnt_thld) 
        {
            vad_param->update_flag = TRUE;
        }
    }

	// 'hysteresis' logic to prevent long-term creeping of update_count
    if (vad_param->update_count == vad_param->last_update_count) 
    {
        vad_param->hyster_count++;
    } 
    else 
    {
        vad_param->hyster_count = 0;
    }

    vad_param->last_update_count = vad_param->update_count;

    if (vad_param->hyster_count > vad_param->hyster_cnt_thld) 
    {
        vad_param->update_count = 0;
    }

	// more stric VAD for communication case
	vad_param->update_flag_stric = FALSE;
	if (vad_param->vm_mean <= vad_param->update_thld_stric) 
    {
        // update_thld: higher for more noise suppresion
        vad_param->update_flag_stric = TRUE;
        vad_param->update_count_stric = 0;
    }
    // forced update logic
    else if (vad_param->tce > vad_param->noise_floor_enrg && vad_param->ch_enrg_dev < vad_param->dev_thld) 
    {
        // dev_thld: the distinction of speech and background noise
        vad_param->update_count_stric++;
        if (vad_param->update_count_stric >= vad_param->update_cnt_thld) 
        {
            vad_param->update_flag_stric = TRUE;
        }
    }

    if (vad_param->update_count_stric == vad_param->last_update_count_stric) 
    {
        vad_param->hyster_count_stric++;
    } 
    else 
    {
        vad_param->hyster_count_stric = 0;
    }

    vad_param->last_update_count_stric = vad_param->update_count_stric;

    if (vad_param->hyster_count_stric > vad_param->hyster_cnt_thld) 
    {
        vad_param->update_count_stric = 0;
    }

    // set or reset modify flag
    vad_param->index_count = 0;

    // speech distortion, speech included above vad_param->index_thld, no modifiy
    for (i = vad_param->mid_chan; i <= vad_param->hi_chan; i++) 
    {
        if (vad_param->ch_snr[i] >= vad_param->index_thld) 
        {
            vad_param->index_count++;
        }
    }
    vad_param->modify_flag = (vad_param->index_count < vad_param->index_cnt_thld) ? TRUE : FALSE;
    // modify the SNR indices
    if (vad_param->modify_flag == TRUE) 
    {
        for (i = vad_param->lo_chan; i <= vad_param->hi_chan; i++) 
        {
            // vad_param->metric_thld: strong tune noise, higher vad_param->metric_thld bring more NS
            if ((vad_param->vm_mean <= vad_param->metric_thld) || (vad_param->ch_snr[i] <= vad_param->setback_thld)) 
            {
                vad_param->ch_snr[i] = 1;
            }
        }
    }
    // limit ch_snr to SNR threshold
    for (i = vad_param->lo_chan; i <= vad_param->hi_chan; i++) 
    {
        if (vad_param->ch_snr[i] < vad_param->snr_thld) 
        {
            vad_param->ch_snr[i] = vad_param->snr_thld;
        }
    }
    // compute the channel gains
    vad_param->gain = -10.0f * (float)log10(vad_param->tne);
    if (vad_param->gain < vad_param->min_gain_db) 
    {
        vad_param->gain = vad_param->min_gain_db;
    }
    // calculate channel gains in dB
    for (i = vad_param->lo_chan; i <= vad_param->hi_chan; i++) 
    {
        tmpf = vad_param->gain_slope * (vad_param->ch_snr[i] - vad_param->snr_thld) + vad_param->gain;
        if (tmpf > 0.0) 
        {
            tmpf = 0.0;
        }
        tmpf = (float)pow(10.0, tmpf / 20.0);
        j1 = vad_param->ch_table[i][0];
        j2 = vad_param->ch_table[i][1];
    }
    // update the channel noise estimates
    if ((vad_param->update_flag == TRUE) || (vad_param->energyvad_noise_est->noise_change_flag == 1 && vad_param->energyvad_noise_est->noise_change_update_flag)) 
    {
        for (i = vad_param->lo_chan; i <= vad_param->hi_chan; i++) 
        {
            vad_param->ch_noise[i] = vad_param->cne_sm_fac * vad_param->ch_noise[i] + (1 - vad_param->cne_sm_fac) * vad_param->ch_energy[i];
            if (vad_param->ch_noise[i] < vad_param->min_noise_enrg) 
            {
                vad_param->ch_noise[i] = vad_param->min_noise_enrg;
            }
        }
    }
    vad_param->first_frame = FALSE;

    vad_param->voice_detect_state = (1 - vad_param->update_flag) * nlvad;
	
    if ((vad_param->voice_detect_state_count < 30))
    {
        vad_param->voice_detect_state_count += vad_param->voice_detect_state;
    }
	if(vad_param->voice_detect_state_count > 0 && vad_param->voice_detect_state) 
    {
		vad_param->speech_detect_count = 10;                                       
	}
	else if(vad_param->speech_detect_count> 0)
    {
		vad_param->speech_detect_count--;
    }
    if (vad_param->voice_detect_state == TRUE) 
    {
        vad_param->no_voice_count = 0;
    } 
    else 
    {
        vad_param->no_voice_count++;
    } 
    if (vad_param->no_voice_count > 10)                                            
    {
        vad_param->speech_detect_count = 0;
		vad_param->voice_detect_state_count = 0;
    }
    
	// more stric VAD for communication case
	vad_param->voice_detect_state_stric = (1 - vad_param->update_flag_stric) * nlvad;    
	if ((vad_param->voice_detect_state_count_stric < 30))
    {
        vad_param->voice_detect_state_count_stric += vad_param->voice_detect_state_stric;
    }
	if(vad_param->voice_detect_state_count_stric > 0 && vad_param->voice_detect_state_stric) 
    {
		vad_param->speech_detect_count_stric = 10;                                       
	}
	else if(vad_param->speech_detect_count_stric > 0)
    {
		vad_param->speech_detect_count_stric--;
    }
    if (vad_param->voice_detect_state_stric == TRUE) 
    {
        vad_param->no_voice_count_stric = 0;
    } 
    else 
    {
        vad_param->no_voice_count_stric++;
    }
    if (vad_param->no_voice_count_stric > 10)                                            
    {
        vad_param->speech_detect_count_stric = 0;
		vad_param->voice_detect_state_count_stric = 0;
    }
	if (vad_param->v_t == 0) 
    {
		vad_param->vad_result_stric = (vad_param->speech_detect_count_stric > 0);
    } 
    else 
    {
        vad_param->vad_result_stric = (1 - vad_param->update_flag_stric);
    }

    if (vad_param->v_t == 0) 
    {
		return(vad_param->speech_detect_count > 0);
    } 
    else 
    {
        return(1 - vad_param->update_flag);
    }
}

int dios_ssp_energy_vad_stric_result_get(void* energyvad_ptr)
{
	objENERGYVAD *vad_param;
    vad_param = (objENERGYVAD *)energyvad_ptr;

	return vad_param->vad_result_stric;
}

void dios_ssp_energy_vad_para_get(void* energyvad_ptr, float *noisedev, float *noiselevel_second, float *noiselevel_first, float *vmmean)
{
	objENERGYVAD *vad_param;
    vad_param = (objENERGYVAD *)energyvad_ptr;
	*noisedev = vad_param->ch_enrg_dev;
	*noiselevel_second = vad_param->energyvad_noise_est->noise_level_second;
	*noiselevel_first = vad_param->energyvad_noise_est->noise_level_first;
	*vmmean = vad_param->vm_mean;
}

void dios_ssp_energy_vad_uninit(void* energyvad_ptr)
{
    int ret = 0;
    objENERGYVAD *vad_param;
    vad_param = (objENERGYVAD *)energyvad_ptr;
    free(vad_param->window_overlap);
    free(vad_param->ch_energy);
    free(vad_param->ch_noise);
    free(vad_param->ch_enrg_long_db);
    free(vad_param->data_buffer);
    free(vad_param->ch_snr);
    free(vad_param->ch_energy_db);
    free(vad_param->window_coef);
    free(vad_param->in_buffer);
    free(vad_param->out_buffer);
	free(vad_param->vad_fftbuf_in);
	free(vad_param->vad_fftbuf_out);
    free(vad_param->vad_fft_out);
    
    ret = dios_ssp_share_rfft_uninit(vad_param->rfft_param);
	if (0 != ret)
	{
		vad_param->rfft_param = NULL;
	}
	
    free(vad_param->energyvad_noise_est);
    free(vad_param);
}

