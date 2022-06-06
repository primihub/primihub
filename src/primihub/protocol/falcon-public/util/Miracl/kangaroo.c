/*
 *   Test program to find discrete logarithms using Pollard's lambda method
 *   for catching kangaroos. This algorithm appears to be the best 
 *   available for breaking the Diffie-Hellman key exchange algorithm and
 *   its variants
 *
 *   See "Monte Carlo Methods for Index Computation"
 *   by J.M. Pollard in Math. Comp. Vol. 32 1978 pp 918-924
 */

#include <stdio.h>
#include <string.h>
#include "miracl.h"

#define LIMIT 100000000L
#define LEAPS 10000      /* = square root of LIMIT */ 
#define ALPHA 16         /* primitive root */

static char *modulus=
"295NZNjq8kIndq5Df0NDrA3qk4wxKpbXX4G5bC11A2lRKxcbnap2XDgE4X286glvmxDN66uSaeTjRMrelTY5WfLn";

int main()
{ /* Pollard's lambda algorithm for finding discrete logs  *
   * which are known to be less than a certain limit LIMIT */
    big x,n,t,trap,table[32];
    int i,j,m;
    long dm,dn,s,distance[32];
    miracl *mip=mirsys(50,0);
    x=mirvar(0);
    n=mirvar(0);
    t=mirvar(0);
    trap=mirvar(0);
    for (s=1L,m=1;;m++)
    { /* find table size */
        distance[m-1]=s;
        s*=2;     
        if ((2*s/m)>(LEAPS/4)) break;
    }
    mip->IOBASE=60;    /* get large modulus */
    cinstr(n,modulus);
    mip->IOBASE=10;
    printf("solve discrete logarithm problem - using Pollard's kangaroos\n");
    printf("finds x in y=%d^x mod n, given y, for fixed n and small x\n",ALPHA);
    printf("known to be less than %ld\n",LIMIT);
    printf("n= ");
    cotnum(n,stdout);
    for (i=0;i<m;i++) 
    { /* create table */
        lgconv(distance[i],t);
        table[i]=mirvar(0);
        powltr(ALPHA,t,n,table[i]);
    }       
    lgconv(LIMIT,t);
    powltr(ALPHA,t,n,x);
    printf("setting trap .... \n");
    for (dn=0L,j=0;j<LEAPS;j++)
    { /* set traps beyond LIMIT using tame kangaroo */
        i=subdiv(x,m,t);    /* random function */
        mad(x,table[i],x,n,n,x);
        dn+=distance[i];
    }
    printf("trap set!\n");
    copy(x,trap);
    forever
    { /* ready to solve */
        printf("Enter x= ");
        cinnum(x,stdin);
        if (size(x)<=0) break;
        powltr(ALPHA,x,n,t);
        printf("y= ");
        cotnum(t,stdout);
        copy(t,x);
        for (dm=0L;;)
        { /* unlease wild kangaroo - boing - boing ... */
            i=subdiv(x,m,t);
            mad(x,table[i],x,n,n,x);
            dm+=distance[i];
            if (compare(x,trap)==0 || dm>LIMIT+dn) break;
        }
        if (dm>LIMIT+dn)
        { /* trap stepped over */
            printf("trap failed\n");
            continue;
        }
        printf("Gotcha!\n");
        printf("Discrete log of y= %ld\n",LIMIT+dn-dm);
    }
    return 0;
}

