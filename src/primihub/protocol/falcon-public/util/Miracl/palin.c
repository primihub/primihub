/*
 *   Program to investigate palindromic reversals.
 *   Gruenberger F. Computer Recreations, Scientific American. April 1984.
 *
 */

#include <stdio.h>
#include "miracl.h"

BOOL rever(big x,big y)
{ /* reverse digits of x into y       *
   * returns TRUE if x is palindromic */
    int m,n;
    int i,k,swaps;
    BOOL palin;
    copy(x,y);
    palin=TRUE;
    k=numdig(y)+1;
    swaps=k/2;
    for (i=1;i<=swaps;i++)
    {
        k--;
        m=getdig(y,k);
        n=getdig(y,i);
        if (m!=n) palin=FALSE;
        putdig(m,y,i);
        putdig(n,y,k);
    }
    return palin;
}

int main()
{  /*  palindromic reversals  */
    int iter;
    big x,y;
    mirsys(120,10);
    x=mirvar(0);
    y=mirvar(0);
    printf("palindromic reversals program\n");
    printf("input number\n");
    innum(x,stdin);
    iter=0;
    while (!rever(x,y))
    {
        iter++;
        add(x,y,x);
        otnum(x,stdout);
    }
    printf("palindromic after %d iterations\n",iter);
    return 0;
}

