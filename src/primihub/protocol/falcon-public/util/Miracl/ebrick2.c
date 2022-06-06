/*
 *   Test program to implement Comb method for fast
 *   computation of x*G, for fixed G, using precomputation. 
 *   This idea can be used to substantially speed up certain phases 
 *   of the Elliptic Curve Digital Signature Standard (DSS).
 *
 *   See "Handbook of Applied Cryptography"
 *
 */

#include <stdio.h>
#include "miracl.h"

int main()
{
    FILE *fp;
    int m,a,b,c;
    big e,a2,a6,x,y,r;
    epoint *g;
    ebrick2 binst;
    int i,j,len,bptr,nb,window;
    miracl *mip=mirsys(50,0);
    e=mirvar(0);
    a2=mirvar(0);
    a6=mirvar(0);
    x=mirvar(0);
    y=mirvar(0);
    r=mirvar(0);

    fp=fopen("common2.ecs","rt");
    fscanf(fp,"%d\n",&m);
    mip->IOBASE=16;
    cinnum(a2,fp);
    cinnum(a6,fp);
    cinnum(r,fp);
    cinnum(x,fp);
    cinnum(y,fp);
    mip->IOBASE=10;

    fscanf(fp,"%d\n",&a);
    fscanf(fp,"%d\n",&b);
    fscanf(fp,"%d\n",&c);
    
    printf("modulus is %d bits in length\n",m);
    printf("Enter size of exponent in bits = ");
    scanf("%d",&nb);
    getchar();
    printf("Enter window size in bits (1-10)= ");
    scanf("%d",&window);
    getchar();


    if (!ebrick2_init(&binst,x,y,a2,a6,m,a,b,c,window,nb))
    {
        printf("Failed to Initialize\n");
        return 0;
    }
/* Print out the precomputed table (for use in ecdh2m.c ?) 

len=MR_ROUNDUP(m,MIRACL);
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

    printf("%d points have been precomputed and stored\n",(1<<window));

    bigbits(nb,e);  /* random exponent */  

    printf("naive method\n");
    ecurve2_init(m,a,b,c,a2,a6,FALSE,MR_PROJECTIVE);
    g=epoint_init();
    epoint2_set(x,y,0,g);
    ecurve2_mult(e,g,g);
    epoint2_get(g,x,y);
    cotnum(x,stdout);
    cotnum(y,stdout);

    zero(x); zero(y);
    printf("Comb method\n");
    mul2_brick(&binst,e,x,y);
    ebrick2_end(&binst);
    
    cotnum(x,stdout);
    cotnum(y,stdout);

    return 0;
}


