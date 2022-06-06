/*
 *   Test program to find discrete logarithms using Pollard's lambda method
 *   for catching kangaroos. This algorithm appears to be the best 
 *   available for breaking the Diffie-Hellman key exchange algorithm
 *   and its variants.
 *
 *   See "Monte Carlo Methods for Index Computation"
 *   by J.M. Pollard in Math. Comp. Vol. 32 1978 pp 918-924
 *
 *   Requires: big.cpp
 *
 */

#include <iostream>
#include "big.h"   /* include MIRACL system */

using namespace std;

Miracl precision=50;

#define LIMIT 100000000L
#define LEAPS 10000      /* = square root of LIMIT */ 
#define ALPHA 16         /* primitive root */

static char *Modulus=(char *)
"295NZNjq8kIndq5Df0NDrA3qk4wxKpbXX4G5bC11A2lRKxcbnap2XDgE4X286glvmxDN66uSaeTjRMrelTY5WfLn";

int main()
{ /* Pollard's lambda algorithm for finding discrete logs  *
   * which are known to be less than a certain limit LIMIT */
    Big x,n,trap,table[32];
    int i,j,m;
    long dm,dn,s,distance[32];
    miracl *mip=&precision;
    for (s=1L,m=1;;m++)
    { /* find table size */
        distance[m-1]=s;
        s*=2;     
        if ((2*s/m)>(LEAPS/4)) break;
    }
    mip->IOBASE=60;    /* get large Modulus */
    n=Modulus;
    mip->IOBASE=10;
    cout << "solve discrete logarithm problem - using Pollard's kangaroos\n";
    cout << "finds x in y=" << ALPHA << "^x mod n, given y, for fixed n and small x\n";
    cout << "known to be less than " << LIMIT << "\n";
    cout << "n= " << n << endl;
    for (i=0;i<m;i++) 
    { /* create table */
        table[i]=pow(ALPHA,(Big)distance[i],n);
    }       
    x=pow(ALPHA,(Big)LIMIT,n);
    cout << "setting trap .... \n";
    for (dn=0L,j=0;j<LEAPS;j++)
    { /* set traps beyond LIMIT using tame kangaroo */
        i=x%m;  /* random function */
        x=modmult(x,table[i],n);
        dn+=distance[i];
    }
    cout << "trap set!\n";
    trap=x;
    forever
    { /* ready to solve */
        cout << "Enter x= ";
        cin >> x;
        if (x<=0) break;
        x=pow(ALPHA,x,n);
        cout << "y= " << x << endl;
        for (dm=0L;;)
        { /* unlease wild kangaroo - boing - boing ... */
            i=x%m;
            x=modmult(x,table[i],n);
            dm+=distance[i];
            if (x==trap || dm>LIMIT+dn) break;
        }
        if (dm>LIMIT+dn)
        { /* trap stepped over */
            cout << "trap failed\n";
            continue;
        }
        cout << "Gotcha!\n";
        cout << "Discrete log of y= " << LIMIT+dn-dm << "\n";
    }
    return 0;
}

