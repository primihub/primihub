/*
 *   Elliptic Curve Digital Signature Algorithm (ECDSA)
 *
 *
 *   This program generates one set of public and private keys in files 
 *   public.ecs and private.ecs respectively. Notice that the public key 
 *   can be much shorter in this scheme, for the same security level.
 *
 *   It is assumed that Curve parameters are to be found in file common.ecs
 *
 *   The curve is y^2=x^3+Ax+B mod p
 *
 *   The file common.ecs is presumed to exist, and to contain the domain
 *   information {p,A,B,q,x,y}, where A and B are curve parameters, (x,y) are
 *   a point of order q, p is the prime modulus, and q is the order of the 
 *   point (x,y). In fact normally q is the prime number of points counted
 *   on the curve. 
 *
 *   Requires: big.cpp ecn.cpp
 */

#include <iostream>
#include <fstream>
#include "ecn.h"

using namespace std;

// if MR_STATIC defined, it should be 20

#ifndef MR_NOFULLWIDTH
Miracl precision=20;
#else
Miracl precision(20,MAXBASE);
#endif

int main()
{
    ifstream common("common.ecs");    /* construct file I/O streams */
    ofstream public_key("public.ecs");
    ofstream private_key("private.ecs");
    int bits,ep;
    miracl *mip=&precision;

    ECn G,W;
    Big a,b,p,q,x,y,d;
    long seed;

    cout << "Enter 9 digit random number seed  = ";
    cin >> seed;
    irand(seed);

    common >> bits;
    mip->IOBASE=16;
    common >> p >> a >> b >> q >> x >> y;
    mip->IOBASE=10;

    ecurve(a,b,p,MR_PROJECTIVE);

    if (!G.set(x,y))
    {
        cout << "Problem - point (x,y) is not on the curve" << endl;
        return 0;
    }

    W=G;
    W*=q;

    if (!W.iszero())
    {
        cout << "Problem - point (x,y) is not of order q" << endl;
        return 0;
    }

/* generate public/private keys */

    d=rand(q);
 //   for (int i=0;i<=10000;i++)
    G*=d;
    ep=G.get(x);
    cout << "public key = " << ep << " " << x << endl;
    public_key << ep << " " << x << endl;
    private_key << d << endl;
    return 0;
}

