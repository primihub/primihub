
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
 *    MIRACL  C++ Header file ZZn36.h
 *
 *    AUTHOR  : M. Scott
 *
 *    NOTE:   : Must be used in conjunction with zzn12b.cpp zzn6.cpp zzn3.cpp big.cpp zzn.cpp
 *            : This is designed as a "towering extension", so a ZZn36 consists
 *            : of three ZZn12. 
 *
 *    PURPOSE : Definition of class zzn36  (Arithmetic over n^36)
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 *    Note: This code assumes that 
 *          p=5 mod 8
 * Irreducible poly is X^36+2
 *
 */

#ifndef ZZN36_H
#define ZZN36_H

#include "zzn12b.h"

class ZZn36
{
    ZZn12 a,b,c;
	BOOL unitary; // "unitary" property measns that fast squaring can be used, and inversions are just conjugates
	BOOL miller;  // "miller" property means that arithmetic on this instance can ignore multiplications
	              // or divisions by constants - as instance will eventually be raised to (p-1).
public:
    ZZn36()   {miller=unitary=FALSE;}
    ZZn36(int w) {a=(ZZn12)w; b.clear(); c.clear(); miller=FALSE; if (w==1) unitary=TRUE; else unitary=FALSE;}
    ZZn36(const ZZn36& w) {a=w.a; b=w.b; c=w.c; miller=w.miller; unitary=w.unitary;}
    ZZn36(const ZZn12 &x) {a=x; b.clear(); c.clear(); miller=unitary=FALSE;}
    ZZn36(const ZZn12 &x,const ZZn12& y,const ZZn12& z) {a=x; b=y; c=z; miller=unitary=FALSE;}
    ZZn36(const ZZn &x) {a=(ZZn12)x; b.clear(); c.clear(); miller=unitary=FALSE;}
    ZZn36(const Big &x) {a=(ZZn12)x; b.clear(); c.clear(); miller=unitary=FALSE;}
    
    void set(const ZZn12 &x,const ZZn12 &y,const ZZn12 &z) {a=x; b=y; c=z; unitary=FALSE;}
    void set(const ZZn12 &x) {a=x; b.clear(); c.clear();unitary=FALSE;}
    void set(const ZZn12 &x,const ZZn12 &y) {a=x; b=y; c.clear();unitary=FALSE; }
    void set1(const ZZn12 &x) {a.clear(); b=x; c.clear();unitary=FALSE;}
    void set2(const ZZn12 &x) {a.clear(); b.clear(); c=x; unitary=FALSE;}
    void set(const Big &x) {a=(ZZn12)x; b.clear(); c.clear();unitary=FALSE;}

    void get(ZZn12 &,ZZn12 &,ZZn12 &) const;
    void get(ZZn12 &) const;
    void get1(ZZn12 &) const;
    void get2(ZZn12 &) const;
    
    void clear() {a.clear(); b.clear(); c.clear();}
    void mark_as_unitary() {miller=FALSE; unitary=TRUE;}
	void mark_as_miller()  {miller=TRUE;}
	void mark_as_regular() {miller=unitary=FALSE;}

    BOOL is_unitary() {return unitary;}

	ZZn36& conj() {a.conj(); b.conj(); b=-b; c.conj(); return *this;}

    BOOL iszero()  const {if (a.iszero() && b.iszero() && c.iszero()) return TRUE; return FALSE; }
    BOOL isunity() const {if (a.isunity() && b.iszero() && c.iszero()) return TRUE; return FALSE; }
 //   BOOL isminusone() const {if (a.isminusone() && b.iszero()) return TRUE; return FALSE; }

    ZZn36& powq(const ZZn&);
    ZZn36& operator=(int i) {a=i; b.clear(); c.clear(); if (i==1) unitary=TRUE; else unitary=FALSE; return *this;}
    ZZn36& operator=(const ZZn12& x) {a=x; b.clear(); c.clear(); unitary=FALSE; return *this; }
    ZZn36& operator=(const ZZn36& x) {a=x.a; b=x.b; c=x.c; miller=x.miller; unitary=x.unitary; return *this; }
    ZZn36& operator+=(const ZZn12& x) {a+=x; unitary=FALSE; return *this; }
    ZZn36& operator+=(const ZZn36& x) {a+=x.a; b+=x.b; c+=x.c; unitary=FALSE; return *this; }
    ZZn36& operator-=(const ZZn12& x) {a-=x; unitary=FALSE; return *this; }
    ZZn36& operator-=(const ZZn36& x) {a-=x.a; b-=x.b; c-=x.c; unitary=FALSE; return *this; }
    ZZn36& operator*=(const ZZn36&); 
    ZZn36& operator*=(const ZZn12& x) {a*=x; b*=x; c*=x; unitary=FALSE; return *this; }
    ZZn36& operator*=(int x) {a*=x; b*=x; c*=x; unitary=FALSE; return *this;}
    ZZn36& operator/=(const ZZn36&); 
    ZZn36& operator/=(const ZZn12&);

    friend ZZn36 operator+(const ZZn36&,const ZZn36&);
    friend ZZn36 operator+(const ZZn36&,const ZZn12&);
    friend ZZn36 operator-(const ZZn36&,const ZZn36&);
    friend ZZn36 operator-(const ZZn36&,const ZZn12&);
    friend ZZn36 operator-(const ZZn36&);

    friend ZZn36 operator*(const ZZn36&,const ZZn36&);
    friend ZZn36 operator*(const ZZn36&,const ZZn12&);
    friend ZZn36 operator*(const ZZn12&,const ZZn36&);

    friend ZZn36 operator*(int,const ZZn36&);
    friend ZZn36 operator*(const ZZn36&,int);

    friend ZZn36 operator/(const ZZn36&,const ZZn36&);
    friend ZZn36 operator/(const ZZn36&,const ZZn12&);

    friend ZZn36 tx(const ZZn36&);
    friend ZZn36 pow(const ZZn36&,const Big&);
	friend ZZn36 pow(int,const ZZn36*,const Big*);
//    friend ZZn6 pow(int,const ZZn6*,const Big*);
  
    friend ZZn36 inverse(const ZZn36&);
	friend ZZn36 conj(const ZZn36&);
#ifndef MR_NO_RAND
    friend ZZn36 randn36(void);        // random ZZn36
#endif
    friend BOOL operator==(const ZZn36& x,const ZZn36& y)
    {if (x.a==y.a && x.b==y.b && x.c==y.c) return TRUE; else return FALSE; }

    friend BOOL operator!=(const ZZn36& x,const ZZn36& y)
    {if (x.a!=y.a || x.b!=y.b || x.c!=y.c) return TRUE; else return FALSE; }

#ifndef MR_NO_STANDARD_IO
    friend ostream& operator<<(ostream&,const ZZn36&);
#endif

    ~ZZn36()  {}
};
#ifndef MR_NO_RAND
extern ZZn36 randn36(void);   
#endif
#endif

