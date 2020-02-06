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

Description: Analysis window and synthesis window of MVDR
==============================================================================*/

#include "dios_ssp_mvdr_win.h" 

void dios_ssp_mvdr_win_init(objMVDRCwin *mvdrwin,int fft_size, int shift_size)
{
    int i, j;
	int tmp;
	int log_fft_size = 0;
	float temp;
	tmp = 1;

	mvdrwin->m_fft_size = fft_size;
	mvdrwin->m_shift_size = shift_size;

	if ( mvdrwin->m_shift_size >= mvdrwin->m_fft_size )
	{
		printf( "shift_size[%d] >= fft_size[%d].\n", mvdrwin->m_shift_size, mvdrwin->m_fft_size );
	}

	while ( tmp < mvdrwin->m_fft_size )
	{
		log_fft_size += 1;
		tmp *= 2;
	}
	if ( tmp > mvdrwin->m_fft_size )
	{
		printf( "FFT Size[%d] should be power of 2.\n", mvdrwin->m_fft_size );
	}
	tmp = 1;
	while ( tmp < mvdrwin->m_shift_size )
	{
		tmp *= 2;
	}
	if ( tmp > mvdrwin->m_shift_size )
	{
		printf( "Shift Size[%d] should be power of 2.\n", mvdrwin->m_shift_size );
	}

	mvdrwin->m_block_num = mvdrwin->m_fft_size / mvdrwin->m_shift_size;
	mvdrwin->m_ana_win = (float*)calloc(mvdrwin->m_fft_size, sizeof(float));
	mvdrwin->m_syn_win = (float*)calloc(mvdrwin->m_fft_size, sizeof(float));
	mvdrwin->m_norm_win = (float*)calloc(mvdrwin->m_fft_size, sizeof(float));
	
    for (i = 0; i < mvdrwin->m_fft_size; ++i )
    {
        mvdrwin->m_ana_win[i] = (float)(0.54 - 0.46*cos((2*i)*PI/(mvdrwin->m_fft_size-1)));
    }
    for (i = 0; i < mvdrwin->m_fft_size; ++i )
    {
        mvdrwin->m_norm_win[i] = mvdrwin->m_ana_win[i] * mvdrwin->m_ana_win[i];
    }
    for (i = 0; i < mvdrwin->m_shift_size; ++i )
    {
        temp = 0;
        for (j = 0; j < mvdrwin->m_block_num; ++j )
        {
            temp += mvdrwin->m_norm_win[i+j*mvdrwin->m_shift_size];
        }
        mvdrwin->m_norm_win[i] = 1.0f / temp;
    }
    for (i = 0; i < mvdrwin->m_shift_size; ++i )
    {
        for (j = 1; j < mvdrwin->m_block_num; ++j )
        {
            mvdrwin->m_norm_win[i+j*mvdrwin->m_shift_size] = mvdrwin->m_norm_win[i];
        }
    }
    for (i = 0; i < mvdrwin->m_fft_size; ++i )
    {
        mvdrwin->m_syn_win[i] = mvdrwin->m_norm_win[i] * mvdrwin->m_ana_win[i]; 
    }
}

int dios_ssp_mvdr_win_add_ana_win(objMVDRCwin *mvdrwin, const float *x, float *x_win)
{
    int i;
	for (i = 0; i < mvdrwin->m_fft_size; ++i )
	{ 
		x_win[i] = x[i] * mvdrwin->m_ana_win[i];
	}

	return 0;
}

int dios_ssp_mvdr_win_add_syn_win(objMVDRCwin *mvdrwin, const float *x, float *x_win)
{
    int i;
	for (i = 0; i < mvdrwin->m_fft_size; ++i )
	{ 
		x_win[i] = x[i] * mvdrwin->m_syn_win[i];
	}

	return 0;
}

int dios_ssp_mvdr_win_delete(objMVDRCwin *mvdrwin)
{
	free(mvdrwin->m_ana_win);
	free(mvdrwin->m_syn_win);
	free(mvdrwin->m_norm_win);

	return 0;
}

