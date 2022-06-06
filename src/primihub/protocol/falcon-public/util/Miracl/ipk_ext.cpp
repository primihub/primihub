/*
   Cocks ID-PKC Scheme

   Extract Phase

   After this program has run the file private.ipk contains

   <b=sqrt(H(ID) mod n>
   OR
   <b=sqrt(-H(ID) mod n>

   Requires : big.cpp crt.cpp

*/


#include <iostream>
#include <fstream>
#include "big.h"
#include "crt.h"

using namespace std;

Miracl precision(100,0);

// Using SHA-1 as basic hash algorithm

#define HASH_LEN 20

//
// Public Hash Function
//

Big H(char *string,Big n)
{ // Hash a zero-terminated string to a number h < modulus n
  // Then increment this number until  (h/n)=+1
    Big h;
    char s[HASH_LEN];
    int i,j; 
    sha sh;

    shs_init(&sh);

    for (i=0;;i++)
    {
        if (string[i]==0) break;
        shs_process(&sh,string[i]);
    }
    shs_hash(&sh,s);

    h=1; j=0; i=1;
    forever
    {
        h*=256; 
        if (j==HASH_LEN)  {h+=i++; j=0;}
        else         h+=s[j++];
        if (h>=n) break;
    }
    while (jacobi(h,n)!=1) h+=1;
    h%=n;
    return h;
}

int main()
{
    ifstream master_key("master.ipk");
    ofstream private_key("private.ipk");
    Big b,x,r[2],n,p[2];
    miracl *mip=&precision;
    int sign;

    mip->IOBASE=16;
    master_key >> p[0] >> p[1];
    n=p[0]*p[1];

// EXTRACT

    char id[1000];

    cout << "Enter your email address (lower case)" << endl;
    cin.getline(id,1000);

    x=H(id,n);
    sign=0;
    if (jacobi(x,p[0])==-1)
    { 
        x=n-x;  // Modify if its not QR
        sign=1;
    }

    Crt chinese(2,p);
    r[0]=sqrt(x,p[0]);
    if (jacobi(r[0],p[0])==-1) r[0]=p[0]-r[0];

    r[1]=sqrt(x,p[1]);
    if (jacobi(r[1],p[1])==-1) r[1]=p[1]-r[1];

    b=chinese.eval(r);

    private_key << b << endl;
    private_key << sign << endl;
    return 0;
}

