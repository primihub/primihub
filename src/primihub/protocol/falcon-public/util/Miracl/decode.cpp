/*
 *   Program to decode message using RSA private key.
 *
 *   Requires: big.cpp crt.cpp
 */

#include <iostream>
#include <fstream>
#include <cstring>
#include "big.h"   /* include MIRACL system */
#include "crt.h"   /* chinese remainder thereom */

using namespace std;

#define NP 2       /* two primes - could be used with more */

Miracl precision=100;

void strip(char *name)
{ /* strip extension off filename */
    int i;
    for (i=0;name[i]!='\0';i++)
    {
        if (name[i]!='.') continue;
        name[i]='\0';
        break;
    }
}

int main()
{  /*  decode using private key  */
    int i;
    Big e,ep[NP],m,ke,p[NP],kp[NP],mn,mx;
    ifstream private_key("private.key");
    ifstream input_file;
    ofstream output_file;
    char ifname[13],ofname[13];
    BOOL flo;
    miracl *mip=&precision;
    mip->IOBASE=16;
    for (i=0;i<NP;i++)
    {
        private_key >> p[i];
    }
 /* construct public and private keys */
    ke=1;
    for (i=0;i<NP;i++)
    { /* Calculate ke */
        ke*=p[i];
    }
    for (i=0;i<NP;i++)
    { /* prepare for Chinese remainder thereom *
       * Find 1/3 mod p[i]-1  Euler TF         */
        kp[i]=(2*(p[i]-1)+1)/3;
    }
    mn=root(ke,3);
    mx=mn*mn*mn-mn*mn;
    do
    { /* get input file */
        cout << "file to be decoded = ";
        cin >> ifname;
    } while (strlen(ifname)==0);
    strip(ifname);
    strcat(ifname,".rsa");
    cout << "output filename = ";
    cin.sync();
    cin.getline(ofname,13);
    flo=FALSE;
    if (strlen(ofname)>0) 
    { /* set up output file */
        flo=TRUE;
        output_file.open(ofname);
    }
    cout << "decoding message\n";
    input_file.open(ifname,ios::in);
    if (!input_file)
    {
        cout << "Unable to open file " << ifname << "\n";
        return 0;
    }
    Crt chinese(NP,p);   /* use chinese remainder thereom */
    forever
    { /* decode line by line */
        mip->IOBASE=16;
        input_file >> m;
        if (m==0) break;
        for (i=0;i<NP;i++) 
            ep[i]=pow(m,kp[i],p[i]);
        e=chinese.eval(ep);
        if (e>=mx) e%=mn;
        mip->IOBASE=128;
        if (flo) output_file << e;
        cout << e << flush;
    }
    cout << "message ends\n";
    return 0;
}

