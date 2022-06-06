/*
 *   Program to investigate hailstone numbers.
 *   Gruenberger F. 'Computer Recreations' Scientific American. April 1984.
 *
 *   Requires: big.cpp
 */

#include <iostream>
#include "big.h"

using namespace std;

Miracl precision(400,10);

int main()
{  /*  hailstone numbers  */
    Big r,x,mx=0;
    int iter;
    iter=0;
    cout << "number = \n";   
    cin >> x;    
    do
    {
        if (x>mx) mx=x;    
        r=x%2;   
        if (r!=0) x=3*x+1;
        else      x/=2;
        cout << x << endl;
        iter++;
    } while (x!=1); 
    cout <<  "path length = " << iter << "\n";
    cout <<  "maximum = " << mx << "\n";
    return 0;
}

