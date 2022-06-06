
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
 *   MIRACL F_p^3 support functions 
 *   mrzzn3.c
 *
 *   This code assumes p=1 mod 3
 *   Irreducible polynomial is x^3+cnr
 *
 *   Did you know that 2 is a cnr iff p cannot be written as x^2+27.y^2 ?
 */

#include <stdlib.h> 
#include "miracl.h"

BOOL zzn3_iszero(zzn3 *x)
{
    if (size(x->a)==0 && size(x->b)==0 && size(x->c)==0) return TRUE;
    return FALSE;
}

BOOL zzn3_isunity(_MIPD_ zzn3 *x)
{
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM || size(x->b)!=0 || size(x->c)!=0) return FALSE;

    if (compare(x->a,mr_mip->one)==0) return TRUE;
    return FALSE;
}

BOOL zzn3_compare(zzn3 *x,zzn3 *y)
{
    if (mr_compare(x->a,y->a)==0 && mr_compare(x->b,y->b)==0 && mr_compare(x->c,y->c)==0) return TRUE;
    return FALSE;
}

void zzn3_from_int(_MIPD_ int i,zzn3 *w)
{
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;

    MR_IN(174)
    convert(_MIPP_ i,mr_mip->w1);
    nres(_MIPP_ mr_mip->w1,w->a);
    zero(w->b); zero(w->c);
    MR_OUT
}

void zzn3_from_ints(_MIPD_ int i,int j,int k,zzn3 *w)
{
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;

    MR_IN(175)
    convert(_MIPP_ i,mr_mip->w1);
    nres(_MIPP_ mr_mip->w1,w->a);
    convert(_MIPP_ j,mr_mip->w1);
    nres(_MIPP_ mr_mip->w1,w->b);
    convert(_MIPP_ k,mr_mip->w1);
    nres(_MIPP_ mr_mip->w1,w->c);

    MR_OUT
}

void zzn3_from_zzns(big x,big y,big z,zzn3 *w)
{
    copy(x,w->a);
    copy(y,w->b);
    copy(z,w->c);
}

void zzn3_from_bigs(_MIPD_ big x,big y,big z,zzn3 *w)
{
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;

    MR_IN(176)
    nres(_MIPP_ x,w->a);
    nres(_MIPP_ y,w->b);
    nres(_MIPP_ z,w->c);
    MR_OUT
}

void zzn3_from_zzn(big x,zzn3 *w)
{
    copy(x,w->a);
    zero(w->b); zero(w->c);
}

void zzn3_from_zzn_1(big x,zzn3 *w)
{
    copy(x,w->b);
    zero(w->a); zero(w->c);
}

void zzn3_from_zzn_2(big x,zzn3 *w)
{
    copy(x,w->c);
    zero(w->a); zero(w->b);
}

void zzn3_from_big(_MIPD_ big x, zzn3 *w)
{
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;

    MR_IN(177)
    nres(_MIPP_ x,w->a);
    zero(w->b); zero(w->c);
    MR_OUT
}

void zzn3_copy(zzn3 *x,zzn3 *w)
{
    if (x==w) return;
    copy(x->a,w->a);
    copy(x->b,w->b);
    copy(x->c,w->c);
}

void zzn3_zero(zzn3 *w)
{
    zero(w->a);
    zero(w->b);
    zero(w->c);
}

void zzn3_negate(_MIPD_ zzn3 *x,zzn3 *w)
{
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;
    MR_IN(177)
    zzn3_copy(x,w);
    nres_negate(_MIPP_ w->a,w->a);
    nres_negate(_MIPP_ w->b,w->b);
    nres_negate(_MIPP_ w->c,w->c);
    MR_OUT
}

void zzn3_powq(_MIPD_ zzn3 *x,zzn3 *w)
{ /* sru - precalculated sixth root of unity */
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    MR_IN(178)
    zzn3_copy(x,w);
    nres_modmult(_MIPP_ mr_mip->sru,mr_mip->sru,mr_mip->w1); /* cube root of unity */
    nres_modmult(_MIPP_ w->b,mr_mip->w1,w->b);
    nres_modmult(_MIPP_ w->c,mr_mip->w1,w->c);
    nres_modmult(_MIPP_ w->c,mr_mip->w1,w->c);

    MR_OUT
}

void zzn3_set(_MIPD_ int cnr,big sru)
{
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    mr_mip->cnr=cnr;
    nres(_MIPP_ sru,mr_mip->sru);
}

void zzn3_add(_MIPD_ zzn3 *x,zzn3 *y,zzn3 *w)
{
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;
    MR_IN(180)
    nres_modadd(_MIPP_ x->a,y->a,w->a);
    nres_modadd(_MIPP_ x->b,y->b,w->b);

    nres_modadd(_MIPP_ x->c,y->c,w->c);

    MR_OUT
}
  
void zzn3_sadd(_MIPD_ zzn3 *x,big y,zzn3 *w)
{
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;
    MR_IN(181)
    nres_modadd(_MIPP_ x->a,y,w->a);
    MR_OUT
}              

void zzn3_sub(_MIPD_ zzn3 *x,zzn3 *y,zzn3 *w)
{
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;
    MR_IN(182)
    nres_modsub(_MIPP_ x->a,y->a,w->a);
    nres_modsub(_MIPP_ x->b,y->b,w->b);
    nres_modsub(_MIPP_ x->c,y->c,w->c);
    MR_OUT
}

void zzn3_ssub(_MIPD_ zzn3 *x,big y,zzn3 *w)
{
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;
    MR_IN(183)
    nres_modsub(_MIPP_ x->a,y,w->a);
    MR_OUT
}

void zzn3_smul(_MIPD_ zzn3 *x,big y,zzn3 *w)
{
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;
    MR_IN(184)
    if (size(x->a)!=0) nres_modmult(_MIPP_ x->a,y,w->a);
    else zero(w->a);
    if (size(x->b)!=0) nres_modmult(_MIPP_ x->b,y,w->b);
    else zero(w->b);
    if (size(x->c)!=0) nres_modmult(_MIPP_ x->c,y,w->c);
    else zero(w->c);

    MR_OUT
}

void zzn3_imul(_MIPD_ zzn3 *x,int y,zzn3 *w)
{
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;
    MR_IN(185)
    if (size(x->a)!=0) nres_premult(_MIPP_ x->a,y,w->a);
    else zero(w->a);
    if (size(x->b)!=0) nres_premult(_MIPP_ x->b,y,w->b);
    else zero(w->b);
    if (size(x->c)!=0) nres_premult(_MIPP_ x->c,y,w->c);
    else zero(w->c);

    MR_OUT
}

void zzn3_mul(_MIPD_ zzn3 *x,zzn3 *y,zzn3 *w)
{
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

    if (mr_mip->ERNUM) return;
    MR_IN(186)

    if (x==y)
    { /* Chung-Hasan SQR2 */
        nres_modmult(_MIPP_ x->a,x->a,mr_mip->w1);
        nres_modmult(_MIPP_ x->b,x->c,mr_mip->w2);
        nres_modadd(_MIPP_ mr_mip->w2,mr_mip->w2,mr_mip->w2);
        nres_modmult(_MIPP_ x->c,x->c,mr_mip->w3);
        nres_modmult(_MIPP_ x->a,x->b,mr_mip->w4);
        nres_modadd(_MIPP_ mr_mip->w4,mr_mip->w4,mr_mip->w4);

        nres_modadd(_MIPP_ x->a,x->b,mr_mip->w5);
        nres_modadd(_MIPP_ mr_mip->w5,x->c,w->c);
        nres_modmult(_MIPP_ w->c,w->c,w->c);

        nres_premult(_MIPP_ mr_mip->w2,mr_mip->cnr,w->a);
        nres_modadd(_MIPP_ mr_mip->w1,w->a,w->a);
        nres_premult(_MIPP_ mr_mip->w3,mr_mip->cnr,w->b);
        nres_modadd(_MIPP_ mr_mip->w4,w->b,w->b);

        nres_modsub(_MIPP_ w->c,mr_mip->w1,w->c);
        nres_modsub(_MIPP_ w->c,mr_mip->w2,w->c);
        nres_modsub(_MIPP_ w->c,mr_mip->w3,w->c);
        nres_modsub(_MIPP_ w->c,mr_mip->w4,w->c);
    }
    else
    {
        nres_modmult(_MIPP_ x->a,y->a,mr_mip->w1); /* Z0 */
        nres_modmult(_MIPP_ x->b,y->b,mr_mip->w2); /* Z2 */
        nres_modmult(_MIPP_ x->c,y->c,mr_mip->w3); /* Z4 */

        nres_modadd(_MIPP_ x->a,x->b,mr_mip->w4);
        nres_modadd(_MIPP_ y->a,y->b,mr_mip->w5);
        nres_modmult(_MIPP_ mr_mip->w4,mr_mip->w5,mr_mip->w6); /* Z1 */
        nres_modsub(_MIPP_ mr_mip->w6,mr_mip->w1,mr_mip->w6);
        nres_modsub(_MIPP_ mr_mip->w6,mr_mip->w2,mr_mip->w6);

        nres_modadd(_MIPP_ x->b,x->c,mr_mip->w4);
        nres_modadd(_MIPP_ y->b,y->c,mr_mip->w5);
        nres_modmult(_MIPP_ mr_mip->w4,mr_mip->w5,w->b); /* Z3 */

        nres_modadd(_MIPP_ x->a,x->c,mr_mip->w4);
        nres_modadd(_MIPP_ y->a,y->c,mr_mip->w5);

        nres_modsub(_MIPP_ w->b,mr_mip->w2,w->b);
        nres_modsub(_MIPP_ w->b,mr_mip->w3,w->b); 
        nres_premult(_MIPP_ w->b,mr_mip->cnr,w->a);

        nres_modmult(_MIPP_ mr_mip->w4,mr_mip->w5,mr_mip->w4);
        nres_modadd(_MIPP_ mr_mip->w2,mr_mip->w4,mr_mip->w2);
        nres_modsub(_MIPP_ mr_mip->w2,mr_mip->w1,mr_mip->w2);
        nres_modsub(_MIPP_ mr_mip->w2,mr_mip->w3,w->c);

        nres_modadd(_MIPP_ w->a,mr_mip->w1,w->a);
        nres_premult(_MIPP_ mr_mip->w3,mr_mip->cnr,w->b);
        nres_modadd(_MIPP_ w->b,mr_mip->w6,w->b);        
    }
    
    MR_OUT
}

void zzn3_inv(_MIPD_ zzn3 *w)
{
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;
    MR_IN(187)

    nres_modmult(_MIPP_ w->a,w->a,mr_mip->w1);
    nres_modmult(_MIPP_ w->b,w->c,mr_mip->w2);

    nres_premult(_MIPP_ mr_mip->w2,mr_mip->cnr,mr_mip->w2);
    nres_modsub(_MIPP_ mr_mip->w1,mr_mip->w2,mr_mip->w3);

    nres_modmult(_MIPP_ w->c,w->c,mr_mip->w1);
    nres_modmult(_MIPP_ w->a,w->b,mr_mip->w2);

    nres_premult(_MIPP_ mr_mip->w1,mr_mip->cnr,mr_mip->w1);
    nres_modsub(_MIPP_ mr_mip->w2,mr_mip->w1,mr_mip->w4);
    nres_negate(_MIPP_ mr_mip->w4,mr_mip->w4);

    nres_modmult(_MIPP_ w->b,w->b,mr_mip->w1);
    nres_modmult(_MIPP_ w->a,w->c,mr_mip->w2);
    nres_modsub(_MIPP_ mr_mip->w1,mr_mip->w2,mr_mip->w5);

    nres_modmult(_MIPP_ w->b,mr_mip->w5,mr_mip->w1);
    nres_modmult(_MIPP_ w->c,mr_mip->w4,mr_mip->w2);
    nres_modadd(_MIPP_ mr_mip->w2,mr_mip->w1,mr_mip->w2);
    nres_premult(_MIPP_ mr_mip->w2,mr_mip->cnr,mr_mip->w2);
    nres_modmult(_MIPP_ w->a,mr_mip->w3,mr_mip->w1);
    nres_modadd(_MIPP_ mr_mip->w1,mr_mip->w2,mr_mip->w1);

    copy(mr_mip->w3,w->a);
    copy(mr_mip->w4,w->b);
    copy(mr_mip->w5,w->c);

    redc(_MIPP_ mr_mip->w1,mr_mip->w6);
    invmodp(_MIPP_ mr_mip->w6,mr_mip->modulus,mr_mip->w6);
    nres(_MIPP_ mr_mip->w6,mr_mip->w6);

    nres_modmult(_MIPP_ w->a,mr_mip->w6,w->a);
    nres_modmult(_MIPP_ w->b,mr_mip->w6,w->b);
    nres_modmult(_MIPP_ w->c,mr_mip->w6,w->c);

    MR_OUT
}

/* divide zzn3 by 2 */

void zzn3_div2(_MIPD_ zzn3 *w)
{
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;
    MR_IN(188)
    copy(w->a,mr_mip->w1);
    if (remain(_MIPP_ mr_mip->w1,2)!=0)
        add(_MIPP_ mr_mip->w1,mr_mip->modulus,mr_mip->w1);
    subdiv(_MIPP_ mr_mip->w1,2,mr_mip->w1);
    copy(mr_mip->w1,w->a);

    copy(w->b,mr_mip->w1);
    if (remain(_MIPP_ mr_mip->w1,2)!=0)
        add(_MIPP_ mr_mip->w1,mr_mip->modulus,mr_mip->w1);
    subdiv(_MIPP_ mr_mip->w1,2,mr_mip->w1);
    copy(mr_mip->w1,w->b);

    copy(w->c,mr_mip->w1);
    if (remain(_MIPP_ mr_mip->w1,2)!=0)
        add(_MIPP_ mr_mip->w1,mr_mip->modulus,mr_mip->w1);
    subdiv(_MIPP_ mr_mip->w1,2,mr_mip->w1);
    copy(mr_mip->w1,w->c);

    MR_OUT
}

/* multiply zzn3 by i */

void zzn3_timesi(_MIPD_ zzn3 *u)
{
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;
    MR_IN(189)

    copy(u->a,mr_mip->w1);
    nres_premult(_MIPP_ u->c,mr_mip->cnr,u->a);
    copy(u->b,u->c);
    copy(mr_mip->w1,u->b);

    MR_OUT
}

/* multiply zzn3 by i^2 */

void zzn3_timesi2(_MIPD_ zzn3 *u)
{
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;
    MR_IN(224)

    copy(u->a,mr_mip->w1);
    nres_premult(_MIPP_ u->b,mr_mip->cnr,u->a);
    nres_premult(_MIPP_ u->c,mr_mip->cnr,u->b);
    copy(mr_mip->w1,u->c);

    MR_OUT
}
