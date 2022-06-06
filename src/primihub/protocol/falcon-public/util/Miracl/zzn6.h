
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
 *    MIRACL  C++ Header file ZZn6.h
 *
 *    AUTHOR  : M. Scott
 *
 *    NOTE:   : Must be used in conjunction with zzn6.cpp zzn3.cpp big.cpp and zzn.cpp
 *            : This is designed as a "towering extension", so a ZZn6 consists
 *            : of a pair of ZZn3. An element looks like (a+x^2.b+x^4.c) + x(d+x^2.e+x^4.f)
 *            : where x is the cubic (and quadratic) non-residue
 *
 *    PURPOSE : Definition of class zzn6  (Arithmetic over n^6)
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 * p = 1 mod 6
 * irreducible poly is x^6+n
 *
 */

#ifndef ZZN6_H
#define ZZN6_H

#include "zzn3.h"

class ZZn6
{
    ZZn3 a,b;
    BOOL unitary;
public:
    ZZn6()   {unitary=FALSE;}
    ZZn6(int w) {a=(ZZn3)w; b=0; if (w==1) unitary=TRUE; else unitary=FALSE;}
    ZZn6(const ZZn6& w) {a=w.a; b=w.b; unitary=w.unitary;}
    ZZn6(const ZZn3 &x,const ZZn3& y) {a=x; b=y; unitary=FALSE; }
	ZZn6(const ZZn3 &x) {a=x; b=0; unitary=FALSE; }
    ZZn6(const ZZn &x) {a=x; b=0; unitary=FALSE;}
    ZZn6(const Big &x) {a=(ZZn)x; b=0; unitary=FALSE;}
    
    void set(const ZZn3 &x,const ZZn3 &y) {a=x; b=y; unitary=FALSE;}
    void set(const ZZn3 &x) {a=x; b.clear(); unitary=FALSE;}
    void seti(const ZZn3 &x) {a.clear(); b=x; unitary=FALSE;}
    void set(const Big &x) {a=(ZZn)x; b.clear(); unitary=FALSE;}

    void get(ZZn3 &,ZZn3 &) const;
	void geti(ZZn3&) const;
    void get(ZZn3 &) const;
    
    void clear() {a=0; b=0; unitary=FALSE;}
    void mark_as_unitary() {unitary=TRUE;}
    BOOL is_unitary() {return unitary;}
    
    BOOL iszero()  const {if (a.iszero() && b.iszero()) return TRUE; return FALSE; }
    BOOL isunity() const {if (a.isunity() && b.iszero()) return TRUE; return FALSE; }
 //   BOOL isminusone() const {if (a.isminusone() && b.iszero()) return TRUE; return FALSE; }

    ZZn6& powq(void);
    ZZn6& operator=(int i) {a=i; b=0; if (i==1) unitary=TRUE; else unitary=FALSE; return *this;}
    ZZn6& operator=(const ZZn& x) {a=x; b=0; unitary=FALSE; return *this; }
    ZZn6& operator=(const ZZn3& x) {a=x; b=0; unitary=FALSE; return *this; }
    ZZn6& operator=(const ZZn6& x) {a=x.a; b=x.b; unitary=x.unitary; return *this; }
    ZZn6& operator+=(const ZZn& x) {a+=x; unitary=FALSE; return *this; }
    ZZn6& operator+=(const ZZn3& x) {a+=x; unitary=FALSE; return *this; }
    ZZn6& operator+=(const ZZn6& x) {a+=x.a; b+=x.b; unitary=FALSE; return *this; }
    ZZn6& operator-=(const ZZn& x) {a-=x; unitary=FALSE;  return *this; }
    ZZn6& operator-=(const ZZn3& x) {a-=x; unitary=FALSE; return *this; }
    ZZn6& operator-=(const ZZn6& x) {a-=x.a; b-=x.b; unitary=FALSE; return *this; }
    ZZn6& operator*=(const ZZn6&); 
    ZZn6& operator*=(const ZZn3& x) {a*=x; b*=x; unitary=FALSE; return *this; }
    ZZn6& operator*=(const ZZn& x) {a*=x; b*=x; unitary=FALSE; return *this; }
    ZZn6& operator*=(int x) {a*=x; b*=x; unitary=FALSE; return *this;}
    ZZn6& operator/=(const ZZn6&); 
    ZZn6& operator/=(const ZZn3&);
    ZZn6& operator/=(const ZZn&);
    ZZn6& operator/=(int);
    ZZn6& conj() {b=-b; return *this;}

    friend ZZn6 operator+(const ZZn6&,const ZZn6&);
    friend ZZn6 operator+(const ZZn6&,const ZZn3&);
    friend ZZn6 operator+(const ZZn6&,const ZZn&);
    friend ZZn6 operator-(const ZZn6&,const ZZn6&);
    friend ZZn6 operator-(const ZZn6&,const ZZn3&);
    friend ZZn6 operator-(const ZZn6&,const ZZn&);
    friend ZZn6 operator-(const ZZn6&);

    friend ZZn6 operator*(const ZZn6&,const ZZn6&);
    friend ZZn6 operator*(const ZZn6&,const ZZn3&);
    friend ZZn6 operator*(const ZZn6&,const ZZn&);
    friend ZZn6 operator*(const ZZn&,const ZZn6&);
    friend ZZn6 operator*(const ZZn3&,const ZZn6&);

    friend ZZn6 operator*(int,const ZZn6&);
    friend ZZn6 operator*(const ZZn6&,int);

    friend ZZn6 operator/(const ZZn6&,const ZZn6&);
    friend ZZn6 operator/(const ZZn6&,const ZZn3&);
    friend ZZn6 operator/(const ZZn6&,const ZZn&);
    friend ZZn6 operator/(const ZZn6&,int);

	friend ZZn6 rhs(const ZZn6&);

    friend ZZn3  real(const ZZn6& x)      {return x.a;}
    friend ZZn3  imaginary(const ZZn6& x) {return x.b;}

    friend ZZn6 pow(const ZZn6&,const Big&);
    friend ZZn6 powu(const ZZn6&,const Big&);
    friend ZZn6 pow(int,const ZZn6*,const Big*);
    friend ZZn6 powl(const ZZn6&,const Big&);
    friend ZZn6 conj(const ZZn6&);
    friend ZZn6 inverse(const ZZn6&);
	friend ZZn6 tx(const ZZn6&);
	friend ZZn6 tx2(const ZZn6&);
	friend ZZn6 tx4(const ZZn6&);
	friend ZZn6 txd(const ZZn6&);

#ifndef MR_NO_RAND
    friend ZZn6 randn6(void);        // random ZZn6
#endif
    friend BOOL qr(const ZZn6&);
    friend ZZn6 sqrt(const ZZn6&);   // square root - 0 if none exists

    friend BOOL operator==(const ZZn6& x,const ZZn6& y)
    {if (x.a==y.a && x.b==y.b) return TRUE; else return FALSE; }

    friend BOOL operator!=(const ZZn6& x,const ZZn6& y)
    {if (x.a!=y.a || x.b!=y.b) return TRUE; else return FALSE; }

#ifndef MR_NO_STANDARD_IO
    friend ostream& operator<<(ostream&,const ZZn6&);
#endif

    ~ZZn6()  {}
};
#ifndef MR_NO_RAND
extern ZZn6 randn6(void);  
#endif
#endif

