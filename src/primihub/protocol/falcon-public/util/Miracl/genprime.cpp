/*
 *   Program to generate "trap-door primes".
 *   Generates primes for use with "index" program
 *
 *   Hard to find factors of the product of two of them n=p.q
 *   - but easy to find discrete logs wrt p & q
 *   See "Non-Interactive Public-Key Cryptography", Maurer & Yacobi
 *   Eurocrypt '91
 *
 *   Requires: big.cpp crt.cpp
 *
 */

#include <iostream>
#include <fstream>
#include <cstring>
#include "big.h"   /* include MIRACL system */
#define NPRIMES 5  /* =9 for > 256 bit primes */
#define PROOT 2

using namespace std;

Miracl precision=100;

int main()
{ /* program to find a trap-door prime */
    BOOL found;
    int i,spins;
    long seed;
    Big pp[NPRIMES],q,p,t;
    ofstream prime_data("prime.dat");
    cout << "Enter 9 digit seed= ";
    cin >> seed;
    irand(seed);
    cout << "Enter 4 digit seed= ";
    cin >> spins;
    for (i=0;i<spins;i++) brand();
    pp[0]=2;
    do
    {  /* find prime p = 2.pp[1].pp[2]....+1 */
        p=2;
        for (i=1;i<NPRIMES-1;i++)
        { /* generate all but last prime */
            q=rand(i+6,10);
            pp[i]=nextprime(q);
            p*=pp[i];
        }
        do
        { /* find last prime component such that p is prime */
            q=nextprime(q);
            pp[NPRIMES-1]=q;
            t=p*pp[NPRIMES-1];
            t+=1;
        } while(!prime(t));
        p=t;
        found=TRUE;
        for (i=0;i<NPRIMES;i++)
        { /* check that PROOT is a primitive root */
            if (pow(PROOT,(p-1)/pp[i],p)==1) 
            {
                found=FALSE;
                break;
            }
        }
    } while (!found);
    prime_data << NPRIMES << "\n";
    for (i=0;i<NPRIMES;i++) prime_data << pp[i] << endl;
    cout << "prime= \n" << p;
    return 0;
}

