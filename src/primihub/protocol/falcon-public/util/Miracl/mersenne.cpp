/*
 *   Program to calculate mersenne primes
 *   using Lucas-Lehmer test - Knuth p.391
 *
 *   Try this only in a 32-bit (or better) environment
 *
 *   Requires: big.cpp
 *
 */

#include <iostream>
#include "big.h"

using namespace std;

#define LIMIT 100000

Miracl precision=1000; 

int main()
{ /* calculate mersenne primes */
    BOOL compo;
    Big L,m,T;
    int i,k,q,r,p;
    miracl *mip=&precision;
    gprime(LIMIT);
    for (k=1;;k++)
    { /* test only prime exponents */
        q=mip->PRIMES[k];
        if (q==0) break;
        m=pow((Big)2,q)-1;

/* try to find a factor. Should perhaps keep trying over a bigger range... */

        compo=FALSE;
        for (i=2;i<16*q;i+=2)
        { /* prime factors (if they exist) are always *
           * of the form i*q+1, and 1 or 7 mod 8      */
            p=i*q+1;
            if ((p-1)%q!=0) break; /* check for overflow */
            if (p%8!=1 && p%8!=7) continue;
            if (spmd(3,p-1,p)!=1) continue; 
            r=m%p;
            if (r==0)  
            {
                if (m!=p)  compo=TRUE;
                break;
            }
        }
        if (compo) 
        {
            cout << "2^" << q << "-1 is NOT prime ; factor = " << p << endl;
            continue;
        }

        L=4;
        for(i=1;i<=q-2;i++)
        { /* Lucas-Lehmer test */
            L=L*L-2;
            T=(L>>q);       /* find remainder mod m */
            L=L-(T<<q)+T;
            if (L>=m) L-=m;
        }
        if (L==0)
        { /* mersenne prime found! */
            cout << "2^" << q << "-1 is prime" << endl;
            cout << m << endl;
        }
        else cout << "2^" << q << "-1 is NOT prime" << endl;
    }
    return 0;
}

