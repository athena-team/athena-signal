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

Description: Analysis window and synthesis window of DOA
==============================================================================*/

#include "dios_ssp_doa_win.h" 

void dios_ssp_doa_win_init(objDOACwin *doawin,int fft_size, int shift_size)
{
    int i, j;
	int tmp;
	int log_fft_size = 0;
	float temp;
	tmp = 1;

	doawin->m_fft_size = fft_size;
	doawin->m_shift_size = shift_size;

	if ( doawin->m_shift_size >= doawin->m_fft_size )
	{
		printf( "shift_size[%d] >= fft_size[%d].\n", doawin->m_shift_size, doawin->m_fft_size );
	}

	while ( tmp < doawin->m_fft_size )
	{
		log_fft_size += 1;
		tmp *= 2;
	}
	if ( tmp > doawin->m_fft_size )
	{
		printf( "FFT Size[%d] should be power of 2.\n", doawin->m_fft_size );
	}
	tmp = 1;
	while ( tmp < doawin->m_shift_size )
	{
		tmp *= 2;
	}
	if ( tmp > doawin->m_shift_size )
	{
		printf( "Shift Size[%d] should be power of 2.\n", doawin->m_shift_size );
	}

	doawin->m_block_num = doawin->m_fft_size / doawin->m_shift_size;
	doawin->m_ana_win = (float*)calloc(doawin->m_fft_size, sizeof(float));
	doawin->m_norm_win = (float*)calloc(doawin->m_fft_size, sizeof(float));
	
    for (i = 0; i < doawin->m_fft_size; ++i )
    {
        doawin->m_ana_win[i] = (float)(0.54 - 0.46*cos((2*i)*PI/(doawin->m_fft_size-1)));
    }
    for (i = 0; i < doawin->m_fft_size; ++i )
    {
        doawin->m_norm_win[i] = doawin->m_ana_win[i] * doawin->m_ana_win[i];
    }
    for (i = 0; i < doawin->m_shift_size; ++i )
    {
        temp = 0;
        for (j = 0; j < doawin->m_block_num; ++j )
        {
            temp += doawin->m_norm_win[i+j*doawin->m_shift_size];
        }
        doawin->m_norm_win[i] = 1.0f / temp;
    }
    for (i = 0; i < doawin->m_shift_size; ++i )
    {
        for (j = 1; j < doawin->m_block_num; ++j )
        {
            doawin->m_norm_win[i+j*doawin->m_shift_size] = doawin->m_norm_win[i];
        }
    }
}

int dios_ssp_doa_win_add_ana_win(objDOACwin *doawin, const float *x, float *x_win)
{
    int i;
	for (i = 0; i < doawin->m_fft_size; ++i )
	{ 
		x_win[i] = x[i] * doawin->m_ana_win[i];
	}

	return 0;
}

int dios_ssp_doa_win_delete(objDOACwin *doawin)
{
	free(doawin->m_ana_win);
	free(doawin->m_norm_win);

	return 0;
}

