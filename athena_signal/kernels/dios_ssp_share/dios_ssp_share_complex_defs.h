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

#ifndef _DIOS_SSP_SHARE_COMPLEX_DEFS_H_
#define _DIOS_SSP_SHARE_COMPLEX_DEFS_H_

#include <math.h>

// define complex declare type
typedef struct FCOMPLEX { float r, i; } fcomplex;

// define fcomplex, float type
typedef fcomplex xcomplex;

// Macros specific for the fixed point implementation
#define DIOS_SSP_WORD16_MAX       32767
#define DIOS_SSP_WORD16_MIN       -32768

float xsqrt(float x);
float xmax(float x, float y);
float xmin(float x, float y);
float xabs(float x);
float xsmooth_proc(float y, float rate, float x);

float xsmooth_factor(float st);

// complex generation
xcomplex complex_gen(float re, float im);

// complex conjugation
xcomplex complex_conjg(xcomplex z);

// complex abs
float complex_abs(xcomplex z);

// complex number absolute value square
float complex_abs2(xcomplex cp);

// complex sqrt
xcomplex complex_sqrt(xcomplex z);

// complex addition
xcomplex complex_add(xcomplex a, xcomplex b);

// complex subtraction
xcomplex complex_sub(xcomplex a, xcomplex b);

// complex multiplication
xcomplex complex_mul(xcomplex a, xcomplex b);

// real and complex mutiplication
xcomplex complex_real_complex_mul(float x, xcomplex a);

// complex division
xcomplex complex_div(xcomplex a, xcomplex b);

// complex division 2
xcomplex complex_div2(xcomplex a, xcomplex b);

// complex number div real number
xcomplex complex_div_real(xcomplex cp, float r);

// complex number averaging
xcomplex complex_avg_vec(xcomplex *cpVec, int cpVecLen);



/* function implementations */
/*---------------------------------------------------
complex FIR filtering of 2nd dimension data
len    : Tap length        : in    : int
hat    : impulse response    : in    : xcomplex[][]
buf    : input buffer        : in    : xcomplex[][]
xout: output data        : out    : xcomplex[]
---------------------------------------------------*/
xcomplex complex_conv(int len, xcomplex *hat, xcomplex *buf);

/*---------------------------------------------------
CmplxDataPush
:  push complex data into 2nd dimension bufer
in    len    bufer length
xin    input data
renewal    buf    bufer
---------------------------------------------------*/
void complex_data_push(int len, xcomplex xin, xcomplex *buf);

// delay function application
int NormW16(short a);

// Shifting with negative numbers not allowed
// We cannot do casting here due to signed/unsigned problem
#define DIOS_SSP_RSHIFT_W32(x, c)     ((x) >> (c))

#endif /* _DIOS_SSP_SHARE__FUNC_DEFS_H_ */

