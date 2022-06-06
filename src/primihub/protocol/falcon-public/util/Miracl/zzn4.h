
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
 *    MIRACL  C++ Header file ZZn4.h
 *
 *    AUTHOR  : M. Scott
 *
 *    NOTE:   : Must be used in conjunction with zzn2.cpp big.cpp and zzn.cpp
 *            : This is designed as a "towering extension", so a ZZn4 consists
 *            : of a pair of ZZn2. An element looks like (a+x^2.b) + x(c+x^2.d)
 *
 *    PURPOSE : Definition of class ZZn4  (Arithmetic over n^4)
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 *    Note: This code assumes that 
 *          p=5 mod 8
 *          OR p=3 mod 8
 *          OR p=7 mod 8, p=2,3 mod 5
 *
 * Irreducible poly is X^2+n, where n=w+sqrt(m), m= {-1,-2} and w= {0,1,2}
 *          if p=5 mod 8, n=sqrt(-2)
 *          if p=3 mod 8, n=1+sqrt(-1)
 *          if p=7 mod 8, p=2,3 mod 5, n=2+sqrt(-1)
 *
 */

#ifndef ZZN4_H
#define ZZN4_H

#include "zzn2.h"

#ifdef ZZNS
#define MR_INIT_ZZN4 {fn.a.a=&at; at.w=a; at.len=UZZNS;  fn.a.b=&bt; bt.w=b; bt.len=UZZNS;  fn.b.a=&ct; ct.w=c; ct.len=UZZNS; fn.b.b=&dt; dt.w=d; dt.len=UZZNS; fn.unitary=FALSE;} 
#define MR_CLONE_ZZN4(x) {fn.unitary=x.fn.unitary; at.len=x.at.len; bt.len=x.bt.len; ct.len=x.ct.len; dt.len=x.dt.len;    for (int i=0;i<UZZNS;i++) {a[i]=x.a[i]; b[i]=x.b[i]; c[i]=x.c[i]; d[i]=x.d[i];}}
#define MR_ZERO_ZZN4 {fn.unitary=FALSE; at.len=bt.len=ct.len=dt.len=0; for (int i=0;i<UZZNS;i++) {a[i]=b[i]=c[i]=d[i]=0;}} 
#else
#define MR_INIT_ZZN4 {fn.a.a=mirvar(0); fn.a.b=mirvar(0); fn.b.a=mirvar(0); fn.b.b=mirvar(0); fn.unitary=FALSE;}
#define MR_CLONE_ZZN4(x) {zzn4_copy((zzn4 *)&x.fn,&fn);}
#define MR_ZERO_ZZN4 {zzn4_zero(&fn);}
#endif


class ZZn4
{
    zzn4 fn;
#ifdef ZZNS
    mr_small a[UZZNS];
    mr_small b[UZZNS];
    mr_small c[UZZNS];
    mr_small d[UZZNS];
    bigtype at,bt,ct,dt;
#endif
public:
    ZZn4()   {MR_INIT_ZZN4 MR_ZERO_ZZN4 }
    ZZn4(int w)  {MR_INIT_ZZN4 if (w==0) MR_ZERO_ZZN4 else zzn4_from_int(w,&fn); }
    ZZn4(const ZZn4& w) {MR_INIT_ZZN4 MR_CLONE_ZZN4(w)  }
    ZZn4(const ZZn2 &x) {MR_INIT_ZZN4 zzn4_from_zzn2(x.getzzn2(),&fn); }
	ZZn4(const ZZn &x)  { MR_INIT_ZZN4 zzn4_from_zzn(x.getzzn(),&fn);}
	ZZn4(const Big &x)              {MR_INIT_ZZN4 zzn4_from_big(x.getbig(),&fn); }
    ZZn4(const ZZn2 &x,const ZZn2& y) {MR_INIT_ZZN4 zzn4_from_zzn2s(x.getzzn2(),y.getzzn2(),&fn); }

    void set(const ZZn2 &x,const ZZn2 &y) {zzn4_from_zzn2s(x.getzzn2(),y.getzzn2(),&fn); }
    void set(const ZZn2 &x)  {zzn4_from_zzn2(x.getzzn2(),&fn); }
    void seth(const ZZn2 &x) {zzn2_copy(x.getzzn2(),&(fn.b)); zzn2_zero(&(fn.a)); fn.unitary=FALSE;}   

    void get(ZZn2 &,ZZn2 &) const;
    void get(ZZn2 &) const;
    void geth(ZZn2 &) const;
    
    void clear() {MR_ZERO_ZZN4 }
    void mark_as_unitary() {fn.unitary=TRUE;}
    BOOL is_unitary() {return fn.unitary;}

    BOOL iszero()  const {return zzn4_iszero((zzn4 *)&fn); }
    BOOL isunity() const {return zzn4_isunity((zzn4 *)&fn); }

    ZZn4& powq(const ZZn2&);
    ZZn4& operator=(int i) {if (i==0) MR_ZERO_ZZN4 else  zzn4_from_int(i,&fn);  return *this;}
    ZZn4& operator=(const ZZn2& x) {zzn4_from_zzn2(x.getzzn2(),&fn); return *this; }
    ZZn4& operator=(const ZZn4& x) {MR_CLONE_ZZN4(x) return *this; }
	ZZn4& operator=(const ZZn& x) {zzn4_from_zzn(x.getzzn(),&fn); return *this; }
    ZZn4& operator+=(const ZZn2& x) {zzn4_sadd(&fn,x.getzzn2(),&fn); return *this; }
    ZZn4& operator+=(const ZZn& x) {zzn2_sadd(&(fn.a),x.getzzn(),&(fn.a)); return *this; }
    ZZn4& operator+=(const ZZn4& x) {zzn4_add(&fn,(zzn4 *)&x.fn,&fn); return *this; }
    ZZn4& operator-=(const ZZn2& x) {zzn4_ssub(&fn,x.getzzn2(),&fn);  return *this; }
	ZZn4& operator-=(const ZZn& x) {zzn2_ssub(&(fn.a),x.getzzn(),&(fn.a));  return *this; }
    ZZn4& operator-=(const ZZn4& x) {zzn4_sub(&fn,(zzn4 *)&x.fn,&fn); return *this; }
    ZZn4& operator*=(const ZZn4& x) {zzn4_mul(&fn,(zzn4 *)&x.fn,&fn); return *this;} 
    ZZn4& operator*=(const ZZn2& x) {zzn4_smul(&fn,x.getzzn2(),&fn); return *this; }
	ZZn4& operator*=(const ZZn& x)  {zzn4_lmul(&fn,x.getzzn(),&fn); return *this; }
	ZZn4& operator*=(int i) {zzn4_imul(&fn,i,&fn); return *this; }
  
    ZZn4& operator/=(const ZZn4&); 
    ZZn4& operator/=(const ZZn2&);
    ZZn4& operator/=(const ZZn&);
    ZZn4& operator/=(int);
    ZZn4& conj() {zzn4_conj(&fn,&fn); return *this;}

    friend ZZn4 operator+(const ZZn4&,const ZZn4&);
    friend ZZn4 operator+(const ZZn4&,const ZZn2&);
    friend ZZn4 operator-(const ZZn4&,const ZZn4&);
    friend ZZn4 operator-(const ZZn4&,const ZZn2&);
    friend ZZn4 operator-(const ZZn4&);

    friend ZZn4 operator*(const ZZn4&,const ZZn4&);
    friend ZZn4 operator*(const ZZn4&,const ZZn2&);
    friend ZZn4 operator*(const ZZn2&,const ZZn4&);

    friend ZZn4 operator*(const ZZn4&,const ZZn&);
    friend ZZn4 operator*(const ZZn&,const ZZn4&);

    friend ZZn4 operator*(int,const ZZn4&);
    friend ZZn4 operator*(const ZZn4&,int);

    friend ZZn4 operator/(const ZZn4&,const ZZn4&);
    friend ZZn4 operator/(const ZZn4&,const ZZn2&);
    friend ZZn4 operator/(const ZZn4&,int);

	friend ZZn4 rhs(const ZZn4&);

    friend ZZn2 real(const ZZn4& x) ;
    friend ZZn2 imaginary(const ZZn4& x) ;

    friend ZZn4 pow(const ZZn4&,const Big&);
    friend ZZn4 powu(const ZZn4&,const Big&);
    friend ZZn4 pow(int,const ZZn4*,const Big*);
    friend ZZn4 powl(const ZZn4&,const Big&);
    friend ZZn4 conj(const ZZn4&);
    friend ZZn4 tx(const ZZn4&);
    friend ZZn4 txd(const ZZn4&);
    friend ZZn4 inverse(const ZZn4&);
#ifndef MR_NO_RAND
    friend ZZn4 randn4(void);        // random ZZn4
#endif
    friend BOOL qr(const ZZn4&);
    friend BOOL is_on_curve(const ZZn4&);
    friend ZZn4 sqrt(const ZZn4&);   // square root - 0 if none exists

    friend BOOL operator==(const ZZn4& x,const ZZn4& y)
    {return zzn4_compare((zzn4 *)&x.fn,(zzn4 *)&y.fn);}

    friend BOOL operator!=(const ZZn4& x,const ZZn4& y)
    {return !zzn4_compare((zzn4 *)&x.fn,(zzn4 *)&y.fn);}

#ifndef MR_NO_STANDARD_IO
    friend ostream& operator<<(ostream&,const ZZn4&);
#endif

    ~ZZn4()  
	{
#ifndef ZZNS  
        mr_free(fn.a.a); 
        mr_free(fn.a.b);
		mr_free(fn.b.a); 
        mr_free(fn.b.b);
#endif
	}
};
//#ifndef MR_NO_RAND
//extern ZZn4 randn4(void);   
//#endif
#endif

