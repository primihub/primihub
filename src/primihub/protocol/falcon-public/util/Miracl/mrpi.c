
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
 *   MIRACL calculate pi - by Gauss-Legendre method
 *   mrpi.c
 */

#include <stdlib.h> 
#include "miracl.h"

#ifdef MR_FLASH  

void fpi(_MIPD_ flash pi)
{ /* Calculate pi using Guass-Legendre method */
    int x,nits,op[5];
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;

    MR_IN(53)

    if (size(mr_mip->pi)!=0)
    {
        copy(mr_mip->pi,pi);
        mr_mip->EXACT=FALSE;
        MR_OUT
        return;
    }
 
    fconv(_MIPP_ 1,2,mr_mip->pi);
    froot(_MIPP_ mr_mip->pi,2,mr_mip->pi);
    fconv(_MIPP_ 1,1,mr_mip->w11);
    fconv(_MIPP_ 1,4,mr_mip->w12);
    x=1;
    op[0]=0x6C;
    op[1]=1;
    op[4]=0;
    nits=mr_mip->lg2b*mr_mip->nib/4;
    while (x<nits)
    {
        copy(mr_mip->w11,mr_mip->w13);
        op[2]=1;
        op[3]=2;
        flop(_MIPP_ mr_mip->w11,mr_mip->pi,op,mr_mip->w11);
        fmul(_MIPP_ mr_mip->pi,mr_mip->w13,mr_mip->pi);
        froot(_MIPP_ mr_mip->pi,2,mr_mip->pi);
        fsub(_MIPP_ mr_mip->w11,mr_mip->w13,mr_mip->w13);
        fmul(_MIPP_ mr_mip->w13,mr_mip->w13,mr_mip->w13);
        op[3]=1;
        op[2]=(-x);
        flop(_MIPP_ mr_mip->w12,mr_mip->w13,op,mr_mip->w12);  /* w12 = w12 - x.w13 */
        x*=2;
    }
    fadd(_MIPP_ mr_mip->w11,mr_mip->pi,mr_mip->pi);
    fmul(_MIPP_ mr_mip->pi,mr_mip->pi,mr_mip->pi);
    op[0]=0x48;
    op[2]=0;
    op[3]=4;
    flop(_MIPP_ mr_mip->pi,mr_mip->w12,op,mr_mip->pi);   /* pi = pi/(4.w12) */
    if (pi!=NULL) copy(mr_mip->pi,pi);   
    MR_OUT
}

#endif

