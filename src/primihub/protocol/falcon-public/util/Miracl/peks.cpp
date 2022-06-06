/*
   Boneh, Di Crescenzo, Ostrovsky & Persiano
   Public Key Encryption with keyword Search

   See http://eprint.iacr.org/2003/195.pdf
   Section 3.1

   (From reading the protocol to a working implementation - 10 minutes!)

   Compile with modules as specified below

   For MR_PAIRING_CP curve
   cl /O2 /GX peks.cpp cp_pair.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   For MR_PAIRING_MNT curve
   cl /O2 /GX peks.cpp mnt_pair.cpp zzn6a.cpp ecn3.cpp zzn3.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib
	
   For MR_PAIRING_BN curve
   cl /O2 /GX peks.cpp bn_pair.cpp zzn12a.cpp ecn2.cpp zzn4.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   For MR_PAIRING_KSS curve
   cl /O2 /GX peks.cpp kss_pair.cpp zzn18.cpp zzn6.cpp ecn3.cpp zzn3.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   For MR_PAIRING_BLS curve
   cl /O2 /GX peks.cpp bls_pair.cpp zzn24.cpp zzn8.cpp zzn4.cpp zzn2.cpp ecn4.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   Test program 
*/

#include <iostream>
#include <ctime>

//********* choose just one of these pairs **********
//#define MR_PAIRING_CP      // AES-80 security   
//#define AES_SECURITY 80

//#define MR_PAIRING_MNT	// AES-80 security
//#define AES_SECURITY 80

#define MR_PAIRING_BN    // AES-128 or AES-192 security
#define AES_SECURITY 128
//#define AES_SECURITY 192

//#define MR_PAIRING_KSS    // AES-192 security
//#define AES_SECURITY 192

//#define MR_PAIRING_BLS    // AES-256 security
//#define AES_SECURITY 256
//*********************************************

#include "pairing_3.h"

int main()
{   
	PFC pfc(AES_SECURITY);  // initialise pairing-friendly curve

	time_t seed;
	time(&seed);
    irand((long)seed);

	G1 g,h,PA;
	Big alpha,r,PB;
	GT t;
	G2 HW,TW;

// keygen

	pfc.random(g);
	pfc.precomp_for_mult(g);  // precompute on fixed g
	pfc.random(alpha);		  // private key
	h=pfc.mult(g,alpha);      // public key
	pfc.precomp_for_mult(h);

// PEKS
	pfc.random(r);
	pfc.hash_and_map(HW,(char *)"test");
	t=pfc.pairing(HW,pfc.mult(h,r));
	PA=pfc.mult(g,r);
	PB=pfc.hash_to_aes_key(t);    // [PA,PB] added to ciphertext

// Trapdoor

	pfc.hash_and_map(HW,(char *)"test"); // key word we are looking for
	TW=pfc.mult(HW,alpha);
	pfc.precomp_for_pairing(TW);

// Test
	if (pfc.hash_to_aes_key(pfc.pairing(TW,PA))==PB)
		 cout << "yes" << endl;
	else cout << "no" << endl;

    return 0;
}
