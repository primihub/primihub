/*
   Cocks ID-PKC scheme

   Program to generate ID-PKC global and master keys
 
   Set-up phase
 
   After this program has run the file common.ipk contains
 
   <Global Modulus n>
 
   The file master.ibe contains
 
   <Secret p>
   <Secret q>

   Requires : big.cpp
 
*/

#include <iostream>
#include <fstream>
#include <cstring>
#include "big.h"   /* include MIRACL system */

using namespace std;

#define PBITS 512

Miracl precision(100,0);

static Big pd,pl,ph;

long randise()
{ /* get a random number */
    long seed;
    cout << "Enter 9 digit random number seed  = ";
    cin >> seed;
    return seed;
}

Big strongp(int n,long seed1,long seed2)
{ /* generate strong prime number = 11 mod 12  */
    Big p;
    int r,r1,r2;
    irand(seed1);
    pd=rand(2*n/3,2);
    pd=nextprime(pd);
    ph=pow((Big)2,n-1)/pd;  
    pl=pow((Big)2,n-2)/pd;
    ph-=pl;
    irand(seed2);
    ph=rand(ph);
    ph+=pl;
    r1=pd%12;
    r2=ph%12;
    r=0;
    while ((r1*(r2+r))%12!=5) r++;
    ph+=r;
    do 
    { /* find p=2*r*pd+1 = 11 mod 12 */
        p=2*ph*pd+1;
        ph+=12;
    } while (!prime(p));
    return p;
}

int main()
{  /*  calculate common and master keys   *
    *  for ID-PKC scheme encryption       */
    int k,i;
    long seed[4];
    Big p[2],n;
    ofstream common("common.ipk");
    ofstream master("master.ipk");
    miracl *mip=&precision;

    gprime(15000);  /* speeds up large prime generation */

    for (i=0;i<4;i++)
        seed[i]=randise();

    cout << "generating keys - please wait\n";

    p[0]=strongp(PBITS,seed[0],seed[1]);
    p[1]=strongp(PBITS,seed[2],seed[3]);
    n=p[0]*p[1];

    cout << "Global modulus\n";
    cout << n << endl;
    cout << "Master key\n";
    cout << p[0] << endl;
    cout << p[1] << endl;
    mip->IOBASE=16;
    common << n << endl;
    master << p[0] << endl;
    master << p[1] << endl;
    return 0;
}

