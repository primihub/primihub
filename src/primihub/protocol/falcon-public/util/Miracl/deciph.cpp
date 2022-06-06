/*
 *   Program to decipher message using Blum-Goldwasser probabalistic
 *   Public key method
 *
 *   Define RSA to use cubing instead of squaring. This is no longer
 *   provably as difficult to break as factoring the modulus, its a bit 
 *   slower, but it is resistant to chosen ciphertext attack.
 *
 *   Requires: big.cpp crt.cpp
 */

#include <iostream>
#include <fstream>
#include "big.h"   /* include MIRACL system */
#include "crt.h"
#include <cstring>

using namespace std;

// #define RSA

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
{  /*  decipher using private key  */
    Big x,t,ke,p[2],rem[2];
    ifstream private_key("private.key");
    ifstream input_file,key_file;
    ofstream output_file;
    int k;
    char ch;
    long i,ipt;
    char ifname[13],ofname[13];
    BOOL flo;
    miracl *mip=&precision;

    mip->IOBASE=16;
    private_key >> p[0] >> p[1];
    ke=p[0]*p[1];
    cout << "file to be deciphered = ";
    cin >> ifname;
    strip(ifname);
    strcat(ifname,".key");
    key_file.open(ifname,ios::in);
    if (!key_file)
    {
        cout << "Unable to open file " << ifname << "\n";
        return 0;
    }
    key_file >> ipt >> x;
    strip(ifname);
    strcat(ifname,".blg");
    cout << "output filename = ";
    cin.sync();
    cin.getline(ofname,13);
    flo=FALSE;
    if (strlen(ofname)>0) 
    { /* set up output file */
        flo=TRUE;
        output_file.open(ofname);
    }
    cout << "deciphering message" << endl;

    Crt chinese(2,p);
    
/* recover "one-time pad" */
   
    for (k=0;k<2;k++)
    {
#ifdef RSA
        t=pow((2*(p[k]-1)+1)/3,(Big)ipt,p[k]-1);
#else
        t=pow((p[k]+1)/4,(Big)ipt,p[k]-1);
#endif
        rem[k]=pow(x%p[k],t,p[k]);
    }

    x=chinese.eval(rem);       /* using Chinese remainder thereom */

    input_file.open(ifname,ios::binary|ios::in);
    if (!input_file)
    {
        cout << "Unable to open file " << ifname << "\n";
        return 0;
    }  
    for (i=0;i<ipt;i++)
    { /* decipher character by character */
        input_file.get(ch);
        ch^=x[0];               /* XOR with last byte of x */
        if (flo) output_file << ch << flush;
        else     cout << ch << flush;
#ifdef RSA
        x=pow(x,3,ke);
#else
        x=(x*x)%ke;
#endif
    }
    cout << "\nmessage ends\n";
    return 0;
}

