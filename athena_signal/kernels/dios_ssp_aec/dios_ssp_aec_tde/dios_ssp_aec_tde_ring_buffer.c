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
which is an open source. this file aims to make a a ring buffer to hold 
arbitrary data. Provides no thread safety. Unless otherwise specified, 
functions return 0 on success and -1 on error.
==============================================================================*/

/* include file */
#include "dios_ssp_aec_tde_ring_buffer.h"

enum Wrap
{
    SAME_WRAP
};

struct RingBuffer
{
	size_t read_pos;
	size_t write_pos;
	size_t element_count;
	size_t element_size;
	enum Wrap rw_wrap;
	char* data;
};

RingBuffer* dios_ssp_aec_tde_creatbuffer(size_t element_count, size_t element_size)
{
	RingBuffer* self = NULL;
	if (element_count == 0 || element_size == 0)
	{
		return NULL;
	}

	self = (RingBuffer*)calloc(1, sizeof(RingBuffer));
	if (!self)
	{
		return NULL;
	}

	self->data = (char*)calloc(element_count * element_size, sizeof(char));
	if (!self->data)
	{
		free(self);
		self = NULL;
		return NULL;
	}

	self->element_count = element_count;
	self->element_size = element_size;

	return self;
}

int dios_ssp_aec_tde_initbuffer(RingBuffer* self)
{
	if (!self)
	{
		return -1;
	}

	self->read_pos = 0;
	self->write_pos = 0;
	self->rw_wrap = SAME_WRAP;

	// Initialize buffer to zeros
	memset(self->data, 0, self->element_count * self->element_size);

	return 0;
}

void dios_ssp_aec_tde_freebuffer(void* handle)
{
	RingBuffer* self = (RingBuffer*)handle;
	if (!self)
	{
		return;
	}

	free(self->data);
	free(self);
}
