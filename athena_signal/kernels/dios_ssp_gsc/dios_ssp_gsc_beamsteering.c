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

Description: GSC beamformer steering vector.
==============================================================================*/

#include "dios_ssp_gsc_beamsteering.h"

void FIRFiltering_Direct(float *pIn, float *pOut, int dwBaseSize, float *FIRTaps, 
        int dwFIRTaps, float *pDlyLine, int *DlyLineIndex)
{
	int i; 
	int numIter = dwBaseSize / dwFIRTaps;
	int numLeft = dwBaseSize % dwFIRTaps;
	int oldDlyLineIndex;  /* save the beginning position */

	/* process the dwFIRTaps length data block */
	for (i = 0; i < numIter; i++)
	{
		/* move the dwFIRTaps new samples into the pDlyLine buffer, in order to decrease the copy operations 
         * because the block copy is quicker than the single sample copy (using the circular buffer) */
		if ((2 * dwFIRTaps - (*DlyLineIndex)) >= dwFIRTaps)
		{
			memcpy(pDlyLine + (*DlyLineIndex), pIn + i * dwFIRTaps, dwFIRTaps * sizeof(float));
			oldDlyLineIndex = *DlyLineIndex;  /* save beginning position */
			*DlyLineIndex += dwFIRTaps;  /* new saving position */
			if (*DlyLineIndex == 2 * dwFIRTaps)
            {
				*DlyLineIndex = 0;
            }
		}
		else
		{
			memcpy(pDlyLine + (*DlyLineIndex), pIn + i * dwFIRTaps, 
                    (2 * dwFIRTaps - (*DlyLineIndex)) * sizeof(float));
			memcpy(pDlyLine, pIn + i * dwFIRTaps + (2 * dwFIRTaps - (*DlyLineIndex)), 
                    ((*DlyLineIndex) - dwFIRTaps) * sizeof(float));
			oldDlyLineIndex = *DlyLineIndex;  /* save beginning position */
			*DlyLineIndex -= dwFIRTaps;  /* new saving position */
		}

		/* FIR filtering by convolution */
		for (int k = 0; k < dwFIRTaps; k++)
		{
			float ftemp = 0.0;
			for (int j = 0; j < dwFIRTaps; j++)
            {
				ftemp += pDlyLine[(oldDlyLineIndex + k - j + 2 * dwFIRTaps) % (2 * dwFIRTaps)] * FIRTaps[j];
            }
			pOut[i * dwFIRTaps + k] = ftemp;
		}
	}

	/* process the numLeft length data block */
	if (numLeft > 0)
	{
		/* move the numLeft samples into the DlyLine buffer, in order to decrease the copy operations */
		if ((2 * dwFIRTaps - (*DlyLineIndex)) >= numLeft)
		{
			memcpy(pDlyLine + (*DlyLineIndex), pIn + i * dwFIRTaps, numLeft * sizeof(float));
			oldDlyLineIndex = *DlyLineIndex;  /* save beginning position */
			*DlyLineIndex += numLeft;  /* new saving position */
			if (*DlyLineIndex == 2 * dwFIRTaps)
            {
				*DlyLineIndex = 0;
            }
		}
		else
		{
			memcpy(pDlyLine + (*DlyLineIndex), pIn + i * dwFIRTaps, 
                    (2 * dwFIRTaps - (*DlyLineIndex)) * sizeof(float));
			memcpy(pDlyLine, pIn + i * dwFIRTaps + (2 * dwFIRTaps - (*DlyLineIndex)), 
                    (numLeft + (*DlyLineIndex) - 2 * dwFIRTaps) * sizeof(float));
			oldDlyLineIndex = *DlyLineIndex;  /* save beginning position */
			*DlyLineIndex += numLeft - 2 * dwFIRTaps;  /* new saving position */	
		}

		/* FIR filtering by convolution */
		for (int k = 0; k < numLeft; k++)
		{
			float ftemp = 0.0;
			for (int j = 0; j < dwFIRTaps; j++)
            {
				ftemp += pDlyLine[(oldDlyLineIndex + k - j + 2 * dwFIRTaps) % (2 * dwFIRTaps)] * FIRTaps[j];
            }
			pOut[i * dwFIRTaps + k] = ftemp;
		}
	}
}

void dios_ssp_gsc_gscbeamsteer_init(objCGSCbeamsteer* gscbeamsteer, int nMic, int nBlockSize, DWORD dwKernelRate, int nTaps)
{
	gscbeamsteer->m_nMic = nMic;
	gscbeamsteer->m_nBlockSize = nBlockSize; 
	gscbeamsteer->m_dwKernelRate = dwKernelRate;
	gscbeamsteer->m_nTaps = nTaps;

	gscbeamsteer->m_pDlyLine = (float**)calloc(gscbeamsteer->m_nMic, sizeof(float*));
	for (int i_mic = 0; i_mic < gscbeamsteer->m_nMic; i_mic++)
	{
		gscbeamsteer->m_pDlyLine[i_mic] = (float*)calloc(2 * gscbeamsteer->m_nTaps, sizeof(float));
	}
	gscbeamsteer->m_pTaps = (float**)calloc(gscbeamsteer->m_nMic, sizeof(float*));
	for (int i_mic = 0; i_mic < gscbeamsteer->m_nMic; i_mic++)
	{
		gscbeamsteer->m_pTaps[i_mic] = (float*)calloc(gscbeamsteer->m_nTaps, sizeof(float));
	}
	gscbeamsteer->generalfirdesign =  (objCGeneralFIRDesigner*)calloc(1, sizeof(objCGeneralFIRDesigner));
	dios_ssp_gscfirfilterdesign_init(gscbeamsteer->generalfirdesign, gscbeamsteer->m_nTaps, General_WinBlackman);
	
	for (int ch = 0; ch < gscbeamsteer->m_nMic; ch++)
    {
		dios_ssp_gscfirfilterdesign_fractionaldelay(gscbeamsteer->generalfirdesign, -1.0f, 1.0f, (float)(gscbeamsteer->m_nTaps / 2), gscbeamsteer->m_pTaps[ch]);
    }
	gscbeamsteer->m_nDlyLineIndex = (int*)calloc(gscbeamsteer->m_nMic, sizeof(int));
}

int dios_ssp_gsc_gscbeamsteer_reset(objCGSCbeamsteer* gscbeamsteer)
{
	for (int m = 0; m < gscbeamsteer->m_nMic; m++)
	{
		memset(gscbeamsteer->m_pDlyLine[m], 0, sizeof(float) * 2 * gscbeamsteer->m_nTaps);
	}
	for (int m = 0; m < gscbeamsteer->m_nMic; m++)
	{
		memset(gscbeamsteer->m_pTaps[m], 0, sizeof(float) * gscbeamsteer->m_nTaps);
	}

	for (int ch = 0; ch < gscbeamsteer->m_nMic; ch++)
    {
		dios_ssp_gscfirfilterdesign_fractionaldelay(gscbeamsteer->generalfirdesign, -1.0f, 1.0f, (float)(gscbeamsteer->m_nTaps / 2), gscbeamsteer->m_pTaps[ch]);
    }
	memset(gscbeamsteer->m_nDlyLineIndex, 0, sizeof(int) * gscbeamsteer->m_nMic);

	return 0;
}

int dios_ssp_gsc_gscbeamsteering(objCGSCbeamsteer* gscbeamsteer, float *delay_sample, DWORD dwInputRate)
{
	for (int ch = 0; ch < gscbeamsteer->m_nMic; ch++) 
	{
		float delay = delay_sample[ch] * (float)gscbeamsteer->m_dwKernelRate / (float)dwInputRate;
		dios_ssp_gscfirfilterdesign_fractionaldelay(gscbeamsteer->generalfirdesign, -1.0f, 1.0f, (float)(gscbeamsteer->m_nTaps / 2) + delay, gscbeamsteer->m_pTaps[ch]);
	}

	return 0;
}

int dios_ssp_gsc_gscbeamsteer_process(objCGSCbeamsteer* gscbeamsteer, float **X, float **Y)
{
	for (int ch = 0; ch < gscbeamsteer->m_nMic; ch++)
	{
        FIRFiltering_Direct(X[ch], Y[ch], gscbeamsteer->m_nBlockSize, gscbeamsteer->m_pTaps[ch], gscbeamsteer->m_nTaps, gscbeamsteer->m_pDlyLine[ch], &(gscbeamsteer->m_nDlyLineIndex[ch]));
	}

	return 0;
}
int dios_ssp_gsc_gscbeamsteer_delete(objCGSCbeamsteer* gscbeamsteer)
{
	dios_ssp_gscfirfilterdesign_delete(gscbeamsteer->generalfirdesign);
	for (int i_mic = 0; i_mic < gscbeamsteer->m_nMic; i_mic++)
	{
		free(gscbeamsteer->m_pDlyLine[i_mic]);
	}
	free(gscbeamsteer->m_pDlyLine);
	for (int i_mic = 0; i_mic < gscbeamsteer->m_nMic; i_mic++)
	{
		free(gscbeamsteer->m_pTaps[i_mic]);
	}
	free(gscbeamsteer->m_pTaps);
	free(gscbeamsteer->m_nDlyLineIndex);
	return 0;
}
