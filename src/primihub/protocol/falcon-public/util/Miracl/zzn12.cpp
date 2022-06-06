
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
 *    MIRACL  C++ Implementation file zzn12.cpp
 *
 *    AUTHOR  : M. Scott
 *  
 *    PURPOSE : Implementation of class ZZn12  (Arithmetic over n^12)
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 *    NOTE: - The irreducible polynomial is assumed to be of the form 
 *            x^6-i, where i is  
 *            1+sqrt(-1) if p=3 mod 8
 *            or sqrt(-2) if p=5 mod 8 
 *            or sqrt(2+sqrt(-1)), for p=7 mod 8 and p=2,3 mod 5 
 *
 */

#include "zzn12.h"

using namespace std;

// Frobenius...

ZZn12& ZZn12::powq(const ZZn2& X)
{
    ZZn2 W=X*X;
    BOOL ku=unitary;
	a.powq(W); b.powq(W);
    b*=X;
    unitary=ku;
    return *this;
}

void ZZn12::get(ZZn6 &x,ZZn6 &y) const 
{x=a; y=b; } 

void ZZn12::get(ZZn6& x) const
{x=a; }

ZZn12& ZZn12::operator*=(const ZZn12& x)
{ 
    if (&x==this)
    {  
/* See Stam & Lenstra, "Efficient subgroup exponentiation in Quadratic .. Extensions", CHES 2002 */
        if (unitary)
        {
            ZZn6 t=b; t*=t;
            b+=a; b*=b;
            b-=t;
            a=tx(t);
            b-=a;
            a+=a; a+=one();
            b-=one();
       // cout << "in here" << endl;
        }
        else 
        {
            ZZn6 t=a; t+=b;
            ZZn6 t2=a; t2+=tx(b);
            t*=t2;
            b*=a;
            t-=b;
            t-=tx(b);
            b+=b;
            a=t;
        }
    }
    else
    { // Karatsuba multiplication
        ZZn6 ac=a; ac*=x.a;
        ZZn6 bd=b; bd*=x.b;
        ZZn6 t=x.a; t+=x.b;
        b+=a; b*=t; b-=ac; b-=bd;
        a=ac; a+=tx(bd);
        
        if (!x.unitary) unitary=FALSE;
    }

    return *this;
}

ZZn12 conj(const ZZn12& x)
{
    ZZn12 u=x;
    u.conj();
    return u;
}

ZZn12 inverse(const ZZn12 &w)
{
    ZZn12 y=conj(w);
    if (w.unitary) return y;
    ZZn6 u=w.a;
    ZZn6 v=w.b;
    u*=u;
    v*=v;
    u-=tx(v);
    u=inverse(u);
    y*=u;
    return y;
}

ZZn12& ZZn12::operator/=(const ZZn12& x)
{ // inversion 
 *this *= inverse(x);
 if (!x.unitary) unitary=FALSE;
 return *this;
}

ZZn12& ZZn12::operator/=(const ZZn6& x)
{ // inversion 
 *this *= inverse(x);
 unitary=FALSE;
 return *this;
}

ZZn12 operator+(const ZZn12& x,const ZZn12& y) 
{ZZn12 w=x; w+=y; return w;} 

ZZn12 operator+(const ZZn12& x,const ZZn6& y) 
{ZZn12 w=x; w+=y; return w; } //

ZZn12 operator-(const ZZn12& x,const ZZn12& y) 
{ZZn12 w=x; w-=y; return w; } 

ZZn12 operator-(const ZZn12& x,const ZZn6& y) 
{ZZn12 w=x; w-=y; return w; } //

ZZn12 operator-(const ZZn12& x) 
{ZZn12 w; w.a=-x.a; w.b=-x.b; w.unitary=FALSE; return w; } 

ZZn12 operator*(const ZZn12& x,const ZZn12& y)
{
    ZZn12 w=x;
    if (&x==&y) w*=w;
    else        w*=y;    
    return w;
}

ZZn12 operator*(const ZZn12& x,const ZZn6& y)
{ZZn12 w=x; w*=y; return w;} //

ZZn12 operator*(const ZZn6& y,const ZZn12& x)
{ZZn12 w=x; w*=y; return w;} //

ZZn12 operator*(const ZZn12& x,int y)
{ZZn12 w=x; w*=y; return w;}
                                         
ZZn12 operator*(int y,const ZZn12& x)
{ZZn12 w=x; w*=y; return w;}

ZZn12 operator/(const ZZn12& x,const ZZn12& y)
{ZZn12 w=x; w/=y; return w;}

ZZn12 operator/(const ZZn12& x,const ZZn6& y)
{ZZn12 w=x; ZZn6 j=inverse(y); w.a*=j; w.b*=j; w.unitary=FALSE; return w;} //

#ifndef MR_NO_RAND
ZZn12 randn12(void)
{ZZn12 w; w.a=randn6(); w.b=randn6(); w.unitary=FALSE; return w;}
#endif
#ifndef MR_NO_STANDARD_IO

ostream& operator<<(ostream& s,ZZn12& b)
{
    int i;
    ZZn6 x,y;
    b.get(x,y);
    s << "[" << x << "," << y << "]";
    return s;    
}

#endif

// Left to right method - with windows

ZZn12 pow(const ZZn12* t,const ZZn12& x,const Big& k)
{
    int i,j,nb,n,nbw,nzs;
    ZZn12 u=x;

    if (k==0) return (ZZn12)one();
    if (k==1) return u;

    nb=bits(k);
    if (nb>1) for (i=nb-2;i>=0;)
    {
        n=window(k,i,&nbw,&nzs,5);
        for (j=0;j<nbw;j++) u*=u;
        if (n>0) u*=t[n/2];
        i-=nbw;
        if (nzs)
        {
            for (j=0;j<nzs;j++) u*=u;
            i-=nzs;
        }
    }
    return u;
}

void precompute(const ZZn12& x,ZZn12* t)
{
    int i;
    ZZn12 u2,u=x;
    u2=(u*u);
    t[0]=u;
   
    for (i=1;i<16;i++)
        t[i]=u2*t[i-1];

}

/*
ZZn12 pow(const ZZn12& x,const Big& k)
{
    ZZn12 u,t[16];

    if (k==0) return (ZZn12)one();
    u=x;
    if (k==1) return u;
//
// Prepare table for windowing
//
    precompute(u,t);
    return pow(t,u,k);
}
*/

// If k is low Hamming weight this will be just as good..

ZZn12 pow(const ZZn12& x,const Big& k)
{
    int i,j,nb,n;
    ZZn12 u=x;
    Big e=k;
    BOOL invert_it=FALSE;

    if (e==0) return (ZZn12)one();

    if (e<0) 
    {
        e=-e;
        invert_it=TRUE;
    }

    nb=bits(e);
    if (nb>1) for (i=nb-2;i>=0;i--)
    {
        u*=u;
        if (bit(e,i)) u*=x;
    }

    if (invert_it) u=inverse(u);

    return u;
}


// standard MIRACL multi-exponentiation

ZZn12 pow(int n,const ZZn12* x,const Big* b)
{
    int k,j,i,m,nb,ea;
    ZZn12 *G;
    ZZn12 r;
    m=1<<n;
    G=new ZZn12[m];

 // precomputation
    
    for (i=0,k=1;i<n;i++)
    {
        for (j=0; j < (1<<i) ;j++)
        {
            if (j==0)   G[k]=x[i];
            else        G[k]=G[j]*x[i];      
            k++;
        }
    }

    nb=0;
    for (j=0;j<n;j++) 
        if ((k=bits(b[j]))>nb) nb=k;

    r=1;
    for (i=nb-1;i>=0;i--) 
    {
        ea=0;
        k=1;
        for (j=0;j<n;j++)
        {
            if (bit(b[j],i)) ea+=k;
            k<<=1;
        }
        r*=r;
        if (ea!=0) r*=G[ea];
    }
    delete [] G;
    return r;
}
