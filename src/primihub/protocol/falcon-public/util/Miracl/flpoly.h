/*
 * C++ class to implement a polynomial type and to allow 
 * arithmetic on polynomials whose elements are float numbers
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 * See Knuth The Art of Computer Programming Vol.2, Chapter 4.6 
 */

#ifndef FPOLY_H
#define FPOLY_H

#include "floating.h"

class fterm
{
public:
    Float an;
    int n;
    fterm *next;
};
  
class FPoly
{
    fterm *start;
public:
    FPoly() {start=NULL;}
    FPoly(const FPoly&);
    void clear();
    fterm* addterm(Float,int,fterm *pos=NULL);
    void multerm(Float,int);
    Float coeff(int) const;
    BOOL iszero() const;

    FPoly& operator=(const FPoly&);
    FPoly& operator+=(const FPoly&);
    FPoly& operator-=(const FPoly&);
    FPoly& operator*=(const Float&);
    FPoly& divxn(FPoly&,int);
    FPoly& mulxn(int);

    friend int degree(const FPoly&);
    friend FPoly twobytwo(const FPoly&,const FPoly&);
    friend FPoly pow2bypow2(const FPoly&,const FPoly&,int);
    friend FPoly powsof2(const FPoly&,int,const FPoly&,int);
    friend FPoly special(const FPoly&,const FPoly&);

    friend FPoly operator+(const FPoly&,const FPoly&);
    friend FPoly operator-(const FPoly&,const FPoly&);

    friend FPoly operator*(const FPoly&,const FPoly&);
    friend FPoly operator*(const FPoly&,const Float&);

    friend ostream& operator<<(ostream&,const FPoly&);
    ~FPoly();
};


#endif

