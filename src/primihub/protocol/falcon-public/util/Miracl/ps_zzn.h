/*
 * C++ class to implement a power series type and to allow 
 * arithmetic on it. COefficinets are of type ZZn
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 * See Knuth The Art of Computer Programming Vol.2, Chapter 4.7 
 */

#ifndef PS_ZZN_H
#define PS_ZZN_H

#include "zzn.h"

#define FFT_BREAK_EVEN 16

extern int psN;
  
// F(x) = (c0+c1*x^(offset+pwr)+c2.x^(offset+2*pwr)+c3.x^(offset+3*pwr) ..)

class term_ps_zzn
{
public:
    ZZn an;
    int n;
    term_ps_zzn *next;
};

class Ps_ZZn
{
public:
    term_ps_zzn *start;
    int pwr;
    int offset;

    Ps_ZZn() {start=NULL; offset=0; pwr=1; }
    Ps_ZZn(const Ps_ZZn&);
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
    term_ps_zzn *addterm(const ZZn&,int,term_ps_zzn *pos=NULL);
    void multerm(const ZZn&,int);
    ZZn coeff(int) const;
    ZZn cf(int) const;
    Ps_ZZn& operator*=(Ps_ZZn&);
    Ps_ZZn& operator=(const Ps_ZZn&);
    Ps_ZZn& operator=(int);
    Ps_ZZn& operator+=(const Ps_ZZn&);
    Ps_ZZn& operator-=(const Ps_ZZn&);
    Ps_ZZn& operator*=(const ZZn&);
    Ps_ZZn& operator/=(const ZZn&);
    Ps_ZZn& operator/=(int);

    friend Ps_ZZn eta();
    friend Ps_ZZn operator*(Ps_ZZn&,Ps_ZZn&);
    friend Ps_ZZn operator%(const Ps_ZZn&,const Ps_ZZn&);
    friend Ps_ZZn operator/(Ps_ZZn&,Ps_ZZn&);
    friend Ps_ZZn operator-(const Ps_ZZn&,const Ps_ZZn&);
    friend Ps_ZZn operator-(const Ps_ZZn&);
    friend Ps_ZZn operator+(const Ps_ZZn&,const Ps_ZZn&);
    friend Ps_ZZn operator*(const Ps_ZZn&,const ZZn&);
    friend Ps_ZZn operator*(const ZZn&,const Ps_ZZn&);
    
    friend Ps_ZZn operator/(const ZZn&,const Ps_ZZn&);
    friend Ps_ZZn operator/(const Ps_ZZn&,const ZZn&);
    friend Ps_ZZn operator/(const Ps_ZZn&,int);

    friend Ps_ZZn pow(Ps_ZZn&,int);
    friend Ps_ZZn power(const Ps_ZZn&,int);
    friend ostream& operator<<(ostream&,const Ps_ZZn&);
    ~Ps_ZZn();
};

extern Ps_ZZn eta();

#endif

