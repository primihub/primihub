/*
 *    MIRACL  C++ Header file flash.h
 *
 *    AUTHOR  :    N.Coghlan
 *                 Modified by M.Scott
 *             
 *    PURPOSE :    Definition of class Flash
 *
 */

#ifndef FLASH_H
#define FLASH_H

#include "big.h"

#ifdef MR_FLASH

#ifdef BIGS
#define MR_FINIT_BIG fn=&b; b.w=a; b.len=0; for (int i=0;i<BIGS;i++) a[i]=0;
#else
#define MR_FINIT_BIG fn=mirvar(0);
#endif

class Flash
{ /* Flash Class Definitions */
    flash fn;      /* pointer to actual data */
#ifdef BIGS
    mr_small a[BIGS];
    bigtype b;
#endif

public:
    Flash()         {MR_FINIT_BIG}
    Flash(int j)    {MR_FINIT_BIG convert(j,fn); }
    Flash(unsigned int j) {MR_FINIT_BIG uconvert(j,fn); }
    Flash(int x,int y) {MR_FINIT_BIG fconv(x,y,fn); }

#ifdef mr_dltype
 #ifndef MR_DLTYPE_IS_INT
    Flash(mr_dltype dl) {MR_FINIT_BIG dlconv(dl,fn);}
 #endif
#else
    Flash(long lg)  {MR_FINIT_BIG lgconv(lg,fn);}
#endif

    Flash(double d) {MR_FINIT_BIG dconv(d,fn);}
    Flash(const Flash& f) {MR_FINIT_BIG copy(f.fn, fn);}
    Flash(const Big& c)   {MR_FINIT_BIG copy(c.fn, fn);}
    Flash(const Big& n,const Big& d) {MR_FINIT_BIG mround(n.fn,d.fn,fn);}
    Flash(char* s)  {MR_FINIT_BIG cinstr(fn,s);}

    Flash& operator=(int i) {convert(i,fn); return *this;}

#ifdef mr_dltype
 #ifndef MR_DLTYPE_IS_INT
    Flash& operator=(mr_dltype dl){dlconv(dl,fn); return *this;}
 #endif
#else
    Flash& operator=(long lg){lgconv(lg,fn); return *this;}
#endif

    Flash& operator=(double& d)  {dconv(d,fn);    return *this;}
    Flash& operator=(const Flash& f)   {copy(f.fn, fn); return *this;}
    Flash& operator=(const Big& b)     {copy(b.fn, fn); return *this;}
    Flash& operator=(char* s)    {cinstr(fn,s);return *this;}

    Flash& operator++()      {fincr(fn,1,1,fn);  return *this;}
    Flash& operator--()      {fincr(fn,-1,1,fn); return *this;}
    Flash& operator+=(const Flash& f) {fadd(fn,f.fn,fn);  return *this;}

    Flash& operator-=(const Flash& f) {fsub(fn,f.fn,fn);  return *this;}

    Flash& operator*=(const Flash& f) {fmul(fn,f.fn,fn);  return *this;}
    Flash& operator*=(int n)          {fpmul(fn,n,1,fn);  return *this;}


    Flash& operator/=(const Flash& f) {fdiv(fn,f.fn,fn);  return *this;}
    Flash& operator/=(int n)          {fpmul(fn,1,n,fn);  return *this;}


    Flash& operator%=(const Flash& f) {fmodulo(fn,f.fn,fn); return *this;}

    Big trunc(Flash *rem=NULL);
    Big num(void);
    Big den(void);
    BOOL iszero() const;

    friend Flash operator-(const Flash&);   /* unary - */

    /* binary ops */

    friend Flash operator+(const Flash&, const Flash&);

    friend Flash operator-(const Flash&, const Flash&);

    friend Flash operator*(const Flash&, const Flash&);

    friend Flash operator/(const Flash&, const Flash&);

    friend Flash operator%(const Flash&,const Flash&);

    /* relational ops */

    friend BOOL operator<=(const Flash& f1, const Flash& f2)
    {if (fcomp(f1.fn,f2.fn) <= 0) return TRUE; else return FALSE;}
    friend BOOL operator>=(const Flash& f1, const Flash& f2) 
    {if (fcomp(f1.fn,f2.fn) >= 0) return TRUE; else return FALSE;}
    friend BOOL operator==(const Flash& f1, const Flash& f2)
    {if (fcomp(f1.fn,f2.fn) == 0) return TRUE; else return FALSE;}
    friend BOOL operator!=(const Flash& f1, const Flash& f2)
    {if (fcomp(f1.fn,f2.fn) != 0) return TRUE; else return FALSE;}
    friend BOOL operator<(const Flash& f1, const Flash& f2)
    {if (fcomp(f1.fn,f2.fn) < 0)  return TRUE; else return FALSE;}
    friend BOOL operator>(const Flash& f1, const Flash& f2) 
    {if (fcomp(f1.fn,f2.fn) > 0)  return TRUE; else return FALSE;}

    friend Flash inverse(const Flash&);
    friend Flash pi(void); 
    friend Flash cos(const Flash&);
    friend Flash sin(const Flash&);
    friend Flash tan(const Flash&);

    friend Flash acos(const Flash&);
    friend Flash asin(const Flash&);
    friend Flash atan(const Flash&);

    friend Flash cosh(const Flash&);
    friend Flash sinh(const Flash&);
    friend Flash tanh(const Flash&);

    friend Flash acosh(const Flash&);
    friend Flash asinh(const Flash&);
    friend Flash atanh(const Flash&);

    friend Flash log(const Flash&);
    friend Flash exp(const Flash&);
    friend Flash pow(const Flash&,const Flash&);
    friend Flash sqrt(const Flash&);
    friend Flash nroot(const Flash&,int);
    friend Flash fabs(const Flash&);

    friend double todouble(const Flash& f) { return fdsize(f.fn);}

#ifndef MR_NO_STANDARD_IO

    friend istream& operator>>(istream&, Flash&);
    friend ostream& operator<<(ostream&, const Flash&);

#endif


#ifdef BIGS
    ~Flash()   { }
#else
    ~Flash()   {mirkill(fn);}
#endif
};

extern Flash pi(void); 

#endif
#endif

