/*
 *   Cramer-Shoup public Key system - Hybrid Implementation 
 *
 *   This program decrypts the contents of the file xxx.crs to the screen
 *   usng the AES key recovered from xxx.key using the Cramer-Shoup
 *   private key from private.crs
 *
 *   Requires: big.cpp
 */

#include <iostream>
#include <fstream>
#include "big.h"
#include <cstring>

using namespace std;

Miracl precision(300,256);

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

void hash(sha *s,Big x)
{ /* hash in a big number */
    int ch;
    while (x!=0)
    {
        ch=x%256; x/=256;
        shs_process(s,ch);
    }
}

int main()
{
    int i,bits;
    static char ifname[100];
    ifstream common("common.crs");    // construct file I/O streams 
    ifstream private_key("private.crs");
    ifstream key_file,ciphertext,ct;
    Big p,q,g1,g2,x1,x2,y1,y2,z,u1,u2,e,v,m,e1,e2,alpha;
    char ch,H[20],iv[16],block[16]; 
    miracl *mip=&precision;
    sha sh;
    aes a;

// get common data 
    mip->IOBASE=16;
    common >> bits;
    common >> q >> p >> g1 >> g2;

// get private key
    private_key >> x1 >> x2 >> y1 >> y2 >> z;

// figure out where input is coming from

    cout << "file to be decoded = ";
    cin >> ifname;

    strip(ifname);          // open key file
    strcat(ifname,".key");
    key_file.open(ifname,ios::in);
    if (!key_file)
    {
        cout << "Key file " << ifname << " not found\n";
        return 0;
    }

    strip(ifname);          // open ciphertext
    strcat(ifname,".crs");
    ciphertext.open(ifname,ios::binary|ios::in);
    if (!ciphertext)
    {
        cout << "Unable to open file " << ifname << "\n";
        return 0;
    }

    mip->IOBASE=16;
    key_file >> u1 >> u2 >> v;

    shs_init(&sh);
    hash(&sh,u1);
    hash(&sh,u2);

// first pass - scan in ciphertext, and calculate alpha=H(u1,u2,ciphertext)
    cout << "Checking ciphertext validity\n" ;
    forever
    {
        ciphertext.get(ch);
        if (ciphertext.eof()) break;
        shs_process(&sh,ch);
    }

    ciphertext.close();
    shs_hash(&sh,H);
    mip->INPLEN=20; 
    mip->IOBASE=256;
    alpha=(char *)H;      // alpha=H(u1,u2,ciphertext);

    e1=(x1+y1*alpha)%q;
    e2=(x2+y2*alpha)%q;

// check validity of ciphertext

    if (v!=pow(u1,e1,u2,e2,p))
    {
        cout << "*** Cipher text is rejected ***" << endl;
        return 0;
    }

    shs_init(&sh);
    hash(&sh,pow(u1,z,p));                          
    shs_hash(&sh,H);      // H=hash(u1^z mod p) = AES Key

    for (i=0;i<16;i++) iv[i]=i;  // Set CFB IV
    aes_init(&a,MR_PCFB1,16,H,iv);        // Set 128 bit AES Key

// decrypt file on second pass

    ct.open(ifname,ios::binary|ios::in);
    cout << "decoding message\n";
    forever
    { // decrypt input ..
        ct.get(ch);
        if (ct.eof()) break;
        aes_decrypt(&a,&ch);
        cout << ch;
    }
    aes_end(&a);
    ct.close();
    return 0;
}

