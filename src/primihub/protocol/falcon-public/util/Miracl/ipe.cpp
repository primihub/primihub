/*
   J.H. Park 
   Inner-product encryption under standard assumptions - Des. Codes Cryptogr. (2011) 58:235–257
   Implemented on Type-3 pairing

   Compile with modules as specified below

   For MR_PAIRING_CP curve
   cl /O2 /GX ipe.cpp cp_pair.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   For MR_PAIRING_MNT curve
   cl /O2 /GX ipe.cpp mnt_pair.cpp zzn6a.cpp ecn3.cpp zzn3.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib
	
   For MR_PAIRING_BN curve
   cl /O2 /GX ipe.cpp bn_pair.cpp zzn12a.cpp ecn2.cpp zzn4.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   For MR_PAIRING_KSS curve
   cl /O2 /GX ipe.cpp kss_pair.cpp zzn18.cpp zzn6.cpp ecn3.cpp zzn3.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   For MR_PAIRING_BLS curve
   cl /O2 /GX ipe.cpp bls_pair.cpp zzn24.cpp zzn8.cpp zzn4.cpp zzn2.cpp ecn4.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

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

#define n 10             // number of attributes

// generate v[i] such that x.v=0 (dot product)

void inner_product(Big *x,Big *v,Big& order)
{
	Big prod=0;
	for (int i=0;i<n-1;i++)
		prod+=modmult(x[i],v[i],order);
	v[n-1]=moddiv(order-prod,x[n-1],order);
}

int main()
{   
	PFC pfc(AES_SECURITY);  // initialise pairing-friendly curve
	miracl* mip=get_mip();

	time_t seed;

	int i,j;
	G1 g1,g1_1;
	G2 g2,g2_2;
	G1 W1[n],W2[n],T1[n],T2[n],F1[n],F2[n],H1[n],H2[n];
	G1 U1,U2,V1,V2;
	G1 A,B,C1[n],C2[n],C3[n],C4[n];
	G2 KA,KB,K1[n],K2[n],K3[n],K4[n];
	GT alpha;
	Big omega,delta1,delta2,theta1,theta2,w1[n],w2[n],t1[n],t2[n],f1[n],f2[n],h1[n],h2[n];
	Big M,C,D,s1,s2,s3,s4;
	Big x[n],v[n];
	Big lambda1,lambda2,r[n],phi[n],t;

// setup	
	cout << "Setup" << endl;
	time(&seed);
    irand((long)seed);

	Big order=pfc.order();

	pfc.random(g1);
	pfc.random(g2_2);
	pfc.random(g2);

	pfc.random(delta1);
	pfc.random(delta2);
	pfc.random(theta1);
	pfc.random(theta2);
	pfc.random(omega);
	for (i=0;i<n;i++)
	{
		pfc.random(w1[i]);
		pfc.random(t1[i]);
		pfc.random(f1[i]);
		pfc.random(f2[i]);
		pfc.random(h1[i]);
		pfc.random(h2[i]);
		w2[i]=moddiv(omega+modmult(delta2,w1[i],order),delta1,order);
		t2[i]=moddiv(omega+modmult(theta2,t1[i],order),theta1,order);

		W1[i]=pfc.mult(g1,w1[i]);
		W2[i]=pfc.mult(g1,w2[i]);
		pfc.precomp_for_mult(W1[i]);  // precompute on everything!
		pfc.precomp_for_mult(W2[i]);
		T1[i]=pfc.mult(g1,t1[i]);
		T2[i]=pfc.mult(g1,t2[i]);
		pfc.precomp_for_mult(T1[i]);
		pfc.precomp_for_mult(T2[i]);
		F1[i]=pfc.mult(g1,f1[i]);
		F2[i]=pfc.mult(g1,f2[i]);
		pfc.precomp_for_mult(F1[i]);
		pfc.precomp_for_mult(F2[i]);
		H1[i]=pfc.mult(g1,h1[i]);
		H2[i]=pfc.mult(g1,h2[i]);
		pfc.precomp_for_mult(H1[i]);
		pfc.precomp_for_mult(H2[i]);
	}

	U1=pfc.mult(g1,delta1);
	U2=pfc.mult(g1,delta2);
	V1=pfc.mult(g1,theta1);
	V2=pfc.mult(g1,theta2);
	g1_1=pfc.mult(g1,omega);
	alpha=pfc.pairing(g2_2,g1);
	pfc.precomp_for_power(alpha);

	pfc.precomp_for_mult(U1);
	pfc.precomp_for_mult(U2);
	pfc.precomp_for_mult(V1);
	pfc.precomp_for_mult(V2);

	pfc.precomp_for_mult(g2);
	pfc.precomp_for_mult(g1);
	pfc.precomp_for_mult(g1_1);

// Encrypt
	cout << "Encrypt" << endl;
	mip->IOBASE=256;
	M=(char *)"message"; // to be encrypted
	cout << "Message to be encrypted=   " << M << endl;
	mip->IOBASE=16;

	pfc.random(s1);
	pfc.random(s2);
	pfc.random(s3);
	pfc.random(s4);
	for (i=0;i<n;i++)
		pfc.random(x[i]);
	
	A=pfc.mult(g1,s2);
	B=pfc.mult(g1_1,s1);
	for (i=0;i<n;i++)
	{
		C1[i]=pfc.mult(W1[i],s1)+pfc.mult(F1[i],s2)+pfc.mult(U1,modmult(x[i],s3,order));
		C2[i]=pfc.mult(W2[i],s1)+pfc.mult(F2[i],s2)+pfc.mult(U2,modmult(x[i],s3,order));
	}
	for (i=0;i<n;i++)
	{
		C3[i]=pfc.mult(T1[i],s1)+pfc.mult(H1[i],s2)+pfc.mult(V1,modmult(x[i],s4,order));
		C4[i]=pfc.mult(T2[i],s1)+pfc.mult(H2[i],s2)+pfc.mult(V2,modmult(x[i],s4,order));
	}
	D=pfc.hash_to_aes_key(pfc.power(alpha,s2));
	C=lxor(M,D);   // ciphertext

// KeyGen
	cout << "KeyGen" << endl;
	for (i=0;i<n;i++)
		pfc.random(v[i]);
	inner_product(x,v,order);    // frig v such that x.v=0

//	pfc.random(v[0]);  // Now x.v !=0, which will screw up decryption!

	pfc.random(lambda1);
	pfc.random(lambda2);
	for (i=0;i<n;i++)
	{
		pfc.random(r[i]);
		pfc.random(phi[i]);
	}

	for (i=0;i<n;i++)
	{
		t=modmult(lambda1,v[i],order);
		K1[i]=pfc.mult(g2,modmult(t,w2[i],order)-modmult(delta2,r[i],order));
		K2[i]=pfc.mult(g2,modmult(delta1,r[i],order)-modmult(t,w1[i],order));
		pfc.precomp_for_pairing(K1[i]);
		pfc.precomp_for_pairing(K2[i]);
	}
	for (i=0;i<n;i++)
	{
		t=modmult(lambda2,v[i],order);
		K3[i]=pfc.mult(g2,modmult(t,t2[i],order)-modmult(theta2,phi[i],order));
		K4[i]=pfc.mult(g2,modmult(theta1,phi[i],order)-modmult(t,t1[i],order));
		pfc.precomp_for_pairing(K3[i]);
		pfc.precomp_for_pairing(K4[i]);
	}

	KA=g2_2;
	for (i=0;i<n;i++)
	{
		KA=KA+pfc.mult(K1[i],-f1[i])+pfc.mult(K2[i],-f2[i])+pfc.mult(K3[i],-h1[i])+pfc.mult(K4[i],-h2[i]);
		KB=KB+pfc.mult(g2,-(r[i]+phi[i])%order);
	}
	pfc.precomp_for_pairing(KA);
	pfc.precomp_for_pairing(KB);

// Decrypt
	
	cout << "Decrypt" << endl;
	G2 **left=new G2* [4*n+2];
	G1 **right=new G1* [4*n+2];

	left[0]=&KA; right[0]=&A;  // e(K,CD)
	left[1]=&KB; right[1]=&B;  // e(L,TC)
	j=2;
	for (i=0;i<n;i++)
	{

		left[j]=&K1[i];
		right[j]=&C1[i];
		j++;
		left[j]=&K2[i];
		right[j]=&C2[i];
		j++;
		left[j]=&K3[i];
		right[j]=&C3[i];
		j++;
		left[j]=&K4[i];
		right[j]=&C4[i];
		j++;
	}

	M=lxor(C,pfc.hash_to_aes_key(pfc.multi_pairing(4*n+2,left,right)));

	mip->IOBASE=256;
	cout << "Decrypted message=         " << M << endl;

    return 0;
}
