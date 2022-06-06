
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
 *   MIRACL Greatest Common Divisor module.
 *   mrgcd.c
 */

#include "miracl.h"

#ifdef MR_FP
#include <math.h>
#endif

int egcd(_MIPD_ big x,big y,big z)
{ /* greatest common divisor z=gcd(x,y) by Euclids  *
   * method using Lehmers algorithm for big numbers */
    int q,r,a,b,c,d,n;
    mr_small sr,m,sm;
    mr_small u,v,lq,lr;
#ifdef MR_FP
    mr_small dres;
#endif
    big t;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return 0;

    MR_IN(12)

    copy(x,mr_mip->w1);
    copy(y,mr_mip->w2);
    insign(PLUS,mr_mip->w1);
    insign(PLUS,mr_mip->w2);
    a=b=c=d=0;
    while (size(mr_mip->w2)!=0)
    {
	 /*	printf("a= %d b= %d c= %d d=%d\n",a,b,c,d); */
        if (b==0)
        { /* update w1 and w2 */
            divide(_MIPP_ mr_mip->w1,mr_mip->w2,mr_mip->w2);
            t=mr_mip->w1,mr_mip->w1=mr_mip->w2,mr_mip->w2=t;   /* swap(w1,w2) */
        }
        else
        {
            premult(_MIPP_ mr_mip->w1,c,z);
            premult(_MIPP_ mr_mip->w1,a,mr_mip->w1);
            premult(_MIPP_ mr_mip->w2,b,mr_mip->w0);
            premult(_MIPP_ mr_mip->w2,d,mr_mip->w2);
            add(_MIPP_ mr_mip->w1,mr_mip->w0,mr_mip->w1);
            add(_MIPP_ mr_mip->w2,z,mr_mip->w2);
        }
        if (mr_mip->ERNUM || size(mr_mip->w2)==0) break;
        n=(int)mr_mip->w1->len;
        if (mr_mip->w2->len==1)
        { /* special case if mr_mip->w2 is now small */
            sm=mr_mip->w2->w[0];    
#ifdef MR_FP_ROUNDING
            sr=mr_sdiv(_MIPP_ mr_mip->w1,sm,mr_invert(sm),mr_mip->w1);
#else
            sr=mr_sdiv(_MIPP_ mr_mip->w1,sm,mr_mip->w1);
#endif
            if (sr==0)
            {
                copy(mr_mip->w2,mr_mip->w1);
                break;
            }
            zero(mr_mip->w1);
            mr_mip->w1->len=1;
            mr_mip->w1->w[0]=sr;
            while ((sr=MR_REMAIN(mr_mip->w2->w[0],mr_mip->w1->w[0]))!=0)
                  mr_mip->w2->w[0]=mr_mip->w1->w[0],mr_mip->w1->w[0]=sr;
            break;
        }
        a=1;
        b=0;
        c=0;
        d=1;
        m=mr_mip->w1->w[n-1]+1;
    /*    printf("m= %d\n",m); */
#ifndef MR_SIMPLE_BASE
        if (mr_mip->base==0)
        {
#endif
#ifndef MR_NOFULLWIDTH
            if (m==0)
            {
                u=mr_mip->w1->w[n-1];
                v=mr_mip->w2->w[n-1];
            }
            else
            {
			/*		printf("w1[n-1]= %d w1[n-2]= %d\n", mr_mip->w1->w[n-1],mr_mip->w1->w[n-2]);
					printf("w2[n-1]= %d w2[n-2]= %d\n", mr_mip->w2->w[n-1],mr_mip->w2->w[n-2]);*/
                u=muldvm(mr_mip->w1->w[n-1],mr_mip->w1->w[n-2],m,&sr);
                v=muldvm(mr_mip->w2->w[n-1],mr_mip->w2->w[n-2],m,&sr);
            }
#endif
#ifndef MR_SIMPLE_BASE
        }
        else
        {
            u=muldiv(mr_mip->w1->w[n-1],mr_mip->base,mr_mip->w1->w[n-2],m,&sr);
            v=muldiv(mr_mip->w2->w[n-1],mr_mip->base,mr_mip->w2->w[n-2],m,&sr);
        }
#endif
     /*   printf("u= %d v= %d\n",u,v);*/

        forever
        { /* work only with most significant piece */
            if (((v+c)==0) || ((v+d)==0)) break;
            lq=MR_DIV((u+a),(v+c));
            if (lq!=MR_DIV((u+b),(v+d))) break;
            if (lq>=(mr_small)(MR_TOOBIG/mr_abs(d))) break;
            q=(int)lq;
            r=a-q*c;
            a=c;
            c=r;
            r=b-q*d;
            b=d;
            d=r;
            lr=u-lq*v;
            u=v;
            v=lr;
        }
    }
    copy(mr_mip->w1,z);
    MR_OUT
    return (size(mr_mip->w1));
}

