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

#ifndef _DIOS_SSP_SHARE_CINV_H_
#define _DIOS_SSP_SHARE_CINV_H_

#include <stdio.h>
#include <stdlib.h>

typedef struct
{
	int dim;		
	float **ar;
	float **ai;
	float **mat_temp;
	float **mat_temp2;
}objMATRIXinv;

void *dios_ssp_matrix_inv_init(int Rdim);
int dios_ssp_matrix_inv_process(void *matrix_inv, float *R, float *Rinv);
int dios_ssp_matrix_inv_delete(void *matrix_inv);

#endif /* _DIOS_SSP_SHARE_CINV_H_ */

