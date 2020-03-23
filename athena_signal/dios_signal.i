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

Description: Add a new interface file to describe the functions and classes
to be encapsulated. Swig will automatically encapsulate the C code into a
callable module for Python to call.
==============================================================================*/

%module dios_signal

%{
#define SWIG_FILE_WITH_INIT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <stddef.h>

#include "kernels/dios_ssp_share/dios_ssp_share_complex_defs.h"
#include "kernels/dios_ssp_share/dios_ssp_share_rfft.h"
#include "kernels/dios_ssp_share/dios_ssp_share_subband.h"
#include "kernels/dios_ssp_share/dios_ssp_share_cinv.h"
#include "kernels/dios_ssp_share/dios_ssp_share_noiselevel.h"
#include "kernels/dios_ssp_share/dios_ssp_share_typedefs.h"
#include "kernels/dios_ssp_aec/dios_ssp_aec_res.h"
#include "kernels/dios_ssp_aec/dios_ssp_aec_doubletalk.h"
#include "kernels/dios_ssp_aec/dios_ssp_aec_api.h"
#include "kernels/dios_ssp_aec/dios_ssp_aec_macros.h"
#include "kernels/dios_ssp_aec/dios_ssp_aec_firfilter.h"
#include "kernels/dios_ssp_aec/dios_ssp_aec_common.h"
#include "kernels/dios_ssp_aec/dios_ssp_aec_erl_est.h"
#include "kernels/dios_ssp_mvdr/dios_ssp_mvdr_header.h"
#include "kernels/dios_ssp_mvdr/dios_ssp_mvdr_win.h"
#include "kernels/dios_ssp_mvdr/dios_ssp_mvdr_macros.h"
#include "kernels/dios_ssp_mvdr/dios_ssp_mvdr_api.h"
#include "kernels/dios_ssp_ns/dios_ssp_ns_macros.h"
#include "kernels/dios_ssp_ns/dios_ssp_ns_api.h"
#include "kernels/dios_ssp_doa/dios_ssp_doa_api.h"
#include "kernels/dios_ssp_doa/dios_ssp_doa_macros.h"
#include "kernels/dios_ssp_doa/dios_ssp_doa_win.h"
#include "kernels/dios_ssp_hpf/dios_ssp_hpf_api.h"
#include "kernels/dios_ssp_vad/dios_ssp_vad_macros.h"
#include "kernels/dios_ssp_vad/dios_ssp_vad_energy.h"
#include "kernels/dios_ssp_vad/dios_ssp_vad_counter.h"
#include "kernels/dios_ssp_vad/dios_ssp_vad_api.h"
#include "kernels/dios_ssp_agc/dios_ssp_agc_api.h"
#include "kernels/dios_ssp_aec/dios_ssp_aec_tde/dios_ssp_aec_tde.h"
#include "kernels/dios_ssp_aec/dios_ssp_aec_tde/dios_ssp_aec_tde_ring_buffer.h"
#include "kernels/dios_ssp_aec/dios_ssp_aec_tde/dios_ssp_aec_tde_delay_estimator_wrapper.h"
#include "kernels/dios_ssp_aec/dios_ssp_aec_tde/dios_ssp_aec_tde_delay_estimator.h"
#include "kernels/dios_ssp_return_defs.h"
#include "kernels/dios_ssp_api.h"
#include "dios_signal.h"
#include "kernels/dios_ssp_gsc/dios_ssp_gsc_abm.h"
#include "kernels/dios_ssp_gsc/dios_ssp_gsc_adaptctrl.h"
#include "kernels/dios_ssp_gsc/dios_ssp_gsc_aic.h"
#include "kernels/dios_ssp_gsc/dios_ssp_gsc_api.h"
#include "kernels/dios_ssp_gsc/dios_ssp_gsc_beamformer.h"
#include "kernels/dios_ssp_gsc/dios_ssp_gsc_beamsteering.h"
#include "kernels/dios_ssp_gsc/dios_ssp_gsc_dsptools.h"
#include "kernels/dios_ssp_gsc/dios_ssp_gsc_filtsumbeamformer.h"
#include "kernels/dios_ssp_gsc/dios_ssp_gsc_firfilterdesign.h"
#include "kernels/dios_ssp_gsc/dios_ssp_gsc_globaldefs.h"
#include "kernels/dios_ssp_gsc/dios_ssp_gsc_micarray.h"
#include "kernels/dios_ssp_gsc/dios_ssp_gsc_multigscbeamformer.h"
#include "kernels/dios_ssp_gsc/dios_ssp_gsc_rmNPsdOsMs.h"
%}

%typemap(in, numinputs=1) (int argc, char **argv) %{
  /* Check if is a list */
  if (PyList_Check($input)) {
    int i;
    $1 = PyList_Size($input);
    $2 = (char **) malloc(($1+1)*sizeof(char *));
    for (i = 0; i < $1; i++) {
      PyObject *o = PyList_GetItem($input,i);
      if (PyString_Check(o))
	$2[i] = PyString_AsString(PyList_GetItem($input,i));
      else {
	PyErr_SetString(PyExc_TypeError,"list must contain strings");
	free($2);
	return NULL;
      }
    }
    $2[i] = 0;
  } else {
    PyErr_SetString(PyExc_TypeError,"not a list");
    return NULL;
  }
%}

%typemap(freearg) (int argc, char **argv) %{
  free((char *) $2);
%}

%typemap(in, numinputs=1) (int *fe_switch, size_t m) %{
  assert(PyList_Check($input));
  $2 = PyList_Size($input);
  $1 = (int *) malloc($2*sizeof(int));
  size_t j;
  for(j = 0; j < $2; ++j) {
    $1[j] = PyInt_AsLong(PyList_GetItem($input, j));
  }
%}

%typemap(freearg) (int *fe_switch, size_t m) %{
  free($1);
%}

%typemap(in, numinputs=1) (float *mic_coord, size_t n) %{
  assert(PyList_Check($input));
  $2 = PyList_Size($input);
  $1 = (float *) malloc(($2)*sizeof(float));
  size_t k;
  for(k = 0; k < $2; ++k) {
    $1[k] = PyFloat_AsDouble(PyList_GetItem($input, k));
  }
%}

%typemap(freearg) (float *mic_coord, size_t n) %{
  free($1);
%}

%typemap(in, numinputs=1) (int mic_num) %{
  $1 = PyInt_AsLong($input);
%}

%typemap(in, numinputs=1) (int ref_num) %{
  $1 = PyInt_AsLong($input);
%}


int dios_ssp_v1(int argc, char **argv, int *fe_switch, size_t m, float *mic_coord, size_t n, int mic_num, int ref_num);