/*
 *   Lim-Lee prime generation
 *
 *   See "A Key recovery Attack on Discrete Log-based Schemes using a Prime 
 *   Order Subgroup", Lim & Lee, Crypto '97
 *
 *   For certain Discrete Log based protocols these primes are preferable 
 *   to those generated for example by the utilty dssetup.c
 *   This program can be used in place of dssetup.c
 *
 *   When run the program generates a prime p PBITS in length, where 
 *   p-1=2*pa*pb*pc....*q, where q is a prime QBITS in length, and pa,pb,pc 
 *   etc are all primes > QBITS in length. Also generated is g, a generator 
 *   of the prime-order sub-group, of order q.
 *
 *   Finally p, q and g are output to the file common.dss
 *
 *   It may be quicker, and is quite valid, to make OBITS > QBITS.
 *   This allows the pool size to be decreased.
 *
 */

#include <stdio.h>
#include "miracl.h"

#define PBITS 1024
#define QBITS 160
#define OBITS 160    /* minimum size of primes pa,pb,pc etc >=QBITS */
#define POOL_SIZE 20 /* greater than PBITS/OBITS, < number of bits in long */

static int num1bits(long x)
{ /* returns number of '1' bits in x */
    int n=0;
    while (x!=0L) 
    {
        if (x&1L) n++;
        x>>=1;
    }
    return n;
}

static long increment(long permutation)
{ /* move onto next permutation with same number of '1' bits */
    int n=num1bits(permutation);
    do
    {
        permutation++;
    } while (num1bits(permutation)!=n);
    return permutation;
}

int main()
{
    FILE *fp;
    big q,p,p1,h,t,g,low,high;
    big pool[POOL_SIZE];
    BOOL fail;
    int i,j,p1bits,np;
    long seed,m,permutation;
    miracl *mip=mirsys(100,0);
    q=mirvar(0);
    p=mirvar(0);
    h=mirvar(0);
    t=mirvar(0);
    g=mirvar(0);
    p1=mirvar(0);
    low=mirvar(0);
    high=mirvar(0);
    gprime(10000);

/* randomise */
    printf("Enter 9 digit random number seed  = ");
    scanf("%ld",&seed);
    getchar();
    irand(seed);
    
    p1bits=PBITS-QBITS-1;

/* find number of primes pa, pb, pc etc., that will be needed */

    np=1;
    while (p1bits/np >= OBITS) np++;
    np--;

/* find the high/low limits for these primes, so that 
   the generated prime p will be exactly PBITS in length */

    expb2(p1bits-1,t);
    nroot(t,np,low);      /* np-th integer root */
    incr(low,1,low);

    premult(t,2,t);
    decr(t,1,t);
    nroot(t,np,high);

    subtract(high,low,t);   /* raise low limit up to half-way...  */
    subdiv(t,2,t);
    subtract(high,t,low);

/* generate q  */
    forever
    { /* make sure leading two bits of q 11... */
        expb2(QBITS,q);
        bigbits(QBITS-2,t);
        subtract(q,t,q);
        nxprime(q,q);
        if (logb2(q)>QBITS) continue;
        break;
    }
    printf("q= (%d bits)\n",logb2(q));
    cotnum(q,stdout);

/* generate prime pool from which permutations of np 
   primes will be picked until a Lim-Lee prime is found */

    for (i=0;i<POOL_SIZE;i++)
    { /* generate the primes pa, pb, pc etc.. */
        pool[i]=mirvar(0);
        forever
        { 
            bigrand(high,p1);
            if (compare(p1,low)<0) continue;
            nxprime(p1,p1);
            if (compare(p1,high)>0) continue;
            copy(p1,pool[i]);  
            break;
        }
    }

/* The '1' bits in the permutation indicate which primes are 
   picked from the pool. If np=5, start at 11111, then 101111 etc */

    permutation=1L;
    for (i=0;i<np;i++) permutation<<=1;
    permutation-=1;     /* permuation = 2^np-1 */

/* generate p   */
    fail=FALSE;
    forever 
    {
        convert(1,p1);
        for (i=j=0,m=1L;j<np;i++,m<<=1)
        {
            if (i>=POOL_SIZE) 
            { /* ran out of primes... */
                fail=TRUE;
                break;
            }
            if (m&permutation) 
            {
                multiply(p1,pool[i],p1); 
                j++;
            }
        } 
        if (fail) break;
        printf(".");   
        premult(q,2,p);
        multiply(p,p1,p);
        incr(p,1,p);
        permutation=increment(permutation);
        if (logb2(p)!=PBITS) continue;
        if (isprime(p)) break; 
    } 

    if (fail)
    {
        printf("\nFailed - very unlikely! - try increasing POOL_SIZE\n");
        return 0;
    }

    printf("\np= (%d bits)\n",logb2(p));
    cotnum(p,stdout);

/* finally find g */
    do {
        decr(p,1,t);
        bigrand(t,h);
        divide(t,q,t);
        powmod(h,t,p,g);
    } while(size(g)==1);    
   
    printf("g= (%d bits)\n",logb2(g));
    cotnum(g,stdout);

    fp=fopen("common.dss","wt");
    fprintf(fp,"%d\n",PBITS);
    mip->IOBASE=16;
    cotnum(p,fp);
    cotnum(q,fp);
    cotnum(g,fp);
    fclose(fp);
    return 0;
}

