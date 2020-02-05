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

// Performs delay estimation on block by block basis.
// The return value is  0 - OK and -1 - Error, unless otherwise stated.

#ifndef _DIOS_SSP_AEC_TDE_DELAY_ESTIMATOR_WRAPPER_H_
#define _DIOS_SSP_AEC_TDE_DELAY_ESTIMATOR_WRAPPER_H_

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h> 
#include "dios_ssp_aec_tde_ring_buffer.h"
#include "dios_ssp_aec_tde_ring_buffer.h"
#include "../../dios_ssp_share/dios_ssp_share_complex_defs.h"
#include "../../dios_ssp_share/dios_ssp_share_rfft.h"
#include "../dios_ssp_aec_common.h"
#include "dios_ssp_aec_tde_delay_estimator.h"

#define ALIGN8_BEG
    
// aecm_defines.h //
#define AECM_DYNAMIC_Q                 /* Turn on/off dynamic Q-domain. */

/* Algorithm parameters */
#define FRAME_LEN       80             /* Total frame length, 10 ms. */

#define PART_LEN        64             /* Length of partition. */
#define PART_LEN_SHIFT  7              /* Length of (PART_LEN * 2) in base 2. */

#define PART_LEN1       (PART_LEN + 1)  /* Unique fft coefficients. */
#define PART_LEN2       (PART_LEN << 1) /* Length of partition * 2. */
#define PART_LEN4       (PART_LEN << 2) /* Length of partition * 4. */
#define FAR_BUF_LEN     PART_LEN4       /* Length of buffers. */
#define DELAY_WIN_SLIDE_TDE   500 // sliding win 
#define DELAY_WIN_SLIDE       100 // sliding win 
#define MAX_DELAY_LONG       750// 750 // 750 frames, 750 * 64 
#define MAX_DELAY_SHORT   100 // 100 frames, 100 * 64 

/* Counter parameters */
#define CONV_LEN        2048          /* Convergence length used at startup. */ //change
#define CONV_LEN2       (CONV_LEN << 1) /* Used at startup. */

/* Energy parameters */
#define MAX_BUF_LEN     64           /* History length of energy signals. */
#define FAR_ENERGY_MIN  1025         /* Lowest Far energy level: At least 2 */
                                     /* in energy. */
/* Suppression gain parameters: SUPGAIN parameters in Q-(RESOLUTION_SUPGAIN). */
#define RESOLUTION_SUPGAIN      8     /* Channel in Q-(RESOLUTION_SUPGAIN). */     
#define SUPGAIN_DEFAULT (1 << RESOLUTION_SUPGAIN)  /* Default. */
#define SUPGAIN_ERROR_PARAM_A   3072  /* Estimation error parameter */
                                      /* (Maximum gain) (8 in Q8). */
#define SUPGAIN_ERROR_PARAM_B   1536  /* Estimation error parameter */
                                      /* (Gain before going down). */
#define SUPGAIN_ERROR_PARAM_D   SUPGAIN_DEFAULT /* Estimation error parameter */
                                /* (Should be the same as Default) (1 in Q8). */
#define SUPGAIN_EPC_DT  200     /* SUPGAIN_ERROR_PARAM_C * ENERGY_DEV_TOL */

#define ONE_Q14         (1 << 14)

typedef struct {
    short real;
    short imag;
} complex16_t;

typedef struct {
    RingBuffer* farFrameBuf;
    RingBuffer* nearNoisyFrameBuf;
    RingBuffer* nearCleanFrameBuf;
    RingBuffer* outFrameBuf;

    // Delay estimation variables
    void* delay_estimator_farend;
    void* delay_estimator;
    unsigned short currentDelay;

    float far_history[PART_LEN1 * MAX_DELAY_LONG];
    int far_q_domains[MAX_DELAY_LONG];
    int far_history_pos;
    int max_delay_history_size; 
    
    short fixedDelay;

    unsigned int totCount;

    // The extra 16 or 32 bytes in the following buffers are for alignment based
    // Neon code.
    // It's designed this way since the current GCC compiler can't align a
    // buffer in 16 or 32 byte boundaries properly.
    short channelStored_buf[PART_LEN1 + 8];
    short channelAdapt16_buf[PART_LEN1 + 8];
    int channelAdapt32_buf[PART_LEN1 + 8];
    float xBuf_buf[PART_LEN2 + 16];  // farend
    short dBufClean_buf[PART_LEN2 + 16];  // nearend
    float dBufNoisy_buf[PART_LEN2 + 16];  // nearend
    short outBuf_buf[PART_LEN + 8];

    // Pointers to the above buffers
    short *channelStored;
    short *channelAdapt16;
    int *channelAdapt32;
    float *xBuf;
    short *dBufClean;
    float *dBufNoisy;
    short *outBuf;

    int noiseEst[PART_LEN1];

    short farEnergyVAD;
    short farEnergyMSE;
    int currentVADValue;
    short vadUpdateCount;

    int            *delayHistVect; // 
    int            *delayN; 
    int            delay_nframe;
    int            delay_nsample;
    int            max_delay_size;  // short-term delay
	int            max_long_delay_size; // long-term delay
	int            win_slide;

    void *rfft_param;
    float fft_out[PART_LEN2];
    float tde_ana_win[PART_LEN2];

} AecmCore_t;

/**********************************************************************************
Function:      // dios_ssp_aec_tde_freedelayestimatorfarend
Description:   // Releases the memory allocated by 
                  dios_ssp_aec_tde_creatdelayestimatorfarend(...)
Input:         // handle: Pointer to the delay estimation far-end instance.
Output:        // none
Return:        // none
**********************************************************************************/
void dios_ssp_aec_tde_freedelayestimatorfarend(void* handle);

/**********************************************************************************
Function:      // dios_ssp_aec_tde_creatdelayestimatorfarend
Description:   // Allocates the memory needed by the far-end part of the delay 
                  estimation. The memory needs to be initialized separately through
                  dios_ssp_aec_tde_initdelayestimatorfarend(...).
Input:         // spectrum_size: Size of the spectrum used both in far-end and
                                 near-end. Used to allocate memory for spectrum
                                 specific buffers.
                  history_size: The far-end history buffer size. Note that the
                                maximum delay which can be estimated is controlled 
                                together with |lookahead| through
                                dios_ssp_aec_tde_creatdelayestimator().
Output:        // none
Return:        // void*: Created |handle|. If the memory can't be allocated or
                         if any of the input parameters are invalid NULL is returned.
**********************************************************************************/
void* dios_ssp_aec_tde_creatdelayestimatorfarend(int spectrum_size, int history_size);

/**********************************************************************************
Function:      // dios_ssp_aec_tde_initdelayestimatorfarend
Description:   // Initializes the far-end part of the delay estimation instance 
                  returned by dios_ssp_aec_tde_creatdelayestimatorfarend(...)
Input:         // handle: Pointer to the delay estimation far-end instance.
Output:        // handle: Initialized instance.
Return:        // none
**********************************************************************************/
int dios_ssp_aec_tde_initdelayestimatorfarend(void* handle);

/**********************************************************************************
Function:      // dios_ssp_aec_tde_addfarspectrum
Description:   // Adds the far-end spectrum to the far-end history buffer.
Input:         // handle: Pointer to the delay estimation far-end instance.
                  far_spectrum: Far-end spectrum.
                  spectrum_size: The size of the data arrays (same for both far- and
                                 near-end).
                  far_q: The Q-domain of the far-end data.
Output:        // handle: Updated far-end instance.
Return:        // none
**********************************************************************************/
int dios_ssp_aec_tde_addfarspectrum(void* handle, float* far_spectrum, int spectrum_size, int far_q);

/**********************************************************************************
Function:      // dios_ssp_aec_tde_freedelayestimator
Description:   // Releases the memory allocated by 
                  dios_ssp_aec_tde_creatdelayestimator(...)
Input:         // handle: Pointer to the delay estimation instance.
Output:        // none
Return:        // none
**********************************************************************************/
void dios_ssp_aec_tde_freedelayestimator(void* handle);

/**********************************************************************************
Function:      // dios_ssp_aec_tde_creatdelayestimator
Description:   // allocates the memory needed by the delay estimation. The memory 
                  needs to be initialized separately through 
                  dios_ssp_aec_tde_initdelayestimator(...).
Input:         // farend_handle : Pointer to the far-end part of the delay estimation
                                  instance created prior to this call using
                                  dios_ssp_aec_tde_creatdelayestimatorfarend().

                                   Note that dios_ssp_aec_tde_creatdelayestimator 
                                   does not take ownership of |farend_handle|,
                                   which has to be torn down properly after this
                                   instance.

                   max_lookahead : Maximum amount of non-causal lookahead allowed. 

                                   Using lookahead can detect cases in which a 
                                   near-end signal occurs before the corresponding
                                   far-end signal. It will delay the estimate for 
                                   the current block by an equal amount, and the 
                                   returned values will be offset by it.

                                   A value of zero is the typical no-lookahead case.
                                   This also represents the minimum delay which can 
                                   be estimated.

                                   Note that the effective range of delay estimates 
                                   is [-|lookahead|,... ,|history_size|-|lookahead|)
                                   where |history_size| was set upon creating the 
                                   far-end history buffer size.
Output:        // none
Return:        // void*: Created |handle|. If the memory can't be allocated or if 
                         any of the input parameters are invalid NULL is returned.
**********************************************************************************/
void* dios_ssp_aec_tde_creatdelayestimator(void* farend_handle, int max_lookahead);

/**********************************************************************************
Function:      // dios_ssp_aec_tde_initdelayestimator
Description:   // Initializes the delay estimation instance returned by 
                  dios_ssp_aec_tde_creatdelayestimator(...)
Input:         // handle: Pointer to the delay estimation instance.
Output:        // handle: Initialized instance.
Return:        // none
**********************************************************************************/
int dios_ssp_aec_tde_initdelayestimator(void* handle);

/**********************************************************************************
Function:      // dios_ssp_aec_tde_robust_validation
Description:   // Enables/Disables a robust validation functionality in the delay 
                  estimation. This is by default set to disabled at create time.  
                  The state is preserved over a reset.
Input:         // handle: Pointer to the delay estimation instance.
                  enable: Enable (1) or disable (0) this feature.
Output:        // none
Return:        // none
**********************************************************************************/
int dios_ssp_aec_tde_robust_validation(void* handle, int enable);

/**********************************************************************************
Function:      // dios_ssp_aec_tde_delayestimateprocess
Description:   // estimates and returns the delay between the far-end and near-end 
                  blocks. The value will be offset by the lookahead (i.e. the 
                  lookahead should be subtracted from the returned value).
Input:         // handle: Pointer to the delay estimation instance.
                  near_spectrum: Pointer to the near-end spectrum data of the 
                                 current block.
                  spectrum_size: The size of the data arrays (same for both 
                                 far-end and near-end).
                  near_q: The Q-domain of the near-end data.
Output:        // handle: Updated instance.
Return:        // delay:  >= 0 - Calculated delay value.
                          -1 - Error.
                          -2 - Insufficient data for estimation.
**********************************************************************************/
int dios_ssp_aec_tde_delayestimateprocess(void* handle, float* near_spectrum, int spectrum_size, int near_q);

/**********************************************************************************
Function:      // dios_ssp_aec_tde_creatcore
Description:   // allocates the memory needed by the AECM. The memory needs to be
                  initialized separately using the dios_ssp_aec_tde_initcore() 
                  function.
Input:         // srv: Instance that should be created
Output:        // srv: Created instance
Return:        // 0 - Ok
                 -1 - Error
**********************************************************************************/
int dios_ssp_aec_tde_creatcore(AecmCore_t **srv, int max_delay_size, int win_slide);

/**********************************************************************************
Function:      // dios_ssp_aec_tde_initcore
Description:   // this function initializes the AECM instant created with
                  dios_ssp_aec_tde_creatcore(...)
Input:         // srv: Pointer to the AECM instance
Output:        // srv: Initialized instance
Return:        // 0 - Ok
                 -1 - Error
**********************************************************************************/
int dios_ssp_aec_tde_initcore(AecmCore_t * const srv);

/**********************************************************************************
Function:      // dios_ssp_aec_tde_freecore
Description:   // this function releases the memory allocated by 
                  dios_ssp_aec_tde_creatcore()
Input:         // srv: Pointer to the AECM instance
Output:        // none
Return:        // 0 - Ok
                 -1 - Error
                 11001-11016: Error
**********************************************************************************/
int dios_ssp_aec_tde_freecore(AecmCore_t *srv);

/**********************************************************************************
Function:      // dios_ssp_aec_tde_ProcessBlock
Description:   // This function is called for every block within one frame
Input:         // srv: Pointer to the AECM instance
                  farend: In buffer containing one block of echo signal
                  nearendNoisy: In buffer containing one frame of nearend+echo 
                                signal without NS
                  nearendClean: In buffer containing one frame of nearend+echo signal
                                with NS
Output:        // out: Out buffer, one block of nearend signal
Return:        // none
**********************************************************************************/
int dios_ssp_aec_tde_ProcessBlock(AecmCore_t * srv, float * farend, float * nearendNoisy);

int get_tde_final(AecmCore_t * srv);

#endif  /* _DIOS_SSP_AEC_TDE_DELAY_ESTIMATOR_WRAPPER_H_ */

