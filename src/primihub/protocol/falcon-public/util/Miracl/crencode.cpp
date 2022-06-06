/*
 *   Cramer-Shoup Public Key Encryption - Hybrid Implementation 
 *
 *   This program encrypts a file fred.xxx to fred.crs. The AES encryption
 *   key is encrypted under the public key in the file fred.key
 *
 *   The public key comes from from public.crs
 *
 *   Requires: big.cpp
 *
 */

#include <iostream>
#include <fstream>
#include <cstring>
#include "big.h"

using namespace std;

Miracl precision(300,256);

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

void hash(sha *s,Big x)
{ /* hash in a big number one byte at a time */
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
    ifstream common("common.crs");    // construct file I/O streams 
    ifstream plaintext,public_key("public.crs");
    ofstream key_file,ciphertext;
    Big p,q,g1,g2,h,m,r,u1,u2,e,v,ex,c,d,alpha;
    static char ifname[100],ofname[100];
    char ch,H[20],iv[16],block[16]; 
    sha sh;
    aes a;
    long seed;
    miracl *mip=&precision;

// randomise 
    cout << "Enter 9 digit random number seed  = ";
    cin >> seed;
    irand(seed);
    mip->IOBASE=16;

// get common data 
    common >> bits;
    common >> q >> p >> g1 >> g2;

// get public key of receiver 
    public_key >> c >> d >> h;

    r=rand(q);

    shs_init(&sh);
    hash(&sh,pow(h,r,p));                          
    shs_hash(&sh,H);      // AES Key = Hash(h^r mod p)
    for (i=0;i<16;i++) iv[i]=i;  // Set CFB IV
    aes_init(&a,MR_PCFB1,16,H,iv);        // Set 128 bit AES Key

// figure out where input is coming from

    cout << "file to be encoded = " ;
    cin >> ifname;

   /* set up output files */
    strcpy(ofname,ifname);
    strip(ofname);
    strcat(ofname,".crs");
    plaintext.open(ifname,ios::in); 
    if (!plaintext)
    {
        cout << "Unable to open file " << ifname << "\n";
        return 0;
    }
    cout << "encoding message\n";

    ciphertext.open(ofname,ios::binary|ios::out);

    u1=pow(g1,r,p);
    u2=pow(g2,r,p);

    shs_init(&sh);
    hash(&sh,u1);
    hash(&sh,u2);

// now encrypt the plaintext 
// and hash in the ciphertext.

    forever
    { // encrypt input ..
        plaintext.get(ch);
        if (plaintext.eof()) break;
        aes_encrypt(&a,&ch);
        shs_process(&sh,ch);
        ciphertext << ch;
    }

    aes_end(&a);

    shs_hash(&sh,H);
    mip->INPLEN=20; 
    mip->IOBASE=256;
    alpha=(char *)H;   // alpha=Hash(u1,u2,ciphertext)

    ex=(r*alpha)%q;
    v=pow(c,r,d,ex,p);      // multi-exponentiation

    strip(ofname);
    strcat(ofname,".key");
    mip->IOBASE=16;
    key_file.open(ofname);
    key_file << u1 << endl;
    key_file << u2 << endl;
    key_file << v << endl;
    return 0;
}

