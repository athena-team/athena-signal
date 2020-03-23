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

Description: The steering vector is first calculated and applied to Adaptive
Blocking Matrix(ABM) and Adaptive Interference Canceller(AIC) modules. The GSC
beamformer model contains Fixed Beamformering(FBF).
==============================================================================*/

#include "dios_ssp_gsc_beamformer.h"

static const float gsc_c = 340.0f;          /* sound speed */

void dios_ssp_gsc_gscbeamformer_init(objCGSCbeamformer* gscbeamformer, DWORD nMic, DWORD dwSampRate, DWORD dwBlockSize, General_ArrayGeometric type, void *coord)
{
	int param = 0;
	gscbeamformer->m_nMic = (int)nMic;
	gscbeamformer->m_nIOBlockSize = (int)dwBlockSize;
	gscbeamformer->m_dwSampRate = dwSampRate;
	gscbeamformer->m_time = 0;

	/* read parameter file */ 
	gscbeamformer->m_paramGSC.regularize_dyn = 1;
	gscbeamformer->m_paramGSC.delta_con = 0.0001f;
	gscbeamformer->m_paramGSC.delta_dyn = 0.00001f;
	gscbeamformer->m_paramGSC.s0_dyn = 0.00001f;

	gscbeamformer->m_paramABM.mu = 0.5f;
	gscbeamformer->m_paramABM.ntaps = 64;
	gscbeamformer->m_paramABM.fftoverlap = 2;
	gscbeamformer->m_paramABM.lambda = 0.99f;
	gscbeamformer->m_paramABM.tconstfreeze = 100.0f;

	gscbeamformer->m_paramAIC.mu = 0.3f;
	gscbeamformer->m_paramAIC.ntaps = 64;
	gscbeamformer->m_paramAIC.fftoverlap = 4;
	gscbeamformer->m_paramAIC.lambda = 0.985f;
	gscbeamformer->m_paramAIC.maxnorm = 0.001f;
	gscbeamformer->m_paramAIC.tconstfreeze = 100.0f;

	gscbeamformer->m_paramAC.fmin = 300;
	gscbeamformer->m_paramAC.fmax = 600;
	gscbeamformer->m_paramAC.fc = 300;
	gscbeamformer->m_paramAC.ctabm = 0.8f;
	gscbeamformer->m_paramAC.ctaic = 4.0f;
	gscbeamformer->m_paramAC.U = 8;
	gscbeamformer->m_paramAC.V = 18;

	gscbeamformer->m_paramGSC.fftoverlap = 
	gscbeamformer->m_paramABM.fftoverlap > gscbeamformer->m_paramAIC.fftoverlap?gscbeamformer->m_paramABM.fftoverlap : gscbeamformer->m_paramAIC.fftoverlap;
	gscbeamformer->m_paramGSC.fftlength = 2 * (gscbeamformer->m_paramABM.ntaps < gscbeamformer->m_paramAIC.ntaps?gscbeamformer->m_paramABM.ntaps : gscbeamformer->m_paramAIC.ntaps);

	gscbeamformer->gscbeamsteer =  (objCGSCbeamsteer*)calloc(1, sizeof(objCGSCbeamsteer));
	dios_ssp_gsc_gscbeamsteer_init(gscbeamformer->gscbeamsteer, gscbeamformer->m_nMic, gscbeamformer->m_nIOBlockSize, gscbeamformer->m_dwSampRate, 32);

	/* initialize the fixed beamformer unit */
	int ordFbfFilt = 0;
	gscbeamformer->gscfiltsumbeamformer =  (objFGSCfiltsumbeamformer*)calloc(1, sizeof(objFGSCfiltsumbeamformer));
	dios_ssp_gsc_gscfiltsumbeamformer_init(gscbeamformer->gscfiltsumbeamformer, gscbeamformer->m_nMic, gscbeamformer->m_paramGSC.fftlength, gscbeamformer->m_paramGSC.fftoverlap);

	/* initialize delays for intermodule synchronization 
     * which can only be done after fbf initialization since the order of fbf filters are required */
	/* sync delay for aic filter inputs */
	int dly_overlap = (gscbeamformer->m_paramGSC.fftlength / gscbeamformer->m_paramABM.fftoverlap - gscbeamformer->m_paramGSC.fftlength / gscbeamformer->m_paramAIC.fftoverlap) / 8;
	gscbeamformer->m_paramSync.nDelayAIC = gscbeamformer->m_paramGSC.fftlength / 4 + gscbeamformer->m_paramGSC.fftlength / 4 + (SIGN(dly_overlap) + 1) * dly_overlap;

	/* sync delay for abm filter input */
	gscbeamformer->m_paramSync.nDelayABM = (int)floor((float)ordFbfFilt / 2)  /* delay of fbf */
						    + gscbeamformer->m_paramGSC.fftlength / 4;        /* delay for causality of adaptive filters */
	
	/* sync delay for ac reference microphone */
	gscbeamformer->m_paramSync.nDelayAcRef = (int)floor((float)ordFbfFilt / 2);  /* delay of fbf */
	
	/* initialize adaptive blocking matrix */
	gscbeamformer->gscabm =  (objFGSCabm*)calloc(1, sizeof(objFGSCabm));
	dios_ssp_gsc_gscabm_init(gscbeamformer->gscabm, gscbeamformer->m_nMic, gscbeamformer->m_paramGSC.fftlength, gscbeamformer->m_paramGSC.fftoverlap, gscbeamformer->m_paramABM.fftoverlap,
		gscbeamformer->m_paramSync.nDelayABM, gscbeamformer->m_paramABM.lambda, gscbeamformer->m_paramABM.mu, gscbeamformer->m_paramGSC.delta_con, gscbeamformer->m_dwSampRate, 
		gscbeamformer->m_paramABM.tconstfreeze);


	gscbeamformer->m_paramAIC.maxnorm = 0.003f;
	/* initialize adaptive interference canceller */
	gscbeamformer->gscaic =  (objFGSCaic*)calloc(1, sizeof(objFGSCaic));
	dios_ssp_gsc_gscaic_init(gscbeamformer->gscaic, gscbeamformer->m_paramSync.nDelayAIC, gscbeamformer->m_nMic, gscbeamformer->m_paramGSC.fftlength, gscbeamformer->m_paramAIC.maxnorm, gscbeamformer->m_paramAIC.lambda, 
		gscbeamformer->m_paramAIC.mu, gscbeamformer->m_paramGSC.delta_con, gscbeamformer->m_paramGSC.delta_dyn, gscbeamformer->m_paramGSC.s0_dyn, gscbeamformer->m_paramGSC.regularize_dyn, 
		gscbeamformer->m_paramAIC.ntaps, gscbeamformer->m_paramAIC.fftoverlap, gscbeamformer->m_paramGSC.fftoverlap, gscbeamformer->m_dwSampRate, gscbeamformer->m_paramAIC.tconstfreeze);


	/* sync delay for ac reference microphone */
	int acXref = (int)floor((float)ordFbfFilt / 2) + gscbeamformer->m_paramGSC.fftlength / 4;  /* delay of fbf and abm */
	int acYfbf = gscbeamformer->m_paramGSC.fftlength / 4;  /* delay of abm */
	int acCtrlAic = (int)floor((float)(gscbeamformer->m_paramSync.nDelayAIC - acYfbf) 
            * (float)(2 * gscbeamformer->m_paramGSC.fftoverlap) / (float)gscbeamformer->m_paramGSC.fftlength);
	
    /* initialize adaptation control */
	gscbeamformer->gscadaptctrl =  (objFGSCadaptctrl*)calloc(1, sizeof(objFGSCadaptctrl));
	dios_ssp_gsc_gscadaptctrl_init(gscbeamformer->gscadaptctrl, gscbeamformer->m_dwSampRate, gscbeamformer->m_nMic, acXref, acYfbf, acCtrlAic, gscbeamformer->m_paramGSC.fftlength, 
        gscbeamformer->m_paramGSC.fftoverlap, gscbeamformer->m_paramAC.fmin, gscbeamformer->m_paramAC.fmax, gscbeamformer->m_paramAC.fc, gscbeamformer->m_paramAC.ctabm, gscbeamformer->m_paramAC.ctaic, 
        gscbeamformer->m_paramAC.U, gscbeamformer->m_paramAC.V);

	
	/* initialize signals */
	gscbeamformer->m_nGSCUpdateSize = gscbeamformer->m_paramGSC.fftlength / (2 * gscbeamformer->m_paramGSC.fftoverlap);
	gscbeamformer->m_nCCSSize = gscbeamformer->m_paramGSC.fftlength / 2 + 1;

	gscbeamformer->m_input = (float**)calloc(gscbeamformer->m_nMic, sizeof(float*));
	for (int i_mic = 0; i_mic < gscbeamformer->m_nMic; i_mic++)
	{
		gscbeamformer->m_input[i_mic] = (float*)calloc(gscbeamformer->m_nIOBlockSize, sizeof(float));
	}
	param = gscbeamformer->m_nGSCUpdateSize > gscbeamformer->m_nIOBlockSize ? gscbeamformer->m_nGSCUpdateSize : gscbeamformer->m_nIOBlockSize;
	gscbeamformer->m_outSteering = (float**)calloc(gscbeamformer->m_nMic, sizeof(float*));
	for (int i_mic = 0; i_mic < gscbeamformer->m_nMic; i_mic++)
	{
		gscbeamformer->m_outSteering[i_mic] = (float*)calloc(param, sizeof(float));
	}
	gscbeamformer->m_outFBF = (float*)calloc(gscbeamformer->m_nGSCUpdateSize, sizeof(float));
	gscbeamformer->m_ctrlABM = (float*)calloc(gscbeamformer->m_nCCSSize, sizeof(float));
	gscbeamformer->m_ctrlAIC = (float*)calloc(gscbeamformer->m_nCCSSize, sizeof(float));
	gscbeamformer->m_outABM = (float**)calloc(gscbeamformer->m_nMic, sizeof(float*));
	for (int i_mic = 0; i_mic < gscbeamformer->m_nMic; i_mic++)
	{
		gscbeamformer->m_outABM[i_mic] = (float*)calloc(gscbeamformer->m_nGSCUpdateSize, sizeof(float));
	}
	gscbeamformer->m_outAIC = (float*)calloc(gscbeamformer->m_nIOBlockSize, sizeof(float));
	gscbeamformer->m_output = (float*)calloc(gscbeamformer->m_nIOBlockSize, sizeof(float));

	gscbeamformer->m_locMic = (PlaneCoord*)coord;
	gscbeamformer->m_tdoa = (float *)calloc(gscbeamformer->m_nMic, sizeof(float));
	gscbeamformer->m_current_phi = 720.0f * PI / 180.0f;
	gscbeamformer->m_phi_thr = 5.0f * PI / 180.0f;
	gscbeamformer->m_current_abm_phi = 360.0f * PI / 180.0f;
	gscbeamformer->m_abm_phi_thr = 20.0f * PI / 180.0f;

	gscbeamformer->m_soft_vol = 1.0f;
	gscbeamformer->m_alpha_mute = 0.97f;
	gscbeamformer->m_alpha_active = 0.1f;
}

int dios_ssp_gsc_gscbeamformer_reset(objCGSCbeamformer* gscbeamformer)
{
	dios_ssp_gsc_gscbeamsteer_reset(gscbeamformer->gscbeamsteer);
	dios_ssp_gsc_gscfiltsumbeamformer_reset(gscbeamformer->gscfiltsumbeamformer);
	dios_ssp_gsc_gscabm_reset(gscbeamformer->gscabm);

	gscbeamformer->m_paramAIC.maxnorm = 0.003f;
	dios_ssp_gsc_gscaic_reset(gscbeamformer->gscaic);
	dios_ssp_gsc_gscadaptctrl_reset(gscbeamformer->gscadaptctrl);

	int maxTemp = gscbeamformer->m_nGSCUpdateSize > gscbeamformer->m_nIOBlockSize?gscbeamformer->m_nGSCUpdateSize:gscbeamformer->m_nIOBlockSize;
	for (int m = 0; m < gscbeamformer->m_nMic; m++)
	{
		memset(gscbeamformer->m_input[m], 0, sizeof(float) * gscbeamformer->m_nIOBlockSize);
		memset(gscbeamformer->m_outSteering[m], 0, sizeof(float) * maxTemp);
		memset(gscbeamformer->m_outABM[m], 0, sizeof(float) * gscbeamformer->m_nGSCUpdateSize);
	}
	memset(gscbeamformer->m_outFBF, 0, sizeof(float) * gscbeamformer->m_nGSCUpdateSize);
	memset(gscbeamformer->m_ctrlABM, 0, sizeof(float) * gscbeamformer->m_nCCSSize);
	memset(gscbeamformer->m_ctrlAIC, 0, sizeof(float) * gscbeamformer->m_nCCSSize);
	memset(gscbeamformer->m_outAIC, 0, sizeof(float) * gscbeamformer->m_nIOBlockSize);
	memset(gscbeamformer->m_output, 0, sizeof(float) * gscbeamformer->m_nIOBlockSize);
			
	gscbeamformer->m_current_phi = 720.0f * PI / 180.0f;
	gscbeamformer->m_phi_thr = 5.0f * PI / 180.0f;
	gscbeamformer->m_current_abm_phi = 360.0f * PI / 180.0f;
	gscbeamformer->m_abm_phi_thr = 20.0f * PI / 180.0f;

	gscbeamformer->m_soft_vol = 1.0f;
	gscbeamformer->m_alpha_mute = 0.97f;
	gscbeamformer->m_alpha_active = 0.1f;

	return 0;
}

int dios_ssp_gsc_gscbeamformer_arraysteer(objCGSCbeamformer* gscbeamformer, PolarCoord loc)
{
	int i; 
	if (fabs(loc.phi - gscbeamformer->m_current_phi) < gscbeamformer->m_phi_thr)
    {
		return 0;
    }

	PlaneCoord spkloc;
	spkloc.x = loc.rho * (float)cos(loc.theta) * (float)cos(loc.phi);
	spkloc.y = loc.rho * (float)cos(loc.theta) * (float)sin(loc.phi);
	spkloc.z = loc.rho * (float)sin(loc.theta);
	
	for (int ch = 0; ch < gscbeamformer->m_nMic; ch++)
	{
		float temp = spkloc.x - gscbeamformer->m_locMic[ch].x;
		float d = temp * temp;
		temp = spkloc.y - gscbeamformer->m_locMic[ch].y;
		d += temp * temp;
		temp = spkloc.z - gscbeamformer->m_locMic[ch].z;
		d += temp * temp;
		d = (float)sqrt(d);
		gscbeamformer->m_tdoa[ch] = (float)(-d / gsc_c * gscbeamformer->m_dwSampRate);
	}

	float tdoa_min = gscbeamformer->m_tdoa[0];
	for (i = 1; i < gscbeamformer->m_nMic; i++)
	{
		if (gscbeamformer->m_tdoa[i] < tdoa_min)
        {
			tdoa_min = gscbeamformer->m_tdoa[i];
        }
	}
	for (i = 0; i < gscbeamformer->m_nMic; i++)
    {
		gscbeamformer->m_tdoa[i] -= tdoa_min;
    }

	dios_ssp_gsc_gscbeamsteering(gscbeamformer->gscbeamsteer, gscbeamformer->m_tdoa, gscbeamformer->m_dwSampRate);

	if (fabs(loc.phi - gscbeamformer->m_current_abm_phi) > gscbeamformer->m_abm_phi_thr)
	{
		dios_ssp_gsc_gscabm_initabmfreefield(gscbeamformer->gscabm);
		dios_ssp_gsc_gscaic_resetfilterbank(gscbeamformer->gscaic);
		gscbeamformer->m_current_abm_phi = loc.phi;
	}

	gscbeamformer->m_current_phi = loc.phi;

	return 0;
}

int dios_ssp_gsc_gscbeamformer_process(objCGSCbeamformer* gscbeamformer, float** ppInput)
{
	int nBlockBytes = sizeof(float) * gscbeamformer->m_nIOBlockSize;
	for (int i = 0; i < gscbeamformer->m_nMic; i++)
    {
		memcpy(gscbeamformer->m_input[i], ppInput[i], nBlockBytes);
    }
	
	/* beam steering to the desired direction */
	dios_ssp_gsc_gscbeamsteer_process(gscbeamformer->gscbeamsteer, gscbeamformer->m_input, gscbeamformer->m_outSteering);

	/* perform the gsc beamforming processing */
	for (int k = 0; k < gscbeamformer->m_nIOBlockSize; k += gscbeamformer->m_nGSCUpdateSize)  /* 128 / 16 = 8 */
	{
		/* fixed beamformer */
		dios_ssp_gsc_gscfiltsumbeamformer_process(gscbeamformer->gscfiltsumbeamformer, gscbeamformer->m_outSteering, gscbeamformer->m_outFBF, k);
		/* adaptation control */
		dios_ssp_gsc_gscadaptctrl_process(gscbeamformer->gscadaptctrl, gscbeamformer->m_outFBF, gscbeamformer->m_outSteering, k, gscbeamformer->m_ctrlABM, gscbeamformer->m_ctrlAIC);
		/* adaptive blocking matrix */
		dios_ssp_gsc_gscabm_process(gscbeamformer->gscabm, gscbeamformer->m_outSteering, gscbeamformer->m_outFBF, gscbeamformer->m_outABM, gscbeamformer->m_ctrlABM, gscbeamformer->m_ctrlAIC, k);
        /* adaptive interference canceller */
		dios_ssp_gsc_gscaic_process(gscbeamformer->gscaic, gscbeamformer->m_outFBF, gscbeamformer->m_outABM, &gscbeamformer->m_outAIC[k], gscbeamformer->m_ctrlABM, gscbeamformer->m_ctrlAIC);
	}

	float *gsc_out = gscbeamformer->m_outAIC;
    gscbeamformer->m_soft_vol = 1;
	for (int i = 0; i < gscbeamformer->m_nIOBlockSize; i++)
	{
		gscbeamformer->m_output[i] = gscbeamformer->m_soft_vol * gsc_out[i];
	}

	return 0;
}

int dios_ssp_gsc_gscbeamformer_delete(objCGSCbeamformer* gscbeamformer)
{
	dios_ssp_gsc_gscbeamsteer_delete(gscbeamformer->gscbeamsteer);
	free(gscbeamformer->gscbeamsteer);
	dios_ssp_gsc_gscfiltsumbeamformer_delete(gscbeamformer->gscfiltsumbeamformer);
	free(gscbeamformer->gscfiltsumbeamformer);
	dios_ssp_gsc_gscabm_delete(gscbeamformer->gscabm);
	free(gscbeamformer->gscabm);
	dios_ssp_gsc_gscaic_delete(gscbeamformer->gscaic);
	free(gscbeamformer->gscaic);
	dios_ssp_gsc_gscadaptctrl_delete(gscbeamformer->gscadaptctrl);
	free(gscbeamformer->gscadaptctrl);

	for (int i_mic = 0; i_mic < gscbeamformer->m_nMic; i_mic++)
	{
		free(gscbeamformer->m_input[i_mic]);
	}
	free(gscbeamformer->m_input);
	for (int i_mic = 0; i_mic < gscbeamformer->m_nMic; i_mic++)
	{
		free(gscbeamformer->m_outSteering[i_mic]);
	}
	free(gscbeamformer->m_outSteering);
	free(gscbeamformer->m_outFBF);
	free(gscbeamformer->m_ctrlABM);
	free(gscbeamformer->m_ctrlAIC);
	for (int i_mic = 0; i_mic < gscbeamformer->m_nMic; i_mic++)
	{
		free(gscbeamformer->m_outABM[i_mic]);
	}
	free(gscbeamformer->m_outABM);
	free(gscbeamformer->m_outAIC);
	free(gscbeamformer->m_output);
	free(gscbeamformer->m_tdoa);

	return 0;
}
