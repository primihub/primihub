/*
 *    MIRACL C++ Implementation file SF2m12x.cpp
 *
 *    AUTHOR   : M. Scott
 *
 *    PURPOSE  :    Implementation of class SF2m12x (special 12-th extension over 2^m)
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 */

#include "sf2m12x.h"

using namespace std;

void SF2m12x::get(GF2m6x& a,GF2m6x& b)
{a=U; b=V;}

SF2m12x& SF2m12x::powq()
{
    int r=(get_mip())->M%12;
    U.powq();
    V.powq();
    if (r==0) return *this;

    GF2m6x S(0,0,0,1,0,1),T=0;
                                  // x^3 + x^5
    if (r==7)
    { // which it is for M=103!
        T.set(1,0,0,1,0,1);
        U+=(T*V);
        return *this;
    }
    
    for (int i=0;i<r;i++)
    {
        T+=S;
        S*=S;
    }

    U+=(T*V);
    
    return *this;
}

SF2m12x& SF2m12x::conj()
{
    U += V;
    return *this;
}

SF2m12x& SF2m12x::operator*=(const SF2m12x& b)
{
    if (this==&b)
    { // (U+s0.V)(U+s0.V) = U^2+(x^3+x^5)*V^2 + so.V^2
        V*=V;
        U*=U;
        U+=smul(V);
        return *this;
    }
    else
    { // Use Karatsuba

       GF2m6x UX,VY;
       UX=U*b.U;
       VY=V*b.V;
       V=(U+V)*(b.U+b.V)+UX;  
       U=UX+smul(VY);
       
       return *this;
    }
}

SF2m12x& SF2m12x::operator*=(const GF2m& b)
{ 
   U*=b; V*=b;
   return *this;
}

SF2m12x& SF2m12x::operator/=(const GF2m& b)
{
   GF2m ib=(GF2m)1/b;
   U*=ib; V*=ib;
   return *this;
}

void SF2m12x::invert()
{
    GF2m6x S=U+V;
    GF2m6x D=U*S+smul(V*V);
    D.invert();
    U=S*D;
    V*=D;
    return;
}

SF2m12x& SF2m12x::operator/=(const SF2m12x& a)
{
    SF2m12x b=a;
    b.invert();
    *this *= b;
    return *this;
}

SF2m12x operator+(const SF2m12x& a,const SF2m12x& b)
{ SF2m12x r=a; r+=b; return r;}

SF2m12x operator+(const SF2m12x& a,const GF2m& b)
{ SF2m12x r=a; r+=b; return r;}
 
SF2m12x operator+(const GF2m& a,const SF2m12x& b)
{ SF2m12x r=b; r+=a; return r;}

SF2m12x operator*(const SF2m12x& a,const SF2m12x& b)
{ 
    SF2m12x r=a;
    if (&a!=&b) r*=b; 
    else r*=r;
    return r;
}

SF2m12x operator/(const SF2m12x& a,const SF2m12x& b)
{ 
    SF2m12x r=a;
    r/=b; 
    return r;
}

SF2m12x operator*(const SF2m12x& a,const GF2m& b)
{ SF2m12x r=a; r*=b; return r;}
SF2m12x operator*(const GF2m& a,const SF2m12x& b)
{ SF2m12x r=b; r*=a; return r;}

SF2m12x randx12(void)
{
    SF2m12x r;
    r.U=randx6();
    r.V=randx6();                     
    return r;
}

GF2m6x smul(const GF2m6x& a)
{ // special multiplication by x^3+x^5
    GF2m6x r;
/*
    GF2m m,t[2*FM];
    int i;

    t[10]=a.x[5]; t[9]=a.x[4]; t[8]=a.x[3]+a.x[5]; t[7]=a.x[2]+a.x[4];
    t[6]=a.x[1]+a.x[3]; t[5]=a.x[0]+a.x[2]; t[4]=a.x[1]; t[3]=a.x[0];

    for (i=2*FM-2;i>=FM;i--)
    { // reduce mod x^FM+x^FA+x^FB+x^FC+1
        m=t[i]; t[i]=0;
        t[i-FM]+=m;
        t[i-(FM-FA)]+=m; 
#ifdef FB
            t[i-(FM-FB)]+=m; 
            t[i-(FM-FC)]+=m; 
#endif
    }
    for (i=0;i<FM;i++) r.x[i]=t[i];
*/

    r.x[5]=a.x[0]+a.x[1]+a.x[3]+a.x[4];
    r.x[4]=a.x[1]+a.x[2]+a.x[4];
    r.x[3]=a.x[0]+a.x[1]+a.x[3]+a.x[5];
    r.x[2]=a.x[1]+a.x[2]+a.x[3]+a.x[5];
    r.x[1]=r.x[2]+a.x[1];       //a.x[2]+a.x[3]+a.x[5];
    r.x[0]=r.x[4]+a.x[5];       //a.x[1]+a.x[2]+a.x[4]+a.x[5];

    return r;
}

SF2m12x mul(const SF2m12x& a,const SF2m12x& b)
{ // special purpose multiplication code for pairing calculation
    SF2m12x r;
    GF2m6x x(0,0,0,1,0,1);
    r.set(mul(a.U,b.U)+x,a.U+b.U+(GF2m6x)1);
    return r;
}

SF2m12x mull(const SF2m12x& a,const SF2m12x& b)
{
    SF2m12x r;
    GF2m6x UX,VY;
    UX=a.U*b.U;
    VY=mul(a.V,b.V);
    r.V=(a.U+a.V)*(b.U+b.V)+UX;  
    r.U=UX+smul(VY);
    return r;
}

SF2m12x pow(const SF2m12x& a,const Big& k)
{
    int i,j,nb,n,nbw,nzs;
    SF2m12x u,u2,t[16];
    if (k.iszero()) return (SF2m12x)1;
    u=a;
    if (k.isone()) return u;

//
// Prepare table for windowing
//
    u2=(u*u);
    t[0]=u;

    for (i=1;i<16;i++)
        t[i]=u2*t[i-1];

// Left to right method - with windows

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

ostream& operator<<(ostream& os,const SF2m12x& x)
{
    SF2m12x u=x;
 
    os << "{" << u.U << "," << u.V << "}" << endl;

    return os;
}
