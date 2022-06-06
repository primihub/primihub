/* 
 * PAL_DEC.CPP
 *
 * Paillier Homomorphic Decryption
 * See "Public key Cryptosystems based on Composite Degree Residue Classes",
 * Eurocrypt 1999
 *
 * NOTE: This PK scheme is "Homomorphic" - E(m1+m2) = E(m1)*E(m2) -
 *       and probabilistic
 *
 * This program decrypts a short message from the file cipher.pal
 *
 * Uses standard 2-prime RSA key pair as private and public keys
 * These are in public.key and private.key - or use genkey.c/cpp to
 * generate a new pair.
 *
 * Requires: big.cpp crt.cpp
 */

#include <iostream>
#include <fstream>
#include <cstring>
#include "big.h"
#include "crt.h"

using namespace std;

Miracl precision(100,0);

Big CRT(Big *p,Big *r)
{ // Chinese remainder theorem with 2 co-prime moduli
    Crt chinese(2,p);
    return chinese.eval(r);
}

int main()
{
    Big lcm,c,m,d,n,n2,p,q,p2,q2,er[2],em[2];
    ifstream private_key("private.key");
    ifstream ciphertext("cipher.pal");
    miracl *mip=&precision;
    int len;
    char t[100]; 
    mip->IOBASE=16;
    private_key >> p >> q;
    n=p*q;
    n2=n*n;
    p2=p*p;
    q2=q*q;
    lcm=((p-1)*(q-1))/gcd(p-1,q-1); // LCM(p-1,q-1)

    em[0]=lcm;
    em[1]=n;
    er[0]=0; 
    er[1]=1;
    d=CRT(em,er);

    ciphertext >> c;

    em[0]=p2;
    em[1]=q2;
    er[0]=pow(c,d%(p2-p),p2);    // calculate mod p^2 and q^2 separately
    er[1]=pow(c,d%(q2-q),q2);    // .... and combine with CRT
    m=CRT(em,er);                      

    m=(m-1)/n;                   // m=[(c^d mod n^2)-1]/n

    len=to_binary(m,100,t,FALSE);
    t[len]=0;

    cout << "Message = \n" << t << endl;
    return 0;
}

