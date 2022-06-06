
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
 *    MIRACL  C++ Header file ec2.h
 *
 *    AUTHOR  : M. Scott
 *  
 *    PURPOSE : Definition of class EC2  (Arithmetic on an Elliptic Curve,
 *               over GF(2^m)
 *
 *    NOTE    : Must be used in conjunction with ec2.cpp and big.cpp
 *              The active curve is set dynamically (via the Big ecurve2() 
 *              routine) - so beware the pitfalls implicit in declaring
 *              static or global EC2's (which are initialised before the 
 *              curve is set!). Uninitialised data is OK 
 */

#ifndef EC2_H
#define EC2_H

#include <cstring>
#include "big.h"

#ifdef GF2MS
#define MR_INIT_EC2 memset(mem,0,mr_ecp_reserve(1,GF2MS)); p=(epoint *)epoint_init_mem_variable(mem,0,GF2MS); 
#else
#define MR_INIT_EC2 mem=(char *)ecp_memalloc(1); p=(epoint *)epoint_init_mem(mem,0); 
#endif

class EC2
{
    epoint *p;
#ifdef GF2MS
    char mem[mr_ecp_reserve(1,GF2MS)];
#else
    char *mem;
#endif

public:
    EC2()                         { MR_INIT_EC2}
   
    EC2(const Big &x,const Big& y)  {MR_INIT_EC2 
                                   epoint2_set(x.getbig(),y.getbig(),0,p); }
    
  // This next constructor restores a point on the curve from "compressed" 
  // data, that is the full x co-ordinate, and the LSB of y/x  (0 or 1)

    EC2(const Big& x,int cb)       {MR_INIT_EC2
                                   epoint2_set(x.getbig(),x.getbig(),cb,p); }
                   
    EC2(const EC2 &b)             {MR_INIT_EC2 epoint2_copy(b.p,p);}

    epoint *get_point() const;
    
    EC2& operator=(const EC2& b)  {epoint2_copy(b.p,p);return *this;}

    EC2& operator+=(const EC2& b) {ecurve2_add(b.p,p); return *this;}
    EC2& operator-=(const EC2& b) {ecurve2_sub(b.p,p); return *this;}

  // Multiplication of a point by an integer. 

    EC2& operator*=(const Big& k) {ecurve2_mult(k.getbig(),p,p); return *this;}
    big add(const EC2& b)         {return ecurve2_add(b.p,p); } 
                                  // returns line slope as a big
    big sub(const EC2& b)         {return ecurve2_sub(b.p,p); }

    void clear() {epoint2_set(NULL,NULL,0,p);}
    BOOL set(const Big& x,const Big& y)    {return epoint2_set(x.getbig(),y.getbig(),0,p);}
    int get(Big& x,Big& y) const;
    BOOL iszero() const;
  // This gets the point in compressed form. Return value is LSB of y-coordinate
    int get(Big& x) const;

    void getx(Big &x) const;
    void getxy(Big &x,Big& y) const;
    void getxyz(Big &x,Big &y,Big& z) const;

  // point compression

  // This sets the point from compressed form. cb is LSB of y/x 

    BOOL set(const Big& x,int cb=0)  {return epoint2_set(x.getbig(),x.getbig(),cb,p);}

    friend EC2 operator-(const EC2&);
    friend void multi_add(int,EC2 *,EC2 *);
  
    friend EC2 mul(const Big&, const EC2&, const Big&, const EC2&);
    friend EC2 mul(int, const Big *, EC2 *);
  
    friend void normalise(EC2 &e) {epoint2_norm(e.p);}

    friend BOOL operator==(const EC2& a,const EC2& b)
                                  {return epoint2_comp(a.p,b.p);}    
    friend BOOL operator!=(const EC2& a,const EC2& b)
                                  {return (!epoint2_comp(a.p,b.p));}    

    friend EC2 operator*(const Big &,const EC2&);

#ifndef MR_NO_STANDARD_IO

    friend ostream& operator<<(ostream&,const EC2&);

#endif

    ~EC2() 
    {
#ifndef GF2MS
        mr_free(mem); 
#endif
    }
};

#endif

