
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
 *   MIRACL methods for modular exponentiation
 *   mrpower.c 
 */

#include <stdlib.h>
#include "miracl.h"

void nres_powltr(_MIPD_ int x,big y,big w)
{ /* calculates w=x^y mod z using Left to Right Method   */
  /* uses only n^2 modular squarings, because x is small */
  /* Note: x is NOT an nresidue */
    int i,nb;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

    if (mr_mip->ERNUM) return;
    copy(y,mr_mip->w1);

    MR_IN(86)

    zero(w);
    if (x==0) 
    {
        if (size(mr_mip->w1)==0) 
        { /* 0^0 = 1 */
            copy(mr_mip->one,w);
        }
        MR_OUT
        return;
    }

    copy(mr_mip->one,w);
    if (size(mr_mip->w1)==0) 
    {
        MR_OUT
        return;
    }
    if (size(mr_mip->w1)<0) mr_berror(_MIPP_ MR_ERR_NEG_POWER);
    if (mr_mip->ERNUM)
    {
        MR_OUT
        return;
    }

#ifndef MR_ALWAYS_BINARY 
    if (mr_mip->base==mr_mip->base2)
    { 
#endif
        nb=logb2(_MIPP_ mr_mip->w1);
        convert(_MIPP_ x,w);
        nres(_MIPP_ w,w);
        if (nb>1) for (i=nb-2;i>=0;i--)
        { /* Left to Right binary method */

            if (mr_mip->user!=NULL) (*mr_mip->user)();

            nres_modmult(_MIPP_ w,w,w);
            if (mr_testbit(_MIPP_ mr_mip->w1,i))
            { /* this is quick */
                premult(_MIPP_ w,x,w);
                divide(_MIPP_ w,mr_mip->modulus,mr_mip->modulus);
            }
        }
#ifndef MR_ALWAYS_BINARY 
    }    
    else
    {
        expb2(_MIPP_ logb2(_MIPP_ mr_mip->w1)-1,mr_mip->w2);
        while (size(mr_mip->w2)!=0)
        { /* Left to Right binary method */

            if (mr_mip->user!=NULL) (*mr_mip->user)();
            if (mr_mip->ERNUM) break;
            nres_modmult(_MIPP_ w,w,w);
            if (mr_compare(mr_mip->w1,mr_mip->w2)>=0)
            {
                premult(_MIPP_ w,x,w);
                divide(_MIPP_ w,mr_mip->modulus,mr_mip->modulus);
                subtract(_MIPP_ mr_mip->w1,mr_mip->w2,mr_mip->w1);
            }
            subdiv(_MIPP_ mr_mip->w2,2,mr_mip->w2);
        }
    }
#endif
    if (size(w)<0) add(_MIPP_ w,mr_mip->modulus,w);
    MR_OUT
    return;
}

#ifndef MR_STATIC

void nres_powmodn(_MIPD_ int n,big *x,big *y,big w)
{
    int i,j,k,m,nb,ea;
    big *G;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;

    MR_IN(112)

    m=1<<n;
    G=(big *)mr_alloc(_MIPP_ m,sizeof(big));

/* 2^n - n - 1 modmults */
/* 4(n=3) 11(n=4) etc   */

    for (i=0,k=1;i<n;i++)
    {
        for (j=0; j < (1<<i) ;j++)
        {
            G[k]=mirvar(_MIPP_ 0);
            if (j==0) copy(x[i],G[k]);
            else      nres_modmult(_MIPP_ G[j],x[i],G[k]);
            k++;
        }
    }

    nb=0;
    for (j=0;j<n;j++) 
        if ((k=logb2(_MIPP_ y[j]))>nb) nb=k;

    copy(mr_mip->one,w);

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
            nres_modmult(_MIPP_ w,w,w);
            if (ea!=0) nres_modmult(_MIPP_ w,G[ea],w);
        }

#ifndef MR_ALWAYS_BINARY 
    }
    else mr_berror(_MIPP_ MR_ERR_NOT_SUPPORTED);
#endif

    for (i=1;i<m;i++) mirkill(G[i]);
    mr_free(G);

    MR_OUT
}

void powmodn(_MIPD_ int n,big *x,big *y,big p,big w)
{/* w=x[0]^y[0].x[1]^y[1] .... x[n-1]^y[n-1] mod n */
    int j;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;

    MR_IN(113)

    prepare_monty(_MIPP_ p);
    for (j=0;j<n;j++) nres(_MIPP_ x[j],x[j]);
    nres_powmodn(_MIPP_ n,x,y,w);   
    for (j=0;j<n;j++) redc(_MIPP_ x[j],x[j]);

    redc(_MIPP_ w,w);

    MR_OUT
}

#endif

void nres_powmod2(_MIPD_ big x,big y,big a,big b,big w)
{ /* finds w = x^y.a^b mod n. Fast for some cryptosystems */ 
    int i,j,nb,nb2,nbw,nzs,n;
    big table[16];
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;
    copy(y,mr_mip->w1);
    copy(x,mr_mip->w2);
    copy(b,mr_mip->w3);
    copy(a,mr_mip->w4);
    zero(w);
    if (size(mr_mip->w2)==0 || size(mr_mip->w4)==0) return;

    MR_IN(99)

    copy(mr_mip->one,w);
    if (size(mr_mip->w1)==0 && size(mr_mip->w3)==0) 
    {
        MR_OUT
        return;
    }
    if (size(mr_mip->w1)<0 || size(mr_mip->w3)<0) mr_berror(_MIPP_ MR_ERR_NEG_POWER);
    if (mr_mip->ERNUM)
    {
        MR_OUT
        return;
    }
     
#ifndef MR_ALWAYS_BINARY 

    if (mr_mip->base==mr_mip->base2)
    { /* uses 2-bit sliding window. This is 25% faster! */
#endif
        nres_modmult(_MIPP_ mr_mip->w2,mr_mip->w4,mr_mip->w5);
        nres_modmult(_MIPP_ mr_mip->w2,mr_mip->w2,mr_mip->w12);
        nres_modmult(_MIPP_ mr_mip->w4,mr_mip->w4,mr_mip->w13);
        nres_modmult(_MIPP_ mr_mip->w4,mr_mip->w13,mr_mip->w14);
        nres_modmult(_MIPP_ mr_mip->w2,mr_mip->w13,mr_mip->w6);
        nres_modmult(_MIPP_ mr_mip->w6,mr_mip->w4,mr_mip->w15);
        nres_modmult(_MIPP_ mr_mip->w4,mr_mip->w12,mr_mip->w8);
        nres_modmult(_MIPP_ mr_mip->w2,mr_mip->w12,mr_mip->w9);
        nres_modmult(_MIPP_ mr_mip->w4,mr_mip->w9,mr_mip->w10);
        nres_modmult(_MIPP_ mr_mip->w14,mr_mip->w12,mr_mip->w11);
        nres_modmult(_MIPP_ mr_mip->w9,mr_mip->w13,mr_mip->w12);
        nres_modmult(_MIPP_ mr_mip->w10,mr_mip->w13,mr_mip->w13);

        table[0]=NULL; table[1]=mr_mip->w4;  table[2]=mr_mip->w2;   table[3]=mr_mip->w5;
        table[4]=NULL; table[5]=mr_mip->w14; table[6]=mr_mip->w6;   table[7]=mr_mip->w15;
        table[8]=NULL; table[9]=mr_mip->w8;  table[10]=mr_mip->w9;  table[11]=mr_mip->w10;
        table[12]=NULL;table[13]=mr_mip->w11;table[14]=mr_mip->w12; table[15]=mr_mip->w13;
        nb=logb2(_MIPP_ mr_mip->w1);
        if ((nb2=logb2(_MIPP_ mr_mip->w3))>nb) nb=nb2;

        for (i=nb-1;i>=0;)
        { 
            if (mr_mip->user!=NULL) (*mr_mip->user)();

            n=mr_window2(_MIPP_ mr_mip->w1,mr_mip->w3,i,&nbw,&nzs);
            for (j=0;j<nbw;j++)
                nres_modmult(_MIPP_ w,w,w);
            if (n>0) nres_modmult(_MIPP_ w,table[n],w);
            i-=nbw;
            if (nzs) 
            {
                nres_modmult(_MIPP_ w,w,w);
                i--;
            } 
        }
#ifndef MR_ALWAYS_BINARY 
    }
    else
    {
        nres_modmult(_MIPP_ mr_mip->w2,mr_mip->w4,mr_mip->w5);

        if (mr_compare(mr_mip->w1,mr_mip->w3)>=0)
             expb2(_MIPP_ logb2(_MIPP_ mr_mip->w1)-1,mr_mip->w6);
        else expb2(_MIPP_ logb2(_MIPP_ mr_mip->w3)-1,mr_mip->w6);
        while (size(mr_mip->w6)!=0)
        {
            if (mr_mip->user!=NULL) (*mr_mip->user)();
            if (mr_mip->ERNUM) break;
            nres_modmult(_MIPP_ w,w,w);
            if (mr_compare(mr_mip->w1,mr_mip->w6)>=0)
            {
                if (mr_compare(mr_mip->w3,mr_mip->w6)>=0)
                {
                     nres_modmult(_MIPP_ w,mr_mip->w5,w);
                     subtract(_MIPP_ mr_mip->w3,mr_mip->w6,mr_mip->w3);
                }

                else nres_modmult(_MIPP_ w,mr_mip->w2,w);
                subtract(_MIPP_ mr_mip->w1,mr_mip->w6,mr_mip->w1);
            }
            else
            {
                if (mr_compare(mr_mip->w3,mr_mip->w6)>=0)
                {
                     nres_modmult(_MIPP_ w,mr_mip->w4,w);
                     subtract(_MIPP_ mr_mip->w3,mr_mip->w6,mr_mip->w3);
                }
            }
            subdiv(_MIPP_ mr_mip->w6,2,mr_mip->w6);
        }
    }
#endif
    MR_OUT
}

void powmod2(_MIPD_ big x,big y,big a,big b,big n,big w)
{/* w=x^y.a^b mod n */
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;

    MR_IN(79)

    prepare_monty(_MIPP_ n);
    nres(_MIPP_ x,mr_mip->w2);
    nres(_MIPP_ a,mr_mip->w4);
    nres_powmod2(_MIPP_ mr_mip->w2,y,mr_mip->w4,b,w);
    redc(_MIPP_ w,w);

    MR_OUT   
}


void powmod(_MIPD_ big x,big y,big n,big w)
{ /* fast powmod, using Montgomery's method internally */

    mr_small norm;
    BOOL mty;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;

    MR_IN(18)

    mty=TRUE;

    if (mr_mip->base!=mr_mip->base2)
    {
        if (size(n)<2 || sgcd(n->w[0],mr_mip->base)!=1) mty=FALSE;
    }
    else
        if (subdivisible(_MIPP_ n,2)) mty=FALSE;

    if (!mty)
    { /* can't use Montgomery */
        copy(y,mr_mip->w1);
        copy(x,mr_mip->w3);
        zero(w);
        if (size(mr_mip->w3)==0) 
        {
            MR_OUT
            return;
        }
        convert(_MIPP_ 1,w);
        if (size(mr_mip->w1)==0) 
        {
            MR_OUT
            return;
        }
        if (size(mr_mip->w1)<0) mr_berror(_MIPP_ MR_ERR_NEG_POWER);
        if (w==n)           mr_berror(_MIPP_ MR_ERR_BAD_PARAMETERS) ;
        if (mr_mip->ERNUM)
        {
            MR_OUT
            return;
        }

        norm=normalise(_MIPP_ n,n);
        divide(_MIPP_ mr_mip->w3,n,n);
        forever
        { 
            if (mr_mip->user!=NULL) (*mr_mip->user)();

            if (subdiv(_MIPP_ mr_mip->w1,2,mr_mip->w1)!=0)
                mad(_MIPP_ w,mr_mip->w3,mr_mip->w3,n,n,w);
            if (mr_mip->ERNUM || size(mr_mip->w1)==0) break;
            mad(_MIPP_ mr_mip->w3,mr_mip->w3,mr_mip->w3,n,n,mr_mip->w3);
        }
        if (norm!=1)
        {
#ifdef MR_FP_ROUNDING
            mr_sdiv(_MIPP_ n,norm,mr_invert(norm),n);
#else
            mr_sdiv(_MIPP_ n,norm,n);
#endif
            divide(_MIPP_ w,n,n);
        }
    }
    else
    { /* optimized code for odd moduli */
        prepare_monty(_MIPP_ n); 
        nres(_MIPP_ x,mr_mip->w3);
        nres_powmod(_MIPP_ mr_mip->w3,y,w);
        redc(_MIPP_ w,w);
    }
    
    MR_OUT
}

int powltr(_MIPD_ int x, big y, big n, big w)
{
    mr_small norm;
    BOOL clean_up,mty;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return 0;

    MR_IN(71)
    mty=TRUE;

    if (mr_mip->base!=mr_mip->base2)
    {
        if (size(n)<2 || sgcd(n->w[0],mr_mip->base)!=1) mty=FALSE;
    }
    else
    {
        if (subdivisible(_MIPP_ n,2)) mty=FALSE;
    }

/* Is Montgomery modulus in use... */

    clean_up=FALSE;
    if (mty)
    { /* still a chance to use monty... */
       if (size(mr_mip->modulus)!=0)
       { /* somebody else is using it */
           if (mr_compare(n,mr_mip->modulus)!=0) mty=FALSE;
       }
       else
       { /* its unused, so use it, but clean up after */
           clean_up=TRUE;
       }
    }
    if (!mty)
    { /* can't use Montgomery! */
        copy(y,mr_mip->w1);
        zero(w);
        if (x==0) 
        {
            MR_OUT
            return 0;
        }
        convert(_MIPP_ 1,w);
        if (size(mr_mip->w1)==0)
        {
            MR_OUT
            return 1;
        }
        
        if (size(mr_mip->w1)<0) mr_berror(_MIPP_ MR_ERR_NEG_POWER);
        if (w==n)               mr_berror(_MIPP_ MR_ERR_BAD_PARAMETERS) ;
        if (mr_mip->ERNUM)
        {
            MR_OUT
            return 0;
        }
        
        norm=normalise(_MIPP_ n,n);

        expb2(_MIPP_ logb2(_MIPP_ mr_mip->w1)-1,mr_mip->w2);
        while (size(mr_mip->w2)!=0)
        { /* Left to Right binary method */

            if (mr_mip->user!=NULL) (*mr_mip->user)();
            if (mr_mip->ERNUM) break;
            mad(_MIPP_ w,w,w,n,n,w);
            if (mr_compare(mr_mip->w1,mr_mip->w2)>=0)
            {
                premult(_MIPP_ w,x,w);
                divide(_MIPP_ w,n,n);
                subtract(_MIPP_ mr_mip->w1,mr_mip->w2,mr_mip->w1);
            }
            subdiv(_MIPP_ mr_mip->w2,2,mr_mip->w2);
        }
        if (norm!=1) 
        {
#ifdef MR_FP_ROUNDING
            mr_sdiv(_MIPP_ n,norm,mr_invert(norm),n);
#else
            mr_sdiv(_MIPP_ n,norm,n);
#endif
            divide(_MIPP_ w,n,n);
        }
    }
    else
    { /* optimized code for odd moduli */
        prepare_monty(_MIPP_ n);
        nres_powltr(_MIPP_ x,y,w);
        redc(_MIPP_ w,w);
        if (clean_up) kill_monty(_MIPPO_ );
    }
    MR_OUT 
    return (size(w));
}

void nres_powmod(_MIPD_ big x,big y,big w)
{  /*  calculates w=x^y mod z, using m-residues       *
    *  See "Analysis of Sliding Window Techniques for *
    *  Exponentiation, C.K. Koc, Computers. Math. &   *
    *  Applic. Vol. 30 pp17-24 1995. Uses work-space  *
    *  variables for pre-computed table. */
    int i,j,k,t,nb,nbw,nzs,n;
    big table[16];
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;

    copy(y,mr_mip->w1);
    copy(x,mr_mip->w3);

    MR_IN(84)
    zero(w);
    if (size(x)==0)
    {
       if (size(mr_mip->w1)==0)
       { /* 0^0 = 1 */
           copy(mr_mip->one,w);
       } 
       MR_OUT
       return;
    }

    copy(mr_mip->one,w);
    if (size(mr_mip->w1)==0) 
    {
        MR_OUT
        return;
    }

    if (size(mr_mip->w1)<0) mr_berror(_MIPP_ MR_ERR_NEG_POWER);

    if (mr_mip->ERNUM)
    {
        MR_OUT
        return;
    }

#ifndef MR_ALWAYS_BINARY 
    if (mr_mip->base==mr_mip->base2)
    { /* build a table. Up to 5-bit sliding windows. Windows with
       * two adjacent 0 bits simply won't happen */
#endif
        table[0]=mr_mip->w3; table[1]=mr_mip->w4; table[2]=mr_mip->w5; table[3]=mr_mip->w14;
        table[4]=NULL;  table[5]=mr_mip->w6; table[6]=mr_mip->w15; table[7]=mr_mip->w8;
        table[8]=NULL;  table[9]=NULL;  table[10]=mr_mip->w9; table[11]=mr_mip->w10;
        table[12]=NULL; table[13]=mr_mip->w11; table[14]=mr_mip->w12; table[15]=mr_mip->w13;

        nres_modmult(_MIPP_ mr_mip->w3,mr_mip->w3,mr_mip->w2);  /* x^2 */
        n=15;
        j=0;
        do
        { /* pre-computations */
            t=1; k=j+1;
            while (table[k]==NULL) {k++; t++;}
            copy(table[j],table[k]);
            for (i=0;i<t;i++) nres_modmult(_MIPP_ table[k],mr_mip->w2,table[k]);
            j=k;
        } while (j<n);

        nb=logb2(_MIPP_ mr_mip->w1);
        copy(mr_mip->w3,w);
		if (nb>1) for (i=nb-2;i>=0;)
        { /* Left to Right method */

            if (mr_mip->user!=NULL) (*mr_mip->user)();

            n=mr_window(_MIPP_ mr_mip->w1,i,&nbw,&nzs,5); 

            for (j=0;j<nbw;j++)
                    nres_modmult(_MIPP_ w,w,w);
            if (n>0) 
                nres_modmult(_MIPP_ w,table[n/2],w);
            
            i-=nbw;
            if (nzs)
            {
                for (j=0;j<nzs;j++)
                    nres_modmult(_MIPP_ w,w,w);
                i-=nzs;
            }
        }

#ifndef MR_ALWAYS_BINARY 
    }
    else
    {
        copy(mr_mip->w3,mr_mip->w2);
        forever
        { /* "Russian peasant" Right-to-Left exponentiation */

            if (mr_mip->user!=NULL) (*mr_mip->user)();

            if (subdiv(_MIPP_ mr_mip->w1,2,mr_mip->w1)!=0)
                nres_modmult(_MIPP_ w,mr_mip->w2,w);
            if (mr_mip->ERNUM || size(mr_mip->w1)==0) break;
            nres_modmult(_MIPP_ mr_mip->w2,mr_mip->w2,mr_mip->w2);
        }
    }
#endif
    MR_OUT
}

