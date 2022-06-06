/*
 *   Digital Signature Standard
 *
 *   See Communications ACM July 1992, Vol. 35 No. 7
 *   This new standard for digital signatures has been proposed by 
 *   the American National Institute of Standards and Technology (NIST)
 *   under advisement from the National Security Agency (NSA). 
 *
 *   This program generates the common values p, q and g to a file
 *   common.dss
 *
 *   See "Applied Cryptography", by Bruce Schneier for a better way
 *   to generate trustworthy primes 
 *
 *   Requires: big.cpp
 *
 */

#include <iostream>
#include <fstream>
#include "big.h"

using namespace std;

#define QBITS 160
#define PBITS 1024 

Miracl precision=100;

int main()
{
    ofstream common("common.dss");    /* construct file I/O streams */
    Big p,q,h,g,x,y,n,s,t;
    long seed;
    miracl *mip=&precision;
    cout << "Enter 9 digit random number seed  = ";
    cin >> seed;
    irand(seed);

/* generate q */
    forever
    {
        n=rand(QBITS-1,2);  /* 159 bit number, base 2 */
        q=2*n+1;
        while (!prime(q)) q+=2;
        if (bits(q)>QBITS) continue;
        break;
    }
    cout << "q= " << q << endl;

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

/* generate g */
    do {
        h=rand(p-1);
        g=pow(h,(p-1)/q,p);
    } while (g==1);    
    cout << "g= " << g << endl;

    common << PBITS << endl;
    mip->IOBASE=16;
    common << p << endl;
    common << q << endl;
    common << g << endl;
    return 0;
}

