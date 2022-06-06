/*
 *   Test program for fast computation of xG for fixed G using precomputation. 
 *   This idea can be used to substantially speed up certain phases 
 *   of the EC Digital Signature Standard (ECDSA).
 *
 *   See "Handbook of Applied Cryptography"
 *
 */

#include <stdio.h>
#include "miracl.h"

int main()
{
    FILE *fp;
    big e,n,a,b,x,y,r;
    epoint *g;
    ebrick binst;
    int nb,bits,window,len,bptr,m,i,j;
    miracl *mip=mirsys(50,0);
    n=mirvar(0);
    e=mirvar(0);
    a=mirvar(0);
    b=mirvar(0);
    x=mirvar(0);
    y=mirvar(0);
    r=mirvar(0);
#ifndef MR_EDWARDS
    fp=fopen("common.ecs","rt");
#else
    fp=fopen("edwards.ecs","rt");
#endif
	fscanf(fp,"%d\n",&bits);
    mip->IOBASE=16;
    cinnum(n,fp);
    cinnum(a,fp);
    cinnum(b,fp);
    cinnum(r,fp);
    cinnum(x,fp);
    cinnum(y,fp);
    mip->IOBASE=10;

    printf("modulus is %d bits in length\n",logb2(n));
    printf("Enter max. size of exponent in bits = ");
    scanf("%d",&nb);
    getchar();
    printf("Enter window size in bits (1-10)= ");
    scanf("%d",&window);
    getchar();

    ebrick_init(&binst,x,y,a,b,n,window,nb);

/* Print out the precomputed table (for use in ecdhp.c ?) 
   In which case make sure that MR_SPECIAL is defined and 
   active in the build of this program, so MR_COMBA must
   also be defined as the number of words in the modulus *

len=MR_ROUNDUP(bits,MIRACL);
bptr=0;
for (i=0;i<2*(1<<window);i++)
{
    for (j=0;j<len;j++)
    {
        printf("0x%x,",binst.table[bptr++]);
    }
    printf("\n");
}

*/

    printf("%d elliptic curve points have been precomputed and stored\n",(1<< window));

    bigbits(nb,e);  /* random exponent */  

    printf("naive method\n");
    ecurve_init(a,b,n,MR_PROJECTIVE);
    g=epoint_init();
    epoint_set(x,y,0,g);
    ecurve_mult(e,g,g);
    epoint_get(g,x,y);
    cotnum(x,stdout);
    cotnum(y,stdout);

    printf("Comb method\n");
    mul_brick(&binst,e,x,y);

    ebrick_end(&binst);
    
    cotnum(x,stdout);
    cotnum(y,stdout);

    return 0;
}


