
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
 *    MIRACL  C++ Implementation file ZZn18.cpp
 *
 *    AUTHOR  : M. Scott
 *  
 *    PURPOSE : Implementation of class ZZn18  (Arithmetic over n^12)
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 */

#include "zzn18.h"

using namespace std;

// Frobenius X=x^p. Assumes p=13 mod 18. Ideally should be generalised depending on mip->pmod9.

ZZn18& ZZn18::powq(ZZn& W)
{ 
    BOOL ku=unitary;
	BOOL km=miller;
	a.powq(); b.powq(); c.powq();
	b=tx4(W*b);
	c=get_mip()->cnr*(tx2((W*W)*c));
	unitary=ku;
	miller=km;
    return *this;
}

void ZZn18::get(ZZn6& x,ZZn6& y,ZZn6& z)  const
{x=a; y=b; z=c;} 

void ZZn18::get(ZZn6& x) const
{x=a; }

void ZZn18::get1(ZZn6& x) const
{x=b; }

void ZZn18::get2(ZZn6& x) const
{x=c; }

ZZn18& ZZn18::operator*=(const ZZn18& x)
{ // optimized to reduce constructor/destructor calls
 if (&x==this)
 { 
	ZZn6 A,B,C,D;
    if (unitary)
	{ // Granger & Scott PKC 2010 - only 3 squarings!
	
		A=a; a*=a; D=a; a+=a; a+=D; A.conj(); A+=A; a-=A;
		B=c; B*=B; 
		B=tx(B); 
		D=B; B+=B; B+=D;
		C=b; C*=C;          D=C; C+=C; C+=D;

		b.conj(); b+=b; c.conj(); c+=c; c=-c;
		b+=B; c+=C;
	//	cout << "unitary" << endl;
	}
	else
    { 
		if (!miller)
		{ // Chung-Hasan SQR2
			A=a; A*=A;
			B=b*c; B+=B;
			C=c; C*=C;
			D=a*b; D+=D;
			c+=(a+b); c*=c;

			a=A+tx(B);
			b=D+tx(C);
			c-=(A+B+C+D);
		}
		else
		{
// Chung-Hasan SQR3 - actually calculate 2x^2 !
// Slightly dangerous - but works as will be raised to p^{k/2}-1
// which wipes out the 2.
			A=a; A*=A;       // a0^2    = S0
			C=c; C*=b; C+=C; // 2a1.a2  = S3
			D=c; D*=D;       // a2^2    = S4
			c+=a;            // a0+a2

			B=b; B+=c; B*=B; // (a0+a1+a2)^2  =S1

			c-=b; c*=c;      // (a0-a1+a2)^2  =S2

			C+=C; A+=A; D+=D; 
    
			a=A+tx(C);
			b=B-c-C+tx(D);
			c+=B-A-D;  // is this code telling me something...?
		}
/*  if you want to play safe!
	c+=B; c/=2;
	B-=c; B-=C;
	c-=A; c-=D;
	a=A+tx(C);
    b=B+tx(D);
*/

	}
 }
 else
 { // Karatsuba
    ZZn6 Z0,Z1,Z2,Z3,Z4,T0,T1;

    Z0=a*x.a;  //9
    Z2=b*x.b;  //+6

    T0=a+b;
    T1=x.a+x.b;
    Z1=T0*T1;  //+9
    Z1-=Z0;
    Z1-=Z2;
    T0=b+c;
    T1=x.b+x.c;
    Z3=T0*T1;  //+6

    Z3-=Z2;
    
    T0=a+c;
    T1=x.a+x.c;
    T0*=T1;   //+9=39 for "special case"
    Z2+=T0;
    Z2-=Z0;

	b=Z1;
	if (!(x.c).iszero())
	{ // exploit special form of BN curve line function
		Z4=c*x.c;
		Z2-=Z4;
		Z3-=Z4;
		b+=tx(Z4);
	}
    a=Z0+tx(Z3);
    c=Z2;

    if (!x.unitary) unitary=FALSE;
 }
 return *this;
}

ZZn18& ZZn18::operator/=(const ZZn6& x)
{
    *this*=inverse(x);
	unitary=FALSE;
    return *this;
}


ZZn18& ZZn18::operator/=(const ZZn18& x)
{
    *this*=inverse(x);
	if (!x.unitary) unitary=FALSE;
    return *this;
}

ZZn18 conj(const ZZn18& x)
{
    ZZn18 u=x;
    u.conj();
    return u;
}

ZZn18 inverse(const ZZn18& w)
{
    ZZn18 y;

	if (w.unitary)
	{
		y=conj(w);
		return y;
	}	
	
	ZZn6 f0;

    y.a=w.a*w.a-tx(w.b*w.c);
    y.b=tx(w.c*w.c)-w.a*w.b;
    y.c=w.b*w.b-w.a*w.c;

    f0=tx(w.b*y.c)+w.a*y.a+tx(w.c*y.b);
    f0=inverse(f0);

    y.c*=f0;
    y.b*=f0;
    y.a*=f0;

    return y;
}

ZZn18 operator+(const ZZn18& x,const ZZn18& y) 
{ZZn18 w=x; w.a+=y.a; w.b+=y.b; w.c+=y.c; return w; } 

ZZn18 operator+(const ZZn18& x,const ZZn6& y) 
{ZZn18 w=x; w.a+=y; return w; } 

ZZn18 operator-(const ZZn18& x,const ZZn18& y) 
{ZZn18 w=x; w.a-=y.a; w.b-=y.b; w.c-=y.c; return w; } 

ZZn18 operator-(const ZZn18& x,const ZZn6& y) 
{ZZn18 w=x; w.a-=y; return w; } 

ZZn18 operator-(const ZZn18& x) 
{ZZn18 w; w.a=-x.a; w.b=-x.b; w.c-=x.c; w.unitary=FALSE; return w; } 

ZZn18 operator*(const ZZn18& x,const ZZn18& y)
{
 ZZn18 w=x;  
 if (&x==&y) w*=w;
 else        w*=y;  
 return w;
}

ZZn18 operator*(const ZZn18& x,const ZZn6& y)
{ZZn18 w=x; w*=y; return w;}

ZZn18 operator*(const ZZn6& y,const ZZn18& x)
{ZZn18 w=x; w*=y; return w;}

ZZn18 operator*(const ZZn18& x,int y)
{ZZn18 w=x; w*=y; return w;}

ZZn18 operator*(int y,const ZZn18& x)
{ZZn18 w=x; w*=y; return w;}

ZZn18 operator/(const ZZn18& x,const ZZn18& y)
{ZZn18 w=x; w/=y; return w;}

ZZn18 operator/(const ZZn18& x,const ZZn6& y)
{ZZn18 w=x; w/=y; return w;}

#ifndef MR_NO_RAND
ZZn18 randn18(void)
{ZZn18 w; w.a=randn6(); w.b=randn6(); w.c=randn6(); w.unitary=FALSE; return w;}
#endif
ZZn18 tx(const ZZn18& w)
{
    ZZn18 u=w;
    
    ZZn6 t=u.a;
    u.a=tx(u.c);
    u.c=u.b;
    u.b=t;

    return u;
}

// regular ZZn18 powering

// If k is low Hamming weight this will be just as good..

ZZn18 pow(const ZZn18& x,const Big& k)
{
    int i,j,nb,n,nbw,nzs;
    ZZn18 u=x;
    Big e=k;
    BOOL invert_it=FALSE;

    if (e==0) return (ZZn18)one();

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

// standard MIRACL multi-exponentiation

#ifndef MR_STATIC

ZZn18 pow(int n,const ZZn18* x,const Big* b)
{
    int k,j,i,m,nb,ea;
    ZZn18 *G;
    ZZn18 r;
    m=1<<n;
    G=new ZZn18[m];

 // precomputation
    
    for (i=0,k=1;i<n;i++)
    {
        for (j=0; j < (1<<i) ;j++)
        {
            if (j==0)   G[k]=x[i];
            else        G[k]=G[j]*x[i];      
            k++;
        }
    }

    nb=0;
    for (j=0;j<n;j++) 
        if ((k=bits(b[j]))>nb) nb=k;

    r=1;
    for (i=nb-1;i>=0;i--) 
    {
        ea=0;
        k=1;
        for (j=0;j<n;j++)
        {
            if (bit(b[j],i)) ea+=k;
            k<<=1;
        }
        r*=r;
        if (ea!=0) r*=G[ea];
    }
    delete [] G;
    return r;
}

#endif

#ifndef MR_NO_STANDARD_IO

ostream& operator<<(ostream& s,const ZZn18& xx)
{
    ZZn18 b=xx;
    ZZn6 x,y,z;
    b.get(x,y,z);
    s << "[" << x << "," << y << "," << z << "]";
    return s;    
}

#endif

