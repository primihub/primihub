/*
 *    MIRACL  C++  Flash functions flash.cpp 
 *
 *    AUTHOR  :    N.Coghlan
 *                 Modified by M.Scott
 *             
 *    PURPOSE :    Implementation of Class Flash functions
 *
 */

#include "flash.h"

#ifdef MR_FLASH


BOOL Flash::iszero() const
{ if (size(fn)==0) return TRUE; return FALSE; }

Big Flash::trunc(Flash *rem) {Big b; 
if (rem==NULL) ftrunc(fn,b.fn,b.fn); 
else ftrunc(fn,b.fn,rem->fn);
return b;}

Big Flash::num(void) {Big b; numer(fn,b.fn); return b;}
Big Flash::den(void) {Big b; denom(fn,b.fn); return b;}

Flash operator-(const Flash& f)  {Flash nf; negify(f.fn,nf.fn); return nf;}

Flash operator+(const Flash& f1, const Flash& f2)
{Flash aff; fadd(f1.fn,f2.fn,aff.fn); return aff;}

Flash operator-(const Flash& f1, const Flash& f2)
{Flash mff; fsub(f1.fn,f2.fn,mff.fn); return mff;}

Flash operator*(const Flash& f1, const Flash& f2)
{Flash xff; fmul(f1.fn,f2.fn,xff.fn); return xff;}

Flash operator/(const Flash& f1, const Flash& f2)
{Flash dff; fdiv(f1.fn,f2.fn,dff.fn); return dff;}
Flash operator%(const Flash& f1, const Flash& f2)
{Flash rff; fmodulo(f1.fn,f2.fn,rff.fn); return rff;}

Flash pi() {Flash z; fpi(z.fn); return z;}

Flash inverse(const Flash& f) {Flash z; frecip(f.fn, z.fn); return z;} 

Flash cos(const Flash& f) {Flash z; fcos(f.fn, z.fn); return z;}
Flash sin(const Flash& f) {Flash z; fsin(f.fn, z.fn); return z;}
Flash tan(const Flash& f) {Flash z; ftan(f.fn, z.fn); return z;}

Flash acos(const Flash& f){Flash z; facos(f.fn, z.fn);return z;}
Flash asin(const Flash& f){Flash z; fasin(f.fn, z.fn);return z;}
Flash atan(const Flash& f){Flash z; fatan(f.fn, z.fn);return z;}

Flash cosh(const Flash& f){Flash z; fcosh(f.fn, z.fn);return z;}
Flash sinh(const Flash& f){Flash z; fsinh(f.fn, z.fn);return z;}
Flash tanh(const Flash& f){Flash z; ftanh(f.fn, z.fn);return z;}

Flash acosh(const Flash& f){Flash z; facosh(f.fn, z.fn);return z;}
Flash asinh(const Flash& f){Flash z; fasinh(f.fn, z.fn);return z;}
Flash atanh(const Flash& f){Flash z; fatanh(f.fn, z.fn);return z;}

Flash log(const Flash& f) {Flash z; flog(f.fn, z.fn); return z;}
Flash exp(const Flash& f) {Flash z; fexp(f.fn, z.fn); return z;}
Flash pow(const Flash& f1,const Flash& f2)            
{Flash z;fpowf(f1.fn,f2.fn,z.fn); return z;}

Flash sqrt(const Flash& f) {Flash z; froot(f.fn, 2, z.fn); return z;}
Flash nroot(const Flash& f,int n) {Flash z; froot(f.fn,n,z.fn); return z;}
Flash fabs(const Flash& f) {Flash z; absol(f.fn,z.fn);     return z;}

#ifndef MR_NO_STANDARD_IO

istream& operator>>(istream& s, Flash& x)
{ 
    miracl *mip=get_mip();
    s >> mip->IOBUFF; 
    if (s.eof() || s.bad()) 
    {
         zero(x.fn); 
         return s;
    } 
    cinstr(x.fn,mip->IOBUFF); 
    return s; 
}

ostream& operator<<(ostream& s, const Flash& x)
{
    miracl *mip=get_mip(); 
    cotstr(x.fn,mip->IOBUFF); 
    s << mip->IOBUFF;   
    return s;
}

#endif

#endif

