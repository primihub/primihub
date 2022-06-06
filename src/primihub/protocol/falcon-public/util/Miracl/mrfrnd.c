
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
 *   MIRACL flash random number routine 
 *   mrfrnd.c
 */

#include "miracl.h"

#ifdef MR_FP
#include <math.h>
#endif

#ifdef MR_FLASH

#ifndef MR_NO_RAND

void frand(_MIPD_ flash x)
{ /* generates random flash number 0<x<1 */
    int i;
#ifdef MR_FP
    mr_small dres;
#endif
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;

    MR_IN(46)

    zero(mr_mip->w6);
    mr_mip->w6->len=mr_mip->nib;
    for (i=0;i<mr_mip->nib;i++) 
    { /* generate a full width random number */
        if (mr_mip->base==0) mr_mip->w6->w[i]=brand(_MIPPO_ );
        else                 mr_mip->w6->w[i]=MR_REMAIN(brand(_MIPPO_ ),mr_mip->base);
    }
    mr_mip->check=OFF;
    bigrand(_MIPP_ mr_mip->w6,mr_mip->w5);
    mr_mip->check=ON;
    mround(_MIPP_ mr_mip->w5,mr_mip->w6,x);

    MR_OUT
}

#endif

#endif

