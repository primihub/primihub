/*
 *   Program to generate RSA keys suitable for use with an encryption
 *   exponent of 3, and which are also 'Blum Integers'.
 *
 *   Note that an exponent of 3 must be used with extreme care!
 *
 *   For example:-
 *   If the same message m is encrypted under 3 or more different public keys
 *   the message can be recovered without factoring the modulus (using the
 *   Chinese remainder thereom).
 *   If the messages m and m+1 are encrypted under the same public key, again 
 *   the message can be recovered without factoring the modulus. Indeed any
 *   simple relationship between a pair of messages can be exploited in a
 *   similar way. 
 *
 */

#include <stdio.h>
#include "miracl.h"
#define NP 2            /* use two primes - could be more */
#define PRIME_BITS 512  /* >=256 bits */

static miracl *mip;

static big pd,pl,ph;

long randise(void)
{ /* get a random number */
    long seed;
    printf("Enter 9 digit random number seed  = ");
    scanf("%ld",&seed);
    getchar();
    return seed;
}

void strongp(big p,int n,long seed1,long seed2)
{ /* generate strong prime number =11 mod 12 suitable for RSA encryption */
    int r,r1,r2;
    irand(seed1);
    bigbits(2*n/3,pd);
    nxprime(pd,pd);
    expb2(n-1,ph);
    divide(ph,pd,ph);
    expb2(n-2,pl);
    divide(pl,pd,pl);
    subtract(ph,pl,ph);
    irand(seed2);
    bigrand(ph,ph);
    add(ph,pl,ph);
    r1=subdiv(pd,12,pl);
    r2=subdiv(ph,12,pl);
    r=0;
    while ((r1*(r2+r))%12!=5) r++;
    incr(ph,r,ph);
    do 
    { /* find p=2*r*pd+1 = 11 mod 12 */
        multiply(ph,pd,p);
        premult(p,2,p);
        incr(p,1,p);
        incr(ph,12,ph);
    } while (!isprime(p));
}

int main()
{  /*  calculate public and private keys  *
    *  for rsa encryption                 */
    int i;
    long seed[2*NP];
    big p[NP],ke;
    FILE *outfile;
    mip=mirsys(100,0);
    gprime(15000);  /* speeds up large prime generation */
    for (i=0;i<NP;i++) p[i]=mirvar(0);
    ke=mirvar(0);
    pd=mirvar(0);
    pl=mirvar(0);
    ph=mirvar(0);
    printf("Generating %d primes, each %d bits in length\n",NP,PRIME_BITS);
    for (i=0;i<2*NP;i++)
        seed[i]=randise();
    printf("generating keys - please wait\n");
    convert(1,ke);
    for (i=0;i<NP;i++) 
    {
        strongp(p[i],PRIME_BITS,seed[2*i],seed[2*i+1]);
        multiply(ke,p[i],ke);
    }
    printf("key length= %d bits\n",logb2(ke));

    printf("public encryption key\n");
    cotnum(ke,stdout);
    printf("private decryption key\n");
    for (i=0;i<NP;i++)
        cotnum(p[i],stdout);
    mip->IOBASE=16;
    outfile=fopen("public.key","wt");
    cotnum(ke,outfile);
    fclose(outfile);
    outfile=fopen("private.key","wt");
    for (i=0;i<NP;i++)
        cotnum(p[i],outfile);
    fclose(outfile);
    return 0;
}

