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

Description: Calculate the inverse of complex matrix.
==============================================================================*/

#include  "dios_ssp_share_cinv.h"

void *dios_ssp_matrix_inv_init(int Rdim)
{
    void *matrix_inv = NULL;
    matrix_inv = (void*)calloc(1, sizeof(objMATRIXinv));
    objMATRIXinv *matrixinv;
    matrixinv = (objMATRIXinv*)matrix_inv;
	int i;

	matrixinv->dim = Rdim;
	matrixinv->ar = (float**)calloc(matrixinv->dim, sizeof(float*));
	for(i = 0; i < matrixinv->dim; ++i)
	{
		matrixinv->ar[i] = (float*)calloc(matrixinv->dim, sizeof(float));
	}
		
	matrixinv->ai = (float**)calloc(matrixinv->dim, sizeof(float*));
	for(i = 0; i < matrixinv->dim; ++i)
	{
		matrixinv->ai[i] = (float*)calloc(matrixinv->dim, sizeof(float));
	}
	
	matrixinv->mat_temp = (float**)calloc(matrixinv->dim, sizeof(float*));
	for(i = 0; i < matrixinv->dim; ++i)
	{
		matrixinv->mat_temp[i] = (float*)calloc(matrixinv->dim, sizeof(float));
	}

	matrixinv->mat_temp2 = (float**)calloc(matrixinv->dim, sizeof(float*));
	for(i = 0; i < matrixinv->dim; ++i)
	{
		matrixinv->mat_temp2[i] = (float*)calloc(matrixinv->dim, sizeof(float));
	}

    return (matrix_inv);
}

int dios_ssp_matrix_inv_process(void *matrix_inv, float *R, float *Rinv)
{
    objMATRIXinv *matrixinv;
    matrixinv = (objMATRIXinv*)matrix_inv;

	int *is,*js;
	is = (int *)calloc(matrixinv->dim, sizeof(int));
	js = (int *)calloc(matrixinv->dim, sizeof(int));
	int i, j, k;
	float p, q, s, t, d, b;
	for(i = 0; i<matrixinv->dim; i++)		
	{			
		for(j = 0; j<matrixinv->dim; j++)			
		{						
			matrixinv->ar[i][j] = R[i*matrixinv->dim*2+2*j];
			matrixinv->ai[i][j] = R[i*matrixinv->dim*2+2*j+1];	
		}				
	}
	for (k = 0; k <= matrixinv->dim-1; k++)
	{
		d = 0.0;
		for(i = k; i <= matrixinv->dim-1; i++)
		{
			for(j = k; j <= matrixinv->dim-1; j++)
			{
				p = matrixinv->ar[i][j]*matrixinv->ar[i][j]+matrixinv->ai[i][j]*matrixinv->ai[i][j];
				if( p>d )
				{ 
					d=p;
					is[k]=i; 
					js[k]=j;
				}
			}
		}
		if (d+1.0==1.0)
		{
			free(is);
			free(js);
			printf("The matrix is singular!");
			exit(-1);
		}
		if (is[k]!=k)
		{
			for(j = 0; j <= matrixinv->dim-1; j++)
			{
				t=matrixinv->ar[k][j]; matrixinv->ar[k][j]=matrixinv->ar[is[k]][j]; matrixinv->ar[is[k]][j]=t;
				t=matrixinv->ai[k][j]; matrixinv->ai[k][j]=matrixinv->ai[is[k]][j]; matrixinv->ai[is[k]][j]=t;
			}
		}
		if (js[k]!=k)
		{
			for(i = 0; i <= matrixinv->dim-1; i++)
			{ 
				t=matrixinv->ar[i][k]; matrixinv->ar[i][k]=matrixinv->ar[i][js[k]]; matrixinv->ar[i][js[k]]=t;
				t=matrixinv->ai[i][k]; matrixinv->ai[i][k]=matrixinv->ai[i][js[k]]; matrixinv->ai[i][js[k]]=t;
			}
		}
		matrixinv->ar[k][k] = matrixinv->ar[k][k]/d; matrixinv->ai[k][k]=-matrixinv->ai[k][k]/d;
		for(j=0; j<=matrixinv->dim-1; j++)
		{
			if (j!=k)
			{
				p=matrixinv->ar[k][j]*matrixinv->ar[k][k]; q=matrixinv->ai[k][j]*matrixinv->ai[k][k];
				s=(matrixinv->ar[k][j]+matrixinv->ai[k][j])*(matrixinv->ar[k][k]+matrixinv->ai[k][k]);
				matrixinv->ar[k][j]=p-q; matrixinv->ai[k][j]=s-p-q;
			}
		}
		for(i = 0; i <= matrixinv->dim-1; i++)
		{
			if (i!=k)
			{ 
				for(j = 0; j <= matrixinv->dim-1; j++)
				{
					if (j!=k)
					{ 
						p=matrixinv->ar[k][j]*matrixinv->ar[i][k]; q=matrixinv->ai[k][j]*matrixinv->ai[i][k];
						s=(matrixinv->ar[k][j]+matrixinv->ai[k][j])*(matrixinv->ar[i][k]+matrixinv->ai[i][k]);
						t=p-q; b=s-p-q;
						matrixinv->ar[i][j]=matrixinv->ar[i][j]-t;
						matrixinv->ai[i][j]=matrixinv->ai[i][j]-b;
					}
				}
			}
		}
		for(i = 0; i<=matrixinv->dim-1; i++)
		{
			if (i!=k)
			{ 
				p=matrixinv->ar[i][k]*matrixinv->ar[k][k]; q=matrixinv->ai[i][k]*matrixinv->ai[k][k];
				s=(matrixinv->ar[i][k]+matrixinv->ai[i][k])*(matrixinv->ar[k][k]+matrixinv->ai[k][k]);
				matrixinv->ar[i][k]=q-p; matrixinv->ai[i][k]=p+q-s;
			}
		}
	}
	for(k = matrixinv->dim-1; k >= 0; k--)
	{ 
		if( js[k]!=k )
		{
			for(j=0; j <= matrixinv->dim-1; j++)
			{ 
				t=matrixinv->ar[k][j]; matrixinv->ar[k][j]=matrixinv->ar[js[k]][j]; matrixinv->ar[js[k]][j]=t;
				t=matrixinv->ai[k][j]; matrixinv->ai[k][j]=matrixinv->ai[js[k]][j]; matrixinv->ai[js[k]][j]=t; 
			}
		}
		if( is[k]!=k )
		{
			for(i = 0; i <= matrixinv->dim-1; i++)
			{ 
				t=matrixinv->ar[i][k]; matrixinv->ar[i][k]=matrixinv->ar[i][is[k]]; matrixinv->ar[i][is[k]]=t;
				t=matrixinv->ai[i][k]; matrixinv->ai[i][k]=matrixinv->ai[i][is[k]]; matrixinv->ai[i][is[k]]=t;
			}
		}
	}

	for(i = 0; i < matrixinv->dim; i++)
	{
		for (j = 0; j<matrixinv->dim; j++)
		{
			Rinv[i*matrixinv->dim*2+2*j] = matrixinv->ar[i][j];
			Rinv[i*matrixinv->dim*2+2*j+1] = matrixinv->ai[i][j];
		}
	}
	
	free(is);
	free(js);
	
	return 0;
}

int dios_ssp_matrix_inv_delete(void *matrix_inv)
{
    objMATRIXinv *matrixinv;
    matrixinv = (objMATRIXinv*)matrix_inv;

    int i;
	for(i = 0; i < matrixinv->dim; ++i)
	{
		free(matrixinv->ar[i]);
		free(matrixinv->ai[i]);
		free(matrixinv->mat_temp[i]);
		free(matrixinv->mat_temp2[i]);
	}
	
	free(matrixinv->ar);
	free(matrixinv->ai);
	free(matrixinv->mat_temp);
	free(matrixinv->mat_temp2);

	return 0;
}

