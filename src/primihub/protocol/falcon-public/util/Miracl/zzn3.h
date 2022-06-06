
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
 *    MIRACL  C++ Header file ZZn3.h
 *
 *    AUTHOR  : M. Scott
 *
 *    NOTE:   : Must be used in conjunction with zzn3.cpp big.cpp and zzn.cpp
 *            : An element looks like (a+bx+cx^2)
 *            : where x is the cube root of cnr (a cubic non-residue).
 *
 *    PURPOSE : Definition of class ZZn3  (Arithmetic over n^3)
 *
 *    Note: This code assumes that n=1 mod 3 and x^3+cnr is irreducible for some small cnr;
 *
 */

#ifndef ZZN3_H
#define ZZN3_H

#include "zzn.h"

#ifdef ZZNS
#define MR_INIT_ZZN3 {fn.a=&at; at.w=a; at.len=UZZNS; fn.b=&bt; bt.w=b; bt.len=UZZNS; fn.c=&ct; ct.w=c; ct.len=UZZNS;} 
#define MR_CLONE_ZZN3(x) {at.len=x.at.len; bt.len=x.bt.len; ct.len=x.ct.len; for (int i=0;i<UZZNS;i++) {a[i]=x.a[i]; b[i]=x.b[i]; c[i]=x.c[i];}}
#define MR_ZERO_ZZN3 {at.len=bt.len=ct.len=0; for (int i=0;i<UZZNS;i++) {a[i]=b[i]=c[i]=0;}} 
#else
#define MR_INIT_ZZN3 {fn.a=mirvar(0); fn.b=mirvar(0); fn.c=mirvar(0);}
#define MR_CLONE_ZZN3(x) {zzn3_copy((zzn3 *)&x.fn,&fn);}
#define MR_ZERO_ZZN3 {zzn3_zero(&fn);}
#endif

class ZZn3
{
    zzn3 fn;
#ifdef ZZNS
    mr_small a[UZZNS];
    mr_small b[UZZNS];
    mr_small c[UZZNS];
    bigtype at,bt,ct;
#endif

public:
    ZZn3()   {MR_INIT_ZZN3 MR_ZERO_ZZN3 }
    ZZn3(int w) {MR_INIT_ZZN3 if (w==0) MR_ZERO_ZZN3 else zzn3_from_int(w,&fn);}
    ZZn3(const ZZn3& w) {MR_INIT_ZZN3 MR_CLONE_ZZN3(w) }
    ZZn3(const ZZn &x,const ZZn& y,const ZZn& z) {MR_INIT_ZZN3 zzn3_from_zzns(x.getzzn(),y.getzzn(),z.getzzn(),&fn); }
    ZZn3(const ZZn &x) {MR_INIT_ZZN3 zzn3_from_zzn(x.getzzn(),&fn);}
    ZZn3(const Big &x) {MR_INIT_ZZN3 zzn3_from_big(x.getbig(),&fn);}
    
    void set(const ZZn &x,const ZZn &y,const ZZn &z) {zzn3_from_zzns(x.getzzn(),y.getzzn(),z.getzzn(),&fn);}
    void set(const ZZn &x) {zzn3_from_zzn(x.getzzn(),&fn);}
    void set1(const ZZn &x) {zzn3_from_zzn_1(x.getzzn(),&fn);}
    void set2(const ZZn &x) {zzn3_from_zzn_2(x.getzzn(),&fn);}
    void set(const Big &x)  {zzn3_from_big(x.getbig(),&fn);}

    void get(ZZn &,ZZn &,ZZn &) const;
    void get(ZZn &) const;
    
    void clear() {MR_ZERO_ZZN3 }
    
    BOOL iszero()  const {return zzn3_iszero((zzn3 *)&fn);}
	BOOL isunity() const {return zzn3_isunity((zzn3 *)&fn);}

    ZZn3& powq(void) {zzn3_powq(&fn,&fn); return *this;}
    ZZn3& operator=(int i) {if (i==0) MR_ZERO_ZZN3 else zzn3_from_int(i,&fn); return *this;}
    ZZn3& operator=(const ZZn& x) {zzn3_from_zzn(x.getzzn(),&fn); return *this; }
    ZZn3& operator=(const ZZn3& x) {MR_CLONE_ZZN3(x) return *this; }
    ZZn3& operator+=(const ZZn& x) {zzn3_sadd(&fn,x.getzzn(),&fn); return *this; }
    ZZn3& operator+=(const ZZn3& x) {zzn3_add(&fn,(zzn3 *)&x,&fn); return *this; }
    ZZn3& operator-=(const ZZn& x) {zzn3_ssub(&fn,x.getzzn(),&fn); return *this; }
    ZZn3& operator-=(const ZZn3& x) {zzn3_sub(&fn,(zzn3 *)&x,&fn); return *this; }
    ZZn3& operator*=(const ZZn3& x)   {zzn3_mul(&fn,(zzn3 *)&x,&fn); return *this; } 
    ZZn3& operator*=(const ZZn& x) {zzn3_smul(&fn,x.getzzn(),&fn); return *this; }
    ZZn3& operator*=(int x) {zzn3_imul(&fn,x,&fn); return *this;}

    ZZn3& operator/=(const ZZn3&); 
    ZZn3& operator/=(const ZZn&);
    ZZn3& operator/=(int);
   
    friend ZZn3 operator+(const ZZn3&,const ZZn3&);
    friend ZZn3 operator+(const ZZn3&,const ZZn&);
    friend ZZn3 operator-(const ZZn3&,const ZZn3&);
    friend ZZn3 operator-(const ZZn3&,const ZZn&);
    friend ZZn3 operator-(const ZZn3&);

    friend ZZn3 operator*(const ZZn3&,const ZZn3&);
    friend ZZn3 operator*(const ZZn3&,const ZZn&);
    friend ZZn3 operator*(const ZZn&,const ZZn3&);

    friend ZZn3 operator*(int,const ZZn3&);
    friend ZZn3 operator*(const ZZn3&,int);

    friend ZZn3 operator/(const ZZn3&,const ZZn3&);
    friend ZZn3 operator/(const ZZn3&,const ZZn&);
    friend ZZn3 operator/(const ZZn3&,int);

	friend ZZn3 rhs(const ZZn3&);

    friend ZZn3 pow(const ZZn3&,const Big&);
    friend ZZn3 pow(int,const ZZn3*,const Big*);
    friend ZZn3 powl(const ZZn3&,const Big&);
    friend ZZn3 powl(const ZZn3&,const Big&,const ZZn3&,const Big&,const ZZn3&);
    friend ZZn3 inverse(const ZZn3&);
#ifndef MR_NO_RAND
    friend ZZn3 randn3(void);        // random ZZn3
#endif
    friend ZZn3 tx(const ZZn3&);     // multiply a+bx+cx^2 by x
	friend ZZn3 tx2(const ZZn3&);     // multiply a+bx+cx^2 by x^2
    friend ZZn3 txd(const ZZn3&);    // divide a+bx+cx^2 by x
    friend BOOL qr(const ZZn3&);     // quadratic residue
    friend BOOL is_on_curve(const ZZn3&);
    friend ZZn3 sqrt(const ZZn3&);   // square root - 0 if none exists

    friend BOOL operator==(const ZZn3& x,const ZZn3& y)
    {if (zzn3_compare((zzn3 *)&x,(zzn3 *)&y)) return TRUE; else return FALSE; }

    friend BOOL operator!=(const ZZn3& x,const ZZn3& y)
    {if (!zzn3_compare((zzn3 *)&x,(zzn3 *)&y)) return TRUE; else return FALSE; }

#ifndef MR_NO_STANDARD_IO
    friend ostream& operator<<(ostream&,const ZZn3&);
#endif

    ~ZZn3()      
    {
    //    MR_ZERO_ZZN3  // slower but safer
#ifndef ZZNS  
        mr_free(fn.a); 
        mr_free(fn.b);
        mr_free(fn.c);
#endif
    }
};
#ifndef MR_NO_RAND
extern ZZn3 randn3(void);
#endif
#endif

