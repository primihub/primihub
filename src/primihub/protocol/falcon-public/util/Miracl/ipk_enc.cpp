/*
   Cock's Identity Based Encryption
  
   Encryption phase
  
   Generates a random AES session key, and uses it to encrypt a file.
   Outputs ciphertext <filename>.ipk

   The session key is ID_PKC encrypted, and written to <filename>.key

   Requires : big.cpp

 */

#include <iostream>
#include <fstream>
#include <cstring>
#include "big.h"

using namespace std;

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
    miracl *mip=mirsys(36,0);   // thread-safe ready.
    ifstream common("common.ipk");
    ifstream plaintext;
    ofstream key_file,ciphertext;
    char ifname[100],ofname[100],ch,iv[16],key[16];
    Big t,s,n,x;
    Big tp[128],tm[128],wp[128],wm[128];
    int i,j,bit;
    long seed;
    aes a;
    
    mip->IOBASE=16;
    common >> n;

    cout << "Enter 9 digit random number seed  = ";

    cin >> seed;
    irand(seed);

//
// Generate random numbers (off-line)
// and generate a random key
//

    ch=0;
    for (i=0;i<128;i++)
    {
        tp[i]=rand(n);            // random t
        bit=jacobi(tp[i],n);  

        ch<<=1;
        if (bit==1) ch|=1;
        do {
           tm[i]=rand(n);         // different t, same (t/n)
        } while (jacobi(tm[i],n)!=bit);        

        if (i%8==7) 
        { 
            key[i/8]=ch;
            ch=0; 
        }
    }
    multi_inverse(128,tp,n,wp);   // generate 1/t
    multi_inverse(128,tm,n,wm);

// ENCRYPT

    char id[1000];
    cout << "Enter your correspondents email address (lower case)" << endl;
    cin.get();
    cin.getline(id,1000);

    x=H(id,n);

// figure out where input is coming from

    cout << "Text file to be encrypted = " ;
    cin >> ifname;

   /* set up input file */
    strcpy(ofname,ifname);

//
// Now ID-PKC encrypt a random session key
//

    strip(ofname);
    strcat(ofname,".key");
    mip->IOBASE=16;
    key_file.open(ofname);

    for (i=0;i<128;i++)
    { // ID-PKC encrypt
        s=tp[i]+modmult(x,wp[i],n); if (s>=n) s-=n;
        key_file << s << endl;
        s=tm[i]-modmult(x,wm[i],n); if (s<0)  s+=n;
        key_file << s << endl;
    }   

//
// prepare to encrypt file with random session key
//
    for (i=0;i<16;i++) iv[i]=i; // set CFB IV
    aes_init(&a,MR_CFB1,16,key,iv);
    
   /* set up input file */
    strcpy(ofname,ifname);
    strip(ofname);
    strcat(ofname,".ipk");
    plaintext.open(ifname,ios::in); 
    if (!plaintext)
    {
        cout << "Unable to open file " << ifname << "\n";
        return 0;
    }
    cout << "encrypting message\n";
    ciphertext.open(ofname,ios::binary|ios::out);

// now encrypt the plaintext file

    forever
    { // encrypt input ..
        plaintext.get(ch);
        if (plaintext.eof()) break;
        aes_encrypt(&a,&ch);
        ciphertext << ch;
    }

    aes_end(&a);
    return 0;
}

