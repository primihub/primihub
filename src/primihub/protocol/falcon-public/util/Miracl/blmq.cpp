/*
   Barreto, Libert, McCullagh, Quisquater Signcryption
   http://grouper.ieee.org/groups/1363/IBC/submissions/Libert-IEEE-P1363-submission.pdf
   Section 4.2

   Uses Smart-Vercauteren idea for G2
   http://eprint.iacr.org/2005/116.pdf

   Compile with modules as specified below

   For MR_PAIRING_CP curve
   cl /O2 /GX blmq.cpp cp_pair.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   For MR_PAIRING_MNT curve
   cl /O2 /GX blmq.cpp mnt_pair.cpp zzn6a.cpp ecn3.cpp zzn3.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib
	
   For MR_PAIRING_BN curve
   cl /O2 /GX blmq.cpp bn_pair.cpp zzn12a.cpp ecn2.cpp zzn4.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   For MR_PAIRING_KSS curve
   cl /O2 /GX blmq.cpp kss_pair.cpp zzn18.cpp zzn6.cpp ecn3.cpp zzn3.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   For MR_PAIRING_BLS curve
   cl /O2 /GX blmq.cpp bls_pair.cpp zzn24.cpp zzn8.cpp zzn4.cpp zzn2.cpp ecn4.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

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
	Big order=pfc.order();
	miracl* mip=get_mip();

	Big s,x,a,b,h,c,M;
	G2 Q2,Q2pub,S2a,S2b;
	G1 Q1pub,P,S1a,S1b,S,T;
	GT g,r,rhs;
	time_t seed;

	time(&seed);
    irand((long)seed);

//setup - G2 = Q = {P,Q2)
	pfc.random(P);
	pfc.random(Q2);
	pfc.precomp_for_mult(P);
	pfc.precomp_for_mult(Q2);
	g=pfc.pairing(Q2,P);
	pfc.precomp_for_power(g);
	pfc.random(s);
	Q1pub=pfc.mult(P,s);
	Q2pub=pfc.mult(Q2,s);

//Keygen
	a=pfc.hash_to_group((char *)"Alice");
	S1a=pfc.mult(P,inverse(a+s,order));
	S2a=pfc.mult(Q2,inverse(a+s,order));
	b=pfc.hash_to_group((char *)"Bob");
	S1b=pfc.mult(P,inverse(b+s,order));
	S2b=pfc.mult(Q2,inverse(b+s,order));

//Signcrypt
	mip->IOBASE=256;
	M=(char *)"test message"; // to be signcrypted from Alice to Bob
	cout << "Signed Message=   " << M << endl;
	mip->IOBASE=16;
	
	pfc.precomp_for_mult(S1a);
	pfc.random(x);
	r=pfc.power(g,x);
	c=lxor(M,pfc.hash_to_aes_key(r));
	pfc.start_hash();
	pfc.add_to_hash(M);
	pfc.add_to_hash(r);
	h=pfc.finish_hash_to_group();
	S=pfc.mult(S1a,x+h);   
	T=pfc.mult(pfc.mult(P,b)+Q1pub,x);

//  Unsigncrypt

	pfc.precomp_for_pairing(S2b);  // Bob can precompute on his private key
	r=pfc.pairing(S2b,T);
	M=lxor(c,pfc.hash_to_aes_key(r));
	pfc.start_hash();
	pfc.add_to_hash(M);
	pfc.add_to_hash(r);
	h=pfc.finish_hash_to_group();
	rhs=pfc.pairing(pfc.mult(Q2,a)+Q2pub,S)*pfc.power(g,-h);
	mip->IOBASE=256;
	if (r==rhs)
	{
		cout << "Message is OK" << endl;
		cout << "Verified Message= " << M << endl;
	}
	else
		cout << "Message is bad    " << c << endl;

	return 0;
}
