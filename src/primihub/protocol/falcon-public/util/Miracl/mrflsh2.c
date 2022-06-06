
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
 *   MIRACL flash exponential and logs
 *   mrflsh2.c
 */

#include <math.h>
#include "miracl.h"

#ifdef MR_FLASH

static int expon(_MIPD_ big w,int n)
{  /* generator for C.F. of e */ 
    if (n==0) return 2;
    if (n%3==2) return 2*(n/3)+2;
    else return 1;
}
 
void fexp(_MIPD_ flash x,flash y)
{ /* calculates y=exp(x) */
    int i,n,nsq,m,sqrn,op[5];
    BOOL minus,rem;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;
    if (size(x)==0)
    {
        convert(_MIPP_ 1,y);
        return;
    }
    copy(x,y);

    MR_IN(54)

    minus=FALSE;
    if (size(y)<0)
    {
        minus=TRUE;
        negify(y,y);
    }
    ftrunc(_MIPP_ y,y,mr_mip->w9);
    n=size(y);
    if (n==MR_TOOBIG)
    {
        mr_berror(_MIPP_ MR_ERR_FLASH_OVERFLOW);
        MR_OUT
        return;
    }
    if (n==0) convert(_MIPP_ 1,y);
    else
    {
        build(_MIPP_ y,expon);
        if (minus)
        { /* underflow to zero - bit of a bodge */
            rem=mr_mip->ERCON;
            mr_mip->ERCON=TRUE;
            fpower(_MIPP_ y,n,y);
            mr_mip->ERCON=rem;
            if (mr_mip->ERNUM)
            {
                mr_mip->ERNUM=0;
                zero(y);
                MR_OUT
                return;
            }
        }
        else fpower(_MIPP_ y,n,y);
    }
    if (size(mr_mip->w9)==0)
    {
        if (minus) frecip(_MIPP_ y,y);
        MR_OUT
        return;
    }
    sqrn=isqrt(mr_mip->lg2b*mr_mip->workprec,mr_mip->lg2b);
    nsq=0;
    copy(mr_mip->w9,mr_mip->w8);
    frecip(_MIPP_ mr_mip->w9,mr_mip->w9);
    ftrunc(_MIPP_ mr_mip->w9,mr_mip->w9,mr_mip->w9);
    m=logb2(_MIPP_ mr_mip->w9);
    if (m<sqrn)
    { /* scale fraction down */
        nsq=sqrn-m;
        expb2(_MIPP_ nsq,mr_mip->w9);
        fdiv(_MIPP_ mr_mip->w8,mr_mip->w9,mr_mip->w8);
    }
    zero(mr_mip->w10);
    op[0]=0x4B;  /* set up for x/(C+y) */
    op[1]=1;
    op[2]=0;
    for (m=sqrn;m>0;m--)
    { /* Unwind C.F. expansion for exp(x)-1 */
        if (m%2==0) op[4]=2,op[3]=1;
        else        op[4]=m,op[3]=(-1);
        flop(_MIPP_ mr_mip->w8,mr_mip->w10,op,mr_mip->w10);
    }
    op[0]=0x2C;  /* set up for (x+2).y */
    op[1]=op[3]=1;
    op[2]=2;
    op[4]=0;
    for (i=0;i<nsq;i++)
    { /* now square it back up again */
        flop(_MIPP_ mr_mip->w10,mr_mip->w10,op,mr_mip->w10);
    }
    op[2]=1;
    flop(_MIPP_ mr_mip->w10,y,op,y);
    if (minus) frecip(_MIPP_ y,y);
    MR_OUT
}

void flog(_MIPD_ flash x,flash y)
{ /* calculate y=log(x) to base e */
    BOOL hack;
    int op[5];
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    copy(x,y);
    if (mr_mip->ERNUM) return;
    if (size(y)==1)
    {
        zero(y);
        return;
    }

    MR_IN(55)

    if (size(y)<=0)
    {
        mr_berror(_MIPP_ MR_ERR_NEG_LOG);
        MR_OUT
        return;
    }
    hack=FALSE;
    if (mr_lent(y)<=2)
    { /* for 'simple' values of y */
        hack=TRUE;
        build(_MIPP_ mr_mip->w11,expon);
        fdiv(_MIPP_ y,mr_mip->w11,y);
    }
    op[0]=0x68;
    op[1]=op[3]=1;
    op[2]=(-1);
    op[4]=0;
    mr_mip->workprec=mr_mip->stprec;
    dconv(_MIPP_ log(fdsize(_MIPP_ y)),mr_mip->w11);
    while (mr_mip->workprec!=mr_mip->nib)
    { /* Newtons iteration w11=w11+(y-exp(w11))/exp(w11) */
        if (mr_mip->workprec<mr_mip->nib) mr_mip->workprec*=2;
        if (mr_mip->workprec>=mr_mip->nib) mr_mip->workprec=mr_mip->nib;
        else if (mr_mip->workprec*2>mr_mip->nib) mr_mip->workprec=(mr_mip->nib+1)/2;
        fexp(_MIPP_ mr_mip->w11,mr_mip->w12);
        flop(_MIPP_ y,mr_mip->w12,op,mr_mip->w12);
        fadd(_MIPP_ mr_mip->w12,mr_mip->w11,mr_mip->w11);
    }
    copy(mr_mip->w11,y);
    if (hack) fincr(_MIPP_ y,1,1,y);
    MR_OUT
}
    
void fpowf(_MIPD_ flash x,flash y,flash z)
{ /* raise flash number to flash power *
   *     z=x^y  -> z=exp(y.log(x))     */
    int n;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;

    MR_IN(56)

    n=size(y);
    if (mr_abs(n)<MR_TOOBIG)
    { /* y is small int */
        fpower(_MIPP_ x,n,z);
        MR_OUT
        return;
    }
    frecip(_MIPP_ y,mr_mip->w13);
    n=size(mr_mip->w13);
    if (mr_abs(n)<MR_TOOBIG)
    { /* 1/y is small int */
        froot(_MIPP_ x,n,z);
        MR_OUT
        return;
    }
    copy(x,z);
    flog(_MIPP_ z,z);
    fdiv(_MIPP_ z,mr_mip->w13,z);
    fexp(_MIPP_ z,z);
    MR_OUT
}

#endif

