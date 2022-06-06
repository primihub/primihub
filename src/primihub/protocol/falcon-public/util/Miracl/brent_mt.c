/*
 *   Program to factor big numbers using Brent-Pollard method.
 *   See "An Improved Monte Carlo Factorization Algorithm"
 *   by Richard Brent in BIT Vol. 20 1980 pp 176-184
 */

#include <stdio.h>
#include "miracl.h"

#define mr_min(a,b) ((a) < (b)? (a) : (b))

int main()
{  /*  factoring program using Brents method */
    long k,r,i,m,iter;
    big x,y,z,n,q,ys,c3;
    miracl instance;      /* create miracl workspace on the stack */
#ifndef MR_STATIC
    miracl *mip=mirsys(&instance,16,0);     /* initialise miracl workspace and define size of bigs here */
    char *mem=memalloc(mip,7);              /* allocate space from the heap for 7 bigs */;
#else
    miracl *mip=mirsys(&instance,MR_STATIC,0); /* size of bigs is fixed */
    char mem[MR_BIG_RESERVE(7)];            /* reserve space on the stack for 7 bigs */
    memset(mem,0,MR_BIG_RESERVE(7));        /* clear this memory */
#endif

    x=mirvar_mem(mip,mem,0);                /* initialise the 7 bigs */
    y=mirvar_mem(mip,mem,1);
    ys=mirvar_mem(mip,mem,2);
    z=mirvar_mem(mip,mem,3);
    n=mirvar_mem(mip,mem,4);
    q=mirvar_mem(mip,mem,5);
    c3=mirvar_mem(mip,mem,6);
    convert(mip,3,c3);

    printf("input number to be factored\n");
    cinnum(mip,n,stdin);
    if (isprime(mip,n))
    {
        printf("this number is prime!\n");
        return 0;
    }
    m=10L;
    r=1L;
    iter=0L;
    do
    {
        printf("iterations=%5ld",iter);
        convert(mip,1,q);
        do
        {
            copy(y,x);
            for (i=1L;i<=r;i++)
                mad(mip,y,y,c3,n,n,y);
            k=0;
            do
            {
                iter++;
                if (iter%10==0) printf("\b\b\b\b\b%5ld",iter);
                fflush(stdout);  
                copy(y,ys);
                for (i=1L;i<=mr_min(m,r-k);i++)
                {
                    mad(mip,y,y,c3,n,n,y);
                    subtract(mip,y,x,z);
                    mad(mip,z,q,q,n,n,q);
                }
                egcd(mip,q,n,z);
                k+=m;
            } while (k<r && size(z)==1);
            r*=2;
        } while (size(z)==1);
        if (compare(z,n)==0) do 
        { /* back-track */
            mad(mip,ys,ys,c3,n,n,ys);
            subtract(mip,ys,x,z);
        } while (egcd(mip,z,n,z)==1);
        if (!isprime(mip,z))
             printf("\ncomposite factor ");
        else printf("\nprime factor     ");
        cotnum(mip,z,stdout);
        if (compare(z,n)==0) return 0;
        divide(mip,n,z,n);
        divide(mip,y,n,n);
    } while (!isprime(mip,n));
    printf("prime factor     ");
    cotnum(mip,n,stdout);
    return 0;
}

