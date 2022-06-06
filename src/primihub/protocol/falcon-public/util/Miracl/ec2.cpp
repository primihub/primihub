
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
 *    MIRACL  C++  functions ec2.cpp
 *
 *    AUTHOR  :    M. Scott
 *             
 *    PURPOSE :    Implementation of class EC2 functions 
 *    NOTE    :    Must be used in conjunction with big.h and big.cpp
 *
 */

#include "ec2.h"

int EC2::get(Big& x,Big& y) const 
        {return epoint2_get(p,x.getbig(),y.getbig());}
int EC2::get(Big& x) const   
        {return epoint2_get(p,x.getbig(),x.getbig());}
#ifndef MR_STATIC
void EC2::getx(Big& x) const
        {epoint2_getxyz(p,x.getbig(),NULL,NULL);}
void EC2::getxy(Big& x,Big &y) const
        {epoint2_getxyz(p,x.getbig(),y.getbig(),NULL);}
void EC2::getxyz(Big& x,Big &y,Big &z) const
        {epoint2_getxyz(p,x.getbig(),y.getbig(),z.getbig());}
#endif

BOOL EC2::iszero() const
        {if (p->marker==MR_EPOINT_INFINITY) return TRUE; return FALSE;}

epoint * EC2::get_point() const
{ return p; }

EC2 operator-(const EC2& e)
{ EC2 t=e; epoint2_negate(t.p); return t;}


#ifndef MR_NO_ECC_MULTIADD
EC2 mul(const Big& e1,const EC2& p1,const Big& e2,const EC2& p2)
{ 
    EC2 t; 
    ecurve2_mult2(e1.getbig(),p1.get_point(),e2.getbig(),p2.get_point(),t.get_point());
    return t;
}
#endif

EC2 operator*(const Big& e,const EC2& b)
{
    EC2 t;
    ecurve2_mult(e.getbig(),b.p,t.p);
    return t;
}

#ifndef MR_STATIC
#ifndef MR_NO_ECC_MULTIADD

EC2 mul(int n,const Big *y,EC2 *x)
{
    EC2 w;
    int i;
    big *a=(big *)mr_alloc(n,sizeof(big));
    epoint **b=(epoint **)mr_alloc(n,sizeof(epoint *));
    for (i=0;i<n;i++)
    {
        a[i]=y[i].getbig();
        b[i]=x[i].p;
    }
    ecurve2_multn(n,a,b,w.p);

    mr_free(b);
    mr_free(a);
    return w;  
}

void multi_add(int m,EC2 *x, EC2 *w)
{
    int i;
    epoint **xp=(epoint **)mr_alloc(m,sizeof(epoint *));
    epoint **wp=(epoint **)mr_alloc(m,sizeof(epoint *));
    for (i=0;i<m;i++)
    {
        xp[i]=x[i].p;
        wp[i]=w[i].p;
    }
    ecurve2_multi_add(m,xp,wp);
    mr_free(wp);
    mr_free(xp);
}
#endif
#endif

#ifndef MR_NO_STANDARD_IO

ostream& operator<<(ostream& s,const EC2& b)
{
    Big x,y;
    if (b.iszero())
    {
        s << "(Infinity)";
        return s;
    }
    b.get(x,y);
    s << "(" << x << "," << y << ")";
    return s;
}

#endif


