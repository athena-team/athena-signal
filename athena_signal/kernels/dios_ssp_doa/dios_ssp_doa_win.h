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
==============================================================================*/

#ifndef _DIOS_SSP_DOA_WIN_H_
#define _DIOS_SSP_DOA_WIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "dios_ssp_doa_macros.h"

typedef struct
{
	int m_fft_size;
	int m_shift_size;
	int m_block_num;
	
	float *m_ana_win;
	float *m_norm_win;
}objDOACwin;

/**********************************************************************************
Function:      // dios_ssp_doa_win_init
Description:   // doa win init
Input:         // doawin
				  fft_size
				  shift_size
Output:        // none
Return:        // success: return doa win object pointer (void*)doawin
				  failure: return NULL
**********************************************************************************/
void dios_ssp_doa_win_init(objDOACwin *doawin,int fft_size, int shift_size);

/**********************************************************************************
Function:      // dios_ssp_doa_win_add_ana_win
Description:   // doa win ana
Input:         // doawin: doa win object pointer
				  x: enter data
Output:        // x_win
Return:        // success: return 0
**********************************************************************************/
int dios_ssp_doa_win_add_ana_win(objDOACwin *doawin, const float *x, float *x_win);

/**********************************************************************************
Function:      // dios_ssp_doa_win_delete
Description:   // doa win syn
Input:         // doawin: doa win object pointer
Output:        // none
Return:        // success: return 0
**********************************************************************************/
int dios_ssp_doa_win_delete(objDOACwin *doawin);

#endif /* _DIOS_SSP_DOA_WIN_H_ */

