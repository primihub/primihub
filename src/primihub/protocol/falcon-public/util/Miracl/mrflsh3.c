
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
 *   MIRACL flash trig.
 *   mrflsh3.c
 */

#include <math.h>
#include "miracl.h"

#ifdef MR_FLASH

#define TAN 1
#define SIN 2
#define COS 3

static int norm(_MIPD_ int type,flash y)
{ /* convert y to first quadrant angle *
   * and return sign of result         */
    int s;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return 0;
    s=PLUS;
    if (size(y)<0)
    {
        negify(y,y);
        if (type!=COS) s=(-s);
    }
    fpi(_MIPP_ mr_mip->pi);
    fpmul(_MIPP_ mr_mip->pi,1,2,mr_mip->w8);
    if (fcomp(_MIPP_ y,mr_mip->w8)<=0) return s;
    fpmul(_MIPP_ mr_mip->pi,2,1,mr_mip->w8);
    if (fcomp(_MIPP_ y,mr_mip->w8)>0)
    { /* reduce mod 2.pi */
        fdiv(_MIPP_ y,mr_mip->w8,mr_mip->w9);
        ftrunc(_MIPP_ mr_mip->w9,mr_mip->w9,mr_mip->w9);
        fmul(_MIPP_ mr_mip->w9,mr_mip->w8,mr_mip->w9);
        fsub(_MIPP_ y,mr_mip->w9,y);
    }
    if (fcomp(_MIPP_ y,mr_mip->pi)>0)
    { /* if greater than pi */
        fsub(_MIPP_ y,mr_mip->pi,y);
        if (type!=TAN) s=(-s);
    }
    fpmul(_MIPP_ mr_mip->pi,1,2,mr_mip->w8);
    if (fcomp(_MIPP_ y,mr_mip->w8)>0)
    { /* if greater than pi/2 */
        fsub(_MIPP_ mr_mip->pi,y,y);
        if (type!=SIN) s=(-s);
    }
    return s;
}

static int tan1(_MIPD_ big w,int n)
{  /* generator for C.F. of tan(1) */ 
    if (n==0) return 1;
    if (n%2==1) return 2*(n/2)+1;
    else return 1;
}

void ftan(_MIPD_ flash x,flash y)
{ /* calculates y=tan(x) */
    int i,n,nsq,m,sqrn,sgn,op[5];
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    copy(x,y);
    if (mr_mip->ERNUM || size(y)==0) return;

    MR_IN(57)

    sgn=norm(_MIPP_ TAN,y);
    ftrunc(_MIPP_ y,y,mr_mip->w10);
    n=size(y);
    if (n!=0) build(_MIPP_ y,tan1);
    if (size(mr_mip->w10)==0)
    {
        insign(sgn,y);
        MR_OUT
        return;
    }
    sqrn=isqrt(mr_mip->lg2b*mr_mip->workprec,mr_mip->lg2b);
    nsq=0;
    copy(mr_mip->w10,mr_mip->w8);
    frecip(_MIPP_ mr_mip->w10,mr_mip->w10);
    ftrunc(_MIPP_ mr_mip->w10,mr_mip->w10,mr_mip->w10);
    m=logb2(_MIPP_ mr_mip->w10);
    if (m<sqrn)
    { /* scale fraction down */
        nsq=sqrn-m;
        expb2(_MIPP_ nsq,mr_mip->w10);
        fdiv(_MIPP_ mr_mip->w8,mr_mip->w10,mr_mip->w8);
    }
    zero(mr_mip->w10);
    fmul(_MIPP_ mr_mip->w8,mr_mip->w8,mr_mip->w9);
    negify(mr_mip->w9,mr_mip->w9);
    op[0]=0x4B;    /* set up for x/(y+C) */
    op[1]=op[3]=1;
    op[2]=0;
    for (m=sqrn;m>1;m--)
    { /* Unwind C.F for tan(x)=z/(1-z^2/(3-z^2/(5-...)))))) */
        op[4]=2*m-1;
        flop(_MIPP_ mr_mip->w9,mr_mip->w10,op,mr_mip->w10);
    }
    op[4]=1;
    flop(_MIPP_ mr_mip->w8,mr_mip->w10,op,mr_mip->w10);
    op[0]=0x6C;     /* set up for tan(x+y)=tan(x)+tan(y)/(1-tan(x).tan(y)) */
    op[1]=op[2]=op[3]=1;
    op[4]=(-1);
    for (i=0;i<nsq;i++)
    { /* now square it back up again */
        flop(_MIPP_ mr_mip->w10,mr_mip->w10,op,mr_mip->w10);
    }
    flop(_MIPP_ y,mr_mip->w10,op,y);
    insign(sgn,y);
    MR_OUT
}

void fatan(_MIPD_ flash x,flash y)
{ /* calculate y=atan(x) */
    int s,c,op[5];
    BOOL inv,hack;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    copy(x,y);
    if (mr_mip->ERNUM || size(y)==0) return;

    MR_IN(58)

    s=exsign(y);
    insign(PLUS,y);
    inv=FALSE;
    fpi(_MIPP_ mr_mip->pi);
    fconv(_MIPP_ 1,1,mr_mip->w11);
    c=fcomp(_MIPP_ y,mr_mip->w11);
    if (c==0)
    { /* atan(1)=pi/4 */
        fpmul(_MIPP_ mr_mip->pi,1,4,y);
        insign(s,y);
        MR_OUT
        return;
    }
    if (c>0)
    { /* atan(x)=pi/2 - atan(1/x) for x>1 */
        inv=TRUE;
        frecip(_MIPP_ y,y);
    }
    hack=FALSE;
    if (mr_lent(y)<=2)
    { /* for 'simple' values of y */
        hack=TRUE;
        fconv(_MIPP_ 3,1,mr_mip->w11);
        froot(_MIPP_ mr_mip->w11,2,mr_mip->w11);
        op[0]=0xC6;
        op[2]=op[3]=op[4]=1;
        op[1]=(-1);
        flop(_MIPP_ y,mr_mip->w11,op,y);
    }
    op[0]=0x4B;
    op[1]=op[3]=op[4]=1;
    op[2]=0;
    mr_mip->workprec=mr_mip->stprec;
    dconv(_MIPP_ atan(fdsize(_MIPP_ y)),mr_mip->w11);
    while (mr_mip->workprec!=mr_mip->nib)
    { /* Newtons iteration w11=w11+(y-tan(w11))/(tan(w11)^2+1) */
        if (mr_mip->workprec<mr_mip->nib) mr_mip->workprec*=2;
        if (mr_mip->workprec>=mr_mip->nib) mr_mip->workprec=mr_mip->nib;
        else if (mr_mip->workprec*2>mr_mip->nib) mr_mip->workprec=(mr_mip->nib+1)/2;
        ftan(_MIPP_ mr_mip->w11,mr_mip->w12);
        fsub(_MIPP_ y,mr_mip->w12,mr_mip->w8);
        fmul(_MIPP_ mr_mip->w12,mr_mip->w12,mr_mip->w12);
        flop(_MIPP_ mr_mip->w8,mr_mip->w12,op,mr_mip->w12);  /* w12=w8/(w12+1) */
        fadd(_MIPP_ mr_mip->w12,mr_mip->w11,mr_mip->w11);
    }
    copy(mr_mip->w11,y);
    op[0]=0x6C;
    op[1]=op[3]=6;
    op[2]=1;
    op[4]=0;
    if (hack) flop(_MIPP_ y,mr_mip->pi,op,y);
    op[1]=(-2);
    op[3]=2;
    if (inv) flop(_MIPP_ y,mr_mip->pi,op,y);
    insign(s,y);
    MR_OUT
}

void fsin(_MIPD_ flash x,flash y)
{ /*  calculate y=sin(x) */
    int sgn,op[5];
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    copy(x,y);
    if (mr_mip->ERNUM || size(y)==0) return;

    MR_IN(59)
    sgn=norm(_MIPP_ SIN,y);
    fpmul(_MIPP_ y,1,2,y);
    ftan(_MIPP_ y,y);
    op[0]=0x6C;
    op[1]=op[2]=op[3]=op[4]=1;
    flop(_MIPP_ y,y,op,y);
    insign(sgn,y);
    MR_OUT
}

void fasin(_MIPD_ flash x,flash y)
{ /* calculate y=asin(x) */
    int s,op[5];
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    copy(x,y);
    if (mr_mip->ERNUM || size(y)==0) return;
  
    MR_IN(60)
    s=exsign(y);
    insign(PLUS,y);
    op[0]=0x3C;
    op[1]=(-1);
    op[2]=op[3]=1;
    op[4]=0;
    flop(_MIPP_ y,y,op,mr_mip->w11);  /* mr_w11 = -(y.y-1) */
    froot(_MIPP_ mr_mip->w11,2,mr_mip->w11);
    if (size(mr_mip->w11)==0)
    {
        fpi(_MIPP_ mr_mip->pi);
        fpmul(_MIPP_ mr_mip->pi,1,2,y);
    }
    else
    {
        fdiv(_MIPP_ y,mr_mip->w11,y);
        fatan(_MIPP_ y,y);
    }
    insign(s,y);    
    MR_OUT
}

void fcos(_MIPD_ flash x,flash y)
{ /* calculate y=cos(x) */
    int sgn,op[5];
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    copy(x,y);
    if (mr_mip->ERNUM || size(y)==0)
    {
        convert(_MIPP_ 1,y);
        return;
    }

    MR_IN(61)
    sgn=norm(_MIPP_ COS,y);
    fpmul(_MIPP_ y,1,2,y);
    ftan(_MIPP_ y,y);
    op[0]=0x33;
    op[1]=op[3]=op[4]=1;
    op[2]=(-1);
    flop(_MIPP_ y,y,op,y);
    insign(sgn,y);
    MR_OUT
}

void facos(_MIPD_ flash x,flash y)
{ /* calculate y=acos(x) */
    int op[5];
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;

    MR_IN(62)

    fasin(_MIPP_ x,y);
    fpi(_MIPP_ mr_mip->pi);
    op[0]=0x6C;
    op[1]=(-2);
    op[2]=1;
    op[3]=2;
    op[4]=0;
    flop(_MIPP_ y,mr_mip->pi,op,y);    /* y = pi/2 - y */
    MR_OUT
}

#endif

