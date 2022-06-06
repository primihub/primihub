
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
 *    MIRACL  C++ Implementation file zzn4.cpp
 *
 *    AUTHOR  : M. Scott
 *  
 *    PURPOSE : Implementation of class ZZn4  (Arithmetic over n^4)
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 *    Note: This code assumes that -2 is a Quadratic Non-Residue,
 *          so modulus p=5 mod 8
 *          OR p=3 mod 8
 *          OR p=7 mod 8, p=2,3 mod 5
 *
 *   For example for p=3 mod 8 the representation is
 *
 *   A+IB, where A=(a+ib), B=(c+id), I=sqrt(1+i)
 *   where i=sqrt(-1)
 */

#include "zzn4.h"

using namespace std;

ZZn4& ZZn4::powq(const ZZn2 &m)
{ 
	zzn4_powq(m.getzzn2(),&fn);
    return *this;
}

void ZZn4::get(ZZn2& x,ZZn2& y) const  
{zzn2_copy((zzn2 *)&fn.a,x.getzzn2()); zzn2_copy((zzn2 *)&fn.b,y.getzzn2());} 

void ZZn4::get(ZZn2& x) const
{zzn2_copy((zzn2 *)&fn.a,x.getzzn2()); }

void ZZn4::geth(ZZn2& x) const
{zzn2_copy((zzn2 *)&fn.b,x.getzzn2()); }

ZZn4& ZZn4::operator/=(const ZZn2& x)
{
    *this*=inverse(x);
    fn.unitary=FALSE;
    return *this;
}

ZZn4& ZZn4::operator/=(const ZZn4& x)
{
 *this*=inverse(x);
 if (!x.fn.unitary) fn.unitary=FALSE;
 return *this;
}

ZZn4& ZZn4::operator/=(int i)
{
	if (i==2)
	{
		zzn2_div2(&fn.a);
		zzn2_div2(&fn.b);
	}
	else
	{
		ZZn t=(ZZn)1/i;
		zzn2_smul((zzn2 *)&fn.a,t.getzzn(),(zzn2 *)&fn.a);
		zzn2_smul((zzn2 *)&fn.b,t.getzzn(),(zzn2 *)&fn.b);
	}
    fn.unitary=FALSE;
    return *this;
}

ZZn4 inverse(const ZZn4& w)
{
	ZZn4 y=w;
	zzn4_inv((zzn4 *)&y.fn);
    return y;
}

ZZn4 operator+(const ZZn4& x,const ZZn4& y) 
{ZZn4 w=x; w+=y; return w; } 

ZZn4 operator+(const ZZn4& x,const ZZn2& y) 
{ZZn4 w=x; w+=y; return w; } 

ZZn4 operator+(const ZZn4& x,const ZZn& y) 
{ZZn4 w=x; w+=y; return w; } 

ZZn4 operator-(const ZZn4& x,const ZZn4& y) 
{ZZn4 w=x; w-=y; return w; } 

ZZn4 operator-(const ZZn4& x,const ZZn2& y) 
{ZZn4 w=x; w-=y; return w; } 

ZZn4 operator-(const ZZn4& x,const ZZn& y) 
{ZZn4 w=x; w-=y; return w; } 

ZZn4 operator-(const ZZn4& x) 
{ZZn4 w; zzn4_negate((zzn4 *)&x.fn,&w.fn);  return w; } 

ZZn4 operator*(const ZZn4& x,const ZZn4& y)
{
 ZZn4 w=x;  
 if (&x==&y) w*=w;
 else        w*=y;  
 return w;
}

ZZn4 operator*(const ZZn4& x,const ZZn2& y)
{ZZn4 w=x; w*=y; return w;}

ZZn4 operator*(const ZZn2& y,const ZZn4& x)
{ZZn4 w=x; w*=y; return w;}

ZZn4 operator*(const ZZn4& x,int y)
{ZZn4 w=x; w*=y; return w;}

ZZn4 operator*(int y,const ZZn4& x)
{ZZn4 w=x; w*=y; return w;}

ZZn4 operator*(const ZZn4& x,const ZZn& y)
{ZZn4 w=x; w*=y; return w;}

ZZn4 operator*(const ZZn& y,const ZZn4& x)
{ZZn4 w=x; w*=y; return w;}

ZZn4 operator/(const ZZn4& x,const ZZn4& y)
{ZZn4 w=x; w/=y; return w;}

ZZn4 operator/(const ZZn4& x,const ZZn2& y)
{ZZn4 w=x; w/=y; return w;}

ZZn4 operator/(const ZZn4& x,int i)
{ZZn4 w=x; w/=i; return w;}

#ifndef MR_NO_RAND
ZZn4 randn4(void)
{ZZn4 w; zzn4_from_zzn2s(randn2().getzzn2(),randn2().getzzn2(),&w.fn); return w;}
#endif

ZZn4 rhs(const ZZn4& x)
{
	ZZn4 w,A,B;
	miracl *mip=get_mip();
	int twist=mip->TWIST;
	w=x*x*x;
	A=(ZZn4)getA(); B=(ZZn4)getB();
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

BOOL is_on_curve(const ZZn4& x)
{
    ZZn4 w;
	w=rhs(x);

    if (qr(w)) return TRUE;
    return FALSE;
}

BOOL qr(const ZZn4& x)
{
    ZZn2 a,s;

	if (x.iszero()) return TRUE;
	x.get(a,s);
    if (s.iszero()) return TRUE;

    s*=s; 
    a*=a; a-=txx(s);
    if (!qr(a)) return FALSE;
	return TRUE;

//    s=sqrt(a);

//	if (qr((x.a+s)/2) || qr((x.a-s)/2)) return TRUE;
//    return FALSE;

}

ZZn4 sqrt(const ZZn4& x)
{
// sqrt(a+xb) = sqrt((a+sqrt(a*a-n*b*b))/2)+x.b/(2*sqrt((a+sqrt(a*a-n*b*b))/2))
// sqrt(a) = x.sqrt(a/n)
// where x*x=n

    ZZn4 w;
    ZZn2 a,s,t;
    if (x.iszero()) return w;

	x.get(a,s);
    if (s.iszero())
    {
        if (qr(a))
        {
            s=sqrt(a);
			w.set(s,0);
        }
        else
        {
            s=sqrt(txd(a));
			w.set(0,s);
        }
        return w;
    }

    s*=s; 
    a*=a; a-=txx(s);
    s=sqrt(a);

    if (s.iszero()) return w;
	x.get(t);
    if (qr((ZZn2)((t+s)/2)))
    {
        a=sqrt((t+s)/2);
    }
    else
    {
        a=sqrt((t-s)/2);
        if (a.iszero()) return w;
    }
	x.geth(t);
	w.set(a,t/(2*a));

    return w;
}

ZZn4 conj(const ZZn4& x)
{
    ZZn4 u;
	zzn4_conj((zzn4 *)&x.fn,(zzn4 *)&u.fn);
    return u;
}

// For use by ZZn8

ZZn4 tx(const ZZn4& x)
{
	ZZn4 w=x;
	zzn4_tx(&w.fn);
	return w;
}

ZZn4 txd(const ZZn4& x)
{
	ZZn2 u,v;
	x.get(u,v);
	u=txd(u);
	ZZn4 w(v,u);

    return w;
}

// ZZn4 powering of unitary elements

ZZn4 powu(const ZZn4& x,const Big& k)
{
    int i,j,nb,n,nbw,nzs;
    ZZn4 u,u2,t[11];
    Big k3;
    if (k==0) return (ZZn4)one();
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

// regular ZZn4 powering - but see powl function in zzn2.h

ZZn4 pow(const ZZn4& x,const Big& k)
{
    int i,j,nb,n,nbw,nzs;
    ZZn4 u,u2,t[16];
    if (k==0) return (ZZn4)1;
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

ZZn4 pow(int n,const ZZn4* x,const Big* b)
{
    int k,j,i,m,nb,ea;
    ZZn4 *G;
    ZZn4 r;
    m=1<<n;
    G=new ZZn4[m];
    
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

ZZn4 powl(const ZZn4& x,const Big& k)
{
    ZZn4 w8,w9,two,y;
    int i,nb;

    two=(ZZn)2;
    y=two*x;
    if (k==0)  return (ZZn4)two;
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

ZZn2 real(const ZZn4 &x)
{
    ZZn2 r;
    x.get(r);
    return r;
}

ZZn2 imaginary(const ZZn4 &x)
{
    ZZn2 i;
    x.geth(i);
    return i;
}

#ifndef MR_NO_STANDARD_IO

ostream& operator<<(ostream& s,const ZZn4& xx)
{
    ZZn4 b=xx;
    ZZn2 x,y;
    b.get(x,y);
    s << "[" << x << "," << y << "]";
    return s;    
}

#endif

