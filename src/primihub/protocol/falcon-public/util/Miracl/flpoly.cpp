/*
 * C++ class to implement a polynomial type and to allow 
 * arithmetic on polynomials whose elements are float numbers
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 * See Knuth The Art of Computer Programming Vol.2, Chapter 4.6 
 */

#include "flpoly.h"

#include <iostream>
using namespace std;

FPoly::FPoly(const FPoly& p)
{
    fterm *ptr=p.start;
    fterm *pos=NULL;
    start=NULL;
    while (ptr!=NULL)
    {  
        pos=addterm(ptr->an,ptr->n,pos);
        ptr=ptr->next;
    }    
}

FPoly::~FPoly()
{
   fterm *nx;
   while (start!=NULL)
   {   
       nx=start->next;
       delete start;
       start=nx;
   }
}

BOOL FPoly::iszero() const
{
    if (start==NULL) return TRUE;
    else return FALSE;
}

FPoly& FPoly::operator*=(const Float& x)
{
    fterm *ptr=start;
    while (ptr!=NULL)
    {
        ptr->an*=x;
        ptr=ptr->next;
    }
    return *this;
}

FPoly operator*(const FPoly& x,const Float& y)
{
    FPoly z=x;
    z*=y;
    return z;
}

Float FPoly::coeff(int power) const
{
    fterm *ptr=start;
    while (ptr!=NULL)
    {
        if (ptr->n==power) return ptr->an;
        ptr=ptr->next;
    }
    return (Float)0;
}

FPoly& FPoly::divxn(FPoly& a,int n)
{
    fterm *lptr=NULL;
    fterm *ptr=start;

    while(ptr!=NULL)
    {
        if (ptr->n<n)
        {
            a.start=ptr;
            if (lptr==NULL) start=NULL;
            else lptr->next=NULL;
            break;
        }
        else
        {
            ptr->n-=n;
        }
        lptr=ptr;
        ptr=ptr->next;    
    }
    return *this;
}

FPoly& FPoly::mulxn(int n)
{
    fterm *ptr=start;
    while (ptr!=NULL)
    {
        ptr->n+=n;
        ptr=ptr->next;
    }
    return *this;
}

// fast Karatsuba multiplication of polynomials with n terms, where n=2^m

FPoly twobytwo(const FPoly& a,const FPoly& b)
{ // a and b both have 2 terms (a1.x+a0)*(b1.x+b0)
    FPoly R;

    Float a0,b0,a1,b1,ax;
    fterm *ptr,*pos=NULL;

    ptr=a.start;
    while (ptr!=NULL)
    {
        if (ptr->n==1) a1=ptr->an;
        if (ptr->n==0) a0=ptr->an;
        ptr=ptr->next;
    }

    ptr=b.start;
    while (ptr!=NULL)
    {
        if (ptr->n==1) b1=ptr->an;
        if (ptr->n==0) b0=ptr->an;
        ptr=ptr->next;
    }

    ax=a0; ax*=b0;

    a0+=a1; b0+=b1; a0*=b0;
    a0-=ax; 
  
    a1*=b1;
    a0-=a1;

    pos=R.addterm(a1,2,pos);
    pos=R.addterm(a0,1,pos);
    pos=R.addterm(ax,0,pos);

    return R;
}

FPoly pow2bypow2(const FPoly &a,const FPoly &b,int deg)
{ // a and b are both have 2^n terms

    if (deg==2) return twobytwo(a,b);

    FPoly higha,lowa,highb,lowb,highr,lowr,midr;
    int half=deg/2;

    higha=a;                       // chop into low and high halves
    higha.divxn(lowa,half);

    highb=b;
    highb.divxn(lowb,half);

    lowr =pow2bypow2(lowa,lowb,half);   // Karatsuba recursion
    highr=pow2bypow2(higha,highb,half);

    higha+=lowa;
    highb+=lowb;

    midr =pow2bypow2(higha,highb,half);
    midr-=lowr;
    midr-=highr;

    midr.mulxn(half);
    highr.mulxn(2*half);

    lowr+=midr;
    lowr+=highr;

    return lowr;
}

FPoly powsof2(const FPoly& a,int n,const FPoly& b,int m)
{ // a has n terms, b has m terms with n >= m amd m|n and m=2^i

    if (n==m) return pow2bypow2(a,b,n);
    FPoly r,q,w,t;
    int i,c=0;

    q=a;
    for (i=0;i<n/m;i++)
    {
        q.divxn(r,m);
        t=pow2bypow2(r,b,m);
        t.mulxn(c);
        w+=t;
        c+=m;
    }
    return w;
}

FPoly special(const FPoly& a,const FPoly& b)
{ // special multiplier of monic polynomials a and b 
  // where degree of a is a multiple of the degree of b,
  // and the degree of b is 2^n
    FPoly x=a;
    FPoly y=b;
    FPoly w;
    int n=degree(x);
    int m=degree(y);

    fterm *ptr,nptr;

    ptr=x.start;           // remove x^n term
    x.start=ptr->next;
    delete ptr;

    ptr=y.start;           // remove x^m term
    y.start=ptr->next;
    delete ptr;

    if (n>=m) w=powsof2(x,n,y,m);
    else      w=powsof2(y,m,x,n);

    x.mulxn(m);
    w+=x;
    y.mulxn(n);
    w+=y;

    w.addterm((Float)1,m+n);
    return w;
}

FPoly operator*(const FPoly& a,const FPoly& b)
{
    FPoly prod;
    fterm *iptr,*pos;
    fterm *ptr;
    miracl *mip=get_mip();

    if (&a==&b)
    { // squaring
        pos=NULL;
        ptr=b.start;
        while (ptr!=NULL)
        { // diagonal terms
            pos=prod.addterm(ptr->an*ptr->an,ptr->n+ptr->n,pos);
            ptr=ptr->next;
        }
        ptr=b.start;
        while (ptr!=NULL)
        { // above the diagonal
            iptr=ptr->next;
            pos=NULL;
            while (iptr!=NULL)
            {
                pos=prod.addterm(2*ptr->an*iptr->an,ptr->n+iptr->n,pos);
                iptr=iptr->next;
            }
            ptr=ptr->next; 
        }
    }
    else 
    {
        ptr=b.start;

        mip->same=TRUE;    // multiplying by a constant
        while (ptr!=NULL)
        {
            pos=NULL;
            iptr=a.start;
            mip->first_one=FALSE;    
                           // first multiplication by this constant needed
            while (iptr!=NULL)
            {
                pos=prod.addterm(iptr->an*ptr->an,iptr->n+ptr->n,pos);
                iptr=iptr->next;
            }
            ptr=ptr->next;
        }
        mip->same=FALSE;
    }
    return prod;
}

int degree(const FPoly& p)
{
    return p.start->n;
}

void FPoly::clear()
{
    fterm *ptr;
    while (start!=NULL)
    {   
       ptr=start->next;
       delete start;
       start=ptr;
    }
    
}

FPoly &FPoly::operator=(const FPoly& p)
{
    fterm *ptr,*pos=NULL;
    clear();
    ptr=p.start;
    while (ptr!=NULL)
    {  
        pos=addterm(ptr->an,ptr->n,pos);
        ptr=ptr->next;
    }    
    return *this;
    
}

FPoly& FPoly::operator+=(const FPoly& p)
{
    fterm *ptr,*pos=NULL;
    ptr=p.start;
    while (ptr!=NULL)
    {  
        pos=addterm(ptr->an,ptr->n,pos);
        ptr=ptr->next;
    }    
    return *this;
}

FPoly operator+(const FPoly &a,const FPoly &b)
{
    FPoly r=a;
    r+=b;
    return r;
}

FPoly& FPoly::operator-=(const FPoly& p)
{
    fterm *ptr,*pos=NULL;
    ptr=p.start;
    while (ptr!=NULL)
    {  
        pos=addterm(-(ptr->an),ptr->n,pos);
        ptr=ptr->next;
    }    
    return *this;
}

FPoly operator-(const FPoly &a,const FPoly &b)
{
    FPoly r=a;
    r-=b;
    return r;
}
 
void FPoly::multerm(Float a,int power)
{
    fterm *ptr=start;
    while (ptr!=NULL)
    {
        ptr->an*=a;
        ptr->n+=power;
        ptr=ptr->next;
    }
}

fterm* FPoly::addterm(Float a,int power,fterm *pos)
{
    fterm* newone;  
    fterm* ptr;
    fterm *t,*iptr;
    ptr=start;
    iptr=NULL;
    if (a.iszero()) return pos;

// quick scan through to detect if term exists already
// and to find insertion point

    if (pos!=NULL) ptr=pos;
    while (ptr!=NULL) 
    { 
        if (ptr->n==power)
        {
            ptr->an+=a;
            if (ptr->an.iszero()) 
            { // delete term
                if (ptr==start)
                { // delete first one
                    start=ptr->next;
                    delete ptr;
                    return start;
                }
                iptr=ptr;
                ptr=start;
                while (ptr->next!=iptr)ptr=ptr->next;
                ptr->next=iptr->next;
                delete iptr;
                return ptr;
            }
            return ptr;
        }
        if (ptr->n>power) iptr=ptr;
        else break;
        ptr=ptr->next;
    }
    newone=new fterm;
    newone->next=NULL;
    newone->an=a;
    newone->n=power;
    pos=newone;
    if (start==NULL)
    {
        start=newone;
        return pos;
    }

// insert at the start

    if (iptr==NULL)
    { 
        t=start;
        start=newone;
        newone->next=t;
        return pos;
    }

// insert new term

    t=iptr->next;
    iptr->next=newone;
    newone->next=t;
    return pos;    
}

ostream& operator<<(ostream& s,const FPoly& p)
{
    int first=1;
    Float a;
    fterm *ptr=p.start;
    if (ptr==NULL)
    { 
        s << "0";
        return s;
    }
    while (ptr!=NULL)
    {
        
        a=ptr->an;
        if (a>0)
        {
            if (!first) s << " + ";
        }
        else            s << " - ";

        if (a<0) a=(-a);
        if (ptr->n==0) 
           s << a; 
        else 
        {
            if (a!=(Float)1) s << a << "*x"; 
            else            s << "x";
            if (ptr->n!=1)  s << "^" << ptr->n;
        }
        first=0;
        ptr=ptr->next;
    }
    return s;
} 

