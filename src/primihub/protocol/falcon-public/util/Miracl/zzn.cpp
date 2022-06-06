
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
 *    MIRACL  C++  functions zzn.cpp
 *
 *    AUTHOR  :    M. Scott
 *             
 *    PURPOSE :    Implementation of class ZZn functions using Montgomery
 *                 representation
 *    NOTE    :    Must be used in conjunction with big.h and big.cpp
 *
 */

#include "zzn.h"

big ZZn::getzzn(void) const         
         { return fn;}

ZZn& ZZn::operator/=(int i)
{
 if (i==1) return *this; 
 if (i==2)
 { // make a special effort... modulus is odd
    nres_div2(fn,fn);
    return *this;
 }
 ZZn x=i;
 nres_moddiv(fn,x.fn,fn);
 return *this;
}

BOOL ZZn::iszero() const
{ if (size(fn)==0) return TRUE; return FALSE;}

ZZn operator-(const ZZn& b)
{ZZn x=b; nres_negate(x.fn,x.fn); return x;}

ZZn operator+(const ZZn& b,int i)
{ZZn abi=b; abi+=i; return abi;}
ZZn operator+(int i, const ZZn& b)
{ZZn aib=b; aib+=i; return aib;}
ZZn operator+(const ZZn& b1, const ZZn& b2)
{ZZn abb=b1; abb+=b2; return abb;}

ZZn operator-(const ZZn& b, int i)
{ZZn mbi=b; mbi-=i; return mbi;}
ZZn operator-(int i, const ZZn& b)
{ZZn mib=i; mib-=b; return mib;}
ZZn operator-(const ZZn& b1, const ZZn& b2)
{ZZn mbb=b1; mbb-=b2;  return mbb;}

ZZn operator*(const ZZn& b,int i)
{ZZn xbb=b; xbb*=i; return xbb;}
ZZn operator*(int i, const ZZn& b)
{ZZn xbb=b; xbb*=i; return xbb;}
ZZn operator*(const ZZn& b1, const ZZn& b2)
{ZZn xbb=b1; xbb*=b2; return xbb;}

ZZn operator/(const ZZn& b1, int i)
{ZZn z=b1; z/=i; return z; }

ZZn operator/(int i, const ZZn& b2)
{ZZn z=i; nres_moddiv(z.fn,b2.fn,z.fn); return z;}
ZZn operator/(const ZZn& b1, const ZZn& b2)
{ZZn z=b1; z/=b2; return z;}

ZZn pow( const ZZn& b1, const Big& b2)
{ZZn z; nres_powmod(b1.fn,b2.getbig(),z.fn);return z;}

ZZn pow( const ZZn& b,int i)
{ZZn z; Big ib=i; nres_powmod(b.fn,ib.getbig(),z.fn); return z;}

ZZn pow( const ZZn& b1, const Big& b2, const ZZn& b3,const Big& b4)
{ZZn z; nres_powmod2(b1.fn,b2.getbig(),b3.fn,b4.getbig(),z.fn); return z;}

int jacobi(const ZZn& x)
{redc(x.fn,get_mip()->w1); return jack(get_mip()->w1,get_mip()->modulus); }

#ifndef MR_NO_RAND
ZZn randn(void)
{ZZn z; bigrand(get_mip()->modulus,z.fn); return z;}
#endif
BOOL qr(const ZZn& x)
{redc(x.fn,get_mip()->w1); if (jack(get_mip()->w1,get_mip()->modulus)==1) return TRUE; return FALSE; }

BOOL qnr(const ZZn& x)
{redc(x.fn,get_mip()->w1); if (jack(get_mip()->w1,get_mip()->modulus)==-1) return TRUE; return FALSE;}

ZZn one(void) 
{
    ZZn w;
    w=get_mip()->one;
    return w;
}

ZZn getA(void)
{
 ZZn w; 
 if (get_mip()->Asize<MR_TOOBIG) w=get_mip()->Asize;
 else w=get_mip()->A;
 return w;
}

ZZn getB(void)
{ 
 ZZn w; 
 if (get_mip()->Bsize<MR_TOOBIG) w=get_mip()->Bsize;
 else w=get_mip()->B;
 return w;
}

#ifndef MR_STATIC

ZZn pow(int n,ZZn *a,Big *b)
{
   ZZn z;
   int i;
   big *x=(big *)mr_alloc(n,sizeof(big));
   big *y=(big *)mr_alloc(n,sizeof(big));
   for (i=0;i<n;i++)
   {
       x[i]=a[i].fn;
       y[i]=b[i].getbig();
   }
   nres_powmodn(n,x,y,z.fn);

   mr_free(y); mr_free(x);
   return z;
}

#endif

// fast ZZn2 powering using lucas functions..

ZZn powl(const ZZn& x,const Big& k)
{
    return luc(2*x,k)/2;
}

ZZn luc( const ZZn& b1, const Big& b2, ZZn *b3)
{ZZn z; if (b3!=NULL) nres_lucas(b1.fn,b2.getbig(),b3->fn,z.fn); 
        else          nres_lucas(b1.fn,b2.getbig(),z.fn,z.fn); 
 return z;}


ZZn sqrt(const ZZn& b)
{ZZn z; nres_sqroot(b.fn,z.fn); return z;}

#ifndef MR_NO_STANDARD_IO

ostream& operator<<(ostream& s,const ZZn& xx)
{
    ZZn b=xx;
    Big x=(Big)b;
    s << x;
    return s;    
}

#endif
