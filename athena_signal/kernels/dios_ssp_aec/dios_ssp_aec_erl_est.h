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

#ifndef _DIOS_SSP_AEC_ERL_EST_H_
#define _DIOS_SSP_AEC_ERL_EST_H_

#include <stdio.h>
#include "dios_ssp_aec_firfilter.h"

/**********************************************************************************
Function:      // dios_ssp_aec_erl_est_process
Description:   // run dios speech signal process aec erl estimation module by frames
Input:         // srv: dios speech signal process aec firfilter pointer
Output:        // none
Return:        // success: return 0, failure: return ERR_AEC
**********************************************************************************/
int dios_ssp_aec_erl_est_process(objFirFilter* srv);

#endif /* _DIOS_SSP_AEC_ERL_EST_H_ */

