
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
 *   integer instruction. This is the case for the 80486/Pentium processor
 *   which has a built-in co-processor. This can be exploited to give even 
 *   faster performance.
 *
 *   As before the fixed modulus size to be used is pre-defined as 
 *   MR_PENTIUM in mirdef.h
 *   
 *   Note that since the partial products are accumulated in a 64-bit register
 *   this implies that a full-width number base (2^32) cannot be used. 
 *   The maximum number base that can be used is 2^x where x is
 *   calculated such that 2^(64-2*x) > 2*MR_PENTIUM. This means that 
 *   x will usually be 28 or 29
 *
 *   To use this code:-
 *   
 *   (1) Define MR_PENTIUM in mirdef.h to the fixed size of the modulus
 *       
 *   (2) Use as a number base the value of x calculated as shown above
 *       For example, for 512 bit exponentiation, #define MR_PENTIUM 18 
 *       in mirdef.h and call mirsys(50,536870912L) in your main program.
 *       (Observe that 536870912 = 2^29, and that 18*29 = 522, big enough 
 *       for 512 bit calculations).
 *
 *   (3) Use Montgomery representation when implementing your crypto-system
 *       i.e. use monty_powmod(). This will automatically call the 
 *       routines in this module.
 *
 *   Note that this module generates a *lot* of code e.g. > 49kbytes for 
 *   MR_PENTIUM = 36. Compile using -B switch - you will need 
 *   the TASM macro-assembler. If out-of-memory, try using the TASMX /ml 
 *   version of the assembler.
 *
 *   Note that it is *VITAL* that double arrays be aligned on 8-byte 
 *   boundaries for maximum speed on a Pentium. 
 *
 *   Many thanks are due to Paul Rubin, who suggested to me that this approach
 *   might be faster than the all-integer method described elsewhere.
 *
 *   The FP stack is primed in prepare_monty() :-
 *   magic  - (2^63+2^62)*base. By adding and then subtracting this number we
 *            get the top half of the sum.               
 *   1/base - Inverse of the number base
 *   ndash  - Montgomery's constant
 */

#include "miracl.h"

#ifdef MR_PENTIUM
  
#if INLINE_ASM == 1    
#define N 8
#define POINTER QWORD PTR  
#define PBX bx   
#define PSI si   
#define PDI di   
#define PCX cx
#endif   
 
#if INLINE_ASM == 2    
#define N 8
#define POINTER QWORD PTR 
#define PBX bx   
#define PSI si   
#define PDI di   
#define PCX cx
#endif           
  
#if INLINE_ASM == 3    
#define N 8
#define POINTER QWORD PTR   
#define PBX ebx   
#define PSI esi   
#define PDI edi   
#define PCX ecx
#endif           

#ifdef INLINE_ASM
#ifndef MR_LMM
                /* not implemented for large memory model 16 bit */  

void fastmodmult(_MIPD_ big x,big y,big z)
{
    int ij;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    big w0=mr_mip->w0;
    big modulus=mr_mip->modulus;
    mr_small *wg,*mg,*xg,*yg;
    wg=w0->w;
    mg=modulus->w;
    xg=x->w;
    yg=y->w;

    for (ij=2*MR_PENTIUM;ij<(int)(w0->len&MR_OBITS);ij++) w0->w[ij]=0.0;
    w0->len=2*MR_PENTIUM;

    ASM  
    { 
     FSTEP MACRO i,j  
/* some fancy Pentium scheduling going on here ... */
       fld  POINTER [PBX+N*i]
       fmul POINTER [PSI+N*j]
       fxch st(2)
       fadd           
     ENDM         

     FRSTEP MACRO i,j  
       fld  POINTER [PDI+N*i]
       fmul POINTER [PSI+N*j]
       fxch st(2)
       fadd
     ENDM         

     FDSTEP MACRO i,j  
       fld  POINTER [PBX+N*i]
       fmul POINTER [PBX+N*j]
       fxch st(2)
       fadd
     ENDM          

     SELF MACRO k 
       fld POINTER [PBX+N*k]
       fmul st,st(0)
       fadd
     ENDM          

     RFINU MACRO k  
       fld  st(0)

       fadd st,st(2)
       fsub st,st(2)

       fsubr st,st(1)      
       fmul st,st(4)
       fld  st(0)

       fadd st,st(3)
       fsub st,st(3)

       fsub
       fst  POINTER [PDI+N*k]
       fmul POINTER [PSI]
       fadd
       fmul st,st(2)   
     ENDM           

     RFIND MACRO k  
       fld st(0)

       fadd st,st(2)
       fsub st,st(2)

       fsub st(1),st
       fmul st,st(3)
       fxch st(1)
       fstp POINTER [PDI+N*k]

     ENDM           

     DIAG MACRO ns,ne  
       CNT1=ns         
       CNT2=ne
       fld  POINTER [PBX+N*CNT1]
       fmul POINTER [PSI+N*CNT2]        
       CNT1=CNT1+1   
       CNT2=CNT2-1   
       WHILE CNT1 LE ne 
           FSTEP CNT1,CNT2 
           CNT1=CNT1+1   
           CNT2=CNT2-1   
       ENDM 
       fadd   
     ENDM 

     SDIAG MACRO ns,ne  
        CNT1=ns  
        CNT2=ne 
        IF CNT1 LT CNT2
            fstp st(5)   /* store carry */
            fldz    
            fld  POINTER [PBX+N*CNT1]
            fmul POINTER [PBX+N*CNT2]  
            CNT1=CNT1+1
            CNT2=CNT2-1
            WHILE CNT1 LT CNT2  
                FDSTEP CNT1,CNT2 
                CNT1=CNT1+1     
                CNT2=CNT2-1     
            ENDM
            fadd 
            fld st(0)      /* now double it ... */
            fadd   
            fadd st,st(5)  /* add in carry */
        ENDIF   
     ENDM 

     RDIAGU MACRO ns,ne 
        CNT1=ns  
        CNT2=ne 
        IF CNT1 LT ne 
           fld  POINTER [PDI+N*CNT1]
           fmul POINTER [PSI+N*CNT2]        
           CNT1=CNT1+1
           CNT2=CNT2-1         
           WHILE CNT1 LT ne   
               FRSTEP CNT1,CNT2 
               CNT1=CNT1+1    
               CNT2=CNT2-1    
           ENDM 
           fadd
         ENDIF
     ENDM

     RDIAGD MACRO ns,ne  
       CNT1=ns         
       CNT2=ne
       fld  POINTER [PDI+N*CNT1]
       fmul POINTER [PSI+N*CNT2]        
       CNT1=CNT1+1
       CNT2=CNT2-1         
       WHILE CNT1 LE ne 
           FRSTEP CNT1,CNT2 
           CNT1=CNT1+1   
           CNT2=CNT2-1   
       ENDM    
       fadd
     ENDM 
 
     MODMULT MACRO
        CNT=0
        WHILE CNT LT MR_PENTIUM
            DIAG 0,CNT
            xchg PSI,PCX
            RDIAGU 0,CNT
            RFINU CNT
            xchg PSI,PCX
            CNT=CNT+1
        ENDM
        SCNT=0
        WHILE SCNT LT (MR_PENTIUM-1)
            SCNT=SCNT+1
            DIAG SCNT,(MR_PENTIUM-1)
            xchg PSI,PCX
            RDIAGD SCNT,(MR_PENTIUM-1)
            RFIND CNT
            xchg PSI,PCX
            CNT=CNT+1
        ENDM
        RFIND CNT
        CNT=CNT+1
        fstp POINTER [PDI+N*CNT]
     ENDM

     MODSQUARE MACRO
        CNT=0
        WHILE CNT LT MR_PENTIUM
            SDIAG 0,CNT
            IF (CNT MOD 2) EQ 0
                SELF (CNT/2)
            ENDIF
            RDIAGU 0,CNT
            RFINU CNT
            CNT=CNT+1
        ENDM
        SCNT=0
        WHILE SCNT LT (MR_PENTIUM-1)
            SCNT=SCNT+1
            SDIAG SCNT,(MR_PENTIUM-1)
            IF (CNT MOD 2) EQ 0
                SELF (CNT/2)
            ENDIF
            RDIAGD SCNT,(MR_PENTIUM-1)
            RFIND CNT
            CNT=CNT+1
        ENDM
        RFIND CNT
        CNT=CNT+1
        fstp POINTER [PDI+N*CNT]
     ENDM
    }
    ASM
    {
        push PDI
        push PSI

        mov PBX,xg  
        mov PSI,yg  
        mov PCX,mg
        mov PDI,wg   
   

        fldz

        MODMULT

        pop PSI
        pop PDI
    }  

    for (ij=MR_PENTIUM;ij<(int)(z->len&MR_OBITS);ij++) z->w[ij]=0.0;
    z->len=MR_PENTIUM;
    for (ij=0;ij<MR_PENTIUM;ij++) z->w[ij]=w0->w[ij+MR_PENTIUM];
    if (z->w[MR_PENTIUM-1]==0.0) mr_lzero(z);
} 

void fastmodsquare(_MIPD_ x,z)
big x,z;
{
    int ij;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    big w0=mr_mip->w0;
    big modulus=mr_mip->modulus;
    mr_small *wg,*mg,*xg;   
    wg=w0->w;
    mg=modulus->w;
    xg=x->w;

    for (ij=2*MR_PENTIUM;ij<(int)(w0->len&MR_OBITS);ij++) w0->w[ij]=0.0;
    w0->len=2*MR_PENTIUM;

    ASM
    {
        push PDI
        push PSI

        mov PBX,xg  
        mov PSI,mg
        mov PDI,wg   

        fldz

        MODSQUARE

        pop PSI
        pop PDI
    }  
    for (ij=MR_PENTIUM;ij<(int)(z->len&MR_OBITS);ij++) z->w[ij]=0.0;
    z->len=MR_PENTIUM;

    for (ij=0;ij<MR_PENTIUM;ij++) z->w[ij]=w0->w[ij+MR_PENTIUM];
    if (z->w[MR_PENTIUM-1]==0.0) mr_lzero(z);

} 
 
#endif
#endif
#endif

