
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
 *   MIRACL random number routines 
 *   mrrand.c
 */

#include "miracl.h"

#ifdef MR_FP
#include <math.h>
#endif

#ifndef MR_NO_RAND

void bigrand(_MIPD_ big w,big x)
{  /*  generate a big random number 0<=x<w  */
    int m;
    mr_small r;
#ifdef MR_FP
    mr_small dres;
#endif
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;

    MR_IN(20)

 /*   decr(_MIPP_ w,2,w);  */
    m=0;
    zero(mr_mip->w0);

    do
    { /* create big rand piece by piece */
        m++;
        mr_mip->w0->len=m;
        r=brand(_MIPPO_ );
        if (mr_mip->base==0) mr_mip->w0->w[m-1]=r;
        else                 mr_mip->w0->w[m-1]=MR_REMAIN(r,mr_mip->base);
    } while (mr_compare(mr_mip->w0,w)<0);
    mr_lzero(mr_mip->w0);
    divide(_MIPP_ mr_mip->w0,w,w);

    copy(mr_mip->w0,x);
 /*   incr(_MIPP_ x,2,x);
    if (w!=x) incr(_MIPP_ w,2,w); */
    MR_OUT
}

void bigdig(_MIPD_ int n,int b,big x)
{ /* generate random number n digits long *
   * to "printable" base b                */
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;

    MR_IN(19)

    if (b<2 || b>256)
    {
        mr_berror(_MIPP_ MR_ERR_BASE_TOO_BIG);
        MR_OUT
        return;
    }

    do
    { /* repeat if x too small */
        expint(_MIPP_ b,n,mr_mip->w1);
        bigrand(_MIPP_ mr_mip->w1,x);
        subdiv(_MIPP_ mr_mip->w1,b,mr_mip->w1);
    } while (!mr_mip->ERNUM && mr_compare(x,mr_mip->w1)<0);

    MR_OUT
}

#endif
