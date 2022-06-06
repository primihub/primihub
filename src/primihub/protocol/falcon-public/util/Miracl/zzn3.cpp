
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
 *    MIRACL  C++ Implementation file zzn3.cpp
 *
 *    AUTHOR  : M. Scott
 *  
 *    PURPOSE : Implementation of class ZZn3  (Arithmetic over n^3)
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 * Assumes p=1 mod 3 and p NOT 1 mod 8
 * Irreducible is x^3+cnr (cubic non-residue)
 *
 */

#include "zzn3.h"

using namespace std;

// First find a cubic non-residue cnr, which is also a quadratic non-residue
// X is precalculated sixth root of unity (cnr)^(p-1)/6 

void ZZn3::get(ZZn& x,ZZn& y,ZZn& z)  const
{copy(fn.a,x.getzzn()); copy(fn.b,y.getzzn()); copy(fn.c,z.getzzn());} 

void ZZn3::get(ZZn& x) const
{{copy(fn.a,x.getzzn());}}

ZZn3& ZZn3::operator/=(const ZZn& x)
{
    ZZn t=(ZZn)1/x;
    zzn3_smul(&fn,t.getzzn(),&fn);
    return *this;
}

ZZn3& ZZn3::operator/=(int i)
{
    if (i==2) {zzn3_div2(&fn); return *this;}

    ZZn t=one()/i;
    zzn3_smul(&fn,t.getzzn(),&fn);
    return *this;
}

ZZn3& ZZn3::operator/=(const ZZn3& x)
{
 *this*=inverse(x);
 return *this;
}

ZZn3 inverse(const ZZn3& w)
{ZZn3 i=w; zzn3_inv(&i.fn); return i;}

ZZn3 operator+(const ZZn3& x,const ZZn3& y) 
{ZZn3 w=x; w+=y; return w; } 

ZZn3 operator+(const ZZn3& x,const ZZn& y) 
{ZZn3 w=x; w+=y; return w; } 

ZZn3 operator-(const ZZn3& x,const ZZn3& y) 
{ZZn3 w=x; w-=y;  return w; } 

ZZn3 operator-(const ZZn3& x,const ZZn& y) 
{ZZn3 w=x; w-=y; return w; } 

ZZn3 operator-(const ZZn3& x) 
{ZZn3 w; zzn3_negate((zzn3 *)&x.fn,&w.fn); return w; } 

ZZn3 operator*(const ZZn3& x,const ZZn3& y)
{
 ZZn3 w=x;  
 if (&x==&y) w*=w;
 else        w*=y;  
 return w;
}

ZZn3 operator*(const ZZn3& x,const ZZn& y)
{ZZn3 w=x; w*=y; return w;}

ZZn3 operator*(const ZZn& y,const ZZn3& x)
{ZZn3 w=x; w*=y; return w;}

ZZn3 operator*(const ZZn3& x,int y)
{ZZn3 w=x; w*=y; return w;}

ZZn3 operator*(int y,const ZZn3& x)
{ZZn3 w=x; w*=y; return w;}

ZZn3 operator/(const ZZn3& x,const ZZn3& y)
{ZZn3 w=x; w/=y; return w;}

ZZn3 operator/(const ZZn3& x,const ZZn& y)
{ZZn3 w=x; w/=y; return w;}

ZZn3 operator/(const ZZn3& x,int i)
{ZZn3 w=x; w/=i; return w;}
#ifndef MR_NO_RAND
ZZn3 randn3(void)
{ZZn3 w; zzn3_from_zzns(randn().getzzn(),randn().getzzn(),randn().getzzn(),&w.fn); return w;}
#endif

ZZn3 rhs(const ZZn3& x)
{
	ZZn3 w;
	miracl *mip=get_mip();
	int twist=mip->TWIST;
	int qnr=mip->cnr;   // the cubic non-residue is also a quadratic non-residue
	w=x*x*x;
	if (twist)
	{
	  if (twist==MR_QUARTIC_M)
	  {
			w+=tx((ZZn3)getA())*x;
	  }
	  if (twist==MR_QUARTIC_D)
	  {
			w+=txd((ZZn3)getA())*x;
	  }
	  if (twist==MR_SEXTIC_M || twist==MR_CUBIC_M)
	  {
			w+=tx((ZZn3)getB());
	  }
	  if (twist==MR_SEXTIC_D || twist==MR_CUBIC_D)
	  {
			w+=txd((ZZn3)getB());
	  }
	  if (twist==MR_QUADRATIC)
	  {
			w+=qnr*qnr*getA()*x+(qnr*qnr*qnr)*getB();
	  }
	}
	else
	{
        w+=getA()*x+getB();
    }
	return w;
}

BOOL is_on_curve(const ZZn3& x)
{
    ZZn3 w;
	if (qr(rhs(x))) return TRUE;
	return FALSE;
}

BOOL qr(const ZZn3& x)
{
    Big p=get_modulus();
    ZZn3 r,t;
    
    t=r=x;
    t.powq();
    r*=t;
    t.powq();
    r*=t;
// check x^[(p^3-1)/2] = 1
    if (pow(r,(p-1)/2)==1) return TRUE;
    else return FALSE;
}

ZZn3 sqrt(const ZZn3& x)
{
    ZZn3 r,w,t;
    Big p=get_modulus();

    if (!qr(x)) return r;

// oh boy this is clever! 

    switch (get_mip()->pmod8)
    {
    case 5:
        r=(x+x);
        r.powq(); 
        w=r*r; t=w*r; w*=t;
        r.powq();
        r*=(w*(x+x));
        r=pow(r,(p-5)/8);
        r*=t;
        w=r*r; w*=x; w+=w;
        r*=x; r*=(w-(ZZn)1); 
        break;
    case 3:
    case 7:
        r=x;
        r.powq();
        t=r*r;
        w=t*r;
        r.powq();
        r*=(w*x);
        r=pow(r,(p-3)/4);
        r*=(t*x);
        break;
    default: break;
    }

    return r;
}

ZZn3 tx(const ZZn3& w)
{
    ZZn3 u=w;
    zzn3_timesi(&u.fn);
    return u;
}

ZZn3 tx2(const ZZn3& w)
{
    ZZn3 u=w;
    zzn3_timesi2(&u.fn);
    return u;
}


ZZn3 txd(const ZZn3& w)
{
    ZZn3 u;
    ZZn wa,wb,wc;
    w.get(wa,wb,wc);

    u.set(wb,wc,(wa/get_mip()->cnr));

    return u;
}

// regular ZZn3 powering

ZZn3 pow(const ZZn3& x,const Big& k)
{
    int i,j,nb,n,nbw,nzs;
    ZZn3 u,u2,t[16];
    if (k==0) return (ZZn3)1;
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

ZZn3 powl(const ZZn3& x,const Big& k)
{
    ZZn3 A,B,two,y;
    int j,nb;

    two=(ZZn)2;
    y=two*x;
 
 //   y=x;
    if (k==0)  return (ZZn3)two;
    if (k==1)  return y;

 //   w8=two;
 //   w9=y;
    A=y;
    B=y*y-two;

    nb=bits(k);
//cout << "nb= " << nb << endl;
    for (j=nb-1;j>=1;j--)
    {
        if (bit(k,j-1))
        {
            A*=B; A-=y; B*=B; B-=two;
        }
        else
        {
            B*=A; B-=y; A*=A; A-=two;
        }
//cout << "1. int A= " << A << endl;
    }
//cout << "1. B= " << B << endl;
    return A/2;
//    return (w8/2);
}

// double exponention - see Schoenmaker's method from Stam's thesis

ZZn3 powl(const ZZn3& x,const Big& n,const ZZn3& y,const Big& m,const ZZn3& a)
{
    ZZn3 A,B,C,T,two,vk,vl,va;
    int j,nb;

    two=(ZZn)2;
    vk=x+x;
    vl=y+y;
    va=a+a;

    nb=bits(n);
    if (bits(m)>nb) nb=bits(m);
//cout << "nb= " << nb << endl;
    A=two; B=vk; C=vl;

/*
cout << "A= " << A << endl;
cout << "B= " << B << endl;
cout << "C= " << C << endl;
cout << "vk= " << vk << endl;
cout << "vl= " << vl << endl;
cout << "va= " << va << endl;
cout << "va*va-2= " << va*va-two << endl;
*/
    for (j=nb;j>=1;j--)
    {
        if (bit(n,j-1)==0 && bit(m,j-1)==0)
        {
           B*=A; B-=vk; C*=A; C-=vl; A*=A; A-=two;
        }
        if (bit(n,j-1)==1 && bit(m,j-1)==0)
        {
            A*=B; A-=vk; C*=B; C-=va; B*=B; B-=two;
        }
        if (bit(n,j-1)==0 && bit(m,j-1)==1)
        {
            A*=C; A-=vl; B*=C; B-=va; C*=C; C-=two;
        }
        if (bit(n,j-1)==1 && bit(m,j-1)==1)
        {
            T=B*C-va; B*=A; B-=vk; C*=A; C-=vl; A=T;
            T=A*vl-B; B=A*vk-C; C=T;
        }
    }
//    return A;
    return (A/2);
}

#ifndef MR_NO_STANDARD_IO

ostream& operator<<(ostream& s,const ZZn3& xx)
{
    ZZn3 b=xx;
    ZZn x,y,z;
    b.get(x,y,z);
    s << "[" << x << "," << y << "," << z << "]";
    return s;    
}

#endif

