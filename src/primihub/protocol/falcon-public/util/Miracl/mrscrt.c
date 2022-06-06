
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
 *   MIRACL Chinese Remainder Thereom routines (for use with small moduli) 
 *   mrscrt.c
 */


#include <stdlib.h>
#include "miracl.h"

#ifdef MR_FP
#include <math.h>
#endif

static mr_utype in_range(mr_utype x,mr_utype y)
{ /* x=x%y, and positive */
    mr_utype r;
#ifdef MR_FP
    mr_small dres;
#endif
    r=MR_REMAIN(x,y);
    if (r<0) r+=y;
    return r;
}

#ifndef MR_STATIC

BOOL scrt_init(_MIPD_ small_chinese *c,int r,mr_utype *moduli)
{ /* calculate CRT constants - returns FALSE if there is a problem */
    int i,j,k;
    if (r<1) return FALSE;
    if (r==1)
    {
        c->NP=1;
        c->M=(mr_utype *)mr_alloc(_MIPP_ r,sizeof(mr_utype));
        if (c->M==NULL) return FALSE;
        c->M[0]=moduli[0];
        return TRUE;
    }
    for (i=0;i<r;i++) if (moduli[i]<2) return FALSE;
    c->M=(mr_utype *)mr_alloc(_MIPP_ r,sizeof(mr_utype));
    if (c->M==NULL) return FALSE;
    c->C=(mr_utype *)mr_alloc(_MIPP_ r*(r-1)/2,sizeof(mr_utype));
    if (c->C==NULL)
    { /* no room */
        mr_free(c->M);
        return FALSE;
    }
    c->V=(mr_utype *)mr_alloc(_MIPP_ r,sizeof(mr_utype));
    if (c->V==NULL)
    { /* no room */
        mr_free(c->M);
        mr_free(c->C);
        return FALSE;
    }
    for (k=0,i=0;i<r;i++)
    {
        c->M[i]=moduli[i];
        for (j=0;j<i;j++,k++)
            c->C[k]=invers(c->M[j],c->M[i]);
    }
    c->NP=r;
    return TRUE;
}

void scrt_end(small_chinese *c)
{ /* clean up after CRT */
    if (c->NP<1) 
    {
        c->NP=0;
        return;
    }
    if (c->NP==1)
    {
        mr_free(c->M);
        c->NP=0;
        return;
    }
    mr_free(c->M);
    mr_free(c->V);
    mr_free(c->C);
    c->NP=0;
}

#endif

void scrt(_MIPD_ small_chinese *c,mr_utype *u,big x)
{ /* Chinese Remainder Thereom                  *
   * Calculate x given remainders u[i] mod M[i] */
    int i,j,k,len;
    mr_utype *V,*C,*M;
    mr_small t;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
#ifdef MR_FP_ROUNDING
    mr_large im;
#endif
    V=c->V;
    C=c->C;
    M=c->M;
    len=c->NP;
    if (len<1) return;
    if (len==1)
    {
        t=smul(1,in_range(u[0],M[0]),M[0]);
        convert(_MIPP_ 1,mr_mip->w5);
        mr_pmul(_MIPP_ mr_mip->w5,t,x);
        return;
    }
    V[0]=u[0];
    k=0;
    for (i=1;i<len;i++)
    { /* Knuth P. 274 */
        V[i]=u[i] - V[0];
#ifdef MR_FP_ROUNDING
        im=mr_invert(M[i]);
        imuldiv(V[i],C[k],(mr_small)0,M[i],im,&V[i]);
        if (V[i]<0) V[i]+=M[i];
#else
        V[i]=smul(in_range(V[i],M[i]),C[k],M[i]);
#endif
        k++;
	if (i==1) continue;
#ifndef MR_FP
#ifdef INLINE_ASM
#if INLINE_ASM == 3
#define MR_IMPASM

        ASM mov ebx,DWORD PTR V
        ASM mov esi,DWORD PTR M
        ASM mov edi,DWORD PTR C
        ASM mov ecx,1
        ASM mov edx,DWORD PTR i
        ASM mov esi,[esi+4*edx]
     s1:
        ASM cmp ecx,edx
        ASM jge s2

        ASM mov eax,[ebx+4*edx]
        ASM push edx

        ASM sub eax,[ebx+4*ecx]
        ASM cdq
        ASM idiv esi
        ASM mov eax,edx
        ASM add eax,esi

        ASM mov edx,DWORD PTR k
        ASM mul DWORD PTR [edi+4*edx]
        ASM div esi

        ASM mov eax,edx

        ASM pop edx
        ASM mov [ebx+4*edx],eax   
        ASM inc DWORD PTR k
        ASM inc ecx
        ASM jmp s1
     s2:
        ASM nop
#endif

#if INLINE_ASM == 4
#define MR_IMPASM
    ASM (
	   "movl %0,%%ecx\n"
           "movl %1,%%ebx\n"
           "movl %2,%%esi\n"
           "movl %3,%%edi\n"
           "movl %4,%%edx\n"
           "pushl %%ebp\n"
	   "movl $1,%%ebp\n"
           "movl (%%esi,%%edx,4),%%esi\n"
         "0:\n"
           "cmpl %%edx,%%ebp\n"
           "jge 1f\n"
           "movl (%%ebx,%%edx,4),%%eax\n"
           "subl (%%ebx,%%ebp,4),%%eax\n"
           "pushl %%edx\n"
           "cltd \n"
           "idivl %%esi\n"
           "movl %%edx,%%eax\n"
           "addl %%esi,%%eax\n"

           "mull (%%edi,%%ecx,4)\n"
           "divl %%esi\n"
           "movl %%edx,%%eax\n"
           "popl %%edx\n"
           "movl %%eax,(%%ebx,%%edx,4)\n"
           "incl %%ecx\n"
           "incl %%ebp\n"
           "jmp 0b\n"
         "1:\n"
           "popl %%ebp\n"
	   "movl %%ecx,%0\n"
         :"=m"(k)
         :"m"(V),"m"(M),"m"(C),"m"(i)
         :"eax","edi","esi","ebx","ecx","edx","memory"
        );
#endif
#endif
#endif
#ifndef MR_IMPASM
        for (j=1;j<i;j++,k++)
        {
            V[i]-=V[j];
#ifdef MR_FP_ROUNDING
            imuldiv(V[i],C[k],(mr_small)0,M[i],im,&V[i]);
            if (V[i]<0) V[i]+=M[i];
#else
            V[i]=smul(in_range(V[i],M[i]),C[k],M[i]);
#endif
        }
#endif
    }

    convert(_MIPP_ 1,x);
    mr_pmul(_MIPP_ x,(mr_small)V[0],x);
    convert(_MIPP_ 1,mr_mip->w5);
    for (j=1;j<len;j++)
    {
        mr_pmul(_MIPP_ mr_mip->w5,(mr_small)(M[j-1]),mr_mip->w5);
        mr_pmul(_MIPP_ mr_mip->w5,(mr_small)(V[j]),mr_mip->w0);
        mr_padd(_MIPP_ x,mr_mip->w0,x);         
    } 
}

