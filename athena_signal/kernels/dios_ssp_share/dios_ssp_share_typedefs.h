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

#ifndef _DIOS_SSP_SHARE_TYPEDEFS_H_
#define _DIOS_SSP_SHARE_TYPEDEFS_H_

#define CharacterNumMax 16

struct objTimeSynchInfo
{
	int timer_tick;
	int character_num;
	int character_begin_time[CharacterNumMax];
	int character_end_time[CharacterNumMax];
};

typedef	struct
{
	float	x;		//	left to right
	float	y;		//	near to far
	float	z;		//	low to high
} PlaneCoord;

typedef struct {
	float  rho;        ///< distance (metre)
	float  phi;        ///< horizontal angle (Radian), value range from 0 to 2*PI (PI is 3.1415926535)
	float  theta;      ///< vertical angle (Radian), value range from 0 to PI
} PolarCoord;

typedef struct
{
	int nIndexR;	//	the index of the right microphone (when speak faces to array)
	int nIndexL;	//	the index of the left microphone (should be larger than nIndexR)
} PairList;

#endif /* _DIOS_SSP_SHARE_TYPEDEFS_H_ */

