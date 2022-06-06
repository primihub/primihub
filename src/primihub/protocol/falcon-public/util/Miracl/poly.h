/*
 * C++ class to implement a polynomial type and to allow 
 * arithmetic on polynomials whose elements are from
 * the finite field mod p
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 * New! it is now possible to assign a polynomial as:-
 *
 * Variable x;
 * Poly P=3*pow(x,3)+2*x+1  // P=3x^3+2x+1
 *
 * See Knuth The Art of Computer Programming Vol.2, Chapter 4.6 
 */

#ifndef POLY_H
#define POLY_H

#include "zzn.h"
#include "variable.h"

#define FFT_BREAK_EVEN 16

class term
{
public:
    ZZn an;
    int n;
    term *next;
};

class Poly
{
public:
    term *start;
    Poly() {start=NULL;}
    Poly(const Poly&);

    Poly(const ZZn&,int);
    Poly(Variable &);

    void clear();
    term *addterm(const ZZn&,int,term *pos=NULL);
    void multerm(const ZZn&,int);
    ZZn F(const ZZn&) const;
    ZZn coeff(int) const;
    ZZn min() const;

    Poly& operator=(const Poly&);
    Poly& operator=(const ZZn&);
    Poly& operator=(int);
    Poly& operator+=(const Poly&);
    Poly& operator-=(const Poly&);
    Poly& operator+=(const ZZn& m) {addterm(m,0);    return *this; } 
    Poly& operator-=(const ZZn& m) {addterm((-m),0); return *this; }
    Poly& operator%=(const Poly&);
    Poly& operator*=(const ZZn&);
    Poly& operator/=(const ZZn&);

    friend void setpolymod(const Poly&);
    friend BOOL iszero(const Poly&);
    friend BOOL isone(const Poly&);
    friend Poly divxn(const Poly&,int);
    friend Poly mulxn(const Poly&,int);
    friend Poly modxn(const Poly&,int);    
    friend Poly invmodxn(const Poly&,int);
    friend Poly reverse(const Poly&);
    friend int degree(const Poly&);
    friend Poly compose(const Poly&,const Poly&,const Poly&);
    friend Poly compose(const Poly&,const Poly&);
    friend Poly modmult(const Poly&,const Poly&,const Poly&);
    friend void egcd(Poly result[], const Poly&,const Poly&);
    friend void makemonic(Poly&);
    friend Poly differentiate(const Poly&);

    friend Poly reduce(const Poly&,const Poly&);
    friend Poly operator*(const Poly&,const Poly&);
    friend Poly operator%(const Poly&,const Poly&);
    friend Poly operator/(const Poly&,const Poly&);
    friend Poly operator-(const Poly&,const Poly&);
    friend Poly operator+(const Poly&,const Poly&);
    friend Poly operator-(const Poly&);

    friend Poly operator*(const ZZn& ,Variable);
    friend Poly pow(Variable,int);


    friend Poly operator-(const Poly&,const ZZn&);
    friend Poly operator+(const Poly&,const ZZn&);
    friend Poly operator*(const Poly&,const ZZn&);
    friend Poly operator*(const ZZn&,const Poly&);
    
    friend Poly operator/(const Poly&,const ZZn&);

    friend Poly gcd(const Poly&,const Poly&);
    friend Poly diff(const Poly&);
    friend Poly pow(const Poly&,const Big&,const Poly&);
    friend Poly pow(const Poly&,int);

    friend BOOL operator==(const Poly&,const Poly&);
    friend BOOL operator!=(const Poly&,const Poly&);

    friend Poly factor(const Poly&,int);
    friend ostream& operator<<(ostream&,const Poly&);
    ~Poly();
};

extern Poly pow(Variable,int);


#endif

