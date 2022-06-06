
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
 *    MIRACL C++ functions gf2m.cpp
 *
 *    AUTHOR  : M. Scott
 *    
 *    PURPOSE : Implementation of class GF2m
 *
 *    NOTE:   : Must be used in conjunction with big.h and big.cpp
 *
 */

#include "gf2m.h"


GF2m square(const GF2m& b) {GF2m r=b; modsquare2(r.fn,r.fn); return r;}

GF2m inverse(const GF2m& b) {GF2m r=b; inverse2(r.fn,r.fn); return r;}

BOOL GF2m::iszero() const
{ if (size(fn)==0) return TRUE; else return FALSE; }

BOOL GF2m::isone() const
{ if (size(fn)==1) return TRUE; else return FALSE; }

BOOL modulo(int m,int a,int b,int c,BOOL check) {return prepare_basis(m,a,b,c,check);}

GF2m& GF2m::operator/=(const GF2m& b)
{ GF2m z=b; inverse2(z.fn,z.fn); modmult2(fn,z.fn,fn); return *this;}
#ifndef MR_NO_RAND
GF2m random2(void)
{GF2m z; rand2(z.fn); return z;}
#endif
GF2m operator+(const GF2m& b1,const GF2m& b2)
{GF2m abb=b1; abb+=b2; return abb;}

GF2m operator+(const GF2m& b1,int b2)
{GF2m abb=b1; abb+=b2; return abb;}

GF2m operator*(const GF2m& b1,const GF2m& b2)
{
    GF2m abb=b1;
    if (&b1==&b2)
        abb*=abb;
    else
        abb*=b2; 
    return abb; 
}

GF2m operator/(const GF2m& b1,const GF2m& b2)
{GF2m abb; inverse2(b2.fn,abb.fn); modmult2(b1.fn,abb.fn,abb.fn); return abb;}

#ifndef MR_STATIC
GF2m pow(const GF2m& b,int m)
{GF2m z; power2(b.fn,m,z.fn); return z;}
#endif

GF2m sqrt(const GF2m& b)
{GF2m z; sqroot2(b.fn,z.fn); return z;}

GF2m halftrace(const GF2m& b)
{GF2m z; halftrace2(b.fn,z.fn); return z;}

GF2m quad(const GF2m& b)
{GF2m z; if (!quad2(b.fn,z.fn)) zero(z.fn); return z;}

GF2m gcd(const GF2m& b1,const GF2m& b2)
{GF2m g; gcd2(b1.fn,b2.fn,g.fn); return g;}

void kar2x2(const GF2m *x,const GF2m *y,GF2m *z)
{
    z[0]=x[0]*y[0];
    z[2]=x[1]*y[1];
    z[1]=(x[0]+x[1])*(y[0]+y[1]);
}

void kar3x3(const GF2m *x,const GF2m *y,GF2m *z)
{
    z[0]=x[0]*y[0]; 
    z[2]=x[1]*y[1]; 
    z[4]=x[2]*y[2]; 
    z[1]= (x[0]+x[1])*(y[0]+y[1])+z[2]+z[0];
    z[3]= (x[1]+x[2])*(y[1]+y[2])+z[2]+z[4];
    z[2]+=(x[0]+x[2])*(y[0]+y[2])+z[0]+z[4];
}
