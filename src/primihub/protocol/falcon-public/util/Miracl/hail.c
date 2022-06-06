/*
 *   Program to investigate hailstone numbers.
 *   Gruenberger F. 'Computer Recreations' Scientific American. April 1984.
 *
 */

#include <stdio.h>
#include "miracl.h"

int main ()
{  /*  hailstone numbers  */

    int iter,r;
    big x,y,mx;
    mirsys(400,10);
    x=mirvar(0);
    y=mirvar(0);
    mx=mirvar(0);
    iter=0;
    printf("number = \n");
    innum(x,stdin);
    do
    { /* main loop */
        if (compare(x,mx)>0) copy(x,mx);
        r=subdiv(x,2,y);
        if (r!=0)
        { /* what goes up ... */
            premult(x,3,x);
            incr(x,1,x);
        }
        /* ... must come down */
        else copy(y,x);
        otnum(x,stdout);
        iter++;
    } while (size(x)!=1);
    printf("path length = %d \n",iter);
    printf("maximum = \n");
    otnum(mx,stdout);
    return 0;
}

