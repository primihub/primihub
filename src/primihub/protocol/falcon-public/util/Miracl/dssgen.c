/*
 *   Digital Signature Algorithm (DSA)
 *
 *   See Communications ACM July 1992, Vol. 35 No. 7
 *   This new standard for digital signatures has been proposed by 
 *   the American National Institute of Standards and Technology (NIST)
 *   under advisement from the National Security Agency (NSA). 
 *
 *   This program generates one set of public and private keys in files 
 *   public.dss and private.dss respectively
 *
 */

#include <stdio.h>
#include "miracl.h"

int main()
{
    FILE *fp;
    big p,q,g,x,y;
    long seed;
    int bits;
    miracl *mip;
/* get common data */
    fp=fopen("common.dss","rt");
    if (fp==NULL)
    {
        printf("file common.dss does not exist\n");
        return 0;
    }
    fscanf(fp,"%d\n",&bits);

    mip=mirsys(bits/4,16);    /* use Hex internally */
    p=mirvar(0);
    q=mirvar(0);
    g=mirvar(0);
    x=mirvar(0);
    y=mirvar(0);

    innum(p,fp);
    innum(q,fp);
    innum(g,fp);
    fclose(fp);

/* randomise */
    printf("Enter 9 digit random number seed  = ");
    scanf("%ld",&seed);
    getchar();
    irand(seed);

    powmod(g,q,p,y);
    if (size(y)!=1)
    {
        printf("Problem - generator g is not of order q\n");
        return 0;
    }

/* generate public/private keys */
    bigrand(q,x);
    powmod(g,x,p,y);
    printf("public key = ");
    otnum(y,stdout);
    fp=fopen("public.dss","wt");
    otnum(y,fp);
    fclose(fp);
    fp=fopen("private.dss","wt");
    otnum(x,fp);
    fclose(fp);
    mirexit();
    return 0;
}

