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

#ifndef _DIOS_SSP_AEC_TDE_DELAY_ESTIMATOR_H_
#define _DIOS_SSP_AEC_TDE_DELAY_ESTIMATOR_H_

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const int kMaxBitCountsQ9 = (32 << 9);

typedef struct
{
    // Pointer to bit counts.
    int* far_bit_counts;
    // Binary history variables.
    unsigned int* binary_far_history;
    int history_size;
} BinaryDelayEstimatorFarend;

typedef struct
{
    // Pointer to bit counts.
    int* mean_bit_counts;
    // Array only used locally in ProcessBinarySpectrum() but whose size is
    // determined at run-time.
    int* bit_counts;

    // Binary history variables.
    unsigned int* binary_near_history;
    int near_history_size;

    // Delay estimation variables.
    int minimum_probability;
    int last_delay_probability;

    // Delay memory.
    int last_delay;

    // Robust validation
    int robust_validation_enabled;
    int last_candidate_delay;
    int compare_delay;
    int candidate_hits;
    float* histogram;
    double last_delay_histogram;

    // For dynamically changing the lookahead when using SoftReset...().
    int lookahead;

    // Far-end binary spectrum history buffer etc.
    BinaryDelayEstimatorFarend* farend;
} BinaryDelayEstimator;

// --- delay_estimator_internal.h ---//
typedef union
{
    float float_;
    int int32_;
} SpectrumType;

typedef struct
{
    // Pointers to mean values of spectrum.
    SpectrumType* mean_far_spectrum;
    int far_spectrum_initialized;

    int spectrum_size;

    // Far-end part of binary spectrum based delay estimation.
    BinaryDelayEstimatorFarend* binary_farend;
} DelayEstimatorFarend;

typedef struct
{
    // Pointers to mean values of spectrum.
    SpectrumType* mean_near_spectrum;
    int near_spectrum_initialized;

    int spectrum_size;

    // Binary spectrum based delay estimator
    BinaryDelayEstimator* binary_handle;
} DelayEstimator;
// ---- end of delay_estimator_internal.h ---//

/**********************************************************************************
Function:      // dios_ssp_aec_tde_freebinarydelayestimatorfarend
Description:   // releases the memory allocated
Input:         // self: Pointer to the binary delay estimation far-end 
                        instance which is the return value of 
                        dios_ssp_aec_tde_creatbinarydelayestimatorfarend().
Output:        // none
Return:        // none
**********************************************************************************/
void dios_ssp_aec_tde_freebinarydelayestimatorfarend(BinaryDelayEstimatorFarend* self);

/**********************************************************************************
Function:      // dios_ssp_aec_tde_creatbinarydelayestimatorfarend
Description:   // allocates the memory needed by the far-end part of the binary 
                  delay estimation. The memory needs to be initialized separately.
Input:         // history_size: size of the far-end binary spectrum history
Output:        // none
Return:        // BinaryDelayEstimatorFarend*: Created |handle|. If the memory 
                  can't be allocated or if any of the input parameters are invalid
                  NULL is returned.
**********************************************************************************/
BinaryDelayEstimatorFarend* dios_ssp_aec_tde_creatbinarydelayestimatorfarend(int history_size);

/**********************************************************************************
Function:      // dios_ssp_aec_tde_initbinarydelayestimatorfarend
Description:   // initializes the delay estimation far-end instance.
Input:         // self: pointer to the delay estimation far-end instance.
Output:        // self: initialized far-end instance.
Return:        // none
**********************************************************************************/
void dios_ssp_aec_tde_initbinarydelayestimatorfarend(BinaryDelayEstimatorFarend* self);

/**********************************************************************************
Function:      // dios_ssp_aec_tde_addbinaryfarspectrum
Description:   // adds the binary far-end spectrum to the internal far-end history 
                  buffer. This spectrum is used as reference when calculating the 
                  delay
Input:         // self: pointer to the delay estimation far-end instance.
                  binary_far_spectrum: Far-end binary spectrum.
Output:        // self: Updated far-end instance.
Return:        // none
**********************************************************************************/
void dios_ssp_aec_tde_addbinaryfarspectrum(BinaryDelayEstimatorFarend* self,
                                 unsigned int binary_far_spectrum);

/**********************************************************************************
Function:      // dios_ssp_aec_tde_freebinarydelayestimator
Description:   // Releases the memory allocated
                  Note that BinaryDelayEstimator utilizes BinaryDelayEstimatorFarend,
                  but does not take ownership of it, hence the BinaryDelayEstimator 
                  has to be torn down before the far-end.
Input:         // self: pointer to the delay estimation far-end instance which is 
                  the return value of dios_ssp_aec_tde_creatbinarydelayestimator().
Output:        // none
Return:        // none
**********************************************************************************/
void dios_ssp_aec_tde_freebinarydelayestimator(BinaryDelayEstimator* self);


BinaryDelayEstimator* dios_ssp_aec_tde_creatbinarydelayestimator(
    BinaryDelayEstimatorFarend* farend, int max_lookahead);

/**********************************************************************************
Function:      // dios_ssp_aec_tde_initbinarydelayestimator
Description:   // initializes the delay estimation instance                  
Input:         // self: pointer to the delay estimation instance.
Output:        // self: initialized instance.
Return:        // none
**********************************************************************************/
void dios_ssp_aec_tde_initbinarydelayestimator(BinaryDelayEstimator* self);

/**********************************************************************************
Function:      // dios_ssp_aec_tde_processbinaryspectrum
Description:   // estimates and returns the delay between the binary far-end and 
                  binary near-end spectra. It is assumed the binary far-end spectrum
                  has been added using dios_ssp_aec_tde_addbinaryfarspectrum() prior
                  to this call. The value will be offset by the lookahead (i.e. the
                  lookahead should be subtracted from the returned value).
Input:         // self: pointer to the delay estimation instance.
               // binary_near_spectrum: near-end binary spectrum of the current block.
Output:        // self: Updated instance.
Return:        // delay: >= 0 - Calculated delay value.
                         -2  - Insufficient data for estimation.       
**********************************************************************************/
int dios_ssp_aec_tde_processbinaryspectrum(BinaryDelayEstimator* self,
                                 unsigned int binary_near_spectrum);

/**********************************************************************************
Function:      // dios_ssp_aec_tde_meanestimate
Description:   // updates the |mean_value| recursively with a step size of 
                  2^-|factor|. This function is used internally in the Binary Delay
                  Estimator as well as the Fixed point wrapper.
Input:         // new_value: the new value the mean should be updated with.
                  factor: the step size, in number of right shifts.
Output:        // mean_value: pointer to the mean value.
Return:        // none
**********************************************************************************/
void dios_ssp_aec_tde_meanestimate(int new_value,
                             int factor,
                             int* mean_value);


#endif  /* _DIOS_SSP_AEC_TDE_DELAY_ESTIMATOR_H_ */
