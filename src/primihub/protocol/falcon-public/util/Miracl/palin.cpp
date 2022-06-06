/*
 *   Program to investigate palindromic reversals.
 *   Gruenberger F. Computer Recreations, Scientific American. April 1984.
 *
 *   Requires: big.cpp
 *
 */

#include <iostream>
#include "big.h"   /* include MIRACL system */

using namespace std;

Miracl precision(120,10);

BOOL rever(Big &x,Big &y)
{ /* reverse digits of x into y       *
   * returns TRUE if x is palindromic */
    int m,n;
    int i,k,swaps;
    BOOL palin;
    y=x;
    palin=TRUE;
    k=y.len()+1;
    swaps=k/2;
    for (i=1;i<=swaps;i++)
    {
        k--;
        m=y.get(k);
        n=y.get(i);
        if (m!=n) palin=FALSE;
        y.set(i,m);
        y.set(k,n);
    }
    return palin;
}

int main()
{  /*  palindromic reversals  */
    int iter;
    Big x,y;
    cout << "palindromic reversals program\n";
    cout << "input number\n";
    cin >> x;
    iter=0;
    while (!rever(x,y))
    {
        iter++;
        x+=y;
        cout << x << endl;
    }
    cout << "palindromic after " << iter << " iterations\n";
    return 0;
}

