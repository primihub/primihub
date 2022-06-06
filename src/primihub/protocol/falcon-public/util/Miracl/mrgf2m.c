
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
 *   MIRACL routines for arithmetic over GF(2^m), 
 *   mrgf2m.c
 *
 *   For algorithms used, see IEEE P1363 Standard, Appendix A
 *   unless otherwise stated.
 *
 *   The time-critical routines are the multiplication routine multiply2()
 *   and (for AFFINE co-ordinates), the modular inverse routine inverse2() 
 *   and the routines it calls.
 *
 *   READ COMMENTS CAREFULLY FOR VARIOUS OPTIMIZATION SUGGESTIONS
 *
 *   No assembly language used.
 *
 *   Use utility irp.cpp to generate optimal code for function reduce2(.) below
 *
 *   Space can be saved by removing unneeded functions and 
 *   deleting unrequired functionality. 
 *   For example in reduce2(.) remove code for those irreducible polynomials
 *   which will not be used by your code.
 */

#include <stdlib.h> 
#include "miracl.h"
#ifdef MR_STATIC
#include <string.h>
#endif

#ifdef MR_COUNT_OPS
extern int fpm2,fpi2; 
#endif

/* must use /arch:SSE2 in compilation */

#ifdef _M_IX86_FP 
#if _M_IX86_FP >= 2
#define MR_SSE2_INTRINSICS
#endif
#endif

/* must use -msse2 in compilation */

#ifdef __SSE2__
#define MR_SSE2_INTRINSICS
#endif

#ifdef MR_SSE2_INTRINSICS
  #ifdef __GNUC__
    #include <xmmintrin.h>
  #else
    #include <emmintrin.h>
  #endif

  #if MIRACL==64
    #define MR_SSE2_64
/* Can use SSE2 registers for 64-bit manipulations */
  #endif

#endif

#ifndef MR_NOFULLWIDTH
                     /* This does not make sense using floating-point! */

/* This is extremely time-critical, and expensive */

/* Some experimental MMX code for x86-32. Seems to be slower than the standard code (on a PIV anyway).. */

#ifdef MR_MMX_x86_32

#ifdef __GNUC__
    #include <xmmintrin.h>
#else
    #include <emmintrin.h>
#endif

static mr_small mr_mul2(mr_small a,mr_small b,mr_small *r)
{
    __m64 rg,tt[4];
    mr_small q;

    tt[0]=_m_from_int(0);
    tt[1]=_m_from_int(a);
    tt[2]=_m_psllqi(tt[1],1);
    tt[3]=_m_pxor(tt[1],tt[2]);

    rg=tt[b&3]; 
    rg=_m_pxor(rg,_m_psllqi(tt[(b>>2)&3],2));
    rg=_m_pxor(rg,_m_psllqi(tt[(b>>4)&3],4));
    rg=_m_pxor(rg,_m_psllqi(tt[(b>>6)&3],6));
    rg=_m_pxor(rg,_m_psllqi(tt[(b>>8)&3],8));
    rg=_m_pxor(rg,_m_psllqi(tt[(b>>10)&3],10));
    rg=_m_pxor(rg,_m_psllqi(tt[(b>>12)&3],12));
    rg=_m_pxor(rg,_m_psllqi(tt[(b>>14)&3],14));
    rg=_m_pxor(rg,_m_psllqi(tt[(b>>16)&3],16));
    rg=_m_pxor(rg,_m_psllqi(tt[(b>>18)&3],18));
    rg=_m_pxor(rg,_m_psllqi(tt[(b>>20)&3],20));
    rg=_m_pxor(rg,_m_psllqi(tt[(b>>22)&3],22));
    rg=_m_pxor(rg,_m_psllqi(tt[(b>>24)&3],24));
    rg=_m_pxor(rg,_m_psllqi(tt[(b>>26)&3],26));
    rg=_m_pxor(rg,_m_psllqi(tt[(b>>28)&3],28));
    rg=_m_pxor(rg,_m_psllqi(tt[(b>>30)],30));

    *r=_m_to_int(rg);
    q=_m_to_int(_m_psrlqi(rg,32));

    return q;
}

#else

/* This might be faster on a 16-bit processor with no variable shift instructions. 
   The line w=hi&1; hi>>=1; lo>>=1; lo|=(w<<15); is just a 1-bit right shift on 
   the hi|lo value - should be really fast in assembly language

unsigned short mr_mul2(unsigned short x,unsigned short y,unsigned short *r)
{
    unsigned short lo,hi,bit,w;
    hi=0;  
    lo=x;   
    bit=-(lo&1); 
    lo>>=1;

    hi^=(y&bit);    bit=-(lo&1); 
    w=hi&1; hi>>=1; lo>>=1; lo|=(w<<15);

    hi^=(y&bit);    bit=-(lo&1); 
    w=hi&1; hi>>=1; lo>>=1; lo|=(w<<15);

    hi^=(y&bit);    bit=-(lo&1); 
    w=hi&1; hi>>=1; lo>>=1; lo|=(w<<15);

    hi^=(y&bit);    bit=-(lo&1); 
    w=hi&1; hi>>=1; lo>>=1; lo|=(w<<15);

    hi^=(y&bit);    bit=-(lo&1); 
    w=hi&1; hi>>=1; lo>>=1; lo|=(w<<15);

    hi^=(y&bit);    bit=-(lo&1); 
    w=hi&1; hi>>=1; lo>>=1; lo|=(w<<15);

    hi^=(y&bit);    bit=-(lo&1); 
    w=hi&1; hi>>=1; lo>>=1; lo|=(w<<15);

    hi^=(y&bit);    bit=-(lo&1); 
    w=hi&1; hi>>=1; lo>>=1; lo|=(w<<15);

    hi^=(y&bit);    bit=-(lo&1); 
    w=hi&1; hi>>=1; lo>>=1; lo|=(w<<15);

    hi^=(y&bit);    bit=-(lo&1); 
    w=hi&1; hi>>=1; lo>>=1; lo|=(w<<15);

    hi^=(y&bit);    bit=-(lo&1); 
    w=hi&1; hi>>=1; lo>>=1; lo|=(w<<15);

    hi^=(y&bit);    bit=-(lo&1); 
    w=hi&1; hi>>=1; lo>>=1; lo|=(w<<15);

    hi^=(y&bit);    bit=-(lo&1); 
    w=hi&1; hi>>=1; lo>>=1; lo|=(w<<15);

    hi^=(y&bit);    bit=-(lo&1); 
    w=hi&1; hi>>=1; lo>>=1; lo|=(w<<15);

    hi^=(y&bit);    bit=-(lo&1); 
    w=hi&1; hi>>=1; lo>>=1; lo|=(w<<15);

    hi^=(y&bit);   
    w=hi&1; hi>>=1; lo>>=1; lo|=(w<<15);

    *r=lo;
    return hi;
}
      
*/


/* This might be faster on an 8-bit processor with no variable shift instructions. 
   The line w=hi&1; hi>>=1; lo>>=1; lo|=(w<<7); is just a 1-bit right shift on 
   the hi|lo value - should be really fast in assembly language

unsigned char mr_mul2(unsigned char x,unsigned char y,unsigned char *r)
{
    unsigned char lo,hi,bit,w;
    hi=0;  
    lo=x;   
    bit=-(lo&1); 
    lo>>=1;

    hi^=(y&bit);    bit=-(lo&1); 
    w=hi&1; hi>>=1; lo>>=1; lo|=(w<<7);

    hi^=(y&bit);    bit=-(lo&1); 
    w=hi&1; hi>>=1; lo>>=1; lo|=(w<<7);

    hi^=(y&bit);    bit=-(lo&1); 
    w=hi&1; hi>>=1; lo>>=1; lo|=(w<<7);

    hi^=(y&bit);    bit=-(lo&1); 
    w=hi&1; hi>>=1; lo>>=1; lo|=(w<<7);

    hi^=(y&bit);    bit=-(lo&1); 
    w=hi&1; hi>>=1; lo>>=1; lo|=(w<<7);

    hi^=(y&bit);    bit=-(lo&1); 
    w=hi&1; hi>>=1; lo>>=1; lo|=(w<<7);

    hi^=(y&bit);    bit=-(lo&1); 
    w=hi&1; hi>>=1; lo>>=1; lo|=(w<<7);

    hi^=(y&bit);   
    w=hi&1; hi>>=1; lo>>=1; lo|=(w<<7);

    *r=lo;
    return hi;
}
      
*/

/* wouldn't it be nice if instruction sets supported a 
   one cycle "carry-free" multiplication instruction ... 
   The SmartMips does - its called maddp                 */

#ifndef MR_COMBA2

#if MIRACL==8

/* maybe use a small precomputed look-up table? */

static mr_small mr_mul2(mr_small a,mr_small b,mr_small *r)
{
    static const mr_small look[256]=
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
     0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,
     0,3,6,5,12,15,10,9,24,27,30,29,20,23,18,17,
     0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60,
     0,5,10,15,20,17,30,27,40,45,34,39,60,57,54,51,
     0,6,12,10,24,30,20,18,48,54,60,58,40,46,36,34,
     0,7,14,9,28,27,18,21,56,63,54,49,36,35,42,45,
     0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120,
     0,9,18,27,36,45,54,63,72,65,90,83,108,101,126,119,
     0,10,20,30,40,34,60,54,80,90,68,78,120,114,108,102,
     0,11,22,29,44,39,58,49,88,83,78,69,116,127,98,105,
     0,12,24,20,48,60,40,36,96,108,120,116,80,92,72,68,
     0,13,26,23,52,57,46,35,104,101,114,127,92,81,70,75,
     0,14,28,18,56,54,36,42,112,126,108,98,72,70,84,90,
     0,15,30,17,60,51,34,45,120,119,102,105,68,75,90,85
    };

    mr_small x1,y0,m,p,q;
    x1=a&0xf0;
    y0=b&0x0f;
    a<<=4;
    b>>=4;

    p=look[(a|y0)];
	q=look[(x1|b)];

	m=look[a^b^x1^y0]^p^q;  /* Karatsuba! */
 
    p^=(m<<4);
    q^=(m>>4);

    *r=p;
    return q; 
}

#else

#ifdef MR_SSE2_64

static mr_small mr_mul2(mr_small a,mr_small b,mr_small *r)
{
    int i,j;
	__m128i pp,tt[16],m;

    m=_mm_set_epi32(0,0,0xf0<<24,0);  

    tt[0]=_mm_setzero_si128();
    tt[1]=_mm_loadl_epi64((__m128i *)&a);
    tt[2]=_mm_xor_si128(_mm_slli_epi64(tt[1],1),_mm_srli_epi64( _mm_slli_si128(_mm_and_si128(m ,tt[1]),1) ,7));
    tt[4]=_mm_xor_si128(_mm_slli_epi64(tt[1],2),_mm_srli_epi64( _mm_slli_si128(_mm_and_si128(m ,tt[1]),1) ,6));
    tt[8]=_mm_xor_si128(_mm_slli_epi64(tt[1],3),_mm_srli_epi64( _mm_slli_si128(_mm_and_si128(m ,tt[1]),1) ,5));
    tt[3]=_mm_xor_si128(tt[1],tt[2]);
    tt[5]=_mm_xor_si128(tt[1],tt[4]);
    tt[6]=_mm_xor_si128(tt[2],tt[4]);
    tt[7]=_mm_xor_si128(tt[6],tt[1]);
    tt[9]=_mm_xor_si128(tt[8],tt[1]);
    tt[10]=_mm_xor_si128(tt[8],tt[2]);
    tt[11]=_mm_xor_si128(tt[10],tt[1]);
    tt[12]=_mm_xor_si128(tt[8],tt[4]);
    tt[13]=_mm_xor_si128(tt[12],tt[1]);
    tt[14]=_mm_xor_si128(tt[8],tt[6]);
    tt[15]=_mm_xor_si128(tt[14],tt[1]);

/* Thanks to Darrel Hankerson, who pointed out an optimization for this code ... */

    i=(int)(b&0xF); j=(int)((b>>4)&0xF);
    pp=_mm_xor_si128(tt[i],  _mm_or_si128(_mm_slli_epi64(tt[j],4),
        _mm_srli_epi64(_mm_slli_si128(tt[j],8), 60)) );
    i=(int)((b>>8)&0xF); j=(int)((b>>12)&0xF);

    pp=_mm_xor_si128(pp, _mm_slli_si128( _mm_xor_si128(tt[i], 
        _mm_or_si128(_mm_slli_epi64(tt[j],4),
        _mm_srli_epi64(_mm_slli_si128(tt[j],8), 60))  
        )   ,1) );
    i=(int)((b>>16)&0xF); j=(int)((b>>20)&0xF);
    pp=_mm_xor_si128(pp, _mm_slli_si128(_mm_xor_si128(tt[i],
        _mm_or_si128(_mm_slli_epi64(tt[j],4),
        _mm_srli_epi64(_mm_slli_si128(tt[j],8), 60))   
        )   ,2) );
    i=(int)((b>>24)&0xF); j=(int)((b>>28)&0xF);
    pp=_mm_xor_si128(pp, _mm_slli_si128(_mm_xor_si128(tt[i], 
        _mm_or_si128(_mm_slli_epi64(tt[j],4),
        _mm_srli_epi64(_mm_slli_si128(tt[j],8), 60))   
        )   ,3) );
    i=(int)((b>>32)&0xF); j=(int)((b>>36)&0xF);
    pp=_mm_xor_si128(pp, _mm_slli_si128(_mm_xor_si128(tt[i], 
        _mm_or_si128(_mm_slli_epi64(tt[j],4),
        _mm_srli_epi64(_mm_slli_si128(tt[j],8), 60))   
        )   ,4) );
    i=(int)((b>>40)&0xF); j=(int)((b>>44)&0xF);
    pp=_mm_xor_si128(pp, _mm_slli_si128(_mm_xor_si128(tt[i],
        _mm_or_si128(_mm_slli_epi64(tt[j],4),
        _mm_srli_epi64(_mm_slli_si128(tt[j],8), 60))    
        )   ,5) );
    i=(int)((b>>48)&0xF); j=(int)((b>>52)&0xF);
    pp=_mm_xor_si128(pp, _mm_slli_si128(_mm_xor_si128(tt[i],
        _mm_or_si128(_mm_slli_epi64(tt[j],4),
        _mm_srli_epi64(_mm_slli_si128(tt[j],8), 60))   
        )   ,6) );
    i=(int)((b>>56)&0xF); j=(int)(b>>60);
    pp=_mm_xor_si128(pp, _mm_slli_si128(_mm_xor_si128(tt[i], 
        _mm_or_si128(_mm_slli_epi64(tt[j],4),
        _mm_srli_epi64(_mm_slli_si128(tt[j],8), 60))  
        )   ,7) );

    *r=((unsigned long long *)&pp)[0];
    return ((unsigned long long *)&pp)[1];
}

#else

static mr_small mr_mul2(mr_small a,mr_small b,mr_small *r)
{
    int k;
    mr_small kb,t[16];
    mr_small x,q,p;
    mr_utype tb0;
#if MIRACL > 32
    mr_utype tb1,tb2;
#endif

    kb=b;

#if MIRACL <= 32
    t[0]=0;               /* small look up table */
    t[3]=t[2]=a<<1;       /* it can overflow.... */
    t[1]=t[2]>>1;
    t[3]^=t[1];

    tb0=(mr_utype)(a&TOPBIT);       /* remember top bit    */
    tb0>>=M1;             /* all ones if top bit is one */
#else
    t[0]=0;               /* larger look-up table */
    t[8]=a<<3;
    t[4]=t[8]>>1;
    t[2]=t[4]>>1;
    t[1]=t[2]>>1;
    t[3]=t[5]=t[7]=t[9]=t[11]=t[13]=t[15]=t[1];
    t[3]^=t[2];
    t[5]^=t[4];
    t[9]^=t[8]; 
    t[6]=t[3]<<1;
    t[7]^=t[6];
    t[10]=t[5]<<1;
    t[11]^=t[10];
    t[12]=t[6]<<1;
    t[13]^=t[12];
    t[14]=t[7]<<1;
    t[15]^=t[14];

    tb0=(a&TOPBIT);       /* remember top bits  */
    tb0>>=M1;             /* all bits one, if this bit is set in a */
    tb1=(a&SECBIT)<<1;      
    tb1>>=M1;
    tb2=(a&THDBIT)<<2;
    tb2>>=M1;
#endif

#if MIRACL == 8
#define UNWOUNDM
    p=q=t[b&3];                       q>>=2; 
    x=t[(b>>2)&3];  q^=x; p^=(x<<2);  q>>=2;   
    x=t[(b>>4)&3];  q^=x; p^=(x<<4);  q>>=2;   
    x=t[(b>>6)];    q^=x; p^=(x<<6);  q>>=2; 
#endif

#if MIRACL == 16
#define UNWOUNDM
    p=q=t[b&3];                       q>>=2;
    x=t[(b>>2)&3];  q^=x; p^=(x<<2);  q>>=2;   
    x=t[(b>>4)&3];  q^=x; p^=(x<<4);  q>>=2;   
    x=t[(b>>6)&3];  q^=x; p^=(x<<6);  q>>=2;
    x=t[(b>>8)&3];  q^=x; p^=(x<<8);  q>>=2;
    x=t[(b>>10)&3]; q^=x; p^=(x<<10); q>>=2;
    x=t[(b>>12)&3]; q^=x; p^=(x<<12); q>>=2;
    x=t[(b>>14)];   q^=x; p^=(x<<14); q>>=2;
#endif

#if MIRACL == 32
#define UNWOUNDM
    p=q=t[b&3];                       q>>=2;
    x=t[(b>>2)&3];  q^=x; p^=(x<<2);  q>>=2;   /* 8 ASM 80386 instructions */
    x=t[(b>>4)&3];  q^=x; p^=(x<<4);  q>>=2;   /* but only 4 ARM instructions! */
    x=t[(b>>6)&3];  q^=x; p^=(x<<6);  q>>=2;
    x=t[(b>>8)&3];  q^=x; p^=(x<<8);  q>>=2;
    x=t[(b>>10)&3]; q^=x; p^=(x<<10); q>>=2;
    x=t[(b>>12)&3]; q^=x; p^=(x<<12); q>>=2;
    x=t[(b>>14)&3]; q^=x; p^=(x<<14); q>>=2;
    x=t[(b>>16)&3]; q^=x; p^=(x<<16); q>>=2;
    x=t[(b>>18)&3]; q^=x; p^=(x<<18); q>>=2;
    x=t[(b>>20)&3]; q^=x; p^=(x<<20); q>>=2;
    x=t[(b>>22)&3]; q^=x; p^=(x<<22); q>>=2;
    x=t[(b>>24)&3]; q^=x; p^=(x<<24); q>>=2;
    x=t[(b>>26)&3]; q^=x; p^=(x<<26); q>>=2;
    x=t[(b>>28)&3]; q^=x; p^=(x<<28); q>>=2;
    x=t[(b>>30)];   q^=x; p^=(x<<30); q>>=2;
#endif

#if MIRACL == 64
#define UNWOUNDM
    p=q=t[b&0xf];                       q>>=4;
    x=t[(b>>4)&0xf];  q^=x; p^=(x<<4);  q>>=4;
    x=t[(b>>8)&0xf];  q^=x; p^=(x<<8);  q>>=4;
    x=t[(b>>12)&0xf]; q^=x; p^=(x<<12); q>>=4;
	x=t[(b>>16)&0xf]; q^=x; p^=(x<<16); q>>=4;
    x=t[(b>>20)&0xf]; q^=x; p^=(x<<20); q>>=4;
    x=t[(b>>24)&0xf]; q^=x; p^=(x<<24); q>>=4;
    x=t[(b>>28)&0xf]; q^=x; p^=(x<<28); q>>=4;
    x=t[(b>>32)&0xf]; q^=x; p^=(x<<32); q>>=4;
    x=t[(b>>36)&0xf]; q^=x; p^=(x<<36); q>>=4;
    x=t[(b>>40)&0xf]; q^=x; p^=(x<<40); q>>=4;
    x=t[(b>>44)&0xf]; q^=x; p^=(x<<44); q>>=4;
    x=t[(b>>48)&0xf]; q^=x; p^=(x<<48); q>>=4;
    x=t[(b>>52)&0xf]; q^=x; p^=(x<<52); q>>=4;
    x=t[(b>>56)&0xf]; q^=x; p^=(x<<56); q>>=4;
    x=t[(b>>60)];     q^=x; p^=(x<<60); q>>=4;

#endif

#ifndef UNWOUNDM

    q=p=(mr_small)0;
    for (k=0;k<MIRACL;k+=8)
    {                 
        q^=(t[b&3]);   
        b>>=2; 
        p>>=2; 
        p|=q<<M2;
        q>>=2;

        q^=(t[b&3]);
        b>>=2;
        p>>=2;
        p|=q<<M2;
        q>>=2;

        q^=(t[b&3]);
        b>>=2;
        p>>=2;
        p|=q<<M2;
        q>>=2;

        q^=(t[b&3]);
        b>>=2;
        p>>=2;
        p|=q<<M2;
        q>>=2;
    }
#endif

#if MIRACL <= 32
    p^=(tb0&(kb<<M1));       /* compensate for top bit */
    q^=(tb0&(kb>>1));        /* don't break pipeline.. */
#else
    p^=(tb0&(kb<<M1));
    q^=(tb0&(kb>>1));
    p^=(tb1&(kb<<M2));
    q^=(tb1&(kb>>2));
    p^=(tb2&(kb<<M3));
    q^=(tb2&(kb>>3));
#endif

    *r=p;
    return q;
}

#endif

#endif

#endif

#endif

static int numbits(big x)
{ /* return degree of x */
    mr_small *gx=x->w,bit=TOPBIT;
    int m,k=x->len;
    if (k==0) return 0;
    m=k*MIRACL;
    while (!(gx[k-1]&bit))
    {
        m--;
        bit>>=1;
    }
    return m;
}

int degree2(big x)
{ /* returns -1 for x=0 */
    return (numbits(x)-1);
}

/*
static int zerobits(big x)
{ 
    int m,n,k;
    mr_small *gx,lsb,bit=1;
    k=x->len; 
    if (k==0) return (-1);
    gx=x->w;
    for (m=0;m<k;m++)
    {
        if (gx[m]==0) continue;
        n=0;
        lsb=gx[m];
        while (!(bit&lsb))
        {
            n++; 
            bit<<=1;
        }
        break;
    }
    return (MIRACL*m+n);
}

static void shiftrightbits(big x,int m)
{
    int i,k=x->len;
    int w=m/MIRACL;
    int b=m%MIRACL;  
    mr_small *gx=x->w;
    if (k==0 || m==0) return;
    if (w>0)
    {
        for (i=0;i<k-w;i++)
            gx[i]=gx[i+w];
        for (i=k-w;i<k;i++) gx[i]=0;
        x->len-=w;
    }
    if (b!=0) 
    {
        for (i=0;i<k-w-1;i++)
            gx[i]=(gx[i]>>b)|(gx[i+1]<<(MIRACL-b));   
        gx[k-w-1]>>=b;
        if (gx[k-w-1]==0) x->len--;
    }
}
*/

static void shiftleftbits(big x,int m)
{
    int i,k=x->len;
    mr_small j; 
    int w=m/MIRACL;  /* words */
    int b=m%MIRACL;  /* bits  */
    mr_small *gx=x->w;
    if (k==0 || m==0) return;
    if (w>0)
    {
        for (i=k+w-1;i>=w;i--)
            gx[i]=gx[i-w];
        for (i=w-1;i>=0;i--) gx[i]=0;
        x->len+=w;
    }
/* time critical */
    if (b!=0) 
    {
        j=gx[k+w-1]>>(MIRACL-b);
        if (j!=0)
        {
            x->len++;
            gx[k+w]=j;
        }
        for (i=k+w-1;i>w;i--)
        {
            gx[i]=(gx[i]<<b)|(gx[i-1]>>(MIRACL-b));
        }
        gx[w]<<=b;
    }
}

static void square2(big x,big w)
{ /* w=x*x where x can be NULL so be careful */
    int i,j,n,m;
    mr_small a,t,r,*gw;

    static const mr_small look[16]=
    {0,(mr_small)1<<M8,(mr_small)4<<M8,(mr_small)5<<M8,(mr_small)16<<M8,
    (mr_small)17<<M8,(mr_small)20<<M8,(mr_small)21<<M8,(mr_small)64<<M8,
    (mr_small)65<<M8,(mr_small)68<<M8,(mr_small)69<<M8,(mr_small)80<<M8,
    (mr_small)81<<M8,(mr_small)84<<M8,(mr_small)85<<M8};

    if (x!=w) copy(x,w);
    n=w->len;
    if (n==0) return;
    m=n+n;
    w->len=m;
    gw=w->w; 

    for (i=n-1;i>=0;i--)
    {
        a=gw[i];

#if MIRACL == 8
#define UNWOUNDS
        gw[i+i]=look[a&0xF];
        gw[i+i+1]=look[(a>>4)];
#endif

#if MIRACL == 16
#define UNWOUNDS
        gw[i+i]=(look[a&0xF]>>8)|look[(a>>4)&0xF];
        gw[i+i+1]=(look[(a>>8)&0xF]>>8)|look[(a>>12)];
#endif

#if MIRACL == 32
#define UNWOUNDS
        gw[i+i]=(look[a&0xF]>>24)|(look[(a>>4)&0xF]>>16)|(look[(a>>8)&0xF]>>8)|look[(a>>12)&0xF]; 
        gw[i+i+1]=(look[(a>>16)&0xF]>>24)|(look[(a>>20)&0xF]>>16)|(look[(a>>24)&0xF]>>8)|look[(a>>28)];
#endif

#ifndef UNWOUNDS

        r=0;
        for (j=0;j<MIRACL/2;j+=4)
        {
            t=look[a&0xF];
            a>>=4;
            r>>=8;
            r|=t;
        }
        gw[i+i]=r; r=0;

        for (j=0;j<MIRACL/2;j+=4)
        {
            t=look[a&0xF];
            a>>=4;
            r>>=8;
            r|=t;
        }
        gw[i+i+1]=r;

#endif

    }
    if (gw[m-1]==0) 
    {
        w->len--;
        if (gw[m-2]==0)
            mr_lzero(w);
    }
}

/* Use karatsuba to multiply two polynomials with coefficients in GF(2^m) */

#ifndef MR_STATIC

void karmul2_poly(_MIPD_ int n,big *t,big *x,big *y,big *z)
{
    int m,nd2,nd,md,md2;                          
    if (n==1) 
    { /* finished */
        modmult2(_MIPP_ *x,*y,*z);
        zero(z[1]);
        return;
    }       
    if (n==2)
    {  /* in-line 2x2 */
        modmult2(_MIPP_ x[0],y[0],z[0]);
        modmult2(_MIPP_ x[1],y[1],z[2]);
        add2(x[0],x[1],t[0]);
        add2(y[0],y[1],t[1]);
        modmult2(_MIPP_ t[0],t[1],z[1]);
        add2(z[1],z[0],z[1]);
        add2(z[1],z[2],z[1]);
        zero(z[3]);
        return;
    }

    if (n==3)
    {
        modmult2(_MIPP_ x[0],y[0],z[0]); 
        modmult2(_MIPP_ x[1],y[1],z[2]); 
        modmult2(_MIPP_ x[2],y[2],z[4]); 
        add2(x[0],x[1],t[0]);
        add2(y[0],y[1],t[1]);
        modmult2(_MIPP_ t[0],t[1],z[1]); 
        add2(z[1],z[0],z[1]);  
        add2(z[1],z[2],z[1]);  
        add2(x[1],x[2],t[0]);
        add2(y[1],y[2],t[1]);
        modmult2(_MIPP_ t[0],t[1],z[3]); 
        add2(z[3],z[2],z[3]);
        add2(z[3],z[4],z[3]); 
        add2(x[0],x[2],t[0]);
        add2(y[0],y[2],t[1]);   
        modmult2(_MIPP_ t[0],t[1],t[0]); 
        add2(z[2],t[0],z[2]);
        add2(z[2],z[0],z[2]);
        add2(z[2],z[4],z[2]); 
        zero(z[5]);
        return;
    }

    if (n%2==0)
    {
        md=nd=n;
        md2=nd2=n/2;
    }
    else
    {
        nd=n+1;
        md=n-1;
        nd2=nd/2; md2=md/2;
    }

    for (m=0;m<nd2;m++)
    {
        copy(x[m],z[m]);
        copy(y[m],z[nd2+m]);
    }
    for (m=0;m<md2;m++)
    { 
        add2(z[m],x[nd2+m],z[m]);
        add2(z[nd2+m],y[nd2+m],z[nd2+m]);
    }

    karmul2_poly(_MIPP_ nd2,&t[nd],z,&z[nd2],t); 

    karmul2_poly(_MIPP_ nd2,&t[nd],x,y,z);  

    for (m=0;m<nd;m++) add2(t[m],z[m],t[m]);

    karmul2_poly(_MIPP_ md2,&t[nd],&x[nd2],&y[nd2],&z[nd]);

    for (m=0;m<md;m++) add2(t[m],z[nd+m],t[m]);
    for (m=0;m<nd;m++) add2(z[nd2+m],t[m],z[nd2+m]);
}

void karmul2_poly_upper(_MIPD_ int n,big *t,big *x,big *y,big *z)
{ /* n is large and even, and upper half of z is known already */
    int m,nd2,nd;
    nd2=n/2; nd=n;

    for (m=0;m<nd2;m++)
    { 
        add2(x[m],x[nd2+m],z[m]);
        add2(y[m],y[nd2+m],z[nd2+m]);
    }

    karmul2_poly(_MIPP_ nd2,&t[nd],z,&z[nd2],t); 

    karmul2_poly(_MIPP_ nd2,&t[nd],x,y,z);   /* only 2 karmuls needed! */

    for (m=0;m<nd;m++) add2(t[m],z[m],t[m]);

    for (m=0;m<nd2;m++) 
    {
        add2(z[nd+m],z[nd+nd2+m],z[nd+m]);
        add2(z[nd+m],t[nd2+m],z[nd+m]);
    }

    for (m=0;m<nd;m++) 
    {
        add2(t[m],z[nd+m],t[m]);
        add2(z[nd2+m],t[m],z[nd2+m]);
    }
}

#endif

/* Some in-line karatsuba down at the bottom... */

#ifndef MR_COMBA2

static void mr_bottom2(mr_small *x,mr_small *y,mr_small *z)
{
    mr_small q0,r0,q1,r1,q2,r2;

    q0=mr_mul2(x[0],y[0],&r0);
    q1=mr_mul2(x[1],y[1],&r1);
    q2=mr_mul2((mr_small)(x[0]^x[1]),(mr_small)(y[0]^y[1]),&r2);

    z[0]=r0;
    z[1]=q0^r1^r0^r2;
    z[2]=q0^r1^q1^q2;
    z[3]=q1;
}

static void mr_bottom3(mr_small *x,mr_small *y,mr_small *z)
{ /* just 6 mr_muls... */
    mr_small q0,r0,q1,r1,q2,r2;
    mr_small a0,b0,a1,b1,a2,b2;

    q0=mr_mul2(x[0],y[0],&r0);
    q1=mr_mul2(x[1],y[1],&r1);
    q2=mr_mul2(x[2],y[2],&r2);

    a0=mr_mul2((mr_small)(x[0]^x[1]),(mr_small)(y[0]^y[1]),&b0);
    a1=mr_mul2((mr_small)(x[1]^x[2]),(mr_small)(y[1]^y[2]),&b1);
    a2=mr_mul2((mr_small)(x[0]^x[2]),(mr_small)(y[0]^y[2]),&b2);

    b0^=r0^r1;
    a0^=q0^q1;
    b1^=r1^r2;
    a1^=q1^q2;
    b2^=r0^r2;
    a2^=q0^q2;

    z[0]=r0;
    z[1]=q0^b0;
    z[2]=r1^a0^b2;
    z[3]=q1^b1^a2;
    z[4]=r2^a1;
    z[5]=q2;
}

static void mr_bottom4(mr_small *x,mr_small *y,mr_small *z)
{ /* unwound 4x4 karatsuba multiplication - only 9 muls */
    mr_small q0,r0,q1,r1,q2,r2,tx,ty;
    mr_small t0,t1,t2,t3;
    mr_small z0,z1,z2,z3,z4,z5,z6,z7;
    mr_small x0,x1,x2,x3,y0,y1,y2,y3;

    x0=x[0]; x1=x[1]; x2=x[2]; x3=x[3];
    y0=y[0]; y1=y[1]; y2=y[2]; y3=y[3];

    q0=mr_mul2(x0,y0,&r0);
    q1=mr_mul2(x1,y1,&r1);

    tx=x0^x1;
    ty=y0^y1;
    q2=mr_mul2(tx,ty,&r2);

    z0=r0;
    q0^=r1;
    z1=q0^r0^r2;
    z2=q0^q1^q2;
    z3=q1;

    q0=mr_mul2(x2,y2,&r0);

    q1=mr_mul2(x3,y3,&r1);

    tx=x2^x3;
    ty=y2^y3;
    q2=mr_mul2(tx,ty,&r2);

    z4=r0;
    q0^=r1;
    z5=q0^r0^r2;
    z6=q0^q1^q2;
    z7=q1;

    x2^=x0;
    y2^=y0;
    q0=mr_mul2(x2,y2,&r0);
   
    x3^=x1;
    y3^=y1;
    q1=mr_mul2(x3,y3,&r1);

    x2^=x3;
    y2^=y3;
    q2=mr_mul2(x2,y2,&r2);

    q0^=r1;
    t0=z0^z4^r0;
    t1=z1^z5^q0^r0^r2;
    t2=z2^z6^q0^q1^q2;
    t3=z3^z7^q1; 

    z2^=t0;
    z3^=t1;
    z4^=t2;
    z5^=t3;

    z[0]=z0;
    z[1]=z1;
	z[2]=z2;
	z[3]=z3;
	z[4]=z4;
	z[5]=z5;
	z[6]=z6;
	z[7]=z7;
}

static void mr_bottom5(mr_small *x,mr_small *y,mr_small *z)
{ /* Montgomery's 5x5 formula. Requires 13 mr_muls.... */
    mr_small u0,v0,u1,v1,u2,v2,s0,t0,q,r,t;
	mr_small z1,z2,z3,z4,z5,z6,z7,z8;
    mr_small x0,x1,x2,x3,x4,y0,y1,y2,y3,y4;

    x0=x[0]; x1=x[1]; x2=x[2]; x3=x[3]; x4=x[4];
    y0=y[0]; y1=y[1]; y2=y[2]; y3=y[3]; y4=y[4];

	q=mr_mul2(x0,y0,&r);
	t=q^r;
	z[0]=r;
	z1=t;
	z2=t;
	z3=q;
	z4=r;
	z5=t;
	z6=t;
	z7=q;
	q=mr_mul2(x1,y1,&r);
	z1^=r;
	z2^=q;
	z4^=r;
	z5^=q;
	q=mr_mul2(x3,y3,&r);
	z4^=r;
	z5^=q;
	z7^=r;
	z8=q;
	q=mr_mul2(x4,y4,&r);
	t=q^r;
	z2^=r;
	z3^=t;
	z4^=t;
	z5^=q;
	z6^=r;
	z7^=t;
    z8^=t;
	z[9]=q;

	u0=x0^x4;   /* u0=a0+a4 */
	v0=y0^y4;   /* v0=b0+b4 */
	q=mr_mul2(u0,v0,&r);
	t=q^r;
	z2^=r;
	z3^=t;
	z4^=q;
	z5^=r;
	z6^=t;
	z7^=q;

	u1=x0^x1;  /* u1=a0+a1 */
	v1=y0^y1;  /* v1=b0+b1 */
	q=mr_mul2(u1,v1,&r);
	t=q^r;
	z1^=r;
	z2^=t;
	z3^=q;
	z4^=r;
	z5^=t;
	z6^=q;

	u2=x3^x4; /* u2=a3+a4 */
	v2=y3^y4; /* v2=b3+b4 */
	q=mr_mul2(u2,v2,&r);
	t=q^r;
	z3^=r;
	z4^=t;
	z5^=q;
	z6^=r;
	z7^=t;
	z8^=q;

	/*u=u1^u2;   u=a0+a1+a3+a4 */
	/*v=v1^v2;    v=b0+b1+b3+b4 */
	q=mr_mul2(u1^u2,v1^v2,&r);
	z3^=r;
	z4^=q;
	z5^=r;
	z6^=q;

	s0=u1^x2;  /* s0=a0+a1+a2 */
	t0=v1^y2;   /* t0=b0+b1+b2 */
	u2^=x2;    /* u2=a2+a3+a4 */
	v2^=y2;    /* v2=b2+b3+b4 */
	u1^=u2;        /* u1=a0+a1+a2+a3+a4 */
	v1^=v2;        /* v1=b0+b1+b2+b3+b4 */
	q=mr_mul2(u1,v1,&r);
	t=q^r;
	z3^=r;
	z4^=t;
	z5^=t;
	z6^=q;

	u2^=x0;  /* s1=a0+a2+a3+a4 */
	v2^=y0;   /* t1=b0+b2+b3+b4 */
	q=mr_mul2(u2,v2,&r);
	z3^=r;
	z4^=q;
	z6^=r;
	z7^=q;

	s0^=x4;  /* s0=a0+a1+a2+a4 */
	t0^=y4;   /* t0=b0+b1+b2+b4 */
	q=mr_mul2(s0,t0,&r);
	z2^=r;
	z3^=q;
	z5^=r;
	z6^=q;

	u2^=x4; /* u2=a0+a2+a3 */
	v2^=y4;  /* v2=b0+b2+b3 */
	q=mr_mul2(u2,v2,&r);
	z4^=r;
	z5^=q;
	z6^=r;
	z7^=q;

	s0^=x0; /* s0=a1+a2+a4 */
	t0^=y0;  /* t0=b1+b2+b4 */
	q=mr_mul2(s0,t0,&r);
	z2^=r;
	z3^=q;
	z4^=r;
	z5^=q;

	z[1]=z1;
	z[2]=z2;
	z[3]=z3;
	z[4]=z4;
	z[5]=z5;
	z[6]=z6;
	z[7]=z7;
	z[8]=z8;
}


void karmul2(int n,mr_small *t,mr_small *x,mr_small *y,mr_small *z)
{ /* Karatsuba multiplication - note that n can be odd or even */
    int m,nd2,nd,md,md2;

    if (n<=5)
    {
        if (n==1)
        {
            z[1]=mr_mul2(x[0],y[0],&z[0]);
            return;
        }
        if (n==2)
        {   
            mr_bottom2(x,y,z);
            return;
        }
        if (n==3)
        {   
            mr_bottom3(x,y,z);
            return;
        }
        if (n==4)
        {   
            mr_bottom4(x,y,z);
            return;
        }
        if (n==5)
        {   
            mr_bottom5(x,y,z);
            return;
        }
    }
    if (n%2==0)
    {
        md=nd=n;
        md2=nd2=n/2;
    }
    else
    {
        nd=n+1;
        md=n-1;
        nd2=nd/2; md2=md/2;
    }

    for (m=0;m<nd2;m++)
    {
        z[m]=x[m];
        z[nd2+m]=y[m];
    }
    for (m=0;m<md2;m++)
    {
        z[m]^=x[nd2+m];
        z[nd2+m]^=y[nd2+m];
    }

    karmul2(nd2,&t[nd],z,&z[nd2],t); 
    karmul2(nd2,&t[nd],x,y,z);  

    for (m=0;m<nd;m++) t[m]^=z[m];

    karmul2(md2,&t[nd],&x[nd2],&y[nd2],&z[nd]);

    for (m=0;m<md;m++) t[m]^=z[nd+m];
    for (m=0;m<nd;m++) z[nd2+m]^=t[m];
}

#endif

/* this is time-critical, so use karatsuba here, since addition is cheap *
 * and easy (no carries to worry about...)                               */

/* #define CLAIRE */

void multiply2(_MIPD_ big x,big y,big w)
{

#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

#ifdef MR_COMBA2
    comba_mult2(_MIPP_ x,y,w);
    return;
#else
    int i,j,xl,yl,ml;
#ifdef CLAIRE
    int d;
    mr_small hi,lo;
#endif
    mr_small p,q;

    big w0=mr_mip->w0;

    if (x==NULL || y==NULL)
    {
        zero(w);
        return;
    }
    if (x->len==0 || y->len==0)
    {
        zero(w);
        return;
    }

    xl=x->len;
    yl=y->len;
    zero(w0);

#ifdef CLAIRE

/* Comba method */

    w0->len=xl+yl;
    d=1+mr_mip->M/MIRACL;
    hi=lo=0;
    for (i=0;i<d;i++)
    {
        for (j=0;j<=i;j++)
        {
            q=mr_mul2(x->w[j],y->w[i-j],&p);
            hi^=q; lo^=p;
        }
        w0->w[i]=lo; lo=hi; hi=0;
    }
    for (i=d;i<2*d-1;i++)
    {
        for (j=i-d+1;j<d;j++)
        {
            q=mr_mul2(x->w[j],y->w[i-j],&p);
            hi^=q; lo^=p;
        }
        w0->w[i]=lo; lo=hi; hi=0;
    }
    w0->w[2*d-1]=lo;
    mr_lzero(w0);
    copy(w0,w);

#else

/* recommended method as mr_mul2 is so slow... */

    if (xl>=MR_KARATSUBA && yl>=MR_KARATSUBA)
    { 
        if (xl>yl) ml=xl;
        else       ml=yl;
     
        karmul2(ml,mr_mip->w7->w,x->w,y->w,w0->w);

        mr_mip->w7->len=w0->len=2*ml+1;
        mr_lzero(w0);
        mr_lzero(mr_mip->w7);    
        copy(w0,w);
        return;
    }

    w0->len=xl+yl;
    for (i=0;i<xl;i++)
    { 
        for (j=0;j<yl;j++)
        {
            q=mr_mul2(x->w[i],y->w[j],&p); 
            w0->w[i+j]^=p;
            w0->w[i+j+1]^=q;
        } 
    }
    mr_lzero(w0);
    copy(w0,w);
#endif
#endif
}

void add2(big x,big y,big z)
{ /* XOR x and y */
    int i,lx,ly,lz,lm;
    mr_small *gx,*gy,*gz;

    if (x==y)
    {
        zero(z);
        return;
    }
    if (y==NULL)
    {
        copy(x,z);
        return;
    }
    else if (x==NULL) 
    {
        copy(y,z);
        return;
    }

    if (x==z)
    {
        gy=y->w; gz=z->w;
        ly=y->len; lz=z->len;
        lm=lz; if (ly>lz) lm=ly;
        for (i=0;i<lm;i++)
            gz[i]^=gy[i];
        z->len=lm;
        if (gz[lm-1]==0) mr_lzero(z);
    }
    else
    {
        gx=x->w; gy=y->w; gz=z->w;
        lx=x->len; ly=y->len; lz=z->len;
        lm=lx; if (ly>lx) lm=ly;

        for (i=0;i<lm;i++)
            gz[i]=gx[i]^gy[i];
        for (i=lm;i<lz;i++)
            gz[i]=0;
        z->len=lm;
        if (gz[lm-1]==0) mr_lzero(z);
    }
}

static void remain2(_MIPD_ big y,big x)
{ /* generic "remainder" program. x%=y */
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    int my=numbits(y);
    int mx=numbits(x);
    while (mx>=my)
    {
        copy(y,mr_mip->w7);
        shiftleftbits(mr_mip->w7,mx-my);
        add2(x,mr_mip->w7,x);    
        mx=numbits(x);
    }
    return;
}

void gcd2(_MIPD_ big x,big y,big g)
{
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (size(y)==0) 
    {
        copy(x,g);
        return;
    }
    copy(x,mr_mip->w1);
    copy(y,mr_mip->w2);
    forever
    {
        remain2(_MIPP_ mr_mip->w2,mr_mip->w1);
        if (size(mr_mip->w1)==0) break;
        copy(mr_mip->w1,mr_mip->w3);
        copy(mr_mip->w2,mr_mip->w1);
        copy(mr_mip->w3,mr_mip->w2);
    }
    copy(mr_mip->w2,g);
}


/* See "Elliptic Curves in Cryptography", Blake, Seroussi & Smart, 
   Cambridge University Press, 1999, page 20, for this fast reduction
   routine - algorithm II.9 */

void reduce2(_MIPD_ big y,big x)
{ /* reduction wrt the trinomial or pentanomial modulus        *
   * Note that this is linear O(n), and thus not time critical */
    int k1,k2,k3,k4,ls1,ls2,ls3,ls4,rs1,rs2,rs3,rs4,i;
    int M,A,B,C;
    int xl;
    mr_small top,*gx,w;

#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

    if (x!=y) copy(y,x);
    xl=x->len;
    gx=x->w;

    M=mr_mip->M;
    A=mr_mip->AA;
    if (A==0)
    {
        mr_berror(_MIPP_ MR_ERR_NO_BASIS);
        return;
    }
    B=mr_mip->BB;
    C=mr_mip->CC;

 
/* If optimizing agressively it makes sense to make this code specific to a particular field
   For example code like this can be optimized for the case 
   m=163. Note that the general purpose code involves lots of branches - these cause breaks in 
   the pipeline and they are slow. Further loop unrolling would be even faster...

   Version 5.10 - optimal code for 32-bit processors and for some NIST curves added
   Version 5.22 - some code for a 16-bit processor..

   Version 5.23 - Use findbase.cpp to find "best" irreducible polynomial
   Version 5.23 - Use utility irp.cpp to automatically generate optimal code for insertion here
*/

#if MIRACL == 8

    if (M==163 && A==7 && B==6 && C==3)
    {
        for (i=xl-1;i>=21;i--)
        {
            w=gx[i]; gx[i]=0;
            gx[i-19]^=(w>>4)^(w>>5);
            gx[i-20]^=(w>>3)^(w<<4)^(w<<3)^w;
            gx[i-21]^=(w<<5);
        }   /* XORs= 7 shifts= 6 */
        top=gx[20]>>3;
        gx[0]^=top;
        top<<=3;
        gx[0]^=(top<<4)^(top<<3)^top;
        gx[1]^=(top>>4)^(top>>5);
        gx[20]^=top;
        x->len=21;
        if (gx[20]==0) mr_lzero(x);
        return;
    }

    if (M==271 && A==201)
    {
        for (i=xl-1;i>=34;i--)
        {
            w=gx[i]; gx[i]=0;
            gx[i-8]^=(w>>6);
            gx[i-9]^=(w<<2);
            gx[i-33]^=(w>>7);
            gx[i-34]^=(w<<1);
        }   /* XORs= 4 shifts= 4 */
        top=gx[33]>>7;
        gx[0]^=top;
        top<<=7;
        gx[24]^=(top<<2);
        gx[25]^=(top>>6);
        gx[33]^=top;
        x->len=34;
        if (gx[33]==0) mr_lzero(x);
        return;
    }

    if (M==271 && A==207 && B==175 && C==111)
    {
        for (i=xl-1;i>=34;i--)
        {
            w=gx[i]; gx[i]=0;
            gx[i-8]^=w;
            gx[i-12]^=w;
            gx[i-20]^=w;
            gx[i-33]^=(w>>7);
            gx[i-34]^=(w<<1);
        }   /* XORs= 5 shifts= 2 */
        top=gx[33]>>7;
        gx[0]^=top;
        top<<=7;
        gx[13]^=top;
        gx[21]^=top;
        gx[25]^=top;
        gx[33]^=top;
        x->len=34;
        if (gx[33]==0) mr_lzero(x);
        return;
    }

#endif

#if MIRACL == 16

    if (M==163 && A==7 && B==6 && C==3)
    {
        for (i=xl-1;i>=11;i--)
        {
            w=gx[i]; gx[i]=0;
            gx[i-10]^=(w>>3)^(w<<3)^(w<<4)^w;
            gx[i-11]^=(w<<13);
            gx[i-9]^=(w>>12)^(w>>13);
        }
        top=gx[10]>>3;
        gx[0]^=top;
        top<<=3;

        gx[1]^=(top>>12)^(top>>13);
        gx[0]^=(top<<4)^(top<<3)^top;

        gx[10]^=top;
        x->len=11;
        if (gx[10]==0) mr_lzero(x);
        
        return;
    }

    if (M==271 && A==201 && B==0)
    {
        for (i=xl-1;i>=17;i--)
        {
            w=gx[i]; gx[i]=0;
            gx[i-17]^=(w<<1);
            gx[i-16]^=(w>>15);
            gx[i-5]^=(w<<10);
            gx[i-4]^=(w>>6);
        }
        top=gx[16]>>15;
        gx[0]^=top;
        top<<=15;
        gx[12]^=(top>>6);
        gx[11]^=(top<<10);

        gx[16]^=top;
        x->len=17;
        if (gx[16]==0) mr_lzero(x);

        return;
    }

    if (M==271 && A==207 && B==175 && C==111)
    {
        for (i=xl-1;i>=17;i--)
        {
            w=gx[i]; gx[i]=0;
            gx[i-4]^=w;
            gx[i-6]^=w;
            gx[i-10]^=w;
            gx[i-16]^=(w>>15);
            gx[i-17]^=(w<<1);
        }   /* XORs= 5 shifts= 2 */
        top=gx[16]>>15;
        gx[0]^=top;
        top<<=15;
        gx[6]^=top;
        gx[10]^=top;
        gx[12]^=top;
        gx[16]^=top;
        x->len=17;
        if (gx[16]==0) mr_lzero(x);
        return;
    }

#endif

#if MIRACL == 32

    if (M==127 && A==63)
    {
        for (i=xl-1;i>=4;i--)
        {
            w=gx[i]; gx[i]=0;
            gx[i-2]^=w;
            gx[i-3]^=(w>>31);
            gx[i-4]^=(w<<1);
        }   /* XORs= 3 shifts= 2 */
        top=gx[3]>>31; gx[0]^=top; top<<=31;
        gx[1]^=top;
        gx[3]^=top;
        x->len=4;
        if (gx[3]==0) mr_lzero(x);
        return;
    }

    if (M==163 && A==7 && B==6 && C==3)
    {
        for (i=xl-1;i>=6;i--)
        {
            w=gx[i]; gx[i]=0;
            gx[i-5]^=((w>>3)^(w<<4)^(w<<3)^w);
            gx[i-6]^=(w<<29);              
            gx[i-4]^=((w>>28)^(w>>29));
        }
        top=gx[5]>>3;
        
        gx[0]^=top;
        top<<=3;
        gx[1]^=(top>>28)^(top>>29);
        gx[0]^=top^(top<<4)^(top<<3);

        gx[5]^=top;
        
        x->len=6; 
        if (gx[5]==0) mr_lzero(x);
        
        return;
    }

    if (M==163 && A==99 && B==97 && C==3)
    {
        for (i=xl-1;i>=6;i--)
        {
            w=gx[i]; gx[i]=0;
            gx[i-2]^=w^(w>>2);
            gx[i-3]^=(w<<30);
            gx[i-5]^=(w>>3)^w;
            gx[i-6]^=(w<<29);
        } 
        top=gx[5]>>3;
        gx[0]^=top;
        top<<=3;
        gx[0]^=top;
        gx[2]^=(top<<30);
        gx[3]^=top^(top>>2);
        gx[5]^=top;
        x->len=6;
        if (gx[5]==0) mr_lzero(x);
        return;
    }

    if (M==233 && A==74 && B==0)
    {
        for (i=xl-1;i>=8;i--)
        {
            w=gx[i]; gx[i]=0;
            gx[i-8]^=(w<<23);
            gx[i-7]^=(w>>9);
            gx[i-5]^=(w<<1);
            gx[i-4]^=(w>>31);
        }
        top=gx[7]>>9;
        gx[0]^=top;
        gx[2]^=(top<<10);
        gx[3]^=(top>>22);
        gx[7]&=0x1FF;

        x->len=8;
        if (gx[7]==0) mr_lzero(x);
        return;
    }

    if (M==233 && A==159 && B==0)
    {
        for (i=xl-1;i>=8;i--)
        {
            w=gx[i]; gx[i]=0;
            gx[i-2]^=(w>>10);
            gx[i-3]^=(w<<22);
            gx[i-7]^=(w>>9);
            gx[i-8]^=(w<<23);
        }   
        top=gx[7]>>9;
        gx[0]^=top;
        top<<=9;
        gx[4]^=(top<<22);
        gx[5]^=(top>>10);
        gx[7]^=top;
        x->len=8;
        if (gx[7]==0) mr_lzero(x);
        return;
    }

    if (M==233 && A==201 && B==105 && C==9)
    {
        for (i=xl-1;i>=8;i--)
        {
            w=gx[i]; gx[i]=0;
            gx[i-1]^=w;
            gx[i-4]^=w;
            gx[i-7]^=(w>>9)^w;
            gx[i-8]^=(w<<23);
        }   
        top=gx[7]>>9;
        gx[0]^=top;
        top<<=9;
        gx[0]^=top;
        gx[3]^=top;
        gx[6]^=top;
        gx[7]^=top;
        x->len=8;
        if (gx[7]==0) mr_lzero(x);

        return;
    }

    if (M==103 && A==9 && B==0)
    {
        for (i=xl-1;i>=4;i--)
        {
            w=gx[i]; gx[i]=0;
            gx[i-3]^=((w>>7)^(w<<2));
            gx[i-4]^=(w<<25);
            gx[i-2]^=(w>>30);
        }
        top=gx[3]>>7;
        gx[0]^=top;
        top<<=7;
        gx[1]^=(top>>30);
        gx[0]^=(top<<2);
        gx[3]^=top;
        x->len=4;
        if (gx[3]==0) mr_lzero(x);

        return;
    }

    if (M==283 && A==12 && B==7 && C==5)
    {
        for (i=xl-1;i>=9;i--)
        {
            w=gx[i]; gx[i]=0;
            gx[i-9]^=(w<<5)^(w<<10)^(w<<12)^(w<<17);
            gx[i-8]^=(w>>27)^(w>>22)^(w>>20)^(w>>15);
        }

        top=gx[8]>>27;
        gx[0]^=top^(top<<5)^(top<<7)^(top<<12);
        gx[8]&=0x7FFFFFF;

        x->len=9;
        if (gx[8]==0) mr_lzero(x);
        return;
    }

    if (M==283 && A==249 && B==219 && C==27)
    {
        for (i=xl-1;i>=9;i--)
        {
            w=gx[i]; gx[i]=0;
            gx[i-1]^=(w>>2);
            gx[i-2]^=(w<<30)^w;
            gx[i-8]^=(w>>27)^w;
            gx[i-9]^=(w<<5);
        }   /* XORs= 6 shifts= 4 */
        top=gx[8]>>27;
        gx[0]^=top;
        top<<=27;
        gx[0]^=top;
        gx[6]^=(top<<30)^top;
        gx[7]^=(top>>2);
        gx[8]^=top;
        x->len=9;
        if (gx[8]==0) mr_lzero(x);
        return;
    }

    if (M==313 && A==121 && B==0)
    {
        for (i=xl-1;i>=10;i--)
        {
            w=gx[i]; gx[i]=0;
            gx[i-6]^=w;
            gx[i-9]^=(w>>25);
            gx[i-10]^=(w<<7);
        }  
        top=gx[9]>>25;
        gx[0]^=top;
        top<<=25;
        gx[3]^=top;
        gx[9]^=top;
        x->len=10;
        if (gx[9]==0) mr_lzero(x);
        return;
    }

    if (M==379 && A==253 && B==251 && C==59)
    {
        for (i=xl-1;i>=12;i--)
        {
            w=gx[i]; gx[i]=0;
            gx[i-3]^=(w>>30);
            gx[i-4]^=(w<<2)^w;
            gx[i-10]^=w;
            gx[i-11]^=(w>>27);
            gx[i-12]^=(w<<5);
        }   /* XORs= 6 shifts= 4 */
        top=gx[11]>>27; gx[0]^=top; top<<=27;
        gx[1]^=top;
        gx[7]^=(top<<2)^top;
        gx[8]^=(top>>30);
        gx[11]^=top;
        x->len=12;
        if (gx[11]==0) mr_lzero(x);
        return;
    }

    if (M==571 && A==10 && B==5 && C==2)
    {
        for (i=xl-1;i>=18;i--)
        {
            w=gx[i]; gx[i]=0;
            gx[i-18]^=(w<<5)^(w<<7)^(w<<10)^(w<<15);
            gx[i-17]^=(w>>27)^(w>>25)^(w>>22)^(w>>17);
        }

        top=gx[17]>>27;
        gx[0]^=top^(top<<2)^(top<<5)^(top<<10);
        gx[17]&=0x7FFFFFF;

        x->len=18;
        if (gx[17]==0) mr_lzero(x);

        return;
    }

    if (M==571 && A==507 && B==475 && C==417)
    {
        for (i=xl-1;i>=18;i--)
        {
            w=gx[i]; gx[i]=0;
            gx[i-2]^=w;
            gx[i-3]^=w;
            gx[i-4]^=(w>>26);
            gx[i-5]^=(w<<6);
            gx[i-17]^=(w>>27);
            gx[i-18]^=(w<<5);
        }   /* XORs= 6 shifts= 4 */
        top=gx[17]>>27;
        gx[0]^=top;
        top<<=27;
        gx[12]^=(top<<6);
        gx[13]^=(top>>26);
        gx[14]^=top;
        gx[15]^=top;
        gx[17]^=top;
        x->len=18;
        if (gx[17]==0) mr_lzero(x);
        return;
    }

    if (M==1223 && A==255)
    {
        for (i=xl-1;i>=39;i--)
        {
            w=gx[i]; gx[i]=0;
            gx[i-30]^=(w>>8);
            gx[i-31]^=(w<<24);
            gx[i-38]^=(w>>7);
            gx[i-39]^=(w<<25);
        }   /* XORs= 4 shifts= 4 */
        top=gx[38]>>7; gx[0]^=top; top<<=7;
        gx[7]^=(top<<24);
        gx[8]^=(top>>8);
        gx[38]^=top;
        x->len=39;
        if (gx[38]==0) mr_lzero(x);
        return;
    }

#endif

#if MIRACL == 64
    if (M==1223 && A==255)
    {
        for (i=xl-1;i>=20;i--)
        {
            w=gx[i]; gx[i]=0;
            gx[i-15]^=(w>>8);
            gx[i-16]^=(w<<56);
            gx[i-19]^=(w>>7);
            gx[i-20]^=(w<<57);
        }  
        top=gx[19]>>7; gx[0]^=top; top<<=7;
        gx[3]^=(top<<56);
        gx[4]^=(top>>8);
        gx[19]^=top;
        x->len=20;
        if (gx[19]==0) mr_lzero(x);
        return;
    }

    if (M==379 && A==253 && B==251 && C==59)
    {
        for (i=xl-1;i>=6;i--)
        {
            w=gx[i]; gx[i]=0;
            gx[i-1]^=(w>>62);
            gx[i-2]^=(w<<2)^w;
            gx[i-5]^=(w>>59)^w;
            gx[i-6]^=(w<<5);
        }   /* XORs= 6 shifts= 4 */
        top=gx[5]>>59; gx[0]^=top; top<<=59;
        gx[0]^=top;
        gx[3]^=(top<<2)^top;
        gx[4]^=(top>>62);
        gx[5]^=top;
        x->len=6;
        if (gx[5]==0) mr_lzero(x);
        return;
    }
#endif

    k3=k4=rs3=ls3=rs4=ls4=0;
    k1=1+M/MIRACL;       /* words from MSB to LSB */

    if (xl<=k1)
    {
      if (numbits(x)<=M) return;
    }

    rs1=M%MIRACL;
    ls1=MIRACL-rs1;

    if (M-A < MIRACL)
    { /* slow way */
        while (numbits(x)>=M+1)
        {
            copy(mr_mip->modulus,mr_mip->w7);
            shiftleftbits(mr_mip->w7,numbits(x)-M-1);
            add2(x,mr_mip->w7,x);    
        }
        return;
    }

    k2=1+(M-A)/MIRACL;   /* words from MSB to bit */
    rs2=(M-A)%MIRACL;
    ls2=MIRACL-rs2;

    if (B)
    { /* Pentanomial */
        k3=1+(M-B)/MIRACL;
        rs3=(M-B)%MIRACL;
        ls3=MIRACL-rs3;

        k4=1+(M-C)/MIRACL;
        rs4=(M-C)%MIRACL;
        ls4=MIRACL-rs4;
    }
    
    for (i=xl-1;i>=k1;i--)
    {
        w=gx[i]; gx[i]=0;
        if (rs1==0) gx[i-k1+1]^=w;
        else
        {
            gx[i-k1+1]^=(w>>rs1);
            gx[i-k1]^=(w<<ls1);
        }
        if (rs2==0) gx[i-k2+1]^=w;
        else
        {
            gx[i-k2+1]^=(w>>rs2);
            gx[i-k2]^=(w<<ls2);
        }
        if (B)
        {
            if (rs3==0) gx[i-k3+1]^=w;
            else
            {
                gx[i-k3+1]^=(w>>rs3);
                gx[i-k3]^=(w<<ls3);
            }
            if (rs4==0) gx[i-k4+1]^=w;
            else
            {
                gx[i-k4+1]^=(w>>rs4);
                gx[i-k4]^=(w<<ls4);
            }
        }
    }

    top=gx[k1-1]>>rs1;

    if (top!=0)
    {  
        gx[0]^=top;
        top<<=rs1;

        if (rs2==0) gx[k1-k2]^=top;
        else
        {
            gx[k1-k2]^=(top>>rs2);
            if (k1>k2) gx[k1-k2-1]^=(top<<ls2);
        }
        if (B)
        {
            if (rs3==0) gx[k1-k3]^=top;
            else
            {
                gx[k1-k3]^=(top>>rs3);
                if (k1>k3) gx[k1-k3-1]^=(top<<ls3);
            }
            if (rs4==0) gx[k1-k4]^=top;
            else
            {
                gx[k1-k4]^=(top>>rs4);
                if (k1>k4) gx[k1-k4-1]^=(top<<ls4);
            }
        }
        gx[k1-1]^=top;
    }
    x->len=k1;
    if (gx[k1-1]==0) mr_lzero(x);
}

void incr2(big x,int n,big w)
{ /* increment x by small amount */
    if (x!=w) copy(x,w);
    if (n==0) return;
    if (w->len==0)
    {
        w->len=1;
        w->w[0]=n;
    }
    else
    {
        w->w[0]^=(mr_small)n;
        if (w->len==1 && w->w[0]==0) w->len=0;
    }
}

void modsquare2(_MIPD_ big x,big w)
{ /* w=x*x mod f */
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

    square2(x,mr_mip->w0);
    reduce2(_MIPP_ mr_mip->w0,mr_mip->w0);
    copy(mr_mip->w0,w);
}

/* Experimental code for GF(2^103) modular multiplication *
 * Inspired by Robert Harley's ECDL code                  */

#ifdef SP103

#ifdef __GNUC__
#include <xmmintrin.h>
#else
#include <emmintrin.h>
#endif

void modmult2(_MIPD_ big x,big y,big w)
{
    int i,j;
    mr_small b;

    __m128i t[16];
    __m128i m,r,s,p,q,xe,xo;
    __m64 a3,a2,a1,a0,top;

    if (x==y)
    {
        modsquare2(_MIPP_ x,w);
        return;
    }
    
    if (x->len==0 || y->len==0)
    {
        zero(w);
        return;
    }

#ifdef MR_COUNT_OPS
fpm2++; 
#endif

    m=_mm_set_epi32(0,0,0xff<<24,0);    /* shifting mask */

/* precompute a small table */

    t[0]=_mm_set1_epi32(0);
    xe=_mm_set_epi32(0,x->w[2],0,x->w[0]);
    xo=_mm_set_epi32(0,x->w[3],0,x->w[1]);
    t[1]=_mm_xor_si128(xe,_mm_slli_si128(xo,4));
    xe=_mm_slli_epi64(xe,1);
    xo=_mm_slli_epi64(xo,1);
    t[2]=_mm_xor_si128(xe,_mm_slli_si128(xo,4));
    t[3]=_mm_xor_si128(t[2],t[1]);
    xe=_mm_slli_epi64(xe,1);
    xo=_mm_slli_epi64(xo,1);
    t[4]=_mm_xor_si128(xe,_mm_slli_si128(xo,4));
    t[5]=_mm_xor_si128(t[4],t[1]);
    t[6]=_mm_xor_si128(t[4],t[2]);
    t[7]=_mm_xor_si128(t[4],t[3]);
    xe=_mm_slli_epi64(xe,1);
    xo=_mm_slli_epi64(xo,1);
    t[8]=_mm_xor_si128(xe,_mm_slli_si128(xo,4));
    t[9]=_mm_xor_si128(t[8],t[1]);
    t[10]=_mm_xor_si128(t[8],t[2]);
    t[11]=_mm_xor_si128(t[8],t[3]);
    t[12]=_mm_xor_si128(t[8],t[4]);
    t[13]=_mm_xor_si128(t[8],t[5]);
    t[14]=_mm_xor_si128(t[8],t[6]);
    t[15]=_mm_xor_si128(t[8],t[7]);

    b=y->w[0];

    i=b&0xf; j=(b>>4)&0xf;    r=t[j]; 
    s=_mm_and_si128(r,m);     r=_mm_slli_epi64(r,4);
    s=_mm_slli_si128(s,1);    s=_mm_srli_epi64(s,4);  /* net shift left 4 */
    r=_mm_xor_si128(r,s);     r=_mm_xor_si128(r,t[i]);    
    p=q=r;                    q=_mm_srli_si128(q,1); 

    i=(b>>8)&0xf; j=(b>>12)&0xf; r=t[j]; 
    s=_mm_and_si128(r,m);    r=_mm_slli_epi64(r,4);
    s=_mm_slli_si128(s,1);   s=_mm_srli_epi64(s,4);
    r=_mm_xor_si128(r,s);    r=_mm_xor_si128(r,t[i]);
    q=_mm_xor_si128(q,r);    r=_mm_slli_si128(r,1); 
    p=_mm_xor_si128(p,r);    q=_mm_srli_si128(q,1);

    i=(b>>16)&0xf; j=(b>>20)&0xf; r=t[j]; 
    s=_mm_and_si128(r,m);    r=_mm_slli_epi64(r,4);
    s=_mm_slli_si128(s,1);   s=_mm_srli_epi64(s,4);
    r=_mm_xor_si128(r,s);    r=_mm_xor_si128(r,t[i]);
    q=_mm_xor_si128(q,r);    r=_mm_slli_si128(r,2); 
    p=_mm_xor_si128(p,r);    q=_mm_srli_si128(q,1);

    i=(b>>24)&0xf; j=(b>>28); r=t[j]; 
    s=_mm_and_si128(r,m);    r=_mm_slli_epi64(r,4);
    s=_mm_slli_si128(s,1);   s=_mm_srli_epi64(s,4);
    r=_mm_xor_si128(r,s);    r=_mm_xor_si128(r,t[i]);
    q=_mm_xor_si128(q,r);    r=_mm_slli_si128(r,3); 
    p=_mm_xor_si128(p,r);    q=_mm_srli_si128(q,1);

    b=y->w[1];

    i=(b)&0xf; j=(b>>4)&0xf; r=t[j]; 
    s=_mm_and_si128(r,m);    r=_mm_slli_epi64(r,4);
    s=_mm_slli_si128(s,1);   s=_mm_srli_epi64(s,4);
    r=_mm_xor_si128(r,s);    r=_mm_xor_si128(r,t[i]);
    q=_mm_xor_si128(q,r);    r=_mm_slli_si128(r,4); 
    p=_mm_xor_si128(p,r);    q=_mm_srli_si128(q,1);

    i=(b>>8)&0xf; j=(b>>12)&0xf; r=t[j]; 
    s=_mm_and_si128(r,m);    r=_mm_slli_epi64(r,4);
    s=_mm_slli_si128(s,1);   s=_mm_srli_epi64(s,4);
    r=_mm_xor_si128(r,s);    r=_mm_xor_si128(r,t[i]);
    q=_mm_xor_si128(q,r);    r=_mm_slli_si128(r,5); 
    p=_mm_xor_si128(p,r);    q=_mm_srli_si128(q,1);

    i=(b>>16)&0xf; j=(b>>20)&0xf; r=t[j]; 
    s=_mm_and_si128(r,m);    r=_mm_slli_epi64(r,4);
    s=_mm_slli_si128(s,1);   s=_mm_srli_epi64(s,4);
    r=_mm_xor_si128(r,s);    r=_mm_xor_si128(r,t[i]);
    q=_mm_xor_si128(q,r);    r=_mm_slli_si128(r,6); 
    p=_mm_xor_si128(p,r);    q=_mm_srli_si128(q,1);

    i=(b>>24)&0xf; j=(b>>28); r=t[j]; 
    s=_mm_and_si128(r,m);    r=_mm_slli_epi64(r,4);
    s=_mm_slli_si128(s,1);   s=_mm_srli_epi64(s,4);
    r=_mm_xor_si128(r,s);    r=_mm_xor_si128(r,t[i]);
    q=_mm_xor_si128(q,r);    r=_mm_slli_si128(r,7); 
    p=_mm_xor_si128(p,r);    q=_mm_srli_si128(q,1);

    b=y->w[2];

    i=(b)&0xf; j=(b>>4)&0xf; r=t[j]; 
    s=_mm_and_si128(r,m);    r=_mm_slli_epi64(r,4);
    s=_mm_slli_si128(s,1);   s=_mm_srli_epi64(s,4);
    r=_mm_xor_si128(r,s);    r=_mm_xor_si128(r,t[i]);
    q=_mm_xor_si128(q,r);    r=_mm_slli_si128(r,8); 
    p=_mm_xor_si128(p,r);    q=_mm_srli_si128(q,1);

    i=(b>>8)&0xf; j=(b>>12)&0xf; r=t[j]; 
    s=_mm_and_si128(r,m);    r=_mm_slli_epi64(r,4);
    s=_mm_slli_si128(s,1);   s=_mm_srli_epi64(s,4);
    r=_mm_xor_si128(r,s);    r=_mm_xor_si128(r,t[i]);
    q=_mm_xor_si128(q,r);    r=_mm_slli_si128(r,9); 
    p=_mm_xor_si128(p,r);    q=_mm_srli_si128(q,1);

    i=(b>>16)&0xf; j=(b>>20)&0xf; r=t[j]; 
    s=_mm_and_si128(r,m);    r=_mm_slli_epi64(r,4);
    s=_mm_slli_si128(s,1);   s=_mm_srli_epi64(s,4);
    r=_mm_xor_si128(r,s);    r=_mm_xor_si128(r,t[i]);
    q=_mm_xor_si128(q,r);    r=_mm_slli_si128(r,10); 
    p=_mm_xor_si128(p,r);    q=_mm_srli_si128(q,1);

    i=(b>>24)&0xf; j=(b>>28); r=t[j]; 
    s=_mm_and_si128(r,m);    r=_mm_slli_epi64(r,4);
    s=_mm_slli_si128(s,1);   s=_mm_srli_epi64(s,4);
    r=_mm_xor_si128(r,s);    r=_mm_xor_si128(r,t[i]);
    q=_mm_xor_si128(q,r);    r=_mm_slli_si128(r,11); 
    p=_mm_xor_si128(p,r);    q=_mm_srli_si128(q,1);

    b=y->w[3];

    i=(b)&0xf; j=(b>>4)&0xf; r=t[j]; 
    s=_mm_and_si128(r,m);    r=_mm_slli_epi64(r,4);
    s=_mm_slli_si128(s,1);   s=_mm_srli_epi64(s,4);
    r=_mm_xor_si128(r,s);    r=_mm_xor_si128(r,t[i]);
    q=_mm_xor_si128(q,r);    r=_mm_slli_si128(r,12); 
    p=_mm_xor_si128(p,r);

    q=_mm_srli_si128(q,4);   /* only 103 bits, so we are done */

/* modular reduction - x^103+x^9+1 */

    a0=_mm_movepi64_pi64(p);
    a1=_mm_movepi64_pi64(_mm_srli_si128(p,8));
    a2=_mm_movepi64_pi64(q);
    a3=_mm_movepi64_pi64(_mm_srli_si128(q,8));

    a2=_m_pxor(a2,_m_psrlqi(a3,39));
    a2=_m_pxor(a2,_m_psrlqi(a3,30));
    a1=_m_pxor(a1,_m_psllqi(a3,25));
    a1=_m_pxor(a1,_m_psllqi(a3,34));

    a1=_m_pxor(a1,_m_psrlqi(a2,39));
    a1=_m_pxor(a1,_m_psrlqi(a2,30));
    a0=_m_pxor(a0,_m_psllqi(a2,25));
    a0=_m_pxor(a0,_m_psllqi(a2,34));

    top=_m_psrlqi(a1,39);
    a0=_m_pxor(a0,top);
    top=_m_psllqi(top,39);
    a0=_m_pxor(a0,_m_psrlqi(top,30));
    a1=_m_pxor(a1,top);

    if (w->len>4) zero(w);
    
    w->w[0]=_m_to_int(a0);
    a0=_m_psrlqi(a0,32);
    w->w[1]=_m_to_int(a0);
    w->w[2]=_m_to_int(a1);
    a1=_m_psrlqi(a1,32);
    w->w[3]=_m_to_int(a1);

    w->len=4;
    if (w->w[3]==0) mr_lzero(w);
    _m_empty();
}

#endif

#ifdef SP79

#ifdef __GNUC__
#include <xmmintrin.h>
#else
#include <emmintrin.h>
#endif

void modmult2(_MIPD_ big x,big y,big w)
{
    int i,j;
    mr_small b;

    __m128i t[16];
    __m128i m,r,s,p,q,xe,xo;
    __m64 a2,a1,a0,top;

    if (x==y)
    {
        modsquare2(_MIPP_ x,w);
        return;
    }
#ifdef MR_COUNT_OPS
fpm2++; 
#endif    
    if (x->len==0 || y->len==0)
    {
        zero(w);
        return;
    }

    m=_mm_set_epi32(0,0,0xff<<24,0);    /* shifting mask */

/* precompute a small table */

    t[0]=_mm_set1_epi32(0);
    xe=_mm_set_epi32(0,x->w[2],0,x->w[0]);
    xo=_mm_set_epi32(0,0,0,x->w[1]);
    t[1]=_mm_xor_si128(xe,_mm_slli_si128(xo,4));
    xe=_mm_slli_epi64(xe,1);
    xo=_mm_slli_epi64(xo,1);
    t[2]=_mm_xor_si128(xe,_mm_slli_si128(xo,4));
    t[3]=_mm_xor_si128(t[2],t[1]);
    xe=_mm_slli_epi64(xe,1);
    xo=_mm_slli_epi64(xo,1);
    t[4]=_mm_xor_si128(xe,_mm_slli_si128(xo,4));
    t[5]=_mm_xor_si128(t[4],t[1]);
    t[6]=_mm_xor_si128(t[4],t[2]);
    t[7]=_mm_xor_si128(t[4],t[3]);
    xe=_mm_slli_epi64(xe,1);
    xo=_mm_slli_epi64(xo,1);
    t[8]=_mm_xor_si128(xe,_mm_slli_si128(xo,4));
    t[9]=_mm_xor_si128(t[8],t[1]);
    t[10]=_mm_xor_si128(t[8],t[2]);
    t[11]=_mm_xor_si128(t[8],t[3]);
    t[12]=_mm_xor_si128(t[8],t[4]);
    t[13]=_mm_xor_si128(t[8],t[5]);
    t[14]=_mm_xor_si128(t[8],t[6]);
    t[15]=_mm_xor_si128(t[8],t[7]);

    b=y->w[0];

    i=b&0xf; j=(b>>4)&0xf;    r=t[j]; 
    s=_mm_and_si128(r,m);     r=_mm_slli_epi64(r,4);
    s=_mm_slli_si128(s,1);    s=_mm_srli_epi64(s,4);  /* net shift left 4 */
    r=_mm_xor_si128(r,s);     r=_mm_xor_si128(r,t[i]);    
    p=q=r;                    q=_mm_srli_si128(q,1); 

    i=(b>>8)&0xf; j=(b>>12)&0xf; r=t[j]; 
    s=_mm_and_si128(r,m);    r=_mm_slli_epi64(r,4);
    s=_mm_slli_si128(s,1);   s=_mm_srli_epi64(s,4);
    r=_mm_xor_si128(r,s);    r=_mm_xor_si128(r,t[i]);
    q=_mm_xor_si128(q,r);    r=_mm_slli_si128(r,1); 
    p=_mm_xor_si128(p,r);    q=_mm_srli_si128(q,1);

    i=(b>>16)&0xf; j=(b>>20)&0xf; r=t[j]; 
    s=_mm_and_si128(r,m);    r=_mm_slli_epi64(r,4);
    s=_mm_slli_si128(s,1);   s=_mm_srli_epi64(s,4);
    r=_mm_xor_si128(r,s);    r=_mm_xor_si128(r,t[i]);
    q=_mm_xor_si128(q,r);    r=_mm_slli_si128(r,2); 
    p=_mm_xor_si128(p,r);    q=_mm_srli_si128(q,1);

    i=(b>>24)&0xf; j=(b>>28); r=t[j]; 
    s=_mm_and_si128(r,m);    r=_mm_slli_epi64(r,4);
    s=_mm_slli_si128(s,1);   s=_mm_srli_epi64(s,4);
    r=_mm_xor_si128(r,s);    r=_mm_xor_si128(r,t[i]);
    q=_mm_xor_si128(q,r);    r=_mm_slli_si128(r,3); 
    p=_mm_xor_si128(p,r);    q=_mm_srli_si128(q,1);

    b=y->w[1];

    i=(b)&0xf; j=(b>>4)&0xf; r=t[j]; 
    s=_mm_and_si128(r,m);    r=_mm_slli_epi64(r,4);
    s=_mm_slli_si128(s,1);   s=_mm_srli_epi64(s,4);
    r=_mm_xor_si128(r,s);    r=_mm_xor_si128(r,t[i]);
    q=_mm_xor_si128(q,r);    r=_mm_slli_si128(r,4); 
    p=_mm_xor_si128(p,r);    q=_mm_srli_si128(q,1);

    i=(b>>8)&0xf; j=(b>>12)&0xf; r=t[j]; 
    s=_mm_and_si128(r,m);    r=_mm_slli_epi64(r,4);
    s=_mm_slli_si128(s,1);   s=_mm_srli_epi64(s,4);
    r=_mm_xor_si128(r,s);    r=_mm_xor_si128(r,t[i]);
    q=_mm_xor_si128(q,r);    r=_mm_slli_si128(r,5); 
    p=_mm_xor_si128(p,r);    q=_mm_srli_si128(q,1);

    i=(b>>16)&0xf; j=(b>>20)&0xf; r=t[j]; 
    s=_mm_and_si128(r,m);    r=_mm_slli_epi64(r,4);
    s=_mm_slli_si128(s,1);   s=_mm_srli_epi64(s,4);
    r=_mm_xor_si128(r,s);    r=_mm_xor_si128(r,t[i]);
    q=_mm_xor_si128(q,r);    r=_mm_slli_si128(r,6); 
    p=_mm_xor_si128(p,r);    q=_mm_srli_si128(q,1);

    i=(b>>24)&0xf; j=(b>>28); r=t[j]; 
    s=_mm_and_si128(r,m);    r=_mm_slli_epi64(r,4);
    s=_mm_slli_si128(s,1);   s=_mm_srli_epi64(s,4);
    r=_mm_xor_si128(r,s);    r=_mm_xor_si128(r,t[i]);
    q=_mm_xor_si128(q,r);    r=_mm_slli_si128(r,7); 
    p=_mm_xor_si128(p,r);    q=_mm_srli_si128(q,1);

    b=y->w[2];

    i=(b)&0xf; j=(b>>4)&0xf; r=t[j]; 
    s=_mm_and_si128(r,m);    r=_mm_slli_epi64(r,4);
    s=_mm_slli_si128(s,1);   s=_mm_srli_epi64(s,4);
    r=_mm_xor_si128(r,s);    r=_mm_xor_si128(r,t[i]);
    q=_mm_xor_si128(q,r);    r=_mm_slli_si128(r,8); 
    p=_mm_xor_si128(p,r);    q=_mm_srli_si128(q,1);

    i=(b>>8)&0xf; j=(b>>12)&0xf; r=t[j]; 
    s=_mm_and_si128(r,m);    r=_mm_slli_epi64(r,4);
    s=_mm_slli_si128(s,1);   s=_mm_srli_epi64(s,4);
    r=_mm_xor_si128(r,s);    r=_mm_xor_si128(r,t[i]);
    q=_mm_xor_si128(q,r);    r=_mm_slli_si128(r,9); 
    p=_mm_xor_si128(p,r); 

    q=_mm_srli_si128(q,7);    /* only 79 bits, so we are done */
    
/* modular reduction - x^79+x^9+1 */

    a0=_mm_movepi64_pi64(p);
    a1=_mm_movepi64_pi64(_mm_srli_si128(p,8));
    a2=_mm_movepi64_pi64(q);

    a1=_m_pxor(a1,_m_psrlqi(a2,15));
    a1=_m_pxor(a1,_m_psrlqi(a2,6));
    a0=_m_pxor(a0,_m_psllqi(a2,49));
    a0=_m_pxor(a0,_m_psllqi(a2,58));

    top=_m_psrlqi(a1,15);
    a0=_m_pxor(a0,top);
    top=_m_psllqi(top,15);
    a0=_m_pxor(a0,_m_psrlqi(top,6));
    a1=_m_pxor(a1,top);

    w->w[2]=_m_to_int(a1);

    if (w->len>3)
    { /* Yes I know its crazy, but its needed to fix the broken /O2 optimizer */
        for (i=3;i<w->len;i++) w->w[i]=0;
    }

    w->w[0]=_m_to_int(a0);
    a0=_m_psrlqi(a0,32);
    w->w[1]=_m_to_int(a0);
    
    w->len=3;
    if (w->w[2]==0) mr_lzero(w);
    _m_empty();
}

#endif


#ifndef SP103
#ifndef SP79
/*#ifndef SP271 */

void modmult2(_MIPD_ big x,big y,big w)
{ /* w=x*y mod f */
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

    if (x==NULL || y==NULL)
    {
        zero(w);
        return;
    }

    if (x==y)
    {
        modsquare2(_MIPP_ x,w);
        return;
    }

    if (y->len==0)
    {
        zero(w);
        return;
    }

    if (y->len==1)
    {
        if (y->w[0]==1)
        {
            copy(x,w);
            return;
        }
    }    

#ifdef MR_COUNT_OPS
fpm2++; 
#endif

    multiply2(_MIPP_ x,y,mr_mip->w0);
    reduce2(_MIPP_ mr_mip->w0,mr_mip->w0);
    copy(mr_mip->w0,w);
}

#endif
#endif
/*#endif*/

/* Will be *much* faster if M,A,(B and C) are all odd */
/* This could/should be optimized for a particular irreducible polynomial and fixed A, B and C */

void sqroot2(_MIPD_ big x,big y)
{ 
    int i,M,A,B,C;
    int k,n,h,s,a,aw,ab,bw,bb,cw,cb;
 #if MIRACL != 32
    int mm,j;
 #endif
    mr_small *wk,w,we,wo;
    BOOL slow;
/* Using Harley's trick */
    static const mr_small evens[16]=
    {0,1,4,5,2,3,6,7,8,9,12,13,10,11,14,15};
    static const mr_small odds[16]=
    {0,4,1,5,8,12,9,13,2,6,3,7,10,14,11,15};

#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    M=mr_mip->M;
    A=mr_mip->AA;
    if (A==0)
    {
        mr_berror(_MIPP_ MR_ERR_NO_BASIS);
        return;
    }
    B=mr_mip->BB;
    C=mr_mip->CC;

    slow=FALSE;
    if (B)
    {
        if (M%2!=1 || A%2!=1 || B%2!=1 || C%2!=1) slow=TRUE;
    }
    else
    {
        if (M%2!=1 || A%2!=1) slow=TRUE;
    }

    if (slow)
    {
        copy(x,y);
        for (i=1;i<mr_mip->M;i++)
            modsquare2(_MIPP_ y,y);
        return;
    }

    bb=cb=cw=bw=0;
/* M, A (B and C) are all odd - so use fast
   Fong, Hankerson, Lopez and Menezes method */

    if (x==y)
    {
        copy (x,mr_mip->w0);
        wk=mr_mip->w0->w;
    }
    else
    {
        wk=x->w;
    }
    zero(y);

#if MIRACL==8
    if (M==271 && A==207 && B==175 && C==111)
    {
        y->len=34;
        for (i=0;i<34;i++)
        {
            n=i/2;
            w=wk[i];

            we=evens[((w&0x5)+((w&0x50)>>3))];
            wo=odds[((w&0xA)+((w&0xA0)>>5))];

            i++;
            w=wk[i];

            we|=evens[((w&0x5)+((w&0x50)>>3))]<<4;
            wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<4;
      
            y->w[n]^=we;
            y->w[n+17]=wo;

            y->w[n+13]^=wo;
            y->w[n+11]^=wo;
            y->w[n+7]^=wo;
        }
        if (y->w[33]==0) mr_lzero(y);
        return;
    }
#endif    

#if MIRACL==32
    if (M==1223 && A==255)
    {
        y->len=39;
        for (i=0;i<39;i++)
        {
            n=i/2;
            w=wk[i];

            we=evens[((w&0x5)+((w&0x50)>>3))];
            wo=odds[((w&0xA)+((w&0xA0)>>5))];
            w>>=8;
            we|=evens[((w&0x5)+((w&0x50)>>3))]<<4;
            wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<4;
            w>>=8;
            we|=evens[((w&0x5)+((w&0x50)>>3))]<<8;
            wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<8;
            w>>=8;
            we|=evens[((w&0x5)+((w&0x50)>>3))]<<12;
            wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<12;

            i++;
            if (i<39)
            {
            w=wk[i];

            we|=evens[((w&0x5)+((w&0x50)>>3))]<<16;
            wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<16;
            w>>=8;
            we|=evens[((w&0x5)+((w&0x50)>>3))]<<20;
            wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<20;
            w>>=8;
            we|=evens[((w&0x5)+((w&0x50)>>3))]<<24;
            wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<24;
            w>>=8;
            we|=evens[((w&0x5)+((w&0x50)>>3))]<<28;
            wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<28;
            }
            y->w[n]^=we;

            y->w[20+n-1]^=wo<<4; 
            y->w[20+n]^=wo>>28; 

            y->w[n+4]^=wo;
        }
        if (y->w[38]==0) mr_lzero(y);
        return;
    }

#endif

#if MIRACL==64
    if (M==1223 && A==255)
    {
        y->len=20;
        for (i=0;i<20;i++)
        {
            n=i/2;
            w=wk[i];

            we=evens[((w&0x5)+((w&0x50)>>3))];
            wo=odds[((w&0xA)+((w&0xA0)>>5))];
            w>>=8;
            we|=evens[((w&0x5)+((w&0x50)>>3))]<<4;
            wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<4;
            w>>=8;
            we|=evens[((w&0x5)+((w&0x50)>>3))]<<8;
            wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<8;
            w>>=8;
            we|=evens[((w&0x5)+((w&0x50)>>3))]<<12;
            wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<12;
            w>>=8;
            we|=evens[((w&0x5)+((w&0x50)>>3))]<<16;
            wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<16;
            w>>=8;
            we|=evens[((w&0x5)+((w&0x50)>>3))]<<20;
            wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<20;
            w>>=8;
            we|=evens[((w&0x5)+((w&0x50)>>3))]<<24;
            wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<24;
            w>>=8;
            we|=evens[((w&0x5)+((w&0x50)>>3))]<<28;
            wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<28;

            i++;          
            w=wk[i];

            we|=evens[((w&0x5)+((w&0x50)>>3))]<<32;
            wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<32;
            w>>=8;
            we|=evens[((w&0x5)+((w&0x50)>>3))]<<36;
            wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<36;
            w>>=8;
            we|=evens[((w&0x5)+((w&0x50)>>3))]<<40;
            wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<40;
            w>>=8;
            we|=evens[((w&0x5)+((w&0x50)>>3))]<<44;
            wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<44;
            w>>=8;
            we|=evens[((w&0x5)+((w&0x50)>>3))]<<48;
            wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<48;
            w>>=8;
            we|=evens[((w&0x5)+((w&0x50)>>3))]<<52;
            wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<52;
            w>>=8;
            we|=evens[((w&0x5)+((w&0x50)>>3))]<<56;
            wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<56;
            w>>=8;
            we|=evens[((w&0x5)+((w&0x50)>>3))]<<60;
            wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<60;

            y->w[n]^=we;

            y->w[10+n-1]^=wo<<36; 
            y->w[10+n]^=wo>>28; 

            y->w[n+2]^=wo;
        }
        if (y->w[19]==0) mr_lzero(y);
        return;
    }

#endif

    k=1+(M/MIRACL);
    h=(k+1)/2;

    a=(A+1)/2;
    aw=a/MIRACL;
    ab=a%MIRACL;

    if (B)
    {
        a=(B+1)/2;
        bw=a/MIRACL;
        bb=a%MIRACL;

        a=(C+1)/2;
        cw=a/MIRACL;
        cb=a%MIRACL;
    }
    s=h*MIRACL-1-(M-1)/2;
    
    y->len=k;
    for (i=0;i<k;i++)
    {
        n=i/2;
        w=wk[i];
       
#if MIRACL == 32

        we=evens[((w&0x5)+((w&0x50)>>3))];
        wo=odds[((w&0xA)+((w&0xA0)>>5))];
        w>>=8;

        we|=evens[((w&0x5)+((w&0x50)>>3))]<<4;
        wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<4;
        w>>=8;

        we|=evens[((w&0x5)+((w&0x50)>>3))]<<8;
        wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<8;
        w>>=8;

        we|=evens[((w&0x5)+((w&0x50)>>3))]<<12;
        wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<12;

#else
        mm=0;
        we=wo=0;
        for (j=0;j<MIRACL/8;j++)
        {
            we|=evens[((w&0x5)+((w&0x50)>>3))]<<mm;
            wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<mm;
            mm+=4; w>>=8;
        }

#endif
        i++;
        if (i<k)
        {
            w=wk[i];
#if MIRACL == 32

        we|=evens[((w&0x5)+((w&0x50)>>3))]<<16;
        wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<16;
        w>>=8;

        we|=evens[((w&0x5)+((w&0x50)>>3))]<<20;
        wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<20;
        w>>=8;

        we|=evens[((w&0x5)+((w&0x50)>>3))]<<24;
        wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<24;
        w>>=8;

        we|=evens[((w&0x5)+((w&0x50)>>3))]<<28;
        wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<28;


#else 
        for (j=0;j<MIRACL/8;j++)
        {
            we|=evens[((w&0x5)+((w&0x50)>>3))]<<mm;
            wo|=odds[((w&0xA)+((w&0xA0)>>5))]<<mm;
            mm+=4; w>>=8;
        }

#endif
        }
        y->w[n]^=we; 

        if (s==0) y->w[h+n]=wo;
        else
        {
            y->w[h+n-1]^=wo<<(MIRACL-s); 
            y->w[h+n]^=wo>>s;     /* abutt odd bits to even */
        }
        if (ab==0) y->w[n+aw]^=wo;
        else
        {
            y->w[n+aw]^=wo<<ab; 
            y->w[n+aw+1]^=wo>>(MIRACL-ab);
        }
        if (B)
        {
            if (bb==0) y->w[n+bw]^=wo;
            else
            {
                y->w[n+bw]^=wo<<bb; 
                y->w[n+bw+1]^=wo>>(MIRACL-bb);
            }
            if (cb==0) y->w[n+cw]^=wo;
            else
            {
                y->w[n+cw]^=wo<<cb; 
                y->w[n+cw+1]^=wo>>(MIRACL-cb);
            }
        }
    }

    if (y->w[k-1]==0) mr_lzero(y);
}

#ifndef MR_STATIC

void power2(_MIPD_ big x,int m,big w)
{ /* w=x^m mod f. Could be optimised a lot, but not time critical for me */
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    copy(x,mr_mip->w1);
    
    convert(_MIPP_ 1,w);
    forever
    {
        if (m%2!=0)
            modmult2(_MIPP_ w,mr_mip->w1,w);
        m/=2;
        if (m==0) break;
        modsquare2(_MIPP_ mr_mip->w1,mr_mip->w1);
    }
}

#endif

/* Euclidean Algorithm */

BOOL inverse2(_MIPD_ big x,big w)
{
    mr_small bit;
    int i,j,n,n3,k,n4,mb,mw;
    big t;
    BOOL newword;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (size(x)==0) return FALSE;
 
    convert(_MIPP_ 1,mr_mip->w1);     
    zero(mr_mip->w2);                 
    copy(x,mr_mip->w3);               
    copy(mr_mip->modulus,mr_mip->w4); 

    n3=numbits(mr_mip->w3);
    n4=mr_mip->M+1;

#ifdef MR_COUNT_OPS
fpi2++; 
#endif

    while (n3!=1)
    {
        j=n3-n4;
        
        if (j<0)
        { 
            t=mr_mip->w3; mr_mip->w3=mr_mip->w4; mr_mip->w4=t;
            t=mr_mip->w1; mr_mip->w1=mr_mip->w2; mr_mip->w2=t;
            j=-j; n=n3; n3=n4; n4=n;
        }
       
        mw=j/MIRACL; mb=j%MIRACL;
        
        if (n3<MIRACL)
        {
            mr_mip->w3->w[0]^=mr_mip->w4->w[0]<<mb;
            n3--; 
            
            bit=((mr_small)1<<((n3-1)%MIRACL));

            while (!(mr_mip->w3->w[0]&bit))
            {
                n3--;
                bit>>=1;
            }
        }
        else
        {
            k=mr_mip->w3->len;
            if (mb==0)
            {
                for (i=mw;i<k;i++)
                    mr_mip->w3->w[i]^=mr_mip->w4->w[i-mw];
            }
            else
            {
                mr_mip->w3->w[mw]^=mr_mip->w4->w[0]<<mb;
                for (i=mw+1;i<k;i++)
                    mr_mip->w3->w[i]^=((mr_mip->w4->w[i-mw]<<mb) | (mr_mip->w4->w[i-mw-1]>>(MIRACL-mb)));
            }

            newword=FALSE;
            while (mr_mip->w3->w[k-1]==0) {k--; newword=TRUE;}

/*
    bit=mr_mip->w3->w[k-1];
    ASM mov eax,bit
    ASM bsr ecx,eax
    ASM mov shift,ecx
    n3=(k-1)*MIRACL+shift+1;   
    
*/
            if (newword)
            {           
                bit=TOPBIT;
                n3=k*MIRACL;
            }
            else
            {
                n3--;
                bit=((mr_small)1<<((n3-1)%MIRACL));
            }
            while (!(mr_mip->w3->w[k-1]&bit))
            {
                n3--;
                bit>>=1;         
            }

            mr_mip->w3->len=k;
        }

        k=mr_mip->w2->len+mw+1;
        if ((int)mr_mip->w1->len>k) k=mr_mip->w1->len;
        
        if (mb==0)
        {
            for (i=mw;i<k;i++)
                mr_mip->w1->w[i]^=mr_mip->w2->w[i-mw];
        }
        else
        {
            mr_mip->w1->w[mw]^=mr_mip->w2->w[0]<<mb;
            for (i=mw+1;i<k;i++)
                mr_mip->w1->w[i]^=((mr_mip->w2->w[i-mw]<<mb) | (mr_mip->w2->w[i-mw-1]>>(MIRACL-mb)));
        }  
        while (mr_mip->w1->w[k-1]==0) k--;
        mr_mip->w1->len=k;
    }

    copy(mr_mip->w1,w);
    return TRUE;
}

/* Schroeppel, Orman, O'Malley, Spatscheck    *
 * "Almost Inverse" algorithm, Crypto '95     *
 * More optimization here and in-lining would *
 * speed up AFFINE mode. I observe that       *
 * pentanomials would be more efficient if C  *
 * were greater                               */

/*
BOOL inverse2(_MIPD_ big x,big w)
{
    mr_small lsw,*gw;
    int i,n,bits,step,n3,n4,k;
    int k1,k2,k3,k4,ls1,ls2,ls3,ls4,rs1,rs2,rs3,rs4;
    int M,A,B,C;
    big t;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (size(x)==0) return FALSE;

    M=mr_mip->M;
    A=mr_mip->AA;
	if (A==0)
	{
        mr_berror(_MIPP_ MR_ERR_NO_BASIS);
        return FALSE;
	}
 
    B=mr_mip->BB;
    C=mr_mip->CC;
    convert(_MIPP_ 1,mr_mip->w1);     
    zero(mr_mip->w2);                 
    copy(x,mr_mip->w3);               
    copy(mr_mip->modulus,mr_mip->w4); 

    bits=zerobits(mr_mip->w3);
    shiftrightbits(mr_mip->w3,bits);
    k=bits;    
    n3=numbits(mr_mip->w3);
    n4=M+1;

    if (n3>1) forever
    {
        if (n3<n4)
        { 
            t=mr_mip->w3; mr_mip->w3=mr_mip->w4; mr_mip->w4=t;
            t=mr_mip->w1; mr_mip->w1=mr_mip->w2; mr_mip->w2=t;
            n=n3; n3=n4; n4=n;
        }
        
        add2(mr_mip->w3,mr_mip->w4,mr_mip->w3); 
 
        add2(mr_mip->w1,mr_mip->w2,mr_mip->w1);

        if (n3==n4) n3=numbits(mr_mip->w3);
        bits=zerobits(mr_mip->w3);
        k+=bits;    
        n3-=bits;
        if (n3==1) break;
        shiftrightbits(mr_mip->w3,bits);
        shiftleftbits(mr_mip->w2,bits);
   }

    copy(mr_mip->w1,w);

    if (k==0) 
    {
        mr_lzero(w);
        return TRUE;
    }
    step=MIRACL;

    if (A<MIRACL) step=A;
    
    k1=1+M/MIRACL;  
    rs1=M%MIRACL;
    ls1=MIRACL-rs1;

    k2=1+A/MIRACL;  
    rs2=A%MIRACL;
    ls2=MIRACL-rs2;

    if (B)
    { 
        if (C<MIRACL) step=C;

        k3=1+B/MIRACL;
        rs3=B%MIRACL;
        ls3=MIRACL-rs3;

        k4=1+C/MIRACL;
        rs4=C%MIRACL;
        ls4=MIRACL-rs4;
    }

    gw=w->w;
    while (k>0)
    {
        if (k>step) n=step;
        else        n=k;
 
        if (n==MIRACL) lsw=gw[0];
        else           lsw=gw[0]&(((mr_small)1<<n)-1);

        w->len=k1;
        if (rs1==0) gw[k1-1]^=lsw;
        else
        {
            w->len++;
            gw[k1]^=(lsw>>ls1);
            gw[k1-1]^=(lsw<<rs1);
        }
        if (rs2==0) gw[k2-1]^=lsw;
        else
        {
            gw[k2]^=(lsw>>ls2);
            gw[k2-1]^=(lsw<<rs2);
        }
        if (B)
        {
            if (rs3==0) gw[k3-1]^=lsw;
            else
            {
                gw[k3]^=(lsw>>ls3);
                gw[k3-1]^=(lsw<<rs3);
            }
            if (rs4==0) gw[k4-1]^=lsw;
            else
            {
                gw[k4]^=(lsw>>ls4);
                gw[k4-1]^=(lsw<<rs4);
            }
        }
        shiftrightbits(w,n);
        k-=n;
    }
    mr_lzero(w);
    return TRUE;
}

*/

BOOL multi_inverse2(_MIPD_ int m,big *x,big *w)
{ /* find w[i]=1/x[i] mod f, for i=0 to m-1 */
    int i;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (m==0) return TRUE;
    if (m<0) return FALSE;

    if (x==w)
    {
        mr_berror(_MIPP_ MR_ERR_BAD_PARAMETERS);
        return FALSE;
    }
    if (m==1)
    {
        inverse2(_MIPP_ x[0],w[0]);
        return TRUE;
    }
    convert(_MIPP_ 1,w[0]);
    copy(x[0],w[1]);
    for (i=2;i<m;i++)
        modmult2(_MIPP_ w[i-1],x[i-1],w[i]);

    modmult2(_MIPP_ w[m-1],x[m-1],mr_mip->w6);
    if (size(mr_mip->w6)==0)
    {
        mr_berror(_MIPP_ MR_ERR_DIV_BY_ZERO);
        return FALSE;
    }

    inverse2(_MIPP_ mr_mip->w6,mr_mip->w6);  /* y=1/y */

    copy(x[m-1],mr_mip->w5);
    modmult2(_MIPP_ w[m-1],mr_mip->w6,w[m-1]);

    for (i=m-2;;i--)
    {
        if (i==0)
        {
            modmult2(_MIPP_ mr_mip->w5,mr_mip->w6,w[0]);
            break;
        }
        modmult2(_MIPP_ w[i],mr_mip->w5,w[i]);
        modmult2(_MIPP_ w[i],mr_mip->w6,w[i]);
        modmult2(_MIPP_ mr_mip->w5,x[i],mr_mip->w5);
    }
    return TRUE;
}

#ifndef MR_STATIC

int trace2(_MIPD_ big x)
{
    int i;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    copy(x,mr_mip->w1);
    for (i=1;i<mr_mip->M;i++)
    {
        modsquare2(_MIPP_ mr_mip->w1,mr_mip->w1);
        add2(mr_mip->w1,x,mr_mip->w1);
    }   
    return (int)(mr_mip->w1->w[0]&1);
}

#endif

#ifndef MR_NO_RAND

void rand2(_MIPD_ big x)
{ /* random number */
    int i,k;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    zero(x);
    k=1+mr_mip->M/MIRACL;        
    x->len=k;
    for (i=0;i<k;i++) x->w[i]=brand(_MIPPO_ );
    mr_lzero(x);
    reduce2(_MIPP_ x,x);    
}

#endif

int parity2(big x)
{ /* return LSB */
   if (x->len==0) return 0;
   return (int)(x->w[0]%2);
}

void halftrace2(_MIPD_ big b,big w)
{
    int i,M;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

    M=mr_mip->M;
    if (M%2==0) return;
   
    copy(b,mr_mip->w1);
    copy(b,w);
    
    for (i=1;i<=(M-1)/2;i++)
    { 
        modsquare2(_MIPP_ w,w);
        modsquare2(_MIPP_ w,w);
        add2(w,mr_mip->w1,w);   
    } 
}

BOOL quad2(_MIPD_ big b,big w)
{ /* Solves x^2 + x = b  for a root w  *
   * returns TRUE if a solution exists *
   * the "other" solution is w+1       */
    int i,M;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

    M=mr_mip->M;
    copy(b,mr_mip->w1);
    if (M%2==1) halftrace2(_MIPP_ b,w); /* M is odd, so its the Half-Trace */

    else
    {
        zero(mr_mip->w2);
        forever
        {
#ifndef MR_NO_RAND
            rand2(_MIPP_ mr_mip->w2);
#else
            incr(_MIPP_ mr_mip->w2,1,mr_mip->w2);
#endif
            zero(w);
            copy(mr_mip->w2,mr_mip->w3);
            for (i=1;i<M;i++)
            {
                modsquare2(_MIPP_ mr_mip->w3,mr_mip->w3);
                modmult2(_MIPP_ mr_mip->w3,mr_mip->w1,mr_mip->w4);
                modsquare2(_MIPP_ w,w);
                add2(w,mr_mip->w4,w);
                add2(mr_mip->w3,mr_mip->w2,mr_mip->w3);
            }    
            if (size(mr_mip->w3)!=0) break; 
        }
    }

    copy(w,mr_mip->w2);
    modsquare2(_MIPP_ mr_mip->w2,mr_mip->w2);
    add2(mr_mip->w2,w,mr_mip->w2);
    if (mr_compare(mr_mip->w1,mr_mip->w2)==0) return TRUE;
    return FALSE;
}

#ifndef MR_STATIC

void gf2m_dotprod(_MIPD_ int n,big *x,big *y,big w)
{ /* dot product - only one reduction! */
    int i;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    mr_mip->check=OFF;
    zero(mr_mip->w5);

    for (i=0;i<n;i++)
    {
        multiply2(_MIPP_ x[i],y[i],mr_mip->w0);
        add2(mr_mip->w5,mr_mip->w0,mr_mip->w5);
    }

    reduce2(_MIPP_ mr_mip->w5,mr_mip->w5);
    copy(mr_mip->w5,w);

    mr_mip->check=ON;
}

#endif

BOOL prepare_basis(_MIPD_ int m,int a,int b,int c,BOOL check)
{
    int i,k,sh;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return FALSE;

    if (b==0) c=0;
    if (m==mr_mip->M && a==mr_mip->AA && b==mr_mip->BB && c==mr_mip->CC)
        return TRUE;   /* its already prepared... */

    MR_IN(138)
    if (m <=0 || a<=0 || a>=m || b>=a) 
    {
        mr_berror(_MIPP_ MR_ERR_BAD_MODULUS);
        MR_OUT
        return FALSE;
    }
    
    mr_mip->M=m;
    mr_mip->AA=a;
    mr_mip->BB=0;
    mr_mip->CC=0;
    zero(mr_mip->modulus);
    convert(_MIPP_ 1,mr_mip->one);

    k=1+m/MIRACL;

    if (k>mr_mip->nib)
    {
        mr_berror(_MIPP_ MR_ERR_OVERFLOW);
        MR_OUT
        return FALSE;
    }

    mr_mip->modulus->len=k;
    sh=m%MIRACL;
    mr_mip->modulus->w[k-1]=((mr_small)1<<sh);
    mr_mip->modulus->w[0]^=1;
    mr_mip->modulus->w[a/MIRACL]^=((mr_small)1<<(a%MIRACL));
    if (b!=0)
    {
         mr_mip->BB=b;
         mr_mip->CC=c;
         mr_mip->modulus->w[b/MIRACL]^=((mr_small)1<<(b%MIRACL));
         mr_mip->modulus->w[c/MIRACL]^=((mr_small)1<<(c%MIRACL));
    }

    if (!check)
    {
        MR_OUT
        return TRUE;
    }

/* check for irreducibility of basis */

    zero(mr_mip->w4);
    mr_mip->w4->len=1;
    mr_mip->w4->w[0]=2;       /* f(t) = t */
    for (i=1;i<=m/2;i++)
    {
        modsquare2(_MIPP_ mr_mip->w4,mr_mip->w4);
        incr2(mr_mip->w4,2,mr_mip->w5);
        gcd2(_MIPP_ mr_mip->w5,mr_mip->modulus,mr_mip->w6);
        if (size(mr_mip->w6)!=1)
        {
            mr_berror(_MIPP_ MR_ERR_NOT_IRREDUC);
            MR_OUT
            return FALSE;
        }
    }
                   
    MR_OUT
    return TRUE;
}

#endif
