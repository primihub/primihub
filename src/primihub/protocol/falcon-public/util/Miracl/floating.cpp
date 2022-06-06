/*
 *    MIRACL  C++  Float functions float.cpp 
 *
 *    AUTHOR  :    M. Scott
 *             
 *    PURPOSE :    Implementation of Class Float functions
 */

#include <iostream>
#include "floating.h"

using namespace std;

#define TAN 1
#define SIN 2
#define COS 3

static Float *spi;
static int precision=4; // must be power of 2
static BOOL pi_cooked=FALSE;

Float makefloat(int a,int b)
{
    return (Float)a/b;
}

Float fabs(const Float& f)
{
    Float r=f;
    if (r.m<0) r.m.negate();
    return r;
}

int norm(int type,Float& y)
{ // convert y to 1st quadrant angle, and return sign
    int s=PLUS;
    Float pi,w,t;
    if (y.sign()<0)
    {
        y.negate();
        if (type!=COS) s=-s;
    }
    pi=fpi();
    w=pi/2;
    if (fcomp(y,w)<=0) return s;
    w=2*pi;
    if (fcomp(y,w)>0)
    { // reduce mod 2.pi
        t=y/w;
        t=trunc(t);
        t*=w;
        y-=t;
    }
    if (fcomp(y,pi)>0) 
    {
        y-=pi;
        if (type!=TAN) s=(-s);
    }
    w=pi/2;
    if (fcomp(y,w)>0)
    {
        y=pi-y;
        if (type!=SIN) s=(-s);
    }
    return s;
}

Float cos(const Float& f)
{ // see Brent - "The complexity of multiprecision arithmetic"
    int i,q,sgn;
    Float t,r,y,x=f;
    Float one=(Float)1;
    Big lambda;
    sgn=norm(COS,x);

    if (x.iszero()) return sgn*one;

    q=isqrt(MIRACL*precision,2);
    lambda=pow((Big)2,q);

    x/=(Float)lambda;

    r=0; y=one; x*=x;
    for (i=2;i<=q+2;i+=2)
    {
       t=x; t.negate(); t/=(i*(i-1)); y*=t;
    //   y*=(-x/(i*(i-1)));
       if (!r.add(y)) break;
    }

    for (i=0;i<q;i++)
    {
        r+=one;
        r*=r;
        r-=one;
        r*=2;
    } 
    r+=one;
    if (sgn==MINUS) r.negate();
    return r;
}

Float sin(const Float& f)
{ // see Brent
    int i,q,sgn;
    Float t,r,y,x=f;
    Big lambda;
    sgn=norm(SIN,x);

    if (x.iszero()) return (Float)0;

    q=isqrt(MIRACL*precision,2);
    lambda=pow((Big)2,q);

    x/=(Float)lambda;

    r=x; y=x; x*=x;
    for (i=3;i<=q+2;i+=2)
    {
       t=x; t.negate(); t/=(i*(i-1)); y*=t;
//       y*=(-x/(i*(i-1)));
       if (!r.add(y)) break;
    }

    for (i=0;i<q;i++) r=2*r*sqrt(1-r*r);   

    if (sgn==MINUS) r.negate();
    return r;
}

int fcomp(const Float& a,const Float& b)
{
    Big x,y;
    int sa,sb;

    sa=a.sign();
    sb=b.sign();

    if (sa>sb) return 1;
    if (sa<sb) return -1;
    if (sa==0 && sb==0) return 0;

    if (a.e>b.e) return sa;
    if (a.e<b.e) return -sa;
    x=a.m; y=b.m;

    x.shift(precision-length(x));  // make them precision length
    y.shift(precision-length(y));

    if (x>y) return 1;
    if (x<y) return -1;

    return 0;
}

Big Float::trunc(Float *rem)
{
    Big b=0;
    if (iszero()) return b;
    b=shift(m,e-length(m));
    if (rem!=NULL) *rem=*this-(Float)b;
    return b;
}

Big trunc(const Float& f)
{
    Big b=0;
    if (f.iszero()) return b;
    b=shift(f.m,f.e-length(f.m));
    return b;
}

Float exp(const Float &a)
{ // from Brent
    int i;
    Float t,x,y,r,one=(Float)1;
    int q;
    Big lambda;

    if (a.iszero()) return one;
    x=a;
    q=isqrt(MIRACL*precision,2);
    lambda=pow((Big)2,q);

    x/=(Float)lambda;

    r=0; y=one;
    for (i=1;i<=q+2;i++)
    {
       t=x; t/=i; y*=t;
//       y*=(x/i);
       if (!r.add(y)) break;
    }

    for (i=0;i<q;i++)
    {
        r+=one;
        r*=r;
        if (i<q-1) r-=one;
    }

    return r;
}

Float fpi(void)
{ // Gauss-Legendre iteration
    Float r;
    Float x,y,w,a,b,t=1;
    int p;

    if (pi_cooked) return *spi;
    t/=4;
    a=1;
    b=2;
    b=sqrt(b);
    b/=2;
    for (p=1;p<(MIRACL*precision)/4;p*=2)
    {
        x=(a+b)/2;
        y=sqrt(a*b);
        w=a-x;
        t-=(p*(w*w));
        a=x;
        b=y;
    }

    r=a+b;
    r=(r*r)/(4*t);
    pi_cooked=TRUE;
    spi=new Float();
    *spi=r;
    return r;
}

Float reciprocal(const Float &f)
{
    Float x,v=f;
    double d;
    int ke,prec,keep=precision;

    if (v.iszero()) 
    {
       cout << "Divide by zero error from float" << endl;
       exit(0);
    }

// scale v first !
    ke=v.e; v.e=0;

    d=todouble(v); d=1.0/d; x=d;     // initial approximation

    for (prec=1;prec<=keep;prec*=2)
    { // Newtons method
        if (prec<=4) precision=4;
        else         precision=prec;
        x=2*x-(x*x)*v;
    }
    x=2*x-(x*x)*v;  // .. and one for luck

// .. and rescale answer
    x.e-=ke;

    return x;
}

Float nroot(const Float &f,int n)
{
    Float x,v=f;
    double d;
    BOOL minus;
    int ke,prec,keep=precision;

    if (v.iszero()) return (Float)0;
    if (n%2==0 && f.m<0)
    {
        cout << "Even root of a negative number error from Float" << endl;
        exit(0);
    }
    if (n==(-1)) 
    {
        x=reciprocal(v);
        return x;
    }
    if (n==1) return v;
    minus=FALSE;
    if (n<0)
    {
        minus=TRUE;
        n=(-n);
    }

// rescale..
    ke=v.e/n; v.e%=n;

    d=todouble(v); d=pow(d,1.0/(double)n); x=d;

    for (prec=1;prec<=keep;prec*=2)
    { // Newtons method
        if (prec<=4) precision=4;
        else         precision=prec;
        x=(v/pow(x,n-1)+(n-1)*x)/n;

    }
    x=(v/pow(x,n-1)+(n-1)*x)/n;  // .. and one for luck
    x.e+=ke;

    if (minus) x=reciprocal(x);
    return x;
}

Float sqrt(const Float &f)
{
    Float x,v=f;
    double d;
    int ke,prec,keep=precision;

    if (v.iszero()) return (Float)0;
    if (f.m<0)
    {
        cout << "Square root of a negative number error from Float" << endl;
        exit(0);
    }

// rescale..
    ke=v.e/2; v.e%=2;

    d=todouble(v); d=sqrt(d); x=d;
    for (prec=1;prec<=keep;prec*=2)
    { // Newtons method
        if (prec<=4) precision=4;
        else         precision=prec;
        x=(v/x+x)/2;
    }
    x=(v/x+x)/2;    // one for luck....

    x.e+=ke;

    return x;
}

Float pow(const Float &f,int n)
{
    Float x,r=1;

    if (n==0) return r;
    if (f.iszero()) return f;

    x=f;
    if (n<0)
    {
        n=(-n);
        x=reciprocal(x);
    }
    if (n==1) return x;
    forever
    {
        if (n%2!=0) r*=x;
        n/=2;
        if (n==0) break;
        x*=x;
    }

    return r;
}

void Float::negate() const
{
    if (m.iszero()) return;
    m.negate();
}

BOOL Float::iszero() const
{
    if (m.iszero()) return TRUE;
    else return FALSE;
}

BOOL Float::isone() const
{
    if (m.isone() && e==1) return TRUE;
    else return FALSE;
}

int Float::sign() const
{
    if (m>0) return PLUS;
    if (m<0) return MINUS;
    return 0;
}

Float operator-(const Float& f)
{
    Float r=f;
    r.m.negate();
    return r;
}

Float& Float::operator=(const Float &f)
{
    e=f.e;
    m=f.m;
    return *this;
}

Float::Float(double d)
{
    Big t;
    double ip,word;
    int c,s=PLUS;
    if (d<0) {s=MINUS; d=-d;}

    m=0;
    e=0;
    if (d==0.0) return; 

    c=1;
    word=pow(2.0,(double)MIRACL);

    while (d>=word)
    {
        d/=word; c++;
    }
    while (d<1.0) 
    {
        d*=word; c--;
    }

    d=modf(d,&ip);
    m=(mr_small)ip;
    forever
    {
        d*=word;
        d=modf(d,&ip);
        t=(mr_small)ip;
        m=shift(m,1)+t;
        if (d==0.0 || length(m)>precision) break;
    }
    e=c;
    if (s==MINUS) m.negate();
}

Float& Float::operator=(double d)
{
    Float t=d;
    *this=t;
    return *this;
}

double todouble(const Float &f)
{
    int i,s;
    Big x=f.m;
    double word,d=0.0;
    mr_small dig;

    if (f.iszero()) return d;

    if (f.m>=0) s=PLUS;
    else        {s=MINUS; x.negate();}

    word=pow(2.0,(double)MIRACL);
    for (i=0;i<length(x);i++)
    {
        dig=x[i];
        d+=(double)dig;
        d/=word;
    }

    if (f.e>0) for (i=0;i<f.e;i++) d*=word;
    if (f.e<0) for (i=0;i<-f.e;i++) d/=word;

    if (s==MINUS) d=-d;
    return d;
}

BOOL Float::add(const Float &b)
{ // return TRUE if this is affected, FALSE if b has no effect

    if (b.iszero()) return FALSE;
    if (iszero()) {*this=b; return TRUE;}
     
    Big y=b.m;

    m.shift(precision-length(m)); // make them precision length
    y.shift(precision-length(y));

    if (e>=b.e)
    {
        if (e-b.e>precision) return FALSE;
        y.shift(b.e-e);
        m+=y;

        if (m.iszero()) e=0;
        else e+=length(m)-precision;
    }
    else
    {
        if (b.e-e>precision) {*this=b; return TRUE;}
        m.shift(e-b.e);
        m+=y;

        if (m.iszero()) e=0;
        else e=b.e+length(m)-precision;
    }

    m.shift(precision-length(m));

    return TRUE;
}


Float& Float::operator+=(const Float &b)
{
    add(b);
    return *this;
}

BOOL Float::sub(const Float &b)
{

    if (b.iszero()) return FALSE;
    if (iszero()) {*this=-b; return TRUE;}

    Big y=b.m;

    m.shift(precision-length(m));  // make them precision length
    y.shift(precision-length(y));

    if (e>=b.e)
    {
        if (e-b.e>precision) return FALSE;
        y.shift(b.e-e);
        m-=y;
        if (m.iszero()) e=0;
        else e+=length(m)-precision;
    }
    else
    {
        if (b.e-e>precision) {*this=-b; return TRUE;}
        m.shift(e-b.e);
        m-=y;
        if (m.iszero()) e=0;
        else e=b.e+length(m)-precision;
    }

    m.shift(precision-length(m));
    return TRUE;
}

Float& Float::operator-=(const Float &b)
{
    sub(b);
    return *this;
}

Float operator+(const Float& a,const Float &b)
{
    Float r=a;
    r+=b;
    return r;
}

Float operator-(const Float& a,const Float &b)
{
    Float r=a;
    r-=b;
    return r;
}

Float& Float::operator*=(const Float& b)
{
    BOOL extra;

    if (iszero() || b.isone()) return *this;
    if (b.iszero() || isone()) {*this=b; return *this;}

    if (&b==this)
    {
        if (m<0) m.negate();  // result will be positive
        m.shift(precision-length(m));  // make them precision length
        extra=fmt(precision,m,m,m);
        e+=e;
        if (extra) e--;
    }
    else
    {
        Big y=b.m;
        int s=PLUS;

        if (m<0) { s*=MINUS; m.negate(); }
        if (y<0) { s*=MINUS; y.negate(); }
        
        m.shift(precision-length(m));  // make them precision length
        y.shift(precision-length(y));

        extra=fmt(precision,m,y,m);

        if (s<0) m.negate();
        e+=b.e;
        if (extra) e--;
    }
    return *this;
}

Float operator*(const Float& a,const Float& b)
{
    Float r=a;
    r*=b;
    return r;
}

Float operator*(const Float& a,int b)
{
    Float r=a;
    r*=b;
    return r;
}

Float operator*(int a,const Float& b)
{
    Float r=b;
    r*=a;
    return r;
}

Float& Float::operator*=(int x)
{
    int olm=length(m);
    m*=x;
    e+=length(m)-olm;

    m.shift(precision-length(m));
    return *this;
}

Float& Float::operator/=(int x)
{
    int olm;
    if (x==1) return *this;

    m.shift(precision-length(m));
    olm=length(m);
    m/=x;
    e+=length(m)-olm;

    m.shift(precision-length(m));
    return *this;
}

Float operator/(const Float& f,int x)
{
    Float r=f;
    r/=x;
    return r;
}

Float& Float::operator/=(const Float &f)
{
    Float g=reciprocal(f);
    *this*=g;
    return *this;
}

Float operator/(const Float& a,const Float &b)
{
    Float r=reciprocal(b);
    r*=a;
    return r;
}

void setprecision(int p) {precision=(1<<p);}

ostream& operator<<(ostream& s,const Float &f)
{
    return otfloat(s,f.m,f.e);
}

