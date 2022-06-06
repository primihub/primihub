/*
 *   Program to calculate factorials.
 */

#include <stdio.h>
#include "miracl.h"   /* include MIRACL system */

int main()
{ /* calculate factorial of number */
    big nf;           /* declare "big" variable nf */
    int n,m;
#if MIRACL==16
#ifdef MR_FLASH
    mirsys(500,10);    /* initialise system to base 10, 500 digits per "big" */
#else
    mirsys(5000,10);   /* bigger numbers possible if no flash arithmetic     */
#endif
#else
    mirsys(5000,10);  /* 5000 digits per "big" */
#endif
    nf=mirvar(1);     /* initialise "big" variable nf=1 */
    printf("factorial program\n");
    printf("input number n= \n");
    m=scanf("%d",&n);
    getchar();
    while (n>1) premult(nf,n--,nf);   /* nf=n!=n*(n-1)*(n-2)*....3*2*1  */
    printf("n!= \n");
    otnum(nf,stdout); /* output result */ 
    return 0;
}

