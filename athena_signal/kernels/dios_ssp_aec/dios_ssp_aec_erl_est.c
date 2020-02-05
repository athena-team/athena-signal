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

Description: the file contains echo return loss estimation related functions, 
the output value of which is the ratio of the estimated echo energy to the 
reference signal energy.
==============================================================================*/

/* include file */
#include "dios_ssp_aec_erl_est.h"

int dios_ssp_aec_erl_est_process(objFirFilter* srv)
{
	int ch;
	int i_spk;
	int i;
	float erl_dyn_bound_high;
	float erl_dyn_bound_low;

	if (NULL == srv)
	{
		return ERR_AEC;
	}

	for (ch = AEC_LOW_CHAN; ch < AEC_HIGH_CHAN; ch++)
	{
		/* get psd */		
		srv->mic_rec_psd[ch] = complex_abs2(srv->sig_mic_rec[ch]);/* mic record signal */

		for (i_spk = 0; i_spk < srv->ref_num; i_spk++)
		{
			srv->ref_psd[i_spk][ch] = complex_abs2(srv->sig_spk_ref[i_spk][ch]); /* reference signal */
			srv->power_echo_rtn_fir[i_spk][ch] = complex_abs2(srv->est_ref_fir[i_spk][ch]);
			srv->power_echo_rtn_adpt[i_spk][ch] = complex_abs2(srv->est_ref_adf[i_spk][ch]);
		}
	}
	for (ch = AEC_LOW_CHAN; ch < AEC_HIGH_CHAN; ch++)
	{
		if (srv->energy_err_fir[ch] < srv->energy_err_adf[ch])
		{
			srv->power_mic_send_smooth[ch] = srv->energy_err_fir[ch];
			for (i_spk = 0; i_spk < srv->ref_num; i_spk++) 
			{
				srv->power_echo_rtn_smooth[i_spk][ch] = srv->power_echo_rtn_fir[i_spk][ch];
			}
		}
		else
		{
			srv->power_mic_send_smooth[ch] = srv->energy_err_adf[ch];
			for (i_spk = 0; i_spk < srv->ref_num; i_spk++)
			{
				srv->power_echo_rtn_smooth[i_spk][ch] = srv->power_echo_rtn_adpt[i_spk][ch];
			}
		}
	}

	for (i = 0; i < ERL_BAND_NUM; i++)
	{
		srv->mic_rec_part_band_energy[i] = 0.0f;
		srv->mic_send_part_band_energy[i] = 0.0f;
		for (ch = srv->band_table[i][0]; ch <= srv->band_table[i][1]; ch++)
		{
			srv->mic_rec_part_band_energy[i] += srv->mic_rec_psd[ch];
			srv->mic_send_part_band_energy[i] += srv->power_mic_send_smooth[ch];
		}
	}
	
	for (i_spk = 0; i_spk < srv->ref_num; i_spk++)
	{
		for (i = 0; i < ERL_BAND_NUM; i++)
		{
			srv->echo_return_band_energy[i] = 0.0f;
			for (ch = srv->band_table[i][0]; ch <= srv->band_table[i][1]; ch++)
			{
				srv->echo_return_band_energy[i] += srv->power_echo_rtn_smooth[i_spk][ch];
			}
			/* Mic peak value tracking */
			float temp = srv->echo_return_band_energy[i];
			if (temp > srv->mic_peak[i_spk][i])
			{
				srv->mic_peak[i_spk][i] = temp;
			}
			else
			{
				srv->mic_peak[i_spk][i] = AEC_PEAK_ALPHA * srv->mic_peak[i_spk][i] + (1 - AEC_PEAK_ALPHA) * temp;
			}

			if ((srv->spk_part_band_energy[i] > 10.0f * srv->noise_est_spk_part[i_spk][i]->noise_level_first)
				&& (srv->mic_rec_part_band_energy[i] > 4.0f * srv->mic_send_part_band_energy[i]))
			{
				float erl_inst = srv->mic_peak[i_spk][i] / (srv->spk_peak[i_spk][i] + 1e-006f);
				if (erl_inst > ERL_BOUND_H)
					erl_inst = ERL_BOUND_H;
				else if (erl_inst < ERL_BOUND_L)
					erl_inst = ERL_BOUND_L;

				srv->erl_ratio[i_spk][i] = AEC_ERL_ALPHA * srv->erl_ratio[i_spk][i] + (1 - AEC_ERL_ALPHA) * erl_inst;
			}
		}
		
		erl_dyn_bound_high = 8.0f * srv->erl_ratio[i_spk][1];
		erl_dyn_bound_low = srv->erl_ratio[i_spk][1] / 8.0f;
		for (i = 0; i < ERL_BAND_NUM; i++)
		{
			if (i == 1)
				continue;

			if (srv->erl_ratio[i_spk][i] > erl_dyn_bound_high)
				srv->erl_ratio[i_spk][i] = erl_dyn_bound_high;
			else if (srv->erl_ratio[i_spk][i] < erl_dyn_bound_low)
				srv->erl_ratio[i_spk][i] = erl_dyn_bound_low;
		}
	}
	return 0;
}

