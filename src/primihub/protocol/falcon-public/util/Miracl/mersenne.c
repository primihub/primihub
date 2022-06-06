/*
 *   Program to calculate mersenne primes
 *   using Lucas-Lehmer test - Knuth p.391
 *
 *   Try this only in a 32-bit (or better!) environment
 *
 */

#include <stdio.h>
#include "miracl.h"
#define LIMIT 100000

int main()
{ /* calculate mersenne primes */
    BOOL compo;
    big L,m,T;
    int i,k,q,p,r;
    miracl *mip=mirsys(5000,0);
    L=mirvar(0);
    m=mirvar(0);
    T=mirvar(0);
    gprime(LIMIT);
    for (k=1;;k++)
    { /* test only prime exponents */
        q=mip->PRIMES[k];
        if (q==0) break;
        expb2(q,m);
        decr(m,1,m);    /* m=2^q-1 */

/* try to find a factor. Should perhaps keep trying over a bigger range... */

        compo=FALSE;
        for(i=2;i<16*q;i+=2)
        { /* prime factors (if they exist) are always *
           * of the form i*q+1, and 1 or 7 mod 8      */
            p=i*q+1;
            if ((p-1)%q!=0) break; /* check for overflow */
            if (p%8!=1 && p%8!=7) continue;
            if (spmd(3,p-1,p)!=1) continue;   /* check for prime p */

            r=subdiv(m,p,T);
            if (r==0)  
            {
                if (size(T)!=1)  compo=TRUE;
                break;
            }
        }
        if (compo) 
        {
            printf("2^%d-1 is NOT prime ; factor = %7d\n",q,p);
            continue;
        }

        convert(4,L);
        for(i=1;i<=q-2;i++)
        { /* Lucas-Lehmer test */
            fft_mult(L,L,L);
            decr(L,2,L);

            sftbit(L,-q,T);
            add(L,T,L);
            sftbit(T,q,T);
            subtract(L,T,L);
            if (compare(L,m)>=0) subtract(L,m,L);
        }
        if (size(L)==0)
        { /* mersenne prime found! */
            printf("2^%d-1 is prime = \n",q);
            cotnum(m,stdout);
        }
        else printf("2^%d-1 is NOT prime\n",q);
    }
    return 0;
}

