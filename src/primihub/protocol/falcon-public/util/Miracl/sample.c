/*
 *   Example program
 */

#include <stdio.h>
#include "miracl.h"

int main()
{ /* Brents example program */
    flash x,pi;
    miracl *mip=mirsys(-35,0);
    x=mirvar(0);
    pi=mirvar(0);
    mip->RPOINT=ON;
    printf("Calculating pi..\n");
    fpi(pi);
    cotnum(pi,stdout); /* output pi */
    printf("Calculating exp(pi*(163/9)^0.5)\n");
    fconv(163,9,x);
    froot(x,2,x);
    fmul(x,pi,x);
    fexp(x,x);
    cotnum(x,stdout);
    printf("Calculating exp(pi*(163)^0.5)\n");
    fpower(x,3,x);
    cotnum(x,stdout);
    return 0;
}

