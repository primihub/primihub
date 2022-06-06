
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
 *    MIRACL  C++ Header file ZZn2.h
 *
 *    AUTHOR  : M. Scott
 *
 *    PURPOSE : Definition of class ZZn2  (Arithmetic over n^2)
 *
 *    Note: This code assumes that 
 *          p=5 mod 8
 *          OR p=3 mod 4
 */

#ifndef ZZN2_H
#define ZZN2_H

#include "zzn.h"

#ifdef ZZNS
#define MR_INIT_ZZN2 {fn.a=&at; at.w=a; at.len=UZZNS; fn.b=&bt; bt.w=b; bt.len=UZZNS;} 
#define MR_CLONE_ZZN2(x) {at.len=x.at.len; bt.len=x.bt.len; for (int i=0;i<UZZNS;i++) {a[i]=x.a[i]; b[i]=x.b[i];}}
#define MR_ZERO_ZZN2 {at.len=bt.len=0; for (int i=0;i<UZZNS;i++) {a[i]=b[i]=0;}} 
#else
#define MR_INIT_ZZN2 {fn.a=mirvar(0); fn.b=mirvar(0);}
#define MR_CLONE_ZZN2(x) {zzn2_copy((zzn2 *)&x.fn,&fn);}
#define MR_ZERO_ZZN2 {zzn2_zero(&fn);}
#endif

class ZZn2
{
    zzn2 fn;
#ifdef ZZNS
    mr_small a[UZZNS];
    mr_small b[UZZNS];
    bigtype at,bt;
#endif

public:
    ZZn2()   {MR_INIT_ZZN2 MR_ZERO_ZZN2 }
    ZZn2(int w) {MR_INIT_ZZN2 if (w==0) MR_ZERO_ZZN2 else zzn2_from_int(w,&fn);}
    ZZn2(int w,int z){MR_INIT_ZZN2 zzn2_from_ints(w,z,&fn);}
    ZZn2(const ZZn2& w) {MR_INIT_ZZN2 MR_CLONE_ZZN2(w) }
    ZZn2(const Big &x,const Big& y) {MR_INIT_ZZN2 zzn2_from_bigs(x.getbig(),y.getbig(),&fn); }
    ZZn2(const ZZn &x,const ZZn& y) {MR_INIT_ZZN2 zzn2_from_zzns(x.getzzn(),y.getzzn(),&fn); }
    ZZn2(const ZZn &x) {MR_INIT_ZZN2 zzn2_from_zzn(x.getzzn(),&fn);}
    ZZn2(const Big &x)              {MR_INIT_ZZN2 zzn2_from_big(x.getbig(),&fn); }
    
    void set(const Big &x,const Big &y) {zzn2_from_bigs(x.getbig(),y.getbig(),&fn); }
    void set(const ZZn &x,const ZZn &y) {zzn2_from_zzns(x.getzzn(),y.getzzn(),&fn);}
    void set(const Big &x)              {zzn2_from_big(x.getbig(),&fn); }
    void set(int x,int y)               {zzn2_from_ints(x,y,&fn);}

    void get(Big &,Big &) const; 
    void get(Big &) const; 

    void get(ZZn &,ZZn &) const;  
    void get(ZZn &) const; 

    void clear() {MR_ZERO_ZZN2}
    BOOL iszero()  const { return zzn2_iszero((zzn2 *)&fn); }
    BOOL isunity() const { return zzn2_isunity((zzn2 *)&fn); }

    friend class ECn2;

    ZZn2& negate()
        {zzn2_negate(&fn,&fn); return *this;}

    ZZn2& operator=(int i) {if (i==0) MR_ZERO_ZZN2 else zzn2_from_int(i,&fn); return *this;}
    ZZn2& operator=(const ZZn& x) {zzn2_from_zzn(x.getzzn(),&fn); return *this; }
    ZZn2& operator=(const ZZn2& x) {MR_CLONE_ZZN2(x) return *this; }
    ZZn2& operator+=(const ZZn& x) {zzn2_sadd(&fn,x.getzzn(),&fn); return *this; }
    ZZn2& operator+=(const ZZn2& x){zzn2_add(&fn,(zzn2 *)&x.fn,&fn); return *this;}
    ZZn2& operator-=(const ZZn& x) {zzn2_ssub(&fn,x.getzzn(),&fn); return *this; }
    ZZn2& operator-=(const ZZn2& x) {zzn2_sub(&fn,(zzn2 *)&x.fn,&fn); return *this; }
    ZZn2& operator*=(const ZZn2& x) {zzn2_mul(&fn,(zzn2 *)&x.fn,&fn); return *this; }
    ZZn2& operator*=(const ZZn& x) {zzn2_smul(&fn,x.getzzn(),&fn); return *this; }
    ZZn2& operator*=(int x) {zzn2_imul(&fn,x,&fn); return *this;}

    ZZn2& conj() {zzn2_conj(&fn,&fn); return *this;}
    
    ZZn2& operator/=(const ZZn2&); 
    ZZn2& operator/=(const ZZn&); 
    ZZn2& operator/=(int);

    friend ZZn2 operator+(const ZZn2&,const ZZn2&);
    friend ZZn2 operator+(const ZZn2&,const ZZn&);
    friend ZZn2 operator-(const ZZn2&,const ZZn2&);
    friend ZZn2 operator-(const ZZn2&,const ZZn&);
    friend ZZn2 operator-(const ZZn2&);

    friend ZZn2 operator*(const ZZn2&,const ZZn2&);
    friend ZZn2 operator*(const ZZn2&,const ZZn&);
    friend ZZn2 operator*(const ZZn&,const ZZn2&);

    friend ZZn2 operator*(int,const ZZn2&);
    friend ZZn2 operator*(const ZZn2&,int);

    friend ZZn2 operator/(const ZZn2&,const ZZn2&);
    friend ZZn2 operator/(const ZZn2&,const ZZn&);
    friend ZZn2 operator/(const ZZn2&,int);

    friend ZZn  real(const ZZn2&);      
    friend ZZn  imaginary(const ZZn2&); 

    friend ZZn2 pow(const ZZn2&,const Big&);
	friend ZZn2 powu(const ZZn2&,const Big&);
    friend ZZn2 powl(const ZZn2&,const Big&);
    friend ZZn2 conj(const ZZn2&);
    friend ZZn2 inverse(const ZZn2&);
    friend ZZn2 txx(const ZZn2&);
    friend ZZn2 txd(const ZZn2&);
    friend ZZn2 tx(const ZZn2&);
#ifndef MR_NO_RAND
    friend ZZn2 randn2(void);        // random ZZn2
#endif
    friend BOOL qr(const ZZn2&);
    friend BOOL is_on_curve(const ZZn2&);
    friend ZZn2 sqrt(const ZZn2&);   // square root - 0 if none exists

    friend BOOL operator==(const ZZn2& x,const ZZn2& y)
    {return (zzn2_compare((zzn2 *)&x.fn,(zzn2 *)&y.fn)); }

    friend BOOL operator!=(const ZZn2& x,const ZZn2& y)
    {return !(zzn2_compare((zzn2 *)&x.fn,(zzn2 *)&y.fn)); }

#ifndef MR_NO_STANDARD_IO
    friend ostream& operator<<(ostream&,const ZZn2&);
#endif

    zzn2* getzzn2(void) const;

    ~ZZn2()  
    {
    //    MR_ZERO_ZZN2  // slower but safer
#ifndef ZZNS  
        mr_free(fn.a); 
        mr_free(fn.b);
#endif
    }
};
#ifndef MR_NO_RAND
extern ZZn2 randn2(void);     
#endif
#endif

