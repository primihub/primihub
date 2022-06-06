
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
 * ss.cpp
 *
 * Super-Singular curve, eta_T pairing embedding degree 4
 *
 * Provides high level interface to pairing functions
 * 
 * GT=pairing(G1,G1)
 *
 * This is calculated on a Pairing Friendly Curve (PFC), which must first be defined.
 *
 * G1 is a point over the base field
 * GT is a finite field point over the 4-th extension, where 4 is the embedding degree.
 */


#define MR_PAIRING_SS2
#include "pairing_1.h"

// Using SHA256 as basic hash algorithm
//
// Hash function
// 

#define HASH_LEN 32

Big H1(char *string)
{ // Hash a zero-terminated string to a number < modulus
    Big h;
    char s[HASH_LEN];
    int i,j,M; 
    sha256 sh;

    shs256_init(&sh);

    for (i=0;;i++)
    {
        if (string[i]==0) break;
        shs256_process(&sh,string[i]);
    }
    shs256_hash(&sh,s);
    M=get_mip()->M;
    h=1; j=0; i=1;
    forever
    {
        h*=256; 
        if (j==HASH_LEN)  {h+=i++; j=0;}
        else         h+=s[j++];
        if (bits(h)>=M) break;
    }
	while (bits(h)>=M) h/=2;
    return h;
}

// general hash

void PFC::start_hash(void)
{
	shs256_init(&SH);
}

Big PFC::finish_hash_to_group(void)
{
	Big hash;
	Big o=pow((Big)2,2*S);
	char s[HASH_LEN];
    shs256_hash(&SH,s);
    hash=from_binary(HASH_LEN,s);
	return hash%o;
}

void PFC::add_to_hash(const GT& x)
{
	int i,m;
	GF2m xx[4];
	Big a;
	GF2m4x w=x.g;
	
	w.get(xx[0],xx[1],xx[2],xx[3]);

    for (i=0;i<4;i++)
    {
        a=(Big)xx[i];
        while (a>0)
        {
            m=a%256;
            shs256_process(&SH,m);
            a/=256;
        }
    }
}

void PFC::add_to_hash(const G1& x)
{
	Big a,X,Y;
	int i,m;
	x.g.get(X,Y);
	a=X;
    while (a>0)
    {
        m=a%256;
        shs256_process(&SH,m);
        a/=256;
    }
	a=Y;
    while (a>0)
    {
        m=a%256;
        shs256_process(&SH,m);
        a/=256;
    }
}

void PFC::add_to_hash(const Big& x)
{
	int m;
	Big a=x;
    while (a>0)
    {
        m=a%256;
        shs256_process(&SH,m);
        a/=256;
    }
}

// random multiplier

void PFC::random(Big& w)
{
	if (RNG==NULL) w=rand(2*S,2);
	else w=strong_rand(RNG,2*S,2);
}

// random AES key

void PFC::rankey(Big& k)
{
	if (RNG==NULL) k=rand(S,2);
	else k=strong_rand(RNG,S,2);
}

// Compress and hash a GF2m4x to a big number

Big H2(GF2m4x x)
{ 
    sha256 sh;
    Big a,hash;
	GF2m xx[4];
    char s[HASH_LEN];
    int i,j,m;

    shs256_init(&sh);
    x.get(xx[0],xx[1],xx[2],xx[3]); 
 
    for (i=0;i<4;i++)
    {
        a=xx[i];
        while (a>0)
        {
            m=a%256;
            shs256_process(&sh,m);
            a/=256;
        }
    }
    shs256_hash(&sh,s);
    hash=from_binary(HASH_LEN,s);
    return hash;
}

//
// Extract ECn point in internal GF2m format
//

void extract(EC2& A,GF2m& x,GF2m& y)
{ 
    x=(A.get_point())->X;
    y=(A.get_point())->Y;
}

// Does nothing...

int PFC::precomp_for_pairing(G1& w)
{
	return 0;
}

//
// eta_T Pairing G1 x G1 -> GT
//
// P is a point of order q in G1. Q is also a point of order q in G1. 
//
// Note miller -> miller variable
// Loop unrolled x2 for speed
// 

GT PFC::miller_loop(const G1& PP,const G1& QQ)
{ 
    GF2m xp,yp,xq,yq,t;
    GF2m4x miller,w,u,u0,u1,v,f,res;
	EC2 P,Q;
	GT mill;
    int i,imod4,m=get_mip()->M;         

	P=PP.g; Q=QQ.g;
    normalise(P); normalise(Q);
    extract(P,xp,yp);
    extract(Q,xq,yq);
 
	imod4=((m+1)/2)%4;
	if (imod4==1)
	{                                                      //              (X=1)                                                     //              (Y=0)   
		t=xp;                                                 // 0            (X+1)
		f.set(t*(xp+xq+1)+yq+yp+B,t+xq+1,t+xq,0);             // 0            (Y)
		miller=1;
		for (i=0;i<(m-3)/2;i+=2)
		{
			t=xp+1; xp=sqrt(xp); yp=sqrt(yp);                 // 1            (X)
			u0.set(t*(xp+xq+1)+yp+yq,t+xq+1,t+xq,0);          // 1   0        (X) ((X+1)*(xp+1)+Y)
			xq*=xq; yq*=yq;

			t=xp+1; xp=sqrt(xp); yp=sqrt(yp);
			u1.set(t*(xp+xq+1)+yp+yq,t+xq+1,t+xq,0);
			xq*=xq; yq*=yq;

			u=mul(u0,u1);
			miller*=u;
		}

// final step

		t=xp+1; xp=sqrt(xp); yp=sqrt(yp);
		u.set(t*(xp+xq+1)+yp+yq,t+xq+1,t+xq,0);
		miller*=u;
	}

	if (imod4==0)
	{													  //              (X=0)                                                     //              (Y=0)
		t=xp+1;                                               // 1            (X+1)
		f.set(t*(xq+xp+1)+yq+yp+B,t+xq+1,t+xq,0);             // 0            (Y)
		miller=1;
  
		for (i=0;i<(m-1)/2;i+=2)  
		{
// loop is unrolled x 2 
			t=xp; xp=sqrt(xp); yp=sqrt(yp);                   // 0            (X)
			u0.set(t*(xp+xq)+yp+yq+xp+1,t+xq+1,t+xq,0);       // 0  xp+1      (X)  ((X+1)*(xp+1)+Y
			xq*=xq; yq*=yq;

			t=xp; xp=sqrt(xp); yp=sqrt(yp);
			u1.set(t*(xp+xq)+yp+yq+xp+1,t+xq+1,t+xq,0);
			xq*=xq; yq*=yq;

			u=mul(u0,u1);
			miller*=u;
		}
	}

	if (imod4==2)
	{													  //              (X=0)                                                         //              (Y=1)
		t=xp+1;                                               // 1            (X+1)
		f.set(t*(xq+xp+1)+yq+yp+B+1,t+xq+1,t+xq,0);           // 1            (Y)
		miller=1;
		for (i=0;i<(m-1)/2;i+=2)
		{
			t=xp;  xp=sqrt(xp); yp=sqrt(yp);                 // 0            (X)
			u0.set(t*(xp+xq)+yp+yq+xp,t+xq+1,t+xq,0);         // 0   xp+0     (X)  ((X+1)*(xp+1)+Y)
			xq*=xq; yq*=yq;
			t=xp;  xp=sqrt(xp); yp=sqrt(yp);
			u1.set(t*(xp+xq)+yp+yq+xp,t+xq+1,t+xq,0);
			xq*=xq; yq*=yq;
			u=mul(u0,u1);
			miller*=u;
		}
	}

	if (imod4==3)
    {                                                      //              (X=1)                                                        //              (Y=1)
		t=xp;                                                 // 0            (X+1)
		f.set(t*(xq+xp+1)+yq+yp+B+1,t+xq+1,t+xq,0);           // 1            (Y)

		miller=1;
		for (i=0;i<(m-3)/2;i+=2)
		{
			t=xp+1; xp=sqrt(xp); yp=sqrt(yp);                 // 1            (X)
			u0.set(t*(xp+xq+1)+yp+yq+1,t+xq+1,t+xq,0);        // 1   1        (X) ((X+1)*(xp+1)+Y)
			xq*=xq; yq*=yq;

			t=xp+1; xp=sqrt(xp); yp=sqrt(yp);
			u1.set(t*(xp+xq+1)+yp+yq+1,t+xq+1,t+xq,0);
			xq*=xq; yq*=yq;

			u=mul(u0,u1);
			miller*=u;
		}
// final step

		t=xp+1; xp=sqrt(xp); yp=sqrt(yp);
		u.set(t*(xp+xq+1)+yp+yq+1,t+xq+1,t+xq,0);
		miller*=u;
	}

    miller*=f;
	mill.g=miller;
	return mill;
}

GT PFC::final_exp(const GT& z)
{
	int i,m,TYPE;
	GT res;
	GF2m4x r,u,v,w,y;
// raising to the power (2^m-2^[m+1)/2]+1)(2^[(m+1)/2]+1)(2^(2m)-1) or (2^m+2^[(m+1)/2]+1)(2^[(m+1)/2]-1)(2^(2m)-1)
// 6 Frobenius, 4 big field muls...
	y=z.g;
	
	if (B==0)
	{
		if (M%8==1 || M%8==7) TYPE=1;
		else				  TYPE=2;
	}
	if (B==1)
	{
		if (M%8==3 || M%8==5) TYPE=1;
		else				  TYPE=2;
	}

    u=v=w=y;
    for (i=0;i<(M+1)/2;i++) u*=u;

	if (TYPE==1)   
	{
		u.powq();
		w.powq();
		v=w;
		w.powq();
		r=w;
		w.powq();
		w*=u;
		w*=y;
		r*=v;
		u.powq();
		u.powq();
		r*=u;
	}
	else
	{
		u.powq();
		v.powq();
		w=u*v;
		v.powq();
		w*=v;
		v.powq();
		u.powq();
		u.powq();
		r=v*u;
		r*=y;
	}

    r/=w;
	res.g=r;
    return res;            
}

// initialise Pairing Friendly Curve

PFC::PFC(int s, csprng *rng)
{
	int t,u,v,b,words,mod_bits;

	if (s!=80 && s!=128)
	{
		cout << "No suitable curve available" << endl;
		exit(0);
	}

	if (s==80)  mod_bits=379;
	if (s==128) mod_bits=1223;

	words=(mod_bits/MIRACL)+1;

#ifdef MR_SIMPLE_BASE
	miracl *mip=mirsys((MIRACL/4)*words,16);
#else
	miracl *mip=mirsys(words,0); 
	mip->IOBASE=16;
#endif

	ord=new Big;

	S=s;
	M=mod_bits;
	if (s==80)
	{
		t=253; u=251; v=59; B=1; CF=1; 
		*ord=pow((Big)2,M)+pow((Big)2,(M+1)/2)+1; //TYPE=1
	}
	if (s==128)
	{
		t=255; u=0; v=0; B=0; CF=5;
		*ord=pow((Big)2,M)+pow((Big)2,(M+1)/2)+1; //TYPE=1
	}
	*ord/=CF;
	
#ifdef MR_AFFINE_ONLY
	ecurve2(-M,t,u,v,(Big)1,(Big)B,TRUE,MR_AFFINE);
#else
	ecurve2(-M,t,u,v,(Big)1,(Big)B,TRUE,MR_PROJECTIVE);
#endif

	RNG=rng;
}

G1 PFC::mult(const G1& w,const Big& k)
{
	G1 z=w;
	z.g*=k;
	return z;
}

GT PFC::power(const GT& w,const Big& k)
{
	GT z;
	z.g=powu(w.g,k);
	return z;
}

// Precomputation not used here

//
// Spill precomputation on pairing to byte array
//

int PFC::spill(G1& w,char *& bytes)
{
	return 0;
}

//
// Restore precomputation on pairing to byte array
//

void PFC::restore(char * bytes,G1& w)
{
}

//
// spill precomputation on GT to byte array
//

int GT::spill(char *& bytes)
{
	return 0;
}

//
// restore precomputation for GT from byte array
//

void GT::restore(char *bytes)
{
}

//
// spill precomputation on G1 to byte array
//

int G1::spill(char *& bytes)
{
	return 0;
}

//
// restore precomputation for G1 from byte array
//

void G1::restore(char *bytes)
{
}

void PFC::hash_and_map(G1& w,char *ID)
{
    Big x0=H1(ID);
    while (!w.g.set(x0,x0)) x0+=1;
	w.g*=CF;
}

void PFC::random(G1& w)
{
	Big x0;
	if (RNG==NULL) x0=rand(M,2);
	else x0=strong_rand(RNG,M,2);

	while (!w.g.set(x0,x0)) x0+=1;
	w.g*=CF;
}

Big PFC::hash_to_aes_key(const GT& w)
{
	Big m=pow((Big)2,S);
	return H2(w.g)%m;
}

// hash to group multiplier

Big PFC::hash_to_group(char *ID)
{
	Big m=H1(ID);
	Big o=pow((Big)2,2*S);
	return m%o;
}

GT operator*(const GT& x,const GT& y)
{
	GT z=x;
	z.g*=y.g;
	return z; 
}

// elements in GT are unitary

GT operator/(const GT& x,const GT& y)
{
	GT z=x;
	z.g*=conj(y.g);
	return z; 
}

G1 operator+(const G1& x,const G1& y)
{
	G1 z=x;
	z.g+=y.g;
	return z;
}

G1 operator-(const G1& x)
{
	G1 z=x;
	z.g=-z.g;
	return z;
}

// Fast PFC group membership check

BOOL PFC::member(const GT &z)
{
	GF2m4x w=z.g;
	GF2m4x r=z.g;
	if (pow(r,CF)==1) return FALSE;
	w.powq();
	if (r*w==pow(r,pow((Big)2,(M+1)/2))) return TRUE;
	return FALSE;
}

GT PFC::pairing(const G1& x,const G1& y)
{
	GT z;
	z=miller_loop(x,y);
	z=final_exp(z);
	return z;
}

GT PFC::multi_pairing(int n,G1 **y,G1 **x)
{
	int i;
	GT z=1;
	for (i=0;i<n;i++)
		z=z*miller_loop(*y[i],*x[i]);
	z=final_exp(z);
	return z;

}

int PFC::precomp_for_mult(G1& w,BOOL small)
{
	return 0;
}

int PFC::precomp_for_power(GT& w,BOOL small)
{
	return 0;
}