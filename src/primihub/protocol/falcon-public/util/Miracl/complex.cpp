#include "complex.h"

#include <iostream>
using namespace std;

BOOL Complex::iszero() const
{ if (x.iszero() && y.iszero()) return TRUE; return FALSE; }

Complex& Complex::operator=(const Complex& a)
{x=a.x; y=a.y; return *this;}

Complex& Complex::operator+=(const Complex& a)
{x+=a.x; y+=a.y; return *this;}

Complex& Complex::operator-=(const Complex& a)
{x-=a.x; y-=a.y; return *this;}

Complex& Complex::operator*=(const Complex& a)
{ // using karatsuba....
    Float t,t1,t2;

    if (this==&a)
    { // squaring - a bit faster

// x=(x+y)(x-y), y=2xy

        t=x; t+=y; t1=x; t1-=y; t*=t1;
        y*=x;
        y*=2;
        x=t;
        return *this;    
    }

// x=x*a.x-y*a.y, y=(x+y)*(a.x+a.y) - x*a.x - y*a.y;

    t=x; t*=a.x; 
    t2=y; t2*=a.y;
    t1=a.x; t1+=a.y;
    y+=x; y*=t1; y-=t; y-=t2;
    t-=t2; x=t;

    return *this;
}

Complex& Complex::operator/=(const Complex& a)
{Float t,d=reciprocal(a.x*a.x+a.y*a.y);
 t=(x*a.x+y*a.y)*d; y=(y*a.x-x*a.y)*d; x=t; return *this;}

Complex& Complex::operator/=(const Float& f)
{
    Float t=reciprocal(f);
    x*=t;
    y*=t;
    return *this;
}

Float real(const Complex &a) {Float f; f=a.x; return f;}
Float imaginary(const Complex &a) {Float f; f=a.y; return f;}

Complex recip(const Complex &a)
{ 
  Complex c=a;
  Float d=reciprocal(c.x*c.x+c.y*c.y);
  c.x*=d;
  c.y*=d;
  c.y.negate();
  return c;
}

Complex operator-(const Complex& a)
{Complex c=a; c.x.negate(); c.y.negate(); return c;}

BOOL operator==(const Complex& a,const Complex& b)
{ if (a.x==b.x && a.y==b.y) return TRUE; else return FALSE;}

BOOL operator!=(const Complex& a,const Complex& b)
{ if (a.x!=b.x || a.y!=b.y) return TRUE; else return FALSE;}

Complex operator+(const Complex &a,const Complex &b)
{Complex c=a; c+=b; return c;}

Complex operator-(const Complex &a,const Complex &b)
{Complex c=a; c-=b; return c;}

Complex operator*(const Complex &a,const Complex &b)
{Complex c=a; c*=b; return c;}

Complex operator/(const Complex &a,const Complex &b)
{Complex c=a; c/=b; return c; }

Complex exp(const Complex& a)
{
    
    Complex c; 
    Float t,d,e=a.y;

    d=exp(a.x);
    t=cos(e);
    c.x=d*t;
    t=sqrt(1-t*t);
    if (norm(2,e)==MINUS) t.negate();   
    c.y=d*t;

    return c;
}

Complex pow(const Complex& a,int b)
{
 Complex w=1;
 Complex c=a; 

 if (b==0) return w;
 if (b==1) return c;
 forever
 {
    if (b%2!=0) w*=c;
    b/=2;
    if (b==0) break;
    c*=c;
 }
 return w; 
}

ostream& operator<<(ostream& s,const Complex& a)
{
    s << "(" << a.x << "," << a.y << ")";
    return s;
}

// Note - this function is flawed...it only returns the POSITIVE root
Complex sqrt(Complex &c)
{
    if(c.y != 0 || c.x < 0) {
        Float r = sqrt(c.x*c.x + c.y*c.y);
        Float b = sqrt((r-c.x)/2);
        Float a = c.y/(2*b);
        c.x = a;
        c.y = b;
    } else {
        Float r = sqrt(c.x);
        c.x = r;
    }
    return c;
}

// Get the absolute value of the complex number squared
// |z|^2 = x^2 + y^2
Float norm2(const Complex &a) 
{
    Float f; 
    f = (a.x * a.x) + (a.y * a.y);
    return f;
}

Float norm(const Complex &a) 
{
    Float f; 
    f = (a.x * a.x) + (a.y * a.y);
    return sqrt(f);
}
