/*
   Barreto & McCullagh Signcryption
   See http://eprint.iacr.org/2004/117.pdf
   Section 5.2

   Compile with modules as specified below

   For MR_PAIRING_CP curve
   cl /O2 /GX bmc.cpp cp_pair.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   For MR_PAIRING_MNT curve
   cl /O2 /GX bmc.cpp mnt_pair.cpp zzn6a.cpp ecn3.cpp zzn3.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib
	
   For MR_PAIRING_BN curve
   cl /O2 /GX bmc.cpp bn_pair.cpp zzn12a.cpp ecn2.cpp zzn4.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   For MR_PAIRING_KSS curve
   cl /O2 /GX bmc.cpp kss_pair.cpp zzn18.cpp zzn6.cpp ecn3.cpp zzn3.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   For MR_PAIRING_BLS curve
   cl /O2 /GX bmc.cpp bls_pair.cpp zzn24.cpp zzn8.cpp zzn4.cpp zzn2.cpp ecn4.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

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
	miracl* mip=get_mip();
	Big order=pfc.order();

	Big s,u,a,b,x,c,h,M;
	G1 P,Ppub,Pa,Pb,R,S,U;
	G2 Q,Qsa,Qsb,T;
	GT g,N,V;
	time_t seed;

	time(&seed);
    irand((long)seed);

//setup
	pfc.random(s);
	pfc.random(P);
	pfc.random(Q);

	g=pfc.pairing(Q,P);
	pfc.precomp_for_power(g);
	Ppub=pfc.mult(P,s);

//Keygen
	a=pfc.hash_to_group((char *)"Alice");
	Qsa=pfc.mult(Q,inverse(modmult(s,a,order),order));
	b=pfc.hash_to_group((char *)"Bob");
	Qsb=pfc.mult(Q,inverse(modmult(s,b,order),order));
	Pa=pfc.mult(Ppub,a);
	Pb=pfc.mult(Ppub,b);

//Signcrypt
	mip->IOBASE=256;
	M=(char *)"test message"; // to be signcrypted from Alice to Bob
	cout << "Signed Message=   " << M << endl;
	mip->IOBASE=16;
	
	pfc.precomp_for_mult(Pa);
	pfc.precomp_for_mult(Qsa);
	pfc.random(x);
	N=pfc.power(g,inverse(x,order));
	R=pfc.mult(Pa,x);
	S=pfc.mult(Pb,inverse(x,order));
	c=lxor(M,pfc.hash_to_aes_key(N));
	pfc.start_hash();
	pfc.add_to_hash(R);
	pfc.add_to_hash(S);
	pfc.add_to_hash(c);
	h=pfc.finish_hash_to_group();
	T=pfc.mult(Qsa,inverse(x+h,order));

//  Unsigncrypt
	pfc.precomp_for_pairing(Qsb);   // Bob can precompute on his private key
	pfc.start_hash();
	pfc.add_to_hash(R);
	pfc.add_to_hash(S);
	pfc.add_to_hash(c);
	h=pfc.finish_hash_to_group();

	U=pfc.mult(Pa,h);
	V=pfc.pairing(T,R+U);
	N=pfc.pairing(Qsb,S);
	M=lxor(c,pfc.hash_to_aes_key(N));
	mip->IOBASE=256;
	if (V==g)
	{
		cout << "Message is OK" << endl;
		cout << "Verified Message= " << M << endl;
	}
	else
		cout << "Message is bad    " << M << endl;

	return 0;
}
