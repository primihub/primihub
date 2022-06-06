/* RSA program for comparison

Use this mirdef.h file

#define MIRACL 32
#define MR_LITTLE_ENDIAN  
#define mr_utype int
#define mr_unsign32 unsigned int                           
#define MR_IBITS      32    
#define MR_LBITS      32    
#define MR_STATIC     16 
#define MR_GENERIC_MT
#define MR_COMBA 16                          
#define MR_ALWAYS_BINARY
#define mr_dltype long long   
#define mr_unsign64 unsigned long long
#define MAXBASE ((mr_small)1<<(MIRACL-1)) 

For some compilers replace long long with __int64
If I/O is not wanted add these lines as well

#define MR_NO_STANDARD_IO
#define MR_NO_FILE_IO 

To build on a PC using Microsoft C, execute this batch file
(the modules mrio1.c and mrio2.c can be removed if no I/O required)

mex 16 ms86 mrcomba
cl /c /O2 /W3 mrcomba.c
cl /c /O2 /W3 mrcore.c
cl /c /O2 /W3 mrarth0.c
cl /c /O2 /W3 mrarth1.c
cl /c /O2 /W3 mrarth2.c
cl /c /O2 /W3 mrio1.c
cl /c /O2 /W3 mrio2.c
cl /c /O2 /W3 mrbits.c
cl /c /O2 /W3 mrxgcd.c
cl /c /O2 /W3 mrmonty.c
cl /c /O2 /W3 mrpower.c
copy mrmuldv.c32 mrmuldv.c
cl /c /O2 /W3 mrmuldv.c

rem
rem Create library 'miracl.lib'
del miracl.lib

lib /OUT:miracl.lib mrio2.obj mrio1.obj mrcomba.obj mrbits.obj
lib /OUT:miracl.lib miracl.lib mrarth2.obj mrpower.obj mrxgcd.obj
lib /OUT:miracl.lib miracl.lib mrmonty.obj mrarth1.obj mrarth0.obj mrcore.obj mrmuldv.obj

del mr*.obj

cl /O2 rsat.c miracl.lib

*/

#include <stdio.h>
#include <string.h>
#include "miracl.h"

#define WORDS 16

static const mr_small rom[]={
0xe69486b3, 0x72a0e606, 0x3ecbe4f9, 0xc1f74ed2, 0x506b7584, 0x36dd2b64, 0xf54934bd, 0x299440dd, 
0x73b02a3c, 0x3fe08d6, 0xde4acb63, 0x635ebdec, 0x857f31ba, 0xd72eef27, 0xd8e8e11f, 0xf58dd26d,
0xb2838293, 0x793eb355, 0x1454cd06, 0xd7aedb5e, 0x9b06ea9c, 0xa4e860e2, 0xe0464051, 0xbd30c0f5, 
0x3aaef142, 0x1e998792, 0xabb18c9f, 0x1532799b, 0x92f44cbf, 0xf9fff267, 0x184a0ca0, 0x99ceb83b,
0x44630477, 0xf715eeaf, 0x29dd4350, 0x2bfa348c, 0x359cf903, 0x79e8c798, 0x4e30cdd3, 0xc662d5e9, 
0xf7cac6d2, 0x57feb08e, 0x3edc8797, 0xece9d3f3, 0x3aa2126, 0xe4c9f4c5, 0x909b40bf, 0xa3b3e19e,
0x21ad01b7, 0xfb7f2239, 0xb83888ae, 0x3a74923e, 0xbcaf4713, 0x189aeb41, 0x95842ae1, 0x28cb2b4e, 
0x7c74a0d7, 0x14665a61, 0x1d21086a, 0x6376fbbd, 0xb74d887f, 0xfbfff6ef, 0x6586b315, 0x6689d027
};

/* Test driver program */
/* Max stack requirement << 8K */

int main()
{
	big p,q,dp,dq,m,c,m1;

	int i,romptr;
    miracl instance;                           /* sizeof(miracl)= 2124 bytes from the stack */
#ifndef MR_STATIC
	miracl *mr_mip=mirsys(&instance,WORDS*8,16);
	char *mem=memalloc(_MIPP_ ,7);   
#else
    miracl *mr_mip=mirsys(&instance,MR_STATIC*8,16); /* size of bigs is fixed */
    char mem[MR_BIG_RESERVE(7)];               /* reserve space on the stack for 7 bigs */
    memset(mem,0,MR_BIG_RESERVE(7));           /* clear this memory */

#endif

/* Initialise bigs */   

    p=mirvar_mem(_MIPP_ mem,0);
	q=mirvar_mem(_MIPP_ mem,1);
    dp=mirvar_mem(_MIPP_ mem,2);
	dq=mirvar_mem(_MIPP_ mem,3);
	m=mirvar_mem(_MIPP_ mem,4);
	c=mirvar_mem(_MIPP_ mem,5);
    m1=mirvar_mem(_MIPP_ mem,6);
  
    romptr=0;

    init_big_from_rom(p,WORDS,rom,256,&romptr);
    init_big_from_rom(q,WORDS,rom,256,&romptr);
    init_big_from_rom(dp,WORDS,rom,256,&romptr);
    init_big_from_rom(dq,WORDS,rom,256,&romptr);

    bigbits(_MIPP_ 512,c);

/* count clocks, instructions and CPI from here.. */
//for (i=0;i<100000;i++)
//{
    powmod(_MIPP_ c,dp,p,m);
    powmod(_MIPP_ c,dq,q,m1);
//}
/* to here... */

#ifndef MR_NO_STANDARD_IO
otnum(_MIPP_ m,stdout);
otnum(_MIPP_ m1,stdout);
#endif

#ifndef MR_STATIC
    memkill(_MIPP_ mem,7);
#else
    memset(mem,0,MR_BIG_RESERVE(6));        /* clear this stack memory */
#endif

    mirexit(_MIPPO_ );  /* clears workspace memory */
    return 0;
}
