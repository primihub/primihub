/*
 * C++ class to implement a bivariate polynomial type and to allow 
 * arithmetic on such polynomials whose coefficients are from
 * the finite field of characteristic 2
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 * See Knuth The Art of Computer Programming Vol.2, Chapter 4.6 
 */

#include "poly2xy.h"
#include <iostream>

using namespace std;

Poly2XY::Poly2XY(const GF2m& c, int p, int y)
{
    start=NULL;
    addterm(c,p,y);
}

Poly2XY::Poly2XY(int p)
{
    start=NULL;
    addterm((GF2m)p,0,0);
}

Poly2XY::Poly2XY(const Poly2XY& p)
{
    term2XY *ptr=p.start;
    term2XY *pos=NULL;
    start=NULL;
    while (ptr!=NULL)
    {  
        pos=addterm(ptr->an,ptr->nx,ptr->ny,pos);
        ptr=ptr->next;
    }    
}

Poly2XY::Poly2XY(const Poly2& p)
{
    term2 *ptr=p.start;
    term2XY *pos=NULL;
    start=NULL;
    while (ptr!=NULL)
    {
        pos=addterm(ptr->an,ptr->n,0,pos);
        ptr=ptr->next;
    }
}

Poly2XY::~Poly2XY()
{
    term2XY *nx;
    while (start!=NULL)
    {   
        nx=start->next;
        delete start;
        start=nx;
    }
}

GF2m Poly2XY::coeff(int powx,int powy) const
{
    GF2m c=0;
    term2XY *ptr=start;
    while (ptr!=NULL)
    {
        if (ptr->nx==powx && ptr->ny==powy)
        {
            c=ptr->an;
            return c;
        }
        ptr=ptr->next;
    }
    return c;
}

Poly2XY diff_dx(const Poly2XY& f)
{
    Poly2XY r;
    term2XY *pos=NULL;
    term2XY *ptr=f.start;
    while (ptr!=NULL)
    {
        pos=r.addterm(ptr->an*(GF2m)ptr->nx,ptr->nx-1,ptr->ny,pos);
        ptr=ptr->next;
    }
    return r;
}

Poly2XY diff_dy(const Poly2XY& f)
{
    Poly2XY r;
    term2XY *pos=NULL;
    term2XY *ptr=f.start;
    while (ptr!=NULL)
    {
        pos=r.addterm(ptr->an*(GF2m)ptr->ny,ptr->nx,ptr->ny-1,pos);
        ptr=ptr->next;
    }
    return r;
}

Poly2 Poly2XY::F(const GF2m& y)
{
    Poly2 r;
    term2 *pos=NULL;
    int i,maxy=0;
    GF2m f;
    term2XY *ptr=start;

    while (ptr!=NULL)
    {
        if (ptr->ny>maxy) maxy=ptr->ny;
        ptr=ptr->next;
    }

    // max y is max power of y present

    GF2m *pw=new GF2m[maxy+1];   // powers of y

    pw[0]=(GF2m)1;
    for (i=1;i<=maxy;i++)
        pw[i]=y*pw[i-1];

    ptr=start;
    while (ptr!=NULL)
    {
        pos=r.addterm(ptr->an*pw[ptr->ny],ptr->nx,pos);
        ptr=ptr->next;
    }

    delete [] pw;

    return r;
}

GF2m Poly2XY::F(const GF2m& x,const GF2m& y)
{
    Poly2 r=F(y);
    // cout << "r: " << r << endl;
    return r.F(x);
}

void Poly2XY::clear()
{
    term2XY *ptr;
    while (start!=NULL)
    {   
        ptr=start->next;
        delete start;
        start=ptr;
    }

}

// Convert a poly2xy object to a poly2 object by just taking the
// "x" value
Poly2 Poly2XY::convert_x() const
{
    term2XY *ptr=start;
    term2 *pos=NULL;
    Poly2 newpoly;
    while (ptr!=NULL)
    {  
        pos = newpoly.addterm(ptr->an,ptr->nx,pos);
        ptr = ptr->next;
    }    
    return newpoly;
}


// Convert a poly2xy object to a poly2 object by just taking the
// "x" value when the corresponding "y" value is 0
Poly2 Poly2XY::convert_x2() const
{
    term2XY *ptr=start;
    term2 *pos=NULL;
    Poly2 newpoly;
    while (ptr!=NULL)
    {  
        if(ptr->ny == 0)
            pos = newpoly.addterm(ptr->an,ptr->nx,pos);
        ptr = ptr->next;
    }    
    return newpoly;
}

// Convert a poly2xy object to a poly2 object by just taking the
// "x" value when the corresponding "y" value is greater than 0
Poly2 Poly2XY::convert_x3() const
{
    term2XY *ptr=start;
    term2 *pos=NULL;
    Poly2 newpoly;
    while (ptr!=NULL)
    {  
        if(ptr->ny > 0) 
            pos = newpoly.addterm(ptr->an,ptr->nx,pos);
        ptr = ptr->next;
    }    
    return newpoly;
}



Poly2XY& Poly2XY::operator=(int p)
{
    clear();
    addterm((GF2m)p, 0, 0);
    return *this;
}

Poly2XY& Poly2XY::operator=(const Poly2XY& p)
{
    term2XY *ptr,*pos=NULL;
    clear();
    ptr=p.start;
    while (ptr!=NULL)
    {  
        pos=addterm(ptr->an,ptr->nx,ptr->ny,pos);
        ptr=ptr->next;
    }    
    return *this;

}

term2XY* Poly2XY::addterm(const GF2m& a,int powx,int powy,term2XY *pos)
{
    term2XY* newone;  
    term2XY* ptr;
    term2XY *t,*iptr;
    ptr=start;
    iptr=NULL;
    if (a.iszero()) return pos;

    // quick scan through to detect if term exists already
    // and to find insertion point
    if (pos!=NULL) ptr=pos;
    while (ptr!=NULL) 
    { 
        if (ptr->nx==powx && ptr->ny==powy)
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
        if (ptr->nx>powx) iptr=ptr;
        if (ptr->nx==powx && ptr->ny>powy) iptr=ptr;
        ptr=ptr->next;
    }
    newone=new term2XY;
    newone->next=NULL;
    newone->an=a;
    newone->nx=powx;
    newone->ny=powy;

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

ostream& operator<<(ostream& s,const Poly2XY& p)
{
    BOOL first=TRUE;
    GF2m a;
    term2XY *ptr=p.start;
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
        if (ptr->nx==0 && ptr->ny==0) 
            s << a; 
        else 
        {
            if (a!=(GF2m)1) s << a << "*";
            if (ptr->nx!=0)
            {
                s << "x";
                if (ptr->nx!=1)  s << "^" << ptr->nx;
            }
            if (ptr->ny!=0)
            {
                if (ptr->nx!=0) s << ".";
                s << "y";
                if (ptr->ny!=1)  s << "^" << ptr->ny;
            }
        }
        first=FALSE;
        ptr=ptr->next;
    }
    return s;
} 

/*
   Poly2XY operator*(const GF2m& c,Variable x)
   {   
   Poly2XY t(c,1);
   return t;
   }
   */

Poly2XY operator*(Variable x,Variable y)
{   
    Poly2XY t((GF2m)1,1,1);
    return t;
}

Poly2XY pow2X(Variable x,int n)
{   
    Poly2XY r((GF2m)1,n,0);
    return r;
}

Poly2XY pow2Y(Variable y,int n)
{   
    Poly2XY r((GF2m)1,0,n);
    return r;
}

Poly2XY operator%(const Poly2XY& u,const Poly2XY& v)
{
    Poly2XY r=u;
    r%=v;
    return r;
}

/*
   Poly2XY operator*(const Poly2XY& u,const Poly2XY& v)
   {
   Poly2XY r=u;
   r*=v;
   return r;
   }
   */

Poly2XY& Poly2XY::operator%=(const Poly2XY& v)
{
    GF2m m,pq;
    int power, power2;
    term2XY *rptr=start;
    term2XY *vptr=v.start;
    term2XY *ptr,*pos;
    if (degreeX(*this)<degreeX(v) && degreeY(*this)<degreeY(v)) return *this;
    m=((GF2m)1/vptr->an);

    while (rptr!=NULL && rptr->nx>=vptr->nx && rptr->ny>=vptr->ny)
    {
        pq=rptr->an*m;
        power=rptr->nx-vptr->nx;
        power2=rptr->ny-vptr->ny;
        pos=NULL;
        ptr=v.start;
        while (ptr!=NULL)
        {
            pos=addterm(ptr->an*pq,ptr->nx+power,ptr->ny+power2,pos);
            ptr=ptr->next;
        }
        rptr=start;
    }

    return *this;
}

int degreeX(const Poly2XY& p)
{   
    if (p.start==NULL) return 0;
    else return p.start->nx;
}

int degreeY(const Poly2XY& p)
{   
    term2XY *ptr=p.start;
    int maxdeg = 0;

    if (p.start==NULL) return 0;

    while (ptr!=NULL)
    {  
        if(ptr->ny > maxdeg)
            maxdeg = ptr->ny;
        ptr = ptr->next;
    }    
    return maxdeg;
}

BOOL iszero(const Poly2XY& p)
{
    if (degreeX(p)==0 && degreeY(p)==0 && p.coeff(0,0)==0) return TRUE;
    else return FALSE;
}

Poly2XY operator+(const Poly2XY& a,const Poly2XY& b)
{
    Poly2XY sum;
    sum=a;
    sum+=b;
    return sum;
}

Poly2XY operator+(const Poly2XY& a,const GF2m& b)
{
    Poly2XY sum=a;
    sum.addterm(b,0,0);
    return sum;
}

Poly2XY operator-(const Poly2XY& a,const Poly2XY& b)
{
    Poly2XY sum;
    sum=a;
    sum-=b;
    return sum;
}

Poly2XY& Poly2XY::operator+=(const Poly2XY& p)
{
    term2XY *ptr,*pos=NULL;
    ptr=p.start;
    while (ptr!=NULL)
    {
        pos=addterm(ptr->an,ptr->nx,ptr->ny,pos);
        ptr=ptr->next;
    }
    return *this;
}

Poly2XY& Poly2XY::operator-=(const Poly2XY& p)
{
    term2XY *ptr,*pos=NULL;
    ptr=p.start;
    while (ptr!=NULL)
    {
        pos=addterm(-(ptr->an),ptr->nx,ptr->ny,pos);
        ptr=ptr->next;
    }
    return *this;
}

Poly2XY& Poly2XY::operator*=(const GF2m& x)
{
    term2XY *ptr=start;
    while (ptr!=NULL)
    {
        ptr->an*=x;
        ptr=ptr->next;
    }
    return *this;
}

BOOL operator==(const Poly2XY& a,const Poly2XY& b)
{
    Poly2XY diff=a-b;
    if (iszero(diff)) return TRUE;
    return FALSE;
}

Poly2XY operator*(const Poly2XY &p,const GF2m& z)
{
    Poly2XY r=p;
    r*=z;
    return r;
}

BOOL isone(const Poly2XY& p)
{
    if (degreeX(p)==0 && degreeY(p)==0 && p.coeff(0,0)==1) return TRUE;
    else return FALSE;
}

/*
 * This function is only meant to be called from compose. It puts in a value
 * for y, and then merges it with the x value
 */
Poly2XY compoY(const Poly2XY& q, const Poly2XY& p)
{           
    Poly2XY poly;
    Poly2XY temp(p);
    Variable x,y;
    term2XY *qptr = q.start;
    term2XY *pos=NULL;
    term2XY *pptr = p.start;
    int qdegree = 0; 
    poly.start = NULL;

    while (qptr!=NULL) {
       qdegree = qptr->ny;
       // Multiply out y term
       if(qdegree > 0) {
           temp = p;
           for(int i = 1; i < qdegree; i++)
               temp = temp * p;

           // Multiply by x term if it exists
           if(qptr->nx > 0)
               temp = temp * pow2X(x,qptr->nx);

           poly += temp;
       } else {
           // Multiply by x term if it exists
           if(qptr->nx > 0)
               poly = poly + pow2X(x,qptr->nx);
           else
               poly = poly + qptr->an;
       }

       qptr = qptr->next;
       pptr = p.start;
   }

   return poly;
}

Poly2XY compose(const Poly2XY& q,const Poly2XY& p,const Poly2XY& p2)
{ // compose polynomials
    // assume P(x) = P3x^3 + P2x^2 + P1x^1 +P0
    // Calculate P(Q(x)) = P3.(Q(x))^3 + P2.(Q(x))^2 ....   
    Poly2XY poly;
    Poly2XY temp(p);
    Variable x,y;
    term2XY *qptr = q.start;
    term2XY *pos=NULL;
    term2XY *pptr = p.start;
    int qdegree = 0;
    poly.start = NULL;

    while (qptr!=NULL) {
        qdegree = qptr->nx;
        // Multiply out x term
        if(qdegree > 0) {
            temp = p;
            for(int i = 1; i < qdegree; i++)
                temp = temp * p;

            // Multiply by y term if it exists
            if(qptr->ny > 0)
                temp = temp * pow2Y(y,qptr->ny);

            poly += temp;
        } else {
            // Multiply by y term if it exists
            if(qptr->ny > 0)
                poly = poly + pow2Y(y,qptr->ny);
            else
                poly = poly + qptr->an;
        }

        qptr = qptr->next;
        pptr = p.start;
    }

    // return poly;

    Poly2XY result = compoY(poly, p2);
    return result;
}

Poly2XY operator*(const Poly2XY& a,const Poly2XY& b)
{
    int i,d,dega,degb,deg;
    GF2m t;
    Poly2XY prod;
    term2XY *iptr,*pos;
    term2XY *ptr=b.start;
    if (&a==&b)
    { // squaring - only diagonal terms count!
        pos=NULL;
        while (ptr!=NULL)
        { // diagonal terms
            pos=prod.addterm(ptr->an*ptr->an,ptr->nx+ptr->nx,ptr->ny+ptr->ny,pos);
            ptr=ptr->next;
        }
        return prod;
    }

/*
    dega=degree(a);
    deg=dega;
    degb=degree(b);
    if (degb<dega) deg=degb;  // deg is smallest

    if (deg>=KARAT_BREAK_EVEN)
    { // use fast method 
        int len,m,inc;

        big *A,*B,*C,*T;
        deg=dega;
        if (dega<degb) deg=degb;   // deg is biggest
        m=deg; inc=1;
        while (m!=0) { m/=2; inc++; }

        len=2*(deg+inc);

        A=(big *)mr_alloc(deg+1,sizeof(big));
        B=(big *)mr_alloc(deg+1,sizeof(big));
        C=(big *)mr_alloc(len,sizeof(big));
        T=(big *)mr_alloc(len,sizeof(big));

        char *memc=(char *)memalloc(len);
        char *memt=(char *)memalloc(len);
        for (i=0;i<len;i++)
        {
            C[i]=mirvar_mem(memc,i);
            T[i]=mirvar_mem(memt,i);
        }

        ptr=a.start;
        while (ptr!=NULL)
        {
            A[ptr->n]=getbig(ptr->an);
            ptr=ptr->next;
        }
        ptr=b.start;
        while (ptr!=NULL)
        {
            B[ptr->n]=getbig(ptr->an);
            ptr=ptr->next;
        }

        karmul2_poly(deg+1,T,A,B,C);

        pos=NULL;
        for (d=dega+degb;d>=0;d--)
        {
            t=(Big)C[d];
            if (t.iszero()) continue;
            pos=prod.addterm(t,d,pos);
        }

        memkill(memc,len);
        memkill(memt,len);

        mr_free(T);
        mr_free(C);
        mr_free(B);
        mr_free(A);
        return prod;
    }
*/

    while (ptr!=NULL)
    {
        pos=NULL;
        iptr=a.start;
        while (iptr!=NULL)
        {
            pos=prod.addterm(ptr->an*iptr->an,ptr->nx+iptr->nx,ptr->ny+iptr->ny,pos);
            iptr=iptr->next;
        }
        ptr=ptr->next;
    }

    return prod;
}
