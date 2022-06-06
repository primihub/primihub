
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
 *   MIRACL flash hyperbolic trig.
 *   mrflsh4.c
 */

#include "miracl.h"

#ifdef MR_FLASH

void ftanh(_MIPD_ flash x,flash y)
{ /* calculates y=tanh(x) */
    int op[5];
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    copy(x,y);
    if (mr_mip->ERNUM || size(y)==0) return;

    MR_IN(63)
    fexp(_MIPP_ y,y);
    op[0]=0x33;
    op[1]=op[3]=op[4]=1;
    op[2]=(-1);
    flop(_MIPP_ y,y,op,y);
    MR_OUT
}

void fatanh(_MIPD_ flash x,flash y)
{ /* calculate y=atanh(x) */
    int op[5];
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    copy(x,y);
    if (mr_mip->ERNUM || size(y)==0) return;

    MR_IN(64)
    fconv(_MIPP_ 1,1,mr_mip->w11);
    op[0]=0x66;
    op[1]=op[2]=op[3]=1;
    op[4]=(-1);
    flop(_MIPP_ mr_mip->w11,y,op,y);
    flog(_MIPP_ y,y);
    fpmul(_MIPP_ y,1,2,y);
    MR_OUT
}

void fsinh(_MIPD_ flash x,flash y)
{ /*  calculate y=sinh(x) */
    int op[5];
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    copy(x,y);
    if (mr_mip->ERNUM || size(y)==0) return;

    MR_IN(65)
    fexp(_MIPP_ y,y);
    op[0]=0xC6;
    op[2]=op[3]=op[4]=1;
    op[1]=(-1);
    flop(_MIPP_ y,y,op,y);
    MR_OUT
}

void fasinh(_MIPD_ flash x,flash y)
{ /* calculate y=asinh(x) */
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    copy(x,y);
    if (mr_mip->ERNUM || size(y)==0) return;

    MR_IN(66)
    fmul(_MIPP_ y,y,mr_mip->w11);
    fincr(_MIPP_ mr_mip->w11,1,1,mr_mip->w11);
    froot(_MIPP_ mr_mip->w11,2,mr_mip->w11);
    fadd(_MIPP_ y,mr_mip->w11,y);
    flog(_MIPP_ y,y);
    MR_OUT
}

void fcosh(_MIPD_ flash x,flash y)
{ /* calculate y=cosh(x) */
    int op[5];
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    copy(x,y);
    if (mr_mip->ERNUM || size(y)==0)
    {
        convert(_MIPP_ 1,y);
        return;
    }

    MR_IN(67)
    fexp(_MIPP_ y,y);
    op[0]=0xC6;
    op[1]=op[2]=op[3]=op[4]=1;
    flop(_MIPP_ y,y,op,y);
    MR_OUT
}

void facosh(_MIPD_ flash x,flash y)
{ /* calculate y=acosh(x) */
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    copy(x,y);
    if (mr_mip->ERNUM) return;

    MR_IN(68)
    fmul(_MIPP_ y,y,mr_mip->w11);
    fincr(_MIPP_ mr_mip->w11,(-1),1,mr_mip->w11);
    froot(_MIPP_ mr_mip->w11,2,mr_mip->w11);
    fadd(_MIPP_ y,mr_mip->w11,y);
    flog(_MIPP_ y,y);
    MR_OUT
}

#endif

