/*
 * C++ class to implement a power series type with Big coefficients 
 * and to allow arithmetic on it
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 * See Knuth The Art of Computer Programming Vol.2, Chapter 4.7 
 */

#ifndef PS_BIG_H
#define PS_BIG_H

#include "big.h"

#define FFT_BREAK_EVEN 16

extern int psN;
  
// F(x) = (c0+c1*x^(offset+pwr)+c2.x^(offset+2*pwr)+c3.x^(offset+3*pwr) ..)

class term_ps_big
{
public:
    Big an;
    int n;
    term_ps_big *next;
};

class Ps_Big
{
public:
    term_ps_big *start;
    int pwr;
    int offset;

    Ps_Big() {start=NULL; offset=0; pwr=1; }
    Ps_Big(const Ps_Big&);
    void clear();
    void compress(int);
    void decompress(int);
    void norm();
    void pad();
    int max();
    BOOL one_term() const;
    BOOL IsInt() const;
    void divxn(int n) {offset+=n;}
    void mulxn(int n) {offset-=n;}
    int first() {return (-offset);}
    term_ps_big *addterm(const Big&,int,term_ps_big *pos=NULL);
    void multerm(const Big&,int);
    Big coeff(int) const;
    Big cf(int) const;
    Ps_Big& operator*=(Ps_Big&);
    Ps_Big& operator=(const Ps_Big&);
    Ps_Big& operator=(int);
    Ps_Big& operator+=(const Ps_Big&);
    Ps_Big& operator-=(const Ps_Big&);
    Ps_Big& operator*=(const Big&);
    Ps_Big& operator/=(const Big&);
    Ps_Big& operator/=(int);

    friend Ps_Big eta(void);
    friend Ps_Big operator*(Ps_Big&,Ps_Big&);
    friend Ps_Big operator%(const Ps_Big&,const Ps_Big&);
    friend Ps_Big operator/(Ps_Big&,Ps_Big&);
    friend Ps_Big operator/(const Big&,const Ps_Big&);
    friend Ps_Big operator-(const Ps_Big&,const Ps_Big&);
    friend Ps_Big operator-(const Ps_Big&);
    friend Ps_Big operator+(const Ps_Big&,const Ps_Big&);
    friend Ps_Big operator*(const Ps_Big&,const Big&);
    friend Ps_Big operator*(const Big&,const Ps_Big&);

    friend Ps_Big operator/(const Ps_Big&,const Big&);
    friend Ps_Big operator/(const Ps_Big&,int);
    
    friend Ps_Big pow(Ps_Big&,int);
    friend Ps_Big power(const Ps_Big&,int);
    friend ostream& operator<<(ostream&,const Ps_Big&);
    ~Ps_Big();
};

extern Ps_Big eta(void);

#endif

