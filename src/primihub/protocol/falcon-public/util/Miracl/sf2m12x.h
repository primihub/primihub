/*
 *    MIRACL C++ Headerfile SF2m12x.h
 *
 *    AUTHOR  : M. Scott
 *
 *    PURPOSE : Definition of class SF2m12x - Arithmetic over the special extension 
 *              field GF(2^12m). Representation is a quadratic extension of GF2m6x
 *
 *    NOTE    : The underlying GF(2^m) field basis must be set by the modulo() routine
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 */


#ifndef SF2m12x_H
#define SF2m12x_H

#include <iostream>
#include "gf2m6x.h"  

class SF2m12x
{
    GF2m6x U,V;
public:
    SF2m12x()                    {}
    SF2m12x(const SF2m12x & b)   {U=b.U; V=b.V; }
    SF2m12x(const GF2m6x& b)     {U=b;}
    SF2m12x(int i)               {U=i;}
    SF2m12x(const GF2m& a,const GF2m& b, const GF2m& c=0, const GF2m& d=0, const GF2m& e=0, const GF2m& f=0, 
    const GF2m& g=0, const GF2m& h=0, const GF2m& i=0, const GF2m& j=0, const GF2m& k=0, const GF2m& l=0)
    {U.set(a,b,c,d,e,f); V.set(g,h,i,j,k,l);} 
    SF2m12x(const Big& a) {U=(GF2m)a;}

    void set(const GF2m& a)      {U=a; V=0;}
    void set(const GF2m6x& a,const GF2m6x& b=(GF2m6x)0) {U=a; V=b;}
    void set(const GF2m& a,const GF2m& b=0, const GF2m& c=0, const GF2m& d=0, const GF2m& e=0, const GF2m& f=0, 
    const GF2m& g=0, const GF2m& h=0, const GF2m& i=0, const GF2m& j=0, const GF2m& k=0, const GF2m& l=0)      
    {U.set(a,b,c,d,e,f); V.set(g,h,i,j,k,l);}

    void invert();

    void get(GF2m6x&,GF2m6x&);
    void clear() {U=V=0;}

    BOOL iszero() const 
    {if (U.iszero() && V.iszero()) return TRUE; return FALSE; } 

    BOOL isunity() const
    {if (U.isunity() && V.iszero()) return TRUE; return FALSE; }
   
    SF2m12x& powq();
    SF2m12x& conj();

    SF2m12x& operator=(const SF2m12x& b)
        { U=b.U; V=b.V; return *this; }
    SF2m12x& operator=(const GF2m& b)
        { U=b; V=0; return *this; }
    SF2m12x& operator=(int b)
        { U=b; V=0; return *this; }
    SF2m12x& operator+=(const SF2m12x& b) 
        { U+=b.U; V+=b.V; return *this; }
    SF2m12x& operator+=(const GF2m& b)
        {U+=b; return *this; }
    SF2m12x& operator*=(const SF2m12x&);
    SF2m12x& operator*=(const GF2m&);
    SF2m12x& operator/=(const SF2m12x&);
    SF2m12x& operator/=(const GF2m&);

    friend SF2m12x operator+(const SF2m12x&,const SF2m12x&);
    friend SF2m12x operator+(const SF2m12x&,const GF2m&);
    friend SF2m12x operator+(const GF2m&,const SF2m12x&);
    
    friend SF2m12x operator*(const SF2m12x&,const SF2m12x&);
    friend SF2m12x operator*(const SF2m12x&,const GF2m&);
    friend SF2m12x operator*(const GF2m&,const SF2m12x&);
    friend SF2m12x operator/(const SF2m12x&,const SF2m12x&); 
    friend SF2m12x mul(const SF2m12x&,const SF2m12x&);
    friend SF2m12x mull(const SF2m12x&,const SF2m12x&);

    friend BOOL operator==(const SF2m12x& a,const SF2m12x& b)
    {if (a.U==b.U && a.V==b.V) return TRUE; return FALSE;}

    friend BOOL operator!=(const SF2m12x& a,const SF2m12x& b)
    {if (a.U!=b.U || a.V==b.V) return TRUE; return FALSE; }

    friend SF2m12x pow(const SF2m12x&,const Big&);
    
    friend SF2m12x randx12(void);
    
    friend ostream& operator<<(ostream&,const SF2m12x&);

    ~SF2m12x() {} ;
};

extern SF2m12x randx12(void);

#endif

