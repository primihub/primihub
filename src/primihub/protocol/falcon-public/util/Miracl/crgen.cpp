/*
 *   Cramer-Shoup Public Key System 
 *
 *   This program generates one set of public and private keys in files 
 *   public.crs and private.crs respectively
 *
 *   Requires: big.cpp
 */

#include <iostream>
#include <fstream>
#include "big.h"

using namespace std;

Miracl precision=100;

int main()
{
    ifstream common("common.crs");    // construct file I/O streams 
    ofstream public_key("public.crs");
    ofstream private_key("private.crs");
    Big p,q,h,g1,g2,x1,x2,y1,y2,z,c,d;
    int bits;
    long seed;
    cout << "Enter 9 digit random number seed  = ";
    cin >> seed;
    irand(seed);
    get_mip()->IOBASE=16;

// get common data 
    common >> bits;
    common >> q >> p >> g1 >> g2;
    
// generate public/private keys 
    x1=rand(q);
    x2=rand(q);
    y1=rand(q);
    y2=rand(q);
    z=rand(q);

    c=pow(g1,x1,g2,x2,p);  // c=g1^x1.g2^x2 mod p
    d=pow(g1,y1,g2,y2,p);
    h=pow(g1,z,p);

    private_key << x1 << endl;
    private_key << x2 << endl;
    private_key << y1 << endl;
    private_key << y2 << endl;
    private_key << z << endl;

    public_key << c << endl;
    public_key << d << endl;
    public_key << h << endl;

    return 0;
}

