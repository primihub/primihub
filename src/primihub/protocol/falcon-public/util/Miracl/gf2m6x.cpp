/*
 *    MIRACL C++ Implementation file GF2m6x.cpp
 *
 *    AUTHOR   : M. Scott
 *
 *    PURPOSE  :    Implementation of class GF2m6x (6-th extension over 2^m)
 *                  uses irreducible polynomial x^6+x^FA+x^FB+x^FC+1
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 */

#include "gf2m6x.h"

using namespace std;

void GF2m6x::get(GF2m* a)
{for (int i=0;i<FM;i++) a[i]=x[i];}

void GF2m6x::get(GF2m& a)
{a=x[0];}

int GF2m6x::degree()
{
    for (int i=FM-1;i>=1;i--) if (!x[i].iszero()) return i;
    return 0;
}

GF2m6x& GF2m6x::powq()
{
    GF2m t[2*FM],m;
    int r=(get_mip()->M)%FM;
    int i,j;
    if (r==0) return *this;

    for (j=0;j<r;j++)
    {
        for (i=0;i<FM;i++) t[2*i]=x[i];
   
        for (i=2*FM-2;i>=FM;i--)
        { // reduce mod x^FM+x^FA+x^FB+x^FC+1
            m=t[i]; if (m.iszero()) continue;
            t[i]=0;
            t[i-FM]+=m;
            t[i-(FM-FA)]+=m; 
#ifdef FB
            t[i-(FM-FB)]+=m; 
            t[i-(FM-FC)]+=m; 
#endif
        }
        for (i=0;i<FM;i++)
        {
            x[i]=t[i];
            t[i]=0;
        }
    }
    return *this;
}

GF2m6x mul(const GF2m6x& a,const GF2m6x& b)
{ // special purpose mul
    int i;
    GF2m6x r;
    GF2m t[2*FM],m;

    kar3x3(a.x,b.x,t);

    t[4]+=a.x[4]*b.x[0]+b.x[4]*a.x[0];
    t[5]= a.x[4]*b.x[1]+b.x[4]*a.x[1];
    t[6]= a.x[4]*b.x[2]+b.x[4]*a.x[2];

    t[8]=a.x[4]*b.x[4];

    for (i=8;i>=FM;i--)
    {
        m=t[i]; if (m.iszero()) continue; 
        t[i]=0;
        t[i-FM]+=m;
        t[i-(FM-FA)]+=m; 
#ifdef FB
        t[i-(FM-FB)]+=m; 
        t[i-(FM-FC)]+=m; 
#endif
    }
    for (i=0;i<FM;i++) r.x[i]=t[i];
    return r;
}

GF2m6x& GF2m6x::operator*=(const GF2m6x& b)
{
    GF2m t[2*FM],m;
    int i;

    if (this==&b)
    { // squaring
        for (i=0;i<FM;i++)
            t[2*i]=x[i]*x[i];

        for (i=2*FM-2;i>=FM;i--)
        { // reduce mod x^FM+x^FA+x^FB+x^FC+1
            m=t[i]; if (m.iszero()) continue;
            t[i]=0;
            t[i-FM]+=m;
            t[i-(FM-FA)]+=m; 
#ifdef FB
            t[i-(FM-FB)]+=m; 
            t[i-(FM-FC)]+=m; 
#endif

        }
        for (i=0;i<FM;i++) x[i]=t[i];

        return *this;
    }
    else
    { // Use Karatsuba
        GF2m w1[3],w2[3],w3[5];
 
        kar3x3(x,b.x,t);
        kar3x3(&x[3],&b.x[3],&t[6]);

        w1[0]=x[0]+x[3];     w1[1]=x[1]+x[4];     w1[2]=x[2]+x[5];
        w2[0]=b.x[0]+b.x[3]; w2[1]=b.x[1]+b.x[4]; w2[2]=b.x[2]+b.x[5];

        kar3x3(w1,w2,w3);

        w3[0]+=t[0]+t[6];
        w3[1]+=t[1]+t[7];
        w3[2]+=t[2]+t[8];
        w3[3]+=t[3]+t[9];
        w3[4]+=t[4]+t[10];

        t[3]+=w3[0];
        t[4]+=w3[1];
        t[5]+=w3[2];
        t[6]+=w3[3];
        t[7]+=w3[4];

        for (i=2*FM-2;i>=FM;i--)
        {
            m=t[i]; t[i]=0;
            t[i-FM]+=m;
            t[i-(FM-FA)]+=m; 
#ifdef FB
            t[i-(FM-FB)]+=m; 
            t[i-(FM-FC)]+=m; 
#endif
        }

        for (i=0;i<FM;i++) x[i]=t[i];


/*        int i; 
        big A[FM],B[FM],C[2*FM],T[2*FM];
        GF2m6x bb=b;
    
        char *memc=(char *)memalloc(4*FM);

        for (i=0;i<2*FM;i++)
        {
            C[i]=mirvar_mem(memc,i);
            T[i]=mirvar_mem(memc,i+2*FM);
        }

        for (i=0;i<FM;i++)
        {
            A[i]=getbig(x[i]);
            B[i]=getbig(bb.x[i]);
        }

        karmul2_poly(FM,T,A,B,C);

        for (i=0;i<2*FM;i++) t[i]=C[i];

        for (i=2*FM-2;i>=FM;i--)
        {
            m=t[i]; t[i]=0;
            t[i-FM]+=m;
            t[i-(FM-FA)]+=m; 
#ifdef FB
            t[i-(FM-FB)]+=m; 
            t[i-(FM-FC)]+=m; 
#endif
        }

        for (i=0;i<FM;i++) x[i]=t[i];

        memkill(memc,4*FM);
*/
        return *this;
    }
}

GF2m6x& GF2m6x::operator*=(const GF2m& b)
{ 
   for (int i=0;i<FM;i++) x[i]*=b;
   return *this;
}

GF2m6x& GF2m6x::operator/=(const GF2m& b)
{
   GF2m ib=(GF2m)1/b;
   for (int i=0;i<FM;i++) x[i]*=ib;
   return *this;
}

//
// Lim & Hwang - just one field inversion
//

void GF2m6x::invert()
{
    int degF,degG,degB,degC,degM,d,i,j;
    GF2m alpha,beta,gamma,BB[FM+1],FF[FM+1],CC[FM+1],GG[FM+1];
    GF2m *B=BB,*C=CC,*F=FF,*G=GG,*T; 

    C[0]=1;
    F[FM]=F[FA]=F[0]=1;  // f(x)

#ifdef FB
    F[FB]=F[FC]=1; 
#endif
    degF=FM; degG=degree(); degC=0; degB=-1;

    if (degG==0)
    {
        x[0]=(GF2m)1/x[0];
        return;
    }

    for (i=0;i<FM;i++)
    {
        G[i]=x[i];
        x[i]=0;
    }
    while (degF!=0)
    {
        if (degF<degG)
        { // swap
            T=F; F=G; G=T;  d=degF; degF=degG; degG=d;
            T=B; B=C; C=T;  d=degB; degB=degC; degC=d;
        }
        j=degF-degG;
        alpha=G[degG]*G[degG]; 
        beta=F[degF]*G[degG];
        gamma=G[degG]*F[degF-1] + F[degF]*G[degG-1];

        for (i=0;i<=degF;i++ )
        {
            F[i]*=alpha;
            if (i>=j-1) F[i]+=gamma*G[i-j+1];
            if (i>=j)   F[i]+=beta*G[i-j];           
        }
        for (i=0;i<=degB || i<=degC+j;i++)
        {
            B[i]*=alpha;
            if (i>=j-1) B[i]+=gamma*C[i-j+1];
            if (i>=j)   B[i]+=beta*C[i-j];
        }
        while (degF>=0 && F[degF]==0) degF--;

        if (degF==degG)
        {
            alpha=F[degF];
            for (i=0;i<=degF;i++)
            {
                F[i]*=G[degF];
                F[i]+=alpha*G[i];
            }
           
            for (i=0;i<=FM-degF;i++)
            {
                B[i]*=G[degF];
                B[i]+=alpha*C[i];
            }
            while (degF>=0 && F[degF]==0) degF--;
        }
        degB=FM-1; while (degB>=0 && B[degB]==0) degB--;
    }
    alpha=(GF2m)1/F[0];
    for (i=0;i<=degB;i++) x[i]=alpha*B[i]; 
    return;
}

GF2m6x& GF2m6x::operator/=(const GF2m6x& a)
{
    GF2m6x b=a;
    b.invert();
    *this *= b;
    return *this;
}

GF2m6x operator+(const GF2m6x& a,const GF2m6x& b)
{ GF2m6x r=a; r+=b; return r;}

GF2m6x operator+(const GF2m6x& a,const GF2m& b)
{ GF2m6x r=a; r+=b; return r;}
 
GF2m6x operator+(const GF2m& a,const GF2m6x& b)
{ GF2m6x r=b; r+=a; return r;}

GF2m6x operator*(const GF2m6x& a,const GF2m6x& b)
{ 
    GF2m6x r=a;
    if (&a!=&b) r*=b; 
    else r*=r;
    return r;
}

GF2m6x operator/(const GF2m6x& a,const GF2m6x& b)
{ 
    GF2m6x r=a;
    r/=b; 
    return r;
}

GF2m6x operator*(const GF2m6x& a,const GF2m& b)
{ GF2m6x r=a; r*=b; return r;}
GF2m6x operator*(const GF2m& a,const GF2m6x& b)
{ GF2m6x r=b; r*=a; return r;}

GF2m6x randx6(void)
{
    int m=get_mip()->M;
    GF2m6x r;
    for (int i=0;i<FM;i++) r.x[i]=rand(m,2);                     
    return r;
}

GF2m6x pow(const GF2m6x& a,const Big& k)
{
    int i,j,nb,n,nbw,nzs;
    GF2m6x u,u2,t[16];
    if (k.iszero()) return (GF2m6x)1;
    u=a;
    if (k.isone()) return u;

//
// Prepare table for windowing
//
    u2=(u*u);
    t[0]=u;

    for (i=1;i<16;i++)
        t[i]=u2*t[i-1];

// Left to right method - with windows

    nb=bits(k);
    if (nb>1) for (i=nb-2;i>=0;)
    {
        n=window(k,i,&nbw,&nzs,5);
        for (j=0;j<nbw;j++) u*=u;
        if (n>0) u*=t[n/2];
        i-=nbw;
        if (nzs)
        {
            for (j=0;j<nzs;j++) u*=u;
            i-=nzs;
        }
    }
    return u;
}

ostream& operator<<(ostream& os,const GF2m6x& x)
{
    GF2m6x u=x;
    GF2m a[FM];
 
    u.get(a);

    os << "[";
    for (int i=0;i<FM-1;i++) os << (Big)a[i] << ",";
    os << (Big)a[FM-1] << "]";

    return os;
}
