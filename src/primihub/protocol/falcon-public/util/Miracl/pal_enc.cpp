/*
 * PAL_ENC.CPP 
 *
 * Paillier Homomorphic Encryption
 * See "Public key Cryptosystems based on Composite Degree Residue Classes",
 * Eurocrypt 1999
 *
 * NOTE: - This PK scheme is "Homomorphic" - E(m1+m2) = E(m1)*E(m2)  -
 *         and probabilistic
 *
 * This program encrypts a short message to the file cipher.pal 
 *
 * Uses standard 2-prime RSA key pair as private and public keys
 * These are in public.key and private.key - or use genkey.c/cpp to
 * generate a new pair.
 *
 * Requires: big.cpp
 *
 */

#include <iostream>
#include <fstream>
#include <cstring>
#include "big.h"

using namespace std;

Miracl precision(100,0);

int main()
{
    Big c,g,r,m,d,n,n2;
    ifstream public_key("public.key");
    ofstream output_file("cipher.pal");
    miracl *mip=&precision;
    long seed;
    char t[100]; 
    mip->IOBASE=16;
    public_key >> n;
    n2=n*n;

    cout << "Enter 9 digit random number seed  = ";
    cin >> seed;
    irand(seed);

    r=pow(rand(n2),n,n2);                 // off-line

    cout << "enter your message" << endl;
    cin.get();
    cin.getline(t,100);
    m=from_binary(strlen(t),t);    // m<n

    g=1+m*n;
    c=modmult(g,r,n2);

    cout << "Ciphertext = \n" << c << endl;
    output_file << c << endl;

    return 0;
}

