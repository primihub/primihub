/*
 * C++ class to implement a polynomial type and to allow 
 * arithmetic on polynomials whose elements are from
 * the finite field mod p. 
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 * This type is automatically reduced
 * wrt a polynomial Modulus.
 *
 * See Knuth The Art of Computer Programming Vol.2, Chapter 4.6 
 */

#ifndef POLYMOD_H
#define POLYMOD_H

#include "poly.h"

extern Poly Modulus;

class PolyMod
{
public:
    Poly p;

    PolyMod() { }
    PolyMod(const Poly& m)     {p=m%Modulus; }
    PolyMod(const PolyMod& m)  {p=m.p; }
    void clear()               {p.clear();}
    term* addterm(const ZZn& z,int i,term *pos=NULL) {return p.addterm(z,i,pos);}
    void multerm(const ZZn& z,int i) {p.multerm(z,i);}
    ZZn F(const ZZn& z)        {return p.F(z); }
    ZZn coeff(int i) const;
    PolyMod& operator=(int m)               {p=m; return *this;}
    PolyMod& operator=(const PolyMod& m)    {p=m.p; return *this;}
    PolyMod& operator=(const Poly& m)       {reduce(m,*this); return *this;}
    PolyMod& operator+=(const PolyMod& m)   {p=(p+m.p)%Modulus;return *this;}
    PolyMod& operator-=(const PolyMod& m)   {p=(p-m.p)%Modulus;return *this;}
    PolyMod& operator+=(const ZZn& m)       {addterm(m,0); return *this;}
    PolyMod& operator-=(const ZZn& m)       {addterm((-m),0); return *this;}
    PolyMod& operator*=(const ZZn& z)       {p*=z;return *this;}
    PolyMod& operator/=(const ZZn& z)       {p/=z;return *this;}
    PolyMod& operator*=(const PolyMod&);
    
    friend void setmod(const Poly&);
    friend void reduce(const Poly&,PolyMod&);

    friend BOOL iszero(const PolyMod&);     
    friend BOOL isone(const PolyMod&); 
    friend int degree(const PolyMod&); 

    friend PolyMod operator*(const PolyMod&,const PolyMod&); 
    friend PolyMod operator-(const PolyMod&,const PolyMod&);
    friend PolyMod operator+(const PolyMod&,const PolyMod&);
    friend PolyMod operator-(const PolyMod&,const ZZn&);
    friend PolyMod operator+(const PolyMod&,const ZZn&);

    friend BOOL operator==(const PolyMod&,const PolyMod&);
    friend BOOL operator!=(const PolyMod&,const PolyMod&);

    friend PolyMod operator*(const PolyMod&,const ZZn&);
    friend PolyMod operator*(const ZZn&,const PolyMod&);
    friend PolyMod compose(const PolyMod&,const PolyMod&);    

    friend PolyMod operator/(const PolyMod&,const ZZn&);
    friend Poly gcd(const PolyMod&);
    friend PolyMod pow(const PolyMod&,const Big&);
    friend ostream& operator<<(ostream& ,const PolyMod&);
    ~PolyMod() {}
};

extern void setmod(const Poly&);

#endif

