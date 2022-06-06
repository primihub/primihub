
/*
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
*/
/*
 *   MIRACL Comba's method for ultimate speed binary polynomial
 *   mrcomba2.tpl 
 *
 *   Here the inner loops of the basic multiplication, and squaring  
 *   algorithms are completely unravelled, and  reorganised for maximum possible speed. 
 *
 *   This approach is recommended for maximum speed where parameters
 *   are fixed and compute resources are constrained. The processor MUST 
 *   support a special binary polynomial multiplication instruction
 *
 *   This file is a template. To fill in the gaps and create mrcomba2.c, 
 *   you must run the mex.c program to insert the C or assembly language 
 *   macros from the appropriate .mcs file. 
 *
 *   This method would appear to be particularly useful for implementing 
 *   fast Elliptic Curve Cryptosystems over GF(2^m) 
 *
 *   The #define MR_COMBA2 in mirdef.h determines the FIXED size of 
 *   modulus to be used. This *must* be determined at compile time. 
 *
 *   Note that this module can generate a *lot* of code for large values 
 *   of MR_COMBA2. This should have a maximum value of 8-20.
 *
 *   Note that on some processors it is *VITAL* that arrays be aligned on 
 *   4-byte boundaries
 *
 * *  **** This code does not like -fomit-frame-pointer using GCC  ***********
 *
 */

#include "miracl.h"    

#ifdef MR_COMBA2

#ifdef MR_WIN64
#if _MSC_VER>=1500
#define MR_PCLMULQDQ
#include <wmmintrin.h>
#endif
#endif

/* NOTE! z must be distinct from x and y */

void comba_mult2(_MIPD_ big x,big y,big z) 
{ /* comba multiplier */
    int i;
    mr_small *a,*b,*c;
    big w;
#ifdef MR_PCLMULQDQ
    __m128i m1,m2,sum;
#endif
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    w=mr_mip->w0; 
    for (i=2*MR_COMBA2;i<(int)(w->len&MR_OBITS);i++) w->w[i]=0;
    w->len=2*MR_COMBA2-1;
    a=x->w; b=y->w; c=w->w;
/*** MULTIPLY2 ***/      /* multiply a by b, result in c */
	
    mr_lzero(w);
    if (w!=z) copy (w,z); 
}   
 
#endif
