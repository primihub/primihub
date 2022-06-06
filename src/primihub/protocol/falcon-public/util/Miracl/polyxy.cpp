/*
 * C++ class to implement a bivariate polynomial type and to allow 
 * arithmetic on such polynomials whose coefficients are from
 * the finite field mod p
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 * See Knuth The Art of Computer Programming Vol.2, Chapter 4.6 
 */

#include "polyxy.h"

PolyXY::PolyXY(const ZZn& c, int p, int y)
{
    start=NULL;
    addterm(c,p,y);
}

PolyXY::PolyXY(int p)
{
    start=NULL;
    addterm((ZZn)p,0,0);
}

PolyXY::PolyXY(const PolyXY& p)
{
    termXY *ptr=p.start;
    termXY *pos=NULL;
    start=NULL;
    while (ptr!=NULL)
    {  
        pos=addterm(ptr->an,ptr->nx,ptr->ny,pos);
        ptr=ptr->next;
    }    
}

PolyXY::PolyXY(const Poly& p)
{
    term *ptr=p.start;
    termXY *pos=NULL;
    start=NULL;
    while (ptr!=NULL)
    {
        pos=addterm(ptr->an,ptr->n,0,pos);
        ptr=ptr->next;
    }
}

PolyXY::~PolyXY()
{
    termXY *nx;
    while (start!=NULL)
    {   
        nx=start->next;
        delete start;
        start=nx;
    }
}

ZZn PolyXY::coeff(int powx,int powy) const
{
    ZZn c=0;
    termXY *ptr=start;
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

PolyXY diff_dx(const PolyXY& f)
{
    PolyXY r;
    termXY *pos=NULL;
    termXY *ptr=f.start;
    while (ptr!=NULL)
    {
        pos=r.addterm(ptr->an*ptr->nx,ptr->nx-1,ptr->ny,pos);
        ptr=ptr->next;
    }
    return r;
}

PolyXY diff_dy(const PolyXY& f)
{
    PolyXY r;
    termXY *pos=NULL;
    termXY *ptr=f.start;
    while (ptr!=NULL)
    {
        pos=r.addterm(ptr->an*ptr->ny,ptr->nx,ptr->ny-1,pos);
        ptr=ptr->next;
    }
    return r;
}

Poly PolyXY::F(const ZZn& y)
{
    Poly r;
    term *pos=NULL;
    int i,maxy=0;
    ZZn f;
    termXY *ptr=start;

    while (ptr!=NULL)
    {
        if (ptr->ny>maxy) maxy=ptr->ny;
        ptr=ptr->next;
    }

    // max y is max power of y present

    ZZn *pw=new ZZn[maxy+1];   // powers of y

    pw[0]=(ZZn)1;
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

ZZn PolyXY::F(const ZZn& x,const ZZn& y)
{
    Poly r=F(y);
    return r.F(x);
}

void PolyXY::clear()
{
    termXY *ptr;
    while (start!=NULL)
    {   
        ptr=start->next;
        delete start;
        start=ptr;
    }

}

PolyXY& PolyXY::operator=(const PolyXY& p)
{
    termXY *ptr,*pos=NULL;
    clear();
    ptr=p.start;
    while (ptr!=NULL)
    {  
        pos=addterm(ptr->an,ptr->nx,ptr->ny,pos);
        ptr=ptr->next;
    }    
    return *this;

}

termXY* PolyXY::addterm(const ZZn& a,int powx,int powy,termXY *pos)
{
    termXY* newone;  
    termXY* ptr;
    termXY *t,*iptr;
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
    newone=new termXY;
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

ostream& operator<<(ostream& s,const PolyXY& p)
{
    BOOL first=TRUE;
    ZZn a;
    termXY *ptr=p.start;
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
            if (a!=(ZZn)1) s << a << "*";
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

PolyXY& PolyXY::operator=(int p)
{
    clear();
    addterm((ZZn)p, 0, 0);
    return *this;
}

/*
PolyXY operator*(Variable x,Variable y)
{
    PolyXY t((ZZn)1,1,1);
    return t;
}
*/

PolyXY powX(Variable x,int n)
{
    PolyXY r((ZZn)1,n,0);
    return r;
}

PolyXY powY(Variable y,int n)
{
    PolyXY r((ZZn)1,0,n);
    return r;
}


PolyXY operator%(const PolyXY& u,const PolyXY& v)
{
    PolyXY r=u;
    r%=v;
    return r;
}

PolyXY& PolyXY::operator%=(const PolyXY& v)
{
    ZZn m,pq;
    int power, power2;
    termXY *rptr=start;
    termXY *vptr=v.start;
    termXY *ptr,*pos;
    if (degreeX(*this)<degreeX(v) && degreeY(*this)<degreeY(v)) return *this;
    m=((ZZn)1/vptr->an);

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

int degreeX(const PolyXY& p)
{
    if (p.start==NULL) return 0;
    else return p.start->nx;
}

int degreeY(const PolyXY& p)
{
    termXY *ptr=p.start;
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

BOOL iszero(const PolyXY& p)
{
    if (degreeX(p)==0 && degreeY(p)==0 && (p.coeff(0,0)==0)) return TRUE;
    else return FALSE;
}

PolyXY operator+(const PolyXY& a,const PolyXY& b)
{
    PolyXY sum;
    sum=a;
    sum+=b;
    return sum;
}

PolyXY operator+(const PolyXY& a,const ZZn& b)
{
    PolyXY sum=a;
    sum.addterm(b,0,0);
    return sum;
}

PolyXY operator-(const PolyXY& a,const PolyXY& b)
{
    PolyXY sum;
    sum=a;
    sum-=b;
    return sum;
}

PolyXY& PolyXY::operator+=(const PolyXY& p)
{
    termXY *ptr,*pos=NULL;
    ptr=p.start;
    while (ptr!=NULL)
    {
        pos=addterm(ptr->an,ptr->nx,ptr->ny,pos);
        ptr=ptr->next;
    }
    return *this;
}

PolyXY& PolyXY::operator-=(const PolyXY& p)
{
    termXY *ptr,*pos=NULL;
    ptr=p.start;
    while (ptr!=NULL)
    {
        pos=addterm(-(ptr->an),ptr->nx,ptr->ny,pos);
        ptr=ptr->next;
    }
    return *this;
}

PolyXY& PolyXY::operator*=(const ZZn& x)
{
    termXY *ptr=start;
    while (ptr!=NULL)
    {
        ptr->an*=x;
        ptr=ptr->next;
    }
    return *this;
}

BOOL operator==(const PolyXY& a,const PolyXY& b)
{
    PolyXY diff=a-b;
    if (iszero(diff)) return TRUE;
    return FALSE;
}

PolyXY operator*(const PolyXY &p,const ZZn& z)
{
    PolyXY r=p;
    r*=z;
    return r;
}

BOOL isone(const PolyXY& p)
{
    if (degreeX(p)==0 && degreeY(p)==0 && p.coeff(0,0)==1) return TRUE;
    else return FALSE;
}

/*
 * This function is only meant to be called from compose. It puts in a value
 * for y, and then merges it with the x value
 */
PolyXY compoY(const PolyXY& q, const PolyXY& p)
{
    PolyXY poly;
    PolyXY temp(p);
    Variable x,y;
    termXY *qptr = q.start;
    termXY *pos=NULL;
    termXY *pptr = p.start;
    int qdegree = 0;
    poly.start = NULL;

    while (qptr!=NULL) {
        qdegree = qptr->ny;
        // Multiply out y term
        if(qdegree > 0) {
            temp = p;
            for(int i = 1; i < qdegree; i++)
                temp = temp * p;// Multiply by x term if it exists
            if(qptr->nx > 0)
                temp = temp * powX(x,qptr->nx);

            temp = temp * qptr->an;
            poly += temp;
        } else {
            // Multiply by x term if it exists
            if(qptr->nx > 0) {
                // poly = poly + powX(x,qptr->nx);
                temp = powX(x,qptr->nx);
                temp = temp * qptr->an;
                poly = poly + temp;
            } else
                poly = poly + qptr->an;
        }

        qptr = qptr->next;
        pptr = p.start;
    }

    return poly;
}


PolyXY compose(const PolyXY& q,const PolyXY& p,const PolyXY& p2)
{ // compose polynomials
    // assume P(x) = P3x^3 + P2x^2 + P1x^1 +P0
    // Calculate P(Q(x)) = P3.(Q(x))^3 + P2.(Q(x))^2 ....   
    PolyXY poly;
    PolyXY temp(p);
    Variable x,y;
    termXY *qptr = q.start;
    termXY *pos=NULL;
    termXY *pptr = p.start;
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
            if(qptr->ny > 0)temp = temp * powY(y,qptr->ny);

            temp = temp * qptr->an;
            poly += temp;
        } else {
            // Multiply by y term if it exists
            if(qptr->ny > 0) {
                temp = powY(y,qptr->ny);
                temp = temp * qptr->an;
                poly = poly + temp;
               //  poly = poly + (qptr->an * powY(y,qptr->ny));
            }
            else
                poly = poly + qptr->an;
        }

        qptr = qptr->next;
        pptr = p.start;
    }

    // return poly;

    PolyXY result = compoY(poly, p2);
    return result;
}


PolyXY operator*(const PolyXY& a,const PolyXY& b)
{
    int i,d,dega,degb,deg;
    ZZn t;
    PolyXY prod;
    termXY *iptr,*pos;
    termXY *ptr=b.start;
    if (&a==&b)
    { // squaring - only diagonal terms count!
        pos=NULL;
        while (ptr!=NULL)
        { // diagonal terms
            pos=prod.addterm(ptr->an*ptr->an,ptr->nx+ptr->nx,ptr->ny+ptr->ny,pos
                    );
            ptr=ptr->next;
        }
        return prod;
    }

    while (ptr!=NULL) {
        pos=NULL;
        iptr=a.start;
        while (iptr!=NULL)
        {
            pos=prod.addterm(ptr->an*iptr->an,ptr->nx+iptr->nx,ptr->ny+iptr->ny,
                    pos);
            iptr=iptr->next;
        }
        ptr=ptr->next;
    }

    return prod;
}

// Convert a polyxy object to a poly object by just taking the
// "x" value
Poly PolyXY::convert_x() const
{
    termXY *ptr=start;
    term *pos=NULL;
    Poly newpoly;
    while (ptr!=NULL)
    {
        pos = newpoly.addterm(ptr->an,ptr->nx,pos);
        ptr = ptr->next;
    }
    return newpoly;
}

// Convert a polyxy object to a poly object by just taking the
// "x" value when the corresponding "y" value is 0
Poly PolyXY::convert_x2() const
{
    termXY *ptr=start;
    term *pos=NULL;
    Poly newpoly;
    while (ptr!=NULL)
    {
        if(ptr->ny == 0)
            pos = newpoly.addterm(ptr->an,ptr->nx,pos);
        ptr = ptr->next;
    }
    return newpoly;
}

// Convert a polyxy object to a poly object by just taking the
// "x" value when the corresponding "y" value is greater than 0
Poly PolyXY::convert_x3() const
{
    termXY *ptr=start;
    term *pos=NULL;
    Poly newpoly;
    while (ptr!=NULL)
    {
        if(ptr->ny > 0)
            pos = newpoly.addterm(ptr->an,ptr->nx,pos);
        ptr = ptr->next;
    }
    return newpoly;
}

