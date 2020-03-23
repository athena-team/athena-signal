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

Description: GSC beamforming includes two functions, steering vector and
processing function.
==============================================================================*/

#include "dios_ssp_gsc_multigscbeamformer.h"

void dios_ssp_gsc_multibeamformer_init(objCMultiGSCbeamformer* multigscbeamformer, DWORD nMic, DWORD nBeam, DWORD dwSampRate, DWORD dwBlockSize, General_ArrayGeometric type, void *coord)
{
    multigscbeamformer->gscbeamformer =  (objCGSCbeamformer*)calloc(1, sizeof(objCGSCbeamformer));
    dios_ssp_gsc_gscbeamformer_init(multigscbeamformer->gscbeamformer, nMic, dwSampRate, dwBlockSize, type, coord);

    multigscbeamformer->m_pOutput = NULL;
    multigscbeamformer->m_nBeam = nBeam;
    multigscbeamformer->m_pOutput = (float**)calloc(multigscbeamformer->m_nBeam, sizeof(float*));
}

int dios_ssp_gsc_multibeamformer_reset(objCMultiGSCbeamformer* multigscbeamformer)
{
    dios_ssp_gsc_gscbeamformer_reset(multigscbeamformer->gscbeamformer);
    for(DWORD bch = 0; bch < multigscbeamformer->m_nBeam; bch++)
    {
       multigscbeamformer->m_pOutput[bch] = multigscbeamformer->gscbeamformer->m_output;
    }
    
    return 0;
}

int dios_ssp_gsc_multibeamformer_arraysteer(objCMultiGSCbeamformer* multigscbeamformer, PolarCoord *pSrcLoc)
{
    for (DWORD bch = 0; bch < multigscbeamformer->m_nBeam; bch++)
    {
        dios_ssp_gsc_gscbeamformer_arraysteer(multigscbeamformer->gscbeamformer, pSrcLoc[bch]);
    }
    return 0;
}

int dios_ssp_gsc_multibeamformer_process(objCMultiGSCbeamformer* multigscbeamformer, float** ppInput)
{
    for (DWORD bch = 0; bch < multigscbeamformer->m_nBeam; bch++)
    {
        dios_ssp_gsc_gscbeamformer_process(multigscbeamformer->gscbeamformer, ppInput);
    }
    return 0;
}

int dios_ssp_gsc_multibeamformer_delete(objCMultiGSCbeamformer* multigscbeamformer)
{
    dios_ssp_gsc_gscbeamformer_delete(multigscbeamformer->gscbeamformer);
    free(multigscbeamformer->gscbeamformer);
    free(multigscbeamformer->m_pOutput);

    return 0;
}
