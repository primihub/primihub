/*
	Sakai & Kasahara IBE key establishment
	using type 1 pairing. See P1363.3

	Compile with modules as specified below

	For MR_PAIRING_SS2 curves
	cl /O2 /GX sk_1.cpp ss2_pair.cpp ec2.cpp gf2m4x.cpp gf2m.cpp big.cpp miracl.lib

	For MR_PAIRING_SSP curves
	cl /O2 /GX sk_1.cpp ssp_pair.cpp ecn.cpp zzn2.cpp zzn.cpp big.cpp miracl.lib

	Very Simple Test program 
*/

#include <iostream>
#include <ctime>

//********* CHOOSE JUST ONE OF THESE **********
#define MR_PAIRING_SS2    // AES-80 or AES-128 security GF(2^m) curve
//#define AES_SECURITY 80   // OR
#define AES_SECURITY 128

//#define MR_PAIRING_SSP    // AES-80 or AES-128 security GF(p) curve
//#define AES_SECURITY 80   // OR
//#define AES_SECURITY 128
//*********************************************

#include "pairing_1.h"

int main()
{   
	PFC pfc(AES_SECURITY);  // initialise pairing-friendly curve
	Big q=pfc.order();

	Big z,b,SSV,r,H,t;
	G1 P,Z,KB,R;
	GT g;
	time_t seed;

	time(&seed);
    irand((long)seed);

// setup
	pfc.random(P);
	g=pfc.pairing(P,P);
	pfc.precomp_for_power(g);
	pfc.random(z);
	pfc.precomp_for_mult(P);
	Z=pfc.mult(P,z);

// extract private key for Robert
	b=pfc.hash_to_group((char *)"Robert");
	KB=pfc.mult(P,inverse(b+z,q));

// verify private key

	pfc.precomp_for_pairing(KB);  // Bob can precompute on his own private key
	if (pfc.pairing(KB,pfc.mult(P,b)+Z)!=g)
	{
		cout << "Bad private key" << endl;
		exit(0);
	}

	char *bytes;
	int len=pfc.spill(KB,bytes);    // demo - spill precomputation to byte array

// Send session key to Bob
	cout << "All set to go.." << endl;
	pfc.rankey(SSV);  // random AES key
	pfc.start_hash();
	pfc.add_to_hash(SSV);
	pfc.add_to_hash(b);
	r=pfc.finish_hash_to_group();

	R=pfc.mult(pfc.mult(P,b)+Z,r);
	t=pfc.hash_to_aes_key(pfc.power(g,r));
	H=lxor(SSV,t);
	cout << "Encryption key= " << SSV << endl;

// Receiver
	pfc.restore(bytes,KB);  // restore precomputation for KB from byte array

	t=pfc.hash_to_aes_key(pfc.pairing(KB,R));
	SSV=lxor(H,t);
	pfc.start_hash();
	pfc.add_to_hash(SSV);
	pfc.add_to_hash(b);
	r=pfc.finish_hash_to_group();

	if (pfc.mult(pfc.mult(P,b)+Z,r)==R)
		cout << "Decryption key= " << SSV << endl;
	else
		cout << "The key is BAD - do not use!" << endl;

    return 0;
}
