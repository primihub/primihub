
/***************************************************************************
                                                                           *
Copyright 2013 CertiVox IOM Ltd.                                           *
                                                                           *
This file is part of CertiVox MIRACL Crypto SDK.                           *
                                                                           *
The CertiVox MIRACL Crypto SDK provides developers with an                 *
extensive and efficient set of cryptographic functions.                    *
For further information about its features and functionalities please      *
refer to http://www.certivox.com                                           *
                                                                           *
* The CertiVox MIRACL Crypto SDK is free software: you can                 *
  redistribute it and/or modify it under the terms of the                  *
  GNU Affero General Public License as published by the                    *
  Free Software Foundation, either version 3 of the License,               *
  or (at your option) any later version.                                   *
                                                                           *
* The CertiVox MIRACL Crypto SDK is distributed in the hope                *
  that it will be useful, but WITHOUT ANY WARRANTY; without even the       *
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. *
  See the GNU Affero General Public License for more details.              *
                                                                           *
* You should have received a copy of the GNU Affero General Public         *
  License along with CertiVox MIRACL Crypto SDK.                           *
  If not, see <http://www.gnu.org/licenses/>.                              *
                                                                           *
You can be released from the requirements of the license by purchasing     *
a commercial license. Buying such a license is mandatory as soon as you    *
develop commercial activities involving the CertiVox MIRACL Crypto SDK     *
without disclosing the source code of your own applications, or shipping   *
the CertiVox MIRACL Crypto SDK with a closed source product.               *
                                                                           *
***************************************************************************/
/*
 * pairing.h
 *
 * High level interface to pairing functions - Type 3 pairings
 * 
 * GT=pairing(G2,G1)
 *
 * This is calculated on a Pairing Friendly Curve (PFC), which must first be defined.
 *
 * G1 is a point over the base field, and G2 is a point over an extension field. 
 * GT is a finite field point over the k-th extension, where k is the embedding degree.
 */

#ifndef PAIRING_3_H
#define PAIRING_3_H

#include "ecn.h"	// G1

// K=2 Cocks-Pinch curve
#ifdef MR_PAIRING_CP
#include "zzn2.h"
#define WINDOW_SIZE 8 // window size for precomputation
#define G2_TYPE ECn
#define G2_SUBTYPE ZZn
#define GT_TYPE ZZn2
#endif

// k=6 MNT curve
#ifdef MR_PAIRING_MNT
#include "zzn2.h"
#include "ecn3.h"	// G2
#include "zzn6a.h"	// GT
#define WINDOW_SIZE 8 // window size for precomputation
#define G2_TYPE ECn3
#define G2_SUBTYPE ZZn3
#define GT_TYPE ZZn6
#define FROB_TYPE ZZn2
#endif

//k=12 BN curve
#ifdef MR_PAIRING_BN
#include "zzn2.h"
#include "ecn2.h"	// G2
#include "zzn12a.h"	// GT
#define WINDOW_SIZE 8 // window size for precomputation
#define G2_TYPE ECn2
#define G2_SUBTYPE ZZn2
#define GT_TYPE ZZn12
#define FROB_TYPE ZZn2
#endif

// k=18 KSS curve
#ifdef MR_PAIRING_KSS
#include "ecn3.h"	// G2
#include "zzn18.h"	// GT
#define WINDOW_SIZE 8 // window size for precomputation
#define G2_TYPE ECn3
#define G2_SUBTYPE ZZn3
#define GT_TYPE ZZn18
#define FROB_TYPE ZZn
#endif

// k=24 BLS curve
#ifdef MR_PAIRING_BLS
#include "ecn4.h"	// G2
#include "zzn24.h"	// GT
#define WINDOW_SIZE 8 // window size for precomputation
#define G2_TYPE ECn4
#define G2_SUBTYPE ZZn4
#define GT_TYPE ZZn24
#define FROB_TYPE ZZn2
#endif

class PFC;
extern void read_only_error(void);

// Multiples of G1 may be precomputed. If it is, the instance becomes read-only.
// Read-only instances cannot be written to - causes an error and exits
// Precomputation for pairing calculation only possible for G2 for ate pairing

class G1
{
public:
	ECn g;

	ECn *mtable;   // pointer to values precomputed for multiplication
 
	int mtbits;
    G1()   {mtable=NULL; mtbits=0;}
	G1(const G1& w) {mtable=NULL; mtbits=0; g=w.g;}

	G1& operator=(const G1& w) 
	{
		if (mtable==NULL) g=w.g; 
		else read_only_error(); 
		return *this;
	} 
	int spill(char *&);
	void restore(char *);
	friend G1 operator-(const G1&);
	friend G1 operator+(const G1&,const G1&);
	friend BOOL operator==(const G1& x,const G1& y)
      {if (x.g==y.g) return TRUE; else return FALSE; }
	friend BOOL operator!=(const G1& x,const G1& y)
      {if (x.g!=y.g) return TRUE; else return FALSE; }
	~G1()  {if (mtable!=NULL) {delete [] mtable; mtable=NULL;}}
};

//
// This is just a G2_TYPE. But we want to restrict the ways in which it can be used. 
// We want the instances to always be of an order compatible with the PFC
//

class G2
{
public:
	G2_TYPE g;

	G2_SUBTYPE *ptable; // pointer to values precomputed for pairing
	G2_TYPE *mtable; // pointer to values precomputed for multiplication
	int mtbits;

    G2()   {ptable=NULL; mtable=NULL; mtbits=0;}
	G2(const G2& w) {ptable=NULL; mtable=NULL; mtbits=0; g=w.g;}
	G2& operator=(const G2& w) 
	{ 
		if (ptable==NULL && mtable==NULL)	g=w.g; 
		else read_only_error();
		return *this; 
	} 
	int spill(char *&);
	void restore(char *);

	friend G2 operator-(const G2&);
	friend G2 operator+(const G2&,const G2&);
	friend BOOL operator==(G2& x,G2& y)
      {if (x.g==y.g) return TRUE; else return FALSE; }
	friend BOOL operator!=(G2& x,G2& y)
      {if (x.g!=y.g) return TRUE; else return FALSE; }

	~G2() {if (ptable!=NULL) {delete [] ptable; ptable=NULL;}
	       if (mtable!=NULL) {delete [] mtable; mtable=NULL;}}
};

class GT
{
public:
	GT_TYPE g;

	GT_TYPE *etable;
	int etbits;

	GT() {etable=NULL; etbits=0;}
	GT(const GT& w) {etable=NULL; etbits=0; g=w.g;}
	GT(int d) {etable=NULL; etbits=0; g=d;}

	GT& operator=(const GT& w)  
	{
		if (etable==NULL) g=w.g; 
		else read_only_error();
		return *this;
	} 
	int spill(char *&);
	void restore(char *);
	friend GT operator*(const GT&,const GT&);
	friend GT operator/(const GT&,const GT&);
	friend BOOL operator==(const GT& x,const GT& y)
      {if (x.g==y.g) return TRUE; else return FALSE; }
	friend BOOL operator!=(const GT& x,const GT& y)
      {if (x.g!=y.g) return TRUE; else return FALSE; }
	~GT() {if (etable!=NULL) {delete [] etable; etable=NULL;}}
};

// pairing friendly curve class

class PFC
{
public:
	Big *B;        // y^2=x^3+Ax+B. This is B 
	Big *x;        // curve parameter
	Big *mod;
	Big *ord;
	Big *cof; 
	Big *npoints;  
	Big *trace;

#ifdef MR_PAIRING_BN
	Big *BB[4][4],*WB[4],*SB[2][2],*W[2];
	ZZn *Beta;
#endif
#ifdef MR_PAIRING_KSS
	Big *BB[6][6],*WB[6],*SB[2][2],*W[2];
	ZZn *Beta;
#endif
#ifdef MR_PAIRING_BLS
	ZZn *Beta;
#endif
#ifdef FROB_TYPE
	FROB_TYPE *frob;    // Frobenius constant
#endif

	int S;
#ifdef MR_PAIRING_MNT
	sha SH;
#else
	sha256 SH;
#endif

#ifndef MR_NO_RAND
	csprng *RNG;
#endif

	PFC(int, csprng *rng=NULL);
	Big order(void) {return *ord;}
	GT power(const GT&,const Big&);  
	G1 mult(const G1&,const Big&);
	G2 mult(const G2&,const Big&);
	void hash_and_map(G1&,char *);
	void hash_and_map(G2&,char *);
	void random(Big&);
	void rankey(Big&);
	void random(G1&);
	void random(G2&);
	BOOL member(const GT&);			// test if element is member of pairing friendly group

	int precomp_for_pairing(G2&);  // precompute multiples of G2 that occur in Miller loop
	int precomp_for_mult(G1&,BOOL small=FALSE);     // precompute multiples of G1 for precomputation
	int precomp_for_mult(G2&,BOOL small=FALSE);
	int precomp_for_power(GT&,BOOL small=FALSE);    // returns number of precomputed values
// small=TRUE if exponent is always less than full group size and equal to 2*Security level
// creates a smaller table

	int spill(G2&,char *&);
	void restore(char *,G2&);

	Big hash_to_aes_key(const GT&);
	Big hash_to_group(char *);
	Big hash_to_group(char *, int);
	GT miller_loop(const G2&,const G1&);
	GT final_exp(const GT&);
	GT pairing(const G2&,const G1&);
// parameters: number of pairings n, pointers to G1 and G2 elements
	GT multi_miller(int n,G2 **,G1 **);
	GT multi_pairing(int n,G2 **,G1 **); //product of pairings
	void start_hash(void);
	void add_to_hash(const G1&);
	void add_to_hash(const G2&);
	void add_to_hash(const GT&);
	void add_to_hash(const Big&);
	Big finish_hash_to_group(void);
	Big finish_hash_to_aes_key(void);
	void seed_rng(int s);
	~PFC() ;
};

#ifndef MR_AFFINE_ONLY

extern void force(ZZn&,ZZn&,ZZn&,ECn&);
extern void extract(ECn&,ZZn&,ZZn&,ZZn&);

#endif

extern void force(ZZn&,ZZn&,ECn&);
extern void extract(ECn&,ZZn&,ZZn&);

#endif
