
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
 *    MIRACL  C++ Implementation file ZZn6.cpp
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

#include "zzn6.h"

using namespace std;

// Frobenius

ZZn6& ZZn6::powq(void)
{ 
    ZZn X;
    copy(get_mip()->sru,X.getzzn());
    a.powq();     
    b.powq();
    b*=X;
    return *this;
}

void ZZn6::get(ZZn3& x,ZZn3& y) const 
{x=a; y=b;} 

void ZZn6::get(ZZn3& x) const
{x=a; }

void ZZn6::geti(ZZn3& y) const
{y=b; }

ZZn6& ZZn6::operator*=(const ZZn6& x)
{ // optimized to reduce constructor/destructor calls
 if (&x==this)
 {
/* See Stam & Lenstra, "Efficient subgroup exponentiation in Quadratic .. Extensions", CHES 2002 */
    if (unitary)
    {
        ZZn3 t=b; t*=t;
        b+=a; b*=b;
        b-=t;
        a=tx(t);
        b-=a;
        a+=a; a+=one();
        b-=one();
    //    cout << "unitary" << endl;
    }
    else 
    {
        ZZn3 t=a; t+=b;
        ZZn3 t2=a; t2+=tx(b);
        t*=t2;
        b*=a;
        t-=b;
        t-=tx(b);
        b+=b;
        a=t;
	//	cout << "not unitary" << endl;
    }
 }
 else
 {
    ZZn3 ac=a; ac*=x.a;
    ZZn3 bd=b; bd*=x.b;
    ZZn3 t=x.a; t+=x.b;
    b+=a; b*=t; b-=ac; b-=bd;
    a=ac; a+=tx(bd);

    if (!x.unitary) unitary=FALSE;
 }
 return *this;
}

ZZn6& ZZn6::operator/=(const ZZn3& x)
{
    *this*=inverse(x);
    unitary=FALSE;
    return *this;
}

ZZn6& ZZn6::operator/=(const ZZn& x)
{
    ZZn t=one()/x;
    a*=t;
    b*=t;
    unitary=FALSE;
    return *this;
}

ZZn6& ZZn6::operator/=(int i)
{
    ZZn t=one()/i;
    a*=t;
    b*=t;
    unitary=FALSE;
    return *this;
}

ZZn6& ZZn6::operator/=(const ZZn6& x)
{
 *this*=inverse(x);
 if (!x.unitary) unitary=FALSE;
 return *this;
}

ZZn6 tx(const ZZn6& x)
{
	ZZn3 t=tx(x.b);
	ZZn6 u(t,x.a);
	return u;
}

ZZn6 tx2(const ZZn6& x)
{
	ZZn6 u(tx(x.a),tx(x.b));
	return u;	
}

ZZn6 tx4(const ZZn6& x)
{
	ZZn6 u(tx2(x.a),tx2(x.b));
	return u;	
}

ZZn6 txd(const ZZn6& x)
{
    ZZn3 t=txd(x.a);
    ZZn6 u(x.b,t);
    return u;
}

ZZn6 inverse(const ZZn6& w)
{
    ZZn6 y=conj(w);
    if (w.unitary) return y;
    ZZn3 u=w.a;
    ZZn3 v=w.b;
    u*=u;
    v*=v;
    u-=tx(v);
    u=inverse(u);
    y*=u;
    return y;
}

ZZn6 operator+(const ZZn6& x,const ZZn6& y) 
{ZZn6 w=x; w+=y; return w; } 

ZZn6 operator+(const ZZn6& x,const ZZn3& y) 
{ZZn6 w=x; w+=y; return w; } 

ZZn6 operator+(const ZZn6& x,const ZZn& y) 
{ZZn6 w=x; w+=y; return w; } 

ZZn6 operator-(const ZZn6& x,const ZZn6& y) 
{ZZn6 w=x; w-=y; return w; } 

ZZn6 operator-(const ZZn6& x,const ZZn3& y) 
{ZZn6 w=x; w-=y; return w; } 

ZZn6 operator-(const ZZn6& x,const ZZn& y) 
{ZZn6 w=x; w-=y; return w; } 

ZZn6 operator-(const ZZn6& x) 
{ZZn6 w; w.a=-x.a; w.b=-x.b; w.unitary=FALSE; return w; } 

ZZn6 operator*(const ZZn6& x,const ZZn6& y)
{
 ZZn6 w=x;  
 if (&x==&y) w*=w;
 else        w*=y;  
 return w;
}

ZZn6 operator*(const ZZn6& x,const ZZn3& y)
{ZZn6 w=x; w*=y; return w;}

ZZn6 operator*(const ZZn6& x,const ZZn& y)
{ZZn6 w=x; w*=y; return w;}

ZZn6 operator*(const ZZn3& y,const ZZn6& x)
{ZZn6 w=x; w*=y; return w;}

ZZn6 operator*(const ZZn& y,const ZZn6& x)
{ZZn6 w=x; w*=y; return w;}

ZZn6 operator*(const ZZn6& x,int y)
{ZZn6 w=x; w*=y; return w;}

ZZn6 operator*(int y,const ZZn6& x)
{ZZn6 w=x; w*=y; return w;}

ZZn6 operator/(const ZZn6& x,const ZZn6& y)
{ZZn6 w=x; w/=y; return w;}

ZZn6 operator/(const ZZn6& x,const ZZn3& y)
{ZZn6 w=x; w/=y; return w;}

ZZn6 operator/(const ZZn6& x,const ZZn& y)
{ZZn6 w=x; w/=y; return w;}

ZZn6 operator/(const ZZn6& x,int i)
{ZZn6 w=x; w/=i; return w;}
#ifndef MR_NO_RAND
ZZn6 randn6(void)
{ZZn6 w; w.a=randn3(); w.b=randn3(); w.unitary=FALSE; return w;}
#endif

ZZn6 rhs(const ZZn6& x)
{
	ZZn6 w,A,B;
	miracl *mip=get_mip();
	int twist=mip->TWIST;
	w=x*x*x;
	A=(ZZn6)getA(); B=(ZZn6)getB();
	if (twist)
	{
	  if (twist==MR_QUARTIC_M)
	  {
			w+=tx(A)*x;
	  }
	  if (twist==MR_QUARTIC_D)
	  {
			w+=txd(A)*x;
	  }
	  if (twist==MR_SEXTIC_M)
	  {
			w+=tx(B);
	  }
	  if (twist==MR_SEXTIC_D)
	  {
			w+=txd(B);
	  }
	  if (twist==MR_QUADRATIC)
	  {
			w+=tx(tx(A))*x+tx(tx(tx(B)));
	  }
	}
	else
	{
        w+=A*x+B;
    }
	return w;
}

BOOL qr(const ZZn6& x)
{
    ZZn3 a,s;
    if (x.iszero()) return TRUE;
    if (x.b.iszero()) return TRUE;
    s=x.b; s*=s; 
    a=x.a; a*=a; a-=tx(s);
    if (!qr(a)) return FALSE;
    return TRUE;
/*
    s=sqrt(a);
    if (qr((x.a+s)/2) || qr((x.a-s)/2)) return TRUE;
    exit(0);
    return FALSE;
*/
}

ZZn6 sqrt(const ZZn6& x)
{
// sqrt(a+xb) = sqrt((a+sqrt(a*a-n*b*b))/2)+x.b/(2*sqrt((a+sqrt(a*a-n*b*b))/2))
// sqrt(a) = x.sqrt(a/n)
// where x*x=n

    ZZn6 w;
    ZZn3 a,s,t;

    if (x.iszero()) return w;


    if (x.b.iszero())
    {
        w.unitary=x.unitary;
		a=x.a;
        if (qr(a))
        {
			
            s=sqrt(a);
            w.a=s; w.b=0;
        }
        else
        {
			a=txd(a);
            s=sqrt(a);
            w.a=0; w.b=s;
        }
        return w;
    }

    s=x.b; s*=s; 
    a=x.a; a*=a; a-=tx(s);
    s=sqrt(a);

    if (s.iszero()) return w;

    w.unitary=x.unitary;
    if (qr((x.a+s)/2))
    {
        a=sqrt((x.a+s)/2);
    }
    else
    {
        a=sqrt((x.a-s)/2);
        if (a.iszero()) return w;
    }

    w.a=a;
    w.b=x.b/(2*a);

    return w;
}

ZZn6 conj(const ZZn6& x)
{
    ZZn6 u=x;
    u.conj();
    return u;
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

// regular ZZn6 powering - but see powl function in ZZn3.h

ZZn6 pow(const ZZn6& x,const Big& k)
{
    int i,j,nb,n,nbw,nzs;
    ZZn6 u,u2,t[16];
    if (k==0) return (ZZn6)one();
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

// standard MIRACL multi-exponentiation

ZZn6 pow(int n,const ZZn6* x,const Big* b)
{
    int k,j,i,m,nb,ea;
    ZZn6 *G;
    ZZn6 r;
    m=1<<n;
    G=new ZZn6[m];
    
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

#ifndef MR_NO_STANDARD_IO

ostream& operator<<(ostream& s,const ZZn6& xx)
{
    ZZn6 b=xx;
    ZZn3 x,y;
    b.get(x,y);
    s << "[" << x << "," << y << "]";
    return s;    
}

#endif

