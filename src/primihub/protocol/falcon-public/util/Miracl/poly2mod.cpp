/*
 * C++ class to implement a polynomial type and to allow 
 * arithmetic on polynomials whose elements are from
 * the finite field GF(2^m). 
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

#include "poly2mod.h"

extern "C"
{
extern miracl *mr_mip;
}

Poly2 Modulus;

big *GF=NULL;
big *GRF,*T,*W,*Q;

int N,INC;
  
BOOL iszero(const Poly2Mod& m)     {return iszero(m.p);}
BOOL isone(const Poly2Mod& m)      {return isone(m.p);}
int degree(const Poly2Mod& m)  {return degree(m.p);}
          
GF2m Poly2Mod::coeff(int i) const {return p.coeff(i);}

Poly2Mod& Poly2Mod::operator*=(const Poly2Mod &b)
{
    reduce(p*b.p,*this);
    return *this;
}

Poly2Mod operator*(const Poly2Mod &a,const Poly2Mod& b)
{
    Poly2Mod prod=a;
    if (&a!=&b) prod*=b;
    else        prod*=prod;
    return prod;
}

void reduce(const Poly2& p,Poly2Mod& rem)
{
    int m,d;
    GF2m t;
    big *G;
    term2 *ptr,*pos=NULL;
    int n=degree(p);
    int degm=degree(Modulus);

    if (n-degm < KARAT_BREAK_EVEN)
    {
        rem=(Poly2Mod)p;
        return;
    }

    G=(big *)mr_alloc(2*(N+2),sizeof(big));
    
    ptr=p.start;
    while (ptr!=NULL)
    {
        G[ptr->n]=getbig(ptr->an);
        ptr=ptr->next;
    }

    karmul2_poly(N,T,GRF,&G[N],W);   // W=(G/x^n) * h

    for (d=N-1;d<2*N;d++) copy(W[d],Q[d-N+1]);
    m=N+1; if(m%2==1) m=N+2;   // make sure m is even - pad if necessary

    for (d=m;d<2*m;d++) copy(G[d],W[d]);
   
    karmul2_poly_upper(m,T,GF,Q,W);

    pos=NULL;
    rem.clear();
    for (d=N-1;d>=0;d--)
    {
        add2(W[d],G[d],W[d]);
        t=W[d];
        if (t.iszero()) continue;
        pos=rem.addterm(t,d,pos);
    }
    
    mr_free(G);
}

void setmod(const Poly2& p) 
{ 
    int i,n,m;
    Poly2 h;
    term2 *ptr;
    Modulus=p;

    n=degree(p);
    if (n<KARAT_BREAK_EVEN) return;
    h=reverse(p);
    h=invmodxn(h,n);
    h=reverse(h);   // h=RECIP(f)
    m=degree(h);
    if (m<n-1) h=mulxn(h,n-1-m);


    if (GF!=NULL)
    { // kill last Modulus
        for (i=0;i<N+2;i++)
        { 
            mr_free(GF[i]);
            mr_free(GRF[i]);
            mr_free(Q[i]); 
        }
        for (i=0;i<2*(N+INC);i++)
        {
            mr_free(W[i]);
            mr_free(T[i]);
        }
        mr_free(GF);
        mr_free(GRF);
        mr_free(Q);
        mr_free(W);
        mr_free(T);;
    }

    N=n;
    m=N; INC=0;
    while (m!=0) { m/=2; INC++; }

    GF=(big *)mr_alloc(N+2,sizeof(big));
    GRF=(big *)mr_alloc(N+2,sizeof(big));
    Q=(big *)mr_alloc(N+2,sizeof(big));

    W=(big *)mr_alloc(2*(N+INC),sizeof(big));
    T=(big *)mr_alloc(2*(N+INC),sizeof(big));

    for (i=0;i<N+2;i++)
    {
        GF[i]=mirvar(0);
        GRF[i]=mirvar(0);
        Q[i]=mirvar(0);
    }
    for (i=0;i<2*(N+INC);i++) 
    {
        W[i]=mirvar(0);
        T[i]=mirvar(0);
    }

    ptr=p.start;
    while (ptr!=NULL)
    {
        copy(getbig(ptr->an),GF[ptr->n]);
        ptr=ptr->next;
    }
    ptr=h.start;
    while (ptr!=NULL)
    {
        copy(getbig(ptr->an),GRF[ptr->n]);
        ptr=ptr->next;
    }
}

Poly2Mod operator+(const Poly2Mod& a,const Poly2Mod& b)
                                     {return (a.p+b.p)%Modulus;}
Poly2Mod operator*(const Poly2Mod& a,const GF2m& z)
                                     {return (z*a.p);}
Poly2Mod operator*(const GF2m& z,const Poly2Mod& m)
                                     {return (z*m.p);}

    
Poly2Mod operator+(const Poly2Mod& a,const GF2m& z)
{
    Poly2Mod p=a;
    p.addterm(z,0);
    return p;
}

Poly2Mod operator/(const Poly2Mod& a,const GF2m& z)
                                     {return (a.p/z);}

Poly2 gcd(const Poly2Mod& m)
{return gcd(m.p,Modulus);}  

Poly2Mod inverse(const Poly2Mod& m)
                                     
{return (Poly2Mod)inverse(m.p,Modulus);}

//
// Brent & Kung's First Algorithm
// See "Fast Algorithms for Manipulating Formal Power Series 
// J.ACM, Vol. 25 No. 4 October 1978 pp 581-595
//

Poly2Mod compose(const Poly2Mod& q,const Poly2Mod& p)
{ // compose polynomials
  // assume P(x) = P3x^3 + P2x^2 + P1x^1 +P0
  // Calculate P(Q(x)) = P3.(Q(x))^3 + P2.(Q(x))^2 ....   
    Poly2Mod C,Q,T; 
    big t; 
    term2 *xptr,*yptr;
    int i,j,ik,L,n=degree(Modulus);
    int k=isqrt(n+1,1);
    if (k*k<n+1) k++;

// step 1

    Poly2Mod *P=new Poly2Mod[k+1];
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

// Asymptotically slow, but fast in practise ...
            gf2m_dotprod(k,x,y,t);
            Q.addterm((GF2m)t,L);
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

ostream& operator<<(ostream& s,const Poly2Mod& m) 
                                     { s << m.p; return s;} 

