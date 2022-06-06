/*
 *   Example program
 *
 *   Requires: floating.cpp big.cpp 
 *
 */

#include <iostream>
#include "floating.h"

using namespace std;

Miracl precision(18,0); // 18=2^4+2

int main()
{ /* Brents example program */
    Float x,y,z;
    setprecision(4);   // 16=2^4
    cout << fpi() << endl;
    x=exp(fpi()*sqrt((Float)163/9));
    cout << x << endl;
    cout << pow(x,3) << endl;
    return 0;
}

