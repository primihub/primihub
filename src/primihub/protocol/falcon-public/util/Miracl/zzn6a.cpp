
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
 *    MIRACL  C++ Implementation file ZZn6a.cpp
 *
 *    AUTHOR  : M. Scott
 *  
 *    PURPOSE : Implementation of class ZZn6  (Arithmetic over n^6)
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 */

#include "zzn6a.h"

using namespace std;

// Frobenius X=x^p

ZZn6& ZZn6::powq(const ZZn2& W)
{ 
	BOOL ku=unitary;
	BOOL km=miller;
	a.conj(); b.conj(); c.conj();
    b*=W; c*=(W*W);
	unitary=ku;
	miller=km;
    return *this;
}

void ZZn6::get(ZZn2& x,ZZn2& y,ZZn2& z) const 
{x=a; y=b; z=c;} 

void ZZn6::get(ZZn2& x) const
{x=a; }

void ZZn6::get1(ZZn2& x) const
{x=b; }

void ZZn6::get2(ZZn2& x) const
{x=c; }

ZZn6& ZZn6::operator*=(const ZZn6& x)
{ // optimized to reduce constructor/destructor calls
 if (&x==this)
 { 
    ZZn2 A,B,C,D;

	if (unitary)
	{ // Granger & Scott 2009 - only 3 squarings! BUT depends on p=5 mod 8, p=1 mod 3
		A=a; a*=a; D=a; a+=a; a+=D; A.conj(); A+=A; a-=A;
		B=c; B*=B; B=txx(B); D=B; B+=B; B+=D;
		C=b; C*=C;           D=C; C+=C; C+=D;

		b.conj(); b+=b; c.conj(); c+=c; c=-c;
		b+=B; c+=C;
	}
	else
	{ // Chung-Hasan SQR2
		if (!miller)
		{
			A=a; A*=A;
			B=b*c; B+=B;
			C=c; C*=C;
			D=a*b; D+=D;
			c+=(a+b); c*=c;

			a=A+txx(B);
			b=D+txx(C);
			c-=(A+B+C+D);
		}
		else
		{ // Chung-Hasan SQR3	- calculates 2*W^2
			A=a; A*=A;       // a0^2    = S0
			C=c; C*=b; C+=C; // 2a1.a2  = S3
			D=c; D*=D;       // a2^2    = S4
			c+=a;            // a0+a2
			B=b; B+=c; B*=B; // (a0+a1+a2)^2  =S1

			c-=b; c*=c;      // (a0-a1+a2)^2  =S2

			C+=C; A+=A; D+=D; 
    
			a=A+txx(C);
			b=B-c-C+txx(D);
			c+=B-A-D;  // is this code telling me something...?
		}
	}

// Standard  Chung-Hasan SQR3	

//	A=a; A*=a;       // a0^2    = S0
//	C=c; C*=b; C+=C; // 2a1.a2  = S3
//	D=c; D*=D;       // a2^2    = S4
//	c+=a;            // a0+a2

//	B=b; B+=c; B*=B; // (a0+a1+a2)^2  =S1

//	c-=b; c*=c;      // (a0-a1+a2)^2  =S2
//	c+=B; c/=2;

//	B-=c; B-=C;
//	c-=A; c-=D;
//     a=A+txx(C);
//     b=B+txx(D);

 }
 else
 { // Karatsuba
    ZZn2 Z0,Z1,Z2,Z3,Z4,T0,T1;
    Z0=a*x.a;
    Z2=b*x.b;
    Z4=c*x.c;
    T0=a+b;
    T1=x.a+x.b;
    Z1=T0*T1;
    Z1-=Z0;
    Z1-=Z2;
    T0=b+c;
    T1=x.b+x.c;
    Z3=T0*T1;
    Z3-=Z2;
    Z3-=Z4;
    T0=a+c;
    T1=x.a+x.c;
    T0*=T1;
    Z2+=T0;
    Z2-=Z0;
    Z2-=Z4;

    a=Z0+txx(Z3);
    b=Z1+txx(Z4);
    c=Z2;

	if (!x.unitary) unitary=FALSE;
 }
 return *this;
}

ZZn6& ZZn6::operator/=(const ZZn2& x)
{
    *this*=inverse(x);
	unitary=FALSE;
    return *this;
}

ZZn6& ZZn6::operator/=(const ZZn& x)
{
    ZZn t=(ZZn)1/x;
    a*=t;
    b*=t;
    c*=t;
	unitary=FALSE;
    return *this;
}

ZZn6& ZZn6::operator/=(int i)
{
    ZZn t=(ZZn)1/i;
    a*=t;
    b*=t;
    c*=t;
	unitary=FALSE;
    return *this;
}

ZZn6& ZZn6::operator/=(const ZZn6& x)
{
    *this*=inverse(x);
	if (!x.unitary) unitary=FALSE;
    return *this;
}

ZZn6 inverse(const ZZn6& w)
{
    ZZn6 y;
    ZZn2 f0;

	if (w.unitary)
	{
		y=w;
		y.conj();
		return y;
	}

    y.a=w.a*w.a-txx(w.b*w.c);
    y.b=txx(w.c*w.c)-w.a*w.b;
    y.c=w.b*w.b-w.a*w.c;

    f0=txx(w.b*y.c)+w.a*y.a+txx(w.c*y.b);
    f0=inverse(f0);

    y.c*=f0;
    y.b*=f0;
    y.a*=f0;

    return y;
}

ZZn6 operator+(const ZZn6& x,const ZZn6& y) 
{ZZn6 w=x; w.a+=y.a; w.b+=y.b; w.c+=y.c; return w; } 

ZZn6 operator+(const ZZn6& x,const ZZn2& y) 
{ZZn6 w=x; w.a+=y; return w; } 

ZZn6 operator+(const ZZn6& x,const ZZn& y) 
{ZZn6 w=x; w.a+=y; return w; } 

ZZn6 operator-(const ZZn6& x,const ZZn6& y) 
{ZZn6 w=x; w.a-=y.a; w.b-=y.b; w.c-=y.c; return w; } 

ZZn6 operator-(const ZZn6& x,const ZZn2& y) 
{ZZn6 w=x; w.a-=y; return w; } 

ZZn6 operator-(const ZZn6& x,const ZZn& y) 
{ZZn6 w=x; w.a-=y; return w; } 

ZZn6 operator-(const ZZn6& x) 
{ZZn6 w; w.a=-x.a; w.b=-x.b; w.c-=x.c; w.unitary=FALSE; return w; } 

ZZn6 operator*(const ZZn6& x,const ZZn6& y)
{
 ZZn6 w=x;  
 if (&x==&y) w*=w;
 else        w*=y;  
 return w;
}

ZZn6 operator*(const ZZn6& x,const ZZn2& y)
{ZZn6 w=x; w*=y; return w;}

ZZn6 operator*(const ZZn6& x,const ZZn& y)
{ZZn6 w=x; w*=y; return w;}

ZZn6 operator*(const ZZn2& y,const ZZn6& x)
{ZZn6 w=x; w*=y; return w;}

ZZn6 operator*(const ZZn& y,const ZZn6& x)
{ZZn6 w=x; w*=y; return w;}

ZZn6 operator*(const ZZn6& x,int y)
{ZZn6 w=x; w*=y; return w;}

ZZn6 operator*(int y,const ZZn6& x)
{ZZn6 w=x; w*=y; return w;}

ZZn6 operator/(const ZZn6& x,const ZZn6& y)
{ZZn6 w=x; w/=y; return w;}

ZZn6 operator/(const ZZn6& x,const ZZn2& y)
{ZZn6 w=x; w/=y; return w;}

ZZn6 operator/(const ZZn6& x,const ZZn& y)
{ZZn6 w=x; w/=y; return w;}

ZZn6 operator/(const ZZn6& x,int i)
{ZZn6 w=x; w/=i; return w;}
#ifndef MR_NO_RAND
ZZn6 randn6(void)
{ZZn6 w; w.a=randn2(); w.b=randn2(); w.c=randn2(); w.unitary=FALSE; return w;}
#endif

ZZn6 tx(const ZZn6& w)
{
    ZZn6 u=w;
    
    ZZn2 t=u.a;
    u.a=txx(u.c);
    u.c=u.b;
    u.b=t;
	u.unitary=FALSE;

    return u;
}

ZZn6 conj(const ZZn6& x)
{
    ZZn6 u=x;
    u.conj();
    return u;
}

// x^a.y^b for unitary elements

ZZn6 powu(const ZZn6& x,const Big& a,const ZZn6 &y,const Big &b)
{
	ZZn6 X=x;
	ZZn6 Y=y;
	Big A=a;
	Big B=b;
	Big A3,B3;
	ZZn6 S,D,R;
	int e1,e2,h1,h2,nb,t;
	if (A<0) { A=-A; X=conj(X);	}
	if (B<0) { B=-B; Y=conj(Y);	}
// joint sparse form
	jsf(A,B,A3,A,B3,B);
	S=X*Y;
	D=Y*conj(X);
	if (A3>B3) nb=bits(A3)-1;
	else       nb=bits(B3)-1;
	R=1;
	while (nb>=0)
	{
		R*=R;
		e1=h1=e2=h2=0;
		t=0;
		if (bit(A,nb))  {e2=1; t+=8;}
		if (bit(A3,nb)) {h2=1; t+=4;}
		if (bit(B,nb))  {e1=1; t+=2;}
		if (bit(B3,nb)) {h1=1; t+=1;}

		if (t==1 || t==13) R*=Y;
		if (t==2 || t==14) R*=conj(Y);
		if (t==4 || t==7)  R*=X;
		if (t==8 || t==11) R*=conj(X);
		if (t==5)  R*=S;
		if (t==10) R*=conj(S);
		if (t==9)  R*=D;
		if (t==6)  R*=conj(D);
		nb-=1;
	}
	return R;
}

// ZZn6 powering of unitary elements

ZZn6 powu(const ZZn6& x,const Big& k)
{
    int i,j,nb,n,nbw,nzs;
    ZZn6 u,u2,t[11];
    Big k3;
    if (k==0) return (ZZn6)one();
    u=x;
    if (k==1) return u;
//
// Prepare table for windowing
//
    k3=3*k;
    u2=(u*u);
    t[0]=u;

    for (i=1;i<=10;i++)
        t[i]=u2*t[i-1];

    nb=bits(k3);
    for (i=nb-2;i>=1;)
    {
        n=naf_window(k,k3,i,&nbw,&nzs,11);

        for (j=0;j<nbw;j++) u*=u;
        if (n>0) u*=t[n/2];
        if (n<0) u*=conj(t[(-n)/2]);
        i-=nbw;
        if (nzs)
        {
            for (j=0;j<nzs;j++) u*=u;
            i-=nzs;
        }
    }
    return u;
}

// regular ZZn6 powering

// If k is low Hamming weight this will be just as good..

ZZn6 pow(const ZZn6& x,const Big& k)
{
    int i,j,nb,n,nbw,nzs;
    ZZn6 u=x;
    Big e=k;
    BOOL invert_it=FALSE;

    if (e==0) return (ZZn6)one();

    if (e<0) 
    {
        e=-e;
        invert_it=TRUE;
    }

    nb=bits(e);
    if (nb>1) for (i=nb-2;i>=0;i--)
    {
        u*=u;
        if (bit(e,i)) u*=x;
    }

    if (invert_it) u=inverse(u);

    return u;
}

/*
ZZn6 pow(const ZZn6& x,const Big& k)
{
    int i,j,nb,n,nbw,nzs;
    ZZn6 u,u2,t[16];
    if (k==0) return (ZZn6)1;
    u=x;
    if (k==1) return u;
//
// Prepare table for windowing
//
    u2=(u*u);
    t[0]=u;
   
    for (i=1;i<16;i++)
        t[i]=u2*t[i-1];

// Left to right method - with windows

    nb=bits(k);
    if (nb>1) for (i=nb-2;i>=0;)
    {
        n=window(k,i,&nbw,&nzs,5);
        for (j=0;j<nbw;j++) u*=u;
        if (n>0) u*=t[n/2];
        i-=nbw;
        if (nzs)
        {
            for (j=0;j<nzs;j++) u*=u;
            i-=nzs;
        }
    }
    return u;
}
*/

ZZn6 powl(const ZZn6& x,const Big& k)
{
     ZZn6 w8,w9,two,y;
     int i,nb;

     two=(ZZn)2;
     y=two*x;
     if (k==0) return (ZZn6)two;
     if (k==1) return y;

     w8=two;
     w9=y;
     nb=bits(k);
     for (i=nb-1;i>=0;i--)
     {
         if (bit(k,i))
         {
             w8*=w9; w8-=y; w9*=w9; w9-=two;
         }
         else
         {
             w9*=w8; w9-=y; w8*=w8; w8-=two;
         }
     }
     return (w8/2);
}

#ifndef MR_NO_STANDARD_IO

ostream& operator<<(ostream& s,const ZZn6& xx)
{
    ZZn6 b=xx;
    ZZn2 x,y,z;
    b.get(x,y,z);
    s << "[" << x << "," << y << "," << z << "]";
    return s;    
}

#endif

