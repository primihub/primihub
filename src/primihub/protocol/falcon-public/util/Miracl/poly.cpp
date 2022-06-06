/*
 * C++ class to implement a polynomial type and to allow 
 * arithmetic on polynomials whose elements are from
 * the finite field mod p
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 * See Knuth The Art of Computer Programming Vol.2, Chapter 4.6 
 */

#include "poly.h"

#include <iostream>
using namespace std;

Poly::Poly(const ZZn& c,int p)
{
    start=NULL;
    addterm(c,p);
}

Poly::Poly(Variable &x)
{
    start=NULL;
    addterm((ZZn)1,1);
}

Poly operator-(const Poly& a)
{
    Poly p=a;
    p.multerm((ZZn)-1,0);
    return p;
}

Poly operator*(const ZZn& c,Variable x)
{
    Poly t(c,1);
    return t;
}

Poly pow(Variable x,int n)
{
    Poly r((ZZn)1,n);
    return r;
}

BOOL operator==(const Poly& a,const Poly& b)
{
    Poly diff=a-b;
    if (iszero(diff)) return TRUE;
    return FALSE;
}

BOOL operator!=(const Poly& a,const Poly& b)
{
    Poly diff=a-b;
    if (iszero(diff)) return FALSE;
    return TRUE;
}

void setpolymod(const Poly& p) 
{ 
    int n,m;
    Poly h;
    term *ptr;
    big *f,*rf;
    n=degree(p);
    if (n<FFT_BREAK_EVEN) return;
    h=reverse(p);
    h=invmodxn(h,n);
    h=reverse(h);   // h=RECIP(f)
    m=degree(h);
    if (m<n-1) h=mulxn(h,n-1-m);

    f=(big *)mr_alloc(n+1,sizeof(big));
    rf=(big *)mr_alloc(n+1,sizeof(big));
 
    ptr=p.start;
    while (ptr!=NULL)
    {
       f[ptr->n]=getbig(ptr->an);
       ptr=ptr->next;
    }   
    ptr=h.start;
    while (ptr!=NULL)
    {
       rf[ptr->n]=getbig(ptr->an);
       ptr=ptr->next;
    }   
 
    mr_polymod_set(n,rf,f);

    mr_free(rf);
    mr_free(f);
}

Poly::Poly(const Poly& p)
{
    term *ptr=p.start;
    term *pos=NULL;
    start=NULL;
    while (ptr!=NULL)
    {  
        pos=addterm(ptr->an,ptr->n,pos);
        ptr=ptr->next;
    }    
}


Poly::~Poly()
{
   term *nx;
   while (start!=NULL)
   {   
       nx=start->next;
       delete start;
       start=nx;
   }
}

ZZn Poly::coeff(int power)  const
{
    ZZn c=0;
    term *ptr=start;
    while (ptr!=NULL)
    {
        if (ptr->n==power)
        {
            c=ptr->an;
            return c;
        }
        ptr=ptr->next;
    }
    return c;
}

ZZn Poly::F(const ZZn& x) const
{
    ZZn f=0;
    int diff;
    term *ptr=start;

// Horner's rule

    if (ptr==NULL) return f;
    f=ptr->an;

    while (ptr->next!=NULL)
    {
        diff=ptr->n-ptr->next->n;
        if (diff==1) f=f*x+ptr->next->an;
        else         f=f*pow(x,diff)+ptr->next->an;    
        ptr=ptr->next;
    }
    f*=pow(x,ptr->n);

    return f;
}

ZZn Poly:: min() const
{
    term *ptr=start;
    if (start==NULL) return (ZZn)0;
    
    while (ptr->next!=NULL) ptr=ptr->next;
    return (ptr->an);
}

Poly compose(const Poly& g,const Poly& b,const Poly& m)
{ // compose polynomials
  // assume G(x) = G3x^3 + G2x^2 + G1x^1 +G0
  // Calculate G(B(x) = G3.(B(x))^3 + G2.(B(x))^2 ....   
    Poly c,t;  
    term *ptr;
    int i,d=degree(g);
    Poly *table=new Poly[d+1];
    table[0].addterm((ZZn)1,0);
    for (i=1;i<=d;i++) table[i]=(table[i-1]*b)%m;
    ptr=g.start;
    while (ptr!=NULL)
    {
        c+=ptr->an*table[ptr->n];
        c=c%m;
        ptr=ptr->next;
    }
    delete [] table;
    return c;
}

Poly compose(const Poly& g,const Poly& b)
{ // compose polynomials
  // assume G(x) = G3x^3 + G2x^2 + G1x^1 +G0
  // Calculate G(B(x) = G3.(B(x))^3 + G2.(B(x))^2 ....   
    Poly c,t;  
    term *ptr;
    int i,d=degree(g);
    Poly *table=new Poly[d+1];
    table[0].addterm((ZZn)1,0);
    for (i=1;i<=d;i++) table[i]=(table[i-1]*b);
    ptr=g.start;
    while (ptr!=NULL)
    {
        c+=ptr->an*table[ptr->n];
        ptr=ptr->next;
    }
    delete [] table;
    return c;
}

Poly reduce(const Poly &x,const Poly &m)
{
    Poly r;
    int i,d;
    ZZn t;
    big *G,*R;
    term *ptr,*pos=NULL;
    int degm=degree(m);
    int n=degree(x);

    if (degm < FFT_BREAK_EVEN || n-degm < FFT_BREAK_EVEN)
    {
        r=x%m;
        return r;
    }
    G=(big *)mr_alloc(n+1,sizeof(big));
    char *memg=(char *)memalloc(n+1);
    for (i=0;i<=n;i++) G[i]=mirvar_mem(memg,i);
    R=(big *)mr_alloc(degm,sizeof(big));
    char *memr=(char *)memalloc(degm);
    for (i=0;i<degm;i++) R[i]=mirvar_mem(memr,i);

    ptr=x.start;
    while (ptr!=NULL)
    {
        copy(getbig(ptr->an),G[ptr->n]);
        ptr=ptr->next; 
    }
    if (!mr_poly_rem(n,G,R))
    {  // reset the modulus - things have changed
        setpolymod(m);
        mr_poly_rem(n,G,R);
    }
 
    r.clear();

    for (d=degm-1;d>=0;d--)
    {
        t=R[d];
        if (t.iszero()) continue;
        pos=r.addterm(t,d,pos);
    }
    memkill(memr,degm);

    mr_free(R);
    memkill(memg,n+1);
    mr_free(G);

    return r;
}

Poly modmult(const Poly &x,const Poly &y,const Poly &m)
{ /* x*y mod m */
    Poly r=x*y;
    r=reduce(r,m);
    return r;
}

Poly operator*(const Poly& a,const Poly& b)
{
    int i,d,dega,degb,deg;
    BOOL squaring;
    ZZn t;
    Poly prod;
    term *iptr,*pos;
    term *ptr=b.start;

    squaring=FALSE;
    if (&a==&b) squaring=TRUE;

    dega=degree(a);
    deg=dega;
    if (!squaring)
    {
        degb=degree(b);
        if (degb<dega) deg=degb;
    }
    else degb=dega;
    if (deg>=FFT_BREAK_EVEN)      /* deg is minimum - both must be less than FFT_BREAK_EVEN */
    { // use fast methods 
        big *A,*B,*C;
        deg=dega+degb;     // degree of product
   
        A=(big *)mr_alloc(dega+1,sizeof(big));
        if (!squaring) B=(big *)mr_alloc(degb+1,sizeof(big));        
        C=(big *)mr_alloc(deg+1,sizeof(big));
        char *memc=(char *)memalloc(deg+1);
        for (i=0;i<=deg;i++) C[i]=mirvar_mem(memc,i);
        ptr=a.start;
        while (ptr!=NULL)
        {
            A[ptr->n]=getbig(ptr->an);
            ptr=ptr->next;
        }

        if (!squaring)
        {
            ptr=b.start;
            while (ptr!=NULL)
            {
                B[ptr->n]=getbig(ptr->an);
                ptr=ptr->next;
            }
            mr_poly_mul(dega,A,degb,B,C);
        }
        else mr_poly_sqr(dega,A,C);
        pos=NULL;
        for (d=deg;d>=0;d--)
        {
            t=C[d];
            if (t.iszero()) continue;
            pos=prod.addterm(t,d,pos);
        }
        memkill(memc,deg+1);
        mr_free(C);
        mr_free(A);
        if (!squaring) mr_free(B);

        return prod;
    }

    if (squaring)
    { // squaring
        pos=NULL;
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
                t=ptr->an*iptr->an;
                pos=prod.addterm(t+t,ptr->n+iptr->n,pos);
                iptr=iptr->next;
            }
            ptr=ptr->next; 
        }
    }
    else while (ptr!=NULL)
    {
        pos=NULL;
        iptr=a.start;
        while (iptr!=NULL)
        {
            pos=prod.addterm(ptr->an*iptr->an,ptr->n+iptr->n,pos);
            iptr=iptr->next;
        }
        ptr=ptr->next;
    }

    return prod;
}

Poly& Poly::operator%=(const Poly&v)
{
    ZZn m,pq;
    int power;
    term *rptr=start;
    term *vptr=v.start;
    term *ptr,*pos;
    if (degree(*this)<degree(v)) return *this;
    m=-((ZZn)1/vptr->an);
    while (rptr!=NULL && rptr->n>=vptr->n)
    {
        pq=rptr->an*m;
        power=rptr->n-vptr->n;
        pos=NULL;
        ptr=v.start;
        while (ptr!=NULL)
        {
            pos=addterm(ptr->an*pq,ptr->n+power,pos);
            ptr=ptr->next;
        } 
        rptr=start;
    }
    return *this;
}

Poly operator%(const Poly& u,const Poly&v)
{
    Poly r=u;
    r%=v;
    return r;
}

Poly operator/(const Poly& u,const Poly&v)
{
    Poly q,r=u;
    term *rptr=r.start;
    term *vptr=v.start;
    term *ptr,*pos;
    while (rptr!=NULL && rptr->n>=vptr->n)
    {
        Poly t=v;
        ZZn pq=rptr->an/vptr->an;
        int power=rptr->n-vptr->n;
  // quotient
        q.addterm(pq,power);
        t.multerm(-pq,power);
        ptr=t.start;
        pos=NULL;
        while (ptr!=NULL)
        {
            pos=r.addterm(ptr->an,ptr->n,pos);
            ptr=ptr->next;
        } 
        rptr=r.start;
    }
    return q;
}

Poly diff(const Poly& f)
{
   Poly d;
   term *pos=NULL;
   term *ptr=f.start;
   while (ptr!=NULL)
   {
       pos=d.addterm(ptr->an*ptr->n,ptr->n-1,pos);
       ptr=ptr->next;
   }

   return d;
}

Poly gcd(const Poly& f,const Poly& g)
{
    Poly a,b;
    a=f; b=g;
    term *ptr;
    forever
    {
        if (b.start==NULL)
        {
            ptr=a.start;
            a.multerm((ZZn)1/ptr->an,0);
            return a;
        }
        a%=b;
        if (a.start==NULL)
        {
            ptr=b.start;
            b.multerm((ZZn)1/ptr->an,0);
            return b;
        }
        b%=a;
    }
}

Poly pow(const Poly& f,int k)
{
    Poly u;
    int w,e,b;

    if (k==0)
    {
        u.addterm((ZZn)1,0);
        return u;
    }
    u=f;
    if (k==1) return u;

    e=k;
    b=0; while (k>1) {k>>=1; b++; }
    w=(1<<b);
    e-=w; w/=2;
    while (w>0)
    {
        u=(u*u);
        if (e>=w)
        {
           e-=w;
           u=(u*f);
        }
        w/=2; 
    }
    return u;
}

Poly pow(const Poly& f,const Big& k,const Poly& m)
{
    Poly u,t,u2,table[16];
    Big w,e;
    int i,j,nb,n,nbw,nzs;
    if (k==0) 
    {
        u.addterm((ZZn)1,0);
        return u;
    }
    u=f%m;
    if (k==1) return u;
    if (degree(m)>=FFT_BREAK_EVEN)
    { /* Uses FFT for fixed base - much faster for large m */
        setpolymod(m);

        u2=modmult(u,u,m);
        table[0]=u;
        for (i=1;i<16;i++)
            table[i]=modmult(u2,table[i-1],m);
        nb=bits(k);
        if (nb>1) for (i=nb-2;i>=0;)
        { 
            n=window(k,i,&nbw,&nzs,5);
            for (j=0;j<nbw;j++)
                u=modmult(u,u,m);
            if (n>0) u=modmult(u,table[n/2],m);
            i-=nbw;
            if (nzs)
            {
                for (j=0;j<nzs;j++) u=modmult(u,u,m);
                i-=nzs;  
            }
        }

        fft_reset();
    }
    else
    {
        e=k;
        w=pow((Big)2,bits(e)-1);
        e-=w; w/=2;
        while (w>0)
        {
            u=(u*u)%m;
            if (e>=w)
            {
               e-=w;
               u=(u*f)%m;
            }
            w/=2;
        }
    } 
    return u;
}

int degree(const Poly& p)
{
    if (p.start==NULL) return 0;
    else return p.start->n;
}


BOOL iszero(const Poly& p) 
{
    if (degree(p)==0 && p.coeff(0)==0) return TRUE;
    else return FALSE;
}

BOOL isone(const Poly& p)
{
    if (degree(p)==0 && p.coeff(0)==1) return TRUE;
    else return FALSE;
}

Poly factor(const Poly& f,int d)
{
    Poly c,u,h,g=f;
    Big r,p=get_modulus();
//    int lim=0;
    while (degree(g) > d)
    {
//       lim++;
//       if (lim>10) break;
// random monic polynomial
       u.clear();
       u.addterm((ZZn)1,2*d-1);
       for (int i=2*d-2;i>=0;i--)
       {
          r=rand(p);
          u.addterm((ZZn)r,i);
       }
       r=(pow(p,d)-1)/2;

       c=pow(u,r,g);
       c.addterm((ZZn)(-1),0);

       h=gcd(c,g);
       if (degree(h)==0 || degree(h)==degree(g)) continue;
       if (2*degree(h)>degree(g))
            g=g/h;
       else g=h;
    }
    return g;
}

void Poly::clear()
{
    term *ptr;
    while (start!=NULL)
    {   
       ptr=start->next;
       delete start;
       start=ptr;
    }
    
}

Poly& Poly::operator=(int m)
{
    clear();
    if (m!=0) addterm((ZZn)m,0);
    return *this;
}

Poly& Poly::operator=(const ZZn& m)
{
    clear();
    if (!m.iszero()) addterm(m,0);
    return *this;
}

Poly &Poly::operator=(const Poly& p)
{
    term *ptr,*pos=NULL;
    clear();
    ptr=p.start;
    while (ptr!=NULL)
    {  
        pos=addterm(ptr->an,ptr->n,pos);
        ptr=ptr->next;
    }    
    return *this;
}

Poly operator+(const Poly& a,const ZZn& z)
{
    Poly p=a;
    p.addterm(z,0);
    return p;
}

Poly operator-(const Poly& a,const ZZn& z)
{
    Poly p=a;
    p.addterm((-z),0);
    return p;
}


Poly operator+(const Poly& a,const Poly& b)
{
    Poly sum;
    sum=a;
    sum+=b;
    return sum;
}

Poly operator-(const Poly& a,const Poly& b)
{
    Poly sum;
    sum=a;
    sum-=b;
    return sum;
}

Poly& Poly::operator+=(const Poly& p)
{
    term *ptr,*pos=NULL;
    ptr=p.start;
    while (ptr!=NULL)
    {  
        pos=addterm(ptr->an,ptr->n,pos);
        ptr=ptr->next;
    }    
    return *this;
}

Poly& Poly::operator*=(const ZZn& x)
{
    term *ptr=start;
    while (ptr!=NULL)
    {
        ptr->an*=x;
        ptr=ptr->next;
    }
    return *this;
}

Poly operator*(const ZZn& z,const Poly &p)
{
    Poly r=p;
    r*=z;
    return r;
}

Poly operator*(const Poly &p,const ZZn& z)
{
    Poly r=p;
    r*=z;
    return r;
}

Poly operator/(const Poly& a,const ZZn& b)
{
    Poly quo;
    quo=a;
    quo/=(ZZn)b;
    return quo;
}

Poly& Poly::operator/=(const ZZn& x)
{
    ZZn t=(ZZn)1/x;
    term *ptr=start;
    while (ptr!=NULL)
    {
        ptr->an*=t;
        ptr=ptr->next;
    }
    return *this;
}

Poly& Poly::operator-=(const Poly& p)
{
    term *ptr,*pos=NULL;
    ptr=p.start;
    while (ptr!=NULL)
    {  
        pos=addterm(-(ptr->an),ptr->n,pos);
        ptr=ptr->next;
    }    
    return *this;
}
 
void Poly::multerm(const ZZn& a,int power)
{
    term *ptr=start;
    while (ptr!=NULL)
    {
        ptr->an*=a;
        ptr->n+=power;
        ptr=ptr->next;
    }
}

Poly invmodxn(const Poly& a,int n)
{ // Newton's method to find 1/a mod x^n
    int i,k;
    Poly b;
    k=0; while ((1<<k)<n) k++;
    b.addterm((ZZn)1/a.coeff(0),0); // important that a0 != 0
    for (i=1;i<=k;i++) b=modxn (2*b-a*b*b,1<<i);
    b=modxn(b,n);
    return b;
}

Poly modxn(const Poly& a,int n)
{ // reduce polynomial mod x^n
    Poly b;
    term* ptr=a.start;
    term *pos=NULL;
    while (ptr!=NULL && ptr->n>=n) ptr=ptr->next;
    while (ptr!=NULL)
    {
        pos=b.addterm(ptr->an,ptr->n,pos);
        ptr=ptr->next;
    }
    return b;
}

Poly divxn(const Poly& a,int n)
{ // divide polynomial by x^n
    Poly b;
    term *ptr=a.start;
    term *pos=NULL;
    while (ptr!=NULL)
    {
        if (ptr->n>=n)
            pos=b.addterm(ptr->an,ptr->n-n,pos);
        else break;
        ptr=ptr->next;
    }
    return b;
}

Poly mulxn(const Poly& a,int n)
{ // multiply polynomial by x^n
    Poly b;
    term *ptr=a.start;
    term *pos=NULL;
    while (ptr!=NULL)
    {
        pos=b.addterm(ptr->an,ptr->n+n,pos);
        ptr=ptr->next;
    }
    return b;
}

Poly reverse(const Poly& a)
{
    term *ptr=a.start;
    int deg=degree(a);
    Poly b;
    while (ptr!=NULL)
    {
        b.addterm(ptr->an,deg-ptr->n);
        ptr=ptr->next;
    } 
    return b;
}

// add term to polynomial. The pointer pos remembers the last
// accessed element - this is faster. 
// Polynomial is stored with large powers first, down to low powers
// e.g. 9x^6 + x^4 + 3x^2 + 1

term* Poly::addterm(const ZZn& a,int power,term *pos)
{
    term* newone;  
    term* ptr;
    term *t,*iptr;
    ptr=start;
    iptr=NULL;
    if (a.iszero()) return pos;
// quick scan through to detect if term exists already
// and to find insertion point
    if (pos!=NULL) ptr=pos;      // start looking from here
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
    newone=new term;
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

// A function to differentiate a polynomial
Poly differentiate(const Poly& orig)
{
    Poly newpoly;
    term *ptr = orig.start;
    term *pos = NULL;
    int power;

    while (ptr!=NULL)
    {   
        power = ptr->n;
        if(ptr->n > 0) 
            pos = newpoly.addterm(ptr->an*power,ptr->n-1,pos);
            
        ptr = ptr->next;
    }
    
    return newpoly;
}

void makemonic(Poly& p)
{
    term *ptr = p.start;
    p.multerm((ZZn)1/ptr->an,0);
}

// The extended euclidean algorithm
// The result is returned in an array of Polys, with the gcd
// in first place, then the two coefficients
void egcd(Poly result[], const Poly& u, const Poly& v)
{       
    Poly u1, u2, u3, v1, v2, v3, t1, t2, t3, zero, q;
    u1 = 1;  
    u2 = 0;
    u3 = u;
    v1 = 0;
    v2 = 1;
    v3 = v;
    zero = 0;
    term *ptr;
            
    while(v3 != zero) {
        q = u3/v3;
        t1 = u1 - v1*q;
        t2 = u2 - v2*q;
        t3 = u3 - v3*q;

        u1 = v1;
        u2 = v2;
        u3 = v3;

        v1 = t1;
        v2 = t2;
        v3 = t3;
    }
    result[0] = u3;
    result[1] = u1;
    result[2] = u2;

}

ostream& operator<<(ostream& s,const Poly& p)
{
    BOOL first=TRUE;
    ZZn a;
    term *ptr=p.start;
    if (ptr==NULL)
    { 
        s << "0";
        return s;
    }
    while (ptr!=NULL)
    {
        a=ptr->an;
        if ((Big)a<get_modulus()/2)
        {
            if (!first) s << " + ";
        }
        else 
        {
           a=(-a);
           s << " - ";
        }
        if (ptr->n==0) 
           s << (Big)a; 
        else 
        {
            if (a!=(ZZn)1)  s << (Big)a << "*x"; 
            else            s << "x";
            if (ptr->n!=1)  s << "^" << ptr->n;
        }
        first=FALSE;
        ptr=ptr->next;
    }
    return s;
} 

