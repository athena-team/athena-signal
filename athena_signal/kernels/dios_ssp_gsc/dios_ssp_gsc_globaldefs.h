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

#ifndef _DIOS_SSP_GSC_GLOBALDEFS_H_
#define _DIOS_SSP_GSC_GLOBALDEFS_H_

#include <math.h>

#define S_OK ((long)0L)
#define S_FALSE ((long)1L)
#define FALSE false
#define TRUE true
#define ABS(TMP) (((TMP) < 0) ? -(TMP) : (TMP))
#define SIGN(TMP) (((TMP) < 0) ? -1 : 1)
#define SQUARE(TMP) ((TMP)*(TMP))

#define DECLARE_HANDLE(name) struct name##__{int unused;}; typedef struct name##__ *name

#ifndef EPSILON
#define EPSILON 1e-5
#endif

typedef unsigned short WORD;
typedef unsigned long DWORD;

typedef enum
{
	General_ArrayEquiSpacingLine,  /* Equi-spacing line */
	General_ArraySymmetricLine,    /* Symmetric line */
	General_ArrayLShape,           /* L shape */
	General_ArrayArbitrary,        /* Arbitrary shape*/
} General_ArrayGeometric;

/* struct GSCFDAF: structure with parameters for the gsc frequency-domain adaptive filters */
typedef struct GSCFDAF
{
	int fftlength;       /* Length of the FFT */
	int fftoverlap;      /* Overlap of input signal segments 
                            = max(ABMPARAMETERS.overlap, AICPARAMETERS.overlap) */
	float delta_con;     /* Threshold for static regularization */
	float delta_dyn;     /* Threshold for dynamic regularization */
	float s0_dyn;        /* 'Lobe' parameter of the dynamic regularization */
	int regularize_dyn;  /* Use dynamic regularization */
} GSCFDAF;

/* struct ADAPTCTRL: structure with parameters for the adaptation control */
typedef struct ADAPTCTRL
{
	int fmin;  /* Minimum frequency for averaging the SNR estimate */
	int fmax;  /* Maximum frequency for averaging the SNR estimate */
	int fc;    /* Transition frequency for adaptation control in discrete frequency bins and averaged */
	float ctabm;  /* Correction factor for the vector of frequency-dependent abm thresholds */
	float ctaic;  /* Correction factor for the vector of frequency-dependent aic thresholds */
	int	U;     /* number_sub_windows_for_minimum_statistics 8 */
	int V;     /* size_sub_windows_for_minimum_statistics 18 */
	float lambda_max;  /* upper_limit_forgetting_factor_minimum_statistics 0.96 */
	float lambda_min;  /* lower_limit_forgetting_factor_minimum_statistics 0.25 */
	float corr_bias;   /* correction_bias_for_minimum_statistics 2.12 */
} ADAPTCTRL;

/* struct ABMPARAMETERS: structure with parameters for the adaptive blocking matrix */
typedef struct ABMPARAMETERS
{
	float mu;    /* Stepsize for the adaptation algorithm */
	int ntaps;   /* Number of filter taps for each adaptive filter */
	int fftoverlap;  /* Overlap factor of the FFT */
	float lambda;    /* Forgetting factor for recursive power estimation */
	float tconstfreeze;  /* Time constant which prevents freezing of adaptive filters (in seconds) */
} ABMPARAMETERS;

/* struct AICPARAMETERS: structure with parameters for the adaptive interference canceller */
typedef struct AICPARAMETERS
{
	float mu;    /* Stepsize for the adaptation algorithm */
	int ntaps;   /* Number of filter taps for each adaptive filter */
	int fftoverlap;  /* Overlap factor of the FFT */
	float lambda;    /* Forgetting factor for recursive power estimation */
	float maxnorm;   /* Maximally allowed filter norm, against desired signal cancellation */
	float tconstfreeze;  /* Time constant which prevents freezing of adaptive filters  (in seconds) */
} AICPARAMETERS;

/* struct SYNCDELAYS: structure with delays for synchronization of the FGSC modules */
typedef struct SYNCDELAYS
{
	int nDelayAIC;    /* Sync delay for aic filter inputs */
	int nDelayABM;    /* Sync delay for abm filter input */
	int nDelayAcRef;  /* Sync delay for ac reference microphone */
	int nDelayAEC;    /* Sync delay for saec filter inputs */
} SYNCDELAYS;

typedef enum	
{
	General_WinRect,	 /* no smoothing (rectangular window) */
	General_WinBlackman, /* smoothing by optimal Blackman window */
	General_WinHamming,  /* smoothing by Hamming window */
	General_WinHanning,  /* smoothing by Hanning window */
} General_WindowType;

#endif  /* _DIOS_SSP_GSC_GLOBALDEFS_H_ */
