/*
 * cm.cpp - Finding an elliptic curve and point of nearly prime order
 * See IEEE 1363 Annex A for documentation!
 *
 * !!! New much improved version - March 2002
 * !!! Now tested for D up to 10^7
 *
 * !!! New faster version - uses Floats instead of Flashs - November 2003
 * !!! Now 100 times faster!
 * !!! Now tested for D up to and beyond 10^9 
 *
 * !!! New invariants - gamma2, w3, w5, w7 and w13. Thanks to Marcel Martin.
 *
 * Sometimes its better to use the -IEEE flag, as this can often be faster 
 * than the Gamma2 invariant, as although it requires a bigger "class number", 
 * it needs less precision.
 *
 * Uses functions from the MIRACL multiprecision library, specifically
 * classes:-
 * Float   - Big floating point 
 * Complex - Big Complex float
 * Big     - Big integer
 * ZZn     - Big integers mod an integer
 * FPoly   - Big Float polynomial
 * Poly    - Big ZZn polynomial
 *
 * Written by Mike Scott, Dublin, Ireland. March 1998 - March 2004
 *
 */

#include <iostream>
#include <fstream>
#include <cmath>
#include <cstring>
#include "ecn.h"
#include "complex.h"
#include "flpoly.h"
#include "poly.h"

using namespace std;

miracl *mip;

FPoly T[25];  // Reduced class Polynomial. 
static char *s;
BOOL fout,suppress,three;

// F(z) Function A.13.3

Complex Fz(Complex z)
{
    Complex t;
    int sign=1;
    Complex osum,sum=(Float)1;

    if (z.iszero()) return sum;

    Complex zi=z;
    Complex zj=z*z;
    Complex r=z;
    Complex z3=zj*z;

    forever
    { // do 2 terms at a time....
        t=zi+zj;
        osum=sum;
        if (sign) sum-=t;
        else      sum+=t;

        if (sum==osum) break;

        r*=z3;        
        zi*=r; zj*=r; zj*=z;

        sign=1-sign;
    }

    return sum;
}

// Fj(A,B,C) function A.13.3

Complex F(int j,Big A,Big B,Big C,Big D,int N)
{
    Complex t,theta24,theta,theta2,theta3,theta6;
    Float sd;

    if (j>=3)
    { // Gamma 2 and Morain's invariants
        sd=-sqrt((Float)D);
        t=Complex(sd*fpi(),(fpi()*(Float)B));    
        t/=((Float)A);
        theta=exp(t);
        if (j==3)
        {
            theta3=theta;
            theta3*=theta3;
            theta3*=theta;
            theta6=theta3;
            theta6*=theta6;
            t=theta*pow((Fz(theta6)/Fz(theta3)),8);
            return (256*t*t*t+1)/t;
        }
        if (j==4)
        {
              return (pow(Fz(theta)/Fz(pow(theta,N)),24/(N-1))/theta);   
        }
        return 0;
    }

    sd=-sqrt((Float)D);
    t=Complex(sd*fpi(),(fpi()*(Float)B));    

    t/=(Float)(24*A);
    theta24=exp(t);                                // theta^(1/24)
    theta=pow(theta24,24);                         // theta

    if (j!=2) 
        t=recip(theta24);      // -24th root
    else  
        t=theta24*theta24;     // 12th root

    theta2=theta;
    theta2*=theta2;

    if (j==0) return (t*Fz(-theta)/Fz(theta2));
    if (j==1) return (t*Fz(theta)/Fz(theta2));
    if (j==2) return (sqrt((Float)2)*t*Fz(theta2*theta2)/Fz(theta2));
    return 0;
}

int geti(Big D)
{
    Big d=D%8;
    if (d==1 || d==2 || d==6 || d==7) return 3;
    if (d==3)
    {
       if (D%3==0) return 2;
       else        return 0;
    } 
    if (d==5) return 6;
    return 0;
}

int getk(Big D)
{
    Big d=D%8;
    if (d==1 || d==2 || d==6) return 2;
    if (d==3 || d==7) return 1;
    if (d==5) return 4;
    return 0; 
}

int getN(Big D)
{
    if (D%13==0) return 13;
    if (D%7==0)  return 7;
    if (D%5==0)  return 5;
    if (D%3==0)  return 3;
    return 2;
}

// A.13.3

void class_poly(Complex& lam,Float *Fi2,Big A,Big B,Big C,Big D,BOOL conj,BOOL P1363)
{
    Big ac,l,t;
    int i,j,k,g,e,m,n,dm8;
    Complex cinv;

    if (P1363)
    {
        g=1;
        if (D%3==0) g=3;

        ac=A*C;
        if (ac%2==1) 
        {
            j=0;
            l=A-C+A*A*C;
        }
        if (C%2==0)  j=1;
        if (A%2==0)  j=2;

        if (A%2==0)
        {
            t=(C*C-1)/8;
            if (t%2==0) m=1;
            else        m=-1;
        }
        else
        {
            t=(A*A-1)/8;
            if (t%2==0) m=1;
            else        m=-1;
        }
    
        dm8=D%8;
        i=geti(D);
        k=getk(D);
        switch (dm8)
        {
        case 1: 
        case 2:    n=m;
                   if (C%2==0) l=A+2*C-A*C*C;
                   if (A%2==0) l=A-C-A*C*C;
                   break;
        case 3:    if (ac%2==1) n=1;
                   else         n=-m;
                   if (C%2==0) l=A+2*C-A*C*C;
                   if (A%2==0) l=A-C+5*A*C*C;
                   break;
        case 5:    n=1;
                   if (C%2==0) l=A-C+A*A*C;
                   if (A%2==0) l=A-C-A*C*C;
                   break;
        case 6:    n=m;
                   if (C%2==0) l=A+2*C-A*C*C;
                   if (A%2==0) l=A-C-A*C*C;
                   break;
        case 7:    if (ac%2==0) n=1;
                   else         n=m;
                   if (C%2==0) l=A+2*C-A*C*C;
                   if (A%2==0) l=A-C-A*C*C;
                   break;
               
        default: break;
        }
        e=(k*B*l)%48;
        if (e<0) e+=48;
        cinv=pow(lam,e);
        cinv*=(n*Fi2[i]);
        cinv=pow(cinv*pow(F(j,A,B,C,D,0),k),g);
    }
    else
    {
        int N=getN(D);   
// adjust A and B

        if (N==2)
        {
            j=3;
            if (A%3!=0)
            {
                if (B%3!=0)
                {
                    if ((B+A+A)%3!=0) B+=(4*A);
                    else              B+=(A+A);
                }
            }
            else
            {
                if (B%3!=0)
                {
                    if (C%3!=0)
                    {
                        if ((C+B)%3!=0) B+=(4*A);
                        else            B+=(A+A);
                    }
                }
            }
            A*=3;
        }
        else
        {
            j=4;
            if ((A%N)==0) 
            {
                A=C;
                B=-B;
            }
            while (B%N!=0) B+=(A+A);
            A*=N;
        }
        cinv=F(j,A,B,C,D,N);
    }


 // multiply polynomial by new term(s)

    FPoly F;
    if (conj)
    { // conjugate pair
      // t^2-2a+(a^2+b^2) , where cinv=a+ib
        F.addterm((Float)1,2);
        F.addterm(-2*real(cinv),1);
        F.addterm(real(cinv)*real(cinv)+imaginary(cinv)*imaginary(cinv),0);
    }
    else 
    { // t-cinv
        F.addterm((Float)1,1);
        F.addterm(-real(cinv),0);

// store as a linear polynomial, or combine 2 to make a quadratic
        if (T[0].iszero())
        {
            T[0]=F;
            return;
        }
        else 
        {
            F=T[0]*F;      // got a quadratic
            T[0].clear();
        }
    }

// accumulate Polynomial as 2^m degree components
// This allows the use of karatsuba via the "special" function
// This is the time critical bit....

    for (i=1;;i++)
    {
        if (T[i].iszero())
        {
            T[i]=F;             // store this 2^i degree polynomial
            break;
        }
        else
        {
            F=special(T[i],F);  // e.g. if i=1 two quadratics make a quartic..
            T[i].clear();
        }
    }
}

// A.13.2
// Set P1363 to False to calculate class number a la Cohen

int groups(Complex& lam,Float *Fi2,unsigned long D,BOOL doit,BOOL P1363)
{
    unsigned long s,t,A,C,B,TB,lim;
    int cn=0;
    s=mr_lsqrt(D/3,1);
    for (B=0;B<=s;B+=1)
    {
//  cout << "B= " << B << " s= " << s << endl;
        t=D+B*B;
        if (!P1363)
        {
            if (t%4!=0) continue;
            else t/=4;
        }
// cout << "t= " << t << endl;
        lim=mr_lsqrt(t,1);
// cout << "lim= " << lim << endl;
        if (P1363) A=2*B;
        else       A=B;
        if (A==0) A+=1;
        for(;;)
        {
            while (t%A!=0) 
            {
                A+=1;
                if (A>lim) break;
            }

            if (A>lim) break;
            C=t/A;
           
            TB=B;
            if (P1363) TB*=2;
            if (lgcd(lgcd(A,TB),C)==1)
            { // output more class group members
                BOOL conj;
                if (TB>0 && C>A && A>TB) 
                { 
                    conj=TRUE;
                    cn+=2;
                    if (doit)
                    { 
                        if (!suppress) cout << ".." << flush; 
                    }  
                }
                else
                {
                    conj=FALSE;
                    cn+=1;
                    if (doit)
                    {   
                        if (!suppress) cout << "." << flush; 
                    }
                } 
                if (doit) class_poly(lam,Fi2,(Big)A,(Big)B,(Big)C,(Big)D,conj,P1363);
            }
            A+=1;
        }        
    }
    return cn;         // class number
}

// check congruence conditions A14.2.1

BOOL isaD(unsigned long d,int pm8,Big k)
{
    unsigned int dm8;
    unsigned long i;
    BOOL sqr;
    dm8=d%8;
    if (k==1 && dm8!=3) return FALSE;
    if ((k==2 || k==3) && dm8==7) return FALSE;
    if (pm8==3 && (dm8==1 || dm8==4 || dm8==5 || dm8==6)) return FALSE;
    if (pm8==5 && dm8%2==0) return FALSE;
    if (pm8==7 && (dm8==1 || dm8==2 || dm8==4 || dm8==5)) return FALSE;
    sqr=FALSE;
    for (i=2;;i++)
    {
        if (d%(i*i)==0)
        {
            sqr=TRUE;
            break;
        }
        if (i*i>d) break;
    }
    if (sqr) return FALSE;
    return TRUE;
}

// Testing for CM discriminants A.14.2.2

Big floor(Big N,Big D)
{
   Big R;
   if (N==0) return 0;
   if (N>0 && D>0) return N/D;
   if (N<0 && D<0) return (-N)/(-D);
   R=N/D;
   if (N%D!=0) R-=1;
   return R;
}

BOOL isacm(Big p,unsigned long D,Big &W,Big &V)
{
    Big B2,A,B,C,t,X,Y,ld,delta;
    B=sqrt(p-(Big)D,p);
    A=p;
    C=(B*B+(Big)D)/p;
    X=1;
    Y=0;
    ld=0;

    while (1)
    {
        if (C>=A)
        {
            B2=2*B;
            if (B2<0) B2=-B2;
            if (A>=B2) break;
        } 
        delta=floor(2*B+C,2*C);

// are we stuck in hopeless loop? This can't happen now - thanks Marcel

        if (delta==0 && ld==0) return FALSE;
        ld=delta;
        t=X;
        X=(delta*X+Y);
        Y=-t;

        t=C;

        C=(A+C*delta*delta-2*B*delta);
        B=(t*delta-B);
        A=t;        
    }
    if (D==11 && A==3)
    {
        t=X;
        X=Y;
        Y=-t;
        B=-B;
        t=C;
        C=A;
        A=t;        
    }      
    if (D==1 || D==3) 
    {
       W=2*X;
       V=2*Y;
       return TRUE;
    }
    else V=0;

    if (A==1) 
    {
        W=2*X;
        return TRUE;
    }

    if (A==4) 
    {
        W=4*X+B*Y;
        return TRUE;
    }

    return FALSE;
}


// Constructing a Curve (and Point if order is prime)
// A.14.4

BOOL get_curve(Complex& lam,Float *Fi2,ofstream& ofile,Big r,Big other_r,Big ord,unsigned long D,Big p,Big W,int m,BOOL always)
{
    Poly polly;
    Big A0,B0,k;
    BOOL unstable;
    char c;
    int i,j,terms,class_number;
    BOOL P1363,pord=prime(ord);

    P1363=TRUE;
    if (!always && D>3 && D%8==3) P1363=FALSE;

    k=r;
    while (k%ord==0)  k/=ord;

    if (!suppress) 
    {
        if (D>1000 && D%3==0) cout  << "D= " << D << " (Divisible by 3 - might need extra precision)" << endl;
        else                  cout  << "D= " << D << endl;
        if (P1363) cout << "IEEE-1363 invariant" << endl;
        else
        {
            switch (getN(D))
            {
            case 2:  cout << "Gamma2 invariant" << endl;
                     break;
            case 3:  cout << "w3 invariant" << endl;
                     break;
            case 5:  cout << "w5 invariant" << endl;
                     break;
            case 7:  cout << "w7 invariant" << endl;
                     break;
            case 13:  cout << "w13 invariant" << endl;
                     break;
            }
        }
        cout << "D mod 24 = " << D%24 << endl;
        if (prime(ord)) cout << "K= " << k << endl;
    }

    class_number=groups(lam,Fi2,D,FALSE,P1363);
    
    cout << "class number= " << class_number << endl;

    cout << "Group order= " << ord << endl;
    cout << "do you want to proceed with this one (Y/N)? " ;
    cin >> c;
    if (c=='N' || c=='n') 
    {
        if (!suppress) cout << "finding a curve..." << endl;
        return FALSE;
    }

    modulo(p);
                                   
// A.14.4.1
    
    if (D==1)
    {
        A0=1; B0=0;
    }
    if (D==3)
    {
        A0=0; B0=1;
    }

    if (D!=1 && D!=3)
    {
        FPoly Acc;

        for (i=0;i<25;i++) T[i].clear();
        if (!suppress) cout << "constructing polynomial";
        groups(lam,Fi2,D,TRUE,P1363);

// Construct Polynomial from its 2^m degree components..

        for (i=24;i>=0;i--)
        {
            if (!T[i].iszero())
            {
                Acc=T[i];              // find the first component..
                T[i].clear();
                break;
            }  
        }
        if (i>0)
        {
            for (j=i-1;j>0;j--)
            {
                if (!T[j].iszero())
                {
                    Acc=special(Acc,T[j]); // special karatsuba function
                    T[j].clear();          // multiply into accumulator
                }   
            }
            if (!T[0].iszero())
            {                              // check for a left-over linear poly
                Acc=Acc*T[0];
                T[0].clear();
            }
        }
        for (i=0;i<25;i++) T[i].clear();

        terms=degree(Acc);
        Float f,rem;
        Big whole;
        int nbits,maxbits=0;

        unstable=FALSE;
        for (i=terms;i>=0;i--)
        {
            f=Acc.coeff(i);
            if (f>0)
                 f+=makefloat(1,10000);
            else f-=makefloat(1,10000);
            whole=f.trunc(&rem);
            nbits=bits(whole);
            if (nbits>maxbits) maxbits=nbits;
            polly.addterm((ZZn)whole,i);
            if (fabs(rem)>makefloat(1,100)) 
            {
                unstable=TRUE; 
                break; 
            }
        }
        Acc.clear();
        if (!suppress) cout << endl;
        if (unstable) 
        {
            if (!suppress) cout << "Curve abandoned - numerical instability!" << endl;
            if (!suppress) cout << "Curve abandoned - double MIRACL precision and try again!" << endl;
            if (!suppress) cout << "finding a curve..." << endl;
            return FALSE;
        }
        if (!suppress) 
        {
            cout << polly << endl;
            cout << "Maximum precision required in bits= " << maxbits << endl;
        }
    }

// save space with smaller miracl

    mirexit();
    mip=mirsys(128,0);
    modulo(p);

    ECn pt,G;
    Big a,b,x,y;
    Big w,eps;
    int at;
    Poly g,spolly=polly;     // smaller polly
    polly.clear();
    forever
    {
        if (D!=1 && D!=3)
        {
            if (!suppress) cout << "Factoring polynomial of degree " << degree(spolly) << " ....." << endl;

            if (P1363)
            {
                if (W%2==0)
                {
                    ZZn V;
                    g=factor(spolly,1);
                    V=-g.coeff(0);
                    V=pow(V,24/(lgcd(D,3)*getk(D)));
                    V*=pow((ZZn)2,(4*geti(D))/getk(D));
                    if (D%2!=0) V=-V;
                    A0=(Big)((ZZn)(-3)*(V+64)*(V+16));   
                    B0=(Big)((ZZn)2*(V+64)*(V+64)*(V-8));
                }
                else
                {
                    Poly V,w,w1,w2,w3,a,b;

                    g=factor(spolly,3);
                    if (D%3!=0)
                        w.addterm(-1,24);
                    else
                        w.addterm(-256,8);
                    V=w%g;
                    w.clear();
                    w1=V; w2=V; w3=V;
                    w1.addterm(64,0);
                    w2.addterm(256,0);
                    w3.addterm(-512,0);
                    a=w1*w2;
                    a.multerm(-3,0);
                    a=a%g;
                    b=w1*w1*w3;
                    b.multerm(2,0);
                    b=b%g;

                    a=((a*a)*a)%g;
                    b=(b*b)%g;
                    for (int d=degree(g)-1;d>=0;d--)
                    {
                        ZZn t,c=a.coeff(d);
                        if (c!=(ZZn)0)
                        {
                            t=b.coeff(d);
                            A0=(Big)(c*t);
                            B0=(Big)(c*t*t);
                            if (d==1) break;
                        }     
                    }
                }
            }
            else
            {
                ZZn X,J,X2,K;

                g=factor(spolly,1);
                X=-g.coeff(0);
                X2=X*X;
                switch (getN(D))
                {
                case 2:  J=X2*X; 
                         break;
                case 3:  J=(pow((X+3),3)*(X+27))/X;
                         break;
                case 5:  J=pow((X2+10*X+5),3)/X;
                         break;
                case 7:  J=(pow((X2+5*X+1),3)*(X2+13*X+49))/X;
                         break;
                case 13: J=(pow((X2*X2+7*X2*X+20*X2+19*X+1),3)*(X2+5*X+13))/X;
                default: break;
                }
                K=J/(J-1728);
                A0=-3*K;
                B0=2*K;
            }

        }

// A.14.4.2
// but try -3 first, followed by small positive values for A

        a=A0;
        b=B0;
        at=-3;
        if (D==3) at=1;
        forever
        {
            if (D==1)
            {
                if (at<100)
                    eps=(ZZn)at/(ZZn)A0;
                else eps=rand(p);
                a=modmult(A0,eps,p);
            }
            if (D==3) 
            {
                if (at<100)
                    eps=(ZZn)at/(ZZn)B0;
                else eps=rand(p);
                b=modmult(B0,eps,p);
            }
            if (D!=1 && D!=3)
            {
                if (at<100)
                { // transform a to be simple if possible
                    w=(ZZn)at/ZZn(A0);
                    if (jacobi(w,p)!=1) 
                    {   
                        if (at==-3) at=1;
                        else at++; 
                        continue;
                    }
                    eps=sqrt(w,p);
                } else eps=rand(p);
                a=modmult(A0,pow(eps,2,p),p);
                b=modmult(B0,pow(eps,3,p),p);   
            } 
            ecurve(a,b,p,MR_PROJECTIVE);
            for (int xc=1;;xc++)
            {
                if (!pt.set((Big)xc)) continue;
                pt*=k;
                if (pt.iszero()) continue;
                break;
            }
            G=pt;                  // check its not the other one...

            if (r!=ord || !(other_r*G).iszero())  
            { 
                pt*=ord;
                if (pt.iszero()) break;
            }

            if (at==-3) at=1;
            else at++;
        }
        if (a==(p-3)) a=-3;

        if (D!=1 && D!=3 && three && a!=-3) continue;

// check MOV condition
// A.12.1

        BOOL MOV=TRUE;
        
        w=1;
        for (i=1;i<100;i++)
        {
            w=modmult(w,p,ord);
            if (w==1) 
            {
               MOV=FALSE;
               if (!suppress) 
               {
                   if (i==1 || pord) cout << "\n*** Failed MOV condition - k = " << i << endl;
                   else              cout << "\n*** Failed MOV condition - k <= " << i << endl;
               }
               break;
            }
        }
    
        if (!suppress)
        {
            if (MOV)  cout << "MOV condition OK" << endl;
            if (pord) cout << "\nCurve and Point Found" << endl;   
            else      cout << "\nCurve Found" << endl; 
        } 

        cout << "A= " << a << endl;
        cout << "B= " << b << endl;
        G.get(x,y);
        cout << "P= " << p << endl;
        cout << "R= " << ord;
        if (pord) 
        {
            cout << " a " << bits(ord) << " bit prime"  << endl;
            cout << "X= " << x << endl;
            cout << "Y= " << y << endl;
        }
        else            cout << " NOT prime" << endl;
        cout << endl;

        if (D!=1 && D!=3 )
        {
                cout << "Try for different random factorisation (Y/N)? ";
                cin >> c;
                if (c=='Y' || c=='y') continue;
        }
        break;
    }
    if (pord) cout << "\nCurve and Point OK (Y/N)? " ;  
    else      cout << "\nCurve OK (Y/N)? " ;
    cin >> c;
    if (c=='N' || c=='n') 
    {
        if (!suppress) cout << "finding a curve..." << endl;
        mirexit();
        int precision=(1<<m);
        mip=mirsys(precision+2,0);     // restart with new precision
        setprecision(m);
        gprime(1000000);
        mip->RPOINT=ON;


        return FALSE;
    }
    if (fout)
    {
        ofile << bits(p) << endl;
        mip->IOBASE=16;
        ofile << p << endl;
        ofile << a << endl;
        ofile << b << endl;
        ofile << ord << endl;
        if (pord)
        {
            ofile << x << endl;
            ofile << y << endl;
        }
        mip->IOBASE=10;
    }
    exit(0);
    return TRUE;
}

// Code to parse formula
// This code isn't mine, but its public domain
// Shamefully I forget the source
//
// NOTE: It may be necessary on some platforms to change the operators * and #

#if defined(unix)
#define TIMES '.'
#define RAISE '^'
#else
#define TIMES '*'
#define RAISE '#'
#endif

void eval_power (Big& oldn,Big& n,char op)
{
        if (op) n=pow(oldn,toint(n));    // power(oldn,size(n),n,n);
}

void eval_product (Big& oldn,Big& n,char op)
{
        switch (op)
        {
        case TIMES:
                n*=oldn; 
                break;
        case '/':
                n=oldn/n;
                break;
        case '%':
                n=oldn%n;
        }
}

void eval_sum (Big& oldn,Big& n,char op)
{
        switch (op)
        {
        case '+':
                n+=oldn;
                break;
        case '-':
                n=oldn-n;
        }
}

void eval (Big& t)
{
        Big oldn[3];
        Big n;
        int i;
        char oldop[3];
        char op;
        char minus;
        for (i=0;i<3;i++)
        {
            oldop[i]=0;
        }
LOOP:
        while (*s==' ')
        s++;
        if (*s=='-')    /* Unary minus */
        {
        s++;
        minus=1;
        }
        else
        minus=0;
        while (*s==' ')
        s++;
        if (*s=='(' || *s=='[' || *s=='{')    /* Number is subexpression */
        {
        s++;
        eval(t);
        n=t;
        }
        else            /* Number is decimal value */
        {
        for (i=0;s[i]>='0' && s[i]<='9';i++)
                ;
        if (!i)         /* No digits found */
        {
                cout <<  "Error - invalid number" << endl;
                exit (20);
        }
        op=s[i];
        s[i]=0;
        n=atoi(s);
        s+=i;
        *s=op;
        }
        if (minus) n=-n;
        do
        op=*s++;
        while (op==' ');
        if (op==0 || op==')' || op==']' || op=='}')
        {
        eval_power (oldn[2],n,oldop[2]);
        eval_product (oldn[1],n,oldop[1]);
        eval_sum (oldn[0],n,oldop[0]);
        t=n;
        return;
        }
        else
        {
        if (op==RAISE)
        {
                eval_power (oldn[2],n,oldop[2]);
                oldn[2]=n;
                oldop[2]=RAISE;
        }
        else
        {
                if (op==TIMES || op=='/' || op=='%')
                {
                eval_power (oldn[2],n,oldop[2]);
                oldop[2]=0;
                eval_product (oldn[1],n,oldop[1]);
                oldn[1]=n;
                oldop[1]=op;
                }
                else
                {
                if (op=='+' || op=='-')
                {
                        eval_power (oldn[2],n,oldop[2]);
                        oldop[2]=0;
                        eval_product (oldn[1],n,oldop[1]);
                        oldop[1]=0;
                        eval_sum (oldn[0],n,oldop[0]);
                        oldn[0]=n;
                        oldop[0]=op;
                }
                else    /* Error - invalid operator */
                {
                        cout <<  "Error - invalid operator" << endl;
                        exit (20);
                }
                }
        }
        }
        goto LOOP;
}

int main(int argc,char **argv)
{
    BOOL good,dir,always;
    int i,ip,m,precision;
    unsigned long SD,D;
    ofstream ofile;

    mip=mirsys(128,0);
    Big p,k,t;

    argv++; argc--;   

    irand(0L);
    if (argc<1)
    {
      cout << "Incorrect Usage" << endl;
      cout << "Program tries to find Elliptic Curve mod a prime P and point of prime order" << endl;
      cout << "that is a point (X,Y) on the curve y^2=x^3+Ax+B of order R" << endl;
      cout << "where R is large and prime > |P/K|. (K defaults to 256)" << endl;
      cout << "cm <prime number P>" << endl;
      cout << "OR" << endl;
      cout << "cm -f <formula for P>" << endl;
#if defined(unix)
      cout << "e.g. cm -f 2^192-2^64-1" << endl;
#else
      cout << "e.g. cm -f 2#192-2#64-1" << endl;
#endif
      cout << "To suppress commentary, use flag -s. To insist on A= -3, use flag -t" << endl;
      cout << "(the commentary will make some sense to readers of IEEE 1363 Annex)" << endl;
      cout << "To search downwards for a prime, use flag -d" << endl;
      cout << "To output to a file, use flag -o <filename>" << endl;
      cout << "To set maximum  co-factor size K, use e.g. flag -K8" << endl;
      cout << "To set infinite co-factor size K, use flag -K0" << endl;
      cout << "(In this case the reported R is the number of points on the curve)" << endl;
      cout << "To start searching from a particular D value, use e.g. flag -D10000" << endl;
      cout << "To set MIRACL precision to 2^n words, use e.g. flag -Pn (default -P5)" << endl;
      cout << "If program fails, try again with n=n+1" << endl;  
      cout << "To insist on IEEE-1363 invariants, use flag -IEEE" << endl;
#if defined(unix)
      cout << "e.g. cm -f 2^224-2^96+1 -K12 -D4000000 -P9 -o common.ecs" << endl;
#else
      cout << "e.g. cm -f 2#224-2#96+1 -K12 -D4000000 -P9 -o common.ecs" << endl;
#endif
      cout << "Freeware from Certivox, Dublin, Ireland" << endl;
      cout << "Full C++ source code and MIRACL multiprecision library available" << endl;
      cout << "Email to mscott@indigo.ie for details" << endl;
      return 0;
    }

    gprime(1000);
    m=5;
    ip=0;
    k=256;
    SD=1;
    suppress=FALSE;
    fout=FALSE;
    dir=FALSE;
    three=FALSE;
    always=FALSE;
    p=0;
    while (ip<argc)
    {
        if (strcmp(argv[ip],"-f")==0)
        {
            if (p==0)
            {
                ip++;
                s=argv[ip++];
                t=0;
                eval(t);
                p=t;
                continue;
            }
            else
            {
                cout << "Error in command line" << endl;
                return 0;
            }
        }    
        if (strcmp(argv[ip],"-s")==0)
        {
            ip++;
            suppress=TRUE;
            continue;
        }
        if (strcmp(argv[ip],"-t")==0)
        {
            ip++;
            three=TRUE;
            continue;
        }
        if (strcmp(argv[ip],"-d")==0)
        {
            ip++;
            dir=TRUE;
            continue;
        }
        if (strncmp(argv[ip],"-K",2)==0)
        {
             k=argv[ip]+2;
             ip++;
             continue;
        }
        if (strncmp(argv[ip],"-D",2)==0)
        {
             SD=atol(argv[ip]+2);
             ip++;
             continue;
        }
        if (strncmp(argv[ip],"-P",2)==0)
        {
             m=atoi(argv[ip]+2);
             ip++;
             continue;
        }
        if (strncmp(argv[ip],"-IEEE",5)==0)
        {
             always=TRUE;
             ip++;
             continue;
        }
        if (strcmp(argv[ip],"-o")==0)
        {
            ip++;
            fout=TRUE;
            ofile.open(argv[ip++]);
            continue;
        }
        if (p==0) p=argv[ip++];
        else
        {
            cout << "Error in command line" << endl;
            return 0;
        }
    }

    if (!prime(p))
    {
        int incr=0;
        cout << "That number is not prime!" << endl;
        if (dir)
        {
            cout << "Looking for next lower prime" << endl;
            p-=1; incr++;
            while (!prime(p)) { p-=1;  incr++; }
            cout << "Prime P = P-" << incr << endl;
        }
        else
        {
            cout << "Looking for next higher prime" << endl;
            p+=1; incr++;
            while (!prime(p)) { p+=1;  incr++; }
            cout << "Prime P = P+" << incr << endl;
        }
        cout << "Prime P = " << p << endl;
    }
    if (p<100)
    {
        cout << "Prime is too small - use another method!" << endl;
        return 0;
    }
    if (bits(p)>2048)
    {
        cout << "Prime is too big - sorry" << endl;
        return 0;
    }


    Big W,V,K,r1,r2,ord,rmin;

    if (k==0) rmin=1;
    else      rmin=(p-2*sqrt(p))/k;
    if (rmin==0) 
    {
        cout << "Bad k co-factor value" << endl;
        return 0;
    }

    W=sqrt(p)+1;
    K=(W*W)/rmin;

    if (!suppress) cout << "P mod  8 = " << p%8 << endl;
    if (!suppress) cout << "P is " << bits(p) << " bits long" << endl;
    if (!suppress) cout << "precomputations..." << endl;

    mirexit();

    if (m<4) m=4;

    precision=(1<<m);
    mip=mirsys(precision+2,0);     // restart with new precision
    setprecision(m);
    if (!suppress) cout << "precision in bits = " << precision*MIRACL << endl;
    gprime(65536);
    mip->RPOINT=ON;

    Complex lam;

    Float Fi2[7];
    Float pi24;

    pi24=fpi()/24; 
    lam=exp(Complex((Float)0,pi24));

    Fi2[0]=1;
    Fi2[2]=reciprocal(nroot((Float)2,3)); // pow((Float)2,Float(-1,3));
    Fi2[3]=reciprocal(sqrt((Float)2));    // pow((Float)2,Float(-1,2));
    Fi2[6]=(Float)1/2;

    if (!suppress) cout << "finding a curve..." << endl;

    for (D=SD;;D++)
    {
        if (three && D==3) continue;
        if (!isaD(D,p%8,K)) continue;
        if (jacobi(p-(Big)D,p)==-1) continue;
        good=TRUE;

// A.14.2.3
        for (i=1;;i++)
        {
            unsigned long sp=mip->PRIMES[i];
            if (D==sp || sp*sp>D) break;
            if (D%sp==0 && jacobi(p,(Big)sp)==-1)
            {
                good=FALSE;
                break;
            }
        }
        if (!good) continue;
        if (!isacm(p,D,W,V)) continue;

        r1=p+1+W;
        r2=p+1-W;

        if (k==0) ord=r1;
        else      
        {
            ord=trial_divide(r1);
            if (!prime(ord) && r1%k==0) ord=r1/k;    
        }
        if (ord==1) ord=r1;

        if (ord>=rmin && (k==0 || prime(ord))) 
            get_curve(lam,Fi2,ofile,r1,r2,ord,D,p,W,m,always);

        if (k==0) ord=r2;
        else      
        {
            ord=trial_divide(r2);
            if (!prime(ord) && r2%k==0) ord=r2/k;    
        }
        if (ord==1) ord=r2;

        if (ord>=rmin && (k==0 || prime(ord))) 
            get_curve(lam,Fi2,ofile,r2,r1,ord,D,p,W,m,always);

        if (D==1)
        {
            r1=p+1+V;
            r2=p+1-V;
            if (k==0) ord=r1;
            else      
            {
                ord=trial_divide(r1);
                if (!prime(ord) && r1%k==0) ord=r1/k;
            }
            if (ord==1) ord=r1;
            if (ord>=rmin && (k==0 || prime(ord))) 
                get_curve(lam,Fi2,ofile,r1,r2,ord,D,p,W,m,always);
 
            if (k==0) ord=r2;
            else      
            {
                ord=trial_divide(r2);
                if (!prime(ord) && r2%k==0) ord=r2/k;
            }
            if (ord==1) ord=r2;
            if (ord>=rmin && (k==0 || prime(ord))) 
                get_curve(lam,Fi2,ofile,r2,r1,ord,D,p,W,m,always);
            
        }
        if (D==3)
        {
            r1=p+1+(W+3*V)/2;
            r2=p+1-(W+3*V)/2;
            if (k==0) ord=r1;
            else 
            {
                ord=trial_divide(r1);
                if (!prime(ord) && r1%k==0) ord=r1/k;
            }
            if (ord==1) ord=r1;
            if (ord>=rmin && (k==0 || prime(ord))) 
                get_curve(lam,Fi2,ofile,r1,r2,ord,D,p,W,m,always);

            if (k==0) ord=r2;
            else 
            {
                ord=trial_divide(r2);
                if (!prime(ord) && r2%k==0) ord=r2/k;
            }
            if (ord==1) ord=r2;
            if (ord>=rmin && (k==0 || prime(ord))) 
                get_curve(lam,Fi2,ofile,r2,r1,ord,D,p,W,m,always);

            r1=p+1+(W-3*V)/2;
            r2=p+1-(W-3*V)/2;
            if (k==0) ord=r1;
            else 
            {
                ord=trial_divide(r1);
                if (!prime(ord) && r1%k==0) ord=r1/k;
            }
            if (ord==1) ord=r1;
            if (ord>=rmin && (k==0 || prime(ord))) 
                get_curve(lam,Fi2,ofile,r1,r2,ord,D,p,W,m,always);

            if (k==0) ord=r2;
            else 
            {
                ord=trial_divide(r2);
                if (!prime(ord) && r2%k==0) ord=r2/k;
            }
            if (ord==1) ord=r2;
            if (ord>=rmin && (k==0 || prime(ord))) 
                get_curve(lam,Fi2,ofile,r2,r1,ord,D,p,W,m,always);
        }
    }
    cout << "No satisfactory curve found" << endl;
    return 0;
}


