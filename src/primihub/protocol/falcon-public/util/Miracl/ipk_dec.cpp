/*
   Cock's Identity Based Encryption

   Decryption phase

   Decrypts a file <filename>.ipk
   Finds the session key by ID-PKC decrypting the file <filename>.key
   Uses this session key to AES decrypt the file.

   Outputs to the screen

   Requires : big.cpp

 */

#include <iostream>
#include <fstream>
#include <cstring>
#include "big.h"

using namespace std;

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
{
    miracl *mip=mirsys(36,0);    // thread-safe ready. 
    ifstream common("common.ipk");
    ifstream private_key("private.ipk");
    ifstream key_file,ciphertext;
    char ifname[100],ch,iv[16],key[16];
    Big s,b,r,x,n;
    int i,j,bit;
    int sign;
    aes a;

// DECRYPT

    mip->IOBASE=16;
    common >> n;
    private_key >> b;
    private_key >> sign;
    b+=b;                  // 2*b

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

    for (i=0;i<16;i++)
    {
        ch=0;
        for (j=0;j<8;j++)
        {
            if (sign==0)
            {
                key_file >> s;
                s+=b; if (s>=n) s-=n;
                bit=jacobi(s,n);
                key_file >> s;
            }
            else
            {    
                key_file >> s;
                key_file >> s;
                s-=b; if (s<0) s+=n;
                bit=jacobi(s,n);
            }
            ch<<=1;
            if (bit==1) ch|=1;
        }
        key[i]=ch;
    }

    strip(ifname);          // open ciphertext
    strcat(ifname,".ipk");
    ciphertext.open(ifname,ios::binary|ios::in);
    if (!ciphertext)
    {
        cout << "Unable to open file " << ifname << "\n";
        return 0;
    }

    cout << "Decrypting message" << endl;    
    
//
// Use session key to decrypt file
//

    for (i=0;i<16;i++) iv[i]=i;  // Set CFB IV
    aes_init(&a,MR_CFB1,16,key,iv);

    forever
    { // decrypt input ..
        ciphertext.get(ch);
        if (ciphertext.eof()) break;
        aes_decrypt(&a,&ch);
        cout << ch;
    }
    aes_end(&a);

    return 0;
}


