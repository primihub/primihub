/*
 *   Weil's theorem
 *
 *   Reads in details of an elliptic curve
 *   The curve is y^2=x^3+A.x+B mod p
 *   and finds the number of points on curve over the extension field p^k
 *
 *   An input file supplies the domain information {p,A,B,q}, 
 *   where A and B are curve parameters, This file can be created by
 *   the cm, schoof or sea applications. Here p is the prime modulus, and q 
 *   is the number of points on the curve. 
 *
 *   Invoke as weil <file> <k>, for example 
 *   prompt:>weil common.ecs 8
 *
 *   This program also calculates the number of points on the twisted 
 *   extension curve. If this is prime, then this will be an ideal setting
 *   for elliptic curve cryptography over the extension field p^k using
 *   the curve y^2=x^3+A.(d^2)x+B.(d^3) where d is any quadratic non residue
 *   in F(p^k)
 *
 *   Inspired by "Fast Generation of Elliptic Curves with prime order over
 *   F_p^{2c}", Nogami & Morikawa
 *
 *   To get the results of their table 7 ...
 *
 *   prompt:>schoof -f 2#24-3 3 10 -o example.ecs
 *   prompt:>weil example.ecs 8
 *
 *   It is easy to create a simple batch file to loop around these programs
 *   until a good curve is found.
 *
 *   Requires: big.cpp ecn.cpp
 */

#include <iostream>
#include <fstream>
#include "big.h"
#include "ecn.h"

using namespace std;

#ifndef MR_NOFULLWIDTH
Miracl precision=100;
#else
Miracl precision(100,MAXBASE);
#endif

int main(int argc,char **argv)
{
    ifstream curve;    /* construct file I/O streams */
    int k,bts;
    Big p,a,b,q;
    miracl *mip=&precision;
    BOOL gotI=FALSE;
    Big t[50],trace,np,npt;

    argv++; argc--; 
    if (argc!=2)
    {
        cout << "Wrong number of parameters" << endl;
        return 1;
    }


    curve.open(argv[0],ios::in);
    if (!curve)
    {
        cout << "Input file " << argv[0] << " could not be opened" << endl;
        return 1;
    }

    k=atoi(argv[1]);

    curve >> bts;
    mip->IOBASE=16;
    curve >> p >> a >> b >> q;
 //   mip->IOBASE=10;

    ecurve(a,b,p,MR_PROJECTIVE);

    cout << "base curve is y^2=x^3";
    if (a<0) cout << a << "x";
    if (a>0) cout << "+" << a << "x";

    if (b<0) cout << b;
    if (b>0) cout << "+" << b;
    cout << " mod " << p << endl;

    if (k<2 || k>32)
    {
        cout << "Bad extension degree specified" << endl;
        return 0;
    }
    trace=p+1-q;
    t[0]=2;
    t[1]=trace;
    for (int i=1;i<k;i++)
        t[i+1]=trace*t[i] - p*t[i-1];

    np= pow(p,k)+1-t[k];
    npt=pow(p,k)+1+t[k];

    cout << "Number of points on the extension curve #E(F_{p^" << k << "}) = " << np << endl;
    cout << "Number of points on the twisted curve  #E'(F_{p^" << k << "}) = " << npt << endl;
    cout << "which is " << bits(npt) << " bits long" << endl;


 //   cout << "q= " << q << endl;
 //   cout << "np%q= " << npt%q << endl;
 //   cout << "np/q= " << npt/q << endl;

    if (prime(npt))
    { 
        cout << "..and is a prime!" << endl;
        return 3;
    }
    return 2;
}

