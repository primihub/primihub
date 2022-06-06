/*
 *   Program to calculate factorials.
 *
 *   Requires: big.cpp
 */

#include <iostream>
#include "big.h"   /* include MIRACL system */

using namespace std;

Miracl precision(5000,10);   /* bigs are 5000 decimal digits long */

int main()
{ /* calculate factorial of number */
    Big nf=1;           /* declare "Big" variable nf */
    int n;
    set_io_buffer_size(5000);
    cout << "factorial program\ninput number n= \n";
    cin >> n;
    while (n>1) nf*=(n--);   /* nf=n!=n*(n-1)*(n-2)*....3*2*1  */
    cout << "n!= \n" << nf << "\n";
    return 0;
}

