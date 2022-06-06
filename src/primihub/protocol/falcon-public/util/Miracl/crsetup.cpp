/*
 *   Cramer-Shoup Public Key Cryptography
 *
 *   This program generates the common values q, p, g1 and g2 to a file
 *   common.crs
 *
 *   Requires: big.cpp
 */

#include <iostream>
#include <fstream>
#include "big.h"

using namespace std;

#define PBITS 1024 
#define QBITS 160

Miracl precision=100;

int main()
{
    ofstream common("common.crs");    // construct file I/O streams
    Big p,q,h,t,s,r,n,g;
    long seed;
    cout << "Enter 9 digit random number seed  = ";
    cin >> seed;
    irand(seed);

    get_mip()->IOBASE=16;

/* generate q */
    forever
    {
        n=rand(QBITS-2,2);  /* QBIT-2 bit number, base 2 */
        q=2*n+1;
        while (!prime(q)) q+=2;
        if (bits(q)>=QBITS) continue;
        break;
    }
    cout << "q= " << q << endl;
    common << PBITS << endl;
    common << q << endl;

/* generate p */
    t=(pow((Big)2,PBITS)-1)/(2*q);
    s=(pow((Big)2,PBITS-1)-1)/(2*q);
    forever 
    {
        n=rand(t);
        if (n<s) continue;
        p=2*n*q+1;
        if (prime(p)) break;
    } 
    cout << "p= " << p << endl;
    common << p << endl;

/* generate g1 */
    do {
        h=rand(p-1);
        g=pow(h,(p-1)/q,p);
    } while (g==1);    
    cout << "g1= " << g << endl;
    common << g << endl;

/* generate g2 */
    do {
        h=rand(p-1);
        g=pow(h,(p-1)/q,p);
    } while (g==1);    
    cout << "g2= " << g << endl;
    common << g << endl;

    return 0;
}

