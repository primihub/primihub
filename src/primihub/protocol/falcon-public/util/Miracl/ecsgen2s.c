/*
 *   Proposed Digital Signature Standard
 *
 *   Elliptic Curve Variation GF(2^m) - See Dr. Dobbs Journal April 1997
 *
 *   This program generates one set of public and private keys in files 
 *   public.ecs and private.ecs respectively. Notice that the public key 
 *   can be much shorter in this scheme, for the same security level.
 *
 *   It is assumed that Curve parameters are to be found in file common2.ecs
 *
 *   The curve is y^2+xy = x^3+Ax^2+B over GF(2^m) using a trinomial or 
 *   pentanomial basis (t^m+t^a+1 or t^m+t^a+t^b+t^c+1). These parameters
 *   can be generated using the findbase.cpp example program, or taken from tables
 *   provided, for example in IEEE-P1363 Annex A
 *
 *   The file common2.ecs is presumed to exist and contain 
 *   {m,A,B,q,x,y,a,b,c} where A and B are parameters of the equation 
 *   above, (x,y) is an initial point on the curve, {m,a,b,c} are the field 
 *   parameters, (b is zero for a trinomial) and q is the order of the 
 *   (x,y) point, itself a large prime. The number of points on the curve is 
 *   cf.q where cf is the "co-factor", normally 2 or 4.
 * 
 *   This program is written for static mode.
 *   For a 163-bit modulus p, MR_STATIC could be defined as 6 in mirdef.h
 *   for a 32-bit processor, or 11 for a 16-bit processor (11*16 > 163).
 *   The system parameters can be found in the file common2.ecs
 *   Assumes MR_GENERIC_MT is defined in mirdef.h
 *
 */

#include <stdio.h>
#include "miracl.h"

int main()
{
    FILE *fp;
    int ep,m,a,b,c;
    epoint *g,*w;
    big a2,a6,q,x,y,d;
    long seed;
    miracl instance;
    miracl *mip=&instance;
    char mem[MR_BIG_RESERVE(6)];            /* reserve space on the stack for 6 bigs */
    char mem1[MR_ECP_RESERVE(2)];           /* and two elliptic curve points         */
    memset(mem,0,MR_BIG_RESERVE(6));
    memset(mem1,0,MR_ECP_RESERVE(2));
  
    fp=fopen("common2.ecs","rt");
    if (fp==NULL)
    {
        printf("file common2.ecs does not exist\n");
        return 0;
    }
    fscanf(fp,"%d\n",&m);

    mip=mirsys(mip,MR_ROUNDUP(abs(m),4),16);  /* MR_ROUNDUP(a/b) rounds up a/b */
    a2=mirvar_mem(mip,mem,0);
    a6=mirvar_mem(mip,mem,1);
    q=mirvar_mem(mip,mem,2);
    x=mirvar_mem(mip,mem,3);
    y=mirvar_mem(mip,mem,4);
    d=mirvar_mem(mip,mem,5);

    innum(mip,a2,fp);
    innum(mip,a6,fp);
    innum(mip,q,fp);
    innum(mip,x,fp);
    innum(mip,y,fp);

    fscanf(fp,"%d\n",&a);
    fscanf(fp,"%d\n",&b);
    fscanf(fp,"%d\n",&c);
    fclose(fp);

/* randomise */
    printf("Enter 9 digit random number seed  = ");
    scanf("%ld",&seed);
    getchar();
    irand(mip,seed);
    ecurve2_init(mip,m,a,b,c,a2,a6,FALSE,MR_PROJECTIVE);  /* initialise curve */

    g=epoint_init_mem(mip,mem1,0);
    w=epoint_init_mem(mip,mem1,1);

    if (!epoint2_set(mip,x,y,0,g)) /* initialise point of order q */
    {
        printf("Problem - point (x,y) is not on the curve\n");
        return 0;
    }

    ecurve2_mult(mip,q,g,w);
    if (!point_at_infinity(w))
    {
        printf("Problem - point (x,y) is not of order q\n");
        return 0;
    }

/* generate public/private keys */

    bigrand(mip,q,d);
    ecurve2_mult(mip,d,g,g);
    
    ep=epoint2_get(mip,g,x,x); /* compress point */

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

