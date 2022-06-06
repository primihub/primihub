/* C version of ake6mntt.cpp 

Example for embedded implementation. 

Should build immediately with standard mirdef.h file on a PC. For example using MS C

cl /O2 ake6mntt.c ms32.lib

To simulate performance on a PC of an 8-bit computer use

#define MR_LITTLE_ENDIAN
#define MIRACL 8
#define mr_utype char
#define MR_IBITS 32
#define MR_LBITS 32
#define mr_unsign32 unsigned int
#define mr_dltype short
#define MR_STATIC 20
#define MR_ALWAYS_BINARY
#define MR_STRIPPED_DOWN
#define MR_GENERIC_MT
#define MAXBASE ((mr_small)1<<(MIRACL-1))
#define MR_COMBA 20
#define MR_NOASM
#define MR_BITSINCHAR 8
#define MR_NOSUPPORT_COMPRESSION

rem Compile MIRACL modules
mex 20 c mrcomba
cl /c /O2 /W3 mrzzn3.c
cl /c /O2 /W3 mrcore.c
cl /c /O2 /W3 mrarth0.c
cl /c /O2 /W3 mrarth1.c
cl /c /O2 /W3 mrarth2.c
cl /c /O2 /W3 mrxgcd.c
cl /c /O2 /W3 mrbits.c
cl /c /O2 /W3 mrmonty.c
cl /c /O2 /W3 mrcurve.c
cl /c /O2 /W3 mrcomba.c
cl /c /O2 /W3 mrio1.c
cl /c /O2 /W3 mrpower.c


rem
rem Create library 'miracl.lib'
del miracl.lib


lib /OUT:miracl.lib mrxgcd.obj mrarth2.obj mrio1.obj mrcomba.obj
lib /OUT:miracl.lib miracl.lib mrmonty.obj mrarth1.obj mrarth0.obj mrcore.obj 
lib /OUT:miracl.lib miracl.lib mrcurve.obj mrbits.obj mrzzn3.obj mrpower.obj

del mr*.obj

cl /O2 ake6mntt.c miracl.lib

For Atmel AVR (atmega128) use

#define MR_LITTLE_ENDIAN
#define MIRACL 8
#define mr_utype char
#define MR_IBITS 16  
#define MR_LBITS 32
#define mr_unsign32 unsigned long
#define mr_dltype int 
#define mr_qltype long
#define MR_STATIC 20
#define MR_ALWAYS_BINARY
#define MR_STRIPPED_DOWN
#define MR_GENERIC_MT
#define MAXBASE ((mr_small)1<<(MIRACL-1))
#define MR_COMBA 20
#define MR_NOASM
#define MR_BITSINCHAR 8
#define MR_NO_STANDARD_IO
#define MR_NO_FILE_IO
#define MR_NOSUPPORT_COMPRESSION
#define MR_AVR

This last line must be added manually - config.c will not do it automatically

and execute 

mex 20 avr4 mrcomba

On an ARM use a header like

#define MR_LITTLE_ENDIAN
#define MIRACL 32
#define mr_utype int
#define MR_IBITS 32
#define MR_LBITS 32
#define mr_unsign32 unsigned int
#define mr_dltype long long
#define MR_STATIC 5
#define MR_ALWAYS_BINARY
#define MR_STRIPPED_DOWN
#define MR_GENERIC_MT
#define MAXBASE ((mr_small)1<<(MIRACL-1))
#define MR_COMBA 5
#define MR_BITSINCHAR 8
#define MR_NOSUPPORT_COMPRESSION

and possible

#define MR_NO_STANDARD_IO
#define MR_NO_FILE_IO

and execute

mex 5 arm mrcomba

*/

#include <stdio.h>
#include <stdlib.h>
#include "miracl.h"

#ifdef MR_COUNT_OPS

    int fpc=0;
    int fpa=0;
    int fpx=0;

#endif

/* Fix the contents of mnt.ecs */

#if MIRACL==64

NOT SUPPORTED

/* Note: n-residue formats are not the same for different word lengths ...*/

#endif

#if MIRACL==32

#define WORDS 5
#define NPW   8   /* Nibbles per Word */
#define ROMSZ 25

static const mr_small romp[]={
0x44626303,0xBB9F14CF,0x749D0195,0xA2E3DDB1,0x7DDCA613,
0x7D8CFD4E,0xF828D365,0xF99273D0,0x7864D1F1,0x21C3F3AC,
0x17D369B7,0xF44414FD,0xBA4E12DE,0xD171EED8,0x3EEE5309,
0xB23BE531,0x1D6BFF48,0xE93BBADB,0x45C7BB62,0xFBB94C27,
0xA92DB540,0x24C6F8B9,0x5913FCF1,0x9EA2B8F4,0x3AA6D7A2
};

/* Points - in n-residue form */

#define PROMSZ 40

static const mr_small Prom[]={
0xB6EF1A16,0x074251AD,0xF062F710,0xD679E3D1,0x2EEEF8B2,
0x8DF7059B,0x3882F3BF,0x92A4F4AA,0xE9D835E6,0x265FB32,
0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,
0x0C05C29F,0xF8512956,0xBD6A3234,0xA3C6C536,0x758EE3E4,
0x3282E036,0xCAC39E95,0x775F3F11,0x1E4FED9B,0x311F2F09,
0x5EBDAB43,0xD4EC58B7,0x406EDA4D,0x294137C6,0x57BD4583,
0x1B936DC6,0xD4A63AB3,0x05026F3B,0x1D4FA7B1,0x5CC7D4CC
};

#endif

#if MIRACL==8

#define WORDS 20
#define NPW   2   /* Nibbles per Word */
#define ROMSZ 100

#ifdef MR_AVR
__attribute__((__progmem__))
#endif 
static const mr_small romp[]={
0x3,0x63,0x62,0x44,0xcf,0x14,0x9f,0xbb,0x95,0x1,0x9d,0x74,0xb1,0xdd,0xe3,0xa2,0x13,0xa6,0xdc,0x7d,
0x4e,0xfd,0x8c,0x7d,0x65,0xd3,0x28,0xf8,0xd0,0x73,0x92,0xf9,0xf1,0xd1,0x64,0x78,0xac,0xf3,0xc3,0x21,
0xb7,0x69,0xd3,0x17,0xfd,0x14,0x44,0xf4,0xde,0x12,0x4e,0xba,0xd8,0xee,0x71,0xd1,0x9,0x53,0xee,0x3e,
0x31,0xe5,0x3b,0xb2,0x48,0xff,0x6b,0x1d,0xdb,0xba,0x3b,0xe9,0x62,0xbb,0xc7,0x45,0x27,0x4c,0xb9,0xfb,
0x40,0xb5,0x2d,0xa9,0xb9,0xf8,0xc6,0x24,0xf1,0xfc,0x13,0x59,0xf4,0xb8,0xa2,0x9e,0xa2,0xd7,0xa6,0x3a};

#define PROMSZ 160

#ifdef MR_AVR
__attribute__((__progmem__))
#endif 
static const mr_small Prom[]={
0x16,0x1a,0xef,0xb6,0xad,0x51,0x42,0x7,0x10,0xf7,0x62,0xf0,0xd1,0xe3,0x79,0xd6,0xb2,0xf8,0xee,0x2e,
0x9b,0x5,0xf7,0x8d,0xbf,0xf3,0x82,0x38,0xaa,0xf4,0xa4,0x92,0xe6,0x35,0xd8,0xe9,0x32,0xfb,0x65,0x2,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x9f,0xc2,0x5,0xc,0x56,0x29,0x51,0xf8,0x34,0x32,0x6a,0xbd,0x36,0xc5,0xc6,0xa3,0xe4,0xe3,0x8e,0x75,
0x36,0xe0,0x82,0x32,0x95,0x9e,0xc3,0xca,0x11,0x3f,0x5f,0x77,0x9b,0xed,0x4f,0x1e,0x9,0x2f,0x1f,0x31,
0x43,0xab,0xbd,0x5e,0xb7,0x58,0xec,0xd4,0x4d,0xda,0x6e,0x40,0xc6,0x37,0x41,0x29,0x83,0x45,0xbd,0x57,
0xc6,0x6d,0x93,0x1b,0xb3,0x3a,0xa6,0xd4,0x3b,0x6f,0x2,0x5,0xb1,0xa7,0x4f,0x1d,0xcc,0xd4,0xc7,0x5c};

#endif

#define CNR 2
#define CF 2

/* Fp6 support functions */

typedef struct
{
    zzn3 x;
    zzn3 y;
    BOOL unitary;
} zzn6;

#ifndef MR_NO_STANDARD_IO
void zzn3_out(_MIPD_ char *p,zzn3 *x)
{
    printf(p); printf("\n");
    redc(_MIPP_ x->a,x->a);
    redc(_MIPP_ x->b,x->b);
    redc(_MIPP_ x->c,x->c); 
    otnum(_MIPP_ x->a,stdout);
    otnum(_MIPP_ x->b,stdout);
    otnum(_MIPP_ x->c,stdout);
    nres(_MIPP_ x->a,x->a);
    nres(_MIPP_ x->b,x->b);
    nres(_MIPP_ x->c,x->c); 
}
#endif

void zzn6_copy(zzn6 *u,zzn6 *w)
{
    if (u==w) return;
    zzn3_copy(&(u->x),&(w->x));
    zzn3_copy(&(u->y),&(w->y));
    w->unitary=u->unitary;
}

void zzn6_from_int(_MIPD_ int i,zzn6 *w)
{
    zzn3_from_int(_MIPP_ i,&(w->x));
    zzn3_zero(&(w->y));
    if (i==1) w->unitary=TRUE;
    else      w->unitary=FALSE;
}

void zzn6_conj(_MIPD_ zzn6 *u,zzn6 *w)
{
    zzn6_copy(u,w);
    zzn3_negate(_MIPP_ &(w->y),&(w->y)); 
}

void zzn6_mul(_MIPD_ zzn6 *u,zzn6 *v,zzn6 *w)
{
    zzn3 t1,t2,t3;
    t1.a=mr_mip->w7;
    t1.b=mr_mip->w8;
    t1.c=mr_mip->w9;
    t2.a=mr_mip->w10;
    t2.b=mr_mip->w11;
    t2.c=mr_mip->w12;
    t3.a=mr_mip->w13;
    t3.b=mr_mip->w14;
    t3.c=mr_mip->w15;
    if (u==v)
    { 
/* See Stam & Lenstra, "Efficient subgroup exponentiation in Quadratic .. Extensions", CHES 2002 */
        if (u->unitary)
        { /* this is a lot faster.. */
            zzn6_copy(u,w);
            zzn3_mul(_MIPP_ &(w->y),&(w->y),&t1);
            zzn3_add(_MIPP_ &(w->y),&(w->x),&(w->y));
            zzn3_mul(_MIPP_ &(w->y),&(w->y),&(w->y));
            zzn3_sub(_MIPP_ &(w->y),&t1,&(w->y));
            zzn3_timesi(_MIPP_ &t1);
            zzn3_copy(&t1,&(w->x));
            zzn3_sub(_MIPP_ &(w->y),&(w->x),&(w->y));
            zzn3_add(_MIPP_ &(w->x),&(w->x),&(w->x));
            zzn3_sadd(_MIPP_ &(w->x),mr_mip->one,&(w->x));
            zzn3_ssub(_MIPP_ &(w->y),mr_mip->one,&(w->y));
        }
        else
        {
            zzn6_copy(u,w);
            zzn3_add(_MIPP_ &(w->x),&(w->y),&t1);
            zzn3_copy(&(w->y),&t3);
            zzn3_timesi(_MIPP_ &t3);
            zzn3_add(_MIPP_ &(w->x),&t3,&t2);
            zzn3_mul(_MIPP_ &t1,&t2,&t1);
            zzn3_mul(_MIPP_ &(w->y),&(w->x),&(w->y));
            zzn3_sub(_MIPP_ &t1,&(w->y),&t1);
            zzn3_copy(&(w->y),&t3);
            zzn3_timesi(_MIPP_ &t3);
            zzn3_sub(_MIPP_ &t1,&t3,&t1);
            zzn3_add(_MIPP_ &(w->y),&(w->y),&(w->y));
            zzn3_copy(&t1,&(w->x));
        }
    }
    else
    {
        zzn3_mul(_MIPP_ &(u->x),&(v->x),&t1); 
        zzn3_mul(_MIPP_ &(u->y),&(v->y),&t2);
        zzn3_add(_MIPP_ &(v->x),&(v->y),&t3);
        zzn3_add(_MIPP_ &(u->x),&(u->y),&(w->y));
        zzn3_mul(_MIPP_ &(w->y),&t3,&(w->y));
        zzn3_sub(_MIPP_ &(w->y),&t1,&(w->y));
        zzn3_sub(_MIPP_ &(w->y),&t2,&(w->y));
        zzn3_copy(&t1,&(w->x));
        zzn3_timesi(_MIPP_ &t2);
        zzn3_add(_MIPP_ &(w->x),&t2,&(w->x));
        if (u->unitary && v->unitary) w->unitary=TRUE;
        else w->unitary=FALSE;
    }
}

/* zzn6 powering of unitary elements */

void zzn6_powu(_MIPD_ zzn6 *x,big k,zzn6 *u)
{
    zzn6 t[5],u2;
    big k3;
    int i,j,n,nb,nbw,nzs;
#ifndef MR_STATIC
    char *mem=memalloc(_MIPP_ 37);
#else
    char mem[MR_BIG_RESERVE(37)];        
    memset(mem,0,MR_BIG_RESERVE(37));
#endif

    if (size(k)==0)
    {
        zzn6_from_int(_MIPP_ 1,u);
        return;
    }
    zzn6_copy(x,u);
    if (size(k)==1) return;

    for (j=i=0;i<5;i++)
    {
        t[i].x.a=mirvar_mem(_MIPP_ mem,j++);
        t[i].x.b=mirvar_mem(_MIPP_ mem,j++);
        t[i].x.c=mirvar_mem(_MIPP_ mem,j++);
        t[i].y.a=mirvar_mem(_MIPP_ mem,j++);
        t[i].y.b=mirvar_mem(_MIPP_ mem,j++);
        t[i].y.c=mirvar_mem(_MIPP_ mem,j++);
        t[i].unitary=FALSE;
    }
    u2.x.a=mirvar_mem(_MIPP_ mem,j++);
    u2.x.b=mirvar_mem(_MIPP_ mem,j++);
    u2.x.c=mirvar_mem(_MIPP_ mem,j++);
    u2.y.a=mirvar_mem(_MIPP_ mem,j++);
    u2.y.b=mirvar_mem(_MIPP_ mem,j++);
    u2.y.c=mirvar_mem(_MIPP_ mem,j++);
    u2.unitary=FALSE;
    k3=mirvar_mem(_MIPP_ mem,j);

    premult(_MIPP_ k,3,k3);
    zzn6_mul(_MIPP_ u,u,&u2);
    zzn6_copy(u,&t[0]);

    for (i=1;i<=4;i++)
        zzn6_mul(_MIPP_ &u2,&t[i-1],&t[i]);

    nb=logb2(_MIPP_ k3);

    for (i=nb-2;i>=1;)
    {
        n=mr_naf_window(_MIPP_ k,k3,i,&nbw,&nzs,5);

        for (j=0;j<nbw;j++) zzn6_mul(_MIPP_ u,u,u);
        if (n>0)            zzn6_mul(_MIPP_ u,&t[n/2],u);
        if (n<0)
        {
            zzn6_conj(_MIPP_ &t[-n/2],&u2);
            zzn6_mul(_MIPP_ u,&u2,u);
        }
        i-=nbw;
        if (nzs)
        {
            for (j=0;j<nzs;j++) zzn6_mul(_MIPP_ u,u,u);
            i-=nzs;
        }
    }

#ifndef MR_STATIC      
    memkill(_MIPP_ mem,37);
#else
    memset(mem,0,MR_BIG_RESERVE(37)); 
#endif
}

/* Lucas-style ladder exponentiation */

void zzn3_powl(_MIPD_ zzn3 *x,big e,zzn3 *w)
{
    int i,s;
    zzn3 t1,t3,t4;
    t1.a=mr_mip->w7;
    t1.b=mr_mip->w8;
    t1.c=mr_mip->w9;
    t3.a=mr_mip->w10;
    t3.b=mr_mip->w11;
    t3.c=mr_mip->w12;
    t4.a=mr_mip->w13;
    t4.b=mr_mip->w14;
    t4.c=mr_mip->w15;

    zzn3_from_int(_MIPP_ 1,&t1);

    s=size(e);
    if (s==0)
    {
        zzn3_copy(&t1,w);
        return;
    }
    zzn3_copy(x,w);
    if (s==1 || s==(-1)) return;

    i=logb2(_MIPP_ e)-1;

    zzn3_copy(w,&t3);
    zzn3_mul(_MIPP_ w,w,&t4);
    zzn3_add(_MIPP_ &t4,&t4,&t4);
    zzn3_sub(_MIPP_ &t4,&t1,&t4);

    while (i--)
    {
        if (mr_testbit(_MIPP_ e,i))
        {
            zzn3_mul(_MIPP_ &t3,&t4,&t3);
            zzn3_add(_MIPP_ &t3,&t3,&t3);
            zzn3_sub(_MIPP_ &t3,w,&t3);
            zzn3_mul(_MIPP_ &t4,&t4,&t4);
            zzn3_add(_MIPP_ &t4,&t4,&t4);
            zzn3_sub(_MIPP_ &t4,&t1,&t4);
        }
        else
        {
            zzn3_mul(_MIPP_ &t4,&t3,&t4);
            zzn3_add(_MIPP_ &t4,&t4,&t4);
            zzn3_sub(_MIPP_ &t4,w,&t4);
            zzn3_mul(_MIPP_ &t3,&t3,&t3);
            zzn3_add(_MIPP_ &t3,&t3,&t3);
            zzn3_sub(_MIPP_ &t3,&t1,&t3);
        }
    }
    zzn3_copy(&t3,w);
}

void zzn6_powq(_MIPD_ zzn6 *w)
{
    zzn3_powq(_MIPP_ &(w->x),&(w->x));
    zzn3_powq(_MIPP_ &(w->y),&(w->y));
    zzn3_smul(_MIPP_ &(w->y),mr_mip->sru,&(w->y));
}

void zzn6_inv(_MIPD_ zzn6 *w)
{
    zzn3 t1,t2;
    if (w->unitary)
    {
        zzn6_conj(_MIPP_ w,w);
        return;
    }
    t1.a=mr_mip->w7;
    t1.b=mr_mip->w8;
    t1.c=mr_mip->w9;
    t2.a=mr_mip->w10;
    t2.b=mr_mip->w11;
    t2.c=mr_mip->w12;

    zzn3_copy(&(w->x),&t1);
    zzn3_copy(&(w->y),&t2);
    zzn3_mul(_MIPP_ &t1,&t1,&t1);
    zzn3_mul(_MIPP_ &t2,&t2,&t2);
    zzn3_timesi(_MIPP_ &t2);
    zzn3_sub(_MIPP_ &t1,&t2,&t2);
    zzn3_inv(_MIPP_ &t2);
    zzn3_mul(_MIPP_ &(w->x),&t2,&(w->x));
    zzn3_negate(_MIPP_ &(w->y),&(w->y));
    zzn3_mul(_MIPP_ &(w->y),&t2,&(w->y));
}

void g(_MIPD_ epoint *A,epoint *B,zzn3 *Qx,zzn3 *Qy,zzn6 *w)
{
    int type;
    big slope;
    zzn3 nn,dd;

    copy(A->X,mr_mip->w10);

    type=ecurve_add(_MIPP_ B,A);
    if (!type)  
    {
        zzn6_from_int(_MIPP_ 1,w);
        return;
    }
    slope=mr_mip->w8; /* slope in w8 */

    nn.a=mr_mip->w13;
    nn.b=mr_mip->w14;
    nn.c=mr_mip->w15;

    dd.a=mr_mip->w5;
    dd.b=mr_mip->w11;
    dd.c=mr_mip->w9;

    zzn3_copy(Qx,&nn);
    zzn3_copy(Qy,&dd);
    zzn3_negate(_MIPP_ &dd,&dd);

#ifndef MR_AFFINE_ONLY
    if (A->marker!=MR_EPOINT_GENERAL)
        copy(mr_mip->one,mr_mip->w12);
    else copy(A->Z,mr_mip->w12);       
#else
    copy(mr_mip->one,mr_mip->w12);
#endif
    if (type==MR_ADD)
    {
        zzn3_ssub(_MIPP_ &nn,B->X,&nn);
        zzn3_smul(_MIPP_ &nn,slope,&nn);
        nres_modmult(_MIPP_ mr_mip->w12,B->Y,mr_mip->w2);
        zzn3_sadd(_MIPP_ &nn,mr_mip->w2,&nn);
        zzn3_smul(_MIPP_ &dd,mr_mip->w12,&dd);
        zzn3_copy(&nn,&(w->x));
        zzn3_copy(&dd,&(w->y));
        return;
    }

    if (type==MR_DOUBLE)
    { /* note that ecurve_add has left useful things for us in w6 and w7! */     
        nres_modmult(_MIPP_ slope,mr_mip->w6,mr_mip->w2);
        zzn3_smul(_MIPP_ &nn,mr_mip->w2,&nn);
        nres_modmult(_MIPP_ slope,mr_mip->w10,slope);
        zzn3_ssub(_MIPP_ &nn,slope,&nn);
        zzn3_sadd(_MIPP_ &nn,mr_mip->w7,&nn);
        nres_modmult(_MIPP_ mr_mip->w12,mr_mip->w6,mr_mip->w12);
        zzn3_smul(_MIPP_ &dd,mr_mip->w12,&dd);
        zzn3_copy(&nn,&(w->x));
        zzn3_copy(&dd,&(w->y));
        return;
    }
}

void fast_tate_pairing(_MIPD_ epoint *P,zzn3 *Qx,zzn3 *Qy,big q,big cf,zzn6 *w,zzn6* res)
{
    int i,j,n,nb,nbw,nzs;
    epoint *t[4],*A,*P2;
    zzn6 zn[4];
    big work[4];

#ifndef MR_STATIC
    char *mem=memalloc(_MIPP_ 28);
	char *mem1=ecp_memalloc(_MIPP_ 6);
#else
    char mem[MR_BIG_RESERVE(28)];  
    char mem1[MR_ECP_RESERVE(6)]; 
    memset(mem,0,MR_BIG_RESERVE(28));
    memset(mem1,0,MR_ECP_RESERVE(6));
#endif

    for (i=0;i<4;i++)
        t[i]=epoint_init_mem(_MIPP_ mem1,i);
    A=epoint_init_mem(_MIPP_ mem1,4);
    P2=epoint_init_mem(_MIPP_ mem1,5);
  
    for (j=i=0;i<4;i++)
    {
        work[i]=mirvar_mem(_MIPP_ mem,j++);
        zn[i].x.a=mirvar_mem(_MIPP_ mem,j++);
        zn[i].x.b=mirvar_mem(_MIPP_ mem,j++);
        zn[i].x.c=mirvar_mem(_MIPP_ mem,j++);
        zn[i].y.a=mirvar_mem(_MIPP_ mem,j++);
        zn[i].y.b=mirvar_mem(_MIPP_ mem,j++);
        zn[i].y.c=mirvar_mem(_MIPP_ mem,j++);
        zn[i].unitary=FALSE;
    }

    zzn6_from_int(_MIPP_ 1,&zn[0]); 
    epoint_copy(P,A);
    epoint_copy(P,P2);
    epoint_copy(P,t[0]);

    g(_MIPP_ P2,P2,Qx,Qy,res);
    epoint_norm(_MIPP_ P2);

    for (i=1;i<4;i++)
    {
        g(_MIPP_ A,P2,Qx,Qy,w);
        epoint_copy(A,t[i]);
        zzn6_mul(_MIPP_ &zn[i-1],w,&zn[i]);
        zzn6_mul(_MIPP_ &zn[i],res,&zn[i]);
    }

    epoint_multi_norm(_MIPP_ 4,work,t);

    epoint_copy(P,A);
    zzn6_from_int(_MIPP_ 1,res);
 
    nb=logb2(_MIPP_ q);
    for (i=nb-2;i>=0;i-=(nbw+nzs))
    {
        n=mr_window(_MIPP_ q,i,&nbw,&nzs,3);
        for (j=0;j<nbw;j++)
        {
            zzn6_mul(_MIPP_ res,res,res);
            g(_MIPP_ A,A,Qx,Qy,w);
            zzn6_mul(_MIPP_ res,w,res);
        }
        if (n>0)
        {
            zzn6_mul(_MIPP_ res,&zn[n/2],res);
            g(_MIPP_ A,t[n/2],Qx,Qy,w);
            zzn6_mul(_MIPP_ res,w,res);
        }
        for (j=0;j<nzs;j++) 
        {
            zzn6_mul(_MIPP_ res,res,res);
            g(_MIPP_ A,A,Qx,Qy,w);
            zzn6_mul(_MIPP_ res,w,res);
        }
    }

    zzn6_copy(res,w);
    zzn6_powq(_MIPP_ w);
    zzn6_mul(_MIPP_ res,w,res);

    zzn6_copy(res,w);
    zzn6_powq(_MIPP_ w);
    zzn6_powq(_MIPP_ w);
    zzn6_powq(_MIPP_ w);

    zzn6_inv(_MIPP_ res);
    zzn6_mul(_MIPP_ res,w,res);

    res->unitary=TRUE;

#ifndef MR_STATIC      
    memkill(_MIPP_ mem,28);
    ecp_memkill(_MIPP_ mem1,6);
#else
    memset(mem,0,MR_BIG_RESERVE(28)); 
    memset(mem1,0,MR_ECP_RESERVE(6));
#endif
}

void ecap(_MIPD_ epoint *P,zzn3 *Qx,zzn3 *Qy,big q,big cf,zzn3* r)
{
    zzn6 res,w;
#ifndef MR_STATIC
    char *mem=memalloc(_MIPP_ 12);
#else
    char mem[MR_BIG_RESERVE(12)];        
    memset(mem,0,MR_BIG_RESERVE(12));
#endif
    res.x.a=mirvar_mem(_MIPP_ mem,0);
    res.x.b=mirvar_mem(_MIPP_ mem,1);
    res.x.c=mirvar_mem(_MIPP_ mem,2);
    res.y.a=mirvar_mem(_MIPP_ mem,3);
    res.y.b=mirvar_mem(_MIPP_ mem,4);
    res.y.c=mirvar_mem(_MIPP_ mem,5);
    w.x.a=mirvar_mem(_MIPP_ mem,6);
    w.x.b=mirvar_mem(_MIPP_ mem,7);
    w.x.c=mirvar_mem(_MIPP_ mem,8);
    w.y.a=mirvar_mem(_MIPP_ mem,9);
    w.y.b=mirvar_mem(_MIPP_ mem,10);
    w.y.c=mirvar_mem(_MIPP_ mem,11);
    res.unitary=FALSE;
    w.unitary=FALSE;

    epoint_norm(_MIPP_ P);
    fast_tate_pairing(_MIPP_ P,Qx,Qy,q,cf,&w,&res);

    zzn6_copy(&res,&w);
    zzn6_powq(_MIPP_ &res);
    zzn6_mul(_MIPP_ &res,&res,&res);

    zzn6_powu(_MIPP_ &w,cf,&w);
    zzn6_mul(_MIPP_ &res,&w,&res);

    zzn3_copy(&(res.x),r);        

#ifndef MR_STATIC      
    memkill(_MIPP_ mem,12);
#else
    memset(mem,0,MR_BIG_RESERVE(12)); 
#endif
}

int main()
{
#ifdef MR_GENERIC_MT
    miracl instance;
#endif
    big p,A,B,Fr,q,cf,sru,t,T;
    zzn3 res,Qx,Qy;
    epoint *P;
    int i,romptr;
#ifndef MR_STATIC
#ifdef MR_GENERIC_MT
    miracl *mr_mip=mirsys(&instance,WORDS*NPW,16);
#else
    miracl *mr_mip=mirsys(WORDS*NPW,16);
#endif
    char *mem=memalloc(_MIPP_ 18);   
	char *mem1=ecp_memalloc(_MIPP_ 1);
#else
#ifdef MR_GENERIC_MT
    miracl *mr_mip=mirsys(&instance,MR_STATIC*NPW,16);
#else
    miracl *mr_mip=mirsys(MR_STATIC*NPW,16);
#endif
    char mem[MR_BIG_RESERVE(18)];              /* reserve space on the stack for 18 bigs */
    char mem1[MR_ECP_RESERVE(1)];              /* reserve space on stack for 1 curve points */
    memset(mem,0,MR_BIG_RESERVE(18));          /* clear this memory */
    memset(mem1,0,MR_ECP_RESERVE(1));       
#endif
    p=mirvar_mem(_MIPP_ mem,0);
    A=mirvar_mem(_MIPP_ mem,1);
    B=mirvar_mem(_MIPP_ mem,2);
    T=mirvar_mem(_MIPP_ mem,3);
    q=mirvar_mem(_MIPP_ mem,4);
    Fr=mirvar_mem(_MIPP_ mem,5);
    cf=mirvar_mem(_MIPP_ mem,6);
    res.a=mirvar_mem(_MIPP_ mem,7);
    res.b=mirvar_mem(_MIPP_ mem,8);
    res.c=mirvar_mem(_MIPP_ mem,9);
    sru=mirvar_mem(_MIPP_ mem,10);
    t=mirvar_mem(_MIPP_ mem,11);
    Qx.a=mirvar_mem(_MIPP_ mem,12);
    Qx.b=mirvar_mem(_MIPP_ mem,13);
    Qx.c=mirvar_mem(_MIPP_ mem,14);
    Qy.a=mirvar_mem(_MIPP_ mem,15);
    Qy.b=mirvar_mem(_MIPP_ mem,16);
    Qy.c=mirvar_mem(_MIPP_ mem,17);

    P=epoint_init_mem(_MIPP_ mem1,0); 
    convert(_MIPP_ -3,A);

    romptr=0;
    init_big_from_rom(p,WORDS,romp,ROMSZ,&romptr);
    init_big_from_rom(B,WORDS,romp,ROMSZ,&romptr);
    init_big_from_rom(q,WORDS,romp,ROMSZ,&romptr);
    init_big_from_rom(cf,WORDS,romp,ROMSZ,&romptr);
    init_big_from_rom(sru,WORDS,romp,ROMSZ,&romptr);


#ifndef MR_NO_STANDARD_IO
printf("ROM size= %ld\n",sizeof(romp)+sizeof(Prom));
printf("sizeof(miracl)= %ld\n",sizeof(miracl));
#endif

    premult(_MIPP_ p,CF,t);
    subtract(_MIPP_ cf,t,cf);
    ecurve_init(_MIPP_ A,B,p,MR_BEST);
    zzn3_set(_MIPP_ CNR,sru);

    romptr=0;
    init_point_from_rom(P,WORDS,Prom,PROMSZ,&romptr);
    init_big_from_rom(Qx.a,WORDS,Prom,PROMSZ,&romptr);
    init_big_from_rom(Qx.b,WORDS,Prom,PROMSZ,&romptr);
    init_big_from_rom(Qx.c,WORDS,Prom,PROMSZ,&romptr);
    init_big_from_rom(Qy.a,WORDS,Prom,PROMSZ,&romptr);
    init_big_from_rom(Qy.b,WORDS,Prom,PROMSZ,&romptr);
    init_big_from_rom(Qy.c,WORDS,Prom,PROMSZ,&romptr);

#ifdef MR_COUNT_OPS
fpa=fpc=fpx=0;
#endif

    ecap(_MIPP_ P,&Qx,&Qy,q,cf,&res);

#ifdef MR_COUNT_OPS
printf("fpc= %d\n",fpc);
printf("fpa= %d\n",fpa);
printf("fpx= %d\n",fpx);
fpa=fpc=fpx=0;
#endif

    bigbits(_MIPP_ 160,t); 
    zzn3_powl(_MIPP_ &res,t,&res);

#ifndef MR_NO_STANDARD_IO
    zzn3_out(_MIPP_ "res= ",&res);
#endif

    ecurve_mult(_MIPP_ t,P,P);
    ecap(_MIPP_ P,&Qx,&Qy,q,cf,&res);

#ifndef MR_NO_STANDARD_IO
    zzn3_out(_MIPP_ "res= ",&res);
#endif

#ifndef MR_STATIC
    memkill(_MIPP_ mem,18);
    ecp_memkill(_MIPP_ mem1,1);
#else
    memset(mem,0,MR_BIG_RESERVE(18));        /* clear this stack memory */
    memset(mem1,0,MR_ECP_RESERVE(1));
#endif
    return 0;
} 


