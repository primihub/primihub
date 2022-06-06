/*
 *   Elliptic Curve Digital Signature Algorithm (ECDSA)
 *
 *
 *   This program generates one set of public and private keys in files 
 *   public.ecs and private.ecs respectively. Notice that the public key 
 *   can be much shorter in this scheme, for the same security level.
 *
 *   It is assumed that Curve parameters are to be found in file common.ecs
 *
 *   The curve is y^2=x^3+Ax+b mod p
 *
 *   The file common.ecs is presumed to exist, and to contain the domain
 *   information {p,A,B,q,x,y}, where A and B are curve parameters, (x,y) are
 *   a point of order q, p is the prime modulus, and q is the order of the 
 *   point (x,y). In fact normally q is the prime number of points counted
 *   on the curve. 
 * 
 *   This program is written for static mode.
 *   For a 160-bit modulus p, MR_STATIC could be defined as 5 in mirdef.h
 *   for a 32-bit processor, or 10 for a 16-bit processor.
 *   The system parameters can be found in the file common.ecs
 *   Assumes MR_GENERIC_MT is defined in mirdef.h
 *
 */

#include <stdio.h>
#include "miracl.h"

int main()
{
    FILE *fp;
    int ep,bits;
    epoint *g,*w;
    big a,b,p,q,x,y,d;
    long seed;
    miracl instance;
    miracl *mip=&instance;
    char mem[MR_BIG_RESERVE(7)];            /* reserve space on the stack for 7 bigs */
    char mem1[MR_ECP_RESERVE(2)];           /* and two elliptic curve points         */
    memset(mem,0,MR_BIG_RESERVE(7));
    memset(mem1,0,MR_ECP_RESERVE(2));
    
#ifndef MR_EDWARDS	
    fp=fopen("common.ecs","rt");
    if (fp==NULL)
    {
        printf("file common.ecs does not exist\n");
        return 0;
    }
    fscanf(fp,"%d\n",&bits); 
#else
    fp=fopen("edwards.ecs","rt");
    if (fp==NULL)
    {
        printf("file edwards.ecs does not exist\n");
        return 0;
    }
    fscanf(fp,"%d\n",&bits); 
#endif

    mirsys(mip,bits/4,16);                 /* Use Hex internally */
    a=mirvar_mem(mip,mem,0);
    b=mirvar_mem(mip,mem,1);
    p=mirvar_mem(mip,mem,2);
    q=mirvar_mem(mip,mem,3);
    x=mirvar_mem(mip,mem,4);
    y=mirvar_mem(mip,mem,5);
    d=mirvar_mem(mip,mem,6);

    innum(mip,p,fp);
    innum(mip,a,fp);
    innum(mip,b,fp);
    innum(mip,q,fp);
    innum(mip,x,fp);
    innum(mip,y,fp);
    
    fclose(fp);

/* randomise */
    printf("Enter 9 digit random number seed  = ");
    scanf("%ld",&seed);
    getchar();
    irand(mip,seed);

    ecurve_init(mip,a,b,p,MR_PROJECTIVE);  /* initialise curve */

    g=epoint_init_mem(mip,mem1,0);
    w=epoint_init_mem(mip,mem1,1);

    if (!epoint_set(mip,x,y,0,g)) /* initialise point of order q */
    {
        printf("Problem - point (x,y) is not on the curve\n");
        exit(0);
    }

    ecurve_mult(mip,q,g,w);
    if (!point_at_infinity(w))
    {
        printf("Problem - point (x,y) is not of order q\n");
        exit(0);
    }

/* generate public/private keys */

    bigrand(mip,q,d);
    ecurve_mult(mip,d,g,g);
    
    ep=epoint_get(mip,g,x,x); /* compress point */

    printf("public key = %d ",ep);
    otnum(mip,x,stdout);

    fp=fopen("public.ecs","wt");
    fprintf(fp,"%d ",ep);
    otnum(mip,x,fp);
    fclose(fp);

    fp=fopen("private.ecs","wt");
    otnum(mip,d,fp);
    fclose(fp);
/* clear all memory used */
    memset(mem,0,MR_BIG_RESERVE(7));
    memset(mem1,0,MR_ECP_RESERVE(2));
 
    return 0;
}

