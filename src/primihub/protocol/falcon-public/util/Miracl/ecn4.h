
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
 *    MIRACL  C++ Header file ecn4.h
 *
 *    AUTHOR  : M. Scott
 *  
 *    PURPOSE : Definition of class ECn4 (Arithmetic on an Elliptic Curve,
 *               mod n^4)
 *
 *    NOTE    : Must be used in conjunction with zzn.cpp, big.cpp and 
 *              zzn2.cpp  and zzn4.cpp
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 */

#ifndef ECN4_H
#define ECN4_H

#include "zzn4.h"

// Affine Only

class ECn4
{
    ZZn4 x,y;
    int marker;
public:
    ECn4()     {marker=MR_EPOINT_INFINITY;}
    ECn4(const ECn4& b) {x=b.x; y=b.y; marker=b.marker; }

    ECn4& operator=(const ECn4& b) 
        {x=b.x; y=b.y; marker=b.marker; return *this; }
    
    BOOL add(const ECn4&,ZZn4&);

    ECn4& operator+=(const ECn4&); 
    ECn4& operator-=(const ECn4&); 
    ECn4& operator*=(const Big&); 
   
    void clear() {x=y=0; marker=MR_EPOINT_INFINITY;}
    BOOL iszero() const; 

    void get(ZZn4&,ZZn4&) const;
    void get(ZZn4&) const;

    BOOL set(const ZZn4&,const ZZn4&); // set on the curve - returns FALSE if no such point
    BOOL set(const ZZn4&);             // sets x coordinate on curve, and finds y coordinate
    
    friend ECn4 operator-(const ECn4&);
    friend ECn4 operator+(const ECn4&,const ECn4&);
    friend ECn4 operator-(const ECn4&,const ECn4&);

	friend ECn4 mul(int,ECn4*,const Big*);
    
	friend BOOL operator==(const ECn4& a,const ECn4 &b) 
        {return (a.x==b.x && a.y==b.y && a.marker==b.marker); }
    friend BOOL operator!=(const ECn4& a,const ECn4 &b) 
        {return (a.x!=b.x || a.y!=b.y || a.marker!=b.marker); }

    friend ECn4 operator*(const Big &,const ECn4&);

#ifndef MR_NO_STANDARD_IO
    friend ostream& operator<<(ostream&,const ECn4&);
#endif

    ~ECn4() {}
};

#endif

