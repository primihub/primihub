/*
   Scott's AKE Client/Server testbed

   Compile with modules as specified below

   For MR_PAIRING_CP curve
   cl /O2 /GX ake.cpp cp_pair.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   For MR_PAIRING_MNT curve
   cl /O2 /GX ake.cpp mnt_pair.cpp zzn6a.cpp ecn3.cpp zzn3.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib
	
   For MR_PAIRING_BN curve
   cl /O2 /GX ake.cpp bn_pair.cpp zzn12a.cpp ecn2.cpp zzn4.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   For MR_PAIRING_KSS curve
   cl /O2 /GX ake.cpp kss_pair.cpp zzn18.cpp zzn6.cpp ecn3.cpp zzn3.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   For MR_PAIRING_BLS curve
   cl /O2 /GX ake.cpp bls_pair.cpp zzn24.cpp zzn8.cpp zzn4.cpp zzn2.cpp ecn4.cpp big.cpp zzn.cpp ecn.cpp miracl.lib

   See http://eprint.iacr.org/2002/164
 
*/

#include <iostream>
#include <ctime>

//********* choose just one of these pairs **********
//#define MR_PAIRING_CP      // AES-80 security   
//#define AES_SECURITY 80

//#define MR_PAIRING_MNT	// AES-80 security
//#define AES_SECURITY 80

//#define MR_PAIRING_BN    // AES-128 or AES-192 security
//#define AES_SECURITY 128
//#define AES_SECURITY 192

//#define MR_PAIRING_KSS    // AES-192 security
//#define AES_SECURITY 192

#define MR_PAIRING_BLS    // AES-256 security
#define AES_SECURITY 256
//*********************************************

#include "pairing_3.h"

int main()
{   
	PFC pfc(AES_SECURITY);  // initialise pairing-friendly curve

	time_t seed;
	G1 Alice,Bob,sA,sB;
    G2 B6,Server,sS;
	GT res,sp,ap,bp;
	Big ss,s,a,b;

    time(&seed);
    irand((long)seed);

    pfc.random(ss);    // TA's super-secret 

    cout << "Mapping Server ID to point" << endl;
	pfc.hash_and_map(Server,(char *)"Server");

    cout << "Mapping Alice & Bob ID's to points" << endl;
    pfc.hash_and_map(Alice,(char *)"Alice");
    pfc.hash_and_map(Bob,(char *)"Robert");

    cout << "Alice, Bob and the Server visit Trusted Authority" << endl; 

    sS=pfc.mult(Server,ss); 
	sA=pfc.mult(Alice,ss);
    sB=pfc.mult(Bob,ss); 

    cout << "Alice and Server Key Exchange" << endl;

	
    pfc.random(a);  // Alice's random number
    pfc.random(s);   // Server's random number

	res=pfc.pairing(Server,sA);

	if (!pfc.member(res))
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
	
	ap=pfc.power(res,a);

	res=pfc.pairing(sS,Alice);
	
   	if (!pfc.member(res))
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }

	sp=pfc.power(res,s);

    cout << "Alice  Key= " << pfc.hash_to_aes_key(pfc.power(sp,a)) << endl;
    cout << "Server Key= " << pfc.hash_to_aes_key(pfc.power(ap,s)) << endl;

    cout << "Bob and Server Key Exchange" << endl;

    pfc.random(b);   // Bob's random number
    pfc.random(s);   // Server's random number

	res=pfc.pairing(Server,sB);
    if (!pfc.member(res))
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    bp=pfc.power(res,b);

	res=pfc.pairing(sS,Bob);
    if (!pfc.member(res))
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }

    sp=pfc.power(res,s);

    cout << "Bob's  Key= " << pfc.hash_to_aes_key(pfc.power(sp,b)) << endl;
    cout << "Server Key= " << pfc.hash_to_aes_key(pfc.power(bp,s)) << endl;

    return 0;
}
