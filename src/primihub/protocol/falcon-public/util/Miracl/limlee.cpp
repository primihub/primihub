/*
 *   Lim-Lee prime generation
 *
 *   See "A Key recovery Attack on Discrete Log-based Schemes using a Prime 
 *   Order Subgroup", Lim & Lee, Crypto '97
 *
 *   For certain Discrete Log based protocols these primes are preferable 
 *   to those generated for example by the utilty dssetup.c
 *   This program can be used in place of dssetup.cpp
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
 *   Requires: big.cpp
 *
 */

#include <iostream>
#include <fstream>
#include <iomanip>
#include "big.h"

using namespace std;

#define PBITS 1024
#define QBITS 160
#define OBITS 160    /* Size of pa, pb, pc etc., >=QBITS */ 
#define POOL_SIZE 20 /* greater than PBITS/OBITS */

Miracl precision=100;

static int num1bits(long x)
{ /* returns number of '1' bits in x */
    int n=0;
    while (x!=0) 
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
    ofstream common("common.dss");
    Big q,p,p1,x,h,t,g,low,high;
    Big pool[POOL_SIZE];
    BOOL fail;
    int i,j,p1bits,np;
    long seed,m,permutation;
    miracl *mip=&precision;

/* randomise */
    cout << "Enter 9 digit random number seed  = ";
    cin >> seed;
    irand(seed);
    
    p1bits=PBITS-QBITS-1;

/* find number of primes pa, pb, pc etc., that will be needed */

    np=1;
    while (p1bits/np >= OBITS) np++;
    np--;

/* find the high/low limits for these primes, so that 
   the generated prime p will be exactly PBITS in length */

    t=pow((Big)2,p1bits-1);
    low=root(t,np)+1;       /* np-th integer root */

    t=2*t-1;
    high=root(t,np);

    low=high-(high-low)/2;  /* raise low limit to half-way... */

/* generate q  */
    forever
    { /* make sure leading two bits of q 11... */
        q=pow((Big)2,QBITS)-rand(QBITS-2,2);
        while (!prime(q)) q+=1;
        if (bits(q)>QBITS) continue;
        break;
    }
    cout << "q= (" << bits(q) << " bits)" << endl;
    cout << q << endl;

/* generate prime pool from which permutations of np 
   primes will be picked until a Lim-Lee prime is found */

    for (i=0;i<POOL_SIZE;i++)
    { /* generate the primes pa, pb, pc etc.. */
        forever
        { 
            p1=rand(high);
            if (p1<low) continue;
            while (!prime(p1)) p1+=1;
            if (p1>high) continue;
            pool[i]=p1;
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
        p1=1;
        for (i=j=0,m=1L;j<np;i++,m<<=1)
        {
            if (i>=POOL_SIZE) 
            { /* ran out of primes... */
                fail=TRUE;
                break;
            }
            if (m&permutation) 
            {
                p1*=pool[i];
                j++;
            }
        } 
        if (fail) break;
        cout << "." << flush; 
        p=2*q*p1+1;  
        permutation=increment(permutation);
        if (bits(p)!=PBITS) continue;
        if (prime(p)) break; 
    } 

    if (fail)
    {
        printf("\nFailed - very unlikely! - try increasing POOL_SIZE\n");
        return 0;
    }

    cout << "\np= (" << bits(p) << " bits)\n";
    cout << p << endl;

/* finally find g */

    do {
        h=rand(p-1);
        g=pow(h,(p-1)/q,p);
    } while (g==1);
  
    cout << "g= (" << bits(g) << " bits)\n";
    cout << g << endl;

    common << PBITS << endl;
    mip->IOBASE=16;
    common << p << endl;
    common << q << endl;
    common << g << endl;

    return 0;
}

