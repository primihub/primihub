/*
 *   Test program to implement the Comb method for fast
 *   computation of e.G on the elliptic curve E(F_p) for fixed G and E, 
 *   using precomputation. 
 *   This idea can be used to substantially speed up certain phases 
 *   of the Elliptic Curve Digital Signature Standard (DSS).
 *
 *   See "Handbook of Applied Cryptography", CRC Press, 2001
 *
 *   Requires: big.cpp ecn.cpp
 *
 */

#include <iostream>
#include <fstream>
#include "ecn.h"
#include "ebrick.h"   /* include MIRACL system */

using namespace std;

Miracl precision=50;

int main()
{
    ifstream common("common.ecs");
    Big a,b,x,y,e,n,r;
    int bits,nb,w;
    miracl *mip=&precision;
    common >> bits; 
    mip->IOBASE=16;
    common >> n >> a >> b >> r >> x >> y;
    mip->IOBASE=10;

    w=8; // window size

    cout << "Enter size of exponent in bits = ";
    cin >> nb;

    EBrick B(x,y,a,b,n,w,nb); 

    e=rand(nb,2); /* random exponent */

    cout << "naive method" << endl;
    ecurve(a,b,n,MR_PROJECTIVE);
    ECn G(x,y);
    G*=e;
    G.get(x,y);
    cout << x << endl;

    x=0;
    cout << "Comb method" << endl;

    B.mul(e,x,y);

    cout << x << endl;
    return 0;
}

