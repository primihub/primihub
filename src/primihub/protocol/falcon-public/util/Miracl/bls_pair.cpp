
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
 * bls_pair.cpp
 *
 * BLS curve, ate pairing embedding degree 24, ideal for security level AES-256
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
 * G1 is a point over the base field, and G2 is a point over an extension field of degree 3
 * GT is a finite field point over the 18-th extension, where 18 is the embedding degree.
 *
 */

#define MR_PAIRING_BLS
#include "pairing_3.h"

// BLS curve
//static char param[]= "E000000000058400";
//static char curveB[]="6";

//Better BLS curve

static char param[]= "8010000A00000000"; 
static char curveB[]="A";

void read_only_error(void)
{
	cout << "Attempt to write to read-only object" << endl; 
	exit(0);
}

// Suitable for p=7 mod 12

void set_frobenius_constant(ZZn2 &X)
{
    Big p=get_modulus();
	X.set((Big)1,(Big)1);  // p=3 mod 8
	X=pow(X,(p-7)/12); 
}

ZZn24 Frobenius(ZZn24 P, ZZn2 &X, int k)
{
  ZZn24 Q=P;
  for (int i=0; i<k; i++)
    Q.powq(X);
  return Q;
}

// Using SHA256 as basic hash algorithm
//
// Hash function
// 

#define HASH_LEN 32

Big H1(char *string)
{ // Hash a zero-terminated string to a number < modulus
    Big h,p;
    char s[HASH_LEN];
    int i,j; 
    sha256 sh;

    shs256_init(&sh);

    for (i=0;;i++)
    {
        if (string[i]==0) break;
        shs256_process(&sh,string[i]);
    }
    shs256_hash(&sh,s);
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

void PFC::add_to_hash(const GT& x)
{
	ZZn8 u;
	ZZn24 v=x.g;
	ZZn4 h,l;
	ZZn2 t,b;
	Big a;
	ZZn xx[8];

	int i,j,m;
	v.get(u);
	u.get(l,h);
	l.get(t,b);
	t.get(xx[0],xx[1]);
	b.get(xx[2],xx[3]);
	h.get(t,b);
	t.get(xx[4],xx[5]);
	b.get(xx[6],xx[7]);
    for (i=0;i<8;i++)
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
	ZZn4 X,Y;
	ECn4 v=x.g;
	Big a;
	ZZn2 t,b;
	ZZn xx[8];

	int i,m;

	v.get(X,Y);
	X.get(t,b);
	t.get(xx[0],xx[1]);
	b.get(xx[2],xx[3]);
	Y.get(t,b);
	t.get(xx[4],xx[5]);
	b.get(xx[6],xx[7]);

	for (i=0;i<8;i++)
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

Big H2(ZZn24 x)
{ // Compress and hash an Fp24 to a big number
    sha256 sh;
    ZZn8 u;
    ZZn4 h,l;
	ZZn2 t,b;
    Big a,hash,p;
	ZZn xx[8];
    char s[HASH_LEN];
    int i,j,m;

    shs256_init(&sh);
    x.get(u);  // compress to single ZZn4
    u.get(l,h);

	l.get(t,b);

	t.get(xx[0],xx[1]);
	b.get(xx[2],xx[3]);
    
	h.get(t,b);

	t.get(xx[4],xx[5]);
	b.get(xx[6],xx[7]);
 
    for (i=0;i<8;i++)
    {
        a=(Big)xx[i];
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

//
// This calculates p.A = (X^p,Y^p) quickly using Frobenius
// 1. Extract A(x,y) from twisted curve to point on curve over full extension, as X=i^2.x and Y=i^3.y
// where i=NR^(1/k)
// 2. Using Frobenius calculate (X^p,Y^p)
// 3. map back to twisted curve
// Here we simplify things by doing whole calculation on the twisted curve
//
// Note we have to be careful as in detail it depends on w where p=w mod k
// Its simplest if w=1.
//

ECn4 psi(ECn4 &A,ZZn2 &F,int n)
{ 
	int i;
	ECn4 R;
	ZZn4 X,Y;
	ZZn2 FF,W;
// Fast multiplication of A by q^n
    A.get(X,Y);
	FF=F*F;
	W=txx(txx(txx(FF*FF*FF)));

	for (i=0;i<n;i++)
	{ // assumes p=7 mod 12
		X.powq(W); X=tx(tx(FF*X));
		Y.powq(W); Y=tx(tx(tx(FF*F*Y)));
	}
	
    R.set(X,Y);
	return R;
}

//
// Line from A to destination C. Let A=(x,y)
// Line Y-slope.X-c=0, through A, so intercept c=y-slope.x
// Line Y-slope.X-y+slope.x = (Y-y)-slope.(X-x) = 0
// Now evaluate at Q -> return (Qy-y)-slope.(Qx-x)
//

ZZn24 line(ECn4& A,ECn4& C,ZZn4& slope,ZZn& Qx,ZZn& Qy)
{
     ZZn24 w;
     ZZn8 nn,dd;
     ZZn4 X,Y;

     A.get(X,Y);
	
	 nn.set((ZZn4)Qy,Y-slope*X);
	 dd.set(slope*Qx);
	 w.set(nn,dd);

     return w;
}

//
// Add A=A+B  (or A=A+A) 
// Return line function value
//

ZZn24 g(ECn4& A,ECn4& B,ZZn& Qx,ZZn& Qy)
{
    ZZn4 lam;
    ZZn24 r;
    ECn4 P=A;

// Evaluate line from A
    A.add(B,lam);
    if (A.iszero())   return (ZZn24)1; 
    r=line(P,A,lam,Qx,Qy);

    return r;
}

// if multiples of G2 can be precalculated, its a lot faster!

ZZn24 gp(ZZn4* ptable,int &j,ZZn& Px,ZZn& Py)
{
	ZZn24 w;
	ZZn8 nn,dd;
	
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
	
	Big n=*x;
	if (w.ptable==NULL) return 0;
	ZZn2 a,b; 
	Big X,Y;

	m=2*(bits(n)+ham(n)-2);
	len=m*4*bytes_per_big;

	bytes=new char[len];
	for (i=j=0;i<m;i++)
	{
		w.ptable[i].get(a,b);
		a.get(X,Y);
		to_binary(X,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(Y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		b.get(X,Y);
		to_binary(X,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(Y,bytes_per_big,&bytes[j],TRUE);
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
	
	Big n=*x;
	if (w.ptable!=NULL) return;
	ZZn2 a,b;
	Big X,Y;

	m=2*(bits(n)+ham(n)-2);
	len=m*4*bytes_per_big;

	w.ptable=new ZZn4[m];
	for (i=j=0;i<m;i++)
	{
		X=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		Y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		a.set(X,Y);
		X=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		Y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		b.set(X,Y);
		w.ptable[i].set(a,b);
	}
	for (i=0;i<len;i++) bytes[i]=0;
	
	delete [] bytes;
}

// precompute G2 table for pairing

int PFC::precomp_for_pairing(G2& w)
{
	int i,j,nb,len;
	ECn4 A,Q,B;
	ZZn4 lam,x1,y1;
	Big n;
	Big X=*x;
	
	A=w.g;
	B=A; 
	n=X;
	nb=bits(n);
	j=0;
	len=2*(nb+ham(n)-2);
	w.ptable=new ZZn4[len];

	for (i=nb-2;i>=0;i--)
    {
		Q=A;
// Evaluate line from A to A+A
		A.add(A,lam);
		Q.get(x1,y1); 
		w.ptable[j++]=lam; w.ptable[j++]=y1-lam*x1;

		if (bit(n,i)==1)
		{
			Q=A;
			A.add(B,lam);
			Q.get(x1,y1);
			w.ptable[j++]=lam; w.ptable[j++]=y1-lam*x1;
		}
    }
	return len;
}

GT PFC::multi_miller(int n,G2** QQ,G1** PP)
{
	GT z;
    ZZn *Px,*Py;
	int i,j,*k,nb;
    ECn4 *Q,*A;
	ECn P;
    ZZn24 res;
	Big X=*x;

	Px=new ZZn[n];
	Py=new ZZn[n];
	Q=new ECn4[n];
	A=new ECn4[n];
	k=new int[n];

    nb=bits(X);
	res=1;  

	for (j=0;j<n;j++)
	{
		k[j]=0;
		P=PP[j]->g; normalise(P); Q[j]=QQ[j]->g; 
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
		if (bit(X,i)==1)
			for (j=0;j<n;j++) 
			{
				if (QQ[j]->ptable==NULL)
					res*=g(A[j],Q[j],Px[j],Py[j]);
				else
					res*=gp(QQ[j]->ptable,k[j],Px[j],Py[j]);
			}
		if (res.iszero()) return 0;  
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
    ECn4 A,Q;
	ECn P;
	ZZn Px,Py;
	BOOL precomp;
    ZZn24 r;
	Big X=*x;

	Q=QQ.g; P=PP.g;

	precomp=FALSE;
	if (QQ.ptable!=NULL) precomp=TRUE;

	normalise(P);
	extract(P,Px,Py);

	n=X;

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

    z.g=r;
	return z;
}
// Automatically generated by Luis Dominguez

ZZn24 HardExpo(ZZn24 &f3x0, ZZn2 &X, Big &x){
//vector=[ 1, 2, 3 ]
  ZZn24 r;
  ZZn24 xA;
  ZZn24 xB;
  ZZn24 t0;
  ZZn24 t1;
  ZZn24 f3x1;
  ZZn24 f3x2;
  ZZn24 f3x3;
  ZZn24 f3x4;
  ZZn24 f3x5;
  ZZn24 f3x6;
  ZZn24 f3x7;
  ZZn24 f3x8;
  ZZn24 f3x9;

  f3x1=pow(f3x0,x);
  f3x2=pow(f3x1,x);
  f3x3=pow(f3x2,x);
  f3x4=pow(f3x3,x);
  f3x5=pow(f3x4,x);
  f3x6=pow(f3x5,x);
  f3x7=pow(f3x6,x);
  f3x8=pow(f3x7,x);
  f3x9=pow(f3x8,x);

  xA=f3x4*inverse(f3x8)*Frobenius(f3x3,X,1)*Frobenius(inverse(f3x7),X,1)*Frobenius(f3x2,X,2)*Frobenius(inverse(f3x6),X,2)*Frobenius(f3x1,X,3)*Frobenius(inverse(f3x5),X,3)*Frobenius(inverse(f3x4),X,4)*Frobenius(inverse(f3x3),X,5)*Frobenius(inverse(f3x2),X,6)*Frobenius(inverse(f3x1),X,7);
  xB=f3x0;
  t0=xA*xB;
  xA=inverse(f3x3)*inverse(f3x5)*f3x7*f3x9*Frobenius(inverse(f3x2),X,1)*Frobenius(inverse(f3x4),X,1)*Frobenius(f3x6,X,1)*Frobenius(f3x8,X,1)*Frobenius(inverse(f3x1),X,2)*Frobenius(inverse(f3x3),X,2)*Frobenius(f3x5,X,2)*Frobenius(f3x7,X,2)*Frobenius(inverse(f3x0),X,3)*Frobenius(inverse(f3x2),X,3)*Frobenius(f3x4,X,3)*Frobenius(f3x6,X,3)*Frobenius(f3x3,X,4)*Frobenius(f3x5,X,4)*Frobenius(f3x2,X,5)*Frobenius(f3x4,X,5)*Frobenius(f3x1,X,6)*Frobenius(f3x3,X,6)*Frobenius(f3x0,X,7)*Frobenius(f3x2,X,7);
  xB=f3x0;
  t1=xA*xB;
  t0=t0*t0;
  t0=t0*t1;

r=t0;
return r;

}

void SoftExpo(ZZn24 &f3, ZZn2 &X){
  ZZn24 t0;
  int i;

  t0=f3; f3.conj(); f3/=t0;
  f3.mark_as_regular();
  t0=f3; for (i=1;i<=4;i++)  f3.powq(X); f3*=t0;
  f3.mark_as_unitary();
}

GT PFC::final_exp(const GT& z)
{
	GT y;
	ZZn24 r=z.g;
	Big X=*x;

	SoftExpo(r,*frob);
	y.g=HardExpo(r,*frob,X);
	
    return y;
}

PFC::PFC(int s, csprng *rng)
{
	int mod_bits,words;
	if (s!=256)
	{
		cout << "No suitable curve available" << endl;
		exit(0);
	}

	mod_bits=(5*s)/2;

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

	S=s;

	B=new Big;
	x=new Big;
	mod=new Big;
	ord=new Big;
	cof=new Big;
	npoints=new Big;
	trace=new Big;
	Beta=new ZZn;
	frob=new ZZn2;

	*B=curveB;
	*x=param;
	Big X=*x;

	*trace=1+X;
	*mod=(1+X+X*X-pow(X,4)+2*pow(X,5)-pow(X,6)+pow(X,8)-2*pow(X,9)+pow(X,10))/3;
	*ord=1-pow(X,4)+pow(X,8);
    *npoints=*mod+1-*trace;
	*cof=(X-1);  // Neat trick! Whole group is non-cyclic - just has (x-1)^2 as a factor
	                 // So multiplication by x-1 is sufficient to create a point of order q
	ecurve(0,*B,*mod,MR_PROJECTIVE);

	*Beta=pow((ZZn)2,(*mod-1)/3);
	*Beta*=(*Beta);   // right cube root of unity
	set_frobenius_constant(*frob);

    mip->TWIST=MR_SEXTIC_D;   // map Server to point on twisted curve E(Fp2)

	RNG=rng;
}

PFC::~PFC()
{
	delete B;
	delete x;
	delete mod;
	delete ord;
	delete cof;
	delete npoints;
	delete trace;
	delete Beta;
	delete frob;
	mirexit();
}

void endomorph(ECn &A,ZZn &Beta)
{ // apply endomorphism P(x,y) = (Beta*x,y) where Beta is cube root of unity
  // Actually (Beta*x,-y) =  x^4.P
	ZZn x,y;
	x=(A.get_point())->X;
	y=(A.get_point())->Y;
	y=-y;
	x*=Beta;
	copy(getbig(x),(A.get_point())->X);
	copy(getbig(y),(A.get_point())->Y);
}

G1 PFC::mult(const G1& w,const Big& k)
{
	G1 z;
	ECn Q;
	Big X=*x;
	if (w.mtable!=NULL)
	{ // we have precomputed values
		Big e=k;
		if (k<0) e=-e;

		int i,j,t=w.mtbits; // MR_ROUNDUP(2*S,WINDOW_SIZE); 
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
		Big x4,u[2],e=k;
		x4=X*X; x4*=x4;
		u[0]=e%x4; u[1]=e/x4;
		Q=w.g;
		endomorph(Q,*Beta);

		Q=mul(u[0],w.g,u[1],Q);
		z.g=Q;
	}
	return z;
}

// GLV + Galbraith-Scott

G2 PFC::mult(const G2& w,const Big& k)
{
	G2 z;
	int i,j;
	Big X=*x;

	if (w.mtable!=NULL)
	{ // we have precomputed values
		Big e=k;
		if (k<0) e=-e;

		int i,j,t=w.mtbits; //MR_ROUNDUP(2*S,WINDOW_SIZE); 
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
		ECn4 Q[8];
		Big u[8],e=k;
		BOOL small=TRUE;

		for (i=0;i<8;i++) {u[i]=e%X; e/=X;}
		Q[0]=w.g;

		for (i=1;i<8;i++)
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
			return z;
		}

		for (i=1;i<8;i++)
			Q[i]=psi(Q[i-1],*frob,1);

		for (i=0;i<8;i++)
		{
			if (u[i]<0)
				{u[i]=-u[i];Q[i]=-Q[i];}
		}

// simple multi-addition

		z.g=mul(8,Q,u);
	}
	return z;
}


// GLV method + Galbraith-Scott idea

GT PFC::power(const GT& w,const Big& k)
{
	GT z;
	Big X=*x;
	
	int i;
	if (w.etable!=NULL)
	{ // precomputation is available
		Big e=k;
		if (k<0) e=-e;

		int i,j,t=w.etbits; // MR_ROUNDUP(2*S,WINDOW_SIZE); 
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
		ZZn24 Y[8];
		Big u[8],e=k;

		for (i=0;i<8;i++) {u[i]=e%X; e/=X;}

		Y[0]=w.g;
		for (i=1;i<8;i++)
			{Y[i]=Y[i-1]; Y[i].powq(*frob);}

// deal with -ve exponents
		for (i=0;i<8;i++)
		{
			if (u[i]<0)
				{u[i]=-u[i];Y[i].conj();}
		}

// simple multi-exponentiation
	    z.g=pow(8,Y,u);
	}
	return z;
}

// Automatically generated by Luis Dominguez

ECn4 HashG2(ECn4& Qx0, Big& x, ZZn2& X){
//vector=[ 1, 2, 3, 4 ]
  ECn4 r;
  ECn4 xA;
  ECn4 xB;
  ECn4 xC;
  ECn4 t0;
  ECn4 t1;
  ECn4 Qx0_;
  ECn4 Qx1;
  ECn4 Qx1_;
  ECn4 Qx2;
  ECn4 Qx2_;
  ECn4 Qx3;
  ECn4 Qx3_;
  ECn4 Qx4;
  ECn4 Qx4_;
  ECn4 Qx5;
  ECn4 Qx5_;
  ECn4 Qx6;
  ECn4 Qx6_;
  ECn4 Qx7;
  ECn4 Qx7_;
  ECn4 Qx8;
  ECn4 Qx8_;

  Qx0_=-(Qx0);
  Qx1=x*Qx0;
  Qx1_=-(Qx1);
  Qx2=x*Qx1;
  Qx2_=-(Qx2);
  Qx3=x*Qx2;
  Qx3_=-(Qx3);
  Qx4=x*Qx3;
  Qx4_=-(Qx4);
  Qx5=x*Qx4;
  Qx5_=-(Qx5);
  Qx6=x*Qx5;
  Qx6_=-(Qx6);
  Qx7=x*Qx6;
  Qx7_=-(Qx7);
  Qx8=x*Qx7;
  Qx8_=-(Qx8);

  xA=Qx0;
  xC=Qx7;
  xA+=xC;
  xC=psi(Qx2,X,4);
  xA+=xC;
  xB=Qx0;
  xC=Qx7;
  xB+=xC;
  xC=psi(Qx2,X,4);
  xB+=xC;
  t0=xA+xB;
  xB=Qx2_;
  xC=Qx3_;
  xB+=xC;
  xC=Qx8_;
  xB+=xC;
  xC=psi(Qx2,X,1);
  xB+=xC;
  xC=psi(Qx3_,X,1);
  xB+=xC;
  xC=psi(Qx1,X,6);
  xB+=xC;
  t0=t0+xB;
  xB=Qx4;
  xC=Qx5_;
  xB+=xC;
  xC=psi(Qx0_,X,4);
  xB+=xC;
  xC=psi(Qx4_,X,4);
  xB+=xC;
  xC=psi(Qx0,X,5);
  xB+=xC;
  xC=psi(Qx1_,X,5);
  xB+=xC;
  xC=psi(Qx2_,X,5);
  xB+=xC;
  xC=psi(Qx3,X,5);
  xB+=xC;
  t0=t0+xB;
  xA=Qx1;
  xC=psi(Qx0_,X,1);
  xA+=xC;
  xC=psi(Qx1,X,1);
  xA+=xC;
  xC=psi(Qx4_,X,1);
  xA+=xC;
  xC=psi(Qx5,X,1);
  xA+=xC;
  xC=psi(Qx0,X,2);
  xA+=xC;
  xC=psi(Qx1_,X,2);
  xA+=xC;
  xC=psi(Qx4_,X,2);
  xA+=xC;
  xC=psi(Qx5,X,2);
  xA+=xC;
  xC=psi(Qx0,X,3);
  xA+=xC;
  xC=psi(Qx1_,X,3);
  xA+=xC;
  xC=psi(Qx4_,X,3);
  xA+=xC;
  xC=psi(Qx5,X,3);
  xA+=xC;
  xC=psi(Qx1,X,4);
  xA+=xC;
  xC=psi(Qx3,X,4);
  xA+=xC;
  xC=psi(Qx0_,X,6);
  xA+=xC;
  xC=psi(Qx2_,X,6);
  xA+=xC;
  xB=Qx4;
  xC=Qx5_;
  xB+=xC;
  xC=psi(Qx0_,X,4);
  xB+=xC;
  xC=psi(Qx4_,X,4);
  xB+=xC;
  xC=psi(Qx0,X,5);
  xB+=xC;
  xC=psi(Qx1_,X,5);
  xB+=xC;
  xC=psi(Qx2_,X,5);
  xB+=xC;
  xC=psi(Qx3,X,5);
  xB+=xC;
  t1=xA+xB;
  t0=t0+t0;
  t0=t0+t1;

r=t0;
return r;
}

// random group element

void PFC::random(Big& w)
{
	if (RNG==NULL) w=rand(*ord);
	else w=strong_rand(RNG,*ord);
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
    ZZn4 XX;
	ZZn2 t;
	Big X=*x;
 
    Big x0=H1(ID);
	
    forever
    {
        x0+=1;
		t.set((ZZn)0,(ZZn)x0);
        XX.set(t,(ZZn2)0);
        if (!w.g.set(XX)) continue;
        break;
    }

  	w.g=HashG2(w.g,X,*frob);
}  

void PFC::random(G2& w)
{
    int i;
    ECn4 S;
    ZZn4 XX;
	ZZn2 t;
	Big X=*x;
 
    Big x0;
	if (RNG==NULL) x0=rand(*mod);
	else x0=strong_rand(RNG,*mod);
	
    forever
    {
        x0+=1;
		t.set((ZZn)0,(ZZn)x0);
        XX.set(t,(ZZn2)0);
        if (!w.g.set(XX)) continue;
        break;
    }

    w.g=HashG2(w.g,X,*frob);

}  

void PFC::hash_and_map(G1& w,char *ID)
{
    Big x0=H1(ID);

    while (!w.g.set(x0,x0)) x0+=1;
	w.g*=*cof;
}

void PFC::random(G1& w)
{
	Big x0;
	if (RNG==NULL) x0=rand(*mod);
	else x0=strong_rand(RNG,*mod);

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
	int len=n*24*bytes_per_big;
	ZZn8 a,b,c;
	ZZn4 f,s;
	ZZn2 p,q;
	Big x,y;

	if (etable==NULL) return 0;

	bytes=new char[len];
	for (i=j=0;i<n;i++)
	{
		etable[i].get(a,b,c);
		a.get(f,s);
		f.get(p,q);
		p.get(x,y);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		q.get(x,y);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		s.get(p,q);
		p.get(x,y);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		q.get(x,y);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		b.get(f,s);
		f.get(p,q);
		p.get(x,y);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		q.get(x,y);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		s.get(p,q);
		p.get(x,y);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		q.get(x,y);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		c.get(f,s);
		f.get(p,q);
		p.get(x,y);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		q.get(x,y);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		s.get(p,q);
		p.get(x,y);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		q.get(x,y);
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
	int len=n*24*bytes_per_big;
	ZZn8 a,b,c;
	ZZn4 f,s;
	ZZn2 p,q;
	Big x,y;
	if (etable!=NULL) return;

	etable=new ZZn24[1<<WINDOW_SIZE];
	for (i=j=0;i<n;i++)
	{
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		p.set(x,y);
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		q.set(x,y);
		f.set(p,q);
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		p.set(x,y);
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		q.set(x,y);
		s.set(p,q);
		a.set(f,s);
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		p.set(x,y);
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		q.set(x,y);
		f.set(p,q);
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		p.set(x,y);
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		q.set(x,y);
		s.set(p,q);
		b.set(f,s);
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		p.set(x,y);
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		q.set(x,y);
		f.set(p,q);
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		p.set(x,y);
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		q.set(x,y);
		s.set(p,q);
		c.set(f,s);

		etable[i].set(a,b,c);
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

G2 operator+(const G2& x,const G2& y)
{
	G2 z=x;
	ECn4 t=y.g;
	z.g+=t;
	return z;
}

G2 operator-(const G2& x)
{
	G2 z=x;
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
	int len=n*8*bytes_per_big;
	ZZn4 a,b;
	ZZn2 f,s;
	Big x,y;

	if (mtable==NULL) return 0;

	bytes=new char[len];
	for (i=j=0;i<n;i++)
	{
		mtable[i].get(a,b);
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
	int len=n*8*bytes_per_big;
	ZZn4 a,b;
	ZZn2 f,s;
	Big x,y;
	if (mtable!=NULL) return;

	mtable=new ECn4[1<<WINDOW_SIZE];
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
		mtable[i].set(a,b);
	}
	delete [] bytes;
}


// Fast group membership check for GT
// check if r is of order q
// Test r^q=r^{(p+1-t)/cf}= 1
// so test r^p=r^x and r^cf !=1
// exploit cf=(x-1)*(x-1)/3

BOOL PFC::member(const GT &z)
{
	ZZn24 w=z.g;
	ZZn24 r=z.g;
	ZZn24 rx;
	Big X=*x;
	if (r*conj(r)!=(ZZn24)1) return FALSE; // not unitary
	w.powq(*frob);
	rx=pow(r,X);
	if (w!=rx) return FALSE;
	if (r*pow(rx,X)==rx*rx) return FALSE;
	return TRUE;
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
	ECn4 v=w.g;
	ZZn4 x,y;
	int i,j,k,bp,is,t;
	if (small) t=MR_ROUNDUP(2*S,WINDOW_SIZE);
	else       t=MR_ROUNDUP(bits(*ord),WINDOW_SIZE);
	w.mtable=new ECn4[1<<WINDOW_SIZE];
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
    }
	return (1<<WINDOW_SIZE);
}

int PFC::precomp_for_power(GT& w,BOOL small)
{
	ZZn24 v=w.g;
	int i,j,k,bp,is,t;
	if (small) t=MR_ROUNDUP(2*S,WINDOW_SIZE);
	else       t=MR_ROUNDUP(bits(*ord),WINDOW_SIZE);
	w.etable=new ZZn24[1<<WINDOW_SIZE];
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

