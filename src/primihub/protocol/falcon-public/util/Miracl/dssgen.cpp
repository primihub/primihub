/*
 *   Digital Signature Algorithm (DSA)
 *
 *   See Communications ACM July 1992, Vol. 35 No. 7
 *   This new standard for digital signatures has been proposed by 
 *   the American National Institute of Standards and Technology (NIST)
 *   under advisement from the National Security Agency (NSA). 
 *
 *   This program generates one set of public and private keys in files 
 *   public.dss and private.dss respectively
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
    ifstream common("common.dss");    /* construct file I/O streams */
    ofstream public_key("public.dss");
    ofstream private_key("private.dss");
    Big p,q,h,g,x,y,n,s,t;
    long seed;
    int bits;
    miracl *mip=&precision;

    cout << "Enter 9 digit random number seed  = ";
    cin >> seed;
    irand(seed);

/* get common data */
    common >> bits;
    mip->IOBASE=16;
    common >> p >> q >> g;
    mip->IOBASE=10;

    y=pow(g,q,p);
    if (y!=1)
    {
        cout << "Problem - generator g is not of order q" << endl;
        exit(0);
    } 

/* generate public/private keys */
    x=rand(q);
    y=pow(g,x,p);
    cout << "public key = " << y << endl;
    public_key << y << endl;
    private_key << x << endl;
    return 0;
}

