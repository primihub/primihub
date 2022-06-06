
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
 *    MIRACL C++ Headerfile gf2m4x.h
 *
 *    AUTHOR  : M. Scott
 *
 *    PURPOSE : Definition of class GF2m4x - Arithmetic over the extension 
 *              field GF(2^4m)
 *
 *    NOTE    : The underlying field basis must be set by the modulo() routine
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 */

#ifndef GF2M4X_H
#define GF2M4X_H

#include <iostream>
#include "gf2m.h"  

class GF2m4x
{
    GF2m x[4];
public:
    GF2m4x()        {  }
    GF2m4x(const GF2m4x & b) 
        { x[0]=b.x[0]; x[1]=b.x[1]; x[2]=b.x[2]; x[3]=b.x[3]; }
    GF2m4x(int i)   { x[0]=i; }
    GF2m4x(const GF2m& a,const GF2m& b,const GF2m& c=0,const GF2m& d=0)
        { x[0]=a; x[1]=b; x[2]=c; x[3]=d; }

    GF2m4x(const Big& a) {x[0]=(GF2m)a;}

    void set(const GF2m& a,const GF2m& b,const GF2m& c,const GF2m& d)
        {x[0]=a; x[1]=b; x[2]=c; x[3]=d; }
    void set(const GF2m& a)  {x[0]=a; x[1]=x[2]=x[3]=0; }
    void invert();

    void get(GF2m&,GF2m&,GF2m&,GF2m&);
    void get(GF2m&,GF2m&);
    void get(GF2m&);


    void clear() {x[0]=x[1]=x[2]=x[3]=0; }
    int degree();

    BOOL iszero() const 
    {if (x[0].iszero() && x[1].iszero() && x[2].iszero() && x[3].iszero()) return TRUE; else return FALSE; } 
    BOOL isunity() const
    {if (x[0].isone() && x[1].iszero() && x[2].iszero() && x[3].iszero()) return TRUE; else return FALSE; }

    GF2m4x& powq();

    GF2m4x& operator=(const GF2m4x& b)
        { x[0]=b.x[0]; x[1]=b.x[1]; x[2]=b.x[2]; x[3]=b.x[3]; return *this; }
    GF2m4x& operator=(const GF2m& b)
        { x[0]=b; x[1]=x[2]=x[3]=0; return *this; }
    GF2m4x& operator=(int b)
        { x[0]=b; x[1]=x[2]=x[3]=0; return *this; }
    GF2m4x& operator+=(const GF2m4x& b) 
        {x[0]+=b.x[0]; x[1]+=b.x[1]; x[2]+=b.x[2]; x[3]+=b.x[3]; return *this; }
    GF2m4x& operator+=(const GF2m& b)
        {x[0]+=b; return *this; }
    GF2m4x& operator*=(const GF2m4x&);
    GF2m4x& operator*=(const GF2m&);
    GF2m4x& operator/=(const GF2m4x&);
    GF2m4x& operator/=(const GF2m&);

    friend GF2m4x operator+(const GF2m4x&,const GF2m4x&);
    friend GF2m4x operator+(const GF2m4x&,const GF2m&);
    friend GF2m4x operator+(const GF2m&,const GF2m4x&);
    
    friend GF2m4x operator*(const GF2m4x&,const GF2m4x&);
    friend GF2m4x operator*(const GF2m4x&,const GF2m&);
    friend GF2m4x operator*(const GF2m&,const GF2m4x&);
    friend GF2m4x operator/(const GF2m4x&,const GF2m4x&); 
    friend GF2m4x operator/(const GF2m4x&,const GF2m&);
 
    friend GF2m4x mul(const GF2m4x&,const GF2m4x&);
	friend GF2m4x conj(const GF2m4x&);

    friend BOOL operator==(const GF2m4x& a,const GF2m4x& b)
    {if (a.x[0]==b.x[0] && a.x[1]==b.x[1] && a.x[2]==b.x[2] && a.x[3]==b.x[3])
     return TRUE; else return FALSE; }

    friend BOOL operator!=(const GF2m4x& a,const GF2m4x& b)
    {if (a.x[0]!=b.x[0] || a.x[1]!=b.x[1] || a.x[2]!=b.x[2] || a.x[3]!=b.x[3])
     return TRUE; else return FALSE; }

    friend GF2m4x pow(const GF2m4x&,const Big&);
	friend GF2m4x powu(const GF2m4x&,const Big&);
#ifndef MR_NO_RAND 
    friend GF2m4x randx4(void);
#endif    
    friend ostream& operator<<(ostream&,const GF2m4x&);

    ~GF2m4x() {} ;
};
#ifndef MR_NO_RAND
extern GF2m4x randx4(void);
#endif
#endif

