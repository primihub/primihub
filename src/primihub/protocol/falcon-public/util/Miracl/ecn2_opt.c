/*
 *   MIRACL E(F_p^2) support functions 
 *   mrecn2.c
 *
 */

#include <stdlib.h> 
#include "miracl.h"
#ifdef MR_STATIC
#include <string.h>
#endif

static inline void zzn2_div2_i(zzn2 *w)
{
	moddiv2(w->a->w);
	w->a->len=2;
	moddiv2(w->b->w);
	w->b->len=2;
}

static inline void zzn2_tim2_i(zzn2 *w)
{
#ifdef MR_COUNT_OPS
fpa+=2; 
#endif

	modtim2(w->a->w);
	modtim2(w->b->w);
	w->a->len=2;
	w->b->len=2;
}

static inline void zzn2_tim3_i(zzn2 *w)
{
#ifdef MR_COUNT_OPS
fpa+=4; 
#endif

	modtim3(w->a->w);
	modtim3(w->b->w);
	w->a->len=2;
	w->b->len=2;
}

static inline void zzn2_copy_i(zzn2 *x,zzn2 *w)
{
    if (x==w) return;

    w->a->len=x->a->len;
    w->a->w[0]=x->a->w[0];
    w->a->w[1]=x->a->w[1];
    w->b->len=x->b->len;
    w->b->w[0]=x->b->w[0];
    w->b->w[1]=x->b->w[1];

}

static inline void zzn2_add_i(zzn2 *x,zzn2 *y,zzn2 *w)
{

#ifdef MR_COUNT_OPS
fpa+=2; 
#endif

    modadd(x->a->w,y->a->w,w->a->w);
	modadd(x->b->w,y->b->w,w->b->w);
	w->a->len=2;
	w->b->len=2;
}

static inline void zzn2_sub_i(zzn2 *x,zzn2 *y,zzn2 *w)
{
#ifdef MR_COUNT_OPS
fpa+=2; 
#endif
    modsub(x->a->w,y->a->w,w->a->w);
	modsub(x->b->w,y->b->w,w->b->w);
	w->a->len=2;
	w->b->len=2;
}

static inline void zzn2_timesi_i(zzn2 *u)
{
    mr_small w1[2];
	w1[0]=u->a->w[0];
	w1[1]=u->a->w[1];

    u->a->w[0]=u->b->w[0];
    u->a->w[1]=u->b->w[1];

	modneg(u->a->w);

	u->b->w[0]=w1[0];
	u->b->w[1]=w1[1];
}

static inline void zzn2_txx_i(zzn2 *u)
{
  /* multiply w by t^2 where x^2-t is irreducible polynomial for ZZn4
  
   for p=5 mod 8 t=sqrt(sqrt(-2)), qnr=-2
   for p=3 mod 8 t=sqrt(1+sqrt(-1)), qnr=-1
   for p=7 mod 8 and p=2,3 mod 5 t=sqrt(2+sqrt(-1)), qnr=-1 */
    zzn2 t;
	struct bigtype aa,bb;
	big a,b;
	mr_small w3[2],w4[2];
    a=&aa;
	b=&bb;
	a->len=2;
	b->len=2;
	a->w=w3;
	b->w=w4;
	t.a=a;
	t.b=b;
    zzn2_copy_i(u,&t);
    zzn2_timesi_i(u);
    zzn2_add_i(u,&t,u);
    zzn2_add_i(u,&t,u); 
    u->a->len=2;
	u->b->len=2;
}

static inline void zzn2_pmul_i(int i,zzn2 *x)
{
    modpmul(i,x->a->w);
    modpmul(i,x->b->w);
}

static inline void zzn2_sqr_i(zzn2 *x,zzn2 *w)
{
	static mr_small w1[2],w2[2];
#ifdef MR_COUNT_OPS
fpa+=3;
fpc+=2;
#endif
	modadd(x->a->w,x->b->w,w1);
    modsub(x->a->w,x->b->w,w2);
	modmult(x->a->w,x->b->w,w->b->w);
	modmult(w1,w2,w->a->w);   // routine that calculates (a+b)(a-b) ??
	modtim2(w->b->w);
	w->a->len=2;
	w->b->len=2;
}

static inline void zzn2_dblsub_i(zzn2 *x,zzn2 *y,zzn2 *w)
{
#ifdef MR_COUNT_OPS
fpa+=4; 
#endif
    moddblsub(w->a->w,x->a->w,y->a->w);
	moddblsub(w->b->w,x->b->w,y->b->w);
	w->a->len=2;
	w->b->len=2;
}

static inline void zzn2_mul_i(zzn2 *x,zzn2 *y,zzn2 *w)
{
	static mr_small w1[2],w2[2],w5[2];
#ifdef MR_COUNT_OPS
fpa+=5;
fpc+=3;
#endif
/*#pragma omp parallel sections
{
	#pragma omp section */
	modmult(x->a->w,y->a->w,w1);
/*	#pragma omp section */
    modmult(x->b->w,y->b->w,w2);
/*}*/
	
	modadd(x->a->w,x->b->w,w5);
	modadd(y->a->w,y->b->w,w->b->w);
    modmult(w->b->w,w5,w->b->w);
    moddblsub(w->b->w,w1,w2);  /* w->b->w - w1 -w2 */

	modsub(w1,w2,w->a->w); 

	w->a->len=2;
	w->b->len=2; 
}

void zzn2_inv_i(_MIPD_ zzn2 *w)
{
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

    if (mr_mip->ERNUM) return;
#ifdef MR_COUNT_OPS
fpc+=4;
fpa+=1;
#endif 
	MR_IN(163)
    modsqr(w->a->w,mr_mip->w1->w); 
    modsqr(w->b->w,mr_mip->w2->w); 
    modadd(mr_mip->w1->w,mr_mip->w2->w,mr_mip->w1->w);
	mr_mip->w1->len=2;

 /*   redc(_MIPP_ mr_mip->w1,mr_mip->w6); */
    copy(mr_mip->w1,mr_mip->w6);
  
    xgcd(_MIPP_ mr_mip->w6,mr_mip->modulus,mr_mip->w6,mr_mip->w6,mr_mip->w6);

/*    nres(_MIPP_ mr_mip->w6,mr_mip->w6); */

    modmult(w->a->w,mr_mip->w6->w,w->a->w);
	modneg(mr_mip->w6->w);
    modmult(w->b->w,mr_mip->w6->w,w->b->w);
    MR_OUT
}

BOOL nres_sqroot(_MIPD_ big x,big w)
{ /* w=sqrt(x) mod p. This depends on p being prime! */
    int i,t,js;
#ifdef MR_COUNT_OPS
fpc+=125; 
#endif   
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return FALSE;

    copy(x,w);
    if (size(w)==0) return TRUE; 


	copy(w,mr_mip->w1);
    for (i=0;i<25;i++)
	{
		modsqr(w->w,w->w);
		modsqr(w->w,w->w);
		modsqr(w->w,w->w);
		modsqr(w->w,w->w);
		modsqr(w->w,w->w);
	}
	w->len=2;

	modsqr(w->w,mr_mip->w2->w);
	mr_mip->w2->len=2;
	if (compare(mr_mip->w1,mr_mip->w2)!=0) {zero(w);return FALSE;}

	
	return TRUE;

}

BOOL zzn2_sqrt(_MIPD_ zzn2 *u,zzn2 *w)
{ /* sqrt(a+ib) = sqrt(a+sqrt(a*a-n*b*b)/2)+ib/(2*sqrt(a+sqrt(a*a-n*b*b)/2))
     where i*i=n */
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
#ifdef MR_COUNT_OPS
fpc+=2;
fpa+=1;
#endif 
    if (mr_mip->ERNUM) return FALSE;

    zzn2_copy(u,w);
    if (zzn2_iszero(w)) return TRUE;

    MR_IN(204)  

	modsqr(w->b->w,mr_mip->w7->w);
	modsqr(w->a->w,mr_mip->w1->w);
	modadd(mr_mip->w1->w,mr_mip->w7->w,mr_mip->w7->w);
	mr_mip->w7->len=2;

//    nres_modmult(_MIPP_ w->b,w->b,mr_mip->w7);
//    nres_modmult(_MIPP_ w->a,w->a,mr_mip->w1);
//    nres_modadd(_MIPP_ mr_mip->w7,mr_mip->w1,mr_mip->w7);

    if (!nres_sqroot(_MIPP_ mr_mip->w7,mr_mip->w7)) /* s=w7 */
    {
        zzn2_zero(w);
        MR_OUT
        return FALSE;
    }
#ifdef MR_COUNT_OPS
fpa+=1;
#endif 
    modadd(w->a->w,mr_mip->w7->w,mr_mip->w15->w);
	moddiv2(mr_mip->w15->w);
	mr_mip->w15->len=2;

//    nres_modadd(_MIPP_ w->a,mr_mip->w7,mr_mip->w15);
//    nres_div2(_MIPP_ mr_mip->w15,mr_mip->w15);

    if (!nres_sqroot(_MIPP_ mr_mip->w15,mr_mip->w15))
    {
#ifdef MR_COUNT_OPS
fpa+=1;
#endif 
    modsub(w->a->w,mr_mip->w7->w,mr_mip->w15->w);
	moddiv2(mr_mip->w15->w);
	mr_mip->w15->len=2;


   //     nres_modsub(_MIPP_ w->a,mr_mip->w7,mr_mip->w15);
   //     nres_div2(_MIPP_ mr_mip->w15,mr_mip->w15);
        if (!nres_sqroot(_MIPP_ mr_mip->w15,mr_mip->w15))
        {
            zzn2_zero(w);
            MR_OUT
            return FALSE;
        }
//		else printf("BBBBBBBBBBBBBBBBBB\n");
    }
//	else printf("AAAAAAAAAAAAAAAAAAA\n");
#ifdef MR_COUNT_OPS
fpa+=1;
#endif 
    copy(mr_mip->w15,w->a);
    modadd(mr_mip->w15->w,mr_mip->w15->w,mr_mip->w15->w);
    nres_moddiv(_MIPP_ w->b,mr_mip->w15,w->b);

    MR_OUT
    return TRUE;
}

/*
BOOL zzn2_multi_inverse(_MIPD_ int m,zzn2 *x,zzn2 *w)
{ 
    int i;
    zzn2 t1,t2;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (m==0) return TRUE;
    if (m<0) return FALSE;
    MR_IN(214)

    if (x==w)
    {
        mr_berror(_MIPP_ MR_ERR_BAD_PARAMETERS);
        MR_OUT
        return FALSE;
    }

    if (m==1)
    {
        zzn2_copy_i(&x[0],&w[0]);
        zzn2_inv_i(_MIPP_ &w[0]);

        MR_OUT
        return TRUE;
    }

    zzn2_from_int(_MIPP_ 1,&w[0]);
    zzn2_copy_i(&x[0],&w[1]);

    for (i=2;i<m;i++)
    {
        if (zzn2_isunity(_MIPP_ &x[i-1]))
            zzn2_copy_i(&w[i-1],&w[i]);
        else
            zzn2_mul_i(&w[i-1],&x[i-1],&w[i]); 
    }

    t1.a=mr_mip->w8;
    t1.b=mr_mip->w9;
    t2.a=mr_mip->w10;
    t2.b=mr_mip->w11;

    zzn2_mul_i(&w[m-1],&x[m-1],&t1);
    if (zzn2_iszero(&t1))
    {
        mr_berror(_MIPP_ MR_ERR_DIV_BY_ZERO);
        MR_OUT
        return FALSE;
    }

    zzn2_inv_i(_MIPP_ &t1);

    zzn2_copy_i(&x[m-1],&t2);
    zzn2_mul_i(&w[m-1],&t1,&w[m-1]);

    for (i=m-2;;i--)
    {
        if (i==0)
        {
            zzn2_mul_i(&t2,&t1,&w[0]);
            break;
        }
        zzn2_mul_i(&w[i],&t2,&w[i]);
        zzn2_mul_i(&w[i],&t1,&w[i]);
        if (!zzn2_isunity(_MIPP_ &x[i])) zzn2_mul_i(&t2,&x[i],&t2);
    }

    MR_OUT 
    return TRUE;   
}

*/

BOOL ecn2_iszero(ecn2 *a)
{
    if (a->marker==MR_EPOINT_INFINITY) return TRUE;
    return FALSE;
}

void ecn2_copy(ecn2 *a,ecn2 *b)
{
    zzn2_copy_i(&(a->x),&(b->x));
    zzn2_copy_i(&(a->y),&(b->y));
#ifndef MR_AFFINE_ONLY
    if (a->marker==MR_EPOINT_GENERAL) zzn2_copy_i(&(a->z),&(b->z));
#endif
    b->marker=a->marker;
}

void ecn2_zero(ecn2 *a)
{
    zzn2_zero(&(a->x)); zzn2_zero(&(a->y)); 
#ifndef MR_AFFINE_ONLY
    if (a->marker==MR_EPOINT_GENERAL) zzn2_zero(&(a->z));
#endif
    a->marker=MR_EPOINT_INFINITY;
}

BOOL ecn2_compare(_MIPD_ ecn2 *a,ecn2 *b)
{
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return FALSE;

    MR_IN(193)
    ecn2_norm(_MIPP_ a);
    ecn2_norm(_MIPP_ b);
    MR_OUT
    if (zzn2_compare(&(a->x),&(b->x)) && zzn2_compare(&(a->y),&(b->y)) && a->marker==b->marker) return TRUE;
    return FALSE;
}

void ecn2_norm(_MIPD_ ecn2 *a)
{
    zzn2 t;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
#ifndef MR_AFFINE_ONLY
    if (mr_mip->ERNUM) return;
    if (a->marker!=MR_EPOINT_GENERAL) return;

    MR_IN(194)

    zzn2_inv_i(_MIPP_ &(a->z));

    t.a=mr_mip->w3;
    t.b=mr_mip->w4;
    zzn2_copy_i(&(a->z),&t);

    zzn2_sqr_i( &(a->z),&(a->z));
    zzn2_mul_i( &(a->x),&(a->z),&(a->x));
    zzn2_mul_i( &(a->z),&t,&(a->z));
    zzn2_mul_i( &(a->y),&(a->z),&(a->y));
    zzn2_from_int(_MIPP_ 1,&(a->z));
    a->marker=MR_EPOINT_NORMALIZED;

    MR_OUT
#endif
}

void ecn2_get(_MIPD_ ecn2 *e,zzn2 *x,zzn2 *y,zzn2 *z)
{
    zzn2_copy_i(&(e->x),x);
    zzn2_copy_i(&(e->y),y);
#ifndef MR_AFFINE_ONLY
    if (e->marker==MR_EPOINT_GENERAL) zzn2_copy_i(&(e->z),z);
	else                              zzn2_from_zzn(mr_mip->one,z);
#endif
}

void ecn2_getxy(ecn2 *e,zzn2 *x,zzn2 *y)
{
    zzn2_copy_i(&(e->x),x);
    zzn2_copy_i(&(e->y),y);
}

void ecn2_getx(ecn2 *e,zzn2 *x)
{
    zzn2_copy_i(&(e->x),x);
}

inline void zzn2_conj_i(zzn2 *x,zzn2 *w)
{
    zzn2_copy_i(x,w);
	modneg(w->b->w);
}

void ecn2_psi(_MIPD_ zzn2 *psi,ecn2 *P)
{
	ecn2_norm(_MIPP_ P);
	zzn2_conj_i(&(P->x),&(P->x));
	zzn2_conj_i(&(P->y),&(P->y));
	zzn2_mul_i(&(P->x),&psi[0],&(P->x));
	zzn2_mul_i(&(P->y),&psi[1],&(P->y));

}

#ifndef MR_AFFINE_ONLY
void ecn2_getz(_MIPD_ ecn2 *e,zzn2 *z)
{
    if (e->marker==MR_EPOINT_GENERAL) zzn2_copy_i(&(e->z),z);
	else                              zzn2_from_zzn(mr_mip->one,z);
}
#endif

void ecn2_rhs(_MIPD_ zzn2 *x,zzn2 *rhs)
{ /* calculate RHS of elliptic curve equation */
    BOOL twist;
    zzn2 A,B;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;
    twist=mr_mip->TWIST;

    MR_IN(202)

    A.a=mr_mip->w10;
    A.b=mr_mip->w11;
    B.a=mr_mip->w12;
    B.b=mr_mip->w13;

    if (mr_abs(mr_mip->Asize)<MR_TOOBIG) zzn2_from_int(_MIPP_ mr_mip->Asize,&A);
    else zzn2_from_zzn(mr_mip->A,&A);

    if (mr_abs(mr_mip->Bsize)<MR_TOOBIG) zzn2_from_int(_MIPP_ mr_mip->Bsize,&B);
    else zzn2_from_zzn(mr_mip->B,&B);
    
    if (twist)
    {
        if (mr_mip->Asize==0 || mr_mip->Bsize==0)
        {
            if (mr_mip->Asize==0)
            {
                zzn2_txd(_MIPP_ &B);
            }
            if (mr_mip->Bsize==0)
            {
                zzn2_mul_i( &A,x,&B);
                zzn2_txd(_MIPP_ &B);
            }
            zzn2_negate(_MIPP_ &B,&B);
        }
        else
        {
            zzn2_txx_i(&B);
            zzn2_txx_i(&B);
            zzn2_txx_i(&B);
            zzn2_mul_i( &A,x,&A);
            zzn2_txx_i(&A);
            zzn2_txx_i(&A);
            zzn2_add_i(&B,&A,&B);
        }
    }
    else
    {
        zzn2_mul_i( &A,x,&A);
        zzn2_add_i(&B,&A,&B);
    }

    zzn2_sqr_i( x,&A);
    zzn2_mul_i( &A,x,&A);
    zzn2_add_i(&B,&A,rhs);

    MR_OUT
}

BOOL ecn2_set(_MIPD_ zzn2 *x,zzn2 *y,ecn2 *e)
{
    zzn2 lhs,rhs;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return FALSE;

    MR_IN(195)

    lhs.a=mr_mip->w10;
    lhs.b=mr_mip->w11;
    rhs.a=mr_mip->w12;
    rhs.b=mr_mip->w13;

    ecn2_rhs(_MIPP_ x,&rhs);

    zzn2_sqr_i( y,&lhs);

    if (!zzn2_compare(&lhs,&rhs))
    {
        MR_OUT
        return FALSE;
    }

    zzn2_copy_i(x,&(e->x));
    zzn2_copy_i(y,&(e->y));

    e->marker=MR_EPOINT_NORMALIZED;

    MR_OUT
    return TRUE;
}

#ifndef MR_NOSUPPORT_COMPRESSION

BOOL ecn2_setx(_MIPD_ zzn2 *x,ecn2 *e)
{
    zzn2 rhs;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return FALSE;

    MR_IN(201)

    rhs.a=mr_mip->w12;
    rhs.b=mr_mip->w13;

    ecn2_rhs(_MIPP_ x,&rhs);

    if (!zzn2_iszero(&rhs))
    {
        if (!zzn2_sqrt(_MIPP_ &rhs,&rhs)) 
        {
            MR_OUT
            return FALSE;
        }
    }

    zzn2_copy_i(x,&(e->x));
    zzn2_copy_i(&rhs,&(e->y));

    e->marker=MR_EPOINT_NORMALIZED;

    MR_OUT
    return TRUE;
}

#endif

#ifndef MR_AFFINE_ONLY
void ecn2_setxyz(zzn2 *x,zzn2 *y,zzn2 *z,ecn2 *e)
{
    zzn2_copy_i(x,&(e->x));
    zzn2_copy_i(y,&(e->y));
    zzn2_copy_i(z,&(e->z));
    e->marker=MR_EPOINT_GENERAL;
}
#endif

void ecn2_negate(_MIPD_ ecn2 *u,ecn2 *w)
{
    ecn2_copy(u,w);
    if (!w->marker!=MR_EPOINT_INFINITY)
        zzn2_negate(_MIPP_ &(w->y),&(w->y));
}
/*
BOOL ecn2_add2(_MIPD_ ecn2 *Q,ecn2 *P,zzn2 *lam,zzn2 *ex1)
{
    BOOL Doubling;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

    Doubling=ecn2_add3(_MIPP_ Q,P,lam,ex1,NULL);

    return Doubling;
}

BOOL ecn2_add1(_MIPD_ ecn2 *Q,ecn2 *P,zzn2 *lam)
{
    BOOL Doubling;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    Doubling=ecn2_add3(_MIPP_ Q,P,lam,NULL,NULL);

    return Doubling;
}
*/

BOOL ecn2_sub(_MIPD_ ecn2 *Q,ecn2 *P)
{
    BOOL Doubling;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

    ecn2_negate(_MIPP_ Q,Q);

    Doubling=ecn2_add(_MIPP_ Q,P);

    ecn2_negate(_MIPP_ Q,Q);

    return Doubling;
}

/*
static void zzn2_print(_MIPD_ char *label, zzn2 *x)
{
    char s1[1024], s2[1024];
    big a, b;

#ifdef MR_STATIC
    char mem_big[MR_BIG_RESERVE(2)];   
 	memset(mem_big, 0, MR_BIG_RESERVE(2)); 
    a=mirvar_mem(_MIPP_ mem_big,0);
    b=mirvar_mem(_MIPP_ mem_big,1);
#else
    a = mirvar(_MIPP_  0); 
    b = mirvar(_MIPP_  0); 
#endif
    redc(_MIPP_ x->a, a); otstr(_MIPP_ a, s1);
    redc(_MIPP_ x->b, b); otstr(_MIPP_ b, s2);

    printf("%s: [%s,%s]\n", label, s1, s2);
#ifndef MR_STATIC
    mr_free(a); mr_free(b);
#endif
}

static void nres_print(_MIPD_ char *label, big x)
{
    char s[1024];
    big a;

    a = mirvar(_MIPP_  0); 

    redc(_MIPP_ x, a);
    otstr(_MIPP_ a, s);

    printf("%s: %s\n", label, s);

    mr_free(a);
}
*/

BOOL ecn2_add_sub(_MIPD_ ecn2 *P,ecn2 *Q,ecn2 *PP,ecn2 *PM)
{ /* PP=P+Q, PM=P-Q. Assumes P and Q are both normalized, and P!=Q */
 #ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    zzn2 t1,t2,lam;

    if (mr_mip->ERNUM) return FALSE;

    MR_IN(211)

    if (P->marker==MR_EPOINT_GENERAL || P->marker==MR_EPOINT_GENERAL)
    { /* Sorry, some restrictions.. */
        mr_berror(_MIPP_ MR_ERR_BAD_PARAMETERS);
        MR_OUT
        return FALSE;
    }

    if (zzn2_compare(&(P->x),&(Q->x)))
    { /* P=Q or P=-Q - shouldn't happen */
        ecn2_copy(P,PP);
        ecn2_add(_MIPP_ Q,PP);
        ecn2_copy(P,PM);
        ecn2_sub(_MIPP_ Q,PM);

        MR_OUT
        return TRUE;
    }

    t1.a = mr_mip->w8;
    t1.b = mr_mip->w9; 
    t2.a = mr_mip->w10; 
    t2.b = mr_mip->w11; 
    lam.a = mr_mip->w12; 
    lam.b = mr_mip->w13;    

    zzn2_copy_i(&(P->x),&t2);
    zzn2_sub_i(&t2,&(Q->x),&t2);
    zzn2_inv_i(_MIPP_ &t2);   /* only one inverse required */
    zzn2_add_i(&(P->x),&(Q->x),&(PP->x));
    zzn2_copy_i(&(PP->x),&(PM->x));

    zzn2_copy_i(&(P->y),&t1);
    zzn2_sub_i(&t1,&(Q->y),&t1);
    zzn2_copy_i(&t1,&lam);
    zzn2_mul_i( &lam,&t2,&lam);
    zzn2_copy_i(&lam,&t1);
    zzn2_sqr_i( &t1,&t1);
    zzn2_sub_i(&t1,&(PP->x),&(PP->x));
    zzn2_copy_i(&(Q->x),&(PP->y));
    zzn2_sub_i(&(PP->y),&(PP->x),&(PP->y));
    zzn2_mul_i( &(PP->y),&lam,&(PP->y));
    zzn2_sub_i(&(PP->y),&(Q->y),&(PP->y));

    zzn2_copy_i(&(P->y),&t1);
    zzn2_add_i(&t1,&(Q->y),&t1);
    zzn2_copy_i(&t1,&lam);
    zzn2_mul_i( &lam,&t2,&lam);
    zzn2_copy_i(&lam,&t1);
    zzn2_sqr_i( &t1,&t1);
    zzn2_sub_i(&t1,&(PM->x),&(PM->x));
    zzn2_copy_i(&(Q->x),&(PM->y));
    zzn2_sub_i(&(PM->y),&(PM->x),&(PM->y));
    zzn2_mul_i( &(PM->y),&lam,&(PM->y));
    zzn2_add_i(&(PM->y),&(Q->y),&(PM->y));

    PP->marker=MR_EPOINT_NORMALIZED;
    PM->marker=MR_EPOINT_NORMALIZED;

    MR_OUT
    return TRUE;
}

BOOL ecn2_add(_MIPD_ ecn2 *Q,ecn2 *P)
{ /* P+=Q */
    BOOL Doubling=FALSE;
    BOOL twist;
    int iA;
    zzn2 t1,t2,t3,lam;
 
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

    t1.a = mr_mip->w8;
    t1.b = mr_mip->w9; 
    t2.a = mr_mip->w10; 
    t2.b = mr_mip->w11; 
    t3.a = mr_mip->w12; 
    t3.b = mr_mip->w13; 
	lam.a = mr_mip->w14;
    lam.b = mr_mip->w15;


    twist=mr_mip->TWIST;
    if (mr_mip->ERNUM) return FALSE;

    if (P->marker==MR_EPOINT_INFINITY)
    {
        ecn2_copy(Q,P);
        return Doubling;
    }
    if (Q->marker==MR_EPOINT_INFINITY) return Doubling;

    MR_IN(205)

    if (Q!=P && Q->marker==MR_EPOINT_GENERAL)
    { /* Sorry, this code is optimized for mixed addition only */
        mr_berror(_MIPP_ MR_ERR_BAD_PARAMETERS);
        MR_OUT
        return Doubling;
    }
#ifndef MR_AFFINE_ONLY
    if (mr_mip->coord==MR_AFFINE)
    {
#endif
        if (!zzn2_compare(&(P->x),&(Q->x)))
        {
            zzn2_copy_i(&(P->y),&t1);
            zzn2_sub_i(&t1,&(Q->y),&t1);
            zzn2_copy_i(&(P->x),&t2);
            zzn2_sub_i(&t2,&(Q->x),&t2);
            zzn2_copy_i(&t1,&lam);
            zzn2_inv_i(_MIPP_ &t2);
            zzn2_mul_i( &lam,&t2,&lam);

            zzn2_add_i(&(P->x),&(Q->x),&(P->x));
            zzn2_copy_i(&lam,&t1);
            zzn2_sqr_i( &t1,&t1);
            zzn2_sub_i(&t1,&(P->x),&(P->x));
           
            zzn2_copy_i(&(Q->x),&(P->y));
            zzn2_sub_i(&(P->y),&(P->x),&(P->y));
            zzn2_mul_i( &(P->y),&lam,&(P->y));
            zzn2_sub_i(&(P->y),&(Q->y),&(P->y));
        }
        else
        {   
            if (!zzn2_compare(&(P->y),&(Q->y)) || zzn2_iszero(&(P->y)))
            {
                ecn2_zero(P);
                zzn2_from_int(_MIPP_ 1,&lam);
                MR_OUT
                return Doubling;
            }
            zzn2_copy_i(&(P->x),&t1);
            zzn2_copy_i(&(P->x),&t2);
            zzn2_copy_i(&(P->x),&lam);
            zzn2_sqr_i( &lam,&lam);
          
		    zzn2_copy_i(&lam,&t3);
			zzn2_tim2_i(&t3);
            zzn2_add_i(&lam,&t3,&lam);

            if (mr_abs(mr_mip->Asize)<MR_TOOBIG) zzn2_from_int(_MIPP_ mr_mip->Asize,&t3);
            else zzn2_from_zzn(mr_mip->A,&t3);
        
            if (twist)
            {
                zzn2_txx_i(&t3);
                zzn2_txx_i(&t3);
            }
            zzn2_add_i(&lam,&t3,&lam);
			zzn2_copy_i(&(P->y),&t3);
			zzn2_tim2_i(&t3);
            zzn2_inv_i(_MIPP_ &t3);
            zzn2_mul_i( &lam,&t3,&lam);

            zzn2_add_i(&t2,&(P->x),&t2);
            zzn2_copy_i(&lam,&(P->x));
            zzn2_sqr_i( &(P->x),&(P->x));
            zzn2_sub_i(&(P->x),&t2,&(P->x));
            zzn2_sub_i(&t1,&(P->x),&t1);
            zzn2_mul_i( &t1,&lam,&t1);
            zzn2_sub_i(&t1,&(P->y),&(P->y));
        }
#ifndef MR_AFFINE_ONLY      
        zzn2_from_int(_MIPP_ 1,&(P->z));
#endif
        P->marker=MR_EPOINT_NORMALIZED;
        MR_OUT
        return Doubling;
#ifndef MR_AFFINE_ONLY
    }

    if (Q==P) Doubling=TRUE;

    if (!Doubling)
    {
        if (P->marker!=MR_EPOINT_NORMALIZED)
        {
			zzn2_sqr_i(&(P->z),&t1);
			zzn2_mul_i(&t1,&(P->z),&t2);
			zzn2_mul_i(&t1,&(Q->x),&t1);
			zzn2_mul_i(&t2,&(Q->y),&t2);
          //  zzn2_sqr_i( &(P->z),&t1);         /* 1S */
          //  zzn2_mul_i( &t3,&t1,&t3);         /* 1M */
          //  zzn2_mul_i( &t1,&(P->z),&t1);     /* 1M */
          //  zzn2_mul_i( &Yzzz,&t1,&Yzzz);     /* 1M */
        }
		else
		{
			zzn2_copy(&(Q->x),&t1);
			zzn2_copy(&(Q->y),&t2);
		}
        if (zzn2_compare(&t1,&(P->x)))  /*?*/
        {
            if (!zzn2_compare(&t2,&(P->y)) || zzn2_iszero(&(P->y)))
            {
                ecn2_zero(P);
                zzn2_from_int(_MIPP_ 1,&lam);
                MR_OUT
                return Doubling;
            }
            else Doubling=TRUE;
        }
    }

    if (!Doubling)
    { /* Addition */
		zzn2_sub_i(&t1,&(P->x),&t1);
		zzn2_sub_i(&t2,&(P->y),&t2);
		if (P->marker==MR_EPOINT_NORMALIZED) zzn2_copy_i(&t1,&(P->z));
		else zzn2_mul_i(&(P->z),&t1,&(P->z));
		zzn2_sqr_i(&t1,&t3);
		zzn2_mul_i(&t3,&t1,&lam);
		zzn2_mul_i(&t3,&(P->x),&t3);
		zzn2_copy_i(&t3,&t1);
		zzn2_tim2_i(&t1);
		zzn2_sqr_i(&t2,&(P->x));
		zzn2_dblsub_i(&t1,&lam,&(P->x));
		zzn2_sub_i(&t3,&(P->x),&t3);
		zzn2_mul_i(&t3,&t2,&t3);
		zzn2_mul_i(&lam,&(P->y),&lam);
		zzn2_sub_i(&t3,&lam,&(P->y));
    }
    else
    { /* doubling */
		if (P->marker==MR_EPOINT_NORMALIZED) zzn2_from_int(_MIPP_ 1,&t1);
		else zzn2_sqr_i(&(P->z),&t1);
        if (twist) zzn2_txx_i(&t1);
		zzn2_sub_i(&(P->x),&t1,&t2);
		zzn2_add_i(&t1,&(P->x),&t1);
		zzn2_mul_i(&t2,&t1,&t2);
		zzn2_tim3_i(&t2);
	
		zzn2_tim2_i(&(P->y));
		if (P->marker==MR_EPOINT_NORMALIZED) zzn2_copy_i(&(P->y),&(P->z));
		else zzn2_mul_i(&(P->z),&(P->y),&(P->z));
		zzn2_sqr_i(&(P->y),&(P->y));
		zzn2_mul_i(&(P->y),&(P->x),&t3);
		zzn2_sqr_i(&(P->y),&(P->y));
		zzn2_div2_i(&(P->y));
		zzn2_sqr_i(&t2,&(P->x));
		zzn2_copy_i(&t3,&t1);
		zzn2_tim2_i(&t1);
		zzn2_sub_i(&(P->x),&t1,&(P->x));
		zzn2_sub_i(&t3,&(P->x),&t1);
		zzn2_mul_i(&t1,&t2,&t1);
		zzn2_sub_i(&t1,&(P->y),&(P->y));
    }

    P->marker=MR_EPOINT_GENERAL;
    MR_OUT
    return Doubling;
#endif
}

static int calc_n(int w)
{ /* number of precomputed values needed for given window size */
    if (w==3) return 3;
    if (w==4) return 5;
    if (w==5) return 11;
    if (w==6) return 41;
    return 0;
}

/* Dahmen, Okeya and Schepers "Affine Precomputation with Sole Inversion in Elliptic Curve Cryptography" */
/* Precomputes table into T. Assumes first P has been copied to P[0], then calculates 3P, 5P, 7P etc. into T */

#define MR_DOS_2 (14+4*MR_STR_SZ_2P)

static void ecn2_dos(_MIPD_ int win,ecn2 *PT)
{
    BOOL twist;
    int i,j,sz;
    zzn2 A,B,C,D,E,T,W,d[MR_STR_SZ_2P],e[MR_STR_SZ_2P];
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

#ifndef MR_STATIC
    char *mem = memalloc(_MIPP_ MR_DOS_2);
#else
    char mem[MR_BIG_RESERVE(MR_DOS_2)];       
 	memset(mem, 0, MR_BIG_RESERVE(MR_DOS_2));   
#endif

    twist=mr_mip->TWIST;
    j=0;
    sz=calc_n(win);

    A.a= mirvar_mem(_MIPP_ mem, j++);
    A.b= mirvar_mem(_MIPP_ mem, j++);
    B.a= mirvar_mem(_MIPP_ mem, j++);
    B.b= mirvar_mem(_MIPP_ mem, j++);
    C.a= mirvar_mem(_MIPP_ mem, j++);
    C.b= mirvar_mem(_MIPP_ mem, j++);
    D.a= mirvar_mem(_MIPP_ mem, j++);
    D.b= mirvar_mem(_MIPP_ mem, j++);
    E.a= mirvar_mem(_MIPP_ mem, j++);
    E.b= mirvar_mem(_MIPP_ mem, j++);
    T.a= mirvar_mem(_MIPP_ mem, j++);
    T.b= mirvar_mem(_MIPP_ mem, j++);
    W.a= mirvar_mem(_MIPP_ mem, j++);
    W.b= mirvar_mem(_MIPP_ mem, j++);

    for (i=0;i<sz;i++)
    {
        d[i].a= mirvar_mem(_MIPP_ mem, j++);
        d[i].b= mirvar_mem(_MIPP_ mem, j++);
        e[i].a= mirvar_mem(_MIPP_ mem, j++);
        e[i].b= mirvar_mem(_MIPP_ mem, j++);
    }

    zzn2_add_i(&(PT[0].y),&(PT[0].y),&d[0]);   /* 1. d_0=2.y */
    zzn2_sqr_i(&d[0],&C);                      /* 2. C=d_0^2 */

    zzn2_sqr_i(&(PT[0].x),&T);
    zzn2_add_i(&T,&T,&A);
    zzn2_add_i(&T,&A,&T);
           
    if (mr_abs(mr_mip->Asize)<MR_TOOBIG) zzn2_from_int(_MIPP_ mr_mip->Asize,&A);
    else zzn2_from_zzn(mr_mip->A,&A);
        
    if (twist)
    {
        zzn2_txx_i(&A);
        zzn2_txx_i(&A);
    }
    zzn2_add_i(&A,&T,&A);             /* 3. A=3x^2+a */
    zzn2_copy_i(&A,&W);

    zzn2_add_i(&C,&C,&B);
    zzn2_add_i(&B,&C,&B);
    zzn2_mul_i(&B,&(PT[0].x),&B);     /* 4. B=3C.x */

    zzn2_sqr_i(&A,&d[1]);
    zzn2_sub_i(&d[1],&B,&d[1]);       /* 5. d_1=A^2-B */

    zzn2_sqr_i(&d[1],&E);             /* 6. E=d_1^2 */
    
    zzn2_mul_i(&B,&E,&B);             /* 7. B=E.B */

    zzn2_sqr_i(&C,&C);                /* 8. C=C^2 */

    zzn2_mul_i(&E,&d[1],&D);          /* 9. D=E.d_1 */

    zzn2_mul_i(&A,&d[1],&A);
    zzn2_add_i(&A,&C,&A);
    zzn2_negate(_MIPP_ &A,&A);             /* 10. A=-d_1*A-C */

    zzn2_add_i(&D,&D,&T);
    zzn2_sqr_i(&A,&d[2]);
    zzn2_sub_i(&d[2],&T,&d[2]);
    zzn2_sub_i(&d[2],&B,&d[2]);       /* 11. d_2=A^2-2D-B */

    if (sz>3)
    {
        zzn2_sqr_i(&d[2],&E);             /* 12. E=d_2^2 */

        zzn2_add_i(&T,&D,&T);
        zzn2_add_i(&T,&B,&T);
        zzn2_mul_i(&T,&E,&B);             /* 13. B=E(B+3D) */
        
        zzn2_add_i(&A,&A,&T);
        zzn2_add_i(&C,&T,&C);
        zzn2_mul_i(&C,&D,&C);             /* 14. C=D(2A+C) */

        zzn2_mul_i(&d[2],&E,&D);          /* 15. D=E.d_2 */

        zzn2_mul_i(&A,&d[2],&A);
        zzn2_add_i(&A,&C,&A);
        zzn2_negate(_MIPP_ &A,&A);             /* 16. A=-d_2*A-C */

 
        zzn2_sqr_i(&A,&d[3]);
        zzn2_sub_i(&d[3],&D,&d[3]);
        zzn2_sub_i(&d[3],&B,&d[3]);       /* 17. d_3=A^2-D-B */

        for (i=4;i<sz;i++)
        {
            zzn2_sqr_i(&d[i-1],&E);       /* 19. E=d(i-1)^2 */
            zzn2_mul_i(&B,&E,&B);         /* 20. B=E.B */
            zzn2_mul_i(&C,&D,&C);         /* 21. C=D.C */
            zzn2_mul_i(&E,&d[i-1],&D);    /* 22. D=E.d(i-1) */

            zzn2_mul_i(&A,&d[i-1],&A);
            zzn2_add_i(&A,&C,&A);
            zzn2_negate(_MIPP_ &A,&A);         /* 23. A=-d(i-1)*A-C */

            zzn2_sqr_i(&A,&d[i]);
            zzn2_sub_i(&d[i],&D,&d[i]);
            zzn2_sub_i(&d[i],&B,&d[i]);   /* 24. d(i)=A^2-D-B */
        }
    }

    zzn2_copy_i(&d[0],&e[0]);
    for (i=1;i<sz;i++)
        zzn2_mul_i(&e[i-1],&d[i],&e[i]);
       
    zzn2_copy_i(&e[sz-1],&A);
    zzn2_inv_i(_MIPP_ &A);

    for (i=sz-1;i>0;i--)
    {
        zzn2_copy_i(&d[i],&B);
        zzn2_mul_i(&e[i-1],&A,&d[i]);  
        zzn2_mul_i(&A,&B,&A);
    }
    zzn2_copy_i(&A,&d[0]);

    for (i=1;i<sz;i++)
    {
        zzn2_sqr_i(&e[i-1],&T);
        zzn2_mul_i(&d[i],&T,&d[i]); /** */
    }

    zzn2_mul_i(&W,&d[0],&W);
    zzn2_sqr_i(&W,&A);
    zzn2_sub_i(&A,&(PT[0].x),&A);
    zzn2_sub_i(&A,&(PT[0].x),&A);
    zzn2_sub_i(&(PT[0].x),&A,&B);
    zzn2_mul_i(&B,&W,&B);
    zzn2_sub_i(&B,&(PT[0].y),&B);

    zzn2_sub_i(&B,&(PT[0].y),&T);
    zzn2_mul_i(&T,&d[1],&T);

    zzn2_sqr_i(&T,&(PT[1].x));
    zzn2_sub_i(&(PT[1].x),&A,&(PT[1].x));
    zzn2_sub_i(&(PT[1].x),&(PT[0].x),&(PT[1].x));

    zzn2_sub_i(&A,&(PT[1].x),&(PT[1].y));
    zzn2_mul_i(&(PT[1].y),&T,&(PT[1].y));
    zzn2_sub_i(&(PT[1].y),&B,&(PT[1].y));

    for (i=2;i<sz;i++)
    {
        zzn2_sub_i(&(PT[i-1].y),&B,&T);
        zzn2_mul_i(&T,&d[i],&T);

        zzn2_sqr_i(&T,&(PT[i].x));
        zzn2_sub_i(&(PT[i].x),&A,&(PT[i].x));
        zzn2_sub_i(&(PT[i].x),&(PT[i-1].x),&(PT[i].x));

        zzn2_sub_i(&A,&(PT[i].x),&(PT[i].y));
        zzn2_mul_i(&(PT[i].y),&T,&(PT[i].y));
        zzn2_sub_i(&(PT[i].y),&B,&(PT[i].y));
    }
    for (i=0;i<sz;i++) PT[i].marker=MR_EPOINT_NORMALIZED;

#ifndef MR_STATIC
    memkill(_MIPP_ mem, MR_DOS_2);
#else
    memset(mem, 0, MR_BIG_RESERVE(MR_DOS_2));
#endif
}

#ifndef MR_DOUBLE_BIG
#define MR_MUL_RESERVE (1+4*MR_STR_SZ_2)
#else
#define MR_MUL_RESERVE (2+4*MR_STR_SZ_2)
#endif

int ecn2_mul(_MIPD_ big k,ecn2 *P)
{
    int i,j,nb,n,nbs,nzs,nadds;
    big h;
    ecn2 T[MR_STR_SZ_2];
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

#ifndef MR_STATIC
    char *mem = memalloc(_MIPP_ MR_MUL_RESERVE);
#else
    char mem[MR_BIG_RESERVE(MR_MUL_RESERVE)];
    memset(mem, 0, MR_BIG_RESERVE(MR_MUL_RESERVE));
#endif

    j=0;
#ifndef MR_DOUBLE_BIG
    h=mirvar_mem(_MIPP_ mem, j++);
#else
    h=mirvar_mem(_MIPP_ mem, j); j+=2;
#endif
    for (i=0;i<MR_STR_SZ_2;i++)
    {
        T[i].x.a= mirvar_mem(_MIPP_ mem, j++);
        T[i].x.b= mirvar_mem(_MIPP_ mem, j++);
        T[i].y.a= mirvar_mem(_MIPP_ mem, j++);
        T[i].y.b= mirvar_mem(_MIPP_ mem, j++);
    }

    MR_IN(207)

    ecn2_norm(_MIPP_ P);

	nadds=0;
    premult(_MIPP_ k,3,h);
    ecn2_copy(P,&T[0]);
    ecn2_dos(_MIPP_ MR_WIN_SZ_2,T);
    nb=logb2(_MIPP_ h);

    for (i=nb-2;i>=1;)
    {
        if (mr_mip->user!=NULL) (*mr_mip->user)();
        n=mr_naf_window(_MIPP_ k,h,i,&nbs,&nzs,MR_WIN_SZ_2);
 
        for (j=0;j<nbs;j++) ecn2_add(_MIPP_ P,P);
       
        if (n>0) {nadds++; ecn2_add(_MIPP_ &T[n/2],P);}
        if (n<0) {nadds++; ecn2_sub(_MIPP_ &T[(-n)/2],P);}
        i-=nbs;
        if (nzs)
        {
            for (j=0;j<nzs;j++) ecn2_add(_MIPP_ P,P);
            i-=nzs;
        }
    }

    ecn2_norm(_MIPP_ P);

    MR_OUT

#ifndef MR_STATIC
    memkill(_MIPP_ mem, MR_MUL_RESERVE);
#else
    memset(mem, 0, MR_BIG_RESERVE(MR_MUL_RESERVE));
#endif
	return nadds;
}

/* Double addition, using Joint Sparse Form */
/* R=aP+bQ */

#define MR_MUL2_JSF_RESERVE 20

int ecn2_mul2_jsf(_MIPD_ big a,ecn2 *P,big b,ecn2 *Q,ecn2 *R)
{
    int e1,h1,e2,h2,bb,nadds;
    ecn2 P1,P2,PS,PD;
    big c,d,e,f;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

#ifndef MR_STATIC
    char *mem = memalloc(_MIPP_ MR_MUL2_JSF_RESERVE);
#else
    char mem[MR_BIG_RESERVE(MR_MUL2_JSF_RESERVE)];
    memset(mem, 0, MR_BIG_RESERVE(MR_MUL2_JSF_RESERVE));
#endif

    c = mirvar_mem(_MIPP_ mem, 0);
    d = mirvar_mem(_MIPP_ mem, 1);
    e = mirvar_mem(_MIPP_ mem, 2);
    f = mirvar_mem(_MIPP_ mem, 3);
    P1.x.a= mirvar_mem(_MIPP_ mem, 4);
    P1.x.b= mirvar_mem(_MIPP_ mem, 5);
    P1.y.a= mirvar_mem(_MIPP_ mem, 6);
    P1.y.b= mirvar_mem(_MIPP_ mem, 7);
    P2.x.a= mirvar_mem(_MIPP_ mem, 8);
    P2.x.b= mirvar_mem(_MIPP_ mem, 9);
    P2.y.a= mirvar_mem(_MIPP_ mem, 10);
    P2.y.b= mirvar_mem(_MIPP_ mem, 11);
    PS.x.a= mirvar_mem(_MIPP_ mem, 12);
    PS.x.b= mirvar_mem(_MIPP_ mem, 13);
    PS.y.a= mirvar_mem(_MIPP_ mem, 14);
    PS.y.b= mirvar_mem(_MIPP_ mem, 15);
    PD.x.a= mirvar_mem(_MIPP_ mem, 16);
    PD.x.b= mirvar_mem(_MIPP_ mem, 17);
    PD.y.a= mirvar_mem(_MIPP_ mem, 18);
    PD.y.b= mirvar_mem(_MIPP_ mem, 19);

    MR_IN(206)

    ecn2_norm(_MIPP_ Q); 
    ecn2_copy(Q,&P2); 

    copy(b,d);
    if (size(d)<0) 
    {
        negify(d,d);
        ecn2_negate(_MIPP_ &P2,&P2);
    }

    ecn2_norm(_MIPP_ P); 
    ecn2_copy(P,&P1); 

    copy(a,c);
    if (size(c)<0) 
    {
        negify(c,c);
        ecn2_negate(_MIPP_ &P1,&P1);
    }

    mr_jsf(_MIPP_ d,c,e,d,f,c);   /* calculate joint sparse form */
 
    if (compare(e,f)>0) bb=logb2(_MIPP_ e)-1;
    else                bb=logb2(_MIPP_ f)-1;

    ecn2_add_sub(_MIPP_ &P1,&P2,&PS,&PD);
    ecn2_zero(R);
	nadds=0;
    while (bb>=0) 
    { /* add/subtract method */
        if (mr_mip->user!=NULL) (*mr_mip->user)();
        ecn2_add(_MIPP_ R,R);
        e1=h1=e2=h2=0;

        if (mr_testbit(_MIPP_ d,bb)) e2=1;
        if (mr_testbit(_MIPP_ e,bb)) h2=1;
        if (mr_testbit(_MIPP_ c,bb)) e1=1;
        if (mr_testbit(_MIPP_ f,bb)) h1=1;

        if (e1!=h1)
        {
            if (e2==h2)
            {
                if (h1==1) {ecn2_add(_MIPP_ &P1,R); nadds++;}
                else       {ecn2_sub(_MIPP_ &P1,R); nadds++;}
            }
            else
            {
                if (h1==1)
                {
                    if (h2==1) {ecn2_add(_MIPP_ &PS,R); nadds++;}
                    else       {ecn2_add(_MIPP_ &PD,R); nadds++;}
                }
                else
                {
                    if (h2==1) {ecn2_sub(_MIPP_ &PD,R); nadds++;}
                    else       {ecn2_sub(_MIPP_ &PS,R); nadds++;}
                }
            }
        }
        else if (e2!=h2)
        {
            if (h2==1) {ecn2_add(_MIPP_ &P2,R); nadds++;}
            else       {ecn2_sub(_MIPP_ &P2,R); nadds++;}
        }
        bb-=1;
    }
    ecn2_norm(_MIPP_ R); 

    MR_OUT
#ifndef MR_STATIC
    memkill(_MIPP_ mem, MR_MUL2_JSF_RESERVE);
#else
    memset(mem, 0, MR_BIG_RESERVE(MR_MUL2_JSF_RESERVE));
#endif
	return nadds;

}

/* General purpose multi-exponentiation engine, using inter-leaving algorithm. Calculate aP+bQ+cR+dS...
   Inputs are divided into two groups of sizes wa<4 and wb<4. For the first group if the points are fixed the 
   first precomputed Table Ta[] may be taken from ROM. For the second group if the points are variable Tb[j] will
   have to computed online. Each group has its own window size, wina (=5?) and winb (=4?) respectively. The values 
   a,b,c.. are provided in ma[] and mb[], and 3.a,3.b,3.c (as required by the NAF) are provided in ma3[] and 
   mb3[]. If only one group is required, set wb=0 and pass NULL pointers.
   */

int ecn2_muln_engine(_MIPD_ int wa,int wina,int wb,int winb,big *ma,big *ma3,big *mb,big *mb3,ecn2 *Ta,ecn2 *Tb,ecn2 *R)
{ /* general purpose interleaving algorithm engine for multi-exp */
    int i,j,tba[4],pba[4],na[4],sa[4],tbb[4],pbb[4],nb[4],sb[4],nbits,nbs,nzs;
    int sza,szb,nadds;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

    sza=calc_n(wina);
    szb=calc_n(winb);

    ecn2_zero(R);

    nbits=0;
    for (i=0;i<wa;i++) {sa[i]=exsign(ma[i]); tba[i]=0; j=logb2(_MIPP_ ma3[i]); if (j>nbits) nbits=j; }
    for (i=0;i<wb;i++) {sb[i]=exsign(mb[i]); tbb[i]=0; j=logb2(_MIPP_ mb3[i]); if (j>nbits) nbits=j; }
    
    nadds=0;
    for (i=nbits-1;i>=1;i--)
    {
        if (mr_mip->user!=NULL) (*mr_mip->user)();
        if (R->marker!=MR_EPOINT_INFINITY) ecn2_add(_MIPP_ R,R);
        for (j=0;j<wa;j++)
        { /* deal with the first group */
            if (tba[j]==0)
            {
                na[j]=mr_naf_window(_MIPP_ ma[j],ma3[j],i,&nbs,&nzs,wina);
                tba[j]=nbs+nzs;
                pba[j]=nbs;
            }
            tba[j]--;  pba[j]--; 
            if (pba[j]==0)
            {
                if (sa[j]==PLUS)
                {
                    if (na[j]>0) {ecn2_add(_MIPP_ &Ta[j*sza+na[j]/2],R); nadds++;}
                    if (na[j]<0) {ecn2_sub(_MIPP_ &Ta[j*sza+(-na[j])/2],R); nadds++;}
                }
                else
                {
                    if (na[j]>0) {ecn2_sub(_MIPP_ &Ta[j*sza+na[j]/2],R); nadds++;}
                    if (na[j]<0) {ecn2_add(_MIPP_ &Ta[j*sza+(-na[j])/2],R); nadds++;}
                }
            }         
        }
        for (j=0;j<wb;j++)
        { /* deal with the second group */
            if (tbb[j]==0)
            {
                nb[j]=mr_naf_window(_MIPP_ mb[j],mb3[j],i,&nbs,&nzs,winb);
                tbb[j]=nbs+nzs;
                pbb[j]=nbs;
            }
            tbb[j]--;  pbb[j]--; 
            if (pbb[j]==0)
            {
                if (sb[j]==PLUS)
                {
                    if (nb[j]>0) {ecn2_add(_MIPP_ &Tb[j*szb+nb[j]/2],R);  nadds++;}
                    if (nb[j]<0) {ecn2_sub(_MIPP_ &Tb[j*szb+(-nb[j])/2],R);  nadds++;}
                }
                else
                {
                    if (nb[j]>0) {ecn2_sub(_MIPP_ &Tb[j*szb+nb[j]/2],R);  nadds++;}
                    if (nb[j]<0) {ecn2_add(_MIPP_ &Tb[j*szb+(-nb[j])/2],R);  nadds++;}
                }
            }         
        }
    }
    ecn2_norm(_MIPP_ R);  
    return nadds;
}

/* Routines to support Galbraith, Lin, Scott (GLS) method for ECC */
/* requires an endomorphism psi */

/* *********************** */

/* Precompute T - first half from i.P, second half from i.psi(P) */ 

void ecn2_precomp_gls(_MIPD_ int win,ecn2 *P,zzn2 *psi,ecn2 *T)
{
    int i,j,sz;

#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

    j=0;
    sz=calc_n(win);

    MR_IN(219)

    ecn2_norm(_MIPP_ P);
    ecn2_copy(P,&T[0]);
    
    ecn2_dos(_MIPP_ win,T); /* precompute table */

    for (i=sz;i<sz+sz;i++)
    {
        ecn2_copy(&T[i-sz],&T[i]);
        ecn2_psi(_MIPP_ psi,&T[i]);
    }

    MR_OUT
}

/* Calculate a[0].P+a[1].psi(P) using interleaving method */

#define MR_MUL2_GLS_RESERVE (2+2*MR_STR_SZ_2*4)

int ecn2_mul2_gls(_MIPD_ big *a,ecn2 *P,zzn2 *psi,ecn2 *R)
{
    int i,j,nadds;
    ecn2 T[2*MR_STR_SZ_2];
    big a3[2];
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

#ifndef MR_STATIC
    char *mem = memalloc(_MIPP_ MR_MUL2_GLS_RESERVE);
#else
    char mem[MR_BIG_RESERVE(MR_MUL2_GLS_RESERVE)];       
 	memset(mem, 0, MR_BIG_RESERVE(MR_MUL2_GLS_RESERVE));   
#endif

    for (j=i=0;i<2;i++)
        a3[i]=mirvar_mem(_MIPP_ mem, j++);

    for (i=0;i<2*MR_STR_SZ_2;i++)
    {
        T[i].x.a=mirvar_mem(_MIPP_  mem, j++);
        T[i].x.b=mirvar_mem(_MIPP_  mem, j++);
        T[i].y.a=mirvar_mem(_MIPP_  mem, j++);
        T[i].y.b=mirvar_mem(_MIPP_  mem, j++);       
        T[i].marker=MR_EPOINT_INFINITY;
    }
    MR_IN(220)

    ecn2_precomp_gls(_MIPP_ MR_WIN_SZ_2,P,psi,T);

    for (i=0;i<2;i++) premult(_MIPP_ a[i],3,a3[i]); /* calculate for NAF */

    nadds=ecn2_muln_engine(_MIPP_ 0,0,2,MR_WIN_SZ_2,NULL,NULL,a,a3,NULL,T,R);

    ecn2_norm(_MIPP_ R);

    MR_OUT

#ifndef MR_STATIC
    memkill(_MIPP_ mem, MR_MUL2_GLS_RESERVE);
#else
    memset(mem, 0, MR_BIG_RESERVE(MR_MUL2_GLS_RESERVE));
#endif
    return nadds;
}

/* Calculates a[0]*P+a[1]*psi(P) + b[0]*Q+b[1]*psi(Q) 
   where P is fixed, and precomputations are already done off-line into FT
   using ecn2_precomp_gls. Useful for signature verification */

#define MR_MUL4_GLS_V_RESERVE (4+2*MR_STR_SZ_2*4)

int ecn2_mul4_gls_v(_MIPD_ big *a,ecn2 *FT,big *b,ecn2 *Q,zzn2 *psi,ecn2 *R)
{ 
    int i,j,nadds;
    ecn2 VT[2*MR_STR_SZ_2];
    big a3[2],b3[2];
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

#ifndef MR_STATIC
    char *mem = memalloc(_MIPP_ MR_MUL4_GLS_V_RESERVE);
#else
    char mem[MR_BIG_RESERVE(MR_MUL4_GLS_V_RESERVE)];       
 	memset(mem, 0, MR_BIG_RESERVE(MR_MUL4_GLS_V_RESERVE));   
#endif
    j=0;
    for (i=0;i<2;i++)
    {
        a3[i]=mirvar_mem(_MIPP_ mem, j++);
        b3[i]=mirvar_mem(_MIPP_ mem, j++);
    }
    for (i=0;i<2*MR_STR_SZ_2;i++)
    {
        VT[i].x.a=mirvar_mem(_MIPP_  mem, j++);
        VT[i].x.b=mirvar_mem(_MIPP_  mem, j++);
        VT[i].y.a=mirvar_mem(_MIPP_  mem, j++);
        VT[i].y.b=mirvar_mem(_MIPP_  mem, j++);       
        VT[i].marker=MR_EPOINT_INFINITY;
    }

    MR_IN(217)

    ecn2_precomp_gls(_MIPP_ MR_WIN_SZ_2,Q,psi,VT); /* precompute for the variable points */
    for (i=0;i<2;i++)
    { /* needed for NAF */
        premult(_MIPP_ a[i],3,a3[i]);
        premult(_MIPP_ b[i],3,b3[i]);
    }
    nadds=ecn2_muln_engine(_MIPP_ 2,MR_WIN_SZ_2P,2,MR_WIN_SZ_2,a,a3,b,b3,FT,VT,R);
    ecn2_norm(_MIPP_ R);

    MR_OUT

#ifndef MR_STATIC
    memkill(_MIPP_ mem, MR_MUL4_GLS_V_RESERVE);
#else
    memset(mem, 0, MR_BIG_RESERVE(MR_MUL4_GLS_V_RESERVE));
#endif
    return nadds;
}

/* Calculate a.P+b.Q using interleaving method. P is fixed and FT is precomputed from it */

void ecn2_precomp(_MIPD_ int win,ecn2 *P,ecn2 *T)
{
    int sz;

#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

    sz=calc_n(win);

    MR_IN(216)

    ecn2_norm(_MIPP_ P);
    ecn2_copy(P,&T[0]);
    ecn2_dos(_MIPP_ win,T); 

    MR_OUT
}

#ifndef MR_DOUBLE_BIG
#define MR_MUL2_RESERVE (2+2*MR_STR_SZ_2*4)
#else
#define MR_MUL2_RESERVE (4+2*MR_STR_SZ_2*4)
#endif

int ecn2_mul2(_MIPD_ big a,ecn2 *FT,big b,ecn2 *Q,ecn2 *R)
{
    int i,j,nadds;
    ecn2 T[2*MR_STR_SZ_2];
    big a3,b3;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

#ifndef MR_STATIC
    char *mem = memalloc(_MIPP_ MR_MUL2_RESERVE);
#else
    char mem[MR_BIG_RESERVE(MR_MUL2_RESERVE)];       
 	memset(mem, 0, MR_BIG_RESERVE(MR_MUL2_RESERVE));   
#endif

    j=0;
#ifndef MR_DOUBLE_BIG
    a3=mirvar_mem(_MIPP_ mem, j++);
	b3=mirvar_mem(_MIPP_ mem, j++);
#else
    a3=mirvar_mem(_MIPP_ mem, j); j+=2;
	b3=mirvar_mem(_MIPP_ mem, j); j+=2;
#endif    
    for (i=0;i<2*MR_STR_SZ_2;i++)
    {
        T[i].x.a=mirvar_mem(_MIPP_  mem, j++);
        T[i].x.b=mirvar_mem(_MIPP_  mem, j++);
        T[i].y.a=mirvar_mem(_MIPP_  mem, j++);
        T[i].y.b=mirvar_mem(_MIPP_  mem, j++);       
        T[i].marker=MR_EPOINT_INFINITY;
    }

    MR_IN(218)

    ecn2_precomp(_MIPP_ MR_WIN_SZ_2,Q,T);

    premult(_MIPP_ a,3,a3); 
	premult(_MIPP_ b,3,b3); 

    nadds=ecn2_muln_engine(_MIPP_ 1,MR_WIN_SZ_2P,1,MR_WIN_SZ_2,&a,&a3,&b,&b3,FT,T,R);

    ecn2_norm(_MIPP_ R);

    MR_OUT

#ifndef MR_STATIC
    memkill(_MIPP_ mem, MR_MUL2_RESERVE);
#else
    memset(mem, 0, MR_BIG_RESERVE(MR_MUL2_RESERVE));
#endif
    return nadds;
}


#ifndef MR_STATIC

BOOL ecn2_brick_init(_MIPD_ ebrick *B,zzn2 *x,zzn2 *y,big a,big b,big n,int window,int nb)
{ /* Uses Montgomery arithmetic internally              *
   * (x,y) is the fixed base                            *
   * a,b and n are parameters and modulus of the curve  *
   * window is the window size in bits and              *
   * nb is the maximum number of bits in the multiplier */
    int i,j,k,t,bp,len,bptr;
    ecn2 *table;
    ecn2 w;

#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

    if (nb<2 || window<1 || window>nb || mr_mip->ERNUM) return FALSE;

    t=MR_ROUNDUP(nb,window);

    if (t<2) return FALSE;

    MR_IN(221)

#ifndef MR_ALWAYS_BINARY
    if (mr_mip->base != mr_mip->base2)
    {
        mr_berror(_MIPP_ MR_ERR_NOT_SUPPORTED);
        MR_OUT
        return FALSE;
    }
#endif

    B->window=window;
    B->max=nb;
    table=mr_alloc(_MIPP_ (1<<window),sizeof(ecn2));
    if (table==NULL)
    {
        mr_berror(_MIPP_ MR_ERR_OUT_OF_MEMORY);   
        MR_OUT
        return FALSE;
    }
    B->a=mirvar(_MIPP_ 0);
    B->b=mirvar(_MIPP_ 0);
    B->n=mirvar(_MIPP_ 0);
    copy(a,B->a);
    copy(b,B->b);
    copy(n,B->n);

    ecurve_init(_MIPP_ a,b,n,MR_AFFINE);
    mr_mip->TWIST=TRUE;

    w.x.a=mirvar(_MIPP_ 0);
    w.x.b=mirvar(_MIPP_ 0);
    w.y.a=mirvar(_MIPP_ 0);
    w.y.b=mirvar(_MIPP_ 0);
    w.marker=MR_EPOINT_INFINITY;
    ecn2_set(_MIPP_ x,y,&w);

    table[0].x.a=mirvar(_MIPP_ 0);
    table[0].x.b=mirvar(_MIPP_ 0);
    table[0].y.a=mirvar(_MIPP_ 0);
    table[0].y.b=mirvar(_MIPP_ 0);
    table[0].marker=MR_EPOINT_INFINITY;
    table[1].x.a=mirvar(_MIPP_ 0);
    table[1].x.b=mirvar(_MIPP_ 0);
    table[1].y.a=mirvar(_MIPP_ 0);
    table[1].y.b=mirvar(_MIPP_ 0);
    table[1].marker=MR_EPOINT_INFINITY;

    ecn2_copy(&w,&table[1]);
    for (j=0;j<t;j++)
        ecn2_add(_MIPP_ &w,&w);

    k=1;
    for (i=2;i<(1<<window);i++)
    {
        table[i].x.a=mirvar(_MIPP_ 0);
        table[i].x.b=mirvar(_MIPP_ 0);
        table[i].y.a=mirvar(_MIPP_ 0);
        table[i].y.b=mirvar(_MIPP_ 0);
        table[i].marker=MR_EPOINT_INFINITY;
        if (i==(1<<k))
        {
            k++;
            ecn2_copy(&w,&table[i]);
            
            for (j=0;j<t;j++)
                ecn2_add(_MIPP_ &w,&w);
            continue;
        }
        bp=1;
        for (j=0;j<k;j++)
        {
            if (i&bp)
                ecn2_add(_MIPP_ &table[1<<j],&table[i]);
            bp<<=1;
        }
    }
    mr_free(w.x.a);
    mr_free(w.x.b);
    mr_free(w.y.a);
    mr_free(w.y.b);

/* create the table */

    len=n->len;
    bptr=0;
    B->table=mr_alloc(_MIPP_ 4*len*(1<<window),sizeof(mr_small));

    for (i=0;i<(1<<window);i++)
    {
        for (j=0;j<len;j++) B->table[bptr++]=table[i].x.a->w[j];
        for (j=0;j<len;j++) B->table[bptr++]=table[i].x.b->w[j];

        for (j=0;j<len;j++) B->table[bptr++]=table[i].y.a->w[j];
        for (j=0;j<len;j++) B->table[bptr++]=table[i].y.b->w[j];

        mr_free(table[i].x.a);
        mr_free(table[i].x.b);
        mr_free(table[i].y.a);
        mr_free(table[i].y.b);
    }
        
    mr_free(table);  

    MR_OUT
    return TRUE;
}

void ecn2_brick_end(ebrick *B)
{
    mirkill(B->n);
    mirkill(B->b);
    mirkill(B->a);
    mr_free(B->table);  
}

#else

/* use precomputated table in ROM */

void ecn2_brick_init(ebrick *B,const mr_small* rom,big a,big b,big n,int window,int nb)
{
    B->table=rom;
    B->a=a; /* just pass a pointer */
    B->b=b;
    B->n=n;
    B->window=window;  /* 2^4=16  stored values */
    B->max=nb;
}

#endif

/*
void ecn2_mul_brick(_MIPD_ ebrick *B,big e,zzn2 *x,zzn2 *y)
{
    int i,j,t,len,maxsize,promptr;
    ecn2 w,z;
 
#ifdef MR_STATIC
    char mem[MR_BIG_RESERVE(10)];
#else
    char *mem;
#endif
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

    if (size(e)<0) mr_berror(_MIPP_ MR_ERR_NEG_POWER);
    t=MR_ROUNDUP(B->max,B->window);
    
    MR_IN(116)

#ifndef MR_ALWAYS_BINARY
    if (mr_mip->base != mr_mip->base2)
    {
        mr_berror(_MIPP_ MR_ERR_NOT_SUPPORTED);
        MR_OUT
        return;
    }
#endif

    if (logb2(_MIPP_ e) > B->max)
    {
        mr_berror(_MIPP_ MR_ERR_EXP_TOO_BIG);
        MR_OUT
        return;
    }

    ecurve_init(_MIPP_ B->a,B->b,B->n,MR_BEST);
    mr_mip->TWIST=TRUE;
  
#ifdef MR_STATIC
    memset(mem,0,MR_BIG_RESERVE(10));
#else
    mem=memalloc(_MIPP_ 10);
#endif

    w.x.a=mirvar_mem(_MIPP_  mem, 0);
    w.x.b=mirvar_mem(_MIPP_  mem, 1);
    w.y.a=mirvar_mem(_MIPP_  mem, 2);
    w.y.b=mirvar_mem(_MIPP_  mem, 3);  
    w.z.a=mirvar_mem(_MIPP_  mem, 4);
    w.z.b=mirvar_mem(_MIPP_  mem, 5);      
    w.marker=MR_EPOINT_INFINITY;
    z.x.a=mirvar_mem(_MIPP_  mem, 6);
    z.x.b=mirvar_mem(_MIPP_  mem, 7);
    z.y.a=mirvar_mem(_MIPP_  mem, 8);
    z.y.b=mirvar_mem(_MIPP_  mem, 9);       
    z.marker=MR_EPOINT_INFINITY;

    len=B->n->len;
    maxsize=4*(1<<B->window)*len;

    for (i=t-1;i>=0;i--)
    {
        j=recode(_MIPP_ e,t,B->window,i);
        ecn2_add(_MIPP_ &w,&w);
        if (j>0)
        {
            promptr=4*j*len;
            init_big_from_rom(z.x.a,len,B->table,maxsize,&promptr);
            init_big_from_rom(z.x.b,len,B->table,maxsize,&promptr);
            init_big_from_rom(z.y.a,len,B->table,maxsize,&promptr);
            init_big_from_rom(z.y.b,len,B->table,maxsize,&promptr);
            z.marker=MR_EPOINT_NORMALIZED;
            ecn2_add(_MIPP_ &z,&w);
        }
    }
    ecn2_norm(_MIPP_ &w);
    ecn2_getxy(&w,x,y);
#ifndef MR_STATIC
    memkill(_MIPP_ mem,10);
#else
    memset(mem,0,MR_BIG_RESERVE(10));
#endif
    MR_OUT
}
*/

void ecn2_mul_brick_gls(_MIPD_ ebrick *B,big *e,zzn2 *psi,zzn2 *x,zzn2 *y)
{
    int i,j,k,t,len,maxsize,promptr,se[2];
    ecn2 w,z;
 
#ifdef MR_STATIC
    char mem[MR_BIG_RESERVE(10)];
#else
    char *mem;
#endif
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

    for (k=0;k<2;k++) se[k]=exsign(e[k]);

    t=MR_ROUNDUP(B->max,B->window);
    
    MR_IN(222)

#ifndef MR_ALWAYS_BINARY
    if (mr_mip->base != mr_mip->base2)
    {
        mr_berror(_MIPP_ MR_ERR_NOT_SUPPORTED);
        MR_OUT
        return;
    }
#endif

    if (logb2(_MIPP_ e[0])>B->max || logb2(_MIPP_ e[1])>B->max)
    {
        mr_berror(_MIPP_ MR_ERR_EXP_TOO_BIG);
        MR_OUT
        return;
    }

    ecurve_init(_MIPP_ B->a,B->b,B->n,MR_BEST);
    mr_mip->TWIST=TRUE;
  
#ifdef MR_STATIC
    memset(mem,0,MR_BIG_RESERVE(10));
#else
    mem=memalloc(_MIPP_ 10);
#endif

    z.x.a=mirvar_mem(_MIPP_  mem, 0);
    z.x.b=mirvar_mem(_MIPP_  mem, 1);
    z.y.a=mirvar_mem(_MIPP_  mem, 2);
    z.y.b=mirvar_mem(_MIPP_  mem, 3);       
    z.marker=MR_EPOINT_INFINITY;

    w.x.a=mirvar_mem(_MIPP_  mem, 4);
    w.x.b=mirvar_mem(_MIPP_  mem, 5);
    w.y.a=mirvar_mem(_MIPP_  mem, 6);
    w.y.b=mirvar_mem(_MIPP_  mem, 7);  
#ifndef MR_AFFINE_ONLY
    w.z.a=mirvar_mem(_MIPP_  mem, 8);
    w.z.b=mirvar_mem(_MIPP_  mem, 9); 
#endif    
    w.marker=MR_EPOINT_INFINITY;

    len=B->n->len;
    maxsize=4*(1<<B->window)*len;

    for (i=t-1;i>=0;i--)
    {
        ecn2_add(_MIPP_ &w,&w);
        for (k=0;k<2;k++)
        {
            j=recode(_MIPP_ e[k],t,B->window,i);
            if (j>0)
            {
                promptr=4*j*len;
                init_big_from_rom(z.x.a,len,B->table,maxsize,&promptr);
                init_big_from_rom(z.x.b,len,B->table,maxsize,&promptr);
                init_big_from_rom(z.y.a,len,B->table,maxsize,&promptr);
                init_big_from_rom(z.y.b,len,B->table,maxsize,&promptr);
                z.marker=MR_EPOINT_NORMALIZED;
                if (k==1) ecn2_psi(_MIPP_ psi,&z);
                if (se[k]==PLUS) ecn2_add(_MIPP_ &z,&w);
                else             ecn2_sub(_MIPP_ &z,&w);
            }
        }      
    }
    ecn2_norm(_MIPP_ &w);
    ecn2_getxy(&w,x,y);
#ifndef MR_STATIC
    memkill(_MIPP_ mem,10);
#else
    memset(mem,0,MR_BIG_RESERVE(10));
#endif
    MR_OUT
}

