/*
   Boneh-Gentry-Waters 
   Collusion Resistant Broadcast Encryption With Short Ciphertexts and Private Keys
   Implemented on Type-1 pairing

   Compile with modules as specified below

	For MR_PAIRING_SSP curves
	cl /O2 /GX bgw.cpp ssp_pair.cpp ecn.cpp zzn2.cpp zzn.cpp big.cpp miracl.lib
  
	For MR_PAIRING_SS2 curves
    cl /O2 /GX bgw.cpp ss2_pair.cpp ec2.cpp gf2m4x.cpp gf2m.cpp big.cpp miracl.lib
    
	or of course

    g++ -O2 bgw.cpp ss2_pair.cpp ec2.cpp gf2m4x.cpp gf2m.cpp big.cpp miracl.a -o bgw

   See http://eprint.iacr.org/2005/018.pdf
   Section 3.1
 
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

#define N 20             // total number of potential recipients

#define NS 5             // number of recipients for this broadcast
int S[NS]={2,4,5,6,14};  // group of recipients
#define PERSON 6		 // sample recipient

int main()
{   
	PFC pfc(AES_SECURITY);  // initialise pairing-friendly curve
	time_t seed;

	int i,j;
	G1 g,v,gi[2*N],d[N],Hdr[2],s;
	GT K;
	Big alpha,gamma,t;

	time(&seed);       // initialise (insecure!) random numbers
    irand((long)seed);

//setup
	pfc.random(g);
	pfc.random(alpha);
	gi[0]=pfc.mult(g,alpha);
	for (i=1;i<2*N;i++)
		gi[i]=pfc.mult(gi[i-1],alpha);

	pfc.random(gamma);
	v=pfc.mult(g,gamma);

	for (i=0;i<N;i++)
		d[i]=pfc.mult(gi[i],gamma);

//encrypt to group S using Public Key
	pfc.random(t);
	K=pfc.power(pfc.pairing(gi[N-1],gi[0]),t);
	Hdr[0]=pfc.mult(g,t);
	Hdr[1]=v;
	for (i=0;i<NS;i++)
	{
		j=S[i];
		Hdr[1]=Hdr[1]+gi[N-j];
	}
	Hdr[1]=pfc.mult(Hdr[1],t);
	cout << "Encryption Key= " << pfc.hash_to_aes_key(K) << endl;

//decrypt by PERSON
	
	s=d[PERSON-1];
	for (i=0;i<NS;i++)
	{
		j=S[i];
		if (j==PERSON) continue;
		s=s+gi[N-j+PERSON];
	}
	Hdr[0]=-Hdr[0];  // to avoid division
	G1 *g1[2],*g2[2];
	g1[0]=&gi[PERSON-1]; g1[1]=&s;
	g2[0]=&Hdr[1]; g2[1]=&Hdr[0];

	K=pfc.multi_pairing(2,g2,g1);

//	K=pfc.pairing(gi[PERSON-1],Hdr[1]);
//	K=K*pfc.pairing(s,Hdr[0]);  
	cout << "Decryption Key= " << pfc.hash_to_aes_key(K) << endl;

    return 0;
}
