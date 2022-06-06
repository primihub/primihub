/* Author: Michael Scott */
/* Date: Dec 2007        */
/* Even Faster Duursma-Lee char 2 Tate pairing based on eta_T pairing */
/* See MIRACL dl2.cpp for more readable C++ version */
/* cl /O2 etat271.c miracl.lib  */
/* 8-bit version */
/* Half sized loop so nearly twice as fast! */

/* MIRACL mirdef.h
 * For Atmel AVR (e.g. ATmega128L) set up mirdef.h as follows 

#define MR_LITTLE_ENDIAN
#define MIRACL 8
#define mr_utype char
#define MR_IBITS 16
#define MR_LBITS 32
#define mr_unsign32 unsigned long
#define mr_dltype int
#define MR_STATIC 34
#define MR_ALWAYS_BINARY
#define MR_NOASM
#define MR_STRIPPED_DOWN
#define MR_GENERIC_MT
#define MAXBASE ((mr_small)1<<(MIRACL-1))
#define MR_BITSINCHAR 8
#define MR_NOKOBLITZ
#define MR_NO_STANDARD_IO 
#define MR_NO_FILE_IO 
#define MR_SIMPLE_BASE 
#define MR_SIMPLE_IO
#define MR_AVR
#define SP271

*/

/* use this mirdef.h to mimic 8-bit implementation on a PC
#define MR_LITTLE_ENDIAN
#define MIRACL 8
#define mr_utype char
#define MR_IBITS 32
#define MR_LBITS 32
#define mr_unsign32 unsigned int
#define mr_dltype short
#define MR_STATIC 34
#define MR_ALWAYS_BINARY
#define MR_NOASM
#define MR_STRIPPED_DOWN
#define MR_GENERIC_MT
#define MAXBASE ((mr_small)1<<(MIRACL-1))
#define MR_BITSINCHAR 8
#define MR_NOKOBLITZ

*/

/* rem build using this batch file for PC
rem Compile MIRACL modules
cl /c /O2 /W3 mrcore.c
cl /c /O2 /W3 mrarth0.c
cl /c /O2 /W3 mrarth1.c
cl /c /O2 /W3 mrio1.c
cl /c /O2 /W3 mrbits.c
cl /c /O2 /W3 mrgf2m.c
cl /c /O2 /W3 mrec2m.c

rem
rem Create library 'miracl.lib'
del miracl.lib

lib /OUT:miracl.lib mrio1.obj
lib /OUT:miracl.lib miracl.lib mrbits.obj 
lib /OUT:miracl.lib miracl.lib mrarth0.obj mrarth1.obj mrcore.obj 
lib /OUT:miracl.lib miracl.lib mrec2m.obj mrgf2m.obj
del mr*.obj

cl /O2 etat271.c miracl.lib

On the ARM use a header like

#define MR_LITTLE_ENDIAN
#define MIRACL 32
#define mr_utype int
#define MR_IBITS 32
#define MR_LBITS 32
#define mr_unsign32 unsigned int
#define mr_dltype long long
#define MR_STATIC 9
#define MR_ALWAYS_BINARY
#define MR_NOASM
#define MR_STRIPPED_DOWN
#define MR_GENERIC_MT
#define MAXBASE ((mr_small)1<<(MIRACL-1))
#define MR_BITSINCHAR 8
#define MR_NOKOBLITZ


/* define one curve or the other.. */

#include <stdio.h>
#include <string.h>
#include "miracl.h"

#define M 271
#define T 207
#define U 175
#define V 111

#define B 0
#define TYPE 1

/* points P and Q from ROM */

/* WORDS = number of words needs to store GF(2^m) = size of bigs */

/* elements of GF(2^m) are stored in bigs */
/* elements of the quartic extension field GF(2^{4m}) are stored as an array of 4 bigs */
/* = {a,b,c,d} = d.X^3+c.X^2+b.X+a */

/* fast inlined addition code */

#if MIRACL==64

#define WORDS 5
#define NPW 16 /* nibbles per word */
#define ROMSZ 20

static const mr_small rom[]={
0x591B401498D66271,0xA16F0C4E5357F2F6,0xD76AEF912696E510,0x75C041258C778D1D,0x10B1,
0x80DC7F385B9C26BF,0x2B65C2A7BAF3B9FD,0x6A84C19620F8D8B9,0x6D0DB856E16E7097,0x7C02,
0x4EDF428FD0EE2151,0x8A4509E6D6013138,0xBB5FBE66F7C468E7,0xA2740AF91652325E,0x2C67,
0x329B869A3E833026,0xB3716EC7D5F80608,0x3EE35C892B03AE59,0x5AF93E7449ABB134,0x48FB
};

void fincr2(big a,big c)
{ 
    mr_small *aa,*cc;
    aa=a->w; cc=c->w;

    cc[0]^=aa[0];
    cc[1]^=aa[1];
    cc[2]^=aa[2];
    cc[3]^=aa[3];
    cc[4]^=aa[4];

    c->len=WORDS;
    if (cc[4]==0) mr_lzero(c);
}

void fadd2(big a,big b,big c)
{ 
    mr_small *aa,*bb,*cc;
    aa=a->w; bb=b->w; cc=c->w;

    cc[0]=aa[0]^bb[0];
    cc[1]=aa[1]^bb[1];
    cc[2]=aa[2]^bb[2];
    cc[3]=aa[3]^bb[3];
    cc[4]=aa[4]^bb[4];

    c->len=WORDS;
    if (cc[4]==0) mr_lzero(c);
}

/* fast inlined copy code - replaces copy(.) */

void fcopy2(big a,big b)
{
    mr_small *aa,*bb;
    aa=a->w; bb=b->w;

    bb[0]=aa[0];
    bb[1]=aa[1];
    bb[2]=aa[2];
    bb[3]=aa[3];
    bb[4]=aa[4];
 
    b->len=a->len;
}


#endif

#if MIRACL==32

#define WORDS 9
#define NPW 8 /* nibbles per word */
#define ROMSZ 36

static const mr_small rom[]={
0x98D66271,0x591B4014,0x5357F2F6,0xA16F0C4E,0x2696E510,0xD76AEF91,0x8C778D1D,0x75C04125,0x10B1,
0x5B9C26BF,0x80DC7F38,0xBAF3B9FD,0x2B65C2A7,0x20F8D8B9,0x6A84C196,0xE16E7097,0x6D0DB856,0x7C02,
0xD0EE2151,0x4EDF428F,0xD6013138,0x8A4509E6,0xF7C468E7,0xBB5FBE66,0x1652325E,0xA2740AF9,0x2C67,
0x3E833026,0x329B869A,0xD5F80608,0xB3716EC7,0x2B03AE59,0x3EE35C89,0x49ABB134,0x5AF93E74,0x48FB
};

void fincr2(big a,big c)
{ 
    mr_small *aa,*cc;
    aa=a->w; cc=c->w;

    cc[0]^=aa[0];
    cc[1]^=aa[1];
    cc[2]^=aa[2];
    cc[3]^=aa[3];
    cc[4]^=aa[4];
    cc[5]^=aa[5];
    cc[6]^=aa[6];
    cc[7]^=aa[7];
    cc[8]^=aa[8];
  

    c->len=WORDS;
    if (cc[8]==0) mr_lzero(c);
}

void fadd2(big a,big b,big c)
{ 
    mr_small *aa,*bb,*cc;
    aa=a->w; bb=b->w; cc=c->w;

    cc[0]=aa[0]^bb[0];
    cc[1]=aa[1]^bb[1];
    cc[2]=aa[2]^bb[2];
    cc[3]=aa[3]^bb[3];
    cc[4]=aa[4]^bb[4];
    cc[5]=aa[5]^bb[5];
    cc[6]=aa[6]^bb[6];
    cc[7]=aa[7]^bb[7];
    cc[8]=aa[8]^bb[8];

    c->len=WORDS;
    if (cc[8]==0) mr_lzero(c);
}

/* fast inlined copy code - replaces copy(.) */

void fcopy2(big a,big b)
{
    mr_small *aa,*bb;
    aa=a->w; bb=b->w;

    bb[0]=aa[0];
    bb[1]=aa[1];
    bb[2]=aa[2];
    bb[3]=aa[3];
    bb[4]=aa[4];
    bb[5]=aa[5];
    bb[6]=aa[6];
    bb[7]=aa[7];
    bb[8]=aa[8];
 
    b->len=a->len;
}

#endif

#if MIRACL==8

#define WORDS 34
#define NPW 2
#define ROMSZ 136

/* For Pentanomial x^271+x^207+x^175+x^111+1 */

#ifdef MR_AVR
__attribute__((__progmem__))
#endif 
static const mr_small rom[]={
0x71,0x62,0xD6,0x98,0x14,0x40,0x1B,0x59,0xF6,0xF2,0x57,0x53,0x4E,0xC,0x6F,0xA1,0x10,0xE5,0x96,0x26,0x91,0xEF,0x6A,0xD7,0x1D,0x8D,0x77,0x8C,0x25,0x41,0xC0,0x75,0xB1,0x10,
0xBF,0x26,0x9C,0x5B,0x38,0x7F,0xDC,0x80,0xFD,0xB9,0xF3,0xBA,0xA7,0xC2,0x65,0x2B,0xB9,0xD8,0xF8,0x20,0x96,0xC1,0x84,0x6A,0x97,0x70,0x6E,0xE1,0x56,0xB8,0xD,0x6D,0x2,0x7C,
0x51,0x21,0xEE,0xD0,0x8F,0x42,0xDF,0x4E,0x38,0x31,0x1,0xD6,0xE6,0x9,0x45,0x8A,0xE7,0x68,0xC4,0xF7,0x66,0xBE,0x5F,0xBB,0x5E,0x32,0x52,0x16,0xF9,0xA,0x74,0xA2,0x67,0x2C,
0x26,0x30,0x83,0x3E,0x9A,0x86,0x9B,0x32,0x8,0x6,0xF8,0xD5,0xC7,0x6E,0x71,0xB3,0x59,0xAE,0x3,0x2B,0x89,0x5C,0xE3,0x3E,0x34,0xB1,0xAB,0x49,0x74,0x3E,0xF9,0x5A,0xFB,0x48
};

void fincr2(big a,big c)
{ 
    mr_small *aa,*cc;
    aa=a->w; cc=c->w;

    cc[0]^=aa[0];
    cc[1]^=aa[1];
    cc[2]^=aa[2];
    cc[3]^=aa[3];
    cc[4]^=aa[4];
    cc[5]^=aa[5];
    cc[6]^=aa[6];
    cc[7]^=aa[7];
    cc[8]^=aa[8];
    cc[9]^=aa[9];
    cc[10]^=aa[10];
    cc[11]^=aa[11];
    cc[12]^=aa[12];
    cc[13]^=aa[13];
    cc[14]^=aa[14];
    cc[15]^=aa[15];
    cc[16]^=aa[16];
    cc[17]^=aa[17];
    cc[18]^=aa[18];
    cc[19]^=aa[19];
    cc[20]^=aa[20];
    cc[21]^=aa[21];
    cc[22]^=aa[22];
    cc[23]^=aa[23];
    cc[24]^=aa[24];
    cc[25]^=aa[25];
    cc[26]^=aa[26];
    cc[27]^=aa[27];
    cc[28]^=aa[28];
    cc[29]^=aa[29];
    cc[30]^=aa[30];
    cc[31]^=aa[31];
    cc[32]^=aa[32];
    cc[33]^=aa[33];

    c->len=WORDS;
    if (cc[33]==0) mr_lzero(c);
}

void fadd2(big a,big b,big c)
{ 
    mr_small *aa,*bb,*cc;
    aa=a->w; bb=b->w; cc=c->w;

    cc[0]=aa[0]^bb[0];
    cc[1]=aa[1]^bb[1];
    cc[2]=aa[2]^bb[2];
    cc[3]=aa[3]^bb[3];
    cc[4]=aa[4]^bb[4];
    cc[5]=aa[5]^bb[5];
    cc[6]=aa[6]^bb[6];
    cc[7]=aa[7]^bb[7];
    cc[8]=aa[8]^bb[8];
    cc[9]=aa[9]^bb[9];
    cc[10]=aa[10]^bb[10];
    cc[11]=aa[11]^bb[11];
    cc[12]=aa[12]^bb[12];
    cc[13]=aa[13]^bb[13];
    cc[14]=aa[14]^bb[14];
    cc[15]=aa[15]^bb[15];
    cc[16]=aa[16]^bb[16];
    cc[17]=aa[17]^bb[17];
    cc[18]=aa[18]^bb[18];
    cc[19]=aa[19]^bb[19];
    cc[20]=aa[20]^bb[20];
    cc[21]=aa[21]^bb[21];
    cc[22]=aa[22]^bb[22];
    cc[23]=aa[23]^bb[23];
    cc[24]=aa[24]^bb[24];
    cc[25]=aa[25]^bb[25];
    cc[26]=aa[26]^bb[26];
    cc[27]=aa[27]^bb[27];
    cc[28]=aa[28]^bb[28];
    cc[29]=aa[29]^bb[29];
    cc[30]=aa[30]^bb[30];
    cc[31]=aa[31]^bb[31];
    cc[32]=aa[32]^bb[32];
    cc[33]=aa[33]^bb[33];

    c->len=WORDS;
    if (cc[33]==0) mr_lzero(c);
}

/* fast inlined copy code - replaces copy(.) */

void fcopy2(big a,big b)
{
    mr_small *aa,*bb;
    aa=a->w; bb=b->w;

    bb[0]=aa[0];
    bb[1]=aa[1];
    bb[2]=aa[2];
    bb[3]=aa[3];
    bb[4]=aa[4];
    bb[5]=aa[5];
    bb[6]=aa[6];
    bb[7]=aa[7];
    bb[8]=aa[8];
    bb[9]=aa[9];
    bb[10]=aa[10];
    bb[11]=aa[11];
    bb[12]=aa[12];
    bb[13]=aa[13];
    bb[14]=aa[14];
    bb[15]=aa[15];
    bb[16]=aa[16];
    bb[17]=aa[17];
    bb[18]=aa[18];
    bb[19]=aa[19];
    bb[20]=aa[20];
    bb[21]=aa[21];
    bb[22]=aa[22];
    bb[23]=aa[23];
    bb[24]=aa[24];
    bb[25]=aa[25];
    bb[26]=aa[26];
    bb[27]=aa[27];
    bb[28]=aa[28];
    bb[29]=aa[29];
    bb[30]=aa[30];
    bb[31]=aa[31];
    bb[32]=aa[32];
    bb[33]=aa[33];

    b->len=a->len;
}

#endif

/* Use internal workspace variables w1-w13 - must be careful doing this! - see comment below */

void mul(_MIPD_ big *a,big *b,big *r)
{
    /* Special multiplier for GF(2^{4m}) values of the form (x,y,y+1,0) */

    fcopy2(a[1],mr_mip->w2);
    fcopy2(b[1],mr_mip->w3);
    fadd2(a[1],a[0],mr_mip->w8);    /* e=w+p */
    fadd2(b[1],b[0],mr_mip->w9);    /* s=t+q */

    /* only 3 modmults.. */

    modmult2(_MIPP_ mr_mip->w9,mr_mip->w8,mr_mip->w9);      /* z=(w+p)*(t+q) */
    modmult2(_MIPP_ mr_mip->w3,mr_mip->w2,mr_mip->w4);      /* tw=t*w */
    modmult2(_MIPP_ a[0],b[0],mr_mip->w8);            /* pq=p*q */
    fincr2(mr_mip->w4,mr_mip->w9);                    /* z+=tw  */     
    fincr2(mr_mip->w8,mr_mip->w9);                    /* z+=pq  */
    fincr2(mr_mip->w3,mr_mip->w2);                    /* w+=t   */
    fadd2(mr_mip->w2,mr_mip->w4,mr_mip->w3);          /* t=w+tw */
    incr2(mr_mip->w3,1,mr_mip->w3);                   /* t=w+tw+1  */

    fadd2(mr_mip->w9,a[0],mr_mip->w12);            /* x=z+p     */
    fincr2(b[0],mr_mip->w12);                      /* x=z+p+q   */

    fadd2(mr_mip->w8,mr_mip->w3,r[0]);                /* r[0]=pq+t */
    fadd2(mr_mip->w9,mr_mip->w3,r[1]);                /* r[1]=z+t  */
    fadd2(mr_mip->w12,mr_mip->w4,r[2]);               /* r[2]=z+p+q+tw */
    fcopy2(mr_mip->w2,r[3]);                       /* r[3]=w    */
}

/* squaring GF(2^{4m}) values */

void square4(_MIPD_ big *a,big *c)
{
    if (a!=c)
    {
        fcopy2(a[0],c[0]);
        fcopy2(a[1],c[1]);
        fcopy2(a[2],c[2]);
        fcopy2(a[3],c[3]);
    }

    modsquare2(_MIPP_ c[3],c[3]);
    fcopy2(c[2],mr_mip->w1);
    modsquare2(_MIPP_ mr_mip->w1,mr_mip->w1);
    fcopy2(c[1],c[2]);
    modsquare2(_MIPP_ c[2],c[2]);
    modsquare2(_MIPP_ c[0],c[0]);
    fincr2(c[3],c[2]);
    fincr2(mr_mip->w1,c[0]);
    fcopy2(mr_mip->w1,c[1]);

    return;
}

/* multiplying general GF(2^{4m}) values */
/* Uses karatsuba - 9 modmults - very time critical */
/* Use internal workspace variables w1-w13 - must be careful doing this! */
/* The thing is to ensure that none of the invoked miracl internal routines are using the same variables */
/* So first check the miracl source code.... I did... Its OK ... */

void mult4(_MIPD_ big *a,big *b,big *c)
{
    fadd2(a[1],a[3],mr_mip->w3);
    fadd2(a[0],a[2],mr_mip->w4);
    fadd2(b[1],b[3],mr_mip->w8);
    fadd2(b[0],b[2],mr_mip->w9);

    modmult2(_MIPP_ mr_mip->w8,mr_mip->w3,mr_mip->w10);
    modmult2(_MIPP_ mr_mip->w9,mr_mip->w4,mr_mip->w11);
    modmult2(_MIPP_ a[1],b[1],mr_mip->w2);
    modmult2(_MIPP_ a[2],b[2],mr_mip->w1);

    fadd2(mr_mip->w2,mr_mip->w1,mr_mip->w13);

    fadd2(a[1],a[0],c[1]);
    fadd2(b[0],b[1],mr_mip->w12);
    modmult2(_MIPP_ c[1],mr_mip->w12,c[1]);
    modmult2(_MIPP_ a[0],b[0],c[0]);
    fincr2(c[0],c[1]);
    fincr2(mr_mip->w2,c[1]);

    fcopy2(a[2],c[2]);
    fadd2(a[2],a[3],mr_mip->w12);
    fadd2(b[2],b[3],mr_mip->w2);
    modmult2(_MIPP_ mr_mip->w12,mr_mip->w2,mr_mip->w12);

    fincr2(mr_mip->w12,mr_mip->w1);
    modmult2(_MIPP_ a[3],b[3],mr_mip->w2);
    fincr2(mr_mip->w2,mr_mip->w1);

    fadd2(mr_mip->w9,mr_mip->w8,mr_mip->w12);
    fcopy2(mr_mip->w12,c[3]);
    fadd2(mr_mip->w4,mr_mip->w3,mr_mip->w12);
    modmult2(_MIPP_ c[3],mr_mip->w12,c[3]);

    fincr2(mr_mip->w2,mr_mip->w1);

    fincr2(mr_mip->w10,c[3]);
    fincr2(mr_mip->w11,c[3]);
    fincr2(mr_mip->w1,c[3]);
    fincr2(c[1],c[3]);

    fadd2(mr_mip->w13,c[0],c[2]);
    fincr2(mr_mip->w11,c[2]);
    fincr2(mr_mip->w1,c[2]);

    fincr2(mr_mip->w1,c[1]);	
    fincr2(mr_mip->w10,mr_mip->w13);
    fincr2(mr_mip->w13,c[1]);

    fincr2(mr_mip->w2,mr_mip->w13);
    fincr2(mr_mip->w13,c[0]);

    return;
}

/* Frobenius action - GF(2^{4m}) ^ 2^m */

void powq(_MIPD_ big *x)
{
    fcopy2(x[1],mr_mip->w1);
    fincr2(x[1],x[0]);
    fcopy2(x[2],x[1]);
    fincr2(x[3],x[1]);
    fcopy2(mr_mip->w1,x[2]);
}

/* return degree of GF(2^{4m}) polynomial */

int degree(big *c)
{
    if (size(c[3])!=0) return 3;
    if (size(c[2])!=0) return 2;
    if (size(c[1])!=0) return 1;
    return 0;
}

/* Raise a GF(2^{4m}) to a power */

void power4(_MIPD_ big *a,big k,big *u)
{ 
    int i,j,m,nb,n,nbw,nzs;
    big u2[4],t[4][4];
#ifndef MR_STATIC
    char *mem=(char *)memalloc(_MIPP_ 20);
#else
    char mem[MR_BIG_RESERVE(20)];       
    memset(mem,0,MR_BIG_RESERVE(20));
#endif
    m=0;
    for (i=0;i<4;i++)
        for (j=0;j<4;j++) t[i][j]=mirvar_mem(_MIPP_ mem,m++);

    u2[0]=mirvar_mem(_MIPP_ mem,16);
    u2[1]=mirvar_mem(_MIPP_ mem,17);
    u2[2]=mirvar_mem(_MIPP_ mem,18);
    u2[3]=mirvar_mem(_MIPP_ mem,19);

    if(size(k)==0) 
    {
        convert(_MIPP_ 1,u[0]);
        zero(u[1]);
        zero(u[2]);
        zero(u[3]);
#ifndef MR_STATIC
        memkill(_MIPP_ mem,20);
#else
        memset(mem,0,MR_BIG_RESERVE(20));
#endif
        return;
    }

    for (i=0;i<4;i++)
        fcopy2(a[i],u[i]);

    if (size(k)==1)
    {
#ifndef MR_STATIC
        memkill(_MIPP_ mem,20);
#else
        memset(mem,0,MR_BIG_RESERVE(20));
#endif
        return;
    }

    square4(_MIPP_ u,u2);
    for (i=0;i<4;i++)	
        fcopy2(u[i],t[0][i]);	

    for(i=1;i<4;i++)
        mult4(_MIPP_ u2,t[i-1],t[i]);

    nb=logb2(_MIPP_ k);
    if(nb>1) for(i=nb-2;i>=0;)
    {			
        n=mr_window(_MIPP_ k,i,&nbw,&nzs,3); /* small 3 bit window to save RAM */
        for(j=0;j<nbw;j++)
            square4(_MIPP_ u,u);
        if(n>0)	mult4(_MIPP_ u,t[n/2],u);
        i-=nbw;
        if(nzs!=0)
        {
            for (j=0;j<nzs;j++) square4(_MIPP_ u,u);
            i-=nzs;	 				
        }				
    }	

#ifndef MR_STATIC
    memkill(_MIPP_ mem,20);
#else
    memset(mem,0,MR_BIG_RESERVE(20));
#endif
}

/* Inversion of GF(2^{4m}) - fast but not time critical */

void invert(_MIPD_ big *x)
{
    int degF,degG,degB,degC,d,i,j;
    big alpha,beta,gamma,BB[4],FF[5],CC[4],GG[5],t;
    big *BP=BB,*CP=CC,*FP=FF,*GP=GG,*TP;
#ifndef MR_STATIC
    char *mem=(char *)memalloc(_MIPP_ 22);  
#else
    char mem[MR_BIG_RESERVE(22)];       /* 972 bytes from the stack */
    memset(mem,0,MR_BIG_RESERVE(22));
#endif

    alpha=mirvar_mem(_MIPP_ mem,0);
    beta=mirvar_mem(_MIPP_ mem,1);
    gamma=mirvar_mem(_MIPP_ mem,2);
    t=mirvar_mem(_MIPP_ mem,3);

    for (i=0;i<4;i++)
        BB[i]=mirvar_mem(_MIPP_ mem,4+i);
    for (i=0;i<5;i++)
        FF[i]=mirvar_mem(_MIPP_ mem,8+i);
    for (i=0;i<4;i++)
        CC[i]=mirvar_mem(_MIPP_ mem,13+i);
    for (i=0;i<5;i++)
        GG[i]=mirvar_mem(_MIPP_ mem,17+i);

    convert(_MIPP_ 1,CP[0]);
    convert(_MIPP_ 1,FP[0]);
    convert(_MIPP_ 1,FP[1]);
    convert(_MIPP_ 1,FP[4]);   /* F = x^4+x+1 - irreducible polynomial for GF(2^{4m}) */

    degF=4; degG=degree(x); degC=0; degB=-1;

    if (degG==0)
    {
        inverse2(_MIPP_ x[0],x[0]);
        return;
    }

    for (i=0;i<4;i++)
    {
        fcopy2(x[i],GP[i]);
        zero(x[i]);
    }

    while (degF!=0)
    {
        if (degF<degG)
        { /* swap */
            TP=FP; FP=GP; GP=TP;  d=degF; degF=degG; degG=d;
            TP=BP; BP=CP; CP=TP;  d=degB; degB=degC; degC=d;
        }
        j=degF-degG;
        modsquare2(_MIPP_ GP[degG],alpha);
        modmult2(_MIPP_ FP[degF],GP[degG],beta);
        modmult2(_MIPP_ GP[degG],FP[degF-1],t);
        modmult2(_MIPP_ FP[degF],GP[degG-1],gamma);
        fincr2(t,gamma);
        zero(t);

        for (i=0;i<=degF;i++ )
        {
            modmult2(_MIPP_ FP[i],alpha,FP[i]);
            if (i>=j-1)
            {
                modmult2(_MIPP_ gamma,GP[i-j+1],t);
                fincr2(t,FP[i]);
            }  
            if (i>=j)
            {
                modmult2(_MIPP_ beta,GP[i-j],t); 
                fincr2(t,FP[i]);
            }
        }	        
        for (i=0;i<=degB || i<=(degC+j);i++)
        {
            modmult2(_MIPP_ BP[i],alpha,BP[i]);
            if (i>=j-1)
            {	
                modmult2(_MIPP_ gamma,CP[i-j+1],t);	
                fincr2(t,BP[i]);
            }
            if (i>=j)  
            {
                modmult2(_MIPP_ beta,CP[i-j],t);
                fincr2(t,BP[i]);
            }
        }

        while (degF>=0 && size(FP[degF])==0) degF--;
        if (degF==degG)
        {
            fcopy2(FP[degF],alpha);
            for (i=0;i<=degF;i++)
            {
                modmult2(_MIPP_ FP[i],GP[degF],FP[i]);
                modmult2(_MIPP_ alpha,GP[i],t);
                fincr2(t,FP[i]);
            } 
            for (i=0;i<=4-degF;i++)
            {
                modmult2(_MIPP_ BP[i],GP[degF],BP[i]);
                modmult2(_MIPP_ CP[i],alpha,t);
                fincr2(t,BP[i]);

            }
            while (degF>=0 && size(FP[degF])==0) degF--;
        }
        degB=3; 
        while (degB>=0 && size(BP[degB])==0) degB--;
    }

    inverse2(_MIPP_ FP[0],alpha);

    for (i=0;i<=degB;i++)
        modmult2(_MIPP_ alpha,BP[i],x[i]);
#ifndef MR_STATIC   
    memkill(_MIPP_ mem,22);
#else
    memset(mem,0,MR_BIG_RESERVE(22));
#endif
}

/* Tate Pairing - calculated from eta_T pairing */

void eta_T(_MIPD_ epoint *P,epoint *Q,big *f,big *res)
{
    big xp,yp,xq,yq,t,d;
    big miller[4],v[4],u[4];
    int i,m=M;
    int promptr;
#ifndef MR_STATIC
    char *mem=(char *)memalloc(_MIPP_ 18);  
#else
    char mem[MR_BIG_RESERVE(18)];            /* 972 bytes from stack */
    memset(mem,0,MR_BIG_RESERVE(18));
#endif

    xp=mirvar_mem(_MIPP_ mem,0);
    yp=mirvar_mem(_MIPP_ mem,1);
    xq=mirvar_mem(_MIPP_ mem,2);
    yq=mirvar_mem(_MIPP_ mem,3);
    t=mirvar_mem(_MIPP_ mem,4);
    d=mirvar_mem(_MIPP_ mem,5);
    for (i=0;i<4;i++) miller[i]=mirvar_mem(_MIPP_ mem,6+i);
    for (i=0;i<4;i++) v[i]=mirvar_mem(_MIPP_ mem,10+i);
    for (i=0;i<4;i++) u[i]=mirvar_mem(_MIPP_ mem,14+i);

    fcopy2(P->X,xp);
    fcopy2(P->Y,yp);
    fcopy2(Q->X,xq);
    fcopy2(Q->Y,yq);

    incr2(xp,1,t);       /* t=xp+1 */

    fadd2(xp,xq,d);
    incr2(d,1,d);        /* xp+xq+1 */
    modmult2(_MIPP_ d,t,d); /* t*(xp+xq+1) */
    fincr2(yp,d);
    fincr2(yq,d);
    incr2(d,B,d);
    incr2(d,1,f[0]);     /* f[0]=t*(xp+xq+1)+yp+yq+B+1 */

    convert(_MIPP_ 1,miller[0]);

    fadd2(t,xq,f[2]);       /* f[2]=t+xq   */
    incr2(f[2],1,f[1]);     /* f[1]=t+xq+1 */   
    zero(f[3]);
    promptr=0;

    for (i=0;i<(m-1)/2;i+=2)
    {
        fcopy2(xp,t);
        sqroot2(_MIPP_ xp,xp);
        sqroot2(_MIPP_ yp,yp);
        fadd2(xp,xq,d);
        modmult2(_MIPP_ d,t,d);
        fincr2(yp,d);
        fincr2(yq,d);
        fadd2(d,xp,v[0]);       /* v[0]=t*(xp+xq)+yp+yq+xp */
        fadd2(t,xq,v[2]);       /* v[2]=t+xq   */
        incr2(v[2],1,v[1]);     /* v[1]=t+xq+1 */
        modsquare2(_MIPP_ xq,xq);  /* xq*=xq */
        modsquare2(_MIPP_ yq,yq);  /* yp*=yp */

        fcopy2(xp,t);           /* same again - unlooped times 2 */

        sqroot2(_MIPP_ xp,xp);
        sqroot2(_MIPP_ yp,yp);

        fadd2(xp,xq,d);
        modmult2(_MIPP_ d,t,d);
        fincr2(yp,d);
        fincr2(yq,d);
        fadd2(d,xp,u[0]);   
        fadd2(t,xq,u[2]);
        incr2(u[2],1,u[1]);   
        modsquare2(_MIPP_ xq,xq);   
        modsquare2(_MIPP_ yq,yq);  

        mul(_MIPP_ u,v,u);         /* fast mul */
        mult4(_MIPP_ miller,u,miller);
    }

    mult4(_MIPP_ miller,f,miller);

    for (i=0;i<4;i++)
    {
        fcopy2(miller[i],u[i]);
        fcopy2(miller[i],v[i]);
        fcopy2(miller[i],f[i]);
    }

    /* final exponentiation */

    for (i=0;i<(m+1)/2;i++) square4(_MIPP_ u,u);  /* u*=u */    
    powq(_MIPP_ u);
    powq(_MIPP_ f);
    for(i=0;i<4;i++) fcopy2(f[i],v[i]);
    powq(_MIPP_ f);
    for(i=0;i<4;i++) fcopy2(f[i],res[i]);
    powq(_MIPP_ f);
    mult4(_MIPP_ f,u,f);
    mult4(_MIPP_ f,miller,f);
    mult4(_MIPP_ res,v,res);
    powq(_MIPP_ u);
    powq(_MIPP_ u);
    mult4(_MIPP_ res,u,res);

    /* doing inversion here could kill the stack... */

#ifndef MR_STATIC
    memkill(_MIPP_ mem,18);
#else
    memset(mem,0,MR_BIG_RESERVE(18));
#endif
}

/* ... so do inversion here to avoid stack overflow */

void tate(_MIPD_ epoint *P,epoint *Q,big *res)
{
    int i;
    big f[4];
#ifndef MR_STATIC
    char *mem=(char *)memalloc(_MIPP_ 4);  
#else
    char mem[MR_BIG_RESERVE(4)];            /* 160 bytes from stack */
    memset(mem,0,MR_BIG_RESERVE(4));
#endif
    for (i=0;i<4;i++) f[i]=mirvar_mem(_MIPP_ mem,i);

    eta_T(_MIPP_ P,Q,f,res);

    invert(_MIPP_ f);
    mult4(_MIPP_ res,f,res);

#ifndef MR_STATIC
    memkill(_MIPP_ mem,4);
#else
    memset(mem,0,MR_BIG_RESERVE(4));
#endif
}

/* Max stack requirement < 4K */

int main()
{
    big a2,a6,bx,r;
    big res[4];
    epoint *P,*Q;

    int i,romptr;
    miracl instance;                           /* sizeof(miracl)= 2000 bytes from the stack */
#ifndef MR_STATIC
#ifdef MR_GENERIC_MT
    miracl *mr_mip=mirsys(WORDS*NPW,16);
#else
    miracl *mr_mip=mirsys(WORDS*NPW,16);
#endif
    char *mem=(char *)memalloc(_MIPP_ 8);   
    char *mem1=(char *)ecp_memalloc(_MIPP_ 2);
#else
#ifdef MR_GENERIC_MT
    miracl *mr_mip=mirsys(&instance,MR_STATIC*NPW,16); /* size of bigs is fixed */
#else
    miracl *mr_mip=mirsys(&instance,MR_STATIC*NPW,16);
#endif
    char mem[MR_BIG_RESERVE(8)];               /* reserve space on the stack for 8 bigs */
    char mem1[MR_ECP_RESERVE(2)];              /* reserve space on stack for 2 curve points */
    memset(mem,0,MR_BIG_RESERVE(8));           /* clear this memory */
    memset(mem1,0,MR_ECP_RESERVE(2));          /* ~668 bytes in all  */
#endif

    /* Initialise bigs */   

    a2=mirvar_mem(_MIPP_ mem,0);
    a6=mirvar_mem(_MIPP_ mem,1);
    bx=mirvar_mem(_MIPP_ mem,2);
    for (i=0;i<4;i++)
        res[i]=mirvar_mem(_MIPP_ mem,3+i);
    r=mirvar_mem(_MIPP_ mem,7);

    /* printf("ROM size= %d\n",sizeof(rom)+sizeof(prom)); */
#ifndef MR_NO_STANDARD_IO
#ifdef MR_STATIC
    printf("n Bigs require n*%d+%d bytes\n",MR_SIZE,MR_SL);
    printf("n Points require n*%d+%d bytes\n",MR_ESIZE,MR_SL);
    printf("sizeof(miracl)= %d\n",sizeof(miracl));
#endif
#endif
    /* Initialise Elliptic curve points */

    P=epoint_init_mem(_MIPP_ mem1,0);
    Q=epoint_init_mem(_MIPP_ mem1,1);

    /* Initialise supersingular curve */

    convert(_MIPP_ 1,a2);
    convert(_MIPP_ B,a6);

    /* The -M tells MIRACL that this is a supersingular curve */

    if (!ecurve2_init(_MIPP_ -M,T,U,V,a2,a6,FALSE,MR_PROJECTIVE))
    {
#ifndef MR_NO_STANDARD_IO
        printf("Problem with the curve\n");
#endif
        return 0;
    }

    /* Get P and Q from ROM */
    /* These should have been multiplied by the cofactor 487805 = 5*97561 */
    /* 487805 is a cofactor of the group order 2^271+2^136+1 */

    romptr=0;
    init_point_from_rom(P,WORDS,rom,ROMSZ,&romptr);
    init_point_from_rom(Q,WORDS,rom,ROMSZ,&romptr);

#ifndef MR_NO_STANDARD_IO
    printf( "P= \n");
    otnum(_MIPP_ P->X,stdout);
    otnum(_MIPP_ P->Y,stdout);
    printf( "Q= \n");
    otnum(_MIPP_ Q->X,stdout);
    otnum(_MIPP_ Q->Y,stdout);
#endif

    bigbits(_MIPP_ 160,r); 

    /* Simple bilinearity test */

    tate(_MIPP_ P,Q,res);

    /* this could break the 4k stack, 2060+668+2996 >4K    */
    /* so we cannot afford much precomputation in power4   */

    power4(_MIPP_ res,r,res);   /* res=res^{sr} */

#ifndef MR_NO_STANDARD_IO
    printf( "\ne(P,Q)^r= \n");
    for (i=0;i<4;i++)
    {
        otnum(_MIPP_ res[i],stdout);
        zero(res[i]);
    }
#endif    

    ecurve2_mult(_MIPP_ r,Q,Q);   /* Q=rQ */

    epoint2_norm(_MIPP_ Q);

    tate(_MIPP_ P,Q,res);         /* Now invert is taken out of Tate, and the stack should be OK */

#ifndef MR_NO_STANDARD_IO
    printf( "\ne(P,rQ)= \n");
    for (i=0;i<4;i++)
        otnum(_MIPP_ res[i],stdout);
#endif

    /* all done */

#ifndef MR_STATIC
    memkill(_MIPP_ mem,8);
    ecp_memkill(_MIPP_ mem1,2);
#else
    memset(mem,0,MR_BIG_RESERVE(8));        /* clear this stack memory */
    memset(mem1,0,MR_ECP_RESERVE(2));
#endif

    mirexit(_MIPPO_ );  /* clears workspace memory */
    return 0;
}


