
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
 *   Many processors support a floating-point coprocessor, which may
 *   implement a faster multiplication instruction than the corresponding
 *   integer instruction. This is the case for the Pentium processor
 *   which has a built-in co-processor. This can be exploited to give even 
 *   faster performance.
 *   
 *   Note that since the partial products are accumulated in a 64-bit register
 *   this implies that a full-width number base (2^32) cannot be used. 
 *   The maximum number base that can be used is 2^x where x is
 *   calculated such that 2^(64-2*x) > 2*WORDS_IN_MODULUS. This means that 
 *   x will usually be 28 or 29
 *
 *   To use this code:-
 *
 *   (1) Implemented and tested only for the Pentium processor and
 *       using the Borland C compiler (BCC and BCC32)
 *   
 *   (2) Define MR_PENTIUM in mirdef.h. Determine the maximum modulus to be
 *       used, and from that determine the value of WORDS_IN_MODULUS.
 *       
 *   (3) Use as a number base the value of x calculated as shown above.
 *       For example, for 512 bit exponentiation, WORDS_IN_MODULUS will be 18 
 *       so call mirsys(50,536870912L) in your main program.
 *       (Observe that 536870912 = 2^29, and that 18*29 = 522, big enough 
 *       for 512 bit calculations).
 *
 *   (4) Use Montgomery representation when implementing your crypto-system
 *       e.g. use monty_powmod(). This will automatically call the 
 *       routines in this module.
 *
 *   Note that it is *VITAL* that double arrays be aligned on 8-byte 
 *   boundaries for the Pentium. The Borland C compiler does *not* do this 
 *   automatically!!!!
 *
 *   Many thanks are due to Paul Rubin, who suggested to me that this approach
 *   might be faster than the all-integer methods described elsewhere.
 *
 *   Further speed increases can be acheived by loop-unrolling. Completely
 *   unrolled code (a la Comba) has been experimented with, and gives a
 *   25% speed-up in some cases. Note that the basic code for a single partial
 *   product takes only 3 cycles. 
 *
 *          fld ...       ;1 cycle
 *          fmul ...      ;1 cycle
 *          fxch st(2)    ;0 cycle
 *          fadd          ;1 cycle
 *
 *   Compare this with the integer "mul" instruction which takes 10 cycles
 *   on a Pentium
 *
 *   Interestingly the fmul is faster than the fimul. So paradoxically it is
 *   quicker to manipulate 64-bit doubles than it is to manipulate 32-bit 
 *   integers. Clearly the Pentium FP processor has been optimised for real 
 *   arithmetic. However this requires us to convert all bigs from integer
 *   arrays to double arrays (see mrmonty.c) which is very wasteful of space
 *   and rather awkward.
 *
 *
 *   The FP stack is primed in prepare_monty() :-
 *   magic  - (2^63+2^62)*base. By adding and then subtracting this number we
 *            get the top half of the sum.               
 *   1/base - Inverse of the number base
 *   ndash  - Montgomery's constant
 *
 */

#include "miracl.h"

#ifdef MR_PENTIUM
#define N 8
#define POINTER QWORD PTR  
  
#if INLINE_ASM == 1    
#define PAX ax
#define PBP bp   
#define PBX bx   
#define PSI si   
#define PDI di   
#define PCX cx
#define PDX dx
#endif   
 
#if INLINE_ASM == 2    
#define PAX ax 
#define PBP bp   
#define PBX bx   
#define PSI si   
#define PDI di   
#define PCX cx
#define PDX dx
#endif           

#if INLINE_ASM == 3    
#define PAX eax
#define PBP ebp   
#define PBX ebx   
#define PSI esi   
#define PDI edi   
#define PCX ecx
#define PDX edx
#endif           
  
#ifdef INLINE_ASM
#ifndef MR_LMM
                 /* not implemented for large memory model 16 bit */
  
void fastmodmult(_MIPD_ big x,big y,big z)
{
    int ij,rn,nrn;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    big modulus=mr_mip->modulus;
    big w0=mr_mip->w0;
    mr_small *wg,*mg,*xg,*yg;
    wg=w0->w;
    mg=modulus->w;
    xg=x->w;
    yg=y->w;
    rn=(int)modulus->len;    
    for (ij=2*rn;ij<(int)(w0->len&MR_OBITS);ij++) w0->w[ij]=0.0;
    w0->len=2*rn;
    nrn=N*rn;

 ASM    push PBP 
 ASM    push PDI
 ASM    push PSI
 
 ASM    mov PBX,xg  
 ASM    mov PSI,yg  
 ASM    mov PDX,mg
 ASM    mov PDI,wg   
 ASM    mov PAX,nrn
  
 ASM    mov PBP,N
 
 ASM    fldz

 ASM    xor  PCX,PCX
     m1:   
 ASM         push  PCX
 ASM         add   PSI,PCX
 ASM         fld  POINTER [PBX]
 ASM         fmul POINTER [PSI]
 ASM         add   PBX,PBP
 ASM         sub   PSI,PBP
 ASM         test   PCX,PCX
 ASM         jz m3
     m2:
 ASM             fld  POINTER [PBX]
 ASM             fmul POINTER [PSI]
 ASM             fxch  st(2)
 ASM             add   PBX,PBP
 ASM             sub   PSI,PBP
 ASM             fadd
 ASM             sub   PCX,PBP
 ASM             jnz   m2
     m3:
 ASM         sub   PBX,PBP
 ASM         add   PSI,PBP
 ASM         fadd
 ASM         pop  PCX             
 ASM         sub  PBX,PCX         /* restore PBX */

 ASM         xchg PSI,PDX         /* PSI -> modulus */
 ASM         push PCX
 ASM         test  PCX,PCX
 ASM         jz   m6
 ASM             add PSI,PCX
 ASM             fld  POINTER [PDI]
 ASM             fmul POINTER [PSI]
 ASM             add PDI,PBP
 ASM             sub PSI,PBP
 ASM             sub PCX,PBP
 ASM             jz m5
     m4:  /* this is typical of the critical inner loop */
 ASM                 fld  POINTER [PDI]     /* 1 cycle  */
 ASM                 fmul POINTER [PSI]     /* 1 cycle  */
 ASM                 fxch  st(2)            /* 0 cycle  */
 ASM                 add PDI,PBP            /* 1 cycle  */
 ASM                 sub PSI,PBP            /* 0 cycle  */
 ASM                 fadd                   /* 1 cycle  */
 ASM                 sub PCX,PBP            /* 1 cycle  */
 ASM                 jnz m4                 /* 0 cycle  */
                                    /* total = 5 cycles */
      m5:
 ASM             fadd
      m6:

 ASM         fld  st(0)
 ASM         fadd st,st(2)
 ASM         fsub st,st(2)
 ASM         fsubr st,st(1)      
 ASM         fmul st,st(4)
 ASM         fld  st(0)
 ASM         fadd st,st(3)
 ASM         fsub st,st(3)
 ASM         fsub
 ASM         fst  POINTER [PDI]
 ASM         fmul POINTER [PSI]
 ASM         fadd
 ASM         fmul st,st(2)   

 ASM         xchg  PSI,PDX
 ASM         pop   PCX
 ASM         sub   PDI,PCX       /* restore PDI */


 ASM         add   PCX,PBP       /* increment PCX */
 ASM         cmp   PCX,PAX
 ASM         jl    m1
 
 ASM         sub   PCX,PBP       /* PCX=12 */
 ASM         add   PSI,PCX
 ASM         add   PBX,PCX       /* PBX -> x[4] */
 ASM         add   PDX,PCX
 ASM         add   PDI,PCX
 ASM         sub   PCX,PBP       /* going back down again PCX=8 */
     m7:
 ASM         push  PCX
 ASM         sub   PBX,PCX
 ASM         fld  POINTER [PBX]
 ASM         fmul POINTER [PSI]
 ASM         add PBX,PBP
 ASM         sub PSI,PBP
 ASM         test   PCX,PCX
 ASM         jz m9
     m8:
 ASM             fld  POINTER [PBX]
 ASM             fmul POINTER [PSI]
 ASM             fxch  st(2)
 ASM             add PBX,PBP
 ASM             sub PSI,PBP
 ASM             fadd
 ASM             sub  PCX,PBP
 ASM             jnz  m8
      m9:
 ASM         sub  PBX,PBP
 ASM         add  PSI,PBP
 ASM         fadd
 ASM         pop  PCX
 ASM         add  PSI,PCX     /* restore PSI */

 ASM         sub   PDI,PCX
 ASM         xchg PSI,PDX     /* PSI -> modulus */
 ASM         push PCX
 ASM         fld  POINTER [PDI]
 ASM         fmul POINTER [PSI]
 ASM         add PDI,PBP
 ASM         sub PSI,PBP
 ASM         test  PCX,PCX
 ASM         jz   m11
     m10:
 ASM             fld  POINTER [PDI]
 ASM             fmul POINTER [PSI]
 ASM             fxch  st(2)
 ASM             add PDI,PBP
 ASM             sub PSI,PBP
 ASM             fadd
 ASM             sub PCX,PBP
 ASM             jnz m10
     m11:
 ASM         sub  PDI,PBP
 ASM         add  PSI,PBP
 ASM         fadd
 ASM         pop   PCX 
 ASM         add   PSI,PCX     /* restore PSI */
 ASM         xchg  PSI,PDX

 ASM         push  PDI
 ASM         add   PDI,PAX
 ASM         sub   PDI,PCX
 ASM         sub   PDI,PBP


 ASM         fld st(0)
 ASM         fadd st,st(2)
 ASM         fsub st,st(2)
 ASM         fst st(5)
 ASM         fmul st,st(3)  
 ASM         fxch st(5)
 ASM         fsub
 ASM         fstp POINTER [PDI]
 ASM         fld st(3)             

 ASM         pop   PDI

 ASM         sub   PCX,PBP
 ASM         jge   m7
 
 ASM    add PDI,PAX

 ASM    fld st(0)
 ASM    fadd st,st(2)
 ASM    fsub st,st(2)
 ASM    fst st(5)
 ASM    fmul st,st(3)  
 ASM    fxch st(5)
 ASM    fsub
 ASM    fstp POINTER [PDI]
 ASM    fld st(3)             

 ASM    add PDI,PBP
 ASM    fstp POINTER [PDI]

 ASM    pop PSI
 ASM    pop PDI
 ASM    pop PBP

    for (ij=rn;ij<(int)(z->len&MR_OBITS);ij++) z->w[ij]=0.0;
    z->len=rn;
    for (ij=0;ij<rn;ij++) z->w[ij]=w0->w[ij+rn];
    if (z->w[rn-1]==0.0) mr_lzero(z);

} 

void fastmodsquare(_MIPD_ big x,big z)
{
    int ij,rn,nrn;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    big modulus=mr_mip->modulus;
    big w0=mr_mip->w0;
    mr_small *wg,*mg,*xg;
    wg=w0->w;
    mg=modulus->w;
    xg=x->w;

    rn=(int)modulus->len;    
    for (ij=2*rn;ij<(int)(w0->len&MR_OBITS);ij++) w0->w[ij]=0.0;
    w0->len=2*rn;
    nrn=N*rn;


 ASM    push PBP     
 ASM    push PDI
 ASM    push PSI

 ASM    mov PBX,xg  
 ASM    mov PSI,xg
 ASM    mov PDX,mg
 ASM    mov PDI,wg
 ASM    mov PAX,nrn   
   
 ASM    mov PBP,N               

 ASM    fldz

 ASM    xor  PCX,PCX
     s1:   
 ASM         push  PBX
 ASM         push  PSI
 ASM         test   PCX,PCX
 ASM         jz s4
 ASM             add PSI,PCX
 ASM             fstp st(5)
 ASM             fldz
 ASM             fld  POINTER [PBX]
 ASM             fmul POINTER [PSI]
 ASM             sub   PSI,PBP
 ASM             add   PBX,PBP
 ASM             cmp   PSI,PBX
 ASM             jle s3
     s2:
 ASM                 fld  POINTER [PBX]
 ASM                 fmul POINTER [PSI]
 ASM                 fxch st(2)
 ASM                 sub PSI,PBP
 ASM                 add PBX,PBP
 ASM                 fadd
 ASM                 cmp PSI,PBX
 ASM                 jg s2
     s3:
 ASM             fadd
 ASM             fld st(0)
 ASM             fadd
 ASM             fadd st,st(5)
     s4:        
 ASM         cmp PSI,PBX
 ASM         jne s5
 ASM             fld POINTER [PBX]
 ASM             fmul st,st(0)
 ASM             fadd
     s5:  
 ASM         pop  PSI
 ASM         pop  PBX     /* restore pointers */

 ASM         xchg PSI,PDX         /* PSI -> modulus */
 ASM         push PCX
 ASM         test  PCX,PCX
 ASM         jz   s8
 ASM             add PSI,PCX
 ASM             fld  POINTER [PDI]
 ASM             fmul POINTER [PSI]
 ASM             add PDI,PBP
 ASM             sub PSI,PBP
 ASM             sub PCX,PBP
 ASM             jz s7
     s6:
 ASM                 fld  POINTER [PDI]
 ASM                 fmul POINTER [PSI]
 ASM                 fxch  st(2)
 ASM                 add PDI,PBP
 ASM                 sub PSI,PBP
 ASM                 fadd
 ASM                 sub PCX,PBP
 ASM                 jnz s6
     s7:
 ASM             fadd
     s8:

 ASM         fld  st(0)
 ASM         fadd st,st(2)
 ASM         fsub st,st(2)
 ASM         fsubr st,st(1)      
 ASM         fmul st,st(4)
 ASM         fld  st(0)
 ASM         fadd st,st(3)
 ASM         fsub st,st(3)
 ASM         fsub
 ASM         fst  POINTER [PDI]
 ASM         fmul POINTER [PSI]
 ASM         fadd
 ASM         fmul st,st(2)   

 ASM         xchg  PSI,PDX
 ASM         pop   PCX
 ASM         sub   PDI,PCX       /* restore PDI */

 ASM         add   PCX,PBP       /* increment PCX */
 ASM         cmp   PCX,PAX
 ASM         jl    s1
 
 ASM         sub   PCX,PBP       /* PCX=12 */
 ASM         add   PSI,PCX
 ASM         add   PBX,PCX       /* PBX -> x[4] */
 ASM         add   PDX,PCX
 ASM         add   PDI,PCX
 ASM         sub   PCX,PBP       /* going back down again PCX=8 */
     s9:
 ASM         push  PBX
 ASM         push  PSI
 ASM         test   PCX,PCX
 ASM         jz s13
     s10:
 ASM             sub PBX,PCX
 ASM             fstp st(5)
 ASM             fldz
 ASM             fld  POINTER [PBX]
 ASM             fmul POINTER [PSI]
 ASM             sub PSI,PBP
 ASM             add PBX,PBP
 ASM             cmp PSI,PBX
 ASM             jle s12
     s11:
 ASM                 fld  POINTER [PBX]
 ASM                 fmul POINTER [PSI]
 ASM                 fxch st(2)
 ASM                 sub PSI,PBP
 ASM                 add PBX,PBP
 ASM                 fadd
 ASM                 cmp PSI,PBX
 ASM                 jg s11
     s12:
 ASM             fadd
 ASM             fld st(0)
 ASM             fadd
 ASM             fadd st,st(5)
     s13:
 ASM         cmp PSI,PBX
 ASM         jne s14
 ASM             fld POINTER [PBX]
 ASM             fmul st,st(0)
 ASM             fadd
     s14:
 ASM         pop  PSI
 ASM         pop  PBX

 ASM         sub   PDI,PCX
 ASM         xchg PSI,PDX     /* PSI -> modulus */
 ASM         push PCX
 ASM         fld  POINTER [PDI]
 ASM         fmul POINTER [PSI]
 ASM         add PDI,PBP
 ASM         sub PSI,PBP
 ASM         test  PCX,PCX
 ASM         jz   s16
     s15:
 ASM             fld  POINTER [PDI]
 ASM             fmul POINTER [PSI]
 ASM             fxch  st(2)
 ASM             add PDI,PBP
 ASM             sub PSI,PBP
 ASM             fadd
 ASM             sub PCX,PBP
 ASM             jnz s15
     s16:
 ASM         sub  PDI,PBP
 ASM         add  PSI,PBP
 ASM         fadd
 ASM         pop   PCX 
 ASM         add   PSI,PCX     /* restore PSI */
 ASM         xchg  PSI,PDX

 ASM         push  PDI
 ASM         add   PDI,PAX
 ASM         sub   PDI,PCX
 ASM         sub   PDI,PBP

 ASM         fld st(0)
 ASM         fadd st,st(2)
 ASM         fsub st,st(2)
 ASM         fst st(5)
 ASM         fmul st,st(3)  
 ASM         fxch st(5)
 ASM         fsub
 ASM         fstp POINTER [PDI]
 ASM         fld st(3)             

 ASM         pop   PDI

 ASM         sub   PCX,PBP
 ASM         jge   s9
 
 ASM    add PDI,PAX

 ASM    fld st(0)
 ASM    fadd st,st(2)
 ASM    fsub st,st(2)
 ASM    fst st(5)
 ASM    fmul st,st(3)  
 ASM    fxch st(5)
 ASM    fsub
 ASM    fstp POINTER [PDI]
 ASM    fld st(3)             

 ASM    add PDI,PBP
 ASM    fstp POINTER [PDI]

 ASM    pop PSI
 ASM    pop PDI
 ASM    pop PBP     


    for (ij=rn;ij<(int)(z->len&MR_OBITS);ij++) z->w[ij]=0.0;
    z->len=rn;
    for (ij=0;ij<rn;ij++) z->w[ij]=w0->w[ij+rn];
    if (z->w[rn-1]==0.0) mr_lzero(z);
} 
 
#endif
#endif
#endif


