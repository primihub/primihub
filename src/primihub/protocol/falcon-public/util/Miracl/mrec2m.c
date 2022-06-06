
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
 *   MIRACL routines for implementation of Elliptic Curve Cryptography over GF(2^m) 
 *   mrec2m.c
 *
 *   Curve equation is Y^2 + XY = X^3 + A.X^2 + B
 *   where A is 0 or 1
 *
 *   For algorithms used, see IEEE P1363 Standard, Appendix A
 *   unless otherwise stated.
 *
 *   New from version 5.1.1 - changed from IEEE to Lopez-Dahab coordinates
 *   See "A note on Lopez-Dahab coordinates" - Tanja Lange (eprint archive)
 *   (x,y,z) = (x/z,y/(z*z),1)
 *
 *   For supersingular curves Ordinary Projective coordinates are used.
 *   (x,y,z) = (x/z,y/z,1)
 *
 *   READ COMMENTS CAREFULLY FOR VARIOUS OPTIMIZATION SUGGESTIONS
 *
 *   No assembly language used.
 *
 *   Space can be saved by removing unneeded functions and 
 *   deleting unrequired functionality 
 */

#include <stdlib.h> 
#include "miracl.h"
#ifdef MR_STATIC
#include <string.h>
#endif

#ifndef MR_NOFULLWIDTH
                     /* This does not make sense using floating-point! */

/* Initialise with Trinomial or Pentanomial      *
 * t^m  + t^a + 1 OR t^m + t^a +t^b + t^c + 1    *
 * Set b=0 for pentanomial. a2 is usually 0 or 1 *
 * m negative indicates a super-singular curve   */

BOOL ecurve2_init(_MIPD_ int m,int a,int b,int c,big a2,big a6,BOOL check,int type)
{
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

/* catch some nonsense conditions */

    if (mr_mip->ERNUM) return FALSE;

#ifndef MR_NOKOBLITZ
    mr_mip->KOBLITZ=FALSE;
#endif

#ifndef MR_NO_SS
    mr_mip->SS=FALSE;
    if (m<0)
    { /* its a supersingular curve! */
        mr_mip->SS=TRUE;
      /*   type=MR_AFFINE;      always AFFINE */
        m=-m;
        if (size(a2)!=1) return FALSE;
        if (size(a6) >1) return FALSE;
    }
#else
    if (m<0) return FALSE;
#endif
    if (size(a2)<0) return FALSE;
    if (size(a6)<0) return FALSE;
    MR_IN(123)

    if (!prepare_basis(_MIPP_ m,a,b,c,check))
    { /* unable to set the basis */
        MR_OUT
        return FALSE;
    }    

    mr_mip->Asize=size(a2);    
    mr_mip->Bsize=size(a6);

#ifndef MR_NOKOBLITZ
#ifndef MR_NO_SS
    if (!mr_mip->SS && mr_mip->Bsize==1)
#else
    if (mr_mip->Bsize==1)
#endif
    {
        if (mr_mip->Asize==0 || mr_mip->Asize==1)
        {
            mr_mip->KOBLITZ=TRUE;
        }
    }
#endif

	if (mr_mip->Asize==MR_TOOBIG)
        copy(a2,mr_mip->A);

    if (mr_mip->Bsize==MR_TOOBIG)
        copy(a6,mr_mip->B);

#ifndef MR_AFFINE_ONLY
    if (type==MR_BEST) mr_mip->coord=MR_PROJECTIVE;
    else mr_mip->coord=type;
#else
    if (type==MR_PROJECTIVE)
        mr_berror(_MIPP_ MR_ERR_NOT_SUPPORTED);
#endif
    MR_OUT
    return TRUE;
}    

BOOL epoint2_set(_MIPD_ big x,big y,int cb,epoint *p)
{ /* initialise a point on active ecurve            *
   * if x or y == NULL, set to point at infinity    *
   * if x==y, a y co-ordinate is calculated - if    *
   * possible - and cb suggests LSB 0/1  of y/x     *
   * (which "decompresses" y). Otherwise, check     *
   * validity of given (x,y) point, ignoring cb.    *
   * Returns TRUE for valid point, otherwise FALSE. */
  
    BOOL valid;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return FALSE;

    MR_IN(125)

    if (x==NULL || y==NULL)
    {
        convert(_MIPP_ 1,p->X);
        convert(_MIPP_ 1,p->Y);
        p->marker=MR_EPOINT_INFINITY;
        MR_OUT
        return TRUE;
    }

    valid=FALSE;       

#ifndef MR_NO_SS
    if (mr_mip->SS)
    { /* Super-singular - calculate x^3+x+B */
        copy (x,p->X);
        modsquare2(_MIPP_ p->X,mr_mip->w5);           /* w5=x^2 */
        modmult2(_MIPP_ mr_mip->w5,p->X,mr_mip->w5);  /* w5=x^3 */
        add2(mr_mip->w5,p->X,mr_mip->w5);             
        incr2(mr_mip->w5,mr_mip->Bsize,mr_mip->w5);  /* w5=x^3+x+B */
        if (x!=y)
        { /* compare with y^2+y */
            copy(y,p->Y);
            modsquare2(_MIPP_ p->Y,mr_mip->w1);
            add2(mr_mip->w1,p->Y,mr_mip->w1);
            if (mr_compare(mr_mip->w1,mr_mip->w5)==0) valid=TRUE;
        }
        else
        { /* no y supplied - calculate one. Solve quadratic */
            valid=quad2(_MIPP_ mr_mip->w5,mr_mip->w5);
            incr2(mr_mip->w5,cb^parity2(mr_mip->w5),p->Y);
        }
    } 
    else
    { /* calculate x^3+Ax^2+B */
#endif
        copy(x,p->X);

        modsquare2(_MIPP_ p->X,mr_mip->w6);           /* w6=x^2 */
        modmult2(_MIPP_ mr_mip->w6,p->X,mr_mip->w5);  /* w5=x^3 */

        if (mr_mip->Asize==MR_TOOBIG)
            copy(mr_mip->A,mr_mip->w1);
        else
            convert(_MIPP_ mr_mip->Asize,mr_mip->w1);
        modmult2(_MIPP_ mr_mip->w6,mr_mip->w1,mr_mip->w0);
        add2(mr_mip->w5,mr_mip->w0,mr_mip->w5);

        if (mr_mip->Bsize==MR_TOOBIG)
            add2(mr_mip->w5,mr_mip->B,mr_mip->w5);    /* w5=x^3+Ax^2+B */
        else
            incr2(mr_mip->w5,mr_mip->Bsize,mr_mip->w5); 
        if (x!=y)
        { /* compare with y^2+xy */
            copy(y,p->Y);
            modsquare2(_MIPP_ p->Y,mr_mip->w2);
            modmult2(_MIPP_ p->Y,p->X,mr_mip->w1);
            add2(mr_mip->w1,mr_mip->w2,mr_mip->w1);
            if (mr_compare(mr_mip->w1,mr_mip->w5)==0) valid=TRUE;
        }
        else
        { /* no y supplied - calculate one. Solve quadratic */
            if (size(p->X)==0) 
            {
                if (mr_mip->Bsize==MR_TOOBIG) 
                    copy(mr_mip->B,mr_mip->w1);
                else convert(_MIPP_ mr_mip->Bsize,mr_mip->w1); 

                sqroot2(_MIPP_ mr_mip->w1,p->Y);
                valid=TRUE;
            }
            else
            {
                inverse2(_MIPP_ mr_mip->w6,mr_mip->w6);  /* 1/x^2 */
                modmult2(_MIPP_ mr_mip->w5,mr_mip->w6,mr_mip->w5);
                valid=quad2(_MIPP_ mr_mip->w5,mr_mip->w5);     
                incr2(mr_mip->w5,cb^parity2(mr_mip->w5),mr_mip->w5);
                modmult2(_MIPP_ mr_mip->w5,p->X,p->Y);
            }
        }
#ifndef MR_NO_SS
    }
#endif
    if (valid)
    {
        p->marker=MR_EPOINT_NORMALIZED;
        MR_OUT
        return TRUE;
    }
    MR_OUT
    return FALSE;
}

BOOL epoint2_norm(_MIPD_ epoint *p)
{ /* normalise a point */
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
#ifndef MR_AFFINE_ONLY
    if (mr_mip->coord==MR_AFFINE) return TRUE;
    if (p->marker!=MR_EPOINT_GENERAL) return TRUE;

    if (mr_mip->ERNUM) return FALSE;

    MR_IN(126)

    if (!inverse2(_MIPP_ p->Z,mr_mip->w8))
    {
        MR_OUT
        return FALSE;
    }

#ifndef MR_NO_SS
    if (mr_mip->SS)
    {
        modmult2(_MIPP_ p->X,mr_mip->w8,p->X);
        modmult2(_MIPP_ p->Y,mr_mip->w8,p->Y); 
    }
    else
    {
#endif
        modmult2(_MIPP_ p->X,mr_mip->w8,p->X);               /* X/Z */
        modsquare2(_MIPP_ mr_mip->w8,mr_mip->w8);            /* 1/ZZ */ 
        modmult2(_MIPP_ p->Y,mr_mip->w8,p->Y);               /* Y/ZZ */
#ifndef MR_NO_SS
    }
#endif

    convert(_MIPP_ 1,p->Z);

    p->marker=MR_EPOINT_NORMALIZED;
    MR_OUT
#endif
    return TRUE;
}

#ifndef MR_STATIC

void epoint2_getxyz(_MIPD_ epoint* p,big x,big y,big z)
{
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    convert(_MIPP_ 1,mr_mip->w1);
    if (p->marker==MR_EPOINT_INFINITY)
    {
#ifndef MR_AFFINE_ONLY
        if (mr_mip->coord==MR_AFFINE)
        { /* (0,0) = O */
#endif
            if (x!=NULL) zero(x);
            if (y!=NULL) zero(y);
#ifndef MR_AFFINE_ONLY
        }

        if (mr_mip->coord==MR_PROJECTIVE)
        { /* (1,1,0) = O */
            if (x!=NULL) copy(mr_mip->w1,x);
            if (y!=NULL) copy(mr_mip->w1,y);
        }
#endif
        if (z!=NULL) zero(z);
        return;
    }
    if (x!=NULL) copy(p->X,x);
    if (y!=NULL) copy(p->Y,y);
#ifndef MR_AFFINE_ONLY
    if (mr_mip->coord==MR_AFFINE)
    {
#endif
        if (z!=NULL) zero(z);
#ifndef MR_AFFINE_ONLY
    }

    if (mr_mip->coord==MR_PROJECTIVE)
    {
        if (z!=NULL)
        {
            if (p->marker!=MR_EPOINT_GENERAL) copy(mr_mip->w1,z);
            else copy(p->Z,z);
        }
    }
#endif
    return;
}

#endif

int epoint2_get(_MIPD_ epoint* p,big x,big y)
{ /* Get point co-ordinates in affine, normal form       *
   * (converted from projective form). If x==y, supplies *
   * x only. Return value is LSB of y/x (useful for      *
   * point compression)                                  */
    int lsb;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    
    if (p->marker==MR_EPOINT_INFINITY)
    {
        zero(x);
        zero(y);
        return 0;
    }
    if (mr_mip->ERNUM) return 0;

    MR_IN(127)

    epoint2_norm(_MIPP_ p);

    copy(p->X,x);
    copy(p->Y,mr_mip->w5);

    if (x!=y) copy(mr_mip->w5,y);
    if (size(x)==0)
    {
        MR_OUT
        return 0;
    }
#ifndef MR_NO_SS
    if (mr_mip->SS)
    {
        lsb=parity2(p->Y);
    }
    else
    {
#endif
        inverse2(_MIPP_ x,mr_mip->w5);
        modmult2(_MIPP_ mr_mip->w5,p->Y,mr_mip->w5);

        lsb=parity2(mr_mip->w5);
#ifndef MR_NO_SS
    }
#endif
    MR_OUT
    return lsb;
}

void ecurve2_double(_MIPD_ epoint *p)
{ /* double epoint on active curve */
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

    if (p->marker==MR_EPOINT_INFINITY)
    { /* 2 times infinity == infinity! */
        return;
    }
#ifndef MR_AFFINE_ONLY
    if (mr_mip->coord==MR_AFFINE)
    {
#endif

#ifndef MR_NO_SS
        if (mr_mip->SS)
        { /* super-singular */
            modsquare2(_MIPP_ p->X,p->X);
            incr2(p->X,1,mr_mip->w8);
            modsquare2(_MIPP_ p->X,p->X);
            modsquare2(_MIPP_ p->Y,p->Y);
            modsquare2(_MIPP_ p->Y,p->Y);
            add2(p->Y,p->X,p->Y);   /* y=x^4+y^4   */
            incr2(p->X,1,p->X);     /* x=x^4+1     */
            return;
        }    
#endif
        if (size(p->X)==0)
        { /* set to point at infinity */
            epoint2_set(_MIPP_ NULL,NULL,0,p);
            return;
        }
   
        inverse2(_MIPP_ p->X,mr_mip->w8);
  
        modmult2(_MIPP_ mr_mip->w8,p->Y,mr_mip->w8);
        add2(mr_mip->w8,p->X,mr_mip->w8);   /* w8 is slope m */
  
        modsquare2(_MIPP_ mr_mip->w8,mr_mip->w6);  /* w6 =m^2 */
        add2(mr_mip->w6,mr_mip->w8,mr_mip->w1);
        if (mr_mip->Asize==MR_TOOBIG)
            add2(mr_mip->w1,mr_mip->A,mr_mip->w1); 
        else
            incr2(mr_mip->w1,mr_mip->Asize,mr_mip->w1); /* w1 = x3 */

        add2(p->X,mr_mip->w1,mr_mip->w6);
        modmult2(_MIPP_ mr_mip->w6,mr_mip->w8,mr_mip->w6);
        copy(mr_mip->w1,p->X);
        add2(mr_mip->w6,mr_mip->w1,mr_mip->w6);
        add2(p->Y,mr_mip->w6,p->Y);
        return;
#ifndef MR_AFFINE_ONLY
    }

#ifndef MR_NO_SS
    if (mr_mip->SS)
    { /* super-singular */

        modsquare2(_MIPP_ p->X,p->X);
        modsquare2(_MIPP_ p->X,p->X);
        modsquare2(_MIPP_ p->Y,p->Y);
        modsquare2(_MIPP_ p->Y,p->Y);
        if (p->marker!=MR_EPOINT_NORMALIZED)
        {
            modsquare2(_MIPP_ p->Z,p->Z);
            modsquare2(_MIPP_ p->Z,p->Z);           /* z^4 */
            add2(p->Y,p->X,p->Y);                   /* y^4+x^4 */ 
            add2(p->X,p->Z,p->X);                   /* z^4+z^4 */
        }
        else
        {
            add2(p->Y,p->X,p->Y);
            incr2(p->X,1,p->X);
        }
 
        return;
    }   
#endif    

    if (size(p->X)==0)
    { /* set to infinity */
        epoint2_set(_MIPP_ NULL,NULL,0,p);
        return;
    }

    modsquare2(_MIPP_ p->X,mr_mip->w1);    /* S=X^2 */
    add2(p->Y,mr_mip->w1,p->Y);            /* U=S+Y */

    if (p->marker!=MR_EPOINT_NORMALIZED) 
    {
        modmult2(_MIPP_ p->X,p->Z,mr_mip->w4); /* T=X*Z */
        modsquare2(_MIPP_ mr_mip->w4,p->Z);    /* Z=T*T */
    }
    else 
    {
        copy(p->X,mr_mip->w4);
        copy(mr_mip->w1,p->Z);
    }

    modmult2(_MIPP_ mr_mip->w4,p->Y,mr_mip->w4); /* T=U*T */

    modsquare2(_MIPP_ p->Y,p->Y);     /* U*U */
    add2(p->Y,mr_mip->w4,p->X);       /* U*U+T */
    if (mr_mip->Asize>0)              /* X=U*U+T+AZ */
    {
        if (mr_mip->Asize>1)
        {
            if (mr_mip->Asize==MR_TOOBIG)
                copy(mr_mip->A,p->Y);
            else 
                convert(_MIPP_ mr_mip->Asize,p->Y);
            modmult2(_MIPP_ p->Y,p->Z,p->Y);
            add2(p->X,p->Y,p->X);
        }
        else
            add2(p->X,p->Z,p->X);
    }

    add2(mr_mip->w4,p->Z,mr_mip->w4);         /* Z+T */
    modmult2(_MIPP_ p->X,mr_mip->w4,p->Y);
    modsquare2(_MIPP_ mr_mip->w1,mr_mip->w1); /* S*S */
    modmult2(_MIPP_ mr_mip->w1,p->Z,mr_mip->w1);
    add2(p->Y,mr_mip->w1,p->Y);

    p->marker=MR_EPOINT_GENERAL;
#endif
}

static BOOL ecurve2_padd(_MIPD_ epoint *p,epoint *pa)
{ /* primitive add two epoints on the active ecurve pa+=p      *
   * note that if p is normalized, its Z coordinate isn't used */
 
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
#ifndef MR_AFFINE_ONLY
    if (mr_mip->coord==MR_AFFINE)
    {
#endif
        add2(p->Y,pa->Y,mr_mip->w8);
        add2(p->X,pa->X,mr_mip->w6);
        if (size(mr_mip->w6)==0)
        {  /* divide by zero */
            if (size(mr_mip->w8)==0)
            { /* should have doubled! */
                return FALSE;
            }
            else
            { /* point at infinity */
                epoint2_set(_MIPP_ NULL,NULL,0,pa);
                return TRUE;
            }
        }
        inverse2(_MIPP_ mr_mip->w6,mr_mip->w5);

        modmult2(_MIPP_ mr_mip->w8,mr_mip->w5,mr_mip->w8); /* w8=m */
        modsquare2(_MIPP_ mr_mip->w8,mr_mip->w5);          /* m^2  */
#ifndef MR_NO_SS
        if (mr_mip->SS)
        {
             add2(pa->X,p->X,pa->X);
             add2(pa->X,mr_mip->w5,pa->X);

             add2(pa->X,p->X,pa->Y);
             modmult2(_MIPP_ pa->Y,mr_mip->w8,pa->Y);
             add2(pa->Y,p->Y,pa->Y);
             incr2(pa->Y,1,pa->Y);
        }
        else
        {
#endif
            add2(mr_mip->w5,mr_mip->w8,mr_mip->w5);
            add2(mr_mip->w5,mr_mip->w6,mr_mip->w5);
            if (mr_mip->Asize==MR_TOOBIG)
                add2(mr_mip->w5,mr_mip->A,mr_mip->w5);
            else
                incr2(mr_mip->w5,mr_mip->Asize,mr_mip->w5); /* w5=x3 */
        
            add2(pa->X,mr_mip->w5,mr_mip->w6);
            modmult2(_MIPP_ mr_mip->w6,mr_mip->w8,mr_mip->w6);
            copy(mr_mip->w5,pa->X);
            add2(mr_mip->w6,mr_mip->w5,mr_mip->w6);
            add2(pa->Y,mr_mip->w6,pa->Y);
#ifndef MR_NO_SS
        }
#endif
        pa->marker=MR_EPOINT_NORMALIZED;
        return TRUE;
#ifndef MR_AFFINE_ONLY
    }
#ifndef MR_NO_SS
    if (mr_mip->SS)
    { /* pa+=p */
        if (p->marker!=MR_EPOINT_NORMALIZED)
        {
            modmult2(_MIPP_ pa->Y,p->Z,mr_mip->w4);       /* w4=y1.z2 */
            modmult2(_MIPP_ pa->X,p->Z,mr_mip->w1);       /* w1=x1.z2 */
            if (pa->marker==MR_EPOINT_NORMALIZED) copy(p->Z,mr_mip->w2);
            else modmult2(_MIPP_ pa->Z,p->Z,mr_mip->w2);  /* w2=z1.z2 */
        }
        else
        {
            if (pa->marker==MR_EPOINT_NORMALIZED) convert(_MIPP_ 1,mr_mip->w2);
            else copy(pa->Z,mr_mip->w2);
            copy(pa->Y,mr_mip->w4);
            copy(pa->X,mr_mip->w1);
        }

        if (pa->marker!=MR_EPOINT_NORMALIZED)
        {
            modmult2(_MIPP_ p->Y,pa->Z,mr_mip->w8);        /* w8=y2.z1 */
            modmult2(_MIPP_ p->X,pa->Z,mr_mip->w5);        /* w5=x2.z1 */
        }
        else
        {
            copy(p->Y,mr_mip->w8);
            copy(p->X,mr_mip->w5);
        }

        add2(mr_mip->w4,mr_mip->w8,mr_mip->w8);             /* A=y2.z1+y1.z2 */
        add2(mr_mip->w1,mr_mip->w5,mr_mip->w1);             /* B=x2.z1+x1.z2 */


		if (size(mr_mip->w1)==0)
		{
			if (mr_compare(mr_mip->w2,mr_mip->w8)==0)
			{ /* point at infinity */
                epoint2_set(_MIPP_ NULL,NULL,0,pa);
                return TRUE;
			}
			else return FALSE; /* should have doubled */
		}
		
/*
        if (size(mr_mip->w8)==0) 
        {
            if (size(mr_mip->w1)==0)
            { 
                return FALSE;
            }
            else
            { 
                epoint2_set(_MIPP_ NULL,NULL,0,pa);
                return TRUE;
            }
        }
*/

        modsquare2(_MIPP_ mr_mip->w1,pa->X);               /* X=B^2 */
        modmult2(_MIPP_ pa->X,mr_mip->w1,pa->Z);           /* Z=B^3 */
        modmult2(_MIPP_ pa->X,mr_mip->w5,pa->Y);           /* Y=x2.z1.B^2 */
        
        modsquare2(_MIPP_ mr_mip->w8,mr_mip->w3);          /* w3=A^2 */
        modmult2(_MIPP_ mr_mip->w3,mr_mip->w2,mr_mip->w5); /* w5=A^2.z1.z2 */

        add2(pa->Y,mr_mip->w5,pa->Y);                      /* Y=x2.z1.B^2 + A^2.z1.z2 */
        modmult2(_MIPP_ pa->Y,mr_mip->w8,pa->Y);           /* Y=A.Y */
        modsquare2(_MIPP_ pa->X,pa->X);                    /* X=B^4 */

        modmult2(_MIPP_ mr_mip->w1,mr_mip->w5,mr_mip->w8); /* w8=B*w5 */
        add2(pa->X,mr_mip->w8,pa->X);                      /* X finished */
        modmult2(_MIPP_ mr_mip->w4,pa->Z,mr_mip->w1);      /* B^3.y1.z2 */
        add2(pa->Y,mr_mip->w1,pa->Y);
        modmult2(_MIPP_ pa->Z,mr_mip->w2,pa->Z);
        add2(pa->Y,pa->Z,pa->Y);

        pa->marker=MR_EPOINT_GENERAL;
        return TRUE;
    }
#endif

    if (p->marker!=MR_EPOINT_NORMALIZED)
    {
        if (pa->marker!=MR_EPOINT_NORMALIZED)
            modmult2(_MIPP_ p->X,pa->Z,mr_mip->w1);  /* A1=x1.z2 =w1 */
        else
            copy(p->X,mr_mip->w1);

        modmult2(_MIPP_ pa->X,p->Z,pa->X);          /* A2=x2.z1 =X3 */
        add2(mr_mip->w1,pa->X,mr_mip->w2);          /* C= A1+A2 =w2 */

        modsquare2(_MIPP_ mr_mip->w1,mr_mip->w3);   /* B1=A1*A1 =w3 */
        modsquare2(_MIPP_ pa->X,mr_mip->w4);        /* B2=A2*A2 =w4 */
        add2(mr_mip->w3,mr_mip->w4,mr_mip->w5);     /* D=B1+B2 =w5 */

        if (pa->marker!=MR_EPOINT_NORMALIZED)
        {
            modsquare2(_MIPP_ pa->Z,mr_mip->w6);
            modmult2(_MIPP_ mr_mip->w6,p->Y,mr_mip->w6);  /* E1=y1.z2^2 = w6 */
        }
        else
            copy(p->Y,mr_mip->w6);

        modsquare2(_MIPP_ p->Z,mr_mip->w8);
        modmult2(_MIPP_ mr_mip->w8,pa->Y,mr_mip->w8); /* E2=y2.z1^2 = w8 */

        add2(mr_mip->w3,mr_mip->w6,mr_mip->w3); /* E1+B1 = w3 */
        add2(mr_mip->w4,mr_mip->w8,mr_mip->w4); /* E2+B2 = w4 */

        add2(mr_mip->w8,mr_mip->w6,mr_mip->w8);     /* F=E1+E2 */

        if (size(mr_mip->w2)==0)
        {
            if (size(mr_mip->w8)==0)
            { /* should have doubled */
                return FALSE;
            }
            else
            {
                epoint2_set(_MIPP_ NULL,NULL,0,pa);
                return TRUE;
            }
        }

        modmult2(_MIPP_ mr_mip->w8,mr_mip->w2,mr_mip->w8); /* G=CF */
        if (pa->marker!=MR_EPOINT_NORMALIZED)
            modmult2(_MIPP_ pa->Z,p->Z,pa->Z);
        else
            copy(p->Z,pa->Z);

        modmult2(_MIPP_  pa->Z,mr_mip->w5,pa->Z);           /* Z3=z1.z2.D */

        modmult2(_MIPP_ mr_mip->w1,mr_mip->w4,mr_mip->w2);
        modmult2(_MIPP_ pa->X,mr_mip->w3,pa->X);
        add2(pa->X,mr_mip->w2,pa->X);              /* x3 = A1(E2+B2)+A2(E1+B1) */     

        modmult2(_MIPP_ mr_mip->w1,mr_mip->w8,mr_mip->w1);  /* A1*G */
        modmult2(_MIPP_ mr_mip->w6,mr_mip->w5,mr_mip->w6);  /* E1*D */
        add2(mr_mip->w1,mr_mip->w6,pa->Y);
        modmult2(_MIPP_ pa->Y,mr_mip->w5,pa->Y);
        add2(mr_mip->w8,pa->Z,mr_mip->w8);
        modmult2(_MIPP_ mr_mip->w8,pa->X,mr_mip->w8);
        add2(pa->Y,mr_mip->w8,pa->Y);
    }
    else
    {
        if (pa->marker!=MR_EPOINT_NORMALIZED) 
        {
            modsquare2(_MIPP_ pa->Z,mr_mip->w1);  
            modmult2(_MIPP_ mr_mip->w1,p->Y,mr_mip->w1);
            add2(mr_mip->w1,pa->Y,mr_mip->w1);               /* U=z2^2.y1 + y2 */
            modmult2(_MIPP_ pa->Z,p->X,mr_mip->w2);
            add2(mr_mip->w2,pa->X,mr_mip->w2);               /* S=z2x1+x2 */
        }
        else
        {
            add2(p->Y,pa->Y,mr_mip->w1);
            add2(p->X,pa->X,mr_mip->w2);
        }

        if (size(mr_mip->w2)==0)
        {
            if (size(mr_mip->w1)==0)
            { /* should have doubled! */
                return FALSE;
            }
            else
            {
                epoint2_set(_MIPP_ NULL,NULL,0,pa);
                return TRUE;
            }
        }

        if (pa->marker!=MR_EPOINT_NORMALIZED)  
            modmult2(_MIPP_ pa->Z,mr_mip->w2,mr_mip->w3);    /* T=z2.S */
        else
            copy(mr_mip->w2,mr_mip->w3);

        modsquare2(_MIPP_ mr_mip->w3,pa->Z);             /* z3=T^2 */

        modmult2(_MIPP_ pa->Z,p->X,mr_mip->w4);          /* V=z3.x1 */
        add2(p->X,p->Y,mr_mip->w5);                      /* C=x1+y1 */
        modsquare2(_MIPP_ mr_mip->w1,pa->X);
        modsquare2(_MIPP_ mr_mip->w2,mr_mip->w2);          /* S^2 */
        add2(mr_mip->w2,mr_mip->w1,mr_mip->w2);
        if (mr_mip->Asize>0)                               /* T(U+S^2+BT) */
        {
            if (mr_mip->Asize>1)
            {      
                if (mr_mip->Asize==MR_TOOBIG)
                    copy(mr_mip->A,mr_mip->w6);
                else 
                    convert(_MIPP_ mr_mip->Asize,mr_mip->w6);
                modmult2(_MIPP_ mr_mip->w6,mr_mip->w3,mr_mip->w6);
                add2(mr_mip->w2,mr_mip->w6,mr_mip->w2);
            }
            else
                add2(mr_mip->w2,mr_mip->w3,mr_mip->w2);
        }
        modmult2(_MIPP_ mr_mip->w2,mr_mip->w3,mr_mip->w2);
        add2(pa->X,mr_mip->w2,pa->X);

        add2(mr_mip->w4,pa->X,mr_mip->w4);                 /* V+X */
        modmult2(_MIPP_ mr_mip->w3,mr_mip->w1,mr_mip->w3); /* T*U */
        add2(pa->Z,mr_mip->w3,pa->Y);                      /* Z3+T*U */
        modmult2(_MIPP_ pa->Y,mr_mip->w4,pa->Y);
        modsquare2(_MIPP_ pa->Z,mr_mip->w1);
        modmult2(_MIPP_ mr_mip->w1,mr_mip->w5,mr_mip->w1); /*z3^2.C */
        add2(pa->Y,mr_mip->w1,pa->Y);
    }

    pa->marker=MR_EPOINT_GENERAL;
    return TRUE;
#endif
}

void epoint2_copy(epoint *a,epoint *b)
{   
    if (a==b) return;
    copy(a->X,b->X);
    copy(a->Y,b->Y);

#ifndef MR_AFFINE_ONLY
    if (a->marker==MR_EPOINT_GENERAL) copy(a->Z,b->Z);
#endif
    b->marker=a->marker;
    return;
}

BOOL epoint2_comp(_MIPD_ epoint *a,epoint *b)
{
    int ia,ib;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return FALSE;
    if (a==b) return TRUE;

    if (a->marker==MR_EPOINT_INFINITY)
    {
        if (b->marker==MR_EPOINT_INFINITY) return TRUE;
        else return FALSE;
    } 
    if (b->marker==MR_EPOINT_INFINITY)
        return FALSE;

    MR_IN(128)

    ia=epoint2_get(_MIPP_ a,mr_mip->w9,mr_mip->w9);
    ib=epoint2_get(_MIPP_ b,mr_mip->w10,mr_mip->w10);

    MR_OUT
    if (ia==ib && mr_compare(mr_mip->w9,mr_mip->w10)==0) return TRUE;
    return FALSE;
}

big ecurve2_add(_MIPD_ epoint *p,epoint *pa)
{  /* pa=pa+p; */
   /* An ephemeral pointer to the line slope is returned *
    * only if curve is super-singular                    */
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return NULL;

    MR_IN(129)

    if (p==pa) 
    {
        ecurve2_double(_MIPP_ pa);
        MR_OUT
        return mr_mip->w8;
    }
    if (pa->marker==MR_EPOINT_INFINITY)
    {
        epoint2_copy(p,pa);
        MR_OUT 
        return NULL;
    }
    if (p->marker==MR_EPOINT_INFINITY) 
    {
        MR_OUT
        return NULL;
    }
    if (!ecurve2_padd(_MIPP_ p,pa)) ecurve2_double(_MIPP_ pa);
    MR_OUT
    return mr_mip->w8;
}

void epoint2_negate(_MIPD_ epoint *p)
{ /* negate a point */
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;
    if (p->marker==MR_EPOINT_INFINITY) return;
    MR_IN(130)
#ifndef MR_AFFINE_ONLY
    if (p->marker==MR_EPOINT_GENERAL)
    {
#ifndef MR_NO_SS
        if (mr_mip->SS)
        {
             add2(p->Y,p->Z,p->Y);
        }
        else
        {
#endif
            modmult2(_MIPP_ p->X,p->Z,mr_mip->w1);
            add2(p->Y,mr_mip->w1,p->Y);
#ifndef MR_NO_SS
        }
#endif
    }
    else 
    {
#endif
#ifndef MR_NO_SS
        if (mr_mip->SS)  incr2(p->Y,1,p->Y);
        else    
#endif            
            add2(p->Y,p->X,p->Y);
#ifndef MR_AFFINE_ONLY                        
    }
#endif
    MR_OUT
}

big ecurve2_sub(_MIPD_ epoint *p,epoint *pa)
{
    big r;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return NULL;

    MR_IN(131)

    if (p==pa)
    {
        epoint2_set(_MIPP_ NULL,NULL,0,pa);
        MR_OUT
        return NULL;
    } 
    if (p->marker==MR_EPOINT_INFINITY) 
    {
        MR_OUT
        return NULL;
    }

    epoint2_negate(_MIPP_ p);
    r=ecurve2_add(_MIPP_ p,pa);
    epoint2_negate(_MIPP_ p);

    MR_OUT
    return r;
}

#ifndef MR_NO_ECC_MULTIADD
#ifndef MR_STATIC

void ecurve2_multi_add(_MIPD_ int m,epoint **x,epoint **w)
{ /* adds m points together simultaneously, w[i]+=x[i] */
    int i,*flag;
    big *A,*B,*C;
    char *mem;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;

    MR_IN(132)
#ifndef MR_NO_SS
    if (mr_mip->SS)
    {
        for (i=0;i<m;i++) ecurve2_add(_MIPP_ x[i],w[i]);
        MR_OUT
        return;
    }
#endif
#ifndef MR_AFFINE_ONLY
    if (mr_mip->coord==MR_AFFINE)
    {
#endif
        A=(big *)mr_alloc(_MIPP_ m,sizeof(big));
        B=(big *)mr_alloc(_MIPP_ m,sizeof(big));
        C=(big *)mr_alloc(_MIPP_ m,sizeof(big));
        flag=(int *)mr_alloc(_MIPP_ m,sizeof(int));

        convert(_MIPP_ 1,mr_mip->w3);  /* unity */
        mem=(char *)memalloc(_MIPP_ 3*m);

        for (i=0;i<m;i++)
        {
            A[i]=mirvar_mem(_MIPP_ mem,3*i);
            B[i]=mirvar_mem(_MIPP_ mem,3*i+1);
            C[i]=mirvar_mem(_MIPP_ mem,3*i+2);
            flag[i]=0;
            if (mr_compare(x[i]->X,w[i]->X)==0 && mr_compare(x[i]->Y,w[i]->Y)==0)
            { /* doubling */
                if (x[i]->marker==MR_EPOINT_INFINITY || size(x[i]->Y)==0)
                {
                    flag[i]=1;     /* result is infinity */
                    copy(mr_mip->w3,B[i]);
                    continue;
                }
                modsquare2(_MIPP_ x[i]->X,A[i]);
                add2(A[i],x[i]->Y,A[i]);
                copy(x[i]->X,B[i]);
            }
            else
            {
                if (x[i]->marker==MR_EPOINT_INFINITY)
                {
                    flag[i]=2;                    /* w[i] unchanged */
                    copy(mr_mip->w3,B[i]);
                    continue;
                }
                if (w[i]->marker==MR_EPOINT_INFINITY)
                {
                    flag[i]=3;                    /* w[i]=x[i] */
                    copy(mr_mip->w3,B[i]);
                    continue;
                }
                add2(x[i]->X,w[i]->X,B[i]);
                if (size(B[i])==0)
                { /* point at infinity */
                    flag[i]=1;                /* result is infinity */
                    copy(mr_mip->w3,B[i]);
                    continue;
                }
                add2(x[i]->Y,w[i]->Y,A[i]);
            }
        }

        multi_inverse2(_MIPP_ m,B,C); /* one inversion only */
        for (i=0;i<m;i++)
        {
            if (flag[i]==1)
            { /* point at infinity */
                epoint2_set(_MIPP_ NULL,NULL,0,w[i]);
                continue;
            }
            if (flag[i]==2)
            {
                continue;
            }
            if (flag[i]==3)
            {
                epoint2_copy(x[i],w[i]);
                continue;
            }
            modmult2(_MIPP_ A[i],C[i],mr_mip->w8);
            modsquare2(_MIPP_ mr_mip->w8,mr_mip->w6); /* m^2 */
            add2(mr_mip->w6,mr_mip->w8,mr_mip->w6);
            add2(mr_mip->w6,x[i]->X,mr_mip->w6);
            add2(mr_mip->w6,w[i]->X,mr_mip->w6);
            if (mr_mip->Asize==MR_TOOBIG)
                add2(mr_mip->w6,mr_mip->A,mr_mip->w6);
            else
                incr2(mr_mip->w6,mr_mip->Asize,mr_mip->w6);

            add2(w[i]->X,mr_mip->w6,mr_mip->w2);
            modmult2(_MIPP_ mr_mip->w2,mr_mip->w8,mr_mip->w2);
            add2(mr_mip->w2,mr_mip->w6,mr_mip->w2);
            add2(mr_mip->w2,w[i]->Y,w[i]->Y);
            copy(mr_mip->w6,w[i]->X);

            w[i]->marker=MR_EPOINT_NORMALIZED;

        }
        memkill(_MIPP_ mem,3*m);
        mr_free(flag);
        mr_free(C); mr_free(B); mr_free(A);
#ifndef MR_AFFINE_ONLY
    }
    else
    { /* no speed-up for projective coordinates */
        for (i=0;i<m;i++) ecurve2_add(_MIPP_ x[i],w[i]);
    }
#endif
    MR_OUT
}

#endif
#endif

#ifndef MR_NOKOBLITZ

static void frobenius(_MIPD_ epoint *P)
{
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (P->marker==MR_EPOINT_INFINITY) return;

    modsquare2(_MIPP_ P->X,P->X);
    modsquare2(_MIPP_ P->Y,P->Y);
#ifndef MR_AFFINE_ONLY
    if (mr_mip->coord==MR_PROJECTIVE && P->marker==MR_EPOINT_GENERAL) modsquare2(_MIPP_ P->Z,P->Z);
#endif
}

/* creates a tnaf from a+tau.b into tm[.] */

static int itnaf(int mu,int a,int b,signed char *tm)
{
    int u,t,len=0;
    int r0=a;
    int r1=b;

    while (r0!=0 || r1!=0)
    {
        if ((r0%2)!=0)
        {
            t=(r0-2*r1)%4; if (t<0) t+=4;
            u=2-t;  
            r0-=u;
        }   
        else u=0;
        tm[len++]=u;
        t=r0;
        if (mu==1) r0=r1+r0/2;
        else       r0=r1-r0/2;
        r1=-t/2;
    }
    return len;
}

/* Here we use the post-processing algorithm of Lutz and Hasan */
/* rather than the more well known Solinas method - see */
/* http://vlsi.uwaterloo.ca/~ahasan/web_papers/technical_reports/web_Hi_Per_ECC.pdf */ 

static int tnaf(_MIPD_ big e,big hp,big hn)
{
    int n,u,t,i,j,len,mu,count;
    signed char tm[8];
    BOOL wrapped=FALSE;
    int m,a;

#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

#ifdef MR_STATIC
    signed char tn[MIRACL*MR_STATIC+8];
#else
    signed char *tn=(signed char *)mr_alloc(_MIPP_ mr_mip->M+8,1);
#endif

    m=mr_mip->M;
    a=mr_mip->Asize;

    if (a==0) mu=-1;
    else      mu=1;

    copy(e,mr_mip->w1);
    zero(mr_mip->w2);
    for (i=0;i<m+8;i++) tn[i]=0;
    i=0;
    while (size(mr_mip->w1)!=0 || size(mr_mip->w2)!=0)
    {
        if (remain(_MIPP_ mr_mip->w1,2)!=0)
        {
            premult(_MIPP_ mr_mip->w2,2,mr_mip->w3);
            subtract(_MIPP_ mr_mip->w1,mr_mip->w3,mr_mip->w3);
            t=remain(_MIPP_ mr_mip->w3,4); if (t<0) t+=4;
            u=2-t;
            decr(_MIPP_ mr_mip->w1,u,mr_mip->w1);
            tn[i]+=u;
        }

        subdiv(_MIPP_ mr_mip->w1,2,mr_mip->w3);
        if (mu>0) add(_MIPP_ mr_mip->w2,mr_mip->w3,mr_mip->w1);
        else      subtract(_MIPP_ mr_mip->w2,mr_mip->w3,mr_mip->w1);
        negify(mr_mip->w3,mr_mip->w2);
        i++;
        if (i==m)
        {
            i=0;
            wrapped=TRUE;
        }
    }
    len=i;
    count=0;
    if (wrapped) forever
    {
        len=m;

        for (i=0;i<len;)
        {   
            if (tn[i]==0) 
            {
                i++;
                continue;
            }
            if (tn[i+1]==0)
            {
                if (tn[i]==1 || tn[i]==-1)
                {
                    i+=2;
                    continue;
                }
            }
            n=itnaf(mu,tn[i],tn[i+1],tm);  

            tn[i]=tm[0]; tn[i+1]=tm[1];
            for (j=2;j<n;j++)
                tn[i+j]+=tm[j];
            if (i+n>=len) len=i+n;
            i++;
        }
        count++;
        if (count<3 && len>m)
        { 
            for (i=m;i<len;i++)
            {
                tn[i-m]+=tn[i];
                tn[i]=0;
            }
            continue;
        }
        break;
    }

    zero(hp);
    zero(hn);   
    for (i=0;i<len;i++)
    {
        if (tn[i]==1)
        {
            expb2(_MIPP_ i,mr_mip->w3);
            add(_MIPP_ hp,mr_mip->w3,hp);
        }
        if (tn[i]==-1)
        {
            expb2(_MIPP_ i,mr_mip->w3);
            add(_MIPP_ hn,mr_mip->w3,hn);
        }
        tn[i]=0;
    }
    for (i=0;i<8;i++) tm[i]=0;
#ifndef MR_STATIC
    mr_free(tn);
#endif
    return len;
}

#endif

BOOL epoint2_multi_norm(_MIPD_ int m,big *work,epoint **p)
{ /* Normalise an array of points of length m<20 - requires a workspace array of length m */

#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
 
#ifndef MR_AFFINE_ONLY
    int i;
    big w[MR_MAX_M_T_S];
    if (mr_mip->coord==MR_AFFINE) return TRUE;
    if (mr_mip->ERNUM) return FALSE;   
    if (m>MR_MAX_M_T_S) return FALSE;

    MR_IN(192)

    for (i=0;i<m;i++)
    {
        if (p[i]->marker==MR_EPOINT_NORMALIZED) w[i]=mr_mip->one;
        else w[i]=p[i]->Z;

    }

    if (!multi_inverse2(_MIPP_ m,w,work)) 
    {
       MR_OUT
       return FALSE;
    }

    for (i=0;i<m;i++)
    {
        copy(mr_mip->one,p[i]->Z);
        p[i]->marker=MR_EPOINT_NORMALIZED;
#ifndef MR_NO_SS
        if (mr_mip->SS)
        {
            modmult2(_MIPP_ p[i]->X,work[i],p[i]->X);
            modmult2(_MIPP_ p[i]->Y,work[i],p[i]->Y); 
        }
        else
        {
#endif
            modmult2(_MIPP_ p[i]->X,work[i],p[i]->X);    /* X/Z */
            modmult2(_MIPP_ work[i],work[i],mr_mip->w1);
            modmult2(_MIPP_ p[i]->Y,mr_mip->w1,p[i]->Y);    /* Y/ZZ */
#ifndef MR_NO_SS
        }
#endif

    }    
    MR_OUT
#endif
    return TRUE;   
}

static void table_init(_MIPD_ epoint *g,epoint **table)
{ /* A precomputation option for the multiplication of a  */
  /* fixed point, would be to precalculate and normalize  */
  /* this table */
    int i,n,n3,nf,nb,t;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    
#ifndef MR_NOKOBLITZ

    if (mr_mip->KOBLITZ)
    {
       epoint2_copy(g,table[0]);
       epoint2_copy(g,table[MR_ECC_STORE_2M-1]);
       frobenius(_MIPP_ table[MR_ECC_STORE_2M-1]);
       frobenius(_MIPP_ table[MR_ECC_STORE_2M-1]);  
       nf=2;

       for (i=1;i<MR_ECC_STORE_2M;i++)
       {  /* Consider i expressed as NAF */

           n=i;
           n3=3*i+1;
           t=n3;
           nb=0;
           while (t>1)
           { /* number of bits in n3 */
               t>>=1;
               nb++;
           }
           while (nb>nf) 
           { /* move to next power of tau */
               frobenius(_MIPP_ table[MR_ECC_STORE_2M-1]); 
               nf++;
           }

           n3-=(1<<nb); /* subtract MSB */
           n=n3-n;      /* get index of previously calculated value */

           if (i==MR_ECC_STORE_2M-1)
           { /* last one.. */
                if (n>0) ecurve2_add(_MIPP_ table[n/2],table[i]);
                else     ecurve2_sub(_MIPP_ table[(-n)/2],table[i]);   
           }
           else
           { /* mostly mixed additions... */

                if (n>0) epoint2_copy(table[n/2],table[i]);
                if (n<0) 
                {
                    epoint2_copy(table[(-n)/2],table[i]);
                    epoint2_negate(_MIPP_ table[i]);
                }

                ecurve2_add(_MIPP_ table[MR_ECC_STORE_2M-1],table[i]);
           }    
       }
    }
    else
    {
#endif

        epoint2_copy(g,table[0]);
        epoint2_copy(g,table[MR_ECC_STORE_2M-1]);
        ecurve2_double(_MIPP_ table[MR_ECC_STORE_2M-1]);

       /* epoint2_norm(_MIPP_ table[MR_ECC_STORE_2M-1]);  makes additions below faster */
        for (i=1;i<MR_ECC_STORE_2M-1;i++)
        {  
            epoint2_copy(table[i-1],table[i]);
            ecurve2_add(_MIPP_ table[MR_ECC_STORE_2M-1],table[i]);
        }
        ecurve2_add(_MIPP_ table[MR_ECC_STORE_2M-2],table[MR_ECC_STORE_2M-1]);

#ifndef MR_NOKOBLITZ
    }
#endif
}

static void prepare_naf(_MIPD_ big e,big hp,big hn)
{ /* prepare NAF - exponent = hp-hn = (3e-e)/2 */
  /* return number of bits */
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
#ifndef MR_NOKOBLITZ
    if (mr_mip->KOBLITZ)
    {
        tnaf(_MIPP_ e,hp,hn);
    }
    else
    {
#endif
        copy(e,hn);
        premult(_MIPP_ hn,3,hp); 
        subdiv(_MIPP_ hn,2,hn);
        subdiv(_MIPP_ hp,2,hp);
#ifndef MR_NOKOBLITZ
    }
#endif
}

void ecurve2_mult(_MIPD_ big e,epoint *pa,epoint *pt)
{ /* pt=e*pa; */
    int i,j,n,nb,nbs,nzs;
#ifndef MR_AFFINE_ONLY
/*    int coord;  */
    big work[MR_ECC_STORE_2M];
#endif
    epoint *table[MR_ECC_STORE_2M];
#ifdef MR_STATIC
    char mem[MR_ECP_RESERVE(MR_ECC_STORE_2M)];
 /*   char mem[MR_ECP_RESERVE_A(MR_ECC_STORE_2M)];   Reserve space for AFFINE (x,y) only */
#ifndef MR_AFFINE_ONLY
    char mem1[MR_BIG_RESERVE(MR_ECC_STORE_2M)];
#endif
#else
    char *mem;
#ifndef MR_AFFINE_ONLY
    char *mem1;
#endif
#endif
#ifndef MR_ALWAYS_BINARY
    epoint *p;
    int ch,ce;
#endif
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;

    MR_IN(133)

    if (size(e)==0) 
    { /* multiplied by 0 */
        epoint2_set(_MIPP_ NULL,NULL,0,pt);
        MR_OUT
        return;
    }
    epoint2_norm(_MIPP_ pa); 
    epoint2_copy(pa,pt);

    copy(e,mr_mip->w9);

    if (size(mr_mip->w9)<0)
    { /* pt = -pt */
        negify(mr_mip->w9,mr_mip->w9);
        epoint2_negate(_MIPP_ pt);
    }

    if (size(mr_mip->w9)==1)
    { 
        MR_OUT
        return;
    }

    prepare_naf(_MIPP_ mr_mip->w9,mr_mip->w10,mr_mip->w9);

    if (size(mr_mip->w9)==0 && size(mr_mip->w10)==0)
    { /* multiplied by 0 */
        epoint2_set(_MIPP_ NULL,NULL,0,pt);
        MR_OUT
        return;
    }

#ifndef MR_STATIC
#ifndef MR_ALWAYS_BINARY
    if (mr_mip->base==mr_mip->base2)
    {
#endif
#endif

#ifdef MR_STATIC
        memset(mem,0,MR_ECP_RESERVE(MR_ECC_STORE_2M));
    /*    memset(mem,0,MR_ECP_RESERVE_A(MR_ECC_STORE_2M)); */
#ifndef MR_AFFINE_ONLY
        memset(mem1,0,MR_BIG_RESERVE(MR_ECC_STORE_2M));
#endif
#else
        mem=(char *)ecp_memalloc(_MIPP_ MR_ECC_STORE_2M);
#ifndef MR_AFFINE_ONLY
        mem1=(char *)memalloc(_MIPP_ MR_ECC_STORE_2M);
#endif
#endif

        for (i=0;i<=MR_ECC_STORE_2M-1;i++)
		{
            table[i]=epoint_init_mem(_MIPP_ mem,i);
#ifndef MR_AFFINE_ONLY
            work[i]=mirvar_mem(_MIPP_ mem1,i);
#endif
		}

		table_init(_MIPP_ pt,table);

#ifndef MR_AFFINE_ONLY
        epoint2_multi_norm(_MIPP_ MR_ECC_STORE_2M,work,table);
#endif

        nb=logb2(_MIPP_ mr_mip->w10);

        if ((n=logb2(_MIPP_ mr_mip->w9))>nb)
        {
            nb=n;
            epoint2_negate(_MIPP_ pt);
        }
        epoint2_set(_MIPP_ NULL,NULL,0,pt);

        for (i=nb-1;i>=0;)
        { /* add/subtract */
            if (mr_mip->user!=NULL) (*mr_mip->user)();
            n=mr_naf_window(_MIPP_ mr_mip->w9,mr_mip->w10,i,&nbs,&nzs,MR_ECC_STORE_2M);
/* printf("n= %d nbs= %d nzs= %d \n",n,nbs,nzs); */
            for (j=0;j<nbs;j++)
            {
#ifndef MR_NOKOBLITZ
                if (mr_mip->KOBLITZ) frobenius(_MIPP_ pt);
                else
#endif
                    ecurve2_double(_MIPP_ pt);
            }
            if (n>0) 
                ecurve2_add(_MIPP_ table[n/2],pt);
            if (n<0) 
                 ecurve2_sub(_MIPP_ table[(-n)/2],pt);
            i-=nbs;
            if (nzs)
            {
                for (j=0;j<nzs;j++) 
                {
#ifndef MR_NOKOBLITZ
                    if (mr_mip->KOBLITZ) frobenius(_MIPP_ pt);
                    else
#endif
                        ecurve2_double(_MIPP_ pt);
                }
                i-=nzs;
            }
        }
/*
#ifndef MR_AFFINE_ONLY
        coord=mr_mip->coord;      switch to AFFINE coordinates 
        mr_mip->coord=MR_AFFINE;
#endif
*/
#ifdef MR_STATIC
/*        memset(mem,0,MR_ECP_RESERVE_A(MR_ECC_STORE_2M)); */
        memset(mem,0,MR_ECP_RESERVE(MR_ECC_STORE_2M));
#else
        ecp_memkill(_MIPP_ mem,MR_ECC_STORE_2M);
#endif        

#ifndef MR_AFFINE_ONLY
        memkill(_MIPP_ mem1,MR_ECC_STORE_2M);
/*        mr_mip->coord=coord; */
#endif

#ifndef MR_STATIC      
#ifndef MR_ALWAYS_BINARY
    }
    else
    { 
        mem=(char *)ecp_memalloc(_MIPP_ 1);
        p=epoint_init_mem(_MIPP_ mem,0);
        epoint2_copy(pt,p);

        expb2(_MIPP_ logb2(_MIPP_ mr_mip->w10)-1,mr_mip->w11);
        mr_psub(_MIPP_ mr_mip->w10,mr_mip->w11,mr_mip->w10);
        subdiv(_MIPP_ mr_mip->w11,2,mr_mip->w11);
        while (size(mr_mip->w11) > 0)
        { /* add/subtract method */
            if (mr_mip->user!=NULL) (*mr_mip->user)();

            ecurve2_double(_MIPP_ pt);
            ce=mr_compare(mr_mip->w9,mr_mip->w11); /* e(i)=1? */
            ch=mr_compare(mr_mip->w10,mr_mip->w11); /* h(i)=1? */
            if (ch>=0) 
            {  /* h(i)=1 */
                if (ce<0) ecurve2_add(_MIPP_ p,pt);
                mr_psub(_MIPP_ mr_mip->w10,mr_mip->w11,mr_mip->w10);
            }
            if (ce>=0) 
            {  /* e(i)=1 */
                if (ch<0) ecurve2_sub(_MIPP_ p,pt);
                mr_psub(_MIPP_ mr_mip->w9,mr_mip->w11,mr_mip->w9);  
            }
            subdiv(_MIPP_ mr_mip->w11,2,mr_mip->w11);
        }
        ecp_memkill(_MIPP_ mem,1);
    }
#endif
#endif
    epoint2_norm(_MIPP_ pt);
    MR_OUT
}

#ifndef MR_NO_ECC_MULTIADD
#ifndef MR_STATIC

void ecurve2_multn(_MIPD_ int n,big *y,epoint **x,epoint *w)
{ /* pt=e[o]*p[0]+e[1]*p[1]+ .... e[n-1]*p[n-1]   */
    int i,j,k,m,nb,ea;
    epoint **G;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;

    MR_IN(134)

    m=1<<n;
    G=(epoint **)mr_alloc(_MIPP_ m,sizeof(epoint*));

    for (i=0,k=1;i<n;i++)
    {
        for (j=0; j < (1<<i) ;j++)
        {
            G[k]=epoint_init(_MIPPO_ );
            epoint2_copy(x[i],G[k]);
            if (j!=0) ecurve2_add(_MIPP_ G[j],G[k]);
            k++;
        }
    }

    nb=0;
    for (j=0;j<n;j++) if ((k=logb2(_MIPP_ y[j])) > nb) nb=k;

    epoint2_set(_MIPP_ NULL,NULL,0,w);            /* w=0 */
    
#ifndef MR_ALWAYS_BINARY
    if (mr_mip->base==mr_mip->base2)
    {
#endif
        for (i=nb-1;i>=0;i--)
        {
            if (mr_mip->user!=NULL) (*mr_mip->user)();
            ea=0;
            k=1;
            for (j=0;j<n;j++)
            {
                if (mr_testbit(_MIPP_ y[j],i)) ea+=k;
                k<<=1;
            }
            ecurve2_double(_MIPP_ w);
            if (ea!=0) ecurve2_add(_MIPP_ G[ea],w);
        }    
#ifndef MR_ALWAYS_BINARY
    }
    else mr_berror(_MIPP_ MR_ERR_NOT_SUPPORTED);
#endif

    for (i=1;i<m;i++) epoint_free(G[i]);
    mr_free(G);
    MR_OUT
}

#endif

void ecurve2_mult2(_MIPD_ big e,epoint *p,big ea,epoint *pa,epoint *pt)
{ /* pt=e*p+ea*pa; */
    int e1,h1,e2,h2,n,nb;
    epoint *p1,*p2,*ps[2];
#ifdef MR_STATIC
    char mem[MR_ECP_RESERVE(4)];
#else
    char *mem;
#endif
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
#ifndef MR_AFFINE_ONLY
    big work[2];
    work[0]=mr_mip->w14;
    work[1]=mr_mip->w15;
#endif
    if (mr_mip->ERNUM) return;

    MR_IN(135)

    if (size(e)==0) 
    {
        ecurve2_mult(_MIPP_ ea,pa,pt);
        MR_OUT
        return;
    }
#ifdef MR_STATIC
    memset(mem,0,MR_ECP_RESERVE(4));
#else
    mem=(char *)ecp_memalloc(_MIPP_ 4);
#endif
    p2=epoint_init_mem(_MIPP_ mem,0);
    p1=epoint_init_mem(_MIPP_ mem,1);
    ps[0]=epoint_init_mem(_MIPP_ mem,2);
    ps[1]=epoint_init_mem(_MIPP_ mem,3);

    epoint2_norm(_MIPP_ pa);
    epoint2_copy(pa,p2);
    copy(ea,mr_mip->w9);
    if (size(mr_mip->w9)<0)
    { /* p2 = -p2 */
        negify(mr_mip->w9,mr_mip->w9);
        epoint2_negate(_MIPP_ p2);
    } 

    epoint2_norm(_MIPP_ p);
    epoint2_copy(p,p1);
    copy(e,mr_mip->w12);
    if (size(mr_mip->w12)<0)
    { /* p1= -p1 */
        negify(mr_mip->w12,mr_mip->w12);
        epoint2_negate(_MIPP_ p1);
    }

#ifdef MR_NOKOBLITZ
    mr_jsf(_MIPP_ mr_mip->w9,mr_mip->w12,mr_mip->w10,mr_mip->w9,mr_mip->w13,mr_mip->w12);
#else
    if (mr_mip->KOBLITZ)
    {
        prepare_naf(_MIPP_ mr_mip->w9,mr_mip->w10,mr_mip->w9);
        prepare_naf(_MIPP_ mr_mip->w12,mr_mip->w13,mr_mip->w12);
    }
    else
        mr_jsf(_MIPP_ mr_mip->w9,mr_mip->w12,mr_mip->w10,mr_mip->w9,mr_mip->w13,mr_mip->w12);
#endif

    nb=logb2(_MIPP_ mr_mip->w10);
    if ((n=logb2(_MIPP_ mr_mip->w13))>nb) nb=n;
    if ((n=logb2(_MIPP_ mr_mip->w9))>nb) nb=n;
    if ((n=logb2(_MIPP_ mr_mip->w12))>nb) nb=n;

    epoint2_set(_MIPP_ NULL,NULL,0,pt);            /* pt=0 */

    expb2(_MIPP_ nb-1,mr_mip->w11);

    epoint2_copy(p1,ps[0]);
    ecurve2_add(_MIPP_ p2,ps[0]);                    /* ps=p1+p2 */
    epoint2_copy(p1,ps[1]);
    ecurve2_sub(_MIPP_ p2,ps[1]);                    /* pd=p1-p2 */

#ifndef MR_AFFINE_ONLY
    epoint2_multi_norm(_MIPP_ 2,work,ps);
#endif
    while (size(mr_mip->w11) > 0)
    { /* add/subtract method */
        if (mr_mip->user!=NULL) (*mr_mip->user)();
#ifndef MR_NOKOBLITZ
        if (mr_mip->KOBLITZ) frobenius(_MIPP_ pt);
        else
#endif            
            ecurve2_double(_MIPP_ pt);

        e1=h1=e2=h2=0;
        if (mr_compare(mr_mip->w9,mr_mip->w11)>=0)
        { /* e1(i)=1? */
            e2=1;  
            mr_psub(_MIPP_ mr_mip->w9,mr_mip->w11,mr_mip->w9);
        }
        if (mr_compare(mr_mip->w10,mr_mip->w11)>=0)
        { /* h1(i)=1? */
            h2=1;  
            mr_psub(_MIPP_ mr_mip->w10,mr_mip->w11,mr_mip->w10);
        } 
        if (mr_compare(mr_mip->w12,mr_mip->w11)>=0)
        { /* e2(i)=1? */
            e1=1;   
            mr_psub(_MIPP_ mr_mip->w12,mr_mip->w11,mr_mip->w12);
        }
        if (mr_compare(mr_mip->w13,mr_mip->w11)>=0) 
        { /* h2(i)=1? */
            h1=1;  
            mr_psub(_MIPP_ mr_mip->w13,mr_mip->w11,mr_mip->w13);
        }

        if (e1!=h1)
        {
            if (e2==h2)
            {
                if (h1==1) ecurve2_add(_MIPP_ p1,pt);
                else       ecurve2_sub(_MIPP_ p1,pt);
            }
            else
            {
                if (h1==1)
                {
                    if (h2==1) ecurve2_add(_MIPP_ ps[0],pt);
                    else       ecurve2_add(_MIPP_ ps[1],pt);
                }
                else
                {
                    if (h2==1) ecurve2_sub(_MIPP_ ps[1],pt);
                    else       ecurve2_sub(_MIPP_ ps[0],pt);
                }
            }
        }
        else if (e2!=h2)
        {
            if (h2==1) ecurve2_add(_MIPP_ p2,pt);
            else       ecurve2_sub(_MIPP_ p2,pt);
        }

        subdiv(_MIPP_ mr_mip->w11,2,mr_mip->w11);
    }
    ecp_memkill(_MIPP_ mem,4);

    MR_OUT
}

#endif

/*   Routines to implement comb method for fast
 *   computation of x*G mod n, for fixed G and n, using precomputation. 
 *
 *   Elliptic curve over GF(2^m) version of mrebrick.c
 *
 *   This idea can be used to substantially speed up certain phases 
 *   of the Elliptic Curve Digital Signature Standard (ECS) for example.
 *
 *   See "Handbook of Applied Cryptography"
 */

#ifndef MR_STATIC

BOOL ebrick2_init(_MIPD_ ebrick2 *B,big x,big y,big a2,big a6,int m,int a,int b,int c,int window,int nb)
{ /* (x,y) is the fixed base                            *
   * a2 and a6 the parameters of the curve              *
   * m, a, b, c are the m in the 2^m modulus, and a,b,c *
   * are the parameters of the irreducible bases,       *
   * trinomial if b!=0, otherwise pentanomial           *
   * window is the window size in bits and              *
   * nb is the maximum number of bits in the multiplier */

    int i,j,k,t,bp,len,bptr,is;
    epoint **table;
    epoint *w;

#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (nb<2 || window<1 || window>nb || mr_mip->ERNUM) return FALSE;

    t=MR_ROUNDUP(nb,window);
    if (t<2) return FALSE;

    MR_IN(136)

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
    table=(epoint **)mr_alloc(_MIPP_ (1<<window),sizeof(epoint *));
    
    if (table==NULL)
    {
        mr_berror(_MIPP_ MR_ERR_OUT_OF_MEMORY);   
        MR_OUT
        return FALSE;
    }
    B->a6=mirvar(_MIPP_ 0);
    copy(a6,B->a6);
    B->a2=mirvar(_MIPP_ 0);
    copy(a2,B->a2);
    B->m=m;
    B->a=a;
    B->b=b;
    B->c=c;   

    if (!ecurve2_init(_MIPP_ m,a,b,c,a2,a6,TRUE,MR_AFFINE))
    {
        MR_OUT
        return FALSE;
    }
 
    if (m<0) m=-m;  /* if it is supersingular */

    w=epoint_init(_MIPPO_ );
    epoint2_set(_MIPP_ x,y,0,w);

    table[0]=epoint_init(_MIPPO_ );
    table[1]=epoint_init(_MIPPO_ );
    epoint2_copy(w,table[1]);
    for (j=0;j<t;j++)
        ecurve2_double(_MIPP_ w);

    k=1;
    for (i=2;i<(1<<window);i++)
    {
        table[i]=epoint_init(_MIPPO_ );
        if (i==(1<<k))
        {
            k++;
            epoint2_copy(w,table[i]);
            
            for (j=0;j<t;j++)
                ecurve2_double(_MIPP_ w);
            continue;
        }
        bp=1;
        for (j=0;j<k;j++)
        {
            if (i&bp)
			{
				is=(1<<j);
                ecurve2_add(_MIPP_ table[is],table[i]);
			}
            bp<<=1;
        }
    }

    epoint_free(w);

/* create the table */

    len=MR_ROUNDUP(m,MIRACL);
    bptr=0;
    B->table=(mr_small *)mr_alloc(_MIPP_ 2*len*(1<<window),sizeof(mr_small));

    for (i=0;i<(1<<window);i++)
    {
        for (j=0;j<len;j++)
            B->table[bptr++]=table[i]->X->w[j];
        for (j=0;j<len;j++)
            B->table[bptr++]=table[i]->Y->w[j];

        epoint_free(table[i]);
    }
        
    mr_free(table);  

    MR_OUT
    return TRUE;
}

void ebrick2_end(ebrick2 *B)
{
    mirkill(B->a2);
    mirkill(B->a6);
    mr_free(B->table);  
}

#else

/* use precomputated table in ROM - use romaker2.c to create the table, and ecdh2m*.c 
   for an example of use */

void ebrick2_init(ebrick2 *B,const mr_small* rom,big a2,big a6,int m,int a,int b,int c,int window,int nb)
{
    B->table=rom;
    B->a2=a2; /* just pass a pointer */
    B->a6=a6;
    B->m=m;
    B->a=a;
    B->b=b;
    B->c=c;
    B->window=window;  /* 2^4=16  stored values */
    B->max=nb;
}

#endif

int mul2_brick(_MIPD_ ebrick2 *B,big e,big x,big y)
{
    int i,j,t,d,m,len,maxsize,promptr;
    epoint *w,*z;
 
#ifdef MR_STATIC
    char mem[MR_ECP_RESERVE(2)];
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
        return 0;
    }
#endif

    if (logb2(_MIPP_ e) > B->max)
    {
        mr_berror(_MIPP_ MR_ERR_EXP_TOO_BIG);
        MR_OUT
        return 0;
    }
    if (!ecurve2_init(_MIPP_ B->m,B->a,B->b,B->c,B->a2,B->a6,FALSE,MR_BEST))
    {
        MR_OUT
        return 0;
    }

#ifdef MR_STATIC
    memset(mem,0,MR_ECP_RESERVE(2));
#else
    mem=(char *)ecp_memalloc(_MIPP_ 2);
#endif
    w=epoint_init_mem(_MIPP_ mem,0);
    z=epoint_init_mem(_MIPP_ mem,1);

    m=B->m;
    if (m<0) m=-m;

    len=MR_ROUNDUP(m,MIRACL);
    maxsize=2*(1<<B->window)*len;

    j=recode(_MIPP_ e,t,B->window,t-1);
    if (j>0)
    {
        promptr=2*j*len;
        init_point_from_rom(w,len,B->table,maxsize,&promptr);
    }

    for (i=t-2;i>=0;i--)
    {
        j=recode(_MIPP_ e,t,B->window,i);
        ecurve2_double(_MIPP_ w);
        if (j>0)
        {
            promptr=2*j*len;
            init_point_from_rom(z,len,B->table,maxsize,&promptr);
            ecurve2_add(_MIPP_ z,w);
        }
    }

    d=epoint2_get(_MIPP_ w,x,y);
#ifndef MR_STATIC
    ecp_memkill(_MIPP_ mem,2);
#else
    memset(mem,0,MR_ECP_RESERVE(2));
#endif
    MR_OUT
    return d;
}

#endif


