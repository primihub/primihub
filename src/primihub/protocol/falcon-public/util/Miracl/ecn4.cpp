
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
 *    MIRACL  C++ Implementation file ecn4.cpp
 *
 *    AUTHOR  : M. Scott
 *  
 *    PURPOSE : Implementation of class ECn4  (Elliptic curves over n^4)
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 */


#include "ecn4.h"

using namespace std;

void ECn4::get(ZZn4& a,ZZn4& b) const
{a=x;b=y;}

void ECn4::get(ZZn4& a) const
{a=x;}

BOOL ECn4::iszero(void) const
{if (marker==MR_EPOINT_INFINITY) return TRUE; return FALSE;}

BOOL ECn4::set(const ZZn4& xx,const ZZn4& yy)
{ 
	if (yy*yy!=rhs(xx)) return FALSE;
	x=xx;
	y=yy;
	marker=MR_EPOINT_NORMALIZED;
	return TRUE;
}

BOOL ECn4::set(const ZZn4& xx)
{ 
	ZZn4 w=rhs(xx);

	if (!w.iszero())
	{
		w=sqrt(w); 
		if (w.iszero()) return FALSE;
	}

	x=xx;
	y=w;
	marker=MR_EPOINT_NORMALIZED;
	return TRUE;
}

ECn4 operator-(const ECn4& a) 
{ECn4 w; 
 if (a.marker!=MR_EPOINT_INFINITY) 
   {w.x=a.x; w.y=-a.y; w.marker=a.marker;} 
 return w; 
}  

ECn4& ECn4::operator*=(const Big& k)
{
    int i,j,n,nb,nbs,nzs;
    ECn4 p2,pt,t[11];
    Big h,kk;

    if (k==0)
    {
        clear();
        return *this;
    }
    if (k==1)
    {
        return (*this);
    }

    pt=*this;
    kk=k;
    if (kk<0)
    {
        pt=-pt;
        kk=-k;
    }
    h=3*kk;

    p2=pt+pt; 
    t[0]=pt;
    for (i=1;i<=10;i++)
        t[i]=t[i-1]+p2;

// Left to Right method

    nb=bits(h);
    for (i=nb-2;i>=1;)
    {
        n=naf_window(kk,h,i,&nbs,&nzs,11);
        for (j=0;j<nbs;j++) pt+=pt;
        if (n>0) pt+=t[n/2];
        if (n<0) pt-=t[(-n)/2];
        i-=nbs;
        if (nzs)
        {
            for (j=0;j<nzs;j++) pt+=pt;
            i-=nzs;
        }
    }
    *this=pt;
    return *this;
}

ECn4 operator*(const Big& r,const ECn4& P)
{
    ECn4 T=P;
    T*=r;
    return T;
}

#ifndef MR_NO_STANDARD_IO

ostream& operator<<(ostream& s,const ECn4& b)
{
    ZZn4 x,y;
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

ECn4 operator+(const ECn4& a,const ECn4& b)
{ECn4 c=a; c+=b; return c;}

ECn4 operator-(const ECn4& a,const ECn4& b)
{ECn4 c=a; c-=b; return c;}

ECn4& ECn4::operator-=(const ECn4& z)
{ECn4 t=(-z); *this+=t; return *this; }

ECn4& ECn4::operator+=(const ECn4& z)
{
    ZZn4 lam;
    add(z,lam);
    return *this;
}

BOOL ECn4::add(const ECn4& z,ZZn4& lam)
{
    int twist=get_mip()->TWIST;

    if (marker==MR_EPOINT_INFINITY)
    {
        *this=z;
        return FALSE;
    }
    if (z.marker==MR_EPOINT_INFINITY)
    {
        return FALSE;
    }

    if (x!=z.x)
    {
        ZZn4 t=y;  t-=z.y;
        ZZn4 t2=x; t2-=z.x;     
        lam=t; lam/=t2;

        x+=z.x; t=lam; t*=t; t-=x; x=t;  
        y=z.x; y-=x; y*=lam; y-=z.y;   

    }
    else
    {
        if (y!=z.y || y.iszero())
        {
            clear();
            lam=(ZZn4)1; 
            return TRUE;    // any non-zero value
        }
        ZZn4 t=x;
        ZZn4 t2=x;

     //   lam=(3*(x*x)+getA())/(y+y);

        lam=x;
        lam*=lam;
        lam*=3;
        if (twist==MR_QUADRATIC)
        {
        //    ZZn4 a4;
        //    ZZn2 x((ZZn)0,getA());
        //    a4.set(x,(ZZn2)0);     // A*i^4
			lam+=txx(  (ZZn2)getA() );
        //    lam+=a4;
        }
        else  lam+=getA();
        lam/=(y+y);       

        t2+=x;
        x=lam;
        x*=x;
        x-=t2;
         
        t-=x;
        t*=lam;
        t-=y;
        y=t;           
    }

    marker=MR_EPOINT_GENERAL;    
    return TRUE;
}

#ifndef MR_NO_ECC_MULTIADD
#ifndef MR_STATIC

ECn4 mul(int n,ECn4* P,const Big* b)
{
    int k,j,i,m,nb,ea;
    ECn4 *G;
    ECn4 R;
    m=1<<n;
    G=new ECn4[m];

 // precomputation
    
    for (i=0,k=1;i<n;i++)
    {
        for (j=0; j < (1<<i) ;j++)
        {
            if (j==0)   G[k]=P[i];
            else        G[k]=G[j]+P[i];      
            k++;
        }
    }

    nb=0;
    for (j=0;j<n;j++) 
        if ((k=bits(b[j]))>nb) nb=k;

    for (i=nb-1;i>=0;i--) 
    {
        ea=0;
        k=1;
        for (j=0;j<n;j++)
        {
            if (bit(b[j],i)) ea+=k;
            k<<=1;
        }
        R+=R;;
        if (ea!=0) R+=G[ea];
    }
    delete [] G;
    return R;
}

#endif
#endif