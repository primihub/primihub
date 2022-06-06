/*

Example Elliptic Curve Diffie-Hellman program for 32-bit constrained environments. Uses point compression.
Stack-only memory allocation

Use with this mirdef.h header (for a PC using MS C)

#define MR_LITTLE_ENDIAN
#define MIRACL 32
#define mr_utype int
#define MR_IBITS 32
#define MR_LBITS 32
#define mr_unsign32 unsigned int
#define mr_dltype __int64
#define mr_unsign64 unsigned __int64
#define MR_STATIC 5
#define MR_ALWAYS_BINARY
#define MR_STRIPPED_DOWN
#define MR_GENERIC_MT
#define MAXBASE ((mr_small)1<<(MIRACL-1))
#define MR_BITSINCHAR 8
#define MR_COMBA 5
#define MR_PSEUDO_MERSENNE
#define MR_SPECIAL
#define MR_SIMPLE_BASE
#define MR_SIMPLE_IO

Build the library from these modules (Example using MS C compiler)

mex 5 ms86 mrcomba
rem (mex 5 sse2 mrcomba) will be faster on most PCs that support SSE2 
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
cl /c /O2 /W3 mrmuldv.c
cl /c /O2 /W3 mrebrick.c

rem
rem Create library 'miracl.lib'
del miracl.lib

lib /OUT:miracl.lib mrio1.obj mrmonty.obj mrcomba.obj mrxgcd.obj mrmuldv.obj
lib /OUT:miracl.lib miracl.lib mrbits.obj mrarth2.obj mrlucas.obj mrjack.obj
lib /OUT:miracl.lib miracl.lib mrarth0.obj mrarth1.obj mrcore.obj mrebrick.obj
lib /OUT:miracl.lib miracl.lib mrcurve.obj mrsroot.obj
del mr*.obj

rem Create the program

cl /O2 ecdhp32.c miracl.lib

For the ARM processor use

#define MIRACL 32
#define MR_LITTLE_ENDIAN      
#define mr_utype int
#define MR_IBITS 32
#define MR_LBITS 32
#define mr_dltype long long
#define mr_unsign32 unsigned int
#define mr_unsign64 unsigned long long
#define MAXBASE ((mr_small)1<<(MIRACL-1))
#define MR_COMBA 5
#define MR_STATIC 5
#define MR_ALWAYS_BINARY
#define MR_STRIPPED_DOWN
#define MR_GENERIC_MT
#define MR_PSEUDO_MERSENNE
#define MR_SPECIAL

and

mex 5 arm mrcomba
armcc -I. -c -O2 mrcore.c
armcc -I. -c -O2 mrarth0.c
armcc -I. -c -O2 mrarth1.c
armcc -I. -c -O2 mrarth2.c
armcc -I. -c -O2 mrsmall.c
armcc -I. -c -O2 mrio1.c
armcc -I. -c -O2 mrjack.c
armcc -I. -c -O2 mrbits.c
armcc -I. -c -O2 mrxgcd.c
armcc -I. -c -O2 mrmonty.c
armcc -I. -c -O2 mrsroot.c
armcc -I. -c -O2 mrcurve.c
armcc -I. -c -O2 mrlucas.c
armcc -I. -c -O2 mrebrick.c

armcc -I. -c -O2 mrcomba.c
armcc -I. -c -O2 mrmuldv.c
armar -rc miracl.a mrcore.o mrarth0.o mrarth1.o mrarth2.o mrsmall.o
armar -r  miracl.a mrio1.o mrjack.o mrxgcd.o 
armar -r  miracl.a mrmonty.o mrcurve.o 
armar -r  miracl.a mrebrick.o mrsroot.o mrlucas.o
armar -r  miracl.a mrbits.o mrcomba.o mrmuldv.o
del mr*.o
armcc -I. --debug -c -O2 ecdhp32.c
armlink ecdhp32.o miracl.a -o ecdhp32.axf

*/

#include <stdio.h>
#include <string.h>
#include "miracl.h"

/* !!!!!! THIS CODE AND THESE ROMS ARE NOW CREATED AUTOMATICALLY USING THE ROMAKER.C APPLICATION !!!!!!!! */
/* !!!!!! READ COMMENTS IN ROMAKER.C !!!!!! */

#define HEXDIGS (MIRACL/4)
#define CURVE_BITS 160

/* Pseudo Mersenne 160 bit elliptic curve Y^2=X^3-3X+383 modulo 2^160-57 
   Here is stored P, B, the group order q, and the generator G(x,y) */

static const mr_small rom[]=
{0xFFFFFFC7,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF, 
0x17F,0x0,0x0,0x0,0x0,
0x717F3225,0x01A04F4B,0xFFFF0E3F,0xFFFFFFFF,0xFFFFFFFF,
0xB75666F1,0xD027BFEE,0x09B7D04A,0xED594E20,0x0A8ADC5E,
0x5545C9DA,0x111C6CCC,0xAC4D389F,0xAF7643D3,0x20FB2D5C};

#define WINDOW 4

/* 32 precomputed points based on fixed generator G(x,y)        */
/* (created using romaker.c program with window size of 4)      */

/* These values are only correct if MR_SPECIAL is defined!      */

static const mr_small prom[]=
{0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,
0xb75666f1,0xd027bfee,0x9b7d04a,0xed594e20,0xa8adc5e,
0x5545c9da,0x111c6ccc,0xac4d389f,0xaf7643d3,0x20fb2d5c,
0xe5dcf80a,0xada27b96,0x19f2a789,0xa0cbd6fe,0xc6af4491,
0xa4c1ee35,0x91dabe76,0xd96fa35c,0x4f41e824,0x670a00e7,
0x30b19d16,0x479afa2d,0xbb823930,0x9e787e39,0x31a5c91d,
0x9b03e364,0x3f51de4f,0x689f02b5,0x2f3a650e,0x79e94aae,
0x3f8666bf,0xc77f5da0,0x75852755,0x97589faa,0xabe6ec1f,
0xf7323692,0xdf9d2073,0x6d298478,0xe792453f,0x2e642dd0,
0x15dd462,0xca2673e0,0x87e7cbaa,0xfea4ddf5,0xc7bfc7d1,
0xea471875,0xd904cb19,0x8bc54f99,0xec0f657,0x8499af11,
0x31493bdc,0xdc55770b,0x7c56aa97,0x2c277035,0xc8eeb5d7,
0xfe43174d,0x30478f80,0x949183c2,0xfd0a6517,0x324aa528,
0xb8ef76b2,0xb00adb49,0xef5f1ab6,0x379f6247,0x980c0c7f,
0x6b6834e1,0x6c07e146,0x5dba824f,0x69ebeaf9,0xa0ac2a9,
0xb0da9c9e,0xd183ecfa,0x7851c738,0x5d4e69d,0x1e56de83,
0x14ce58f9,0xa563c779,0x31f5109b,0x8b1c9c26,0xcc2b7bba,
0xc7f770dc,0xc8f03ab5,0xc1275f7d,0x84267b4f,0x9d88c48d,
0x3eee917d,0x22afc820,0x4c584705,0x76aec683,0xd44b436e,
0xa2dff854,0x83f54fb9,0xd7bd8efd,0xac4af0f8,0xd0a713fa,
0x4f6125f3,0xdf13ec6b,0x7e354de1,0xa3d9f4ba,0xb2826e0c,
0x6df1f4f9,0x7c6372ce,0x42af4b81,0xaba985d8,0x531dc9ea,
0xd5f9ceb2,0x5ac87241,0xbc750c8f,0xe593f7a9,0xd276be6e,
0x15c9546f,0x99e64886,0x1a75ccdc,0x8cd0d6fd,0x39a708cd,
0xc2d64d0,0x5ddaa2f3,0x31f47696,0xe7740a86,0xb2296006,
0x5608debb,0xff3f8b9f,0x32dc4b30,0x630b8f4c,0xb3f47d5,
0x84771cda,0x2faa9d5b,0x1de86f77,0x8f06694e,0x7af7c3d,
0x2ae42a1,0x3153a324,0x5643e61f,0x1f6e7021,0xe417b332,
0xd1c04a51,0xd6eeb7d1,0x9a3ba8c6,0x8c11b8e2,0xdf9bae00,
0x782cbc03,0xba24cf5f,0x4c09607f,0x11dd868c,0xf5e13ffe,
0xb630b0a0,0xb4d880b7,0x432ebc6a,0x28043baa,0x47a1ef13};

#define WORDS 5  /* Number of words per big variable 5*32 = 160 */

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
    big A,B,p,a,b,pa,pb,key;
    ebrick binst;
    miracl instance;      /* create miracl workspace on the stack */

/* Specify base 16 here so that HEX can be read in directly without a base-change */

    miracl *mip=mirsys(&instance,WORDS*HEXDIGS,16); /* size of bigs is fixed */
    char mem_big[MR_BIG_RESERVE(8)];          /* we need 8 bigs... */
    char mem_ecp[MR_ECP_RESERVE(1)];          /* ..and one elliptic curve point */
 	memset(mem_big, 0, MR_BIG_RESERVE(8));    /* clear the memory */
	memset(mem_ecp, 0, MR_ECP_RESERVE(1));

    A=mirvar_mem(mip, mem_big, 0);       /* Initialise big numbers */
    B=mirvar_mem(mip, mem_big, 1);
    pa=mirvar_mem(mip, mem_big, 2);
    pb=mirvar_mem(mip, mem_big, 3);
    key=mirvar_mem(mip, mem_big, 4);
    a=mirvar_mem(mip, mem_big, 5);
    b=mirvar_mem(mip, mem_big, 6);
    p=mirvar_mem(mip, mem_big, 7);

    PB=epoint_init_mem(mip, mem_ecp, 0); /* initialise Elliptic Curve points */

    irand(mip, 6L);                      /* change parameter for different random numbers */

    promptr=0;
    init_big_from_rom(p,WORDS,rom,WORDS*5,&promptr);  /* Read in prime modulus p from ROM   */
	/*init_big_from_rom(A,WORDS,rom,WORDS*5,&promptr);*/
	convert(mip,-3,A);                           /* fix A=-3 */
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

printf("Alice's Key= ");
otnum(mip,key,stdout);

    epoint_set(mip,pa,pa,0,PB); /* decompress PA */
    ecurve_mult(mip,b,PB,PB);
    epoint_get(mip,PB,key,key);

printf("Bob's Key=   ");
otnum(mip,key,stdout);

/* clear the memory */

	memset(mem_big, 0, MR_BIG_RESERVE(8));
	memset(mem_ecp, 0, MR_ECP_RESERVE(1));

	return 0;
}
