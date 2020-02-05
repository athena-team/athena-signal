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

Description: Calculate the inverse of matrix Rnn.
==============================================================================*/

#include  "dios_ssp_mvdr_cinv.h"

void dios_ssp_mvdr_inv_init(objMVDRCinv *mvdrinv, int Rdim)
{
	int i;
	mvdrinv->dim = Rdim;

	mvdrinv->ar = (float**)calloc(mvdrinv->dim, sizeof(float*));
	for(i = 0; i < mvdrinv->dim; ++i)
	{
		mvdrinv->ar[i] = (float*)calloc(mvdrinv->dim, sizeof(float));
	}
		
	mvdrinv->ai = (float**)calloc(mvdrinv->dim, sizeof(float*));
	for(i = 0; i < mvdrinv->dim; ++i)
	{
		mvdrinv->ai[i] = (float*)calloc(mvdrinv->dim, sizeof(float));
	}
	
	mvdrinv->mat_temp = (float**)calloc(mvdrinv->dim, sizeof(float*));
	for(i = 0; i < mvdrinv->dim; ++i)
	{
		mvdrinv->mat_temp[i] = (float*)calloc(mvdrinv->dim, sizeof(float));
	}

	mvdrinv->mat_temp2 = (float**)calloc(mvdrinv->dim, sizeof(float*));
	for(i = 0; i < mvdrinv->dim; ++i)
	{
		mvdrinv->mat_temp2[i] = (float*)calloc(mvdrinv->dim, sizeof(float));
	}
}

int dios_ssp_mvdr_inv_process(objMVDRCinv *mvdrinv, float *R, float *Rinv)
{
	int *is,*js;
	is = (int *)calloc(mvdrinv->dim, sizeof(int));
	js = (int *)calloc(mvdrinv->dim, sizeof(int));
	int i, j, k;
	float p, q, s, t, d, b;
	for(i = 0; i<mvdrinv->dim; i++)		
	{			
		for(j = 0; j<mvdrinv->dim; j++)			
		{						
			mvdrinv->ar[i][j] = R[i*mvdrinv->dim*2+2*j];
			mvdrinv->ai[i][j] = R[i*mvdrinv->dim*2+2*j+1];	
		}				
	}
	for (k = 0; k <= mvdrinv->dim-1; k++)
	{
		d = 0.0;
		for(i = k; i <= mvdrinv->dim-1; i++)
		{
			for(j = k; j <= mvdrinv->dim-1; j++)
			{
				p = mvdrinv->ar[i][j]*mvdrinv->ar[i][j]+mvdrinv->ai[i][j]*mvdrinv->ai[i][j];
				if( p>d )
				{ 
					d=p;
					is[k]=i; 
					js[k]=j;
				}
			}
		}
//		printf("%d, %f \n", k, d);
		if (d+1.0==1.0)
		{
			free(is);
			free(js);
			printf("The matrix is singular!");
			exit(-1);
		}
		if (is[k]!=k)
		{
			for(j = 0; j <= mvdrinv->dim-1; j++)
			{
				t=mvdrinv->ar[k][j]; mvdrinv->ar[k][j]=mvdrinv->ar[is[k]][j]; mvdrinv->ar[is[k]][j]=t;
				t=mvdrinv->ai[k][j]; mvdrinv->ai[k][j]=mvdrinv->ai[is[k]][j]; mvdrinv->ai[is[k]][j]=t;
			}
		}
		if (js[k]!=k)
		{
			for(i = 0; i <= mvdrinv->dim-1; i++)
			{ 
				t=mvdrinv->ar[i][k]; mvdrinv->ar[i][k]=mvdrinv->ar[i][js[k]]; mvdrinv->ar[i][js[k]]=t;
				t=mvdrinv->ai[i][k]; mvdrinv->ai[i][k]=mvdrinv->ai[i][js[k]]; mvdrinv->ai[i][js[k]]=t;
			}
		}
		mvdrinv->ar[k][k] = mvdrinv->ar[k][k]/d; mvdrinv->ai[k][k]=-mvdrinv->ai[k][k]/d;
		for(j=0; j<=mvdrinv->dim-1; j++)
		{
			if (j!=k)
			{
				p=mvdrinv->ar[k][j]*mvdrinv->ar[k][k]; q=mvdrinv->ai[k][j]*mvdrinv->ai[k][k];
				s=(mvdrinv->ar[k][j]+mvdrinv->ai[k][j])*(mvdrinv->ar[k][k]+mvdrinv->ai[k][k]);
				mvdrinv->ar[k][j]=p-q; mvdrinv->ai[k][j]=s-p-q;
			}
		}
		for(i = 0; i <= mvdrinv->dim-1; i++)
		{
			if (i!=k)
			{ 
				for(j = 0; j <= mvdrinv->dim-1; j++)
				{
					if (j!=k)
					{ 
						p=mvdrinv->ar[k][j]*mvdrinv->ar[i][k]; q=mvdrinv->ai[k][j]*mvdrinv->ai[i][k];
						s=(mvdrinv->ar[k][j]+mvdrinv->ai[k][j])*(mvdrinv->ar[i][k]+mvdrinv->ai[i][k]);
						t=p-q; b=s-p-q;
						mvdrinv->ar[i][j]=mvdrinv->ar[i][j]-t;
						mvdrinv->ai[i][j]=mvdrinv->ai[i][j]-b;
					}
				}
			}
		}
		for(i = 0; i<=mvdrinv->dim-1; i++)
		{
			if (i!=k)
			{ 
				p=mvdrinv->ar[i][k]*mvdrinv->ar[k][k]; q=mvdrinv->ai[i][k]*mvdrinv->ai[k][k];
				s=(mvdrinv->ar[i][k]+mvdrinv->ai[i][k])*(mvdrinv->ar[k][k]+mvdrinv->ai[k][k]);
				mvdrinv->ar[i][k]=q-p; mvdrinv->ai[i][k]=p+q-s;
			}
		}
	}
	for(k = mvdrinv->dim-1; k >= 0; k--)
	{ 
		if( js[k]!=k )
		{
			for(j=0; j <= mvdrinv->dim-1; j++)
			{ 
				t=mvdrinv->ar[k][j]; mvdrinv->ar[k][j]=mvdrinv->ar[js[k]][j]; mvdrinv->ar[js[k]][j]=t;
				t=mvdrinv->ai[k][j]; mvdrinv->ai[k][j]=mvdrinv->ai[js[k]][j]; mvdrinv->ai[js[k]][j]=t; 
			}
		}
		if( is[k]!=k )
		{
			for(i = 0; i <= mvdrinv->dim-1; i++)
			{ 
				t=mvdrinv->ar[i][k]; mvdrinv->ar[i][k]=mvdrinv->ar[i][is[k]]; mvdrinv->ar[i][is[k]]=t;
				t=mvdrinv->ai[i][k]; mvdrinv->ai[i][k]=mvdrinv->ai[i][is[k]]; mvdrinv->ai[i][is[k]]=t;
			}
		}
	}
	free(is);
	free(js);
	for(i = 0; i < mvdrinv->dim; i++)
	{
		for (j = 0; j<mvdrinv->dim; j++)
		{
			Rinv[i*mvdrinv->dim*2+2*j] = mvdrinv->ar[i][j];
			Rinv[i*mvdrinv->dim*2+2*j+1] = mvdrinv->ai[i][j];
		}
	}
	
	return 0;
}

int dios_ssp_mvdr_inv_delete(objMVDRCinv *mvdrinv)
{
    int i;
	for(i = 0; i < mvdrinv->dim; ++i)
	{
		free(mvdrinv->ar[i]);
		free(mvdrinv->ai[i]);
		free(mvdrinv->mat_temp[i]);
		free(mvdrinv->mat_temp2[i]);
	}
	
	free(mvdrinv->ar);
	free(mvdrinv->ai);
	free(mvdrinv->mat_temp);
	free(mvdrinv->mat_temp2);

	return 0;
}

