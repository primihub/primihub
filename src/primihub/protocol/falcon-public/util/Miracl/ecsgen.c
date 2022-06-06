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
 */

#include <stdio.h>
#include <stdlib.h>
#include "miracl.h"

int main()
{
    FILE *fp;
    int ep,bits;
    epoint *g,*w;
    big a,b,p,q,x,y,d;
    long seed;
    miracl *mip;
   
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
    mip=mirsys(bits/4,16);  /* Use Hex internally */
    a=mirvar(0);
    b=mirvar(0);
    p=mirvar(0);
    q=mirvar(0);
    x=mirvar(0);
    y=mirvar(0);
    d=mirvar(0);

    innum(p,fp);
    innum(a,fp);
    innum(b,fp);
    innum(q,fp);
    innum(x,fp);
    innum(y,fp);
    
    fclose(fp);

/* randomise */
    printf("Enter 9 digit random number seed  = ");
    scanf("%ld",&seed);
    getchar();
    irand(seed);

    ecurve_init(a,b,p,MR_PROJECTIVE);  /* initialise curve */

    g=epoint_init();
    w=epoint_init();

    if (!epoint_set(x,y,0,g)) /* initialise point of order q */
    {
        printf("1. Problem - point (x,y) is not on the curve\n");
        exit(0);
    }

    ecurve_mult(q,g,w);
    if (!point_at_infinity(w))
    {
        printf("2. Problem - point (x,y) is not of order q\n");
        exit(0);
    }

/* generate public/private keys */

    bigrand(q,d);
    ecurve_mult(d,g,g);
    
    ep=epoint_get(g,x,x); /* compress point */

    printf("public key = %d ",ep);
    otnum(x,stdout);

    fp=fopen("public.ecs","wt");
    fprintf(fp,"%d ",ep);
    otnum(x,fp);
    fclose(fp);

    fp=fopen("private.ecs","wt");
    otnum(d,fp);
    fclose(fp);
    return 0;
}

