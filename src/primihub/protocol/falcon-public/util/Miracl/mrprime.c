
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
 *   MIRACL prime number routines - test for and generate prime numbers
 *   mrprime.c
 */

#include <stdlib.h>
#include "miracl.h"

#ifndef MR_STATIC

#if MIRACL==8
#define MR_MAXPRIME 100
#else
#define MR_MAXPRIME 1000
#endif


void gprime(_MIPD_ int maxp)
{ /* generate all primes less than maxp into global PRIMES */
    char *sv;
    int pix,i,k,prime;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return;
    if (maxp<=0)
    {
        if (mr_mip->PRIMES!=NULL) mr_free(mr_mip->PRIMES);
        mr_mip->PRIMES=NULL;
        return;
    }

    MR_IN(70)

    if (maxp>=MR_TOOBIG)
    {
         mr_berror(_MIPP_ MR_ERR_TOO_BIG);
         MR_OUT
         return;
    }
    if (maxp<MR_MAXPRIME) maxp=MR_MAXPRIME;
    maxp=(maxp+1)/2;
    sv=(char *)mr_alloc(_MIPP_ maxp,1);
    if (sv==NULL)
    {
        mr_berror(_MIPP_ MR_ERR_OUT_OF_MEMORY);
        MR_OUT
        return;
    }
    pix=1;
    for (i=0;i<maxp;i++)
        sv[i]=TRUE;
    for (i=0;i<maxp;i++)
    if (sv[i])
    { /* using sieve of Eratosthenes */
        prime=i+i+3;
        for (k=i+prime;k<maxp;k+=prime)
            sv[k]=FALSE;
        pix++;
    }
    if (mr_mip->PRIMES!=NULL) mr_free(mr_mip->PRIMES);
    mr_mip->PRIMES=(int *)mr_alloc(_MIPP_ pix+2,sizeof(int));
    if (mr_mip->PRIMES==NULL)
    {
        mr_free(sv);
        mr_berror(_MIPP_ MR_ERR_OUT_OF_MEMORY);
        MR_OUT
        return;
    }
    mr_mip->PRIMES[0]=2;
    pix=1;
    for (i=0;i<maxp;i++)
        if (sv[i]) mr_mip->PRIMES[pix++]=i+i+3;
    mr_mip->PRIMES[pix]=0;
    mr_free(sv);
    MR_OUT
    return;
}

#else

void gprime(_MIPD_ int maxp)
{ /* dummy - does nothing */
}


#endif

int trial_division(_MIPD_ big x,big y)
{ 
  /* general purpose trial-division function, works in two modes */
  /* if (x==y) quick test for candidate prime, using trial       *
   * division by the small primes in the instance array PRIMES[] */
  /* returns 0 if definitely not a prime *
   * returns 1 if definitely is  a prime *
   * returns 2 if possibly a prime       */
  /* if x!=y it continues to extract small factors, and returns  *
   * the largest factor of x in y,                               *
   * i.e. completely factors over the small primes in PRIMES     *
   * In this case the returned value refers to the status of y   */
  /* returns 1 if y is definitely a prime  (and x was smooth)    *
   * returns 2 if y is possibly a prime                          */
    int i;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return 0;
    if (size(x)<=1) return 0;

    MR_IN(105)

    copy(x,y);

#ifndef MR_STATIC
    if (mr_mip->PRIMES==NULL) gprime(_MIPP_ MR_MAXPRIME);
#endif
    for (i=0;mr_mip->PRIMES[i]!=0;i++)
    { /* test for divisible by first few primes */
        while (subdiv(_MIPP_ y,mr_mip->PRIMES[i],mr_mip->w1)==0)
        { 
            if (x==y)
            {
                MR_OUT
                if (size(mr_mip->w1)==1) return 1;
                else return 0;
            }
            else 
            {
                if (size(mr_mip->w1)==1)
                { /* y is largest prime factor */
                    MR_OUT
                    return 1;
                }
                copy(mr_mip->w1,y);
                continue;
            }
        }
        if (size(mr_mip->w1)<=mr_mip->PRIMES[i])
        { 
            MR_OUT
            return 1;
        }
    }
    MR_OUT
    return 2;
}

BOOL isprime(_MIPD_ big x)
{  /*  test for primality (probably); TRUE if x is prime. test done NTRY *
    *  times; chance of wrong identification << (1/4)^NTRY. Note however *
    *  that this is an extreme upper bound. For example for a 100 digit  *
    *  "prime" the chances of false witness are actually < (.00000003)^NTRY *
    *  See Kim & Pomerance "The probability that a random probable prime *
    *  is Composite", Math. Comp. October 1989 pp.721-741                *
    *  The value of NTRY is now adjusted internally to account for this. */
    int j,k,n,r,times,d;

#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return TRUE;
    if (size(x)<=1) return FALSE;

    MR_IN(22)

    k=trial_division(_MIPP_ x,x);
    if (k==0) 
    {
        MR_OUT
        return FALSE;
    }
    if (k==1)
    {
        MR_OUT
        return TRUE;
    }

/* Miller-Rabin */

    decr(_MIPP_ x,1,mr_mip->w1); /* calculate k and mr_w8 ...    */
    k=0;
    while (subdiv(_MIPP_ mr_mip->w1,2,mr_mip->w1)==0)
    {
        k++;
        copy(mr_mip->w1,mr_mip->w8);
    }              /* ... such that x=1+mr_w8*2**k */
    times=mr_mip->NTRY;
    if (times>100) times=100;
    d=logb2(_MIPP_ x);    /* for larger primes, reduce NTRYs */
    if (d>220) times=2+((10*times)/(d-210));

    for (n=1;n<=times;n++)
    { /* perform test times times */
        j=0;
#ifndef MR_NO_RAND
        do
        {
            r=(int)brand(_MIPPO_ )%MR_TOOBIG;
            if (size(x)<MR_TOOBIG) r%=size(x);
        } while (r<2);
#else
        r=mr_mip->PRIMES[n];  /* possibly unsafe.. */
        if (r==0) break;
#endif
        powltr(_MIPP_ r,mr_mip->w8,x,mr_mip->w9);
/* use small random numbers...  */
        decr(_MIPP_ x,1,mr_mip->w2);

        while ((j>0 || size(mr_mip->w9)!=1) 
              && mr_compare(mr_mip->w9,mr_mip->w2)!=0)
        {
            j++;
            if ((j>1 && size(mr_mip->w9)==1) || j==k)
            { /* definitely not prime */
                MR_OUT
                return FALSE;
            }
            mad(_MIPP_ mr_mip->w9,mr_mip->w9,mr_mip->w9,x,x,mr_mip->w9);
        }

        if (mr_mip->user!=NULL) if (!(*mr_mip->user)())
        {
            MR_OUT
            return FALSE;
        }
    }

    MR_OUT
    return TRUE;  /* probably prime */
}

BOOL nxprime(_MIPD_ big w,big x)
{  /*  find next highest prime from w using     * 
    *  probabilistic primality test             */
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return FALSE;

    MR_IN(21)

    copy(w,x);
    if (size(x)<2) 
    {
        convert(_MIPP_ 2,x);
        MR_OUT
        return TRUE;
    }
    if (subdiv(_MIPP_ x,2,mr_mip->w1)==0) incr(_MIPP_ x,1,x);
    else                           incr(_MIPP_ x,2,x);
    while (!isprime(_MIPP_ x)) 
    {
        incr(_MIPP_ x,2,x);
        if (mr_mip->user!=NULL) if (!(*mr_mip->user)())
        {
            MR_OUT
            return FALSE;
        }
    }
    MR_OUT
    return TRUE;
}


BOOL nxsafeprime(_MIPD_ int type,int subset,big w,big p)
{ /* If type=0 finds next highest "safe" prime p >= w *
   * A safe prime is one for which q=(p-1)/2 is also  *
   * prime, and will be congruent to 3 mod 4          *
   * However if type=1 finds a prime p for which      *
   * q=(p+1)/2 is prime, congruent to 1 mod 4         *
   * Set subset=1 for q=1 mod 4, subset=3 for         *
   * q=3 mod 4, subset=0 if you don't care which      */

    int rem,increment;
#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (mr_mip->ERNUM) return FALSE;

    MR_IN(106)

    copy(w,p);

    rem=remain(_MIPP_ p,8);
    if (subset==0)
    {
        rem=rem%4;
        if (type==0) incr(_MIPP_ p,3-rem,p);
        else
        {
            if (rem>1) incr(_MIPP_ p,5-rem,p);
            else       incr(_MIPP_ p,1-rem,p);
        }
        increment=4;
    }
    else
    {
        if (subset==1) 
        {
            if (type==0)
            {
                if (rem>3) incr(_MIPP_ p,11-rem,p);
                else       incr(_MIPP_ p,3-rem,p);
            }
            else
            {
                if (rem>1) incr(_MIPP_ p,9-rem,p);
                else       incr(_MIPP_ p,1-rem,p);
            }
        }
        else 
        { 
            if (type==0) incr(_MIPP_ p,7-rem,p);
            else
            {
                if (rem>5) incr(_MIPP_ p,13-rem,p);
                else       incr(_MIPP_ p,5-rem,p);
            }
        }
        increment=8;
    }
    if (type==0) decr(_MIPP_ p,1,mr_mip->w10);
    else         incr(_MIPP_ p,1,mr_mip->w10);
    subdiv(_MIPP_ mr_mip->w10,2,mr_mip->w10);

    forever
    {
        do {
            if (mr_mip->user!=NULL) if (!(*mr_mip->user)())
            {
                MR_OUT
                return FALSE;
            }
            incr(_MIPP_ p,increment,p);
            incr(_MIPP_ mr_mip->w10,increment/2,mr_mip->w10);
        } while (!trial_division(_MIPP_ p,p) || !trial_division(_MIPP_ mr_mip->w10,mr_mip->w10));
        if (!isprime(_MIPP_ mr_mip->w10)) continue;
        if (isprime(_MIPP_ p)) break;
    }

    MR_OUT
    return TRUE;
}

