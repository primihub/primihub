
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
 *    MIRACL  C++ Implementation file zzn2.cpp
 *
 *    AUTHOR  : M. Scott
 *  
 *    PURPOSE : Implementation of class ZZn2  (Arithmetic over n^2)
 *
 *    Note: This code assumes that 
 *          p=5 mod 8
 *          OR p=3 mod 4
 */

#include "zzn2.h"

using namespace std;

zzn2* ZZn2::getzzn2(void) const         
         { return (zzn2 *)&fn;}

void ZZn2::get(Big& x,Big& y)  const
{{redc(fn.a,x.getbig()); redc(fn.b,y.getbig()); }} 

void ZZn2::get(Big& x) const
{{redc(fn.a,x.getbig());} }

void ZZn2::get(ZZn& x,ZZn& y)  const
{{copy(fn.a,x.getzzn()); copy(fn.b,y.getzzn()); }} 

void ZZn2::get(ZZn& x) const
{{copy(fn.a,x.getzzn());} }


ZZn2& ZZn2::operator/=(const ZZn2& x)
{
    ZZn2 inv=x;
    zzn2_inv(&inv.fn);
    zzn2_mul(&fn,&inv.fn,&fn);
    return *this;
}

ZZn2& ZZn2::operator/=(const ZZn& x)
{
    ZZn t=one()/x;
    zzn2_smul(&fn,t.getzzn(),&fn);
    return *this;
}

ZZn2& ZZn2::operator/=(int i)
{
    if (i==2) {zzn2_div2(&fn); return *this;}
    
    ZZn t=one()/i;
    zzn2_smul(&fn,t.getzzn(),&fn);
    return *this;
}

ZZn2 operator+(const ZZn2& x,const ZZn2& y) 
{ZZn2 w=x; w+=y; return w; } 

ZZn2 operator+(const ZZn2& x,const ZZn& y) 
{ZZn2 w=x; w+=y; return w; } 

ZZn2 operator-(const ZZn2& x,const ZZn2& y) 
{ZZn2 w=x; w-=y; return w; } 

ZZn2 operator-(const ZZn2& x,const ZZn& y) 
{ZZn2 w=x; w-=y; return w; } 

ZZn2 operator-(const ZZn2& x) 
{ZZn2 w; zzn2_negate((zzn2 *)&x.fn,&w.fn); return w; } 

ZZn2 operator*(const ZZn2& x,const ZZn2& y)
{
 ZZn2 w=x;  
 if (&x==&y) w*=w;
 else        w*=y;  
 return w;
}

ZZn2 operator*(const ZZn2& x,const ZZn& y)
{ZZn2 w=x; w*=y; return w;}

ZZn2 operator*(const ZZn& y,const ZZn2& x)
{ZZn2 w=x; w*=y; return w;}

ZZn2 operator*(const ZZn2& x,int y)
{ZZn2 w=x; w*=y; return w;}

ZZn2 operator*(int y,const ZZn2& x)
{ZZn2 w=x; w*=y;return w;}

ZZn2 operator/(const ZZn2& x,const ZZn2& y)
{ZZn2 w=x; w/=y; return w;}

ZZn2 operator/(const ZZn2& x,const ZZn& y)
{ZZn2 w=x; w/=y; return w;}

ZZn2 operator/(const ZZn2& x,int i)
{ZZn2 w=x; w/=i; return w;}

ZZn2 inverse(const ZZn2 &x)
{ZZn2 i=x; zzn2_inv(&i.fn); return i;}

#ifndef MR_NO_RAND
ZZn2 randn2(void)
{ZZn2 w; zzn2_from_zzns(randn().getzzn(),randn().getzzn(),&w.fn); return w;}
#endif

BOOL is_on_curve(const ZZn2& x)
{
    ZZn2 w;
    int qnr=get_mip()->qnr;
    int twist=get_mip()->TWIST;

    if (twist==MR_QUADRATIC)
    {
 //       w=x*x*x+qnr*getA()*x+qnr*tx((ZZn2)getB());
		w=x*x*x+txx(txx((ZZn2)getA()))*x+txx(txx(txx((ZZn2)getB())));
    }
    else
    {
        w=x*x*x+getA()*x+getB();
    }

    if (qr(w)) return TRUE;
    return FALSE;
}

BOOL qr(const ZZn2& x)
{
	ZZn2 y=x;
    return (zzn2_qr(&(y.fn)));
/*
    ZZn s,xa,xb;
    int qnr=get_mip()->qnr;
cout << "in qr(Zn2)" << endl;
    if (x.iszero()) return TRUE;
    x.get(xa,xb);
    if (xb.iszero()) return TRUE;
    if (qnr==-1)
    {
        if (xa.iszero()) return TRUE;
    }

    s=xb;
    s*=s;
    if (qnr== -2) s+=s;
    if (!qr(xa*xa+s)) 
        return FALSE;
    return TRUE;  
*/
}

ZZn2 sqrt(const ZZn2& x)
{
// sqrt(a+ib) = sqrt(a+sqrt(a*a-n*b*b)/2)+ib/(2*sqrt(a+sqrt(a*a-n*b*b)/2))
// where i*i=n

    ZZn2 w=x;
    zzn2_sqrt(&(w.fn),&(w.fn));
    return w;

/*
    ZZn2 w;
    ZZn a,s,t,xa,xb;
    int qnr=get_mip()->qnr;

    if (x.iszero()) return w;
    x.get(xa,xb);
    if (xb.iszero())
    {
        if (qr(xa))
        {
            s=sqrt(xa);
            w=s;
        }
        else
        {
            s=sqrt(xa/qnr);
            w.set((ZZn)0,s);
        }
        return w;
    }

    if (qnr==-1)
    {
        if (xa.iszero())
        {
            if (qr(xb/2))
            {
                s=sqrt(xb/2);
                w.set(s,s);
            }
            else
            {
                s=sqrt(-xb/2);
                w.set(-s,s);
            }
            return w;
        }
    }
cout << "in sqrt(zzn2)" << endl;
    s=xb;
    s*=s;
    if (qnr==-2) s+=s;

    s=sqrt(xa*xa+s);

    if (s.iszero()) return w;

    if (qr((xa+s)/2))
    {
        a=sqrt((xa+s)/2);
    }    
    else
    {
        a=sqrt((xa-s)/2);
        if (a.iszero()) return w;
    }   
    w.set(a,xb/(2*a));

    return w;
*/
}

ZZn2 conj(const ZZn2& x)
{
    ZZn2 u=x;
    u.conj();
    return u;
}

// for use by ZZn4 or ZZn6a

ZZn2 txx(const ZZn2& w)
{ // multiply w by t^2 where x^4-t is irreducible polynomial for ZZn4
  //
  // for p=5 mod 8 t=sqrt(sqrt(-2)), qnr=-2
  // for p=3 mod 8 t=sqrt(1+sqrt(-1)), qnr=-1
  // for p=7 mod 8 and p=2,3 mod 5 t=sqrt(2+sqrt(-1)), qnr=-1
  ZZn2 u=w;
  zzn2_txx(&(u.fn));
  return u;
}

ZZn2 txd(const ZZn2& w)
{ // divide w by t^2 where x^4-t is irreducible polynomial for ZZn4
  //
  // for p=5 mod 8 t=sqrt(sqrt(-2)), qnr=-2
  // for p=3 mod 8 t=sqrt(1+sqrt(-1)), qnr=-1
  // for p=7 mod 8 and p=2,3 mod 5 t=sqrt(2+sqrt(-1)), qnr=-1
  ZZn2 u=w;
  zzn2_txd(&(u.fn));
  return u;
}

ZZn2 tx(const ZZn2& w)
{ // multiply w by i, the square root of the qnr
    ZZn2 u=w;
    zzn2_timesi(&u.fn);
    
    return u;
}

// regular ZZn2 powering - but see powl function in zzn.h

ZZn2 pow(const ZZn2& x,const Big& k)
{
    int i,j,nb,n,nbw,nzs;
    ZZn2 u,u2,t[16];

    if (x.iszero()) return (ZZn2)0;
    if (k==0) return (ZZn2)1;
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

// ZZn2 powering of unitary elements

ZZn2 powu(const ZZn2& x,const Big& e)
{
    int i,j,nb,n,nbw,nzs;
    ZZn2 u,u2,t[11];
    Big k,k3;

    if (e.iszero()) return (ZZn2)one();
	k=e;
	if (e<0) k=-k;

    u=x;
    if (k.isone()) 
	{
		if (e<0) u=conj(u);
		return u;
	}
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
	if (e<0) u=conj(u);
    return u;
}

// for use by ZZn4..

ZZn2 powl(const ZZn2& x,const Big& k)
{
    ZZn2 w8,w9,two,y;
    int i,nb;

    two=(ZZn)2;
    y=two*x;
    if (k==0)  return (ZZn2)two;
    if (k==1)  return y;

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

ZZn real(const ZZn2 &x)
{
    ZZn r;
    x.get(r);
    return r;
}

ZZn imaginary(const ZZn2 &x)
{
    ZZn r,i;
    x.get(r,i);
    return i;
}

#ifndef MR_NO_STANDARD_IO

ostream& operator<<(ostream& s,const ZZn2& xx)
{
    ZZn2 b=xx;
    Big x,y;
    b.get(x,y);
    s << "[" << x << "," << y << "]";
    return s;    
}

#endif

