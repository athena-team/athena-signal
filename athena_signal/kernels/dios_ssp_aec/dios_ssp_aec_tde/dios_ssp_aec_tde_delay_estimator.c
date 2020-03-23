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

Description: some codes of this file refers to Webrtc (https://webrtc.org/) 
which is an open source. This file performs delay estimation on binary converted
spectra. The return value is 0 - OK and -1 - Error, unless otherwise stated.
==============================================================================*/

/* include file */
#include "dios_ssp_aec_tde_delay_estimator.h"

// Number of right shifts for scaling is linearly depending on number of bits in
// the far-end binary spectrum.
static const int kShiftsAtZero = 13;  // Right shifts at zero binary spectrum.
static const int kShiftsLinearSlope = 3;

static const int kProbabilityOffset = 1024; 
static const int kProbabilityLowerLimit = 8704; 
static const int kProbabilityMinSpread = 2816; 

// Robust validation settings
static const float kHistogramMax = 3000.f;
static const float kLastHistogramMax = 250.f;
static const float kMinHistogramThreshold = 1.5f;
static const int kMinRequiredHits = 10;
static const int kMaxHitsWhenPossiblyNonCausal = 10;
static const int kMaxHitsWhenPossiblyCausal = 1000;
static const float kQ14Scaling = 1.f / (1 << 14); 
static const float kFractionSlope = 0.05f;
static const float kMinFractionWhenPossiblyCausal = 0.5f;
static const float kMinFractionWhenPossiblyNonCausal = 0.25f;

// Counts and returns number of bits of a 32-bit word.
static int BitCount(unsigned int u32)
{
    u32 = (u32 & 0x55555555) + ((u32 >> 1) & 0x55555555);
    u32 = (u32 & 0x33333333) + ((u32 >> 2) & 0x33333333);
    u32 = (u32 & 0x0f0f0f0f) + ((u32 >> 4) & 0x0f0f0f0f);
    u32 = (u32 & 0x00ff00ff) + ((u32 >> 8) & 0x00ff00ff);
    u32 = (u32 & 0x0000ffff) + ((u32 >> 16) & 0x0000ffff);

    return ((int) u32);
}

void dios_ssp_aec_tde_meanestimate(int new_value, int factor, int* mean_value)
{
    int diff = new_value - *mean_value;

    // mean_new = mean_value + ((new_value - mean_value) >> factor);
    if (diff < 0)
    {
        diff = -((-diff) >> factor);
    } 
    else
    {
        diff = (diff >> factor);
    }
    *mean_value += diff;
}

/* Collects necessary statistics */
static void UpdateRobustValidationStatistics(BinaryDelayEstimator* self,
                                             int candidate_delay,
                                             int valley_depth_q14,
                                             int valley_level_q14)
{
    float valley_depth = valley_depth_q14 * kQ14Scaling;
    float decrease_in_last_set = valley_depth;
    int max_hits_for_slow_change = (candidate_delay < self->last_delay) ? kMaxHitsWhenPossiblyNonCausal : kMaxHitsWhenPossiblyCausal;
    int i = 0;

    // Reset |candidate_hits| if we have a new candidate.
    if (candidate_delay != self->last_candidate_delay)
    {
        self->candidate_hits = 0;
        self->last_candidate_delay = candidate_delay;
    }
    self->candidate_hits++;

    self->histogram[candidate_delay] += valley_depth;
    if (self->histogram[candidate_delay] > kHistogramMax)
    {
        self->histogram[candidate_delay] = kHistogramMax;
    }
    
    if (self->candidate_hits < max_hits_for_slow_change)
    {
        decrease_in_last_set = (self->mean_bit_counts[self->compare_delay] - valley_level_q14) * kQ14Scaling;
    }
    
    for (i = 0; i < self->farend->history_size; ++i)
	{
        int is_in_last_set = (i >= self->last_delay - 2) && (i <= self->last_delay + 1) && (i != candidate_delay);
        int is_in_candidate_set = (i >= candidate_delay - 2) && (i <= candidate_delay + 1);
        
		self->histogram[i] -= decrease_in_last_set * is_in_last_set + valley_depth * (!is_in_last_set && !is_in_candidate_set);
        
        if (self->histogram[i] < 0)
	    {
            self->histogram[i] = 0;
        }
    }
}

// Validates the |candidate_delay|, estimated in dios_ssp_aec_tde_processbinaryspectrum(),
// based on a mix of counting concurring hits with a modified histogram
// of recent delay estimates.  In brief a candidate is valid (returns 1) if it
// is the most likely according to the histogram.
static int HistogramBasedValidation(BinaryDelayEstimator* self,
                                    int candidate_delay)
{
    float fraction = 1.0f;
    float histogram_threshold = self->histogram[self->compare_delay];
    int delay_difference = candidate_delay - self->last_delay;
    int is_histogram_valid = 0;

    if (delay_difference > 0)
    {
        fraction = 1.0f - kFractionSlope * delay_difference;
        fraction = (fraction > kMinFractionWhenPossiblyCausal ? fraction : kMinFractionWhenPossiblyCausal);
    } 
    else if (delay_difference < 0)
    {
        fraction = kMinFractionWhenPossiblyNonCausal - kFractionSlope * delay_difference;
        fraction = (fraction > 1.0f ? 1.0f : fraction);
    }
    histogram_threshold *= fraction;
    histogram_threshold = (histogram_threshold > kMinHistogramThreshold ? histogram_threshold : kMinHistogramThreshold);

    is_histogram_valid = (self->histogram[candidate_delay] >= histogram_threshold) && (self->candidate_hits > kMinRequiredHits);

    return is_histogram_valid;
}

int dios_ssp_aec_tde_processbinaryspectrum(BinaryDelayEstimator* self, unsigned int binary_near_spectrum)
{
    int i = 0;
    int candidate_delay = -1;
    int valid_candidate = 0;
    int value_best_candidate = kMaxBitCountsQ9;
    int value_worst_candidate = 0;
    int valley_depth = 0;
    int shifts = 0;
    int bit_count = 0;
    int threshold = 0;
    int is_histogram_valid = 0;
    int is_robust = 0;

    if(self == NULL)
    {
        return -1;
    }
      
    for (i = 0; i < self->farend->history_size; i++)
	{
        // Compare with delayed spectra and store the |bit_counts| for each delay.
        self->bit_counts[i] = (int) BitCount(binary_near_spectrum ^ self->farend->binary_far_history[i]);
        bit_count = 512 * self->bit_counts[i];

        // Update |mean_bit_counts| only when far-end signal has something to
        // contribute. If |far_bit_counts| is zero the far-end signal is weak and
        // we likely have a poor echo condition, hence don't update.
        if (self->farend->far_bit_counts[i] > 0)
	    {
            shifts = kShiftsAtZero;
            shifts -= (int) (kShiftsLinearSlope * self->farend->far_bit_counts[i]) / 16;
            dios_ssp_aec_tde_meanestimate(bit_count, shifts, &(self->mean_bit_counts[i]));
        }

        if (self->mean_bit_counts[i] < value_best_candidate)
	    {
            value_best_candidate = self->mean_bit_counts[i];
            candidate_delay = i;
        }
        else if (self->mean_bit_counts[i] > value_worst_candidate)
	    {
            value_worst_candidate = self->mean_bit_counts[i];
        }
    }

    valley_depth = value_worst_candidate - value_best_candidate;

    // The |value_best_candidate| is a good indicator on the probability of
    // |candidate_delay| being an accurate delay (a small |value_best_candidate|
    // means a good binary match). 

    // Update |minimum_probability|.
    if ((self->minimum_probability > kProbabilityLowerLimit) && (valley_depth > kProbabilityMinSpread))
    {
        threshold = value_best_candidate + kProbabilityOffset;
        threshold = (threshold < kProbabilityLowerLimit ? kProbabilityLowerLimit : threshold);
        self->minimum_probability = (self->minimum_probability > threshold ? threshold : self->minimum_probability);
    }
    // Update |last_delay_probability|.
    // We use a Markov type model, i.e., a slowly increasing level over time.
    self->last_delay_probability++;
    
    valid_candidate = ((valley_depth > kProbabilityOffset) && ((value_best_candidate < self->minimum_probability) || (value_best_candidate < self->last_delay_probability)));

    if (self->robust_validation_enabled)
    {
        is_histogram_valid = 0;

        UpdateRobustValidationStatistics(self, candidate_delay, valley_depth, value_best_candidate);
        is_histogram_valid = HistogramBasedValidation(self, candidate_delay);
        
        is_robust = (self->last_delay < 0) && (valid_candidate || is_histogram_valid);
        is_robust |= valid_candidate && is_histogram_valid;
        valid_candidate = is_robust | (is_histogram_valid && (self->histogram[candidate_delay] > self->last_delay_histogram));
    }
    if (valid_candidate)
    {
        if (candidate_delay != self->last_delay)
	    {
            self->last_delay_histogram = (self->histogram[candidate_delay] > kLastHistogramMax ? kLastHistogramMax : self->histogram[candidate_delay]);
            self->histogram[self->compare_delay] = (self->histogram[candidate_delay] < self->histogram[self->compare_delay] ? self->histogram[candidate_delay] : 
                                                    self->histogram[self->compare_delay]);
        }	
	    if ((candidate_delay > self->last_delay + 2) || (self->last_delay > candidate_delay + 2 ))
	    {
		    self->last_delay = candidate_delay;
	    }
        self->last_delay_probability = (value_best_candidate < self->last_delay_probability ? value_best_candidate : self->last_delay_probability);
        self->compare_delay = self->last_delay;
    }

    return self->last_delay; 
}

void dios_ssp_aec_tde_addbinaryfarspectrum(BinaryDelayEstimatorFarend* handle, unsigned int binary_far_spectrum)
{
    if(handle == NULL)
    {
        return;
    }
    
    // Shift binary spectrum history and insert current |binary_far_spectrum|.
    memmove(&(handle->binary_far_history[1]), &(handle->binary_far_history[0]), (handle->history_size - 1) * sizeof(unsigned int));
    handle->binary_far_history[0] = binary_far_spectrum;

    memmove(&(handle->far_bit_counts[1]), &(handle->far_bit_counts[0]), (handle->history_size - 1) * sizeof(int));
    handle->far_bit_counts[0] = BitCount(binary_far_spectrum);
}

/* initialization subfunction */
void dios_ssp_aec_tde_freebinarydelayestimatorfarend(BinaryDelayEstimatorFarend* self)
{
    if (self == NULL)
    {
        return;
    }

    free(self->binary_far_history);
    self->binary_far_history = NULL;

    free(self->far_bit_counts);
    self->far_bit_counts = NULL;

    free(self);
}

BinaryDelayEstimatorFarend* dios_ssp_aec_tde_creatbinarydelayestimatorfarend(int history_size)
{
    BinaryDelayEstimatorFarend* self = NULL;

    if (history_size > 1)
    {
        // Sanity conditions fulfilled.
        self = (BinaryDelayEstimatorFarend*)calloc(1, sizeof(BinaryDelayEstimatorFarend));
    }
    if (self != NULL)
    {
        int malloc_fail = 0;

        self->history_size = history_size;

        // Allocate memory for history buffers.
        self->binary_far_history = (unsigned int*)calloc(history_size, sizeof(unsigned int));
        malloc_fail |= (self->binary_far_history == NULL);

        self->far_bit_counts = (int*)calloc(history_size, sizeof(int));
        malloc_fail |= (self->far_bit_counts == NULL);

        if (malloc_fail)
	    {
            dios_ssp_aec_tde_freebinarydelayestimatorfarend(self);
            self = NULL;
        }
    }

    return self;
}

void dios_ssp_aec_tde_initbinarydelayestimatorfarend(BinaryDelayEstimatorFarend* self)
{
    if(self == NULL)
    {
        return;
    }
    memset(self->binary_far_history, 0, sizeof(unsigned int) * self->history_size);
    memset(self->far_bit_counts, 0, sizeof(int) * self->history_size);
}

void dios_ssp_aec_tde_freebinarydelayestimator(BinaryDelayEstimator* self)
{
    if (self == NULL)
    {
        return;
    }

    free(self->mean_bit_counts);
    self->mean_bit_counts = NULL;

    free(self->bit_counts);
    self->bit_counts = NULL;

    free(self->binary_near_history);
    self->binary_near_history = NULL;

    free(self->histogram);
    self->histogram = NULL;

    // BinaryDelayEstimator does not have ownership of |farend|, hence we do not
    // free the memory here. That should be handled separately by the user.
    self->farend = NULL;

    free(self);
}
void dios_ssp_aec_tde_initbinarydelayestimator(BinaryDelayEstimator* self)
{
    int i = 0;
    if(self == NULL)
    {
        return;
    }

    memset(self->bit_counts, 0, sizeof(int) * self->farend->history_size);
    memset(self->binary_near_history, 0, sizeof(unsigned int) * self->near_history_size);
    for (i = 0; i <= self->farend->history_size; ++i)
	{
        self->mean_bit_counts[i] = (20 << 9);
        self->histogram[i] = 0.f;
    }
    self->minimum_probability = kMaxBitCountsQ9; 
    self->last_delay_probability = (int) kMaxBitCountsQ9;

    // Default return value if we're unable to estimate. -1 is used for errors.
    self->last_delay = -2;

    self->last_candidate_delay = -2;
    self->compare_delay = self->farend->history_size;
    self->candidate_hits = 0;
    self->last_delay_histogram = 0.f;
}

BinaryDelayEstimator* dios_ssp_aec_tde_creatbinarydelayestimator(BinaryDelayEstimatorFarend* farend, int max_lookahead)
{
    BinaryDelayEstimator* self = NULL;

    if ((farend != NULL) && (max_lookahead >= 0))
    {
        // Sanity conditions fulfilled.
        self = (BinaryDelayEstimator*)calloc(1, sizeof(BinaryDelayEstimator));
    }

    if (self != NULL)
    {
        int malloc_fail = 0;

        self->farend = farend;
        self->near_history_size = max_lookahead + 1;
        self->robust_validation_enabled = 1;  //0, Disabled by default.

        self->lookahead = max_lookahead;

        
        self->mean_bit_counts = (int*)calloc((farend->history_size + 1), sizeof(int));
        malloc_fail |= (self->mean_bit_counts == NULL);

        self->bit_counts = (int*)calloc(farend->history_size, sizeof(int));
        malloc_fail |= (self->bit_counts == NULL);

        // Allocate memory for history buffers.
        self->binary_near_history = (unsigned int*)calloc((max_lookahead + 1), sizeof(unsigned int));
        malloc_fail |= (self->binary_near_history == NULL);

        self->histogram = (float*)calloc((farend->history_size + 1), sizeof(float));
        malloc_fail |= (self->histogram == NULL);

        if (malloc_fail)
	    {
            dios_ssp_aec_tde_freebinarydelayestimator(self);
            self = NULL;
        }
    }

    return self;
}
