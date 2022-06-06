
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
 *    MIRACL  C++ Header file ecn6.h
 *
 *    AUTHOR  : M. Scott
 *  
 *    PURPOSE : Definition of class ECn6 (Arithmetic on an Elliptic Curve,
 *               mod n^6)
 *
 *    NOTE    : Must be used in conjunction with zzn.cpp, big.cpp and 
 *              zzn2.cpp and zzn6a.cpp
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 */

#ifndef ECN6_H
#define ECN6_H

#include "zzn6.h"

// Affine Only

class ECn6
{
    ZZn6 x,y;
    int marker;
public:
    ECn6()     {marker=MR_EPOINT_INFINITY;}
    ECn6(const ECn6& b) {x=b.x; y=b.y; marker=b.marker; }

    ECn6& operator=(const ECn6& b) 
        {x=b.x; y=b.y; marker=b.marker; return *this; }
    
    BOOL add(const ECn6&,ZZn6&);

    ECn6& operator+=(const ECn6&); 
    ECn6& operator-=(const ECn6&); 
    ECn6& operator*=(const Big&); 
   
    void clear() {x=y=0; marker=MR_EPOINT_INFINITY;}
    BOOL iszero() const; 

    void get(ZZn6&,ZZn6&) const;
    void get(ZZn6&) const;

    BOOL set(const ZZn6&,const ZZn6&); // set on the curve - returns FALSE if no such point
    BOOL set(const ZZn6&);             // sets x coordinate on curve, and finds y coordinate
    
    friend ECn6 operator-(const ECn6&);
    friend ECn6 operator+(const ECn6&,const ECn6&);
    friend ECn6 operator-(const ECn6&,const ECn6&);

    friend BOOL operator==(const ECn6& a,const ECn6 &b) 
        {return (a.x==b.x && a.y==b.y && a.marker==b.marker); }
    friend BOOL operator!=(const ECn6& a,const ECn6 &b) 
        {return (a.x!=b.x || a.y!=b.y || a.marker!=b.marker); }

    friend ECn6 operator*(const Big &,const ECn6&);

#ifndef MR_NO_STANDARD_IO
    friend ostream& operator<<(ostream&,const ECn6&);
#endif

    ~ECn6() {}
};

#endif

