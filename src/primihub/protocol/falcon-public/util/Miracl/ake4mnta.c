/* C version of ake4mnta.cpp 
 *

Example for embedded implementation. 

Should build immediately with standard mirdef.h file on a PC. For example using MS C

cl /O2 ake4mnta.c ms32.lib

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
cl /c /O2 /W3 mrcomba.c
cl /c /O2 /W3 mrcurve.c
cl /c /O2 /W3 mrio1.c
cl /c /O2 /W3 mrpower.c


rem
rem Create library 'miracl.lib'
del miracl.lib


lib /OUT:miracl.lib mrxgcd.obj mrarth2.obj mrio1.obj mrcomba.obj
lib /OUT:miracl.lib miracl.lib mrmonty.obj mrarth1.obj mrarth0.obj mrcore.obj 
lib /OUT:miracl.lib miracl.lib mrcurve.obj mrbits.obj mrzzn2.obj mrpower.obj

del mr*.obj

cl /O2 ake4mnta.c miracl.lib

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

   Speeded up using ideas from 
   "Efficient Computation of Tate Pairing in Projective Coordinate over General Characteristic Fields" 
   by Sanjit Chatterjee1, Palash Sarkar1 and Rana Barua1 

*/

#include <stdio.h>
#include <stdlib.h>
#include "miracl.h"

#ifdef MR_COUNT_OPS
int fpm2,fpi2,fpc,fpa,fpx;
#endif

/* Fix the contents of k4mnt.ecs */
/* ROM contains details of a k=4 MNT pairing-friendly curve. In this case p is 160-bits, and 
   =5 mod 8. The pairing friendly prime-order group is of order (p+1-t)/34 */

#define CF 34

#if MIRACL==32

#define WORDS 5
#define NPW   8   /* Nibbles per Word */
#define ROMSZ 30

static const mr_small romp[]={
0x76A5755D,0x245769E6,0xF33DC5F3,0x42C82027,0xE3F367D5,
0x866BA034,0x14DB64EB,0xDF4CF677,0xE45200C4,0xDABC0397,
0x58290FC5,0x0BD4BB42,0x0EAEF730,0xA014F1E3,0x6B455E0,
0xC1315D34,0x92168B16,0xF191,0x0,0x0,
0xB79C2B47,0x10BEC9C5,0xE0BF4D14,0x67B5A4AC,0xB3657D09,
0xC1315D33,0x92168B16,0xF191,0x0,0x0};

/* Points - in n-residue form */

#define PROMSZ 30

static const mr_small Prom[]={
0x1E6EA84F,0xE7CE6B23,0x7B6AC239,0x805022A9,0x260BF17B,
0x90898C7E,0x9D8C9BC9,0xD482E11C,0x2D4D3F68,0x1D1DA150,
0xC8501310,0x524DD4A,0x3DED1AE2,0x3094317B,0x9DB2C44C,
0x23E533B2,0xC8BD73FB,0xF3D25667,0x1CEE805C,0x24EDB45C,
0x21E3CEBD,0x625F7B2E,0xF924576E,0x35FCAA2,0x67DE6249,
0xF780367B,0x43AD112B,0xD9F11765,0xF0BA13D1,0x1F0355FE};

#endif

#if MIRACL==8

#define WORDS 20
#define NPW   2   /* Nibbles per Word */
#define ROMSZ 120

#ifdef MR_AVR
__attribute__((__progmem__))
#endif 
static const mr_small romp[]={

0x5D,0x75,0xA5,0x76,0xE6,0x69,0x57,0x24,0xF3,0xC5,0x3D,0xF3,0x27,0x20,0xC8,0x42,0xD5,0x67,0xF3,0xE3,
0x34,0xA0,0x6B,0x86,0xEB,0x64,0xDB,0x14,0x77,0xF6,0x4C,0xDF,0xC4,0x00,0x52,0xE4,0x97,0x03,0xBC,0xDA,
0xC5,0x0F,0x29,0x58,0x42,0xBB,0xD4,0x0B,0x30,0xF7,0xAE,0x0E,0xE3,0xF1,0x14,0xA0,0xE0,0x55,0xB4,0x06,
0x34,0x5D,0x31,0xC1,0x16,0x8B,0x16,0x92,0x91,0xF1,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x47,0x2B,0x9C,0xB7,0xC5,0xC9,0xBE,0x10,0x14,0x4D,0xBF,0xE0,0xAC,0xA4,0xB5,0x67,0x09,0x7D,0x65,0xB3,
0x33,0x5D,0x31,0xC1,0x16,0x8B,0x16,0x92,0x91,0xF1,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

#define PROMSZ 120

#ifdef MR_AVR
__attribute__((__progmem__))
#endif 
static const mr_small Prom[]={
0x4F,0xA8,0x6E,0x1E,0x23,0x6B,0xCE,0xE7,0x39,0xC2,0x6A,0x7B,0xA9,0x22,0x50,0x80,0x7B,0xF1,0xB,0x26,
0x7E,0x8C,0x89,0x90,0xC9,0x9B,0x8C,0x9D,0x1C,0xE1,0x82,0xD4,0x68,0x3F,0x4D,0x2D,0x50,0xA1,0x1D,0x1D,
0x10,0x13,0x50,0xC8,0x4A,0xDD,0x24,0x5,0xE2,0x1A,0xED,0x3D,0x7B,0x31,0x94,0x30,0x4C,0xC4,0xB2,0x9D,
0xB2,0x33,0xE5,0x23,0xFB,0x73,0xBD,0xC8,0x67,0x56,0xD2,0xF3,0x5C,0x80,0xEE,0x1C,0x5C,0xB4,0xED,0x24,
0xBD,0xCE,0xE3,0x21,0x2E,0x7B,0x5F,0x62,0x6E,0x57,0x24,0xF9,0xA2,0xCA,0x5F,0x3,0x49,0x62,0xDE,0x67,
0x7B,0x36,0x80,0xF7,0x2B,0x11,0xAD,0x43,0x65,0x17,0xF1,0xD9,0xD1,0x13,0xBA,0xF0,0xFE,0x55,0x3,0x1F};

#endif

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
        { /* this is faster.. - see Lenstra & Stam */
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

/* Lucas-style ladder exponentiation of traces */

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

/* Original from A. Devegili. Thanks Augusto! Projective point addition */

BOOL ecurve_fp2_add(_MIPD_ zzn2 *Zx, zzn2 *Zy, zzn2 *Zz, zzn2 *x, zzn2 *y, zzn2 *z, zzn2 *lam,zzn2 *ex1,zzn2 *ex2)
{
    BOOL doubling,normed;
    zzn2 Xzz, Yzzz, xZZ, yZZZ, zz, ZZ;
    zzn2 t1, t2, t3, x3;
#ifndef MR_STATIC
    char *mem = memalloc(_MIPP_ 20);
#else
    char mem[MR_BIG_RESERVE(20)];
    memset(mem, 0, MR_BIG_RESERVE(20));
#endif

    Xzz.a = mirvar_mem(_MIPP_ mem, 0);
    Xzz.b = mirvar_mem(_MIPP_ mem, 1);
    Yzzz.a = mirvar_mem(_MIPP_ mem, 2);
    Yzzz.b = mirvar_mem(_MIPP_ mem, 3);
    xZZ.a = mirvar_mem(_MIPP_ mem, 4);
    xZZ.b = mirvar_mem(_MIPP_ mem, 5);
    yZZZ.a = mirvar_mem(_MIPP_ mem, 6);
    yZZZ.b = mirvar_mem(_MIPP_ mem, 7);
    zz.a = mirvar_mem(_MIPP_ mem, 8);
    zz.b = mirvar_mem(_MIPP_ mem, 9);
    ZZ.a = mirvar_mem(_MIPP_ mem, 10);
    ZZ.b = mirvar_mem(_MIPP_ mem, 11);
    t1.a = mirvar_mem(_MIPP_ mem, 12);
    t1.b = mirvar_mem(_MIPP_ mem, 13);
    t2.a = mirvar_mem(_MIPP_ mem, 14);
    t2.b = mirvar_mem(_MIPP_ mem, 15);
    t3.a = mirvar_mem(_MIPP_ mem, 16);
    t3.b = mirvar_mem(_MIPP_ mem, 17);
    x3.a = mirvar_mem(_MIPP_ mem, 18);
    x3.b = mirvar_mem(_MIPP_ mem, 19);

    doubling=FALSE;
    if (z==Zz) doubling=TRUE;

    if (!doubling)
    { // maybe we are really doubling? Or P-=P?
        if (!zzn2_isunity(_MIPP_ Zz))
        {
            zzn2_mul(_MIPP_ Zz, Zz, &ZZ); // ZZ = Zz^2
            zzn2_mul(_MIPP_ x, &ZZ, &xZZ); // xZZ = x * Zz^2
            zzn2_mul(_MIPP_ &ZZ, Zz, &yZZZ); // yZZZ = Zz^3
            zzn2_mul(_MIPP_ &yZZZ, y, &yZZZ); // yZZZ = y * Zz^3
            normed=FALSE;
        }
        else
        {
            zzn2_copy(x,&xZZ);
            zzn2_copy(y,&yZZZ);
            normed=TRUE;
        }
        if (!zzn2_isunity(_MIPP_ z))
        {
            zzn2_mul(_MIPP_ z, z, &zz); // zz = z^2
            zzn2_mul(_MIPP_ Zx, &zz, &Xzz); // Xzz = Zx * z^2
            zzn2_mul(_MIPP_ &zz, z, &Yzzz); // Yzzz = z^3
            zzn2_mul(_MIPP_ &Yzzz, Zy, &Yzzz); // Yzzz = Zy * z^3
        }
        else
        {
            zzn2_copy(Zx,&Xzz);
            zzn2_copy(Zy,&Yzzz);
        }

        if (zzn2_compare(&Xzz,&xZZ))
        {
            if (!zzn2_compare(&Yzzz,&yZZZ) || zzn2_iszero(y))
            {
                zzn2_zero(x);
                zzn2_zero(y);
                zzn2_zero(z);
                zzn2_from_int(_MIPP_ 1,lam);
#ifndef MR_STATIC
                memkill(_MIPP_ mem, 20);
#else
                memset(mem, 0, MR_BIG_RESERVE(20));
#endif
                return doubling;
            }
            else
                doubling=TRUE;
        }
    }

    if (!doubling)
    { // point addition
    	zzn2_sub(_MIPP_ &xZZ, &Xzz, &t1); // t1 = Xzz - xZZ
	    zzn2_sub(_MIPP_ &yZZZ, &Yzzz, lam); // lam = yZZZ - yZZZ
        zzn2_mul(_MIPP_ z,&t1,z);
        if (!normed) zzn2_mul(_MIPP_ z,&ZZ,z);

        zzn2_mul(_MIPP_ &t1,&t1,&t2);
        zzn2_add(_MIPP_ &xZZ,&Xzz,&t3);
        zzn2_mul(_MIPP_ &t3,&t2,&t3);
	    zzn2_mul(_MIPP_ lam, lam, &x3); // x3 = lam^2
        zzn2_sub(_MIPP_ &x3,&t3,&x3);
        zzn2_sub(_MIPP_ &t3,&x3,&t3);
        zzn2_sub(_MIPP_ &t3,&x3,&t3);

        zzn2_mul(_MIPP_ &t3,lam,&t3);
        zzn2_mul(_MIPP_ &t2,&t1,&t2);
        zzn2_add(_MIPP_ &yZZZ,&Yzzz,&t1);
        zzn2_mul(_MIPP_ &t1,&t2,&t1);
        zzn2_sub(_MIPP_ &t3,&t1,y);
        zzn2_div2(_MIPP_ y);
        zzn2_copy(&x3,x);
    }
    else
    { // point doubling
	    zzn2_mul(_MIPP_ y, y, &t2); // t2 = y^2

/* its on the twist so A=6! */

        zzn2_mul(_MIPP_ z,z,ex2);
        zzn2_mul(_MIPP_ x,x,lam);
        zzn2_mul(_MIPP_ ex2,ex2,&t1);
        zzn2_add(_MIPP_ &t1,&t1,&t1);
        zzn2_add(_MIPP_ lam,&t1,lam);
        zzn2_copy(lam,&t1);
        zzn2_add(_MIPP_ lam,lam,lam);
        zzn2_add(_MIPP_ lam,&t1,lam);

	    zzn2_mul(_MIPP_ x, &t2, &t1); // t1 = x * y^2
	    zzn2_add(_MIPP_ &t1, &t1, &t1); // t1 = 2(x*y^2)
	    zzn2_add(_MIPP_ &t1, &t1, &t1); // t1 = 4(x*y^2)
	
	// x = lam^2 - t1 - t1
	    zzn2_mul(_MIPP_ lam, lam, x); // x = lam^2
	    zzn2_sub(_MIPP_ x, &t1, x); // x = lam^2 - t1
	    zzn2_sub(_MIPP_ x, &t1, x); // x = lam^2 - 2t1

	    zzn2_mul(_MIPP_ z, y , z); // z = yz
	    zzn2_add(_MIPP_ z, z, z); // z = 2yz

	// 8 * y^2
	    zzn2_add(_MIPP_ &t2, &t2, &t2); // t2 = 2y^2
        zzn2_copy(&t2,ex1);
	    zzn2_mul(_MIPP_ &t2, &t2, &t2); // t2 = 4y^2
	    zzn2_add(_MIPP_ &t2, &t2, &t2); // t2 = 8y^2

	// y = lam*(t - x) - y^2
	    zzn2_sub(_MIPP_ &t1, x, y); // y = t1 - x
	    zzn2_mul(_MIPP_ y, lam, y); // y = lam(t1 - x)
	    zzn2_sub(_MIPP_ y, &t2, y); // y = lam(t1 - x) * y^2
    }

#ifndef MR_STATIC
    memkill(_MIPP_ mem, 20);
#else
    memset(mem, 0, MR_BIG_RESERVE(20));
#endif
    return doubling;
}

void g(_MIPD_ zzn2 *Ax,zzn2 *Ay,zzn2 *Az,zzn2 *Bx,zzn2 *By,zzn2 *Bz,big Px,big Py,zzn4 *w)
{
    BOOL Doubling;
    zzn2 lam,extra1,extra2,Kx,nn,dd;

#ifndef MR_STATIC
    char *mem = memalloc(_MIPP_ 12);
#else
    char mem[MR_BIG_RESERVE(12)];
    memset(mem, 0, MR_BIG_RESERVE(12));
#endif
    lam.a=mirvar_mem(_MIPP_ mem,0);
    lam.b=mirvar_mem(_MIPP_ mem,1);
    extra1.a=mirvar_mem(_MIPP_ mem,2);
    extra1.b=mirvar_mem(_MIPP_ mem,3);
    extra2.a=mirvar_mem(_MIPP_ mem,4);
    extra2.b=mirvar_mem(_MIPP_ mem,5);
    Kx.a=mirvar_mem(_MIPP_ mem,6);
    Kx.b=mirvar_mem(_MIPP_ mem,7);
    nn.a=mirvar_mem(_MIPP_ mem,8);
    nn.b=mirvar_mem(_MIPP_ mem,9);
    dd.a=mirvar_mem(_MIPP_ mem,10);
    dd.b=mirvar_mem(_MIPP_ mem,11);

    zzn2_copy(Ax,&Kx);

    Doubling=ecurve_fp2_add(_MIPP_ Bx,By,Bz,Ax,Ay,Az,&lam,&extra1,&extra2);

/* Get extra information from the point addition, for use in the line functions */

    if (!Doubling)
    {
        zzn2_smul(_MIPP_ Az,Py,&nn);
        zzn2_timesi(_MIPP_ &nn);
        zzn2_copy(Bx,&dd);
        zzn2_timesi(_MIPP_ &dd);
        zzn2_sadd(_MIPP_ &dd,Px,&dd);
        zzn2_mul(_MIPP_ &lam,&dd,&lam);
        zzn2_copy(By,&dd);
        zzn2_timesi(_MIPP_ &dd);
        zzn2_mul(_MIPP_ &dd,Az,&dd);
        zzn2_sub(_MIPP_ &lam,&dd,&dd);
    }
    else
    {
        zzn2_smul(_MIPP_ &extra2,Py,&nn);
        zzn2_mul(_MIPP_ &nn,Az,&nn);
        zzn2_timesi(_MIPP_ &nn);
        zzn2_timesi(_MIPP_ &Kx);
        zzn2_smul(_MIPP_ &extra2,Px,&dd);
        zzn2_add(_MIPP_ &dd,&Kx,&dd);
        zzn2_mul(_MIPP_ &lam,&dd,&lam);
        zzn2_timesi(_MIPP_ &extra1);
        zzn2_sub(_MIPP_ &lam,&extra1,&dd);

    }
    zzn2_copy(&nn,&(w->x));
    zzn2_copy(&dd,&(w->y));

#ifndef MR_STATIC      
    memkill(_MIPP_ mem,12);
#else
    memset(mem,0,MR_BIG_RESERVE(12)); 
#endif
    return;
}

void fast_tate_pairing(_MIPD_ zzn2 *Qx,zzn2 *Qy,epoint *P,big T,big fr,big delta,zzn4 *w,zzn4* res)
{
    int i,j,nb;
    zzn2 Ax,Ay,Az,Qz;
    zzn Px,Py;
   
#ifndef MR_STATIC
	char *mem=memalloc(_MIPP_ 10);
#else
    char mem[MR_BIG_RESERVE(10)]; 
    memset(mem,0,MR_BIG_RESERVE(10));
#endif

#ifdef MR_COUNT_OPS
fpa=fpc=fpx=0;
#endif

    Ax.a=mirvar_mem(_MIPP_ mem,0);
    Ax.b=mirvar_mem(_MIPP_ mem,1);
    Ay.a=mirvar_mem(_MIPP_ mem,2);
    Ay.b=mirvar_mem(_MIPP_ mem,3);
    Az.a=mirvar_mem(_MIPP_ mem,4);
    Az.b=mirvar_mem(_MIPP_ mem,5);
    Qz.a=mirvar_mem(_MIPP_ mem,6);
    Qz.b=mirvar_mem(_MIPP_ mem,7);
    Px=mirvar_mem(_MIPP_ mem,8);
    Py=mirvar_mem(_MIPP_ mem,9);

    copy(P->X,Px);
    copy(P->Y,Py);

    nres_modadd(_MIPP_ Px,Px,Px); /* removes a factor of 2 from g() */
    nres_modadd(_MIPP_ Py,Py,Py);

    zzn2_from_int(_MIPP_ 1,&Qz);
    zzn2_copy(Qx,&Ax);
    zzn2_copy(Qy,&Ay);
    zzn2_copy(&Qz,&Az);

    zzn4_from_int(_MIPP_ 1,res);

/* Simple Miller loop */
    nb=logb2(_MIPP_ T);
    for (i=nb-2;i>=0;i--)
    {
        zzn4_mul(_MIPP_ res,res,res);
        g(_MIPP_ &Ax,&Ay,&Az,&Ax,&Ay,&Az,Px,Py,w);
        zzn4_mul(_MIPP_ res,w,res);
        if (mr_testbit(_MIPP_ T,i))
        {
            g(_MIPP_ &Ax,&Ay,&Az,Qx,Qy,&Qz,Px,Py,w);
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
    zzn4_mul(_MIPP_ res,w,res);    /* res=powu(res,CF) for CF=34 */

#ifndef MR_STATIC      
    memkill(_MIPP_ mem,10);
#else
    memset(mem,0,MR_BIG_RESERVE(10)); 
#endif
}

void ecap(_MIPD_ zzn2 *Qx,zzn2 *Qy,epoint *P,big T,big fr,big delta,zzn2* r)
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

    fast_tate_pairing(_MIPP_ Qx,Qy,P,T,fr,delta,&w,&res);

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
    init_big_from_rom(T,WORDS,romp,ROMSZ,&romptr);

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

    ecap(_MIPP_ &Qx,&Qy,P,T,fr,delta,&res);
/*
#ifdef MR_COUNT_OPS
printf("fpc= %d\n",fpc);
printf("fpa= %d\n",fpa);
printf("fpx= %d\n",fpx);

fpa=fpc=fpx=0;
#endif
*/
    bigbits(_MIPP_ 160,t); 

    zzn2_powl(_MIPP_ &res,t,&res);

#ifndef MR_NO_STANDARD_IO
    zzn2_out(_MIPP_ "res= ",&res);
#endif

    ecurve_mult(_MIPP_ t,P,P);

    ecap(_MIPP_ &Qx,&Qy,P,T,fr,delta,&res);

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


