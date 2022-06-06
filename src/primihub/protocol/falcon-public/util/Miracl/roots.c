/*
 *   Program to calculate square roots
 */

#include <stdio.h>
#include "miracl.h"

int main()
{ /* Find roots */
    flash x,y;
    miracl *mip=mirsys(40,0);
    x=mirvar(0);
    y=mirvar(0);
    mip->RPOINT=ON;
    printf("enter number\n");
    cinnum(x,stdin);
    printf("to the power of 1/2\n");
    froot(x,2,y);
    cotnum(y,stdout);
    fpower(y,2,x);
    printf("to the power of 2 = \n");
    cotnum(x,stdout);
    if (mip->EXACT) printf("Result is exact!\n");
    return 0;
}

