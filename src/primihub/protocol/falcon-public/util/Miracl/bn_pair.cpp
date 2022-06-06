
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
 *
 * bn_pair.cpp
 *
 * BN curve, ate pairing embedding degree 12, ideal for security level AES-128
 *
 *  Irreducible poly is X^3+n, where n=sqrt(w+sqrt(m)), m= {-1,-2} and w= {0,1,2}
 *         if p=5 mod 8, n=sqrt(-2)
 *         if p=3 mod 8, n=1+sqrt(-1)
 *         if p=7 mod 8, p=2,3 mod 5, n=2+sqrt(-1)
 *
 * Provides high level interface to pairing functions
 * 
 * GT=pairing(G2,G1)
 *
 * This is calculated on a Pairing Friendly Curve (PFC), which must first be defined.
 *
 * G1 is a point over the base field, and G2 is a point over an extension field of degree 2
 * GT is a finite field point over the 12-th extension, where 12 is the embedding degree.
 *
 */

#define MR_PAIRING_BN
#include "pairing_3.h"

// BN curve parameters x,A,B
static char param_128[]="-4080000000000001";
// 766 - bit curve
static char param_192[]="-4000000000000000000000000000000000000000000ABBB5";  // Hamming weight of 6*x+2 = 8
static char curveB[]="2";

void read_only_error(void)
{
	cout << "Attempt to write to read-only object" << endl; 
	exit(0);
}

void set_frobenius_constant(ZZn2 &X)
{
    Big p=get_modulus();
    switch (get_mip()->pmod8)
    {
    case 5:
         X.set((Big)0,(Big)1); // = (sqrt(-2)^(p-1)/2     
         break;
    case 3:                    // = (1+sqrt(-1))^(p-1)/2                                
         X.set((Big)1,(Big)1);      
         break;
   case 7: 
         X.set((Big)2,(Big)1); // = (2+sqrt(-1))^(p-1)/2
    default: break;
    }
    X=pow(X,(p-1)/6);
}

// Using SHA256 as basic hash algorithm
//
// Hash function
// 

#define HASH_LEN 32

Big H1(char *string)
{ // Hash a zero-terminated string to a number < modulus
    Big h,p;
    unsigned char s[HASH_LEN];
    int i,j; 
    sha256 sh;

    shs256_init(&sh);

    for (i=0;;i++)
    {
        if (string[i]==0) break;
        shs256_process(&sh,string[i]);
    }
    shs256_hash(&sh,(char *)s);
    p=get_modulus();
    h=1; j=0; i=1;
    forever
    {
        h*=256; 
        if (j==HASH_LEN)  {h+=i++; j=0;}
        else         h+=s[j++];
        if (h>=p) break;
    }
    h%=p;
    return h;
}

void PFC::start_hash(void)
{
	shs256_init(&SH);
}

Big PFC::finish_hash_to_group(void)
{
	Big hash;
	char s[HASH_LEN];
    shs256_hash(&SH,s);
    hash=from_binary(HASH_LEN,s);
	return hash%(*ord);
}

Big PFC::finish_hash_to_aes_key(void)
{
	Big hash;
	char s[HASH_LEN];
    shs256_hash(&SH,s);
	Big m=pow((Big)2,S);
    hash=from_binary(HASH_LEN,s);
	return hash%m;
}

void PFC::add_to_hash(const GT& x)
{
	ZZn4 u;
	ZZn12 v=x.g;
	ZZn2 h,l;
	Big a;
	ZZn xx[6];

	int i,j,m;
	v.get(u);
	u.get(l,h);
	l.get(xx[0],xx[1]);
	h.get(xx[2],xx[3]);

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

void PFC::add_to_hash(const G2& x)
{
	ZZn2 X,Y;
	ECn2 v=x.g;
	Big a;
	ZZn xx[4];

	int i,m;

	v.get(X,Y);
	X.get(xx[0],xx[1]);
	Y.get(xx[2],xx[3]);
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


Big H2(ZZn12 x)
{ // Compress and hash an Fp12 to a big number
    sha256 sh;
    ZZn4 u;
    ZZn2 h,l;
    Big a,hash,p,xx[4];
    char s[HASH_LEN];
    int i,j,m;

    shs256_init(&sh);
    x.get(u);  // compress to single ZZn4
    u.get(l,h);
    xx[0]=real(l); xx[1]=imaginary(l); xx[2]=real(h); xx[3]=imaginary(h);
 
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

#ifndef MR_AFFINE_ONLY

void force(ZZn& x,ZZn& y,ZZn& z,ECn& A)
{  // A=(x,y,z)
    copy(getbig(x),A.get_point()->X);
    copy(getbig(y),A.get_point()->Y);
    copy(getbig(z),A.get_point()->Z);
    A.get_point()->marker=MR_EPOINT_GENERAL;
}

void extract(ECn &A, ZZn& x,ZZn& y,ZZn& z)
{ // (x,y,z) <- A
    big t;
    x=(A.get_point())->X;
    y=(A.get_point())->Y;
    t=(A.get_point())->Z;
    if (A.get_status()!=MR_EPOINT_GENERAL) z=1;
    else                                   z=t;
}

#endif

void force(ZZn& x,ZZn& y,ECn& A)
{ // A=(x,y)
    copy(getbig(x),A.get_point()->X);
    copy(getbig(y),A.get_point()->Y);
    A.get_point()->marker=MR_EPOINT_NORMALIZED;
}

void extract(ECn& A,ZZn& x,ZZn& y)
{ // (x,y) <- A
    x=(A.get_point())->X;
    y=(A.get_point())->Y;
}


// Fast multiplication of A by q (for Trace-Zero group members only)
// Calculate q*P. P(X,Y) -> P(X^p,Y^p))

void q_power_frobenius(ECn2 &A,ZZn2 &F)
{ 
    ZZn2 x,y,z,w,r;

    A.get(x,y,z);
	w=F*F;
	r=F;
    x=w*conj(x);
    y=r*w*conj(y);
	z.conj();
    A.set(x,y,z);
}

//
// Line from A to destination C. Let A=(x,y)
// Line Y-slope.X-c=0, through A, so intercept c=y-slope.x
// Line Y-slope.X-y+slope.x = (Y-y)-slope.(X-x) = 0
// Now evaluate at Q -> return (Qy-y)-slope.(Qx-x)
//

ZZn12 line(ECn2& A,ECn2& C,ECn2& B,ZZn2& slope,ZZn2& extra,BOOL Doubling,ZZn& Qx,ZZn& Qy)
{
    ZZn12 w;
    ZZn4 nn,dd;
    ZZn2 X,Y;

    ZZn2 Z3;

    C.getZ(Z3);

// Thanks to A. Menezes for pointing out this optimization...
    if (Doubling)
    {
        ZZn2 Z,ZZ;
        A.get(X,Y,Z);
        ZZ=Z; ZZ*=ZZ;
		nn.set((Z3*ZZ)*Qy,slope*X-extra);
		dd.set(-(ZZ*slope)*Qx);        
    }
    else
    {
        ZZn2 X2,Y2;
        B.get(X2,Y2);
		nn.set(Z3*Qy,slope*X2-Y2*Z3);
		dd.set(-slope*Qx);       
    }
    w.set(nn,dd);

    return w;
}

//
// Add A=A+B  (or A=A+A) 
// Return line function value
//

ZZn12 g(ECn2& A,ECn2& B,ZZn& Qx,ZZn& Qy)
{
    ZZn2 lam,extra;
    ZZn12 r;
    ECn2 P=A;
    BOOL Doubling;

// Evaluate line from A
    Doubling=A.add(B,lam,extra);

    if (A.iszero())   return (ZZn12)1; 
    r=line(P,A,B,lam,extra,Doubling,Qx,Qy);

    return r;
}

// if multiples of G2 in e(G2,G1) can be precalculated, its a lot faster!

ZZn12 gp(ZZn2* ptable,int &j,ZZn& Px,ZZn& Py)
{
	ZZn12 w;
	ZZn4 nn,dd;
	
	nn.set(Py,ptable[j+1]);
	dd.set(ptable[j]*Px);
	j+=2;
	w.set(nn,dd);

	return w;
}

//
// Spill precomputation on pairing to byte array
//

int PFC::spill(G2& w,char *& bytes)
{
	int i,j,len,m;
	int bytes_per_big=(MIRACL/8)*(get_mip()->nib-1);
	
	Big a,b,n;
	Big X=*x;
	if (w.ptable==NULL) return 0;

	if (X<0) n=-(6*X+2);
    else n=6*X+2;

	m=2*(bits(n)+ham(n));
	len=m*2*bytes_per_big;

	bytes=new char[len];
	for (i=j=0;i<m;i++)
	{
		w.ptable[i].get(a,b);
		to_binary(a,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(b,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;

	}

	delete [] w.ptable; 
	w.ptable=NULL;
	return len;
}

//
// Restore precomputation on pairing to byte array
//

void PFC::restore(char * bytes,G2& w)
{
	int i,j,len,m;
	int bytes_per_big=(MIRACL/8)*(get_mip()->nib-1);
	
	Big a,b,n;
	Big X=*x;
	if (w.ptable!=NULL) return;

	if (X<0) n=-(6*X+2);
    else n=6*X+2;

	m=2*(bits(n)+ham(n));  // number of entries in ptable
	len=m*2*bytes_per_big;

	w.ptable=new ZZn2[m];
	for (i=j=0;i<m;i++)
	{
		a=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		b=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		w.ptable[i].set(a,b);
	}
	for (i=0;i<len;i++) bytes[i]=0;
	
	delete [] bytes;
}

// precompute G2 table for pairing

int PFC::precomp_for_pairing(G2& w)
{
	int i,j,nb,len;
	ECn2 A,Q,B,KA;
	ZZn2 lam,x1,y1;
	Big n;
	Big X=*x;
	
	A=w.g;
	A.norm();
	B=A; KA=A;
	if (X<0) n=-(6*X+2);
    else n=6*X+2;
	nb=bits(n);
	j=0;
	len=2*(nb+ham(n));    // **
	w.ptable=new ZZn2[len];
	get_mip()->coord=MR_AFFINE;  // switch to affine
    for (i=nb-2;i>=0;i--)
    {
		Q=A;
// Evaluate line from A to A+A
		A.add(A,lam);
		Q.get(x1,y1); 
		w.ptable[j++]=-lam; w.ptable[j++]=lam*x1-y1;
		if (bit(n,i)==1)
		{
			Q=A;
			A.add(B,lam);
			Q.get(x1,y1);
			w.ptable[j++]=-lam; w.ptable[j++]=lam*x1-y1;
		}
    }
	
	q_power_frobenius(KA,*frob);
	if (X<0) A=-A;
	Q=A;
	A.add(KA,lam);
	KA.get(x1,y1);
	w.ptable[j++]=-lam; w.ptable[j++]=lam*x1-y1; 

	q_power_frobenius(KA,*frob); KA=-KA;

	Q=A;
	A.add(KA,lam);
	KA.get(x1,y1);
	w.ptable[j++]=-lam; w.ptable[j++]=lam*x1-y1;

	get_mip()->coord=MR_PROJECTIVE;
	return len;
}

GT PFC::multi_miller(int n,G2** QQ,G1** PP)
{
	GT z;
    ZZn *Px,*Py;
	int i,j,*k,nb;
    ECn2 *Q,*A;
	ECn P;
    ZZn12 res;
	Big m;
	Big X=*x;

	Px=new ZZn[n];
	Py=new ZZn[n];
	Q=new ECn2[n];
	A=new ECn2[n];
	k=new int[n];

	if (X<0) m=-(6*X+2);
    else m=6*X+2;

    nb=bits(m);
	res=1;

	for (j=0;j<n;j++)
	{
		k[j]=0;
		P=PP[j]->g; normalise(P); Q[j]=QQ[j]->g; Q[j].norm();
		extract(P,Px[j],Py[j]);
	}

	for (j=0;j<n;j++) A[j]=Q[j];

	for (i=nb-2;i>=0;i--)
	{
		res*=res;
		for (j=0;j<n;j++)
		{
			if (QQ[j]->ptable==NULL)
				res*=g(A[j],A[j],Px[j],Py[j]);
			else
				res*=gp(QQ[j]->ptable,k[j],Px[j],Py[j]);
		}
		if (bit(m,i)==1)
			for (j=0;j<n;j++) 
			{
				if (QQ[j]->ptable==NULL)
					res*=g(A[j],Q[j],Px[j],Py[j]);
				else
					res*=gp(QQ[j]->ptable,k[j],Px[j],Py[j]);
			}
		if (res.iszero()) return 0;  
	}

	if (X<0) res.conj();
	for (j=0;j<n;j++) 
	{	
		q_power_frobenius(Q[j],*frob);
		
		if (QQ[j]->ptable==NULL)
		{
			if (X<0) A[j]=-A[j];
			res*=g(A[j],Q[j],Px[j],Py[j]);
		}
		else
			res*=gp(QQ[j]->ptable,k[j],Px[j],Py[j]);
		q_power_frobenius(Q[j],*frob); 
		
		if (QQ[j]->ptable==NULL)
		{
			Q[j]=-Q[j];
			res*=g(A[j],Q[j],Px[j],Py[j]);
		}
		else
			res*=gp(QQ[j]->ptable,k[j],Px[j],Py[j]);
	}

	delete [] k;
	delete [] A;
	delete [] Q;
	delete [] Py;
	delete [] Px;

	z.g=res;
	return z;
}

//
// R-ate Pairing G2 x G1 -> GT
//
// P is a point of order q in G1. Q(x,y) is a point of order q in G2. 
// Note that Q is a point on the sextic twist of the curve over Fp^2, P(x,y) is a point on the 
// curve over the base field Fp
//

GT PFC::miller_loop(const G2& QQ,const G1& PP)
{ 
	GT z;
	Big n;
	int i,j,nb,nbw,nzs;
    ECn2 A,KA,Q;
	ECn P;
	ZZn Px,Py;
	BOOL precomp;
    ZZn12 r;
	Big X=*x;

	Q=QQ.g; P=PP.g;

	precomp=FALSE;
	if (QQ.ptable!=NULL) precomp=TRUE;
	else Q.norm();

	normalise(P);
	extract(P,Px,Py);

	if (X<0) n=-(6*X+2);
    else n=6*X+2;
    A=Q;
    nb=bits(n);
    r=1;
// Short Miller loop
	r.mark_as_miller();
	j=0;
    for (i=nb-2;i>=0;i--)
    {
		r*=r;
		if (precomp) r*=gp(QQ.ptable,j,Px,Py);
		else         r*=g(A,A,Px,Py);
		if (bit(n,i))
		{
			if (precomp) r*=gp(QQ.ptable,j,Px,Py);
            else         r*=g(A,Q,Px,Py);
		}
    }
// Combining ideas due to Longa, Aranha et al. and Naehrig
	KA=Q;
	q_power_frobenius(KA,*frob);
	if (X<0) {A=-A; r.conj();}
	if (precomp) r*=gp(QQ.ptable,j,Px,Py);
	else         r*=g(A,KA,Px,Py);
	q_power_frobenius(KA,*frob); KA=-KA;
	if (precomp) r*=gp(QQ.ptable,j,Px,Py);
	else         r*=g(A,KA,Px,Py);

    z.g=r;
	return z;
}

GT PFC::final_exp(const GT& z)
{
	GT y;
	ZZn12 r,t0,t1;
	ZZn12 x0,x1,x2,x3,x4,x5;
	Big X=*x;

// The final exponentiation
	r=z.g;
    t0=r;

    r.conj();
    r/=t0;    // r^(p^6-1)
	r.mark_as_regular();  // no longer "miller"
    t0=r;
    r.powq(*frob); r.powq(*frob);
    r*=t0;    // r^[(p^6-1)*(p^2+1)]

    r.mark_as_unitary();  // from now on all inverses are just conjugates !! (and squarings are faster)

    t1=pow(r,-X);  // x is sparse..

    t0=r;    t0.powq(*frob);
    x0=t0;   x0.powq(*frob);

    x0*=(r*t0);
    x0.powq(*frob);
    x1=inverse(r);  // just a conjugation!
    x3=t1; x3.powq(*frob);
    x4=t1;
    t1=pow(t1,-X);
    x2=t1; x2.powq(*frob); 
    x4/=x2; 
    x2.powq(*frob);
    x5=inverse(t1);
    t0=pow(t1,-X);
    t1=t0; t1.powq(*frob); t0*=t1;

    t0*=t0;
    t0*=x4;
    t0*=x5;
    t1=x3*x5;
    t1*=t0;
    t0*=x2;
    t1*=t1;
    t1*=t0;
    t1*=t1;
    t0=t1*x1;
    t1*=x0;
    t0*=t0;
    t0*=t1;
	y.g=t0;
    return y;
}

PFC::PFC(int s, csprng *rng)
{
	int i,j,mod_bits,words;
	if (s!=128 && s!=192)
	{
		cout << "No suitable curve available" << endl;
		exit(0);
	}
	if (s==128)	mod_bits=256;
	if (s==192) mod_bits=768;

	if (mod_bits%MIRACL==0)
		words=(mod_bits/MIRACL);
	else
		words=(mod_bits/MIRACL)+1;

#ifdef MR_SIMPLE_BASE
	miracl *mip=mirsys((MIRACL/4)*words,16);
#else
	miracl *mip=mirsys(words,0); 
	mip->IOBASE=16;
#endif

	B=new Big;
	x=new Big;
	mod=new Big;
	ord=new Big;
	cof=new Big;
	npoints=new Big;
	trace=new Big;

	for (i=0;i<4;i++)
	{
		WB[i]=new Big;
		for (j=0;j<4;j++)
		{
			BB[i][j]=new Big;
		}
	}
	for (i=0;i<2;i++)
	{
		W[i]=new Big;
		for (j=0;j<2;j++)
		{
			SB[i][j]=new Big;
		}
	}

	Beta=new ZZn;
	frob=new ZZn2;

	Big A=0;
	*B=curveB;
	if (s==128)	*x=param_128;
	if (s==192) *x=param_192;
	S=s;

	Big X=*x;

    *mod=36*pow(X,4)+36*pow(X,3)+24*X*X+6*X+1;
    *trace=6*X*X+1;
    *npoints=*mod+1-*trace;
    *cof=*mod-1+*trace;
	*ord=*npoints;
	ecurve(A,*B,*mod,MR_PROJECTIVE);

//	Big Lambda=-(36*pow(x,3)+18*x*x+6*x+2);  // cube root of unity mod q
	*Beta=-(18*pow(X,3)+18*X*X+9*X+2);    // cube root of unity mod p
    set_frobenius_constant(*frob);

// Use standard Gallant-Lambert-Vanstone endomorphism method for G1
	*W[0]=6*X*X+4*X+1;      // This is first column of inverse of SB (without division by determinant) 
	*W[1]=-(2*X+1);
	
	*SB[0][0]=6*X*X+2*X;
	*SB[0][1]=-(2*X+1);
	*SB[1][0]=-(2*X+1);
	*SB[1][1]=-(6*X*X+4*X+1);

// Use Galbraith & Scott Homomorphism idea for G2 & GT ... (http://eprint.iacr.org/2008/117.pdf EXample 5)
	*WB[0]=2*X*X+3*X+1;     // This is first column of inverse of BB (without division by determinant)
	*WB[1]=12*X*X*X+8*X*X+X;
	*WB[2]=6*X*X*X+4*X*X+X;
	*WB[3]=-2*X*X-X;
	*BB[0][0]=X+1;   *BB[0][1]=X;     *BB[0][2]=X;        *BB[0][3]=-2*X;
	*BB[1][0]=2*X+1; *BB[1][1]=-X;    *BB[1][2]=-(X+1);   *BB[1][3]=-X;
	*BB[2][0]=2*X;   *BB[2][1]=2*X+1; *BB[2][2]=2*X+1;    *BB[2][3]=2*X+1;
	*BB[3][0]=X-1;   *BB[3][1]=4*X+2; *BB[3][2]=-(2*X-1); *BB[3][3]=X-1;
    mip->TWIST=MR_SEXTIC_D;   // map Server to point on twisted curve E(Fp2)
    
    RNG = rng;
}

PFC::~PFC()
{
	int i,j;
	delete B;
	delete x;
	delete mod;
	delete ord;
	delete cof;
	delete npoints;
	delete trace;

	for (i=0;i<4;i++)
	{
		delete WB[i];
		for (j=0;j<4;j++)
			delete BB[i][j];
	}
	for (i=0;i<2;i++)
	{
		delete W[i];
		for (j=0;j<2;j++)
			delete SB[i][j];
	}

	delete Beta;
	delete frob;
	mirexit();
}

// GLV method

void glv(const Big &e,Big &r,Big *W[2],Big *B[2][2],Big u[2])
{
	int i,j;
	Big v[2],w;
	for (i=0;i<2;i++)
	{
		v[i]=mad(*W[i],e,(Big)0,r,w);
		u[i]=0;
	}
	u[0]=e;
	for (i=0;i<2;i++)
		for (j=0;j<2;j++)
			u[i]-=v[j]*(*B[j][i]);
	return;
}

void endomorph(ECn &A,ZZn &Beta)
{ // apply endomorphism (x,y) = (Beta*x,y) where Beta is cube root of unity
	ZZn x;
	x=(A.get_point())->X;
	x*=Beta;
	copy(getbig(x),(A.get_point())->X);
}

G1 PFC::mult(const G1& w,const Big& k)
{
	G1 z;
	ECn Q;
	if (w.mtable!=NULL)
	{ // we have precomputed values
		Big e=k;
		if (k<0) e=-e;

		int i,j,t=w.mtbits;  //MR_ROUNDUP(2*S,WINDOW_SIZE); 
		j=recode(e,t,WINDOW_SIZE,t-1);
		z.g=w.mtable[j];
		for (i=t-2;i>=0;i--)
		{
			j=recode(e,t,WINDOW_SIZE,i);
			z.g+=z.g;
			if (j>0) z.g+=w.mtable[j];

		}
		if (k<0) z.g=-z.g;
	}
	else
	{
		Big u[2];
		Q=w.g;
		glv(k,*ord,W,SB,u);
		endomorph(Q,*Beta);
		Q=mul(u[0],w.g,u[1],Q);
		z.g=Q;
	}
	return z;
}

// Use Galbraith & Scott Homomorphism idea ...

void galscott(const Big &e,Big &r,Big *WB[4],Big *B[4][4],Big u[4])
{
	int i,j;
	Big v[4],w;

	for (i=0;i<4;i++)
	{
		v[i]=mad(*WB[i],e,(Big)0,r,w);
		u[i]=0;
	}

	u[0]=e;
	for (i=0;i<4;i++)
		for (j=0;j<4;j++)
			u[i]-=v[j]*(*B[j][i]);
	return;
}

// GLV + Galbraith-Scott

G2 PFC::mult(const G2& w,const Big& k)
{
	G2 z;
	int i;

	if (w.mtable!=NULL)
	{ // we have precomputed values
		Big e=k;
		if (k<0) e=-e;
		int i,j,t=w.mtbits;  //MR_ROUNDUP(2*S,WINDOW_SIZE); 
		j=recode(e,t,WINDOW_SIZE,t-1);
		z.g=w.mtable[j];
		for (i=t-2;i>=0;i--)
		{
			j=recode(e,t,WINDOW_SIZE,i);
			z.g+=z.g;
			if (j>0) z.g+=w.mtable[j];
		}
		if (k<0) z.g=-z.g;
	}
	else
	{
		ECn2 Q[4];
		Big u[4];
		BOOL small=TRUE;
		galscott(k,*ord,WB,BB,u);

		Q[0]=w.g; Q[0].norm();

		for (i=1;i<4;i++)
		{
			if (u[i]!=0)
			{
				small=FALSE;
				break;
			}
		}

		if (small)
		{
			if (u[0]<0)
			{
				u[0]=-u[0];
				Q[0]=-Q[0];
			}
			z.g=Q[0];
			z.g*=u[0];
			z.g.norm();
			return z;
		}

		for (i=1;i<4;i++)
		{
			Q[i]=Q[i-1]; 
			q_power_frobenius(Q[i],*frob);
		}

// deal with -ve multipliers
		for (i=0;i<4;i++)
		{
			if (u[i]<0)
				{u[i]=-u[i];Q[i]=-Q[i];}
		}

// simple multi-addition
		z.g= mul(4,Q,u);
	}
	z.g.norm();
	return z;
}

// GLV method + Galbraith-Scott idea

GT PFC::power(const GT& w,const Big& k)
{
	GT z;
	
	int i;
	if (w.etable!=NULL)
	{ // precomputation is available
		Big e=k;
		if (k<0) e=-e;

		int i,j,t=w.etbits;  // MR_ROUNDUP(2*S,WINDOW_SIZE); 
		j=recode(e,t,WINDOW_SIZE,t-1);
		z.g=w.etable[j];
		for (i=t-2;i>=0;i--)
		{
			j=recode(e,t,WINDOW_SIZE,i);
			z.g*=z.g;
			if (j>0) z.g*=w.etable[j];
		}
		if (k<0) z.g=inverse(z.g);
	}
	else
	{
		ZZn12 Y[4];
		Big u[4];

		galscott(k,*ord,WB,BB,u);

		Y[0]=w.g;
		for (i=1;i<4;i++)
			{Y[i]=Y[i-1]; Y[i].powq(*frob);}

// deal with -ve exponents
		for (i=0;i<4;i++)
		{
			if (u[i]<0)
				{u[i]=-u[i];Y[i].conj();}
		}

// simple multi-exponentiation
		z.g= pow(4,Y,u);
	}
	return z;
}

//
// Faster Hashing to G2 - Fuentes-Castaneda, Knapp and Rodriguez-Henriquez
//

void map(ECn2& S,Big &x,ZZn2 &F)
{
	ECn2 T,K;
	T=S;
	T*=x; // one multiplication by x only
	T.norm();
	K=(T+T);
	K+=T;
	K.norm();
	q_power_frobenius(K,F);
	q_power_frobenius(S,F); q_power_frobenius(S,F); q_power_frobenius(S,F); 
	S+=T; S+=K;
	q_power_frobenius(T,F); q_power_frobenius(T,F);
	S+=T;
	S.norm();
}

// random group element

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

void PFC::hash_and_map(G2& w,char *ID)
{
    int i;
    ZZn2 X;
 
    Big x0=H1(ID);

    forever
    {
        x0+=1;
        X.set((ZZn)1,(ZZn)x0);
        if (!w.g.set(X)) continue;
        break;
    }

	map(w.g,*x,*frob);
}

void PFC::random(G2& w)
{
    int i;
    ZZn2 X;
	Big x0;
 
	if (RNG==NULL) x0=rand(*mod);
    else x0=strong_rand(RNG, *mod);

    forever
    {
        x0+=1;
        X.set((ZZn)1,(ZZn)x0);
        if (!w.g.set(X)) continue;
        break;
    }

	map(w.g,*x,*frob);
}

void PFC::hash_and_map(G1& w,char *ID)
{
    Big x0=H1(ID);

    while (!w.g.set(x0,x0)) x0+=1;
}

void PFC::random(G1& w)
{
	Big x0;
	if (RNG==NULL) x0=rand(*mod);
	else x0=strong_rand(RNG, *mod);

	while (!w.g.set(x0,x0)) x0+=1;
}

Big PFC::hash_to_aes_key(const GT& w)
{
	Big m=pow((Big)2,S);
	return H2(w.g)%m;
}

Big PFC::hash_to_group(char *ID)
{
	Big m=H1(ID);
	return m%(*ord);
}

Big PFC::hash_to_group(char *buffer, int len)
{
    Big h,p;
    char s[HASH_LEN];
    int i,j; 
    sha256 sh;

    shs256_init(&sh);
    for (i=0; i < len; i++)
    {
        shs256_process(&sh,buffer[i]);
    }
    shs256_hash(&sh,s);

    p=get_modulus();
    h=1; j=0; i=1;
    forever
    {
        h*=256; 
        if (j==HASH_LEN)  {h+=i++; j=0;}
        else         h+=(unsigned char)s[j++];
        if (h>=p) break;
    }
    h%=p;
    return h % (*ord);
}

GT operator*(const GT& x,const GT& y)
{
	GT z=x;
	z.g*=y.g;
	return z; 
}

GT operator/(const GT& x,const GT& y)
{
	GT z=x;
	z.g/=y.g;
	return z; 
}

//
// spill precomputation on GT to byte array
//

int GT::spill(char *& bytes)
{
	int i,j,n=(1<<WINDOW_SIZE);
	int bytes_per_big=(MIRACL/8)*(get_mip()->nib-1);
	int len=n*12*bytes_per_big;
	ZZn4 a,b,c;
	ZZn2 f,s;
	Big x,y;

	if (etable==NULL) return 0;

	bytes=new char[len];
	for (i=j=0;i<n;i++)
	{
		etable[i].get(a,b,c);
		a.get(f,s);
		f.get(x,y);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		s.get(x,y);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		b.get(f,s);
		f.get(x,y);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		s.get(x,y);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		c.get(f,s);
		f.get(x,y);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		s.get(x,y);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;

	}
	delete [] etable; 
	etable=NULL;
	return len;
}

//
// restore precomputation for GT from byte array
//

void GT::restore(char *bytes)
{
	int i,j,n=(1<<WINDOW_SIZE);
	int bytes_per_big=(MIRACL/8)*(get_mip()->nib-1);
	int len=n*12*bytes_per_big;
	ZZn4 a,b,c;
	ZZn2 f,s;
	Big x,y;
	if (etable!=NULL) return;

	etable=new ZZn12[1<<WINDOW_SIZE];
	for (i=j=0;i<n;i++)
	{
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		f.set(x,y);
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		s.set(x,y);
		a.set(f,s);
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		f.set(x,y);
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		s.set(x,y);
		b.set(f,s);
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		f.set(x,y);
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		s.set(x,y);
		c.set(f,s);
		etable[i].set(a,b,c);
	}
	delete [] bytes;
}

//
// spill precomputation on G1 to byte array
//

int G1::spill(char *& bytes)
{
	int i,j,n=(1<<WINDOW_SIZE);
	int bytes_per_big=(MIRACL/8)*(get_mip()->nib-1);
	int len=n*2*bytes_per_big;
	Big x,y;

	if (mtable==NULL) return 0;

	bytes=new char[len];
	for (i=j=0;i<n;i++)
	{
		mtable[i].get(x,y);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
	}
	delete [] mtable; 
	mtable=NULL;
	return len;
}

//
// restore precomputation for G1 from byte array
//

void G1::restore(char *bytes)
{
	int i,j,n=(1<<WINDOW_SIZE);
	int bytes_per_big=(MIRACL/8)*(get_mip()->nib-1);
	int len=n*2*bytes_per_big;
	Big x,y;
	if (mtable!=NULL) return;

	mtable=new ECn[1<<WINDOW_SIZE];
	for (i=j=0;i<n;i++)
	{
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		mtable[i].set(x,y);
	}
	delete [] bytes;
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

//
// spill precomputation on G2 to byte array
//

int G2::spill(char *& bytes)
{
	int i,j,n=(1<<WINDOW_SIZE);
	int bytes_per_big=(MIRACL/8)*(get_mip()->nib-1);
	int len=n*4*bytes_per_big;
	ZZn2 a,b;
	Big x,y;

	if (mtable==NULL) return 0;

	bytes=new char[len];
	for (i=j=0;i<n;i++)
	{
		mtable[i].get(a,b);
		a.get(x,y);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		b.get(x,y);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
	}
	delete [] mtable; 
	mtable=NULL;
	return len;
}

//
// restore precomputation for G2 from byte array
//

void G2::restore(char *bytes)
{
	int i,j,n=(1<<WINDOW_SIZE);
	int bytes_per_big=(MIRACL/8)*(get_mip()->nib-1);
	int len=n*4*bytes_per_big;
	ZZn2 a,b;
	Big x,y;
	if (mtable!=NULL) return;

	mtable=new ECn2[1<<WINDOW_SIZE];
	for (i=j=0;i<n;i++)
	{
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		a.set(x,y);
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		b.set(x,y);
		mtable[i].set(a,b);
	}
	delete [] bytes;
}

G2 operator+(const G2& x,const G2& y)
{
	G2 z=x;
	ECn2 t=y.g;
	z.g+=t;
	return z;
}

G2 operator-(const G2& x)
{
	G2 z=x;
	z.g=-z.g;
	return z;
}

// test if a ZZn12 element is of order q
// test r^q = r^p+1-t =1, so test r^p=r^(t-1)

BOOL PFC::member(const GT& z)
{
	ZZn12 r=z.g;
	ZZn12 w=z.g;
	Big X=*x;
	if (!r.is_unitary()) return FALSE;
	if (r*conj(r)!=(ZZn12)1) return FALSE; // not unitary
	w.powq(*frob);
	r=pow(r,X); r=pow(r,X); r=pow(r,(Big)6); // t-1=6x^2
	if (w==r) return TRUE;
	return FALSE;
}

GT PFC::pairing(const G2& x,const G1& y)
{
	GT z;
	z=miller_loop(x,y);
	z=final_exp(z);
	return z;
}

GT PFC::multi_pairing(int n,G2 **y,G1 **x)
{
	GT z;
	z=multi_miller(n,y,x);
	z=final_exp(z);
	return z;

}

int PFC::precomp_for_mult(G1& w,BOOL small)
{
	ECn v=w.g;
	int i,j,k,bp,is,t;
	if (small) t=MR_ROUNDUP(2*S,WINDOW_SIZE);
	else       t=MR_ROUNDUP(bits(*ord),WINDOW_SIZE);
	w.mtable=new ECn[1<<WINDOW_SIZE];
	w.mtable[1]=v;
	w.mtbits=t;
	for (j=0;j<t;j++)
        v+=v;
    k=1;
    for (i=2;i<(1<<WINDOW_SIZE);i++)
    {
        if (i==(1<<k))
        {
            k++;
			normalise(v);
			w.mtable[i]=v;     
            for (j=0;j<t;j++)
				v+=v;
            continue;
        }
        bp=1;
        for (j=0;j<k;j++)
        {
            if (i&bp)
			{
				is=1<<j;
				w.mtable[i]+=w.mtable[is];
			}
            bp<<=1;
        }
        normalise(w.mtable[i]);
    }
	return (1<<WINDOW_SIZE);
}

int PFC::precomp_for_mult(G2& w,BOOL small)
{
	ECn2 v;
	ZZn2 x,y;
	int i,j,k,bp,is,t;
	if (small) t=MR_ROUNDUP(2*S,WINDOW_SIZE);
	else       t=MR_ROUNDUP(bits(*ord),WINDOW_SIZE);
	w.g.norm();
	v=w.g;
	w.mtable=new ECn2[1<<WINDOW_SIZE];
	w.mtable[1]=v;
	w.mtbits=t;
	for (j=0;j<t;j++)
        v+=v;
    k=1;
    for (i=2;i<(1<<WINDOW_SIZE);i++)
    {
        if (i==(1<<k))
        {
            k++;
			v.norm();
			w.mtable[i]=v;     
            for (j=0;j<t;j++)
				v+=v;
            continue;
        }
        bp=1;
        for (j=0;j<k;j++)
        {
            if (i&bp)
			{
				is=1<<j;
				w.mtable[i]+=w.mtable[is];
			}
            bp<<=1;
        }
        w.mtable[i].norm();
    }
	return (1<<WINDOW_SIZE);
}

int PFC::precomp_for_power(GT& w,BOOL small)
{
	ZZn12 v=w.g;
	int i,j,k,bp,is,t;
	if (small) t=MR_ROUNDUP(2*S,WINDOW_SIZE);
	else       t=MR_ROUNDUP(bits(*ord),WINDOW_SIZE);
	w.etable=new ZZn12[1<<WINDOW_SIZE];
	w.etable[0]=1;
	w.etable[1]=v;
	w.etbits=t;
	for (j=0;j<t;j++)
        v*=v;
    k=1;

    for (i=2;i<(1<<WINDOW_SIZE);i++)
    {
        if (i==(1<<k))
        {
            k++;
			w.etable[i]=v;     
            for (j=0;j<t;j++)
				v*=v;
            continue;
        }
        bp=1;
		w.etable[i]=1;
        for (j=0;j<k;j++)
        {
            if (i&bp)
			{
				is=1<<j;
				w.etable[i]*=w.etable[is];
			}
            bp<<=1;
        }
    }
	return (1<<WINDOW_SIZE);
}

