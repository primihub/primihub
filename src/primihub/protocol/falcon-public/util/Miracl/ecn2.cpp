
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
 *    MIRACL  C++ Implementation file ecn2.cpp
 *
 *    AUTHOR  : M. Scott
 *  
 *    PURPOSE : Implementation of class ECn2  (Elliptic curves over n^2)
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 */

#include "ecn2.h"

using namespace std;

#ifndef MR_AFFINE_ONLY
void ECn2::get(ZZn2& x,ZZn2& y,ZZn2& z) const
{ecn2_get(&fn,&(x.fn),&(y.fn),&(z.fn));} 
#endif

void ECn2::get(ZZn2& x,ZZn2& y) const
{norm(); ecn2_getxy(&fn,&(x.fn),&(y.fn));  }

void ECn2::get(ZZn2& x) const
{norm(); ecn2_getx(&fn,&(x.fn));}

#ifndef MR_AFFINE_ONLY
void ECn2::getZ(ZZn2& z) const
{ecn2_getz(&fn,&(z.fn));}
#endif

void ECn2::norm(void) const
{ // normalize a point    
    ecn2_norm(&(fn));
}

BOOL ECn2::iszero(void) const
{if (fn.marker==MR_EPOINT_INFINITY) return TRUE; return FALSE;}

BOOL ECn2::set(const ZZn2& xx,const ZZn2& yy)
{
    return ecn2_set((zzn2 *)&(xx.fn),(zzn2 *)&(yy.fn),&(fn));
}

BOOL ECn2::set(const ZZn2& xx)
{
    return ecn2_setx((zzn2 *)&(xx.fn),&(fn));
}

#ifndef MR_AFFINE_ONLY
void ECn2::set(const ZZn2& xx,const ZZn2& yy,const ZZn2& zz)
{
    ecn2_setxyz((zzn2 *)&(xx.fn),(zzn2 *)&(yy.fn),(zzn2 *)&(zz.fn),&(fn));
}
#endif

ECn2 operator-(const ECn2& a) 
{
    ECn2 w=a;
    ecn2_negate(&(w.fn),&(w.fn));
    return w; 
}  

ECn2& ECn2::operator*=(const Big& k)
{
    ecn2_mul(k.getbig(),&(this->fn));
    return *this;
}

ECn2 operator*(const Big& r,const ECn2& P)
{
    ECn2 T=P;
    T*=r;
    return T;
}

#ifndef MR_NO_STANDARD_IO

ostream& operator<<(ostream& s,const ECn2& b)
{
    ZZn2 x,y,z;
    if (b.iszero())
        s << "(Infinity)";
    else
    {
        b.get(x,y);
 
        s << "(" << x << "," << y << ")";
    }
    return s;
}

#endif

ECn2 operator+(const ECn2& a,const ECn2& b)
{ECn2 c=a; c+=b; return c;}

ECn2 operator-(const ECn2& a,const ECn2& b)
{ECn2 c=a; c-=b; return c;}

ECn2& ECn2::operator-=(const ECn2& z)
{ECn2 t=(-z); *this+=t; return *this; }

ECn2& ECn2::operator+=(const ECn2& w)
{
    ecn2_add(&(w.fn),&(this->fn));
    return *this;
}

#ifndef MR_EDWARDS

BOOL ECn2::add(const ECn2& w,const ZZn2& lam,const ZZn2 &extra1)
{
    return ecn2_add2(&(w.fn),&(this->fn),(zzn2 *)&(lam.fn),(zzn2 *)&(extra1.fn));
}

BOOL ECn2::add(const ECn2& w,const ZZn2& lam)
{
    return ecn2_add1(&(w.fn),&(this->fn),(zzn2 *)&(lam.fn));
}

BOOL ECn2::add(const ECn2& w,const ZZn2& lam,const ZZn2& extra1,const ZZn2& extra2)
{
    return ecn2_add3(&(w.fn),&(this->fn),(zzn2 *)&(lam.fn),(zzn2 *)&(extra1.fn),(zzn2 *)&(extra2.fn));
}

#endif

#ifndef MR_NO_ECC_MULTIADD

ECn2 mul(const Big& a,const ECn2& P,const Big& b,const ECn2& Q)
{
    ECn2 R;
    ecn2_mul2_jsf(a.getbig(),&(P.fn),b.getbig(),&(Q.fn),&(R.fn));
	R.norm();
    return R;
}

// standard MIRACL multi-addition


ECn2 mul4(ECn2* P,const Big* b)
{
    int i,n=4;
	ECn2 R;
	big x[4];
	ecn2 p[4];
    
    for (i=0;i<n;i++)
	{
		x[i]=b[i].getbig();
		p[i]=P[i].fn;
	}

	ecn2_mult4(x,p,&(R.fn));

	R.norm();
	return R;
}

#ifndef MR_STATIC

ECn2 mul(int n,ECn2* P,const Big* b)
{
    int i;
	ECn2 R;
    big  *x=(big  *)mr_alloc(n,sizeof(big));
	ecn2 *p=(ecn2 *)mr_alloc(n,sizeof(ecn2));
    for (i=0;i<n;i++)
	{
		x[i]=b[i].getbig();
		p[i]=P[i].fn;
	}

	ecn2_multn(n,x,p,&(R.fn));

	mr_free(p); mr_free(x);
	R.norm();
	return R;
}
#endif

#endif

