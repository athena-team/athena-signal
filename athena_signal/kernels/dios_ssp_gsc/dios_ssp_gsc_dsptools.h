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

#ifndef _DIOS_SSP_GSC_DSPTOOLS_H_
#define _DIOS_SSP_GSC_DSPTOOLS_H_

#include <math.h>
#include <string.h>
#include "../dios_ssp_share/dios_ssp_share_complex_defs.h"

/**********************************************************************************
Function:      // delayline
Description:   // delay line in the time domain
Input:         // x: input signal
                  dly: delay the input signal x by dly samples
                  line_size: the length of the vector xdline
Output:        // xdline: the delayed output signal is stored in the vector xdline
                          from xdline[0] to xdline[line_size - dly - 1]
Return:        // none
Others:        // none
**********************************************************************************/
void delayline(float *x, float *xdline, int dly, int line_size);

#endif /* _DIOS_SSP_GSC_DSPTOOLS_H_ */

