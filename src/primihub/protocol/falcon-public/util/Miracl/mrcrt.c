
/***************************************************************************
                                                                           *
Copyright 2013 CertiVox IOM Ltd.                                           *
                                                                           *
This file is part of CertiVox MIRACL Crypto SDK.                           *
                                                                           *
The CertiVox MIRACL Crypto SDK provides developers with an                 *
extensive and efficient set of cryptographic functions.                    *
For further information about its features and functionalities please      *
refer to http://www.certivox.com                                           *
                                                                           *
* The CertiVox MIRACL Crypto SDK is free software: you can                 *
  redistribute it and/or modify it under the terms of the                  *
  GNU Affero General Public License as published by the                    *
  Free Software Foundation, either version 3 of the License,               *
  or (at your option) any later version.                                   *
                                                                           *
* The CertiVox MIRACL Crypto SDK is distributed in the hope                *
  that it will be useful, but WITHOUT ANY WARRANTY; without even the       *
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. *
  See the GNU Affero General Public License for more details.              *
                                                                           *
* You should have received a copy of the GNU Affero General Public         *
  License along with CertiVox MIRACL Crypto SDK.                           *
  If not, see <http://www.gnu.org/licenses/>.                              *
                                                                           *
You can be released from the requirements of the license by purchasing     *
a commercial license. Buying such a license is mandatory as soon as you    *
develop commercial activities involving the CertiVox MIRACL Crypto SDK     *
without disclosing the source code of your own applications, or shipping   *
the CertiVox MIRACL Crypto SDK with a closed source product.               *
                                                                           *
***************************************************************************/
/*
 *   MIRACL Chinese Remainder Thereom routines (for use with big moduli) 
 *   mrcrt.c
 */

#include <stdlib.h>
#include "miracl.h"

#ifndef MR_STATIC

BOOL crt_init(_MIPD_ big_chinese *c,int r,big *moduli)
{ /* calculate CRT constants */
    int i,j,k;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (r<2 || mr_mip->ERNUM) return FALSE;
    for (i=0;i<r;i++) if (size(moduli[i])<2) return FALSE;

    MR_IN(73)

    c->M=(big *)mr_alloc(_MIPP_ r,sizeof(big));
    if (c->M==NULL) 
    {
        mr_berror(_MIPP_ MR_ERR_OUT_OF_MEMORY);
        MR_OUT
        return FALSE;
    }
    c->C=(big *)mr_alloc(_MIPP_ r*(r-1)/2,sizeof(big));
    if (c->C==NULL)
    {
        mr_free(c->M);
        mr_berror(_MIPP_ MR_ERR_OUT_OF_MEMORY);
        MR_OUT
        return FALSE;
    }
    c->V=(big *)mr_alloc(_MIPP_ r,sizeof(big));
    if (c->V==NULL)
    {
        mr_free(c->M);
        mr_free(c->C);
        mr_berror(_MIPP_ MR_ERR_OUT_OF_MEMORY);
        MR_OUT
        return FALSE;
    }
    for (k=0,i=0;i<r;i++)
    { 
        c->V[i]=mirvar(_MIPP_ 0);
        c->M[i]=mirvar(_MIPP_ 0);
        copy(moduli[i],c->M[i]);
        for (j=0;j<i;j++,k++)
        {
            c->C[k]=mirvar(_MIPP_ 0);
            invmodp(_MIPP_ c->M[j],c->M[i],c->C[k]);
        }
    }
    c->NP=r;    
    MR_OUT
    return TRUE;
}

void crt_end(big_chinese *c)
{ /* clean up after CRT */
    int i,j,k;
    if (c->NP<2) return;
    for (k=0,i=0;i<c->NP;i++)
    {
        mirkill(c->M[i]);
        for (j=0;j<i;j++)
            mirkill(c->C[k++]); 
        mirkill(c->V[i]);
    }   
    mr_free(c->M);
    mr_free(c->V);
    mr_free(c->C);
    c->NP=0;
}

#endif

void crt(_MIPD_ big_chinese *c,big *u,big x)
{ /* Chinese Remainder Thereom                  *
   * Calculate x given remainders u[i] mod M[i] */
    int i,j,k;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (c->NP<2 || mr_mip->ERNUM) return;

    MR_IN(74)

    copy(u[0],c->V[0]);
    for (k=0,i=1;i<c->NP;i++)
    { /* Knuth page 274 */
        subtract(_MIPP_ u[i],c->V[0],c->V[i]);
        mad(_MIPP_ c->V[i],c->C[k],c->C[k],c->M[i],c->M[i],c->V[i]);
        k++;
        for (j=1;j<i;j++,k++)
        {
            subtract(_MIPP_ c->V[i],c->V[j],c->V[i]);
            mad(_MIPP_ c->V[i],c->C[k],c->C[k],c->M[i],c->M[i],c->V[i]);
        }
        if (size(c->V[i])<0) add(_MIPP_ c->V[i],c->M[i],c->V[i]);
    }
    zero(x);
    convert(_MIPP_ 1,mr_mip->w1);
    for (i=0;i<c->NP;i++)
    {
        multiply(_MIPP_ mr_mip->w1,c->V[i],mr_mip->w2);
        add(_MIPP_ x,mr_mip->w2,x);         
        multiply(_MIPP_ mr_mip->w1,c->M[i],mr_mip->w1);
    }   
    MR_OUT
}

