/*
 *   Test program to implement Comb method for fast
 *   computation of g^x mod n, for fixed g and n, using precomputation. 
 *   This idea can be used to substantially speed up certain phases 
 *   of the Digital Signature Standard (DSS).
 *
 *   See "Handbook of Applied Cryptography", CRC Press, 2001
 *
 *   Requires: big.cpp
 *
 */

#include <iostream>
#include <fstream>
#include "brick.h"   /* include MIRACL system */

using namespace std;

Miracl precision=100;

int main()
{
    ifstream common("common.dss");
    Big a,e,n,g;
    int w,nb,bits;
    miracl *mip=&precision;
    common >> bits;
    mip->IOBASE=16;
    common >> n >> g >> g;
    mip->IOBASE=10;

    w=8; // window size
    
    cout << "Enter size of exponent in bits = ";
    cin >> nb;

    Brick b(g,n,w,nb); 

    e=rand(nb,2); /* random exponent */

    cout << "naive method" << endl;
    a=pow(g,e,n);
    cout << a << endl;

    cout << "Comb method" << endl;

    a=b.pow(e);

    cout << a << endl;
    return 0;
}

