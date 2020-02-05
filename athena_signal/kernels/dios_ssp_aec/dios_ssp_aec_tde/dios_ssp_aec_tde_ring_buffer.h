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

#ifndef _DIOS_SSP_AEC_TDE_RING_BUFFER_H_
#define _DIOS_SSP_AEC_TDE_RING_BUFFER_H_

#include <stddef.h>  // size_t
#include <stdlib.h>
#include <string.h>

typedef struct RingBuffer RingBuffer;

// Returns NULL on failure.
RingBuffer* dios_ssp_aec_tde_creatbuffer(size_t element_count, size_t element_size);
int dios_ssp_aec_tde_initbuffer(RingBuffer* handle);
void dios_ssp_aec_tde_freebuffer(void* handle);

#endif  /* _DIOS_SSP_AEC_TDE_RING_BUFFER_H_ */
