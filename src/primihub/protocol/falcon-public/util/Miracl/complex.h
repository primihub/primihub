/*
 * Quick and dirty complex data type using float arithmetic
 * Should be extended
 */


#ifndef COMFLOAT_H
#define COMFLOAT_H

#include "floating.h"

class Complex
{
    Float x,y;
public:
    Complex() {x=(Float)0; y=(Float)0; }
    Complex(int a) {x=(Float)a; y=(Float)0; }
    Complex(const Float& a) {x=a; y=(Float)0; }
    Complex(const Float& a,const Float& b) {x=a;y=b;}
    Complex(const Complex& a) {x=a.x;y=a.y;}

    Complex& operator=(const Complex &);
    Complex& operator+=(const Complex &);
    Complex& operator-=(const Complex &);
    Complex& operator*=(const Complex &);
    Complex& operator/=(const Complex &);
    Complex& operator/=(const Float &);

    BOOL iszero() const;

    friend Float real(const Complex &);
    friend Float imaginary(const Complex &);
    friend Float norm2(const Complex &);
	friend Float norm(const Complex &);
    friend Complex recip(const Complex &);

    friend Complex operator-(const Complex&);

    friend BOOL operator==(const Complex&,const Complex&);
    friend BOOL operator!=(const Complex&,const Complex&);

    friend Complex operator+(const Complex &, const Complex &);
    friend Complex operator-(const Complex &, const Complex &);
    friend Complex operator*(const Complex &, const Complex &);
    friend Complex operator/(const Complex &, const Complex &);
    friend Complex exp(const Complex &);
    friend Complex pow(const Complex &,int);
    friend Complex sqrt(Complex &);
    friend ostream& operator<<(ostream&,const Complex&);
    ~Complex() {}
};

#endif

