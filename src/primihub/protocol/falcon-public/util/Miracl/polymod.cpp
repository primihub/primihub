/*
 * C++ class to implement a polynomial type and to allow 
 * arithmetic on polynomials whose elements are from
 * the finite field mod p. 
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 * This type is automatically reduced
 * wrt a polynomial Modulus.
 *
 * See Knuth The Art of Computer Programming Vol.2, Chapter 4.6 
 */

#include "polymod.h"

Poly Modulus;
  
BOOL iszero(const PolyMod& m)     {return iszero(m.p);}
BOOL isone(const PolyMod& m)      {return isone(m.p);}
int degree(const PolyMod& m)  {return degree(m.p);}

BOOL operator==(const PolyMod& a,const PolyMod& b)
{
    PolyMod diff=a-b;
    if (iszero(diff)) return TRUE;
    return FALSE;
}

BOOL operator!=(const PolyMod& a,const PolyMod& b)
{
    PolyMod diff=a-b;
    if (iszero(diff)) return FALSE;
    return TRUE;
}

          
ZZn PolyMod::coeff(int i) const {return p.coeff(i);}

PolyMod& PolyMod::operator*=(const PolyMod &b)
{
    int i,m,deg,dega,degb,degm=degree(Modulus);
    big *G,*B,*R;
    BOOL squaring;
    term *ptr=p.start;
    term *newone,*iptr;

    squaring=FALSE;
    if (this==&b) squaring=TRUE;

    dega=degree(*this);
    deg=dega;
    if (!squaring)
    {
        degb=degree(b);
        if (degb<dega) deg=degb;
    }
    else degb=dega;

    if (dega+degb-degm < FFT_BREAK_EVEN || deg < FFT_BREAK_EVEN)
    { // residue is small, or at least one of a or b is small
        reduce(p*b.p,*this);
        return *this;
    }

    R=(big *)mr_alloc(degm,sizeof(big));
  // pad out this to have zero terms
    iptr=NULL;
    m=degm-1;
    do 
    {
        if (ptr==NULL || ptr->n!=m) 
        {
            newone=new term;
            newone->next=ptr;
            newone->an=(ZZn)0;
            newone->n=m;
            ptr=newone;
            if (iptr==NULL) p.start=ptr=newone;
            else iptr->next=ptr;
        }
        R[m]=getbig(ptr->an);
        m--;
        iptr=ptr;
        ptr=ptr->next;
    } while (m>=0);

    deg=dega+degb;

    if (!squaring) B=(big *)mr_alloc(degb+1,sizeof(big));
    G=(big *)mr_alloc(deg+1,sizeof(big));
    char *memg=(char *)memalloc(deg+1);
    for (i=0;i<=deg;i++) G[i]=mirvar_mem(memg,i);

    if (!squaring)
    {
        ptr=b.p.start;
        while (ptr!=NULL)
        {
            B[ptr->n]=getbig(ptr->an);
            ptr=ptr->next;
        }
        mr_poly_mul(dega,R,degb,B,G);
    }
    else mr_poly_sqr(dega,R,G);

    if (!mr_poly_rem(deg,G,R))
    {  // reset the modulus - things have changed
        setmod(Modulus);
        mr_poly_rem(deg,G,R);
    }

// now delete any 0 terms

    ptr=p.start;
    iptr=NULL;
    while (ptr!=NULL)
    {
        if (ptr->an.iszero())
        {
            if (ptr==p.start)
                p.start=ptr->next;
            else
                iptr->next=ptr->next;
            newone=ptr;
            ptr=ptr->next;
            delete newone;
            continue;
        }
        iptr=ptr;
        ptr=ptr->next;
    }

    mr_free(R);
    memkill(memg,deg+1);
    mr_free(G);
    if (!squaring) mr_free(B);
            
    return *this;
}

PolyMod operator*(const PolyMod &a,const PolyMod& b)
{
    PolyMod prod=a;
    prod*=b;
    return prod;
}

void reduce(const Poly& p,PolyMod& rem)
{
    int i,d;
    ZZn t;
    big *G,*R;
    term *ptr,*pos=NULL;
    int n=degree(p);
    int degm=degree(Modulus);
    if (n-degm < FFT_BREAK_EVEN)
    {
        rem=(PolyMod)p;
        return;
    }
    G=(big *)mr_alloc(n+1,sizeof(big));
    char *memg=(char *)memalloc(n+1);
    for (i=0;i<=n;i++) G[i]=mirvar_mem(memg,i);
    R=(big *)mr_alloc(degm,sizeof(big));
    char *memr=(char *)memalloc(degm);
    for (i=0;i<degm;i++) R[i]=mirvar_mem(memr,i);

    ptr=p.start;
    while (ptr!=NULL)
    {
        copy(getbig(ptr->an),G[ptr->n]);
        ptr=ptr->next; 
    }
    if (!mr_poly_rem(n,G,R))
    {  // reset the Modulus - things have changed
        setmod(Modulus);
        mr_poly_rem(n,G,R);
    }
 
    rem.clear();

    for (d=degm-1;d>=0;d--)
    {
        t=R[d];
        if (t.iszero()) continue;
        pos=rem.addterm(t,d,pos);
    }
    memkill(memr,degm);
    mr_free(R);
    memkill(memg,n+1);
    mr_free(G);

}

void setmod(const Poly& p) 
{ 
    Modulus=p;
    setpolymod(p);
}

PolyMod operator-(const PolyMod& a,const PolyMod& b)
                                     {return (a.p-b.p)%Modulus;}
PolyMod operator+(const PolyMod& a,const PolyMod& b)
                                     {return (a.p+b.p)%Modulus;}
PolyMod operator*(const PolyMod& a,const ZZn& z)
                                     {return (z*a.p);}
PolyMod operator*(const ZZn& z,const PolyMod& m)
                                     {return (z*m.p);}

PolyMod operator+(const PolyMod& a,const ZZn& z)
{ 
    PolyMod p=a;
    p.addterm(z,0);
    return p;
}

PolyMod operator-(const PolyMod& a,const ZZn& z)
{ 
    PolyMod p=a;
    p.addterm((-z),0);
    return p;
}
    
PolyMod operator/(const PolyMod& a,const ZZn& z)
                                     {return (a.p/z);}

Poly gcd(const PolyMod& m)
                                     {return gcd(m.p,Modulus);}  


//
// Brent & Kung's First Algorithm
// See "Fast Algorithms for Manipulating Formal Power Series 
// J.ACM, Vol. 25 No. 4 October 1978 pp 581-595
//

PolyMod compose(const PolyMod& q,const PolyMod& p)
{ // compose polynomials
  // assume P(x) = P3x^3 + P2x^2 + P1x^1 +P0
  // Calculate P(Q(x)) = P3.(Q(x))^3 + P2.(Q(x))^2 ....   
    PolyMod C,Q,T; 
    big t; 
    term *xptr,*yptr;
    int i,j,ik,L,n=degree(Modulus);
    int k=isqrt(n+1,1);
    if (k*k<n+1) k++;

// step 1

    PolyMod *P=new PolyMod[k+1];
    P[0]=1;
    for (i=1;i<=k;i++) P[i]=(P[i-1]*p);

    big *x,*y;
    x=(big *)mr_alloc(k,sizeof(big));
    y=(big *)mr_alloc(k,sizeof(big));
    t=mirvar(0);

    T=1;
    for (i=0;i<k;i++)
    {
        ik=i*k;
        Q.clear();
        for (L=0;L<=n;L++)
        {
            zero(t);
            xptr=q.p.start;
            while (xptr!=NULL) 
            {
                if (xptr->n<=ik+k-1) break;
                xptr=xptr->next;
            }
            for (j=k-1;j>=0;j--)
            {
                x[j]=t;
                if (xptr!=NULL)
                {
                    if (ik+j==xptr->n) 
                    {
                        x[j]=getbig(xptr->an);
                        xptr=xptr->next;
                    }
                }                
                              // x[j]=q.coeff(i*k+j)
                y[j]=t;
                yptr=P[j].p.start;
                while (yptr!=NULL)
                {
                    if (yptr->n<=L)
                    {
                        if (yptr->n==L) y[j]=getbig(yptr->an);
                        break;
                    }
                    yptr=yptr->next;
                }
            }                // y[j]=P[j].coeff(L)

// Asymptotically slow, but very fast in practise ...

            nres_dotprod(k,x,y,t);
            Q.addterm((ZZn)t,L);
        }
        C+=(Q*T);  
        if (i<k-1) T*=P[k]; 
    }
    mr_free(t);
    mr_free(y);
    mr_free(x);

    delete [] P;
    return C;
}

PolyMod pow(const PolyMod& f,const Big& k)
{
    PolyMod u,u2,table[16];
    Big w,e;
    int i,j,nb,n,nbw,nzs;    
    if (k==0) 
    {
        u.addterm((ZZn)1,0);
        return u;
    }

    reduce(f.p,u);
    if (k==1) return u;

    if (degree(f)>=FFT_BREAK_EVEN )
    {
        u2=(u*u);

        table[0]=u;
        for (i=1;i<16;i++)
            table[i]=u2*table[i-1];
        nb=bits(k);
        if (nb>1) for (i=nb-2;i>=0;)
        {
            n=window(k,i,&nbw,&nzs,5);
            for (j=0;j<nbw;j++)
            u*=u;

            if (n>0) u*=table[n/2];
            i-=nbw;
            if (nzs)
            {
                for (j=0;j<nzs;j++) u*=u;
                i-=nzs;
            }        
        }
    }
    else
    {
        e=k;
        w=pow((Big)2,bits(e)-1);
        e-=w; w/=2;
        while (w>0)
        {
            u*=u;
            if (e>=w)
            {
               e-=w;
               u*=f;
            }
            w/=2; 
        }
    }
    return u;
}   

ostream& operator<<(ostream& s,const PolyMod& m) 
                                     { s << m.p; return s;} 

