/*
 *  Program to factor big numbers using Pollards (p-1) method.
 *  Works when for some prime divisor p of n, p-1 has itself
 *  only small factors.
 *  See "Speeding the Pollard and Elliptic Curve Methods"
 *  by Peter Montgomery, Math. Comp. Vol. 48 Jan. 1987 pp243-264
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "miracl.h"

#define LIMIT1 10000    /* must be int, and > MULT/2 */
#define LIMIT2 1000000L  /* may be long */
#define MULT   2310      /* must be int, product of small primes 2.3.. */
#define NEXT   13        /* next small prime */

miracl *mip;
static BOOL plus[1+MULT/2],minus[1+MULT/2];

void marks(long start)
{ /* mark non-primes in this interval. Note    *
   * that those < NEXT are dealt with already  */
    int i,pr,j,k;
    for (j=1;j<=MULT/2;j+=2) plus[j]=minus[j]=TRUE;
    for (i=0;;i++)
    { /* mark in both directions */
        pr=mip->PRIMES[i];
        if (pr<NEXT) continue;
        if ((long)pr*pr>start) break;
        k=pr-start%pr;
        for (j=k;j<=MULT/2;j+=pr)
            plus[j]=FALSE;
        k=start%pr;
        for (j=k;j<=MULT/2;j+=pr)
            minus[j]=FALSE;
    }        
}

int main()
{ /* factoring program using Pollards (p-1) method */
    long i,p,pa,interval;
    big n,t,b,bw,bvw,bd,bp,q;
    static big bu[1+MULT/2];
    static BOOL cp[1+MULT/2];
    int phase,m,pos,btch,iv;
    mip=mirsys(30,0);
    n=mirvar(0);
    t=mirvar(0);
    b=mirvar(0);
    q=mirvar(0);
    bw=mirvar(0);
    bvw=mirvar(0);
    bd=mirvar(0);
    bp=mirvar(0);
    gprime(LIMIT1);
    for (m=1;m<=MULT/2;m+=2)
        if (igcd(MULT,m)==1)
        {
            bu[m]=mirvar(0);
            cp[m]=TRUE;
        }
        else cp[m]=FALSE;
    printf("input number to be factored\n");
    cinnum(n,stdin);
    if (isprime(n))
    {
        printf("this number is prime!\n");
        return 0;
    }
    phase=1;
    p=0;
    btch=50;
    i=0;
    convert(2,b); /* if "degenerate case" comes up (see below) try 3 */
    printf("phase 1 - trying all primes less than %d\n",LIMIT1);
    printf("prime= %8ld",p);
    forever
    {
        if (phase==1)
        { /* looking for all factors of (p-1) < LIMIT1 */
            p=mip->PRIMES[i];
            if (mip->PRIMES[i+1]==0)
            {
                 phase=2;
                 printf("\nphase 2 - trying last prime less than %ld\n"
                        ,LIMIT2);
                 printf("prime= %8ld",p);
                 power(b,8,n,bw);
                 convert(1,t);
                 copy(b,bp);
                 copy(b,bu[1]);
                 for (m=3;m<=MULT/2;m+=2)
                 { /* store bu[m] = b^(m*m) */
                     mad(t,bw,bw,n,n,t);
                     mad(bp,t,t,n,n,bp);
                     if (cp[m]) copy(bp,bu[m]);
                 }
                 power(b,MULT,n,t);
                 power(t,MULT,n,t);
                 mad(t,t,t,n,n,bd);        /* bd=b^(2*MULT*MULT) */
                 iv=(int)(p/MULT);
                 if (p%MULT>MULT/2) iv++;
                 interval=(long)iv*MULT;
                 p=interval+1;
                 marks(interval);
                 power(t,2*iv-1,n,bw);
                 power(t,iv,n,bvw);
                 power(bvw,iv,n,bvw);      /* bvw = b^(MULT*MULT*iv*iv) */
                 subtract(bvw,bu[p%MULT],q);
                 btch*=100;
                 i++;
                 continue;
            }
            pa=p;
            while ((LIMIT1/p) > pa) pa*=p;
            power(b,(int)pa,n,b);
            decr(b,1,q);
        }
        else
        { /* looking for last prime factor of (p-1) < LIMIT2 */
            p+=2;
            pos=(int)(p%MULT);
            if (pos>MULT/2) 
            { /* increment giant step */
                iv++;
                interval=(long)iv*MULT;
                p=interval+1;
                marks(interval);
                pos=1;
                mad(bw,bd,bd,n,n,bw);
                mad(bvw,bw,bw,n,n,bvw);
            }
            if (!cp[pos]) continue;

        /* if neither interval+/-pos is prime, don't bother */
                if (!plus[pos] && !minus[pos]) continue;
            subtract(bvw,bu[pos],t);
            mad(q,t,t,n,n,q);  /* batching gcds */
        }
        if (i++%btch==0)
        { /* try for a solution */
            printf("\b\b\b\b\b\b\b\b%8ld",p);
            fflush(stdout);
            egcd(q,n,t);
            if (size(t)==1)
            {
                if (p>LIMIT2) break;
                else continue;
            }
            if (compare(t,n)==0)
            {
                printf("\ndegenerate case");
                break;
            }
            printf("\nfactors are\n");
            if (isprime(t)) printf("prime factor     ");
            else          printf("composite factor ");
            cotnum(t,stdout);
            divide(n,t,n);
            if (isprime(n)) printf("prime factor     ");
            else          printf("composite factor ");
            cotnum(n,stdout);
            return 0;
        }
    }
    printf("\nfailed to factor\n");
    return 0; 
}

