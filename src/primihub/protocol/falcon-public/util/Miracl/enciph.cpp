/*
 *   Program to encipher text using Blum-Goldwasser Probabalistic
 *   Public Key method
 *   See "Modern Cryptology - a tutorial" by Gilles Brassard. 
 *   Published by Springer-Verlag, 1988
 *
 *   Define RSA to use cubing instead of squaring. This is no longer
 *   provably as difficult to break as factoring the modulus, its a bit 
 *   slower, but it is resistant to chosen ciphertext attack.
 *
 *   Note: This implementation uses only the Least Significant Byte
 *   of the big random number x in encipherment/decipherment, as it has 
 *   proven to be completely secure. However it is conjectured that
 *   that up to half the bytes in x (the lower half) are also secure.
 *   They could be used to considerably speed up this method.
 *   
 *   Requires: big.cpp
 */

#include <iostream>
#include <fstream>
#include "big.h"   /* include MIRACL system */
#include <cstring>

using namespace std;

// #define RSA

Miracl precision=100;

void strip(char *name)
{ /* strip off filename extension */
    int i;
    for (i=0;name[i]!='\0';i++)
    {
        if (name[i]!='.') continue;
        name[i]='\0';
        break;
    }
}

int main()
{  /*  encipher using public key  */
    Big x,ke;
    ifstream public_key("public.key");
    ifstream input_file;
    ofstream output_file,key_file;
    char ifname[13],ofname[13];
    BOOL fli;
    char ch;
    long seed,ipt;
    miracl *mip=&precision;

    mip->IOBASE=16;
    public_key >> ke;
    cout << "Enter 9 digit random number seed  = ";
    cin >> seed;
    irand(seed);
    x=rand(ke);
    cout << "file to be enciphered = ";
    cin >> ifname;
//    cin.sync();
//    cin.getline(ifname,13);
    fli=FALSE;
    if (strlen(ifname)>0) fli=TRUE;
    if (fli)
    { /* set up input file */
        strcpy(ofname,ifname);
        strip(ofname);
        strcat(ofname,".blg");
        input_file.open(ifname,ios::in);
        if (!input_file)
        {
            cout << "Unable to open file " << ifname << "\n";;
            return 0;
        }
        cout << "enciphering message" << endl;
    }
    else
    { /* accept input from keyboard */
        cout << "output filename = ";
        cin >> ofname;
        strip(ofname);    
        strcat(ofname,".blg");
        cout << "input message - finish with cntrl z" << endl;
    }
    output_file.open(ofname,ios::binary|ios::out);
    ipt=0;
    forever
    { /* encipher character by character */
#ifdef RSA
        x=pow(x,3,ke);
#else
        x=(x*x)%ke;
#endif
        if (fli) 
        {
            if (input_file.eof()) break;
            input_file.get(ch);
        }
        else
        {
            if (cin.eof()) break;
            cin.get(ch);
        }
        ch^=x[0];              /* XOR with last byte of x */
        output_file << ch;
        ipt++;
    }
    strip(ofname);
    strcat(ofname,".key");
    key_file.open(ofname);
    key_file << ipt << "\n";
    key_file << x << endl;
    return 0;
}   

