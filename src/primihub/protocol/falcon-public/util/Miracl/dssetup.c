/*
 *   Digital Signature Standard
 *
 *   See Communications ACM July 1992, Vol. 35 No. 7
 *   This new standard for digital signatures has been proposed by 
 *   the American National Institute of Standards and Technology (NIST)
 *   under advisement from the National Security Agency (NSA). 
 *
 *   This program generates the common values p, q and g to a file
 *   common.dss
 */

#include <stdio.h>
#include "miracl.h"

#define QBITS 160
#define PBITS 1024

int main()
{
    FILE *fp;
    big p,q,h,g,n,s,t;
    long seed;
    miracl *mip=mirsys(100,0);
    p=mirvar(0);
    q=mirvar(0);
    h=mirvar(0);
    g=mirvar(0);
    n=mirvar(0);
    s=mirvar(0);
    t=mirvar(0);

/* randomise */
    printf("Enter 9 digit random number seed  = ");
    scanf("%ld",&seed);
    getchar();
    irand(seed);

/* generate q */
    forever 
    {
        bigbits(QBITS,q);
        nxprime(q,q);
        if (logb2(q)>QBITS) continue;
        break;
    }
    printf("q= ");
    cotnum(q,stdout);

/* generate p */
    expb2(PBITS,t);
    decr(t,1,t);
    premult(q,2,n);
    divide(t,n,t);
    expb2(PBITS-1,s);
    decr(s,1,s);
    divide(s,n,s);
    forever 
    {
        bigrand(t,p);
        if (compare(p,s)<=0) continue;
        premult(p,2,p);
        multiply(p,q,p);
        incr(p,1,p);
        copy(p,n);
        if (isprime(p)) break;
    } 
    printf("p= ");
    cotnum(p,stdout);

/* generate g */
    do {
        decr(p,1,t);
        bigrand(t,h);
        divide(t,q,t);
        powmod(h,t,p,g);
    } while (size(g)==1);    
    printf("g= ");
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

