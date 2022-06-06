/*
   Fuzzy IBE - Sahai-Waters
   See http://eprint.iacr.org/2004/086.pdf
   Section 4.1

   Compile with modules as specified below

   For MR_PAIRING_CP curve
   cl /O2 /GX fuzzy.cpp cp_pair.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   For MR_PAIRING_MNT curve
   cl /O2 /GX fuzzy.cpp mnt_pair.cpp zzn6a.cpp ecn3.cpp zzn3.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib
	
   For MR_PAIRING_BN curve
   cl /O2 /GX fuzzy.cpp bn_pair.cpp zzn12a.cpp ecn2.cpp zzn4.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   For MR_PAIRING_KSS curve
   cl /O2 /GX fuzzy.cpp kss_pair.cpp zzn18.cpp zzn6.cpp ecn3.cpp zzn3.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   For MR_PAIRING_BLS curve
   cl /O2 /GX fuzzy.cpp bls_pair.cpp zzn24.cpp zzn8.cpp zzn4.cpp zzn2.cpp ecn4.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

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

//
//  Note that in this case Bobs attributes are "close enough" to Alices
//  so that he can decrypt
//

#define NATTR  20 // Universe of attributes
#define NALICE 7  // number of Alice's attributes
#define NBOB   7  // number of Bob's attributes
#define Nd     5  // number required in common

int Alice[NALICE]={7,6,3,4,12,1,9};    // Alice's attributes
int Bob[NBOB]=    {6,3,4,12,5,10,7};   // Bob's attributes

// Check if person has attribute a
int has_attribute(int num,int *attr,int a)
{
	for (int i=0;i<num;i++)
	{
		if (a==attr[i]) return i;
	}
	return -1;
}

// Lagrange interpolation
Big lagrange(int i,int *S,int d,Big order)
{
	int j,k;
	Big z=1;
	for (k=0;k<d;k++)
	{
		j=S[k];
		if (j!=i) z=modmult(z,moddiv(order-j,(Big)(i-j),order),order);
	}
	return z;
}

int main()
{   
	PFC pfc(AES_SECURITY);  // initialise pairing-friendly curve
	miracl *mip=get_mip();  // get handle on mip (Miracl Instance Pointer)
	Big order=pfc.order();  // get pairing-friendly group order

	time_t seed;            // crude randomisation
	time(&seed);
    irand((long)seed);

// setup - for 20 attributes 1-20
	int i,j,k,n,ik,S[NBOB];
	Big s,y,qi,M,ED,t[NATTR];
	Big poly[Nd];
	G1 P,T[NATTR],E[Nd],AE[NALICE];
	G2 Q,AD[NALICE],BD[NBOB];
	GT Y,DB;

	pfc.random(P);
	pfc.random(Q);
	pfc.precomp_for_mult(Q);  // Q is fixed, so precompute on it

	pfc.random(y);
	Y=pfc.power(pfc.pairing(Q,P),y);
	for (i=0;i<NATTR;i++)
	{
		pfc.random(t[i]);                 // Note t[i] will be 2*AES_SECURITY bits long
		T[i]=pfc.mult(P,t[i]);            // which may be less than the group order.
		pfc.precomp_for_mult(T[i],TRUE);  // T[i] are system params, so precompute on them
                                          // Note second parameter indicates that all multipliers 
										  // must  be <=2*AES_SECURITY bits, which may be shorter
										  // than the full group size. 
	}

// key generation for Alice

	poly[0]=y;
	for (i=1;i<Nd;i++)
		pfc.random(poly[i]);
	
	for (j=0;j<NALICE;j++)
	{
		i=Alice[j];
		qi=y; ik=i;
		for (k=1;k<Nd;k++)
		{ // evaluate polynomial a0+a1*x+a2*x^2... for x=i;
			qi+=modmult(poly[k],(Big)ik,order);
			ik*=i;
			qi%=order;
		}

		AD[j]=pfc.mult(Q,moddiv(qi,t[i],order));  // exploits precomputation
	}

// key generation for Bob

	poly[0]=y;
	for (i=1;i<Nd;i++)
		pfc.random(poly[i]);
	
	for (j=0;j<NBOB;j++)
	{
		i=Bob[j];
		qi=y; ik=i;
		for (k=1;k<Nd;k++)
		{ // evaluate polynomial a0+a1*x+a2*x^2... for x=i;
			qi+=modmult(poly[k],(Big)ik,order);
			ik*=i;
			qi%=order;
		}

		BD[j]=pfc.mult(Q,moddiv(qi,t[i],order));
		pfc.precomp_for_pairing(BD[j]);   // Bob precomputes on his private key
	}

// Encryption to Alice
	mip->IOBASE=256;
	M=(char *)"test message"; 
	cout << "Message to be encrypted=   " << M << endl;
	mip->IOBASE=16;
	pfc.random(s);
	ED=lxor(M,pfc.hash_to_aes_key(pfc.power(Y,s)));
	for (j=0;j<NALICE;j++)
	{
		i=Alice[j];
		AE[j]=pfc.mult(T[i],s);   // exploit precomputation
	}

// Decryption by Bob

// set up to exploit multi-pairing
	G1 *g1[5];
	G2 *g2[5];

	k=0;
	for (j=0;j<NBOB;j++)
	{ // check for common attributes
		i=Bob[j];
		n=has_attribute(NALICE,Alice,i);
		if (n<0) continue;  // Alice doesn't have it
		S[k]=i;
		E[k]=AE[n];
		g2[k]=&BD[j];
		k++;
	}
	if (k<Nd)
	{
		cout << "Bob does not have enough attributes in common with Alice to decrypt successfully" << endl;
		exit(0);
	}

// faster to multiply in G1 than to exponentiate in GT

	for (j=0;j<Nd;j++)
	{
		i=S[j];
		E[j]=pfc.mult(E[j],lagrange(i,S,Nd,order));
		g1[j]=&E[j];
	}

	DB=pfc.multi_pairing(Nd,g2,g1);

	M=lxor(ED,pfc.hash_to_aes_key(DB));
	mip->IOBASE=256;
	cout << "Decrypted message=         " << M << endl;

    return 0;
}
