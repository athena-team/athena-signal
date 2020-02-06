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

#ifndef _DIOS_SSP_MVDR_MACROS_H_
#define _DIOS_SSP_MVDR_MACROS_H_

#define VELOCITY                        (340.0f)
#define PI                              (3.141592653589793f)
#define	DEFAULT_MVDR_SAMPLING_FRQ		16000
#define	DEFAULT_MVDR_WIN_SIZE			512
#define	DEFAULT_MVDR_SHIFT_SIZE			128
#define	DEFAULT_MVDR_DELTA_ANGLE		5
#define DEFAULT_MVDR_SD_FACTOR			(0.01f)
#define DEFAULT_MVDR_SD_EPS				(0.000001f)
#define	DEFAULT_MVDR_RNN_EPS			100
#define	DEFAULT_MVDR_ALPHA_RNN			(0.99f)
#define DEFAULT_MVDR_ALPHA_S            (0.8f)
#define DEFAULT_MVDR_ALPHA_P            (0.2f)
#define DEFAULT_MVDR_ALPHA_D            (0.95f)
#define DEFAULT_MVDR_L                  50
#define DEFAULT_MVDR_DELTA_THRES        1.5

#endif /* _DIOS_SSP_MVDR_MACROS_H_ */

