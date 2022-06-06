/*
   Lewko & Waters HIBE
   See http://eprint.iacr.org/2009/482.pdf
   Appendix C

   Compile with modules as specified below

   For MR_PAIRING_CP curve
   cl /O2 /GX hibe.cpp cp_pair.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   For MR_PAIRING_MNT curve
   cl /O2 /GX hibe.cpp mnt_pair.cpp zzn6a.cpp ecn3.cpp zzn3.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib
	
   For MR_PAIRING_BN curve
   cl /O2 /GX hibe.cpp bn_pair.cpp zzn12a.cpp ecn2.cpp zzn4.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   For MR_PAIRING_KSS curve
   cl /O2 /GX hibe.cpp kss_pair.cpp zzn18.cpp zzn6.cpp ecn3.cpp zzn3.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   For MR_PAIRING_BLS curve
   cl /O2 /GX hibe.cpp bls_pair.cpp zzn24.cpp zzn8.cpp zzn4.cpp zzn2.cpp ecn4.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

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

	Big tau,a,alpha,s,c0,M,ID,y,c1,c2,t;
	int i;
	G1 g1,u1,h1,t1;
	G1 g1a,u1a,h1a,g1t,u1t,h1t;
	G1 c[6];
	G2 k[6]; 
	G2 g2,u2,h2,v2,v2d,f2;
	GT w;
	time_t seed;

	time(&seed);
    irand((long)seed);

// setup
	cout << "Setup" << endl;
	pfc.random(g1);
	pfc.random(g2);
	pfc.random(tau);
	u1=pfc.mult(g1,tau);
	u2=pfc.mult(g2,tau);
	pfc.random(tau);
	h1=pfc.mult(g1,tau);
	h2=pfc.mult(g2,tau);
	pfc.random(a);
	pfc.random(alpha);
	pfc.random(v2d);
	pfc.random(f2);
	pfc.random(tau);
	v2=pfc.mult(f2,tau)+(-pfc.mult(v2d,a));
	w=pfc.power(pfc.pairing(g2,g1),alpha);
	g1a=pfc.mult(g1,a);
	u1a=pfc.mult(u1,a);
	h1a=pfc.mult(h1,a);
	g1t=pfc.mult(g1,tau);
	u1t=pfc.mult(u1,tau);
	h1t=pfc.mult(h1,tau);

	pfc.precomp_for_mult(g1);
	pfc.precomp_for_mult(g1a);
	pfc.precomp_for_mult(g1t);	
	pfc.precomp_for_mult(u1);
	pfc.precomp_for_mult(u1a);
	pfc.precomp_for_mult(u1t);
	pfc.precomp_for_mult(h1);
	pfc.precomp_for_mult(h1a);
	pfc.precomp_for_mult(h1t);

	pfc.precomp_for_mult(g2);
	pfc.precomp_for_mult(u2);
	pfc.precomp_for_mult(h2);
	pfc.precomp_for_mult(v2);
	pfc.precomp_for_mult(v2d);
	pfc.precomp_for_mult(f2);

// public parameters {g1,u1,h1,g1a,u1a,h1a,g1t,u1t,h1t,w}
// master secret {g2,alpha,v2,v2d,u2,h2,f2}

// encrypt
	cout << "Encryption" << endl;
	mip->IOBASE=256;
	M=(char *)"a message"; // to be encrypted to Alice
	cout << "Message to be encrypted=   " << M << endl;
	mip->IOBASE=16;

	ID=pfc.hash_to_group((char *)"Alice");
	pfc.random(s);
	c0=lxor(M,pfc.hash_to_aes_key(pfc.power(w,s)));
	t=modmult(s,ID,order);
	c[0]=pfc.mult(h1,s)+pfc.mult(u1,t);
	c[1]=pfc.mult(h1a,s)+pfc.mult(u1a,t); // typo in paper
	c[2]=-(pfc.mult(h1t,s)+pfc.mult(u1t,t));
	c[3]=pfc.mult(g1,s);
	c[4]=pfc.mult(g1a,s);
	c[5]=-pfc.mult(g1t,s); // typo in paper

// keygen
	cout << "Keygen" << endl;
	pfc.random(y);
	pfc.random(c1);
	pfc.random(c2);

	t=modmult(y,ID,order);

	k[0]=-(pfc.mult(g2,y)+pfc.mult(v2,c1));
	k[1]=-pfc.mult(v2d,c1);
	k[2]=-pfc.mult(f2,c1);
	k[3]=pfc.mult(g2,alpha)+pfc.mult(h2,y)+pfc.mult(u2,t)+pfc.mult(v2,c2);
	k[4]=pfc.mult(v2d,c2);
	k[5]=pfc.mult(f2,c2);

	for (i=0;i<6;i++)
		pfc.precomp_for_pairing(k[i]);

// decrypt
	cout << "Decryption" << endl;
	G1 *r[6];
	G2 *l[6];

	for (i=0;i<6;i++)
	{
		r[i]=&c[i];
		l[i]=&k[i];
	}

	M=lxor(c0,pfc.hash_to_aes_key(pfc.multi_pairing(6,l,r)));	// Use private key

	mip->IOBASE=256;
	cout << "Decrypted message=         " << M << endl;

    return 0;
}
