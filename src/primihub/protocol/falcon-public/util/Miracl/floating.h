/*
 *    MIRACL  C++ Header file float.h
 *
 *    AUTHOR  :    M.Scott
 *             
 *    PURPOSE :    Definition of class Float
 *
 */

#ifndef FLOAT_H
#define FLOAT_H

#include <cmath>
#include "big.h"

extern void setprecision(int);

class Float
{
    int e;      // exponent
    Big m;      // mantissa
public:
    Float()      { }
    Float(int i) {m=i; e=1;}
    Float(const Float& f) {e=f.e; m=f.m; }
    Float(const Big &b) {m=b; e=length(b);}
    Float(const Big &b,int ex) {m=b; e=ex;}
    Float(double);

    Big trunc(Float *rem=NULL); 
    void negate() const;
    BOOL iszero() const;
    BOOL isone() const;
    int sign() const;
    Float& operator=(double);
    BOOL add(const Float&);
    Float& operator+=(const Float&);
    BOOL sub(const Float&);
    Float& operator-=(const Float&);
    Float& operator*=(const Float&);
    Float& operator*=(int);
    Float& operator/=(const Float&);
    Float& operator/=(int);
    Float& operator=(const Float&);

    friend Float reciprocal(const Float&);
    friend double todouble(const Float&);
    friend Float makefloat(int,int);
    friend Float operator-(const Float&); 
    friend Float operator+(const Float&,const Float&);
    friend Float operator-(const Float&,const Float&);
    friend Float operator*(const Float&,const Float&);
    friend Float operator*(const Float&,int);
    friend Float operator*(int,const Float&);
    friend Float operator/(const Float&,const Float&);
    friend Float operator/(const Float&,int);
    friend Float sqrt(const Float&);
    friend Float nroot(const Float&,int);
    friend Float exp(const Float&);
    friend Float sin(const Float&);
    friend Float cos(const Float&);
    friend Float pow(const Float&,int);
    friend Float fpi(void);

    friend Big trunc(const Float&);
    friend int norm(int,Float&);
    friend Float fabs(const Float&);

    /* relational ops */
    friend int fcomp(const Float&,const Float&);

    friend BOOL operator<=(const Float& f1, const Float& f2)
    {if (fcomp(f1,f2) <= 0) return TRUE; else return FALSE;}
    friend BOOL operator>=(const Float& f1, const Float& f2) 
    {if (fcomp(f1,f2) >= 0) return TRUE; else return FALSE;}
    friend BOOL operator==(const Float& f1, const Float& f2)
    {if (fcomp(f1,f2) == 0) return TRUE; else return FALSE;}
    friend BOOL operator!=(const Float& f1, const Float& f2)
    {if (fcomp(f1,f2) != 0) return TRUE; else return FALSE;}
    friend BOOL operator<(const Float& f1, const Float& f2)
    {if (fcomp(f1,f2) < 0)  return TRUE; else return FALSE;}
    friend BOOL operator>(const Float& f1, const Float& f2) 
    {if (fcomp(f1,f2) > 0)  return TRUE; else return FALSE;}

    friend ostream& operator<<(ostream&,const Float&);

    ~Float() { }
};

extern Float fpi(void);
extern Float makefloat(int,int);

#endif

