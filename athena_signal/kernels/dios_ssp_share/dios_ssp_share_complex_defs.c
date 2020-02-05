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

Description: Complex operation.
==============================================================================*/

#include "dios_ssp_share_complex_defs.h"

float xsqrt(float x)
{
	return((float)sqrt(x));
}
float xmax(float x, float y)
{
	return((((x) > y)) ? (x) : (y));
}
float xmin(float x, float y)
{
	return(((x) < (y)) ? (x) : (y));
}
float xabs(float x)
{
	return((float)fabs(x));
}
float xsmooth_proc(float y, float rate, float x)
{
	y += ((rate) * ((x)-y));
	return(y);
}

float xsmooth_factor(float st)
{
	return((1.0f - (float)exp(-1.0f / (st))));
}

// complex generation
xcomplex complex_gen(float re, float im)
{
	xcomplex c;

	c.r = re;
	c.i = im;

	return c;
}

// complex conjugation
xcomplex complex_conjg(xcomplex z)
{
	xcomplex c;

	c.r = z.r;
	c.i = -z.i;

	return c;
}

// complex abs
float complex_abs(xcomplex z)
{
	float x, y, ans, temp;

	x = (float)fabs(z.r);
	y = (float)fabs(z.i);

	if (x == 0.0)
		ans = y;
	else if (y == 0.0)
		ans = x;
	else if (x > y)
	{
		temp = y / x;
		ans = x * (float)sqrt(1.0 + temp * temp);
}
	else
	{
		temp = x / y;
		ans = y * (float)sqrt(1.0 + temp * temp);
	}

	return ans;
}

// complex number absolute value square
float complex_abs2(xcomplex cp)
{
	float y;

	y = cp.r * cp.r + cp.i * cp.i;

	return (y);
}

// complex sqrt
xcomplex complex_sqrt(xcomplex z)
{
	xcomplex c;
	float x, y, w, r;

	if ((z.r == 0.0) && (z.i == 0.0))
	{
		c.r = 0.0;
		c.i = 0.0;

		return c;
	}
	else
	{
		x = (float)fabs(z.r);
		y = (float)fabs(z.i);

		if (x >= y)
		{
			r = y / x;
			w = (float)(sqrt(x) * sqrt(0.5 * (1.0 + sqrt(1.0 + r * r))));
		}
		else
		{
			r = x / y;
			w = (float)(sqrt(y) * sqrt(0.5 * (r + sqrt(1.0 + r * r))));
		}

		if (z.r >= 0.0)
		{
			c.r = w;
			c.i = z.i / (2.0f * w);
		}
		else
		{
			c.i = (z.i >= 0) ? w : -w;
			c.r = z.i / (2.0f * c.i);
		}

		return c;
	}
}

// complex addition
xcomplex complex_add(xcomplex a, xcomplex b)
{
	xcomplex c;

	c.r = a.r + b.r;
	c.i = a.i + b.i;

	return c;
}

// complex subtraction
xcomplex complex_sub(xcomplex a, xcomplex b)
{
	xcomplex c;

	c.r = a.r - b.r;
	c.i = a.i - b.i;

	return c;
}

// complex multiplication
xcomplex complex_mul(xcomplex a, xcomplex b)
{
	xcomplex c;

	c.r = a.r * b.r - a.i * b.i;
	c.i = a.i * b.r + a.r * b.i;

	return c;
}

// real and complex mutiplication
xcomplex complex_real_complex_mul(float x, xcomplex a)
{
	xcomplex c;

	c.r = x * a.r;
	c.i = x * a.i;

	return c;
}

// complex division
xcomplex complex_div(xcomplex a, xcomplex b)
{
	xcomplex c;
	float r, den;

	if (fabs(b.r) >= fabs(b.i))
	{
		r = b.i / b.r;
		den = b.r + r * b.i;
		c.r = (a.r + r * a.i) / den;
		c.i = (a.i - r * a.r) / den;
	}
	else
	{
		r = b.r / b.i;
		den = b.i + r * b.r;
		c.r = (a.r * r + a.i) / den;
		c.i = (a.i * r - a.r) / den;
	}

	return c;
}

// complex division 2
xcomplex complex_div2(xcomplex a, xcomplex b)
{
	xcomplex c;
	float den;

	den = b.r * b.r + b.i * b.i;

	c.r = (a.r * b.r + a.i * b.i) / den;
	c.i = (a.i * b.r - a.r * b.i) / den;

	return c;
}

// complex number div real number
xcomplex complex_div_real(xcomplex cp, float r)
{
	xcomplex tmpcp;

	tmpcp.r = cp.r / r;
	tmpcp.i = cp.i / r;

	return (tmpcp);
}

// complex number averaging
xcomplex complex_avg_vec(xcomplex *cpVec, int cpVecLen)
{
	int j;
	xcomplex tmpcp;

	tmpcp.r = 0.0;
	tmpcp.i = 0.0;

	for (j = 0; j < cpVecLen; j++)
		tmpcp = complex_add(tmpcp, cpVec[j]);

	tmpcp = complex_div_real(tmpcp, (float)cpVecLen);

	return (tmpcp);
}



/* function implementations */
/*---------------------------------------------------
complex FIR filtering of 2nd dimension data
len    : Tap length        : in    : int
hat    : impulse response    : in    : xcomplex[][]
buf    : input buffer        : in    : xcomplex[][]
xout: output data        : out    : xcomplex[]
---------------------------------------------------*/
xcomplex complex_conv(int len, xcomplex *hat, xcomplex *buf)
{
	int    i;
	xcomplex    z, xout;

	xout.r = xout.i = 0.0;
	for (i = 0; i<len; i++)
	{
		z = complex_mul(complex_conjg(hat[i]), buf[i]);
		xout = complex_add(xout, z);
	}
	return(xout);
}

/*---------------------------------------------------
CmplxDataPush
:  push complex data into 2nd dimension bufer
in    len    bufer length
xin    input data
renewal    buf    bufer
---------------------------------------------------*/
void complex_data_push(int len, xcomplex xin, xcomplex *buf)
{
	int i;

	for (i = len - 1; i >= 1; i--) buf[i] = buf[i - 1];
	buf[0] = xin;
}

// delay function application
int NormW16(short a)
{
	int zeros;

	if (a == 0)
	{
		return 0;
	}
	else if (a < 0)
	{
		a = ~a;
	}

	if (!(0xFF80 & a))
	{
		zeros = 8;
	}
	else
	{
		zeros = 0;
	}
	if (!(0xF800 & (a << zeros))) zeros += 4;
	if (!(0xE000 & (a << zeros))) zeros += 2;
	if (!(0xC000 & (a << zeros))) zeros += 1;

	return zeros;
}
