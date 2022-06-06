/*
 *   Test program to implement Comb method for fast
 *   computation of x.G, on the elliptic curve E(F_2^m) for fixed G and E, 
 *   using precomputation. 
 *   This idea can be used to substantially speed up certain phases 
 *   of the Elliptic Curve Digital Signature Standard (DSS).
 *
 *   See "Handbook of Applied Cryptography", CRC Press, 2001
 *
 *   Requires: big.cpp ec2.cpp
 *
 */

#include <iostream>
#include <fstream>
#include "ec2.h"
#include "ebrick2.h"   /* include MIRACL system */

using namespace std;

Miracl precision=50;

int main()
{
    ifstream common("common2.ecs");
    int m,a,b,c,w;
    Big a2,a6,x,y,e,r;
    int nb;
    miracl *mip=&precision;

    common >> m;
    mip->IOBASE=16;
    common >> a2 >> a6 >> r >> x >> y;
    mip->IOBASE=10;
    common >> a >> b >> c;

    w=8; // window size

    cout << "Enter size of exponent in bits = ";
    cin >> nb;

    EBrick2 B(x,y,a2,a6,m,a,b,c,w,nb); 

    e=rand(nb,2); /* random exponent */

    cout << "naive method" << endl;
    ecurve2(m,a,b,c,a2,a6,FALSE,MR_PROJECTIVE);
    EC2 G(x,y);
    G*=e;
    G.get(x,y);
    cout << x << endl;
    
    x=0;
    cout << "Comb method" << endl;

    B.mul(e,x,y);

    cout << x << endl;
    return 0;
}

