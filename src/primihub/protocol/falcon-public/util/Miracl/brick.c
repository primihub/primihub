/*
 *   Test program to implement the Comb method for fast
 *   computation of g^x mod n, for fixed g and n, using precomputation. 
 *   This idea can be used to substantially speed up certain phases 
 *   of the Digital Signature Standard (DSS).
 *
 *   See "Handbook of Applied Cryptography", CRC Press, 2001
 *
 */

#include <stdio.h>
#include "miracl.h"

int main()
{
    FILE *fp; 
    big e,n,g,a;
    brick binst;
    int window,nb,bits;
    miracl *mip=mirsys(100,0);
    n=mirvar(0);
    e=mirvar(0);
    a=mirvar(0);
    g=mirvar(0);
    fp=fopen("common.dss","rt");
    fscanf(fp,"%d\n",&bits);
    mip->IOBASE=16;
    cinnum(n,fp);
    cinnum(g,fp);
    cinnum(g,fp);  
    mip->IOBASE=10;  

    printf("modulus is %d bits in length\n",logb2(n));
    printf("Enter size of exponent in bits = ");
    scanf("%d",&nb);
    getchar();
    printf("Enter window size in bits (1-10)= ");
    scanf("%d",&window);
    getchar();

    if (!brick_init(&binst,g,n,window,nb))
    {
        printf("Failed to initialize\n");
        return 0;
    }

    printf("%d big numbers have been precomputed and stored\n",(1<<window));

    bigbits(nb,e);  /* random exponent */  

    printf("naive method\n");
    powmod(g,e,n,a);
    cotnum(a,stdout);

    printf("Comb method\n");
    pow_brick(&binst,e,a);

    brick_end(&binst);
    
    cotnum(a,stdout);

    return 0;
}


