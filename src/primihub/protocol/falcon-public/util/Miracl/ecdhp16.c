/*

Example Elliptic Curve Diffie-Hellman program for 16-bit constrained environments. Uses point compression.
Stack-only memory allocation

Use with this mirdef.h header (for a PC using MS C)

(Use this header also with Blackfin processor, and 
mex 10 blackfin mrcomba)

#define MR_LITTLE_ENDIAN
#define MIRACL 16
#define mr_utype short
#define MR_IBITS 32
#define MR_LBITS 32
#define mr_unsign32 unsigned int
#define mr_dltype int
#define MR_DLTYPE_IS_INT
#define MR_STATIC 10
#define MR_ALWAYS_BINARY
#define MR_NOASM
#define MR_STRIPPED_DOWN
#define MR_GENERIC_MT
#define MAXBASE ((mr_small)1<<(MIRACL-1))
#define MR_COMBA 10
#define MR_GENERALIZED_MERSENNE
#define MR_SPECIAL
#define MR_BITSINCHAR 8
#define MR_SIMPLE_BASE
#define MR_SIMPLE_IO


Build the library from these modules (Example using MS C compiler)

mex 10 c mrcomba
rem In a real 16-bit setting, a suitable assembly language .mcs file would be used. Here we use C.
cl /c /O2 /W3 mrcore.c
cl /c /O2 /W3 mrarth0.c
cl /c /O2 /W3 mrarth1.c
cl /c /O2 /W3 mrio1.c
cl /c /O2 /W3 mrbits.c
cl /c /O2 /W3 mrcurve.c
cl /c /O2 /W3 mrsroot.c
cl /c /O2 /W3 mrjack.c
cl /c /O2 /W3 mrlucas.c
cl /c /O2 /W3 mrarth2.c
cl /c /O2 /W3 mrmonty.c
cl /c /O2 /W3 mrcomba.c
cl /c /O2 /W3 mrxgcd.c
cl /c /O2 /W3 mrebrick.c

rem
rem Create library 'miracl.lib'
del miracl.lib

lib /OUT:miracl.lib mrio1.obj mrmonty.obj mrcomba.obj mrxgcd.obj
lib /OUT:miracl.lib miracl.lib mrbits.obj mrarth2.obj mrlucas.obj mrjack.obj
lib /OUT:miracl.lib miracl.lib mrarth0.obj mrarth1.obj mrcore.obj mrebrick.obj
lib /OUT:miracl.lib miracl.lib mrcurve.obj mrsroot.obj
del mr*.obj

rem Create the program

cl /O2 ecdhp16.c miracl.lib


********************************************************************************************


Using the Turbo C 16-bit compiler use this header

#define MR_LITTLE_ENDIAN
#define MIRACL 16
#define mr_utype short
#define MR_IBITS 16
#define MR_LBITS 32
#define mr_unsign32 unsigned long
#define mr_dltype long
#define MR_STATIC 10
#define MR_ALWAYS_BINARY
#define MR_STRIPPED_DOWN
#define MR_GENERIC_MT
#define MAXBASE ((mr_small)1<<(MIRACL-1))
#define MR_COMBA 10
#define MR_GENERALIZED_MERSENNE
#define MR_SPECIAL
#define MR_BITSINCHAR 8
#define MR_NOKOBLITZ

Extract the Turbo C small model version of mrmuldv.c from mrmuldv.any
and execute

mex 10 tc86 mrcomba
tcc -c -ms mrcore.c
tcc -c -ms mrarth0.c
tcc -c -ms mrarth1.c
tcc -c -ms mrarth2.c
tcc -c -ms mrio1.c
tcc -c -ms mrjack.c
tcc -c -ms mrxgcd.c
tcc -c -ms mrbits.c
tcc -c -ms mrmonty.c
tcc -c -ms mrsroot.c
tcc -c -ms mrcurve.c
tcc -c -ms mrlucas.c
tcc -c -ms mrsmall.c
tcc -c -ms mrebrick.c
tcc -c -ms mrcomba.c
tcc -c -ms mrmuldv.c

rem
rem Create library 'miracl.lib'
del miracl.lib

tlib miracl
tlib miracl +mrio1
tlib miracl +mrjack+mrxgcd+mrarth2+mrsroot+mrbits
tlib miracl +mrmonty+mrarth1+mrarth0+mrsmall+mrcore
tlib miracl +mrcurve+mrlucas+mrebrick+mrcomba+mrmuldv
del mr*.obj

tcc -ms ecdhp16.c miracl.lib

This assumes that the tasm assembler is available 

********************************************************

For the msp430 processor use this header

#define MIRACL 16
#define MR_LITTLE_ENDIAN  
#define mr_utype int
#define mr_unsign32 unsigned long
#define mr_dltype long
#define mr_unsign64 unsigned long long
#define MR_IBITS 16
#define MR_LBITS 32
#define MR_NO_FILE_IO
#define MR_STATIC 10
#define MR_COMBA 10
#define MR_ALWAYS_BINARY
#define MAXBASE ((mr_small)1<<(MIRACL-1))
#define MR_BITSINCHAR 8
#define MR_GENERIC_MT
#define MR_STRIPPED_DOWN
#define MR_SIMPLE_IO
#define MR_SIMPLE_BASE
#define MR_SPECIAL
#define MR_GENERALIZED_MERSENNE
#define MR_SMALL_EWINDOW
#define MR_NO_STANDARD_IO
#define MR_NOASM

mex 10 msp430 mrcomba

But read the comments at the start of msp430.mcs

Then add these files to the project, along with this file

mrcore.c
mrarth0.c
mrarth1.c
mrarth2.c
mrio1.c
mrjack.c
mrxgcd.c
mrbits.c
mrmonty.c
mrsroot.c
mrcurve.c
mrlucas.c
mrsmall.c
mrebrick.c
mrcomba.c

*/

#include <stdio.h>
#include <string.h>
#include "miracl.h"

/* !!!!!! THIS CODE AND THESE ROMS ARE NOW CREATED AUTOMATICALLY USING THE ROMAKER.C APPLICATION !!!!!!!! */
/* !!!!!! READ COMMENTS IN ROMAKER.C !!!!!! */


#define HEXDIGS (MIRACL/4)
#define CURVE_BITS 160

/* SCOTT p160 bit elliptic curve Y^2=X^3-3X+157 modulo 2^160-2^112+2^64+1 
   Here is stored P, B, the group order q, and the generator G(x,y)       */

static const mr_small rom[]=
{0x0001, 0x0000, 0x0000, 0x0000, 0x0001, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF,
0x9D,0,0,0,0,0,0,0,0,0,
0x35C3, 0xC739, 0xA36F, 0x36C9, 0x593A, 0xFFFE, 0xFFFF, 0xFFFE, 0xFFFF, 0xFFFF,
0xA11E, 0x4E35, 0x6AAD, 0x373E, 0x87AD, 0xAC67, 0xE14C, 0xEF68, 0x0583, 0xE116,
0x8460, 0xF29E, 0xCE44, 0x63F0, 0x5E8B, 0x761E, 0xE48A, 0x6D02, 0x7574, 0x5707}; 

#define WINDOW 4

/* 32 precomputed points based on fixed generator G(x,y)        */
/* (created using romaker.c program with window size of 4)      */

/* These values are only correct if MR_SPECIAL is defined!      */

static const mr_small prom[]=
{0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0xa11e,0x4e35,0x6aad,0x373e,0x87ad,0xac67,0xe14c,0xef68,0x583,0xe116,
0x8460,0xf29e,0xce44,0x63f0,0x5e8b,0x761e,0xe48a,0x6d02,0x7574,0x5707,
0x6772,0x397f,0x9767,0xf0ed,0xf5ea,0xf954,0x38f4,0x2928,0x7b45,0x2f0a,
0x6448,0xaeb8,0x22d7,0x7e28,0x1d6a,0x3509,0x7ea7,0xa3cd,0x9c3a,0xf99b,
0x544,0xd389,0x55a6,0xa7d5,0xb755,0xe532,0x77a2,0x6c83,0xc3b6,0xae04,
0x5eca,0xfa9,0x8852,0xf64f,0x40d6,0x5b02,0x8960,0xd05c,0xa31f,0x1675,
0x9873,0x5ab3,0xcfa1,0xcd39,0xf19a,0x5a4c,0xea04,0x23b5,0xc579,0xcb53,
0x8bf1,0x3d76,0x85c8,0x123d,0x445b,0x30,0x2cef,0xb240,0x69c9,0x4367,
0x43ee,0xa57f,0xb704,0xdf9e,0x6410,0x359c,0x8e20,0xf1f5,0x1711,0x52d4,
0x5cf1,0x3481,0xa1ea,0x432c,0xd516,0xb073,0x9ba8,0xc4a3,0x9cc7,0x6c19,
0x601b,0xb7f7,0x9a94,0x8011,0x50d,0xe1b5,0x34db,0x98a5,0xb4cb,0xf354,
0xd8f3,0x8b08,0x1176,0xa270,0xeda6,0x6d69,0xd305,0x6c9a,0xab0a,0x561d,
0xa57a,0xe3ad,0x6d98,0x45a2,0xfa02,0x9930,0x744a,0x7fb,0x263b,0xeca9,
0x230c,0x39f9,0xac8a,0x113e,0x2fd1,0x17cb,0x18bf,0xed10,0xd225,0xda56,
0x7ad3,0x6d66,0x9fed,0x13cf,0xed86,0xe288,0x55eb,0x61a1,0xc92d,0xa067,
0x8ef8,0x6b8d,0x67a,0xa091,0xef31,0x948a,0x553e,0x7280,0xe632,0x45a8,
0x4fc8,0x62c2,0xd82f,0x36b2,0x5143,0x836d,0xb073,0xc6fc,0xf900,0x5505,
0xeff8,0x2835,0xfbb,0x9523,0x8f5,0xfe1c,0xb10e,0x94e5,0x35ea,0xe75b,
0x43c7,0x1dfd,0xe2c8,0x5ed1,0x2cd1,0xcb7e,0x210,0x32ea,0x16d7,0x764b,
0xaf27,0x1721,0x37ac,0xe1f4,0xab94,0xb2f5,0xb1dc,0x7a94,0x611,0x42e,
0xc781,0x54c5,0x5784,0xa020,0xd723,0xdf6d,0x76b9,0xfbc9,0xb44e,0x9ffb,
0x9cd4,0x1bea,0x818b,0x7045,0xad2d,0xbeaa,0x505b,0xc778,0x5361,0x15c2,
0x6b66,0x81a5,0xdf67,0xadfe,0xe2fa,0xbdc5,0xef77,0x5eeb,0x60a5,0x9037,
0xa1cc,0x997,0xb8df,0xfc09,0x49e0,0x918a,0x2993,0x4f18,0xf876,0x9680,
0x694b,0x37d,0xbbce,0x3d9f,0x5494,0x9b16,0x25fb,0x760b,0xc6c3,0x9b41,
0x1bda,0x2fa5,0x45ce,0x3da9,0x4810,0x23b8,0xa722,0xceb9,0xc045,0x3366,
0xe65e,0x62ba,0x675a,0xfdf5,0xc814,0x23fd,0x33c9,0x4c06,0xe787,0xa06d,
0x7358,0x6132,0xf731,0xf884,0xe4d3,0x9f3b,0x9fcc,0x458c,0xfb96,0x58c5,
0x6fb1,0xe7f,0x240,0x8adb,0x9095,0xe4dc,0xe662,0x54b8,0x1716,0x8ae4,
0x9c38,0x8a61,0xfcad,0x78ef,0x2029,0x3c0c,0xed18,0x19af,0xd4fb,0xfbf5};


#define WORDS 10  /* Number of words per big variable 10*16 = 160 */

/* Note that in a real application a source of real random numbers would be required, to
   replace those generated by MIRACL's internal pseudo-random generator "bigbits"  
   Alternatively from a truly random and unguessable seed, use MIRACL's strong random 
   number generator */

/* Elliptic Curve Diffie-Hellman, using point compression to minimize bandwidth, 
   and precomputation to speed up off-line calculation */

int main()
{
    int promptr;
    epoint *PB;
    big A,B,p,a,b,q,pa,pb,key,x,y;
    ebrick binst;
    miracl instance;      /* create miracl workspace on the stack */

/* Specify base 16 here so that HEX can be read in directly without a base-change */

    miracl *mip=mirsys(&instance,WORDS*HEXDIGS,16); /* size of bigs is fixed */
    char mem_big[MR_BIG_RESERVE(10)];         /* we need 10 bigs... */
    char mem_ecp[MR_ECP_RESERVE(1)];          /* ..and two elliptic curve points */
 	memset(mem_big, 0, MR_BIG_RESERVE(10));   /* clear the memory */
	memset(mem_ecp, 0, MR_ECP_RESERVE(1));

    A=mirvar_mem(mip, mem_big, 0);       /* Initialise big numbers */
    B=mirvar_mem(mip, mem_big, 1);
    pa=mirvar_mem(mip, mem_big, 2);
    pb=mirvar_mem(mip, mem_big, 3);
    key=mirvar_mem(mip, mem_big, 4);
    x=mirvar_mem(mip, mem_big, 5);
    y=mirvar_mem(mip, mem_big, 6);
    a=mirvar_mem(mip, mem_big, 7);
    b=mirvar_mem(mip, mem_big, 8);
    p=mirvar_mem(mip, mem_big, 9);

    PB=epoint_init_mem(mip, mem_ecp, 0); /* initialise Elliptic Curve points */

    irand(mip, 3L);                      /* change parameter for different random numbers */

    promptr=0;
    init_big_from_rom(p,WORDS,rom,WORDS*5,&promptr);  /* Read in prime modulus p from ROM   */
    init_big_from_rom(B,WORDS,rom,WORDS*5,&promptr);  /* Read in curve parameter B from ROM */
                                                 /* don't need q or G(x,y) (we have precomputed table from it) */

    convert(mip,-3,A);                           /* set A=-3 */

/* Create precomputation instance from precomputed table in ROM */

    ebrick_init(&binst,prom,A,B,p,WINDOW,CURVE_BITS);

/* offline calculations */

    bigbits(mip,CURVE_BITS,a);  /* A's random number */
    mul_brick(mip,&binst,a,pa,pa);    /* a*G =(pa,ya) */
    bigbits(mip,CURVE_BITS,b);  /* B's random number */
    mul_brick(mip,&binst,b,pb,pb);    /* b*G =(pb,yb) */

/* swap X values of point */

/* online calculations */
    ecurve_init(mip,A,B,p,MR_PROJECTIVE);
    epoint_set(mip,pb,pb,0,PB); /* decompress PB */
    ecurve_mult(mip,a,PB,PB);
    epoint_get(mip,PB,key,key);

/* since internal base is HEX, can use otnum instead of cotnum - avoiding a base change */

#ifndef MR_NO_STANDARD_IO
printf("Alice's Key= ");
otnum(mip,key,stdout);
#endif

    epoint_set(mip,pa,pa,0,PB); /* decompress PA */
    ecurve_mult(mip,b,PB,PB);
    epoint_get(mip,PB,key,key);

#ifndef MR_NO_STANDARD_IO
printf("Bob's Key=   ");
otnum(mip,key,stdout);
#endif

/* clear the memory */

	memset(mem_big, 0, MR_BIG_RESERVE(10));
	memset(mem_ecp, 0, MR_ECP_RESERVE(1));

	return 0;
}
