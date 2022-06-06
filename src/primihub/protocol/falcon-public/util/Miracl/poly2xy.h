/*
 * C++ class to implement a bivariate polynomial type and to allow 
 * arithmetic on polynomials whose coefficients are from
 * the finite field of characteristic 2
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 * See Knuth The Art of Computer Programming Vol.2, Chapter 4.6 
 */

#ifndef POLY2XY_H
#define POLY2XY_H

#include "gf2m.h"
#include "poly2.h"

class term2XY
{
public:
    GF2m an;
    int nx;
    int ny;
    term2XY *next;
};
  
class Poly2XY
{
public:
    term2XY *start;
    Poly2XY() {start=NULL;}
    Poly2XY(const Poly2XY&);
    Poly2XY(const Poly2&);
    Poly2XY(const GF2m&,int,int);
    Poly2XY(int);

    void clear();
    term2XY *addterm(const GF2m&,int,int,term2XY *pos=NULL);
    GF2m F(const GF2m&,const GF2m&);
    Poly2 F(const GF2m&);
    GF2m coeff(int,int) const;
    Poly2 convert_x() const;
    Poly2 convert_x2() const;
    Poly2 convert_x3() const;
    Poly2XY& operator=(const Poly2XY&);
    Poly2XY& operator=(int);
    Poly2XY& operator+=(const Poly2XY&);
    Poly2XY& operator-=(const Poly2XY&);
    Poly2XY& operator%=(const Poly2XY&);
    Poly2XY& operator*=(const GF2m&);

    friend Poly2XY diff_dx(const Poly2XY&);
    friend Poly2XY diff_dy(const Poly2XY&);
    friend BOOL iszero(const Poly2XY&);
    friend BOOL isone(const Poly2XY&);

    friend Poly2XY operator*(const Poly2XY&,const Poly2XY&);
    friend Poly2XY operator%(const Poly2XY&,const Poly2XY&);
    friend Poly2XY operator/(const Poly2XY&,const Poly2XY&);
    friend Poly2XY operator+(const Poly2XY&,const Poly2XY&);
    friend Poly2XY operator+(const Poly2XY&,const GF2m&);
    friend Poly2XY operator-(const Poly2XY&,const Poly2XY&);
    friend Poly2XY operator*(const Poly2XY&,const GF2m&);

    // friend Poly2XY operator*(const GF2m&,Variable);
    friend Poly2XY operator*(Variable, Variable);
    friend Poly2XY pow2X(Variable,int);
    friend Poly2XY pow2Y(Variable,int);
    friend Poly2XY pow(const Poly2XY&,int);
    friend BOOL operator==(const Poly2XY&,const Poly2XY&);
    friend int degreeX(const Poly2XY&);
    friend int degreeY(const Poly2XY&);
    friend Poly2XY compose(const Poly2XY&,const Poly2XY&, const Poly2XY&);

    friend ostream& operator<<(ostream&,const Poly2XY&);
    ~Poly2XY();
};

extern Poly2XY operator*(Variable, Variable);
extern Poly2XY pow2X(Variable,int);
extern Poly2XY pow2Y(Variable,int);

#endif

