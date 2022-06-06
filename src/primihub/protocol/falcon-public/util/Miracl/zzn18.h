
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
 *    MIRACL  C++ Header file ZZn18.h
 *
 *    AUTHOR  : M. Scott
 *
 *    NOTE:   : Must be used in conjunction with ZZn6.cpp zzn3.cpp big.cpp and zzn.cpp
 *            : This is designed as a "towering extension", so a ZZn18 consists
 *            : of three ZZn6. 
 *
 *    PURPOSE : Definition of class ZZn18  (Arithmetic over n^18)
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 *    Note: This code assumes that 
 *          p=5 mod 8
 * Irreducible poly is X^3+n, where n=(-2)^(1/6)
 */

#ifndef ZZn18A_H
#define ZZn18A_H

#include "zzn6.h"

class ZZn18
{
    ZZn6 a,b,c;
	BOOL unitary; // "unitary" property means that fast squaring can be used, and inversions are just conjugates
	BOOL miller;  // "miller" property means that arithmetic on this instance can ignore multiplications
	              // or divisions by constants - as instance will eventually be raised to (p-1).
public:
    ZZn18()   {miller=unitary=FALSE;}
    ZZn18(int w) {a=(ZZn6)w; b.clear(); c.clear(); miller=FALSE; if (w==1) unitary=TRUE; else unitary=FALSE;}
    ZZn18(const ZZn18& w) {a=w.a; b=w.b; c=w.c; miller=w.miller; unitary=w.unitary;}
    ZZn18(const ZZn6 &x) {a=x; b.clear(); c.clear(); miller=unitary=FALSE;}
    ZZn18(const ZZn6 &x,const ZZn6& y,const ZZn6& z) {a=x; b=y; c=z; miller=unitary=FALSE;}
    ZZn18(const ZZn &x) {a=(ZZn6)x; b.clear(); c.clear(); miller=unitary=FALSE;}
    ZZn18(const Big &x) {a=(ZZn6)x; b.clear(); c.clear(); miller=unitary=FALSE;}
    
    void set(const ZZn6 &x,const ZZn6 &y,const ZZn6 &z) {a=x; b=y; c=z; unitary=FALSE;}
    void set(const ZZn6 &x) {a=x; b.clear(); c.clear();unitary=FALSE;}
    void set(const ZZn6 &x,const ZZn6 &y) {a=x; b=y; c.clear();unitary=FALSE; }
    void set1(const ZZn6 &x) {a.clear(); b=x; c.clear();unitary=FALSE;}
    void set2(const ZZn6 &x) {a.clear(); b.clear(); c=x; unitary=FALSE;}
    void set(const Big &x) {a=(ZZn6)x; b.clear(); c.clear();unitary=FALSE;}

    void get(ZZn6 &,ZZn6 &,ZZn6 &) const;
    void get(ZZn6 &) const;
    void get1(ZZn6 &) const;
    void get2(ZZn6 &) const;
    
    void clear() {a.clear(); b.clear(); c.clear();}
    void mark_as_unitary() {miller=FALSE; unitary=TRUE;}
	void mark_as_miller()  {miller=TRUE;}
	void mark_as_regular() {miller=unitary=FALSE;}
    BOOL is_unitary() {return unitary;}

	ZZn18& conj() {a.conj(); b.conj(); b=-b; c.conj(); return *this;}

    BOOL iszero()  const {if (a.iszero() && b.iszero() && c.iszero()) return TRUE; return FALSE; }
    BOOL isunity() const {if (a.isunity() && b.iszero() && c.iszero()) return TRUE; return FALSE; }
 //   BOOL isminusone() const {if (a.isminusone() && b.iszero()) return TRUE; return FALSE; }

    ZZn18& powq(ZZn&);
    ZZn18& operator=(int i) {a=i; b.clear(); c.clear(); if (i==1) unitary=TRUE; else unitary=FALSE; return *this;}
    ZZn18& operator=(const ZZn6& x) {a=x; b.clear(); c.clear(); unitary=FALSE; return *this; }
    ZZn18& operator=(const ZZn18& x) {a=x.a; b=x.b; c=x.c; miller=x.miller; unitary=x.unitary; return *this; }
    ZZn18& operator+=(const ZZn6& x) {a+=x; unitary=FALSE; return *this; }
    ZZn18& operator+=(const ZZn18& x) {a+=x.a; b+=x.b; c+=x.c; unitary=FALSE; return *this; }
    ZZn18& operator-=(const ZZn6& x) {a-=x; unitary=FALSE; return *this; }
    ZZn18& operator-=(const ZZn18& x) {a-=x.a; b-=x.b; c-=x.c; unitary=FALSE; return *this; }
    ZZn18& operator*=(const ZZn18&); 
    ZZn18& operator*=(const ZZn6& x) {a*=x; b*=x; c*=x; unitary=FALSE; return *this; }
    ZZn18& operator*=(int x) {a*=x; b*=x; c*=x; unitary=FALSE; return *this;}
    ZZn18& operator/=(const ZZn18&); 
    ZZn18& operator/=(const ZZn6&);

    friend ZZn18 operator+(const ZZn18&,const ZZn18&);
    friend ZZn18 operator+(const ZZn18&,const ZZn6&);
    friend ZZn18 operator-(const ZZn18&,const ZZn18&);
    friend ZZn18 operator-(const ZZn18&,const ZZn6&);
    friend ZZn18 operator-(const ZZn18&);

    friend ZZn18 operator*(const ZZn18&,const ZZn18&);
    friend ZZn18 operator*(const ZZn18&,const ZZn6&);
    friend ZZn18 operator*(const ZZn6&,const ZZn18&);

    friend ZZn18 operator*(int,const ZZn18&);
    friend ZZn18 operator*(const ZZn18&,int);

    friend ZZn18 operator/(const ZZn18&,const ZZn18&);
    friend ZZn18 operator/(const ZZn18&,const ZZn6&);

    friend ZZn18 tx(const ZZn18&);
    friend ZZn18 pow(const ZZn18&,const Big&);
	friend ZZn18 pow(int,const ZZn18*,const Big*);
//    friend ZZn6 pow(int,const ZZn6*,const Big*);
  
    friend ZZn18 inverse(const ZZn18&);
	friend ZZn18 conj(const ZZn18&);
#ifndef MR_NO_RAND
    friend ZZn18 randn18(void);        // random ZZn18
#endif
    friend BOOL operator==(const ZZn18& x,const ZZn18& y)
    {if (x.a==y.a && x.b==y.b && x.c==y.c) return TRUE; else return FALSE; }

    friend BOOL operator!=(const ZZn18& x,const ZZn18& y)
    {if (x.a!=y.a || x.b!=y.b || x.c!=y.c) return TRUE; else return FALSE; }

#ifndef MR_NO_STANDARD_IO
    friend ostream& operator<<(ostream&,const ZZn18&);
#endif

    ~ZZn18()  {}
};
#ifndef MR_NO_RAND
extern ZZn18 randn18(void);   
#endif
#endif

