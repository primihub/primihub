/*
 *   Program to generate "trap-door primes".
 *   Generates primes for use with "index" program
 *
 *   Hard to find factors of the product of two of them n=p.q
 *   - but easy to find discrete logs wrt p & q
 *   See "Non-Interactive Public-Key Cryptography", Maurer & Yacobi
 *   Eurocrypt '91
 *
 */

#include <stdio.h>
#include "miracl.h"
#define NPRIMES 6    /* =9 for > 256 bit primes */
#define PROOT 2

int main()
{ /* program to find a trap-door prime */
    BOOL found;
    int i,spins;
    long seed;
    big pp[NPRIMES],q,p,t;
    FILE *fp;
    mirsys(50,0);
    for (i=0;i<NPRIMES;i++) pp[i]=mirvar(0);
    q=mirvar(0);
    t=mirvar(0);
    p=mirvar(0);
    printf("Enter 9 digit seed= ");
    scanf("%ld",&seed);
    getchar();
    irand(seed);
    printf("Enter 4 digit seed= ");
    scanf("%d",&spins);
    getchar();
    for (i=0;i<spins;i++) brand();
    convert(2,pp[0]);
    do
    {  /* find prime p = 2.pp[1].pp[2]....+1 */
        convert(2,p);
        for (i=1;i<NPRIMES-1;i++)
        { /* generate all but last prime */
            bigdig(i+6,10,q);
            nxprime(q,pp[i]);
            multiply(p,pp[i],p);
        }
        do
        { /* find last prime component such that p is prime */
            nxprime(q,q);
            copy(q,pp[NPRIMES-1]);
            multiply(p,pp[NPRIMES-1],t);
            incr(t,1,t);
        } while(!isprime(t));
        copy(t,p);
        found=TRUE;
        for (i=0;i<NPRIMES;i++)
        { /* check that PROOT is a primitive root */
            decr(p,1,q);
            divide(q,pp[i],q);
            powltr(PROOT,q,p,t);
            if (size(t)==1) 
            {
                found=FALSE;
                break;
            }
        }
    } while (!found);
    fp=fopen("prime.dat","wt");
    fprintf(fp,"%d\n",NPRIMES);
    for (i=0;i<NPRIMES;i++) cotnum(pp[i],fp);
    fclose(fp);
    printf("prime= \n");
    cotnum(p,stdout);
    return 0;
}

