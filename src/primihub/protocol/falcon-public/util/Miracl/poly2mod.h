/*
 * C++ class to implement a polynomial type and to allow 
 * arithmetic on polynomials whose elements are from
 * the finite field GF(2^m). 
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 * This type is automatically reduced
 * wrt a polynomial modulus.
 *
 * See Knuth The Art of Computer Programming Vol.2, Chapter 4.6 
 */

#ifndef POLY2MOD_H
#define POLY2MOD_H

#include "poly2.h"

extern Poly2 Modulus;

class Poly2Mod
{
public:
    Poly2 p;

    Poly2Mod() { }
    Poly2Mod(const Poly2& m)     {p=m%Modulus; }
    Poly2Mod(const Poly2Mod& m)  {p=m.p; }
    void clear()               {p.clear();}
    term2* addterm(const GF2m& z,int i,term2 *pos=NULL) {return p.addterm(z,i,pos);}
    void multerm(const GF2m& z,int i) {p.multerm(z,i);}
    GF2m F(const GF2m& z)        {return p.F(z); }
    GF2m coeff(int i) const;
    Poly2Mod& operator=(int m)               {p=m; return *this;}
    Poly2Mod& operator=(const Poly2Mod& m)    {p=m.p; return *this;}
    Poly2Mod& operator=(const Poly2& m)       {reduce(m,*this); return *this;}
    Poly2Mod& operator+=(const Poly2Mod& m)   {p=(p+m.p)%Modulus;return *this;}
    Poly2Mod& operator+=(const GF2m& z)       {addterm(z,0); return *this; }
    Poly2Mod& operator*=(const GF2m& z)       {p*=z;return *this;}
    Poly2Mod& operator/=(const GF2m& z)       {p/=z;return *this;}
    Poly2Mod& operator*=(const Poly2Mod&);
    
    friend void setmod(const Poly2&);
    friend void reduce(const Poly2&,Poly2Mod&);

    friend BOOL iszero(const Poly2Mod&);     
    friend BOOL isone(const Poly2Mod&); 
    friend int degree(const Poly2Mod&); 

    friend Poly2Mod operator*(const Poly2Mod&,const Poly2Mod&); 
    friend Poly2Mod operator+(const Poly2Mod&,const Poly2Mod&);
    friend Poly2Mod operator+(const Poly2Mod&,const GF2m&);

    friend Poly2Mod operator*(const Poly2Mod&,const GF2m&);
    friend Poly2Mod operator*(const GF2m&,const Poly2Mod&);
    friend Poly2Mod compose(const Poly2Mod&,const Poly2Mod&);    

    friend Poly2Mod operator/(const Poly2Mod&,const GF2m&);
    friend Poly2 gcd(const Poly2Mod&);
    friend Poly2Mod inverse(const Poly2Mod&);
    friend Poly2Mod pow(const Poly2Mod&,const Big&);
    friend ostream& operator<<(ostream&,const Poly2Mod&);
    ~Poly2Mod() {}
};

extern void setmod(const Poly2&);

#endif

