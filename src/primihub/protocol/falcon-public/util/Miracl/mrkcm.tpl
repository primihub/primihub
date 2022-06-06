
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
 *   MIRACL Karatsuba method for multiprecision multiplication combined with 
 *   Comba's method for high speed assembly language multiplication
 *   and Montgomery's method for modular muliplication - the KCM Method - 
 *   together yield near optimal speed for exponentiation cryptosystems.
 *
 *   mrkcm.tpl 
 *
 *   This approach is recommended for maximum speed where parameters 
 *   are fixed and compute resources are constrained. The processor must 
 *   support an unsigned multiply instruction, and should have a carry flag.
 *
 *   This file is a template. To fill in the gaps and create mrkcm.c
 *   you must run the mex.c program to insert the C or assembly language
 *   macros from the appropriate .mcs file. For use with C, MR_NOASM must
 *   be defined in mirdef.h
 *
 *   This method would appear to be particularly useful for implementing fast
 *   RSA/DSA/DH Cryptosystems.
 *
 *   The #define MR_KCM in mirdef.h affects the size of modulus that can
 *   be used. This *must* be determined at compile time. Then any modulus
 *   of size in words = MR_KCM*2^n can be used. For example if MR_KCM=8 
 *   a modulus of size 8,16,32,64 etc can be used. So if MR_KCM = 8 on a
 *   32 bit computer, then the modulus may be 256, 512, 1024, 2048 etc.
 *   bits in length 
 *
 *   Note that this module can generate a *lot* of code for larger values of 
 *   MR_KCM. This should have a maximum value of 8-16.
 *
 *   Note that on some processors it is *VITAL* that arrays be aligned on 
 *   4-byte boundaries
 *
 *   Inspiration from Wei Dai is acknowledged
 *
 *  **** This code does not like -fomit-frame-pointer using GCC  ***********
 *
 */

#include "miracl.h"

#ifdef MR_KCM
  
#if INLINE_ASM == 1    
#define N 2
#define POINTER WORD PTR  
#define PBP bp   
#define PBX bx 
#define PCX cx  
#define PSI si   
#define PDI di   
#define DSI si   
#define DDI di   
#define DBP bp   
#define DAX ax   
#define DBX bx
#define DCX cx   
#define DDX dx   
#endif   
 
#if INLINE_ASM == 2    
#define N 4
#define POINTER DWORD PTR   
#define PBP bp   
#define PBX bx   
#define PSI si   
#define PDI di   
#define PCX cx
#define DSI esi  
#define DDI edi  
#define DBP ebp  
#define DAX eax
#define DBX ebx  
#define DCX ecx  
#define DDX edx  
#endif           
  
#if INLINE_ASM == 3    
#define N 4
#define POINTER DWORD PTR   
#define PBP ebp   
#define PBX ebx
#define PCX ecx   
#define PSI esi   
#define PDI edi   
#define DSI esi  
#define DDI edi  
#define DBP ebp  
#define DAX eax
#define DBX ebx  
#define DCX ecx  
#define DDX edx  
#endif           

static void mr_comba_mul(mr_small *x,mr_small *y,mr_small *z)
{ /* multiply two arrays of length MR_KCM */ 
    mr_small *a,*b,*c;
#ifdef MR_WIN64
    mr_small lo,hi,sumlo,sumhi,extra;
#endif
#ifdef MR_ITANIUM
    register mr_small lo1,hi1,lo2,hi2,ma,mb,sumlo,sumhi,extra;
#endif
#ifdef MR_NOASM
 #ifdef mr_qltype
    mr_large pp1;
    mr_vlarge sum;
 #else
    register mr_small extra,s0,s1;
    mr_large pp1,pp2,sum;
 #endif
#endif
    a=x; b=y; c=z;
   
/*** MULTIPLY ***/

}   

static void mr_comba_halfm(mr_small *x,mr_small *y,mr_small *z)
{ /* multiply two arrays, but only return lower half */
    mr_small *a,*b,*c;
#ifdef MR_ITANIUM
    register mr_small lo1,hi1,lo2,hi2,ma,mb,sumlo,sumhi,extra;
#endif
#ifdef MR_WIN64
    mr_small lo,hi,sumlo,sumhi,extra;
#endif
#ifdef MR_NOASM
 #ifdef mr_qltype
    mr_large pp1;
    mr_vlarge sum;
 #else
    register mr_small extra,s0,s1;
    mr_large pp1,pp2,sum;
 #endif
#endif
    a=x; b=y;  c=z;
   
/*** MULTUP ***/

}

static void mr_comba_sqr(mr_small *x,mr_small *z)
{ /* square an array of length MR_KCM */
    mr_small *a,*c;
#ifdef MR_ITANIUM
    register mr_small lo1,hi1,lo2,hi2,ma,mb,sumlo,sumhi,extra;
#endif
#ifdef MR_WIN64
    mr_small lo,hi,sumlo,sumhi,extra,cy;
#endif
#ifdef MR_NOASM
 #ifdef mr_qltype
    mr_large pp1;
    mr_vlarge sum;
 #else
    register mr_small extra,s0,s1;
    mr_large pp1,pp2,sum;
 #endif
#endif
    a=x; c=z;
   
/*** SQUARE ***/

}

static int mr_addn(mr_small *x,mr_small *y,mr_small *z,int n)
{ /* add two arrays of length n*MR_KCM */
  /* first some macros */
    mr_small *a,*b,*c;
    mr_small carry;
#ifdef MR_ITANIUM
    register mr_small ma,u;
#endif
#ifdef MR_WIN64
    mr_small ma,u;
#endif
#ifdef MR_NOASM
    mr_large u;
#endif
    a=x; b=y; c=z;

/*** SUMMATION ***/

    return (int)carry;
}

static int mr_incn(mr_small *y,mr_small *z,int n)
{ /* add to an array of length n*MR_KCM */
    mr_small *a,*b;
    mr_small carry;
#ifdef MR_WIN64
    mr_small ma,u;
#endif
#ifdef MR_ITANIUM
    register mr_small ma,u;
#endif
#ifdef MR_NOASM
    mr_large u;
#endif

    a=z; b=y;

/*** INCREMENTATION */

    return (int)carry;
}

static int mr_decn(mr_small *y,mr_small *z,int n)
{ /* subtract from an array of length n*MR_KCM */
    mr_small *a,*b;
    mr_small carry;
#ifdef MR_WIN64
    mr_small ma,u;
#endif
#ifdef MR_ITANIUM
    register mr_small ma,u;
#endif
#ifdef MR_NOASM
    mr_large u;
#endif

    a=z; b=y;

/*** DECREMENTATION */

    return (int)carry;
}

static void mr_cpy(mr_small *x,mr_small *z,int n)
{ /* copy an array of length n*MR_KCM */
    int m;
    for (m=0;m<n*MR_KCM;m++) z[m]=x[m];
}

static void mr_karmul(int n,mr_small *t,mr_small *x,mr_small *y,mr_small *z)
{ /* full multiplication z=x*y */
    int i,nd2,m,md2;
    mr_small c1,c2,c=0;
    if (n==MR_KCM)
    { /* reached the bottom... */
        mr_comba_mul(x,y,z);
        return;
    }
    nd2=n/2;
    m=n/MR_KCM;
    md2=m/2;
 
    c1=mr_addn(x,&x[nd2],z,md2);
    c2=mr_addn(y,&y[nd2],&z[nd2],md2);
    mr_karmul(nd2,&t[n],z,&z[nd2],t);
    if (c1) c+=mr_incn(&z[nd2],&t[nd2],md2);
    if (c2) c+=mr_incn(z,&t[nd2],md2);
    if (c1&c2) c++;    /* form (a0+a1).(b0+b1), carry in c */
    mr_karmul(nd2,&t[n],x,y,z);
    c-=mr_decn(z,t,m);           /* subtract a0.b0 */
    mr_karmul(nd2,&t[n],&x[nd2],&y[nd2],&z[n]);
    c-=mr_decn(&z[n],t,m);       /* subtract a1.b1 */
    c+=mr_incn(t,&z[nd2],m);    /* combine results */

    i=n+nd2;
    z[i]+=c;
    if (z[i]<c)
    { /* propagate carries - very rare */
        do 
        {
            i++;
            z[i]++;
        } while (z[i]==0);
    }
}

static void mr_karsqr(int n,mr_small *t,mr_small *x,mr_small *z)
{ /* Squaring z=x*x */
    int i,nd2,m;
    mr_small c;
    if (n==MR_KCM)
    {
        mr_comba_sqr(x,z);
        return;
    }
    nd2=n/2;
    m=n/MR_KCM;
  
    mr_karsqr(nd2,&t[n],x,z);
    mr_karsqr(nd2,&t[n],&x[nd2],&z[n]);
    mr_karmul(nd2,&t[n],x,&x[nd2],t);

    c=mr_incn(t,&z[nd2],m);
    c+=mr_incn(t,&z[nd2],m);

    i=n+nd2;
    z[i]+=c;
    if (z[i]<c)
    { /* propagate carries - very rare */
        do 
        {
            i++;
            z[i]++;
        } while (z[i]==0);
    }
}

static void mr_karmul_lower(int n,mr_small *t,mr_small *x,mr_small *y,mr_small *z)
{ /* Calculates Least Significant bottom half of x*y */
    int nd2,m,md2;
    if (n==MR_KCM)
    { /* only calculate bottom half of product */
        mr_comba_halfm(x,y,z);
        return;
    }
    nd2=n/2;
    m=n/MR_KCM;
    md2=m/2;

    mr_karmul(nd2,&t[n],x,y,z);
    mr_karmul_lower(nd2,&t[n],&x[nd2],y,t); 
    mr_incn(t,&z[nd2],md2);
    mr_karmul_lower(nd2,&t[n],x,&y[nd2],t); 
    mr_incn(t,&z[nd2],md2);
}

static void mr_karmul_upper(int n,mr_small *t,mr_small *x,mr_small *y,mr_small *z)
{ /* Calculates Most Significant upper half of x*y, given lower part */
    int i,nd2,m,md2;
    mr_small c1,c2,c=0;
    if (n==MR_KCM)
    {
        mr_comba_mul(x,y,z);
        return;
    }
    nd2=n/2;
    m=n/MR_KCM;
    md2=m/2;
    c1=mr_addn(x,&x[nd2],&z[n],md2);
    c2=mr_addn(y,&y[nd2],&z[n+nd2],md2);
    mr_karmul(nd2,&t[n],&z[n],&z[n+nd2],t);
    if (c1) c+=mr_incn(&z[n+nd2],&t[nd2],md2);
    if (c2) c+=mr_incn(&z[n],&t[nd2],md2);
    if (c1&c2) c++;        /* form (a0+a1)(b0+b1), carry in c */

    mr_karmul(nd2,&t[n],&x[nd2],&y[nd2],&z[n]);
    c-=mr_decn(&z[n],t,m);  /* subtract a1.b1 */
                           /* recreate a0.b0 in z */
    mr_incn(z,&z[nd2],md2);
    mr_decn(t,&z[nd2],md2);
    c-=mr_decn(z,t,m);       /* subtract a0.b0 */
    c+=mr_incn(t,&z[nd2],m);

    i=n+nd2;
    z[i]+=c;
    if (z[i]<c)
    { /* propagate carries - very rare */
        do 
        {
            i++;
            z[i]++;
        } while (z[i]==0);
    }
}

void kcm_mul(_MIPD_ big x,big y,big z)
{ /* fast karatsuba multiplication */ 
    unsigned int i;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    int ml=(int)mr_mip->modulus->len;
    zero(mr_mip->w0);
    for (i=2*ml;i<(mr_mip->wt->len&MR_OBITS);i++) mr_mip->wt->w[i]=0;

    mr_karmul(ml,mr_mip->wt->w,x->w,y->w,mr_mip->w0->w);
    mr_mip->w0->len=mr_mip->wt->len=2*ml;
    copy(mr_mip->w0,z);
}

void kcm_multiply(_MIPD_ int n,big x,big y,big z)
{ /* n *must* be MR_KCM*2^m for m>=0 */   
    unsigned int i;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    zero(mr_mip->w0);
    for (i=2*n;i<(mr_mip->wt->len&MR_OBITS);i++) mr_mip->wt->w[i]=0;
    mr_karmul(n,mr_mip->wt->w,x->w,y->w,mr_mip->w0->w);
    mr_mip->w0->len=mr_mip->wt->len=2*n;
    copy(mr_mip->w0,z);
}

void kcm_square(_MIPD_ int n,big x,big z)
{ /* n *must* be MR_KCM*2^m for m>=0 */
    unsigned int i;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    zero(mr_mip->w0);
    for (i=2*n;i<(mr_mip->wt->len&MR_OBITS);i++) mr_mip->wt->w[i]=0;
    mr_karsqr(n,mr_mip->wt->w,x->w,mr_mip->w0->w);    
    mr_mip->w0->len=mr_mip->wt->len=2*n;
    copy(mr_mip->w0,z);
}

BOOL kcm_top(_MIPD_ int n,big x,big y,big z)
{ /* to support floating-point - see float.cpp and fmt function in big.cpp */
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    unsigned int i;
    int len;
    zero(mr_mip->w0);

    if (n<MR_KCM)
    {
        mr_mip->check=OFF;
        multiply(_MIPP_ x,y,mr_mip->w0);
        mr_mip->check=ON;
    }
    else
    {
        for (i=2*n;i<(mr_mip->wt->len&MR_OBITS);i++) mr_mip->wt->w[i]=0;
        if (x==y)  mr_karsqr(n,mr_mip->wt->w,x->w,mr_mip->w0->w);
        else       mr_karmul(n,mr_mip->wt->w,x->w,y->w,mr_mip->w0->w);
        mr_mip->w0->len=mr_mip->wt->len=2*n;
        mr_lzero(mr_mip->w0);
    }
    len=mr_lent(mr_mip->w0);
    mr_shift(_MIPP_ mr_mip->w0,n-len,mr_mip->w0);
    copy(mr_mip->w0,z);
    if (len<2*n) return TRUE;
    return FALSE;
}

void kcm_sqr(_MIPD_ big x,big z)
{ /* fast karatsuba squaring */
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    unsigned int i;
    int ml=(int)mr_mip->modulus->len;
    zero(mr_mip->w0);
    for (i=2*ml;i<(mr_mip->wt->len&MR_OBITS);i++) mr_mip->wt->w[i]=0;
    mr_karsqr(ml,mr_mip->wt->w,x->w,mr_mip->w0->w);
    mr_mip->w0->len=mr_mip->wt->len=2*ml;
    copy(mr_mip->w0,z);
}

void kcm_redc(_MIPD_ big z,big w)
{ /* fast karatsuba Montgomery reduction */
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    int m,ml=(int)mr_mip->modulus->len;
    unsigned int i;
    m=ml/MR_KCM;
    copy(z,mr_mip->w0);

    for (i=2*ml;i<(mr_mip->wt->len&MR_OBITS);i++) mr_mip->wt->w[i]=0;
    mr_cpy(&(mr_mip->w0->w[ml]),w->w,m);

    mr_karmul_lower(ml,mr_mip->wt->w,mr_mip->w0->w,mr_mip->big_ndash->w,mr_mip->ws->w);

    for (i=ml;i<(w->len&MR_OBITS);i++) w->w[i]=0;

    mr_mip->ws->len=w->len=ml;

    mr_karmul_upper(ml,mr_mip->wt->w,mr_mip->ws->w,mr_mip->modulus->w,mr_mip->w0->w);
    mr_mip->w0->len=mr_mip->wt->len=2*ml;

    if (mr_decn(&(mr_mip->w0->w[ml]),w->w,m))
        mr_incn(mr_mip->modulus->w,w->w,m);

    mr_lzero(w);

}

#endif

