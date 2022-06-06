
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
 *
 *    MIRACL  C++ Header file ecn.h
 *
 *    AUTHOR  : M. Scott
 *  
 *    PURPOSE : Definition of class ECn  (Arithmetic on an Elliptic Curve,
 *               mod n)
 *
 *    NOTE    : Must be used in conjunction with ecn.cpp and big.cpp
 *              The active curve is set dynamically (via the Big ecurve() 
 *              routine) - so beware the pitfalls implicit in declaring
 *              static or global ECn's (which are initialised before the 
 *              curve is set!). Uninitialised data is OK 
 *
 */

#ifndef ECN_H
#define ECN_H

#include <cstring>
#include "big.h"

#ifdef ZZNS
#define MR_INIT_ECN memset(mem,0,mr_ecp_reserve(1,ZZNS)); p=(epoint *)epoint_init_mem_variable(mem,0,ZZNS); 
#else
#define MR_INIT_ECN mem=(char *)ecp_memalloc(1); p=(epoint *)epoint_init_mem(mem,0); 
#endif

class ECn
{
    epoint *p;
#ifdef ZZNS
    char mem[mr_ecp_reserve(1,ZZNS)];
#else
    char *mem;
#endif
public:
    ECn()                           {MR_INIT_ECN }
   
    ECn(const Big &x,const Big& y)  {MR_INIT_ECN 
                                   epoint_set(x.getbig(),y.getbig(),0,p); }
    
  // This next constructor restores a point on the curve from "compressed" 
  // data, that is the full x co-ordinate, and the LSB of y  (0 or 1)

#ifndef MR_SUPPORT_COMPRESSION
    ECn(const Big& x,int cb)             {MR_INIT_ECN
                                   epoint_set(x.getbig(),x.getbig(),cb,p); }
#endif
    
    ECn(const ECn &b)                   {MR_INIT_ECN epoint_copy(b.p,p);}

    epoint *get_point() const;
    int get_status() {return p->marker;}
    ECn& operator=(const ECn& b)  {epoint_copy(b.p,p);return *this;}

    ECn& operator+=(const ECn& b) {ecurve_add(b.p,p); return *this;}

    int add(const ECn&,big *,big *ex1=NULL,big *ex2=NULL) const; 
                                  // returns line slope as a big
    int sub(const ECn&,big *,big *ex1=NULL,big *ex2=NULL) const;         
 
    ECn& operator-=(const ECn& b) {ecurve_sub(b.p,p); return *this;}

  // Multiplication of a point by an integer. 

    ECn& operator*=(const Big& k) {ecurve_mult(k.getbig(),p,p); return *this;}

    void clear() {epoint_set(NULL,NULL,0,p);}
    BOOL set(const Big& x,const Big& y)    {return epoint_set(x.getbig(),y.getbig(),0,p);}
#ifndef MR_AFFINE_ONLY
// use with care if at all
	void setz(const Big& z) {nres(z.getbig(),p->Z); p->marker=MR_EPOINT_GENERAL;}
#endif
    BOOL iszero() const;
    int get(Big& x,Big& y) const;

  // This gets the point in compressed form. Return value is LSB of y-coordinate
    int get(Big& x) const;

  // get raw coordinates
    void getx(Big &x) const;
    void getxy(Big &x,Big &y) const;
    void getxyz(Big &x,Big &y,Big &z) const;

  // point compression

  // This sets the point from compressed form. cb is LSB of y coordinate 
#ifndef MR_SUPPORT_COMPRESSION
    BOOL set(const Big& x,int cb=0)  {return epoint_set(x.getbig(),x.getbig(),cb,p);}
#endif
    friend ECn operator-(const ECn&);
    friend void multi_add(int,ECn *,ECn *);
    friend void double_add(ECn&,ECn&,ECn&,ECn&,big&,big&);

    friend ECn mul(const Big&, const ECn&, const Big&, const ECn&);
    friend ECn mul(int, const Big *, ECn *);
  
    friend void normalise(ECn &e) {epoint_norm(e.p);}
    friend void multi_norm(int,ECn *);

    friend BOOL operator==(const ECn& a,const ECn& b)
                                  {return epoint_comp(a.p,b.p);}    
    friend BOOL operator!=(const ECn& a,const ECn& b)
                                  {return (!epoint_comp(a.p,b.p));}    

    friend ECn operator*(const Big &,const ECn&);

#ifndef MR_NO_STANDARD_IO

    friend ostream& operator<<(ostream&,const ECn&);

#endif

    ~ECn() {
#ifndef ZZNS
        mr_free(mem); 
#endif
 }

};

#endif

