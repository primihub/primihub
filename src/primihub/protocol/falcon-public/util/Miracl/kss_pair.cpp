
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
 * kss_pair.cpp
 *
 * KSS curve, ate pairing embedding degree 18, ideal for security level AES-192
 *
 * Irreducible polynomial is of the form x^18+2
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

#define MR_PAIRING_KSS
#include "pairing_3.h"

// KSS curve parameters x,A,B
// irreducible poly is x^18+2
static char param[]= "15000000007004210";
static char curveB[]="2";

// Non-Residue. Irreducible Poly is binomial x^18-NR

#define NR -2

void read_only_error(void)
{
	cout << "Attempt to write to read-only object" << endl; 
	exit(0);
}

// Note - this representation depends on p-1=12 mod 18

void set_frobenius_constant(ZZn &X)
{ // Note X=NR^[(p-13)/18];
    Big p=get_modulus();
	X=pow((ZZn)NR,(p-13)/18);	
}

ZZn18 Frobenius(const ZZn18& W,ZZn& X,int n)
{
	int i;
	ZZn18 V=W;
	for (i=0;i<n;i++)
		V.powq(X);
	return V;
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
	ZZn6 u;
	ZZn18 v=x.g;
	ZZn3 h,l;
	Big a;
	ZZn xx[6];

	int i,j,m;
	v.get(u);
	u.get(l,h);
	l.get(xx[0],xx[1],xx[2]);
	h.get(xx[3],xx[4],xx[5]);

    for (i=0;i<6;i++)
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
	ZZn3 X,Y;
	ECn3 v=x.g;
	Big a;
	ZZn xx[6];

	int i,m;

	v.get(X,Y);
	X.get(xx[0],xx[1],xx[2]);
	Y.get(xx[3],xx[4],xx[5]);
	for (i=0;i<6;i++)
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

Big H2(ZZn18 x)
{ // Compress and hash an Fp18 to a big number
    sha256 sh;
    ZZn6 u;
    ZZn3 h,l;
    Big a,hash;
	ZZn xx[6];
    char s[HASH_LEN];
    int i,j,m;

    shs256_init(&sh);
    x.get(u);  // compress to single ZZn6
    u.get(l,h);
	l.get(xx[0],xx[1],xx[2]);
	h.get(xx[3],xx[4],xx[5]);
    
    for (i=0;i<6;i++)
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
// This calculates p.A quickly using Frobenius
// 1. Extract A(x,y) from twisted curve to point on curve over full extension, as X=i^2.x and Y=i^3.y
// where i=NR^(1/k)
// 2. Using Frobenius calculate (X^p,Y^p)
// 3. map back to twisted curve
// Here we simplify things by doing whole calculation on the twisted curve
//
// Note we have to be careful as in detail it depends on w where p=w mod k
// In this case w=13
//

ECn3 psi(ECn3 &A,ZZn &W,int n)
{ 
	int i;
	ECn3 R;
	ZZn3 X,Y;
	ZZn FF;
// Fast multiplication of A by q^n
    A.get(X,Y);

	FF=NR*W*W;
	for (i=0;i<n;i++)
	{ // assumes p=13 mod 18
		X.powq(); X=tx(FF*X);
		Y.powq(); Y*=(ZZn)get_mip()->sru;
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

ZZn18 line(ECn3& A,ECn3& C,ZZn3& slope,ZZn& Qx,ZZn& Qy)
{
     ZZn18 w;
     ZZn6 nn,dd;
     ZZn3 X,Y;

     A.get(X,Y);
	
	 nn.set(Qy,Y-slope*X);
	 dd.set(slope*Qx);
	 w.set(nn,dd);
//cout << "1. w= " << w << endl;
     return w;
}

//
// Add A=A+B  (or A=A+A) 
// Return line function value
//

ZZn18 g(ECn3& A,ECn3& B,ZZn& Qx,ZZn& Qy)
{
    ZZn3 lam;
    ZZn18 r;
    ECn3 P=A;

// Evaluate line from A
    A.add(B,lam);
    if (A.iszero())   return (ZZn18)1; 
    r=line(P,A,lam,Qx,Qy);

    return r;
}

// if multiples of G2 can be precalculated, its a lot faster!

ZZn18 gp(ZZn3* ptable,int &j,ZZn& Px,ZZn& Py)
{
	ZZn18 w;
	ZZn6 nn,dd;
	
	nn.set(Py,ptable[j+1]);
	dd.set(ptable[j]*Px);
	j+=2;
	w.set(nn,dd);
//cout << "2. w= " << w << endl;
	return w;
}

//
// Spill precomputation on pairing to byte array
//

int PFC::spill(G2& w,char *& bytes)
{
	int i,j,len,m;
	int bytes_per_big=(MIRACL/8)*(get_mip()->nib-1);
	
	Big n;
	Big X=*x;
	ZZn a,b,c;
	if (w.ptable==NULL) return 0;

	n=X/7;

	m=2*(bits(n)+ham(n)+1);
	len=m*3*bytes_per_big;

	bytes=new char[len];
	for (i=j=0;i<m;i++)
	{
		w.ptable[i].get(a,b,c);
		to_binary(a,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(b,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(c,bytes_per_big,&bytes[j],TRUE);
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
	
	Big n;
	Big X=*x;
	ZZn a,b,c;
	if (w.ptable!=NULL) return;

	n=X/7;

	m=2*(bits(n)+ham(n)+1);
	len=m*3*bytes_per_big;

	w.ptable=new ZZn3[m];
	for (i=j=0;i<m;i++)
	{
		a=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		b=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		c=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;

		w.ptable[i].set(a,b,c);
	}
	for (i=0;i<len;i++) bytes[i]=0;
	
	delete [] bytes;
}


// precompute G2 table for pairing

int PFC::precomp_for_pairing(G2& w)
{
	int i,j,nb,len;
	ECn3 A,m2A,dA,Q,B;
	ZZn3 lam,x1,y1;
	Big n;
	Big X=*x;
	
	A=w.g;
	B=A; 
	n=(X/7);
	nb=bits(n);
	j=0;
	len=2*(nb+ham(n)+1);
	w.ptable=new ZZn3[len];

	for (i=nb-2;i>=0;i--)
    {
		Q=A;
// Evaluate line from A to A+B
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
	dA=A;
	Q=A;
	A.add(A,lam);
	Q.get(x1,y1);
	w.ptable[j++]=lam; w.ptable[j++]=y1-lam*x1;

	m2A=A;

	Q=A;
	A.add(dA,lam);
	Q.get(x1,y1);
	w.ptable[j++]=lam; w.ptable[j++]=y1-lam*x1;

	A=psi(A,*frob,6);

	Q=A;
	A.add(m2A,lam);
	Q.get(x1,y1);
	w.ptable[j++]=lam; w.ptable[j++]=y1-lam*x1;

	return len;
}
	
GT PFC::multi_miller(int n,G2** QQ,G1** PP)
{
	GT z;
    ZZn *Px,*Py;
	int i,j,*k,nb;
    ECn3 *Q,*A,*A2;
	ECn P;
    ZZn18 res,rd;
	Big m;
	Big X=*x;

	Px=new ZZn[n];
	Py=new ZZn[n];
	Q=new ECn3[n];
	A=new ECn3[n];
	A2=new ECn3[n];
	k=new int[n];

	m=X/7;

    nb=bits(m);
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

	rd=res;
	res*=res;

	for (j=0;j<n;j++)
	{
		
		if (QQ[j]->ptable==NULL) 
		{
			Q[j]=A[j];
			res*=g(A[j],A[j],Px[j],Py[j]);
		}
		else res*=gp(QQ[j]->ptable,k[j],Px[j],Py[j]);
	}

	rd*=res;

	for (j=0;j<n;j++)
	{
		
		if (QQ[j]->ptable==NULL)
		{
			A2[j]=A[j];
			rd*=g(A[j],Q[j],Px[j],Py[j]);
		}
		else rd*=gp(QQ[j]->ptable,k[j],Px[j],Py[j]);
	}
	
	res*=Frobenius(rd,*frob,6);

	for (j=0;j<n;j++)
	{
		if (QQ[j]->ptable==NULL)
		{
			A[j]=psi(A[j],*frob,6);
			res*=g(A[j],A2[j],Px[j],Py[j]);
		}
		else 
			res*=gp(QQ[j]->ptable,k[j],Px[j],Py[j]);
	}

	delete [] k;
	delete [] A2;
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
// Note that Q is a point on the sextic twist of the curve over Fp^3, P(x,y) is a point on the 
// curve over the base field Fp
//

GT PFC::miller_loop(const G2& QQ,const G1& PP)
{ 
	GT z;
	Big n;
	int i,j,nb,nbw,nzs;
    ECn3 A,m2A,Q;
	ECn P;
	ZZn Px,Py;
	BOOL precomp;
    ZZn18 r,rd;
	Big X=*x;

	Q=QQ.g; P=PP.g;

	precomp=FALSE;
	if (QQ.ptable!=NULL) precomp=TRUE;

	normalise(P);
	extract(P,Px,Py);

	A=Q;
	n=(X/7);

	nb=bits(n);
	r=1; j=0;
	r.mark_as_miller();
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

	rd=r;
	r*=r;

	Q=A;
	if (precomp) r*=gp(QQ.ptable,j,Px,Py);
	else         r*=g(A,A,Px,Py);

	rd*=r;

	m2A=A;
	if (precomp) rd*=gp(QQ.ptable,j,Px,Py);
	else		 rd*=g(A,Q,Px,Py);

	r*=Frobenius(rd,*frob,6);
	if (precomp) r*=gp(QQ.ptable,j,Px,Py);
	else
	{
		A=psi(A,*frob,6);
		r*=g(A,m2A,Px,Py);
	}
	
	z.g=r;
	return z;
}

// Automatically generated by Luis Dominquez

ZZn18 HardExpo(ZZn18 &f3x0, ZZn &X, Big &x){
//vector=[ 3, 5, 7, 14, 15, 21, 25, 35, 49, 54, 62, 70, 87, 98, 112, 245, 273, 319, 343, 434, 450, 581, 609, 784, 931, 1407, 1911, 4802, 6517 ]
  ZZn18 xA;
  ZZn18 xB;
  ZZn18 t0;
  ZZn18 t1;
  ZZn18 t2;
  ZZn18 t3;
  ZZn18 t4;
  ZZn18 t5;
  ZZn18 t6;
  ZZn18 t7;
  ZZn18 f3x1;
  ZZn18 f3x2;
  ZZn18 f3x3;
  ZZn18 f3x4;
  ZZn18 f3x5;
  ZZn18 f3x6;
  ZZn18 f3x7;

  f3x1=pow(f3x0,x);
  f3x2=pow(f3x1,x);
  f3x3=pow(f3x2,x);
  f3x4=pow(f3x3,x);
  f3x5=pow(f3x4,x);
  f3x6=pow(f3x5,x);
  f3x7=pow(f3x6,x);

  xA=Frobenius(inverse(f3x1),X,2);
  xB=Frobenius(inverse(f3x0),X,2);
  t0=xA*xB;
  xB=Frobenius(inverse(f3x2),X,2);
  t1=t0*xB;
  t0=t0*t0;
  xB=Frobenius(inverse(f3x0),X,2);
  t0=t0*xB;
  xB=Frobenius(f3x1,X,1);
  t0=t0*xB;
  xA=Frobenius(inverse(f3x5),X,2)*Frobenius(f3x4,X,4)*Frobenius(f3x2,X,5);
  //xB=Frobenius(f3x1,X,1);
  t5=xA*xB;
  t0=t0*t0;
  t3=t0*t1;
  xA=Frobenius(inverse(f3x4),X,2)*Frobenius(f3x1,X,5);
  xB=Frobenius(f3x2,X,1);
  t1=xA*xB;
  xA=xB;//Frobenius(f3x2,X,1);
  xB=xA; //xB=Frobenius(f3x2,X,1);
  t0=xA*xB;
  xB=Frobenius(f3x2,X,4);
  t0=t0*xB;
  xB=Frobenius(f3x1,X,4);
  t2=t3*xB;
  xB=Frobenius(inverse(f3x1),X,2);
  t4=t3*xB;
  t2=t2*t2;
  xB=Frobenius(inverse(f3x2),X,3);
  t3=t0*xB;
  xB=inverse(f3x2);
  t0=t3*xB;
  t4=t3*t4;
  xB=Frobenius(inverse(f3x3),X,3);
  t0=t0*xB;
  t3=t0*t2;
  xB=Frobenius(inverse(f3x3),X,2)*Frobenius(f3x0,X,5);
  t2=t3*xB;
  t3=t3*t5;
  t5=t3*t2;
  xB=inverse(f3x3);
  t2=t2*xB;
  xA=Frobenius(inverse(f3x6),X,3);
  //xB=inverse(f3x3);
  t3=xA*xB;
  t2=t2*t2;
  t4=t2*t4;
  xB=Frobenius(f3x3,X,1);
  t2=t1*xB;
  xA=xB; //xA=Frobenius(f3x3,X,1);
  xB=Frobenius(inverse(f3x2),X,3);
  t1=xA*xB;
  t6=t2*t4;
  xB=Frobenius(f3x4,X,1);
  t4=t2*xB;
  xB=Frobenius(f3x3,X,4);
  t2=t6*xB;
  xB=Frobenius(inverse(f3x5),X,3)*Frobenius(f3x5,X,4);
  t7=t6*xB;
  t4=t2*t4;
  xB=Frobenius(f3x6,X,1);
  t2=t2*xB;
  t4=t4*t4;
  t4=t4*t5;
  xA=inverse(f3x4);
  xB=Frobenius(inverse(f3x4),X,3);
  t5=xA*xB;
//  xB=Frobenius(inverse(f3x4),X,3);
  t3=t3*xB;
  xA=Frobenius(f3x5,X,1);
  xB=xA; //xB=Frobenius(f3x5,X,1);
  t6=xA*xB;
  t7=t6*t7;
  xB=Frobenius(f3x0,X,3);
  t6=t5*xB;
  t4=t6*t4;
  xB=Frobenius(inverse(f3x7),X,3);
  t6=t6*xB;
  t0=t4*t0;
  xB=Frobenius(f3x6,X,4);
  t4=t4*xB;
  t0=t0*t0;
  xB=inverse(f3x5);
  t0=t0*xB;
  t1=t7*t1;
  t4=t4*t7;
  t1=t1*t1;
  t2=t1*t2;
  t1=t0*t3;
  xB=Frobenius(inverse(f3x3),X,3);
  t0=t1*xB;
  t1=t1*t6;
  t0=t0*t0;
  t0=t0*t5;
  xB=inverse(f3x6);
  t2=t2*xB;
  t2=t2*t2;
  t2=t2*t4;
  t0=t0*t0;
  t0=t0*t3;
  t1=t2*t1;
  t0=t1*t0;
//  xB=inverse(f3x6);
  t1=t1*xB;
  t0=t0*t0;
  t0=t0*t2;
  xB=f3x0*inverse(f3x7);
  t0=t0*xB;
//  xB=f3x0*inverse(f3x7);
  t1=t1*xB;
  t0=t0*t0;
  t0=t0*t1;

  return t0;
}

GT PFC::final_exp(const GT& z)
{
	GT y;
	ZZn18 rd,r=z.g;
	rd=r;
	Big X=*x;

	// final exponentiation
    r.conj();
    r/=rd;    // r^(p^9-1)
	r.mark_as_regular(); // no longer "miller"
	rd=r;
	r.powq(*frob); r.powq(*frob); r.powq(*frob); r*=rd; //r^(p^3+1)

	r.mark_as_unitary();
	r=HardExpo(r,*frob,X);

	y.g=r;
	return y;

}

PFC::PFC(int s, csprng *rng)
{
	int i,j,mod_bits,words;
	if (s!=192)
	{
		cout << "No suitable curve available" << endl;
		exit(0);
	}

	mod_bits=(8*s)/3;

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

	for (i=0;i<6;i++)
	{
		WB[i]=new Big;
		for (j=0;j<6;j++)
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

	S=s;

	Beta=new ZZn;
	frob=new ZZn;

	*B=curveB;
	*x=param;

	Big X=*x;

	*trace=(pow(X,4) + 16*X + 7)/7;
	*ord=(pow(X,6) + 37*pow(X,3) + 343)/343;
		
    *cof=(49*X*X+245*X+343)/3;
	*npoints=*cof*(*ord);
	*mod=*cof*(*ord)+*trace-1; 
	ecurve(0,*B,*mod,MR_PROJECTIVE);

	Big BBeta=(3*pow(X,7)-7*pow(X,6)+46*pow(X,5)+68*pow(X,4)-308*pow(X,3)+189*X*X+145*X-3192)/56;
	BBeta+=X*(pow(X,7)/28);
	BBeta/=3;

	Big sru=*mod-BBeta;  // sixth root of unity = -Beta	
	set_zzn3(NR,sru);
	*Beta=BBeta;
    set_frobenius_constant(*frob);

// Use standard Gallant-Lambert-Vanstone endomorphism method for G1
	
	*W[0]=(X*X*X)/343;        // This is first column of inverse of SB (without division by determinant) 
	*W[1]=(18*X*X*X+343)/343;
	
	*SB[0][0]=(X*X*X)/343;
	*SB[0][1]=-(18*X*X*X+343)/343;
	*SB[1][0]=(19*X*X*X+343)/343;
	*SB[1][1]=(X*X*X)/343;

// Use Galbraith & Scott Homomorphism idea for G2 & GT ... (http://eprint.iacr.org/2008/117.pdf)

	*WB[0]=5*pow(X,3)/49+2;   // This is first column of inverse of BB (without division by determinant) 
	*WB[1]=-(X*X)/49;
	*WB[2]=pow(X,4)/49+3*X/7;
	*WB[3]=-(17*pow(X,3)/343+1);
	*WB[4]=-(pow(X,5)/343+2*(X*X)/49);
	*WB[5]=5*pow(X,4)/343+2*X/7;

	*BB[0][0]=1;      *BB[0][1]=0;     *BB[0][2]=5*X/7; *BB[0][3]=1;   *BB[0][4]=0;   *BB[0][5]=-X/7; 
	*BB[1][0]=-5*X/7; *BB[1][1]=-2;    *BB[1][2]=0;     *BB[1][3]=X/7; *BB[1][4]=1;   *BB[1][5]=0; 
	*BB[2][0]=0;      *BB[2][1]=2*X/7; *BB[2][2]=1;     *BB[2][3]=0;   *BB[2][4]=X/7; *BB[2][5]=0; 
	*BB[3][0]=1;      *BB[3][1]=0;     *BB[3][2]=X;     *BB[3][3]=2;   *BB[3][4]=0;   *BB[3][5]=0; 
	*BB[4][0]=-X;     *BB[4][1]=-3;    *BB[4][2]=0;     *BB[4][3]=0;   *BB[4][4]=1;   *BB[4][5]=0; 
	*BB[5][0]=0;      *BB[5][1]=-X;    *BB[5][2]=-3;    *BB[5][3]=0;   *BB[5][4]=0;   *BB[5][5]=1;

    mip->TWIST=MR_SEXTIC_D;   // map Server to point on twisted curve E(Fp3)

	RNG=rng;
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

	for (i=0;i<6;i++)
	{
		delete WB[i];
		for (j=0;j<6;j++)
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

// Use Galbraith & Scott Homomorphism idea ...

void galscott(const Big &e,Big &r,Big *WB[6],Big *B[6][6],Big u[6])
{
	int i,j;
	Big v[6],w;

	for (i=0;i<6;i++)
	{
		v[i]=mad(*WB[i],e,(Big)0,r,w);
		u[i]=0;
	}

	u[0]=e;
	for (i=0;i<6;i++)
	{
		for (j=0;j<6;j++)
			u[i]-=v[j]*(*B[j][i]);
	}
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
		Big u[2];
		Q=w.g;
		glv(k,*ord,W,SB,u);
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
	int i;
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
		ECn3 Q[6];
		Big u[6];
		BOOL small=TRUE;
		galscott(k,*ord,WB,BB,u);

		Q[0]=w.g;

		for (i=1;i<6;i++)
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

		for (i=1;i<6;i++)
			Q[i]=psi(Q[i-1],*frob,1);

// deal with -ve multipliers
		for (i=0;i<6;i++)
		{
			if (u[i]<0)
				{u[i]=-u[i];Q[i]=-Q[i];}
		}

// simple multi-addition
		z.g= mul(6,Q,u);
	}
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
		ZZn18 Y[6];
		Big u[6];

		galscott(k,*ord,WB,BB,u);

		Y[0]=w.g;
		for (i=1;i<6;i++)
			{Y[i]=Y[i-1]; Y[i].powq(*frob);}

// deal with -ve exponents
		for (i=0;i<6;i++)
		{
			if (u[i]<0)
				{u[i]=-u[i];Y[i].conj();}
		}

// simple multi-exponentiation
		z.g= pow(6,Y,u);
	}
	return z;
}

// Faster Hashing to G2 - Fuentes-Castaneda, Knapp and Rodriguez-Henriquez

ECn3 HashG2(ECn3& Qx0,Big &x,ZZn&F)
{
	ECn3 Qx0_;
	ECn3 Qx1;
	ECn3 Qx1_;
	ECn3 Qx2;
	ECn3 Qx2_;
	ECn3 Qx3;
	ECn3 t1;
	ECn3 t2;
	ECn3 t3;
	ECn3 t4;
	ECn3 t5;
	ECn3 t6;

	Qx0_=-Qx0;
	Qx1=x*Qx0;
	Qx1_=-Qx1;
	Qx2=x*Qx1;
	Qx2_=-Qx2;
	Qx3=x*Qx2;

	t1=Qx0;
	t2=psi(Qx1_,F,2);
	t3=Qx1+psi(Qx1,F,5);
	t4=psi(Qx1,F,3)+psi(Qx2,F,1)+psi(Qx2_,F,2);
	t5=psi(Qx0_,F,4);
	t6=psi(Qx0,F,1)+psi(Qx0,F,3)+psi(Qx2_,F,4)+psi(Qx2,F,5)+psi(Qx3,F,1);

	t2+=t1;  // Olivos addition sequence
	t1+=t1;
	t1+=t3;
	t1+=t2;
	t4+=t2;
	t5+=t1;
	t4+=t1;
	t5+=t4;
	t4+=t6;
	t5+=t5;
	t5+=t4;

	return t5;
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
    ZZn3 XX;
	Big X=*x;
 
    Big x0=H1(ID);
    forever
    {
        x0+=1;
        XX.set((ZZn)0,(ZZn)x0,(ZZn)0);
        if (!w.g.set(XX)) continue;
        break;
    }
	w.g=HashG2(w.g,X,*frob);
}

void PFC::random(G2 &w)
{
    int i;
    ZZn3 XX;
	Big X=*x;
 
    Big x0;
	if (RNG==NULL) x0=rand(*mod);
	else x0=strong_rand(RNG,*mod);

    forever
    {
        x0+=1;
        XX.set((ZZn)0,(ZZn)x0,(ZZn)0);
        if (!w.g.set(X)) continue;
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

	w.g*=*cof;

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
	int len=n*18*bytes_per_big;
	ZZn6 a,b,c;
	ZZn3 f,s;
	ZZn x,y,z;

	if (etable==NULL) return 0;

	bytes=new char[len];
	for (i=j=0;i<n;i++)
	{
		etable[i].get(a,b,c);
		a.get(f,s);
		f.get(x,y,z);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(z,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		s.get(x,y,z);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(z,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		b.get(f,s);
		f.get(x,y,z);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(z,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;

		s.get(x,y,z);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(z,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;

		c.get(f,s);
		f.get(x,y,z);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(z,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;

		s.get(x,y,z);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(z,bytes_per_big,&bytes[j],TRUE);
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
	int len=n*18*bytes_per_big;
	ZZn6 a,b,c;
	ZZn3 f,s;
	ZZn x,y,z;
	if (etable!=NULL) return;

	etable=new ZZn18[1<<WINDOW_SIZE];
	for (i=j=0;i<n;i++)
	{
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		z=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		f.set(x,y,z);
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		z=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		s.set(x,y,z);
		a.set(f,s);
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		z=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;

		f.set(x,y,z);
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		z=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;

		s.set(x,y,z);
		b.set(f,s);
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		z=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;

		f.set(x,y,z);
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		z=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;

		s.set(x,y,z);
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
	z.g+=y.g;
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
	int len=n*6*bytes_per_big;
	ZZn3 x,y;
	ZZn a,b,c;

	if (mtable==NULL) return 0;

	bytes=new char[len];
	for (i=j=0;i<n;i++)
	{
		mtable[i].get(x,y);
		x.get(a,b,c);
		to_binary((Big)a,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary((Big)b,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary((Big)c,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		y.get(a,b,c);
		to_binary((Big)a,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary((Big)b,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary((Big)c,bytes_per_big,&bytes[j],TRUE);
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
	int len=n*6*bytes_per_big;
	ZZn3 x,y;
	ZZn a,b,c;
	if (mtable!=NULL) return;

	mtable=new ECn3[1<<WINDOW_SIZE];
	for (i=j=0;i<n;i++)
	{
		a=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		b=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		c=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		x.set(a,b,c);
		a=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		b=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		c=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y.set(a,b,c);
		mtable[i].set(x,y);
	}
	delete [] bytes;
}


// test if a ZZn18 element is of order q
// can't think of a faster way to do this..

BOOL PFC::member(const GT& z)
{
	ZZn18 r=z.g;
	if (!r.is_unitary()) return FALSE;
	if (r*conj(r)!=(ZZn18)1) return FALSE; // not unitary
	if (pow(r,*ord)!=(ZZn18)1) return FALSE;
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
	ECn3 v=w.g;
	ZZn3 x,y;
	int i,j,k,bp,is,t;
	if (small) t=MR_ROUNDUP(2*S,WINDOW_SIZE);
	else       t=MR_ROUNDUP(bits(*ord),WINDOW_SIZE);
	w.mtable=new ECn3[1<<WINDOW_SIZE];
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
	ZZn18 v=w.g;
	int i,j,k,bp,is,t;
	if (small) t=MR_ROUNDUP(2*S,WINDOW_SIZE);
	else       t=MR_ROUNDUP(bits(*ord),WINDOW_SIZE);
	w.etable=new ZZn18[1<<WINDOW_SIZE];
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

