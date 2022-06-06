/*
 * C++ class to implement a bivariate polynomial type and to allow 
 * arithmetic on polynomials whose coefficients are from
 * the finite field mod p
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 * See Knuth The Art of Computer Programming Vol.2, Chapter 4.6 
 */

#ifndef POLYXY_H
#define POLYXY_H

#include "zzn.h"
#include "poly.h"

class termXY
{
public:
    ZZn an;
    int nx;
    int ny;
    termXY *next;
};
  
class PolyXY
{
public:
    termXY *start;
    PolyXY() {start=NULL;}
    PolyXY(const PolyXY&);
    PolyXY(const Poly&);
    PolyXY(const ZZn&,int,int);
    PolyXY(int);

    void clear();
    termXY *addterm(const ZZn&,int,int,termXY *pos=NULL);
    ZZn F(const ZZn&,const ZZn&);
    Poly F(const ZZn&);
    ZZn coeff(int,int) const;
    Poly convert_x() const;
    Poly convert_x2() const;
    Poly convert_x3() const;
    PolyXY& operator=(const PolyXY&);
    PolyXY& operator=(int);
    PolyXY& operator+=(const PolyXY&);
    PolyXY& operator-=(const PolyXY&);
    PolyXY& operator%=(const PolyXY&);
    PolyXY& operator*=(const ZZn&);

    friend PolyXY diff_dx(const PolyXY&);
    friend PolyXY diff_dy(const PolyXY&);
    friend BOOL iszero(const PolyXY&);
    friend BOOL isone(const PolyXY&);

    friend PolyXY operator*(const PolyXY&,const PolyXY&);
    friend PolyXY operator%(const PolyXY&,const PolyXY&);
    friend PolyXY operator/(const PolyXY&,const PolyXY&);
    friend PolyXY operator+(const PolyXY&,const PolyXY&);
    friend PolyXY operator+(const PolyXY&,const ZZn&);
    friend PolyXY operator-(const PolyXY&,const PolyXY&);
    friend PolyXY operator*(const PolyXY&,const ZZn&);

    // friend PolyXY operator*(Variable, Variable);
    friend PolyXY powX(Variable,int);
    friend PolyXY powY(Variable,int);
    friend PolyXY pow(const PolyXY&,int);
    friend BOOL operator==(const PolyXY&,const PolyXY&);
    friend int degreeX(const PolyXY&);
    friend int degreeY(const PolyXY&);
    friend PolyXY compose(const PolyXY&,const PolyXY&, const PolyXY&);

    friend ostream& operator<<(ostream&,const PolyXY&);
    ~PolyXY();
};

extern PolyXY powX(Variable,int);
extern PolyXY powY(Variable,int);

#endif

