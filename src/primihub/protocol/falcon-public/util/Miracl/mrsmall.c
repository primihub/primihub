
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
 *   MIRACL small number theoretic routines 
 *   mrsmall.c
 */

#include "miracl.h"

#ifdef MR_FP
#include <math.h>
#endif

#ifdef MR_WIN64
#include <intrin.h>
#endif


mr_small smul(mr_small x,mr_small y,mr_small n)
{ /* returns x*y mod n */
    mr_small r;

#ifdef MR_ITANIUM
    mr_small tm;
#endif
#ifdef MR_WIN64
    mr_small tm;
#endif
#ifdef MR_FP
    mr_small dres;
#endif
#ifndef MR_NOFULLWIDTH
    if (n==0)
    { /* Assume n=2^MIRACL */
        muldvd(x,y,(mr_small)0,&r);
        return r;
    }
#endif
    x=MR_REMAIN(x,n);
    y=MR_REMAIN(y,n);
    muldiv(x,y,(mr_small)0,n,&r);
    return r;
}

mr_small invers(mr_small x,mr_small y)
{ /* returns inverse of x mod y */
    mr_small r,s,q,t,p;
#ifdef MR_FP
    mr_small dres;
#endif

    BOOL pos;
    if (y!=0) x=MR_REMAIN(x,y);
    r=1;
    s=0;
    p=y;
    pos=TRUE;
#ifndef MR_NOFULLWIDTH
    if (p==0)
    { /* if modulus is 0, assume its actually 2^MIRACL */
        if (x==1) return (mr_small)1;
        t=r; r=s; s=t;
        p=x;  
        q=muldvm((mr_small)1,(mr_small)0,p,&t);
        t=r+s*q;  r=s; s=t;
        t=0-p*q;  x=p; p=t; 
    }
#endif
    while (p!=0)
    { /* main euclidean loop */
        q=MR_DIV(x,p);
        t=r+s*q;  r=s; s=t;
        t=x-p*q;  x=p; p=t;
        pos=!pos;
    }
    if (!pos) r=y-r;
    return r;
}

int jac(mr_small x,mr_small n)
{ /* finds (x/n) as (-1)^m */ 
    int m,k,n8,u4;
    mr_small t;
#ifdef MR_FP
    mr_small dres;
#endif
    if (x==0)
    {
        if (n==1) return 1;
        else return 0;
    } 
    if (MR_REMAIN(n,2)==0) return 0;
    x=MR_REMAIN(x,n);   
    m=0;
    while(n>1)
    { /* main loop */
        if (x==0) return 0;

/* extract powers of 2 */
        for (k=0;MR_REMAIN(x,2)==0;k++) x=MR_DIV(x,2);
        n8=(int)MR_REMAIN(n,8);
        if (k%2==1) m+=(n8*n8-1)/8;

/* quadratic reciprocity */
        u4=(int)MR_REMAIN(x,4);
        m+=(n8-1)*(u4-1)/4;
        t=n;  t=MR_REMAIN(t,x);
        n=x;  x=t;
        m%=2;
    }
    if (m==0) return 1;
    else      return (-1);
} 

#ifndef MR_STATIC

mr_small spmd(mr_small x,mr_small n,mr_small m) 
{ /*  returns x^n mod m  */
    mr_small r,sx;
#ifdef MR_FP
    mr_small dres;
#endif
    x=MR_REMAIN(x,m);
    r=0;
    if (x==0) return r;
    r=1;
    if (n==0) return r;
    sx=x;
    forever
    {
        if (MR_REMAIN(n,2)!=0) muldiv(r,sx,(mr_small)0,m,&r);
        n=MR_DIV(n,2);
        if (n==0) return r;
        muldiv(sx,sx,(mr_small)0,m,&sx);
    }
}

mr_small sqrmp(mr_small x,mr_small m)
{ /* square root mod a small prime by Shanks method  *
   * returns 0 if root does not exist or m not prime */
    mr_small z,y,v,w,t,q;
#ifdef MR_FP
    mr_small dres;
#endif
    int i,e,n,r;
    BOOL pp;
    x=MR_REMAIN(x,m);
    if (x==0) return 0;
    if (x==1) return 1;
    if (spmd(x,(mr_small)((m-1)/2),m)!=1) return 0;    /* Legendre symbol not 1   */
    if (MR_REMAIN(m,4)==3) return spmd(x,(mr_small)((m+1)/4),m);  /* easy case for m=4.k+3   */
    if (MR_REMAIN(m,8)==5)
    { /* also relatively easy */
        t=spmd(x,(mr_small)((m-1)/4),m);
        if (t==1) return spmd(x,(mr_small)((m+3)/8),m);
        if (t==(mr_small)(m-1))
        {
            muldiv((mr_small)4,x,(mr_small)0,m,&t);
            t=spmd(t,(mr_small)((m+3)/8),m);
            muldiv(t,(mr_small)((m+1)/2),(mr_small)0,m,&t);
            return t;
        }
        return 0;
    }
    q=m-1;
    e=0;
    while (MR_REMAIN(q,2)==0) 
    {
        q=MR_DIV(q,2);
        e++;
    }
    if (e==0) return 0;      /* even m */
    for (r=2;;r++)
    { /* find suitable z */
        z=spmd((mr_small)r,q,m);
        if (z==1) continue;
        t=z;
        pp=FALSE;
        for (i=1;i<e;i++)
        { /* check for composite m */
            if (t==(m-1)) pp=TRUE;
            muldiv(t,t,(mr_small)0,m,&t);
            if (t==1 && !pp) return 0;
        }
        if (t==(m-1)) break;
        if (!pp) return 0;   /* m is not prime */
    }
    y=z;
    r=e;
    v=spmd(x,(mr_small)((q+1)/2),m);
    w=spmd(x,q,m);
    while (w!=1)
    {
        t=w;
        for (n=0;t!=1;n++) muldiv(t,t,(mr_small)0,m,&t);
        if (n>=r) return 0;
        y=spmd(y,mr_shiftbits(1,r-n-1),m);
        muldiv(v,y,(mr_small)0,m,&v);
        muldiv(y,y,(mr_small)0,m,&y);
        muldiv(w,y,(mr_small)0,m,&w);
        r=n;
    }
    return v;
}

#endif
