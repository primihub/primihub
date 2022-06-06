
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
 *    MIRACL  C++ Header file ecn3.h
 *
 *    AUTHOR  : M. Scott
 *  
 *    PURPOSE : Definition of class ECn3 (Arithmetic on an Elliptic Curve,
 *               mod n^3)
 *
 *    NOTE    : Must be used in conjunction with zzn.cpp, big.cpp and 
 *              zzn3.cpp
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 */

#ifndef ECN3_H
#define ECN3_H

#include "zzn3.h"

//#define MR_ECN3_PROJECTIVE

class ECn3
{
    mutable ZZn3 x,y;  // can change from affine to projective, but maintain same logical value
#ifdef MR_ECN3_PROJECTIVE
#ifndef MR_AFFINE_ONLY
	mutable ZZn3 z;
#endif
#endif
	mutable int marker;
public:
    ECn3()     {marker=MR_EPOINT_INFINITY;}
    ECn3(const ECn3& b) 
	{
		x=b.x; y=b.y; 
#ifdef MR_ECN3_PROJECTIVE
#ifndef MR_AFFINE_ONLY
		z=b.z;
#endif
#endif
		marker=b.marker; 
	}

    ECn3& operator=(const ECn3& b) 
    {
		x=b.x; y=b.y; 
#ifdef MR_ECN3_PROJECTIVE
#ifndef MR_AFFINE_ONLY
		z=b.z;
#endif
#endif
		marker=b.marker; 
		return *this; 
	}
    
    int add(const ECn3&,ZZn3&,ZZn3 *ex1=NULL,ZZn3 *ex2=NULL );

    ECn3& operator+=(const ECn3&); 
    ECn3& operator-=(const ECn3&); 
    ECn3& operator*=(const Big&); 
   
    void clear() 
	{
		x=y=0; 
#ifdef MR_ECN3_PROJECTIVE
#ifndef MR_AFFINE_ONLY
		z=0;
#endif
#endif
	marker=MR_EPOINT_INFINITY;
	}

    BOOL iszero() const;

    void get(ZZn3&,ZZn3&) const;
    void get(ZZn3&) const;
#ifdef MR_ECN3_PROJECTIVE
#ifndef MR_AFFINE_ONLY
	void getZ(ZZn3&) const;
	void get(ZZn3&,ZZn3&,ZZn3&) const;

	void set(const ZZn3&,const ZZn3&,const ZZn3&);
#endif
#endif
    BOOL set(const ZZn3&,const ZZn3&); // set on the curve - returns FALSE if no such point
    BOOL set(const ZZn3&);      // sets x coordinate on curve, and finds y coordinate

	void norm(void) const;    
    friend ECn3 operator-(const ECn3&);
    friend ECn3 operator+(const ECn3&,const ECn3&);
    friend ECn3 operator-(const ECn3&,const ECn3&);

	friend ECn3 mul(int,ECn3*,const Big*);
	friend ECn3 mul(const ECn3&,const Big&,const ECn3&,const Big&);

    friend BOOL operator==(const ECn3& a,const ECn3 &b) 
    {
#ifdef MR_ECN3_PROJECTIVE
		a.norm(); b.norm(); 
#endif
		return (a.x==b.x && a.y==b.y && a.marker==b.marker); 
	}
    friend BOOL operator!=(const ECn3& a,const ECn3 &b) 
    {
#ifdef MR_ECN3_PROJECTIVE
		a.norm(); b.norm(); 
#endif
		return (a.x!=b.x || a.y!=b.y || a.marker!=b.marker); 
	}

    friend ECn3 operator*(const Big &,const ECn3&);
#ifndef MR_NO_STANDARD_IO
    friend ostream& operator<<(ostream&,const ECn3&);
#endif

    ~ECn3() {}
};

#endif

