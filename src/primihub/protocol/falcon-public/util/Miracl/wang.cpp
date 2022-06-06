/*
	Wang interactive ID based key exchange
	uses type 1 pairing
	See http://eprint.iacr.org/2005/108

	Compile with modules as specified below
	
	For MR_PAIRING_SS2 curves
	cl /O2 /GX wang.cpp ss2_pair.cpp ec2.cpp gf2m4x.cpp gf2m.cpp big.cpp miracl.lib
  
	For MR_PAIRING_SSP curves
	cl /O2 /GX wang.cpp ssp_pair.cpp ecn.cpp zzn2.cpp zzn.cpp big.cpp miracl.lib

	Very Simple Test program 
*/

#include <iostream>
#include <ctime>

//********* choose just one of these **********
///#define MR_PAIRING_SS2    // AES-80 or AES-128 security GF(2^m) curve
//#define AES_SECURITY 80   // OR
//#define AES_SECURITY 128

#define MR_PAIRING_SSP    // AES-80 or AES-128 security GF(p) curve
#define AES_SECURITY 80   // OR
//#define AES_SECURITY 128
//*********************************************

#include "pairing_1.h"

// Here we ignore the mysterious h co-factor which appears in the paper..

int main()
{   
	PFC pfc(AES_SECURITY);  // initialise pairing-friendly curve

	Big alpha,x,y,key,sA,sB;
	G1 gIDA,gIDB,dIDA,dIDB,RA,RB;
	GT K;
	time_t seed;

	time(&seed);
    irand((long)seed);

// setup
	pfc.random(alpha);

// extract private key for Alice
	pfc.hash_and_map(gIDA,(char *)"Alice");
	dIDA=pfc.mult(gIDA,alpha);
	pfc.precomp_for_mult(dIDA);

// extract private key for Bob
	pfc.hash_and_map(gIDB,(char *)"Robert");
	dIDB=pfc.mult(gIDB,alpha);
	pfc.precomp_for_mult(dIDB);

// Alice to Bob

	pfc.random(x);
	RA=pfc.mult(gIDA,x);
	
// Bob to Alice

	pfc.random(y);
	RB=pfc.mult(gIDB,y);

// Hash values

	pfc.start_hash();
	pfc.add_to_hash(RA);
	pfc.add_to_hash(RB);
	sA=pfc.finish_hash_to_group();

	pfc.start_hash();
	pfc.add_to_hash(RB);
	pfc.add_to_hash(RA);
	sB=pfc.finish_hash_to_group();

// Alice calculates mutual key
	K=pfc.pairing(pfc.mult(gIDB,sB)+RB,pfc.mult(dIDA,x+sA));
	key=pfc.hash_to_aes_key(K);
	cout << "Alice's key= " << key << endl;

// Bob calculates mutual key
	K=pfc.pairing(pfc.mult(gIDA,sA)+RA,pfc.mult(dIDB,y+sB));
	key=pfc.hash_to_aes_key(K);
	cout << "Bob's key=   " << key << endl;

    return 0;
}
