/* C version of ake4mntt.cpp 

Example for embedded implementation. 

Should build immediately with standard mirdef.h file on a PC. For example using MS C

cl /O2 ake4mntt.c ms32.lib

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
cl /c /O2 /W3 mrzzn2.c
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
lib /OUT:miracl.lib miracl.lib mrcurve.obj mrbits.obj mrzzn2.obj mrpower.obj

del mr*.obj

cl /O2 ake4mntt.c miracl.lib

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
int fpm2,fpi2,fpc,fpa,fpx;
#endif

/* Fix the contents of k4mnt.ecs */

#if MIRACL==32

#define WORDS 5
#define NPW   8   /* Nibbles per Word */
#define ROMSZ 25

static const mr_small romp[]={
0x76A5755D,0x245769E6,0xF33DC5F3,0x42C82027,0xE3F367D5,
0x866BA034,0x14DB64EB,0xDF4CF677,0xE45200C4,0xDABC0397,
0x58290FC5,0x0BD4BB42,0x0EAEF730,0xA014F1E3,0x6B455E0,
0xC1315D34,0x92168B16,0xF191,0x0,0x0,
0xB79C2B47,0x10BEC9C5,0xE0BF4D14,0x67B5A4AC,0xB3657D09};

/* Points - in n-residue form */

#define PROMSZ 30

static const mr_small Prom[]={
0x1E6EA84F,0xE7CE6B23,0x7B6AC239,0x805022A9,0x260BF17B,
0x90898C7E,0x9D8C9BC9,0xD482E11C,0x2D4D3F68,0x1D1DA150,
0x0,0x0,0x0,0x0,0x0,
0xFC6F2323,0x4F909200,0x3CA3C030,0x162C2DE5,0x64E3CD65,
0x83EF6EA2,0x3AD75EB7,0x995AF708,0x20ED6DAA,0x6C8A7C3B,
0xE6CB7028,0x41A0F382,0x41C677A4,0x67F9C577,0x5F28122A};

#endif

#if MIRACL==8

#define WORDS 20
#define NPW   2   /* Nibbles per Word */
#define ROMSZ 100

#ifdef MR_AVR
__attribute__((__progmem__))
#endif 
static const mr_small romp[]={

0x5D,0x75,0xA5,0x76,0xE6,0x69,0x57,0x24,0xF3,0xC5,0x3D,0xF3,0x27,0x20,0xC8,0x42,0xD5,0x67,0xF3,0xE3,
0x34,0xA0,0x6B,0x86,0xEB,0x64,0xDB,0x14,0x77,0xF6,0x4C,0xDF,0xC4,0x00,0x52,0xE4,0x97,0x03,0xBC,0xDA,
0xC5,0x0F,0x29,0x58,0x42,0xBB,0xD4,0x0B,0x30,0xF7,0xAE,0x0E,0xE3,0xF1,0x14,0xA0,0xE0,0x55,0xB4,0x06,
0x34,0x5D,0x31,0xC1,0x16,0x8B,0x16,0x92,0x91,0xF1,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x47,0x2B,0x9C,0xB7,0xC5,0xC9,0xBE,0x10,0x14,0x4D,0xBF,0xE0,0xAC,0xA4,0xB5,0x67,0x09,0x7D,0x65,0xB3};

#define PROMSZ 120

#ifdef MR_AVR
__attribute__((__progmem__))
#endif 
static const mr_small Prom[]={

0x4F,0xA8,0x6E,0x1E,0x23,0x6B,0xCE,0xE7,0x39,0xC2,0x6A,0x7B,0xA9,0x22,0x50,0x80,0x7B,0xF1,0xB,0x26,
0x7E,0x8C,0x89,0x90,0xC9,0x9B,0x8C,0x9D,0x1C,0xE1,0x82,0xD4,0x68,0x3F,0x4D,0x2D,0x50,0xA1,0x1D,0x1D,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x23,0x23,0x6F,0xFC,0x0,0x92,0x90,0x4F,0x30,0xC0,0xA3,0x3C,0xE5,0x2D,0x2C,0x16,0x65,0xCD,0xE3,0x64,
0xA2,0x6E,0xEF,0x83,0xB7,0x5E,0xD7,0x3A,0x8,0xF7,0x5A,0x99,0xAA,0x6D,0xED,0x20,0x3B,0x7C,0x8A,0x6C,
0x28,0x70,0xCB,0xE6,0x82,0xF3,0xA0,0x41,0xA4,0x77,0xC6,0x41,0x77,0xC5,0xF9,0x67,0x2A,0x12,0x28,0x5F};

#endif

#define CF 34

/* Fp4 support functions */

typedef struct
{
    zzn2 x;
    zzn2 y;
    BOOL unitary;
} zzn4;

#ifndef MR_NO_STANDARD_IO
void zzn2_out(_MIPD_ char *p,zzn2 *x)
{
    printf(p); printf("\n");
    redc(_MIPP_ x->a,x->a);
    redc(_MIPP_ x->b,x->b);
    otnum(_MIPP_ x->a,stdout);
    otnum(_MIPP_ x->b,stdout);
    nres(_MIPP_ x->a,x->a);
    nres(_MIPP_ x->b,x->b);
}
#endif

/* Irreducible over zzn2 is x^2+n */
/* zzn4 is towered on top of this with irreducible X^2+sqrt(n) p=5 mod 8, or X^2+(1+sqrt(n)) p=3,7 mod 8 */
/* same as txx(.) function in C++  */

void zzn2_times_irp(_MIPD_ zzn2 *u)
{
    zzn2 t;

    switch (mr_mip->pmod8)
    {
    case 5: /* times sqrt(n) */
        zzn2_timesi(_MIPP_ u);
        break;
    case 3: /* times 1+sqrt(n) */  
    case 7:
        t.a=mr_mip->w5;
        t.b=mr_mip->w6;
        zzn2_copy(u,&t);
        zzn2_timesi(_MIPP_ &t);
        zzn2_add(_MIPP_ u,&t,u);
        break;
    default:
        break;
    }
}

void zzn4_copy(zzn4 *u,zzn4 *w)
{
    if (u==w) return;
    zzn2_copy(&(u->x),&(w->x));
    zzn2_copy(&(u->y),&(w->y));
    w->unitary=u->unitary;
}

void zzn4_from_int(_MIPD_ int i,zzn4 *w)
{
    zzn2_from_int(_MIPP_ i,&(w->x));
    zzn2_zero(&(w->y));
    if (i==1) w->unitary=TRUE;
    else      w->unitary=FALSE;

}

void zzn4_conj(_MIPD_ zzn4 *u,zzn4 *w)
{
    zzn4_copy(u,w);
    zzn2_negate(_MIPP_ &(w->y),&(w->y)); 
}

void zzn4_mul(_MIPD_ zzn4 *u,zzn4 *v,zzn4 *w)
{
    zzn2 t1,t2,t3;
    t1.a=mr_mip->w3;
    t1.b=mr_mip->w4;
    t2.a=mr_mip->w8;
    t2.b=mr_mip->w9;
    if (u==v)
    { 
        if (u->unitary)
        { /* this is a lot faster.. - see Lenstra & Stam */
            zzn4_copy(u,w);
            zzn2_mul(_MIPP_ &(w->y),&(w->y),&t1);
            zzn2_add(_MIPP_ &(w->y),&(w->x),&(w->y));
            zzn2_mul(_MIPP_ &(w->y),&(w->y),&(w->y));
            zzn2_sub(_MIPP_ &(w->y),&t1,&(w->y));
            zzn2_timesi(_MIPP_ &t1);
            zzn2_copy(&t1,&(w->x));
            zzn2_sub(_MIPP_ &(w->y),&(w->x),&(w->y));
            zzn2_add(_MIPP_ &(w->x),&(w->x),&(w->x));
            zzn2_sadd(_MIPP_ &(w->x),mr_mip->one,&(w->x));
            zzn2_ssub(_MIPP_ &(w->y),mr_mip->one,&(w->y));
        }
        else
        {
            zzn4_copy(u,w);
            zzn2_copy(&(w->y),&t2); // t2=b;
            zzn2_add(_MIPP_ &(w->x),&t2,&t1); // t1=a+b
            zzn2_times_irp(_MIPP_ &t2);      // t2=txx(b);
            zzn2_add(_MIPP_ &t2,&(w->x),&t2); // t2=a+txx(b)
            zzn2_mul(_MIPP_ &(w->y),&(w->x),&(w->y)); // b*=a
            zzn2_mul(_MIPP_ &t1,&t2,&(w->x)); // a=t1*t2
            zzn2_copy(&(w->y),&t2); //t2=b
            zzn2_sub(_MIPP_ &(w->x),&t2,&(w->x)); //a-=b      
            zzn2_times_irp(_MIPP_ &t2); // t2=txx(b)
            zzn2_sub(_MIPP_ &(w->x),&t2,&(w->x)); // a-=txx(b);
            zzn2_add(_MIPP_ &(w->y),&(w->y),&(w->y)); // b+=b;
        }
    }
    else
    {
        t3.a=mr_mip->w10;
        t3.b=mr_mip->w11;
        zzn2_copy(&(u->x),&t1);
        zzn2_copy(&(u->y),&t2);
        zzn2_mul(_MIPP_ &t1,&(v->x),&t1);
        zzn2_mul(_MIPP_ &t2,&(v->y),&t2);
        zzn2_copy(&(v->x),&t3);
        zzn2_add(_MIPP_ &t3,&(v->y),&t3);

        zzn2_add(_MIPP_ &(u->y),&(u->x),&(w->y));
        zzn2_mul(_MIPP_ &(w->y),&t3,&(w->y));
        zzn2_sub(_MIPP_ &(w->y),&t1,&(w->y));
        zzn2_sub(_MIPP_ &(w->y),&t2,&(w->y));
        zzn2_copy(&t1,&(w->x));
        zzn2_times_irp(_MIPP_ &t2);
        zzn2_add(_MIPP_ &(w->x),&t2,&(w->x));
        if (u->unitary && v->unitary) w->unitary=TRUE;
        else w->unitary=FALSE;
    }
}

/* zzn4 powering of unitary elements */

void zzn4_powu(_MIPD_ zzn4 *x,big k,zzn4 *u)
{
    zzn4 t[5],u2;
    big k3;
    int i,j,n,nb,nbw,nzs;
#ifndef MR_STATIC
    char *mem=memalloc(_MIPP_ 25);
#else
    char mem[MR_BIG_RESERVE(25)];        
    memset(mem,0,MR_BIG_RESERVE(25));
#endif

    if (size(k)==0)
    {
        zzn4_from_int(_MIPP_ 1,u);
        return;
    }
    zzn4_copy(x,u);
    if (size(k)==1) return;

    for (j=i=0;i<5;i++)
    {
        t[i].x.a=mirvar_mem(_MIPP_ mem,j++);
        t[i].x.b=mirvar_mem(_MIPP_ mem,j++);
        t[i].y.a=mirvar_mem(_MIPP_ mem,j++);
        t[i].y.b=mirvar_mem(_MIPP_ mem,j++);
        t[i].unitary=FALSE;
    }
    u2.x.a=mirvar_mem(_MIPP_ mem,j++);
    u2.x.b=mirvar_mem(_MIPP_ mem,j++);
    u2.y.a=mirvar_mem(_MIPP_ mem,j++);
    u2.y.b=mirvar_mem(_MIPP_ mem,j++);
    u2.unitary=FALSE;
    k3=mirvar_mem(_MIPP_ mem,j);

    premult(_MIPP_ k,3,k3);
    zzn4_mul(_MIPP_ u,u,&u2);
    zzn4_copy(u,&t[0]);

    for (i=1;i<=4;i++)
        zzn4_mul(_MIPP_ &u2,&t[i-1],&t[i]);

    nb=logb2(_MIPP_ k3);

    for (i=nb-2;i>=1;)
    {
        n=mr_naf_window(_MIPP_ k,k3,i,&nbw,&nzs,5);

        for (j=0;j<nbw;j++) zzn4_mul(_MIPP_ u,u,u);
        if (n>0)            zzn4_mul(_MIPP_ u,&t[n/2],u);
        if (n<0)
        {
            zzn4_conj(_MIPP_ &t[-n/2],&u2);
            zzn4_mul(_MIPP_ u,&u2,u);
        }
        i-=nbw;
        if (nzs)
        {
            for (j=0;j<nzs;j++) zzn4_mul(_MIPP_ u,u,u);
            i-=nzs;
        }
    }

#ifndef MR_STATIC      
    memkill(_MIPP_ mem,25);
#else
    memset(mem,0,MR_BIG_RESERVE(25)); 
#endif
}

/* Lucas-style ladder exponentiation */

void zzn2_powl(_MIPD_ zzn2 *x,big e,zzn2 *w)
{
    int i,s;
    zzn2 t1,t3,t4;
    t1.a=mr_mip->w7;
    t1.b=mr_mip->w8;
    t3.a=mr_mip->w10;
    t3.b=mr_mip->w11;
    t4.a=mr_mip->w13;
    t4.b=mr_mip->w14;

    zzn2_from_int(_MIPP_ 1,&t1);

    s=size(e);
    if (s==0)
    {
        zzn2_copy(&t1,w);
        return;
    }
    zzn2_copy(x,w);
    if (s==1 || s==(-1)) return;

    i=logb2(_MIPP_ e)-1;

    zzn2_copy(w,&t3);
    zzn2_mul(_MIPP_ w,w,&t4);
    zzn2_add(_MIPP_ &t4,&t4,&t4);
    zzn2_sub(_MIPP_ &t4,&t1,&t4);

    while (i--)
    {
        if (mr_testbit(_MIPP_ e,i))
        {
            zzn2_mul(_MIPP_ &t3,&t4,&t3);
            zzn2_add(_MIPP_ &t3,&t3,&t3);
            zzn2_sub(_MIPP_ &t3,w,&t3);
            zzn2_mul(_MIPP_ &t4,&t4,&t4);
            zzn2_add(_MIPP_ &t4,&t4,&t4);
            zzn2_sub(_MIPP_ &t4,&t1,&t4);
        }
        else
        {
            zzn2_mul(_MIPP_ &t4,&t3,&t4);
            zzn2_add(_MIPP_ &t4,&t4,&t4);
            zzn2_sub(_MIPP_ &t4,w,&t4);
            zzn2_mul(_MIPP_ &t3,&t3,&t3);
            zzn2_add(_MIPP_ &t3,&t3,&t3);
            zzn2_sub(_MIPP_ &t3,&t1,&t3);
        }

    }
    zzn2_copy(&t3,w);
}

void zzn4_powq(_MIPD_ big fr,zzn4 *w)
{
    zzn2_conj(_MIPP_ &(w->x),&(w->x));
    zzn2_conj(_MIPP_ &(w->y),&(w->y));
    nres(_MIPP_ fr,mr_mip->w1);
    zzn2_smul(_MIPP_ &(w->y),mr_mip->w1,&(w->y));
}

void zzn4_inv(_MIPD_ zzn4 *u)
{
    zzn2 t1,t2;
    if (u->unitary)
    {
        zzn4_conj(_MIPP_ u,u);
        return;
    }
    t1.a=mr_mip->w8;
    t1.b=mr_mip->w9;
    t2.a=mr_mip->w3;
    t2.b=mr_mip->w4; 
    zzn2_mul(_MIPP_ &(u->x),&(u->x),&t1);
    zzn2_mul(_MIPP_ &(u->y),&(u->y),&t2);
    zzn2_times_irp(_MIPP_ &t2);
    zzn2_sub(_MIPP_ &t1,&t2,&t1);
    zzn2_inv(_MIPP_ &t1);
    zzn2_mul(_MIPP_ &(u->x),&t1,&(u->x));
    zzn2_negate(_MIPP_ &t1,&t1);
    zzn2_mul(_MIPP_ &(u->y),&t1,&(u->y));
}

void g(_MIPD_ epoint *A,epoint *B,zzn2 *Qx,zzn2 *Qy,zzn4 *w)
{
    int type;
    big slope;
    zzn2 nn,dd;

    copy(A->X,mr_mip->w10);

    type=ecurve_add(_MIPP_ B,A);
    if (!type)  
    {
        zzn4_from_int(_MIPP_ 1,w);
        return;
    }
    slope=mr_mip->w8; /* slope in w8 */

    nn.a=mr_mip->w13;
    nn.b=mr_mip->w14;
   
    dd.a=mr_mip->w5;
    dd.b=mr_mip->w11;
   
    zzn2_copy(Qx,&nn);
    zzn2_copy(Qy,&dd);
    zzn2_negate(_MIPP_ &dd,&dd);

    if (A->marker!=MR_EPOINT_GENERAL)
        copy(mr_mip->one,mr_mip->w12);
    else copy(A->Z,mr_mip->w12);       

    if (type==MR_ADD)
    {
        zzn2_ssub(_MIPP_ &nn,B->X,&nn);
        zzn2_smul(_MIPP_ &nn,slope,&nn);
        nres_modmult(_MIPP_ mr_mip->w12,B->Y,mr_mip->w2);
        zzn2_sadd(_MIPP_ &nn,mr_mip->w2,&nn);
        zzn2_smul(_MIPP_ &dd,mr_mip->w12,&dd);
        zzn2_copy(&nn,&(w->x));
        zzn2_copy(&dd,&(w->y));
        return;
    }

    if (type==MR_DOUBLE)
    { /* note that ecurve_add has left useful things for us in w6 and w7! */     
        nres_modmult(_MIPP_ slope,mr_mip->w6,mr_mip->w2);
        zzn2_smul(_MIPP_ &nn,mr_mip->w2,&nn);
        nres_modmult(_MIPP_ slope,mr_mip->w10,slope);
        zzn2_ssub(_MIPP_ &nn,slope,&nn);
        zzn2_sadd(_MIPP_ &nn,mr_mip->w7,&nn);
        nres_modmult(_MIPP_ mr_mip->w12,mr_mip->w6,mr_mip->w12);
        zzn2_smul(_MIPP_ &dd,mr_mip->w12,&dd);
        zzn2_copy(&nn,&(w->x));
        zzn2_copy(&dd,&(w->y));
        return;
    }
}

void fast_tate_pairing(_MIPD_ epoint *P,zzn2 *Qx,zzn2 *Qy,big q,big fr,big delta,zzn4 *w,zzn4* res)
{
    int i,j,n,nb,nbw,nzs;
    epoint *t[4],*A,*P2;
    zzn4 zn[4];
    big work[4];

#ifndef MR_STATIC
    char *mem=memalloc(_MIPP_ 20);
	char *mem1=ecp_memalloc(_MIPP_ 6);
#else
    char mem[MR_BIG_RESERVE(20)];  
    char mem1[MR_ECP_RESERVE(6)]; 
    memset(mem,0,MR_BIG_RESERVE(20));
    memset(mem1,0,MR_ECP_RESERVE(6));
#endif

#ifdef MR_COUNT_OPS
fpa=fpc=fpx=0;
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
        zn[i].y.a=mirvar_mem(_MIPP_ mem,j++);
        zn[i].y.b=mirvar_mem(_MIPP_ mem,j++);
        zn[i].unitary=FALSE;
    }

    zzn4_from_int(_MIPP_ 1,&zn[0]); 
    epoint_copy(P,A);
    epoint_copy(P,P2);
    epoint_copy(P,t[0]);

    g(_MIPP_ P2,P2,Qx,Qy,res);

    epoint_norm(_MIPP_ P2);
 
    for (i=1;i<4;i++)
    {
        g(_MIPP_ A,P2,Qx,Qy,w);
        epoint_copy(A,t[i]);
        zzn4_mul(_MIPP_ &zn[i-1],w,&zn[i]);
        zzn4_mul(_MIPP_ &zn[i],res,&zn[i]);
    }

    epoint_multi_norm(_MIPP_ 4,work,t);

    epoint_copy(P,A);
    zzn4_from_int(_MIPP_ 1,res);
 
    nb=logb2(_MIPP_ q);
    for (i=nb-2;i>=0;i-=(nbw+nzs))
    {
        n=mr_window(_MIPP_ q,i,&nbw,&nzs,3);
        for (j=0;j<nbw;j++)
        {
            zzn4_mul(_MIPP_ res,res,res);
            g(_MIPP_ A,A,Qx,Qy,w);
            zzn4_mul(_MIPP_ res,w,res);
        }
        if (n>0)
        {
            zzn4_mul(_MIPP_ res,&zn[n/2],res);
            g(_MIPP_ A,t[n/2],Qx,Qy,w);
            zzn4_mul(_MIPP_ res,w,res);
        }
        for (j=0;j<nzs;j++) 
        {
            zzn4_mul(_MIPP_ res,res,res);
            g(_MIPP_ A,A,Qx,Qy,w);
            zzn4_mul(_MIPP_ res,w,res);
        }
    }
#ifdef MR_COUNT_OPS
printf("Millers loop Cost\n");
printf("fpc= %d\n",fpc);
printf("fpa= %d\n",fpa);
printf("fpx= %d\n",fpx);
fpa=fpc=fpx=0;
#endif
    zzn4_copy(res,w);
    zzn4_powq(_MIPP_ fr,w); zzn4_powq(_MIPP_ fr,w); 
    zzn4_inv(_MIPP_ res);
    zzn4_mul(_MIPP_ res,w,res); /* ^(p*p-1) */

    res->unitary=TRUE;

    zzn4_mul(_MIPP_ res,res,res);
    zzn4_copy(res,w);
    zzn4_mul(_MIPP_ res,res,res);
    zzn4_mul(_MIPP_ res,res,res);
    zzn4_mul(_MIPP_ res,res,res);
    zzn4_mul(_MIPP_ res,res,res);
    zzn4_mul(_MIPP_ res,w,res);    /* res=powu(res,CF) */

#ifndef MR_STATIC      
    memkill(_MIPP_ mem,20);
    ecp_memkill(_MIPP_ mem1,6);
#else
    memset(mem,0,MR_BIG_RESERVE(20)); 
    memset(mem1,0,MR_ECP_RESERVE(6));
#endif
}

void ecap(_MIPD_ epoint *P,zzn2 *Qx,zzn2 *Qy,big q,big fr,big delta,zzn2* r)
{
    zzn4 res,w;
#ifndef MR_STATIC
    char *mem=memalloc(_MIPP_ 8);
#else
    char mem[MR_BIG_RESERVE(8)];        
    memset(mem,0,MR_BIG_RESERVE(8));
#endif
    res.x.a=mirvar_mem(_MIPP_ mem,0);
    res.x.b=mirvar_mem(_MIPP_ mem,1);
    res.y.a=mirvar_mem(_MIPP_ mem,2);
    res.y.b=mirvar_mem(_MIPP_ mem,3);
    w.x.a=mirvar_mem(_MIPP_ mem,4);
    w.x.b=mirvar_mem(_MIPP_ mem,5);
    w.y.a=mirvar_mem(_MIPP_ mem,6);
    w.y.b=mirvar_mem(_MIPP_ mem,7);

    res.unitary=FALSE;
    w.unitary=FALSE;

    epoint_norm(_MIPP_ P);   

    fast_tate_pairing(_MIPP_ P,Qx,Qy,q,fr,delta,&w,&res);

    zzn4_copy(&res,&w);
    zzn4_powq(_MIPP_ fr,&res);
   
    zzn4_powu(_MIPP_ &w,delta,&w);
    zzn4_mul(_MIPP_ &res,&w,&res);

    zzn2_copy(&(res.x),r);        

#ifdef MR_COUNT_OPS
printf("Final Exponentiation cost\n");
printf("fpc= %d\n",fpc);
printf("fpa= %d\n",fpa);
printf("fpx= %d\n",fpx);

fpa=fpc=fpx=0;

#endif
#ifndef MR_STATIC      
    memkill(_MIPP_ mem,8);
#else
    memset(mem,0,MR_BIG_RESERVE(8)); 
#endif
}

int main()
{
#ifdef MR_GENERIC_MT
    miracl instance;
#endif
    big p,A,B,fr,q,delta,t,T;
    zzn2 res,Qx,Qy;
    epoint *P;
    int i,romptr;
#ifndef MR_STATIC
#ifdef MR_GENERIC_MT
    miracl *mr_mip=mirsys(&instance,WORDS*NPW,16);
#else
    miracl *mr_mip=mirsys(WORDS*NPW,16);
#endif
    char *mem=memalloc(_MIPP_ 14);   
	char *mem1=ecp_memalloc(_MIPP_ 1);
#else
#ifdef MR_GENERIC_MT
    miracl *mr_mip=mirsys(&instance,MR_STATIC*NPW,16);
#else
    miracl *mr_mip=mirsys(MR_STATIC*NPW,16);
#endif
    char mem[MR_BIG_RESERVE(14)];              /* reserve space on the stack for 14 bigs */
    char mem1[MR_ECP_RESERVE(1)];              /* reserve space on stack for 1 curve points */
    memset(mem,0,MR_BIG_RESERVE(14));          /* clear this memory */
    memset(mem1,0,MR_ECP_RESERVE(1));       
#endif
    p=mirvar_mem(_MIPP_ mem,0);
    A=mirvar_mem(_MIPP_ mem,1);
    B=mirvar_mem(_MIPP_ mem,2);
    T=mirvar_mem(_MIPP_ mem,3);
    q=mirvar_mem(_MIPP_ mem,4);
    fr=mirvar_mem(_MIPP_ mem,5);
    delta=mirvar_mem(_MIPP_ mem,6);
    res.a=mirvar_mem(_MIPP_ mem,7);
    res.b=mirvar_mem(_MIPP_ mem,8);
    t=mirvar_mem(_MIPP_ mem,9);
    Qx.a=mirvar_mem(_MIPP_ mem,10);
    Qx.b=mirvar_mem(_MIPP_ mem,11);
    Qy.a=mirvar_mem(_MIPP_ mem,12);
    Qy.b=mirvar_mem(_MIPP_ mem,13);

    P=epoint_init_mem(_MIPP_ mem1,0); 
    convert(_MIPP_ -3,A);

    romptr=0;
    init_big_from_rom(p,WORDS,romp,ROMSZ,&romptr);
    init_big_from_rom(B,WORDS,romp,ROMSZ,&romptr);
    init_big_from_rom(q,WORDS,romp,ROMSZ,&romptr);
    init_big_from_rom(delta,WORDS,romp,ROMSZ,&romptr);
    init_big_from_rom(fr,WORDS,romp,ROMSZ,&romptr);

#ifndef MR_NO_STANDARD_IO
printf("ROM size= %d\n",sizeof(romp)+sizeof(Prom));
printf("sizeof(miracl)= %d\n",sizeof(miracl));
#endif

    ecurve_init(_MIPP_ A,B,p,MR_PROJECTIVE);
   
    romptr=0;
    init_point_from_rom(P,WORDS,Prom,PROMSZ,&romptr);
    init_big_from_rom(Qx.a,WORDS,Prom,PROMSZ,&romptr);
    init_big_from_rom(Qx.b,WORDS,Prom,PROMSZ,&romptr);
    init_big_from_rom(Qy.a,WORDS,Prom,PROMSZ,&romptr);
    init_big_from_rom(Qy.b,WORDS,Prom,PROMSZ,&romptr);

#ifdef MR_COUNT_OPS
fpa=fpc=fpx=0;
#endif

    ecap(_MIPP_ P,&Qx,&Qy,q,fr,delta,&res);

    bigbits(_MIPP_ 160,t); 
    zzn2_powl(_MIPP_ &res,t,&res);

#ifndef MR_NO_STANDARD_IO
    zzn2_out(_MIPP_ "res= ",&res);
#endif

    ecurve_mult(_MIPP_ t,P,P);
    ecap(_MIPP_ P,&Qx,&Qy,q,fr,delta,&res);

#ifndef MR_NO_STANDARD_IO
    zzn2_out(_MIPP_ "res= ",&res);
#endif

#ifndef MR_STATIC
    memkill(_MIPP_ mem,14);
    ecp_memkill(_MIPP_ mem1,1);
#else
    memset(mem,0,MR_BIG_RESERVE(14));        /* clear this stack memory */
    memset(mem1,0,MR_ECP_RESERVE(1));
#endif
    return 0;
} 


