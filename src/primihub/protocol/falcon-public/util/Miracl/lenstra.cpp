/*
 *   Program to factor big numbers using Lenstras elliptic curve method.
 *   Works when for some prime divisor p of n, p+1+d has only
 *   small factors, where d depends on the particular curve used.
 *   See "Speeding the Pollard and Elliptic Curve Methods"
 *   by Peter Montgomery, Math. Comp. Vol. 48 Jan. 1987 pp243-264
 *
 *   Requires: big.cpp zzn.cpp
 *
 */

#include <iostream>
#include <iomanip>
#include "zzn.h"

using namespace std;

#define LIMIT1 10000     /* must be int, and > MULT/2 */
#define LIMIT2 1000000L  /* may be long */
#define MULT   2310      /* must be int, product of small primes 2.3... */
#define NEXT   13           /* next small prime */
#define NCURVES 20      /* number of curves to try */

Miracl precision=50;    /* number of ints per ZZn */

miracl *mip;
static long p;
static int iv;
static ZZn ak,q,x,z,x1,z1,x2,z2,xt,zt,fvw,fu[1+MULT/2];
static BOOL cp[1+MULT/2],Plus[1+MULT/2],Minus[1+MULT/2];

void marks(long start)
{ /* mark non-primes in this interval. Note    *
   * that those < NEXT are dealt with already  */
    int i,pr,j,k;
    for (j=1;j<=MULT/2;j+=2) Plus[j]=Minus[j]=TRUE;
    for (i=0;;i++)
    { /* mark in both directions */
        pr=mip->PRIMES[i];
        if (pr<NEXT) continue;
        if ((long)pr*pr>start) break;
        k=pr-start%pr;
        for (j=k;j<=MULT/2;j+=pr)
            Plus[j]=FALSE;
        k=start%pr;
        for (j=k;j<=MULT/2;j+=pr)
            Minus[j]=FALSE;
    }        
}

void duplication(ZZn sum,ZZn diff,ZZn& x,ZZn& z)
{ /* double a point on the curve P(x,z)=2.P(x1,z1) */
    ZZn t;
    t=sum*sum;
    z=diff*diff;
    x=z*t;        /* x = sum^2.diff^2 */
    t-=z;         /* t = sum^2-diff^2 */
    z+=ak*t;      /* z = ak*t +diff^2 */
    z*=t;
}

void addition(ZZn xd,ZZn zd,ZZn sm1,ZZn df1,ZZn sm2,ZZn df2,ZZn& x,ZZn& z)
{ /* add two points on the curve P(x,z)=P(x1,z1)+P(x2,z2) *
   * given their difference P(xd,zd)                      */
    ZZn t;
    x=df2*sm1;
    z=df1*sm2;
    t=z+x;
    z-=x;
    x=t*t;
    x*=zd;   /* x = zd.[df1.sm2+sm1.df2]^2 */
    z*=z;
    z*=xd;   /* z = xd.[df1.sm2-sm1.df2]^2 */

}

void ellipse(ZZn x,ZZn z,int r,ZZn& x1,ZZn& z1,ZZn& x2,ZZn& z2)
{ /* calculate point r.P(x,z) on curve */ 
    int k,rr;
    k=1;
    rr=r;
    x1=x;
    z1=z;
    duplication(x1+z1,x1-z1,x2,z2);  /* generate 2.P */
    while ((rr/=2)>1) k*=2;
    while (k>0)
    { /* use binary method */
        if ((r&k)==0)
        { /* double P(x1,z1) mP to 2mP */
            addition(x,z,x1+z1,x1-z1,x2+z2,x2-z2,x2,z2);
            duplication(x1+z1,x1-z1,x1,z1);
        }
        else
        { /* double P(x2,z2) (m+1)P to (2m+2)P */
            addition(x,z,x1+z1,x1-z1,x2+z2,x2-z2,x1,z1);
            duplication(x2+z2,x2-z2,x2,z2);
        }    
        k/=2;
    }
}

void next_phase()
{ /* now change gear */
    ZZn s1,d1,s2,d2;
    long interval;
    xt=x;
    zt=z;
    s2=x+z;
    d2=x-z;                    /* P = (s2,d2) */
    duplication(s2,d2,x,z);
    s1=x+z;
    d1=x-z;                    /* 2.P = (s1,d1) */
    fu[1]=x1/z1;
    addition(x1,z1,s1,d1,s2,d2,x2,z2);  /* 3.P = (x2,z2) */
    for (int m=5;m<=MULT/2;m+=2)
    { /* calculate m.P = (x,z) and store fu[m] = x/z */ 
        addition(x1,z1,x2+z2,x2-z2,s1,d1,x,z);
        x1=x2;
        z1=z2;
        x2=x;
        z2=z;
        if (!cp[m]) continue;
        fu[m]=x2/z2;        
    }
    ellipse(xt,zt,MULT,x,z,x2,z2);
    xt=x+z;
    zt=x-z;                           /* MULT.P = (xt,zt) */
    iv=p/MULT;
    if (p%MULT>MULT/2) iv++;
    interval=(long)iv*MULT;
    p=interval+1;
    ellipse(x,z,iv,x1,z1,x2,z2);      /* (x1,z1) = iv.MULT.P */
    fvw=x1/z1;
    marks(interval);
    q=fvw-fu[p%MULT];  
}

int giant_step()
{ /* increment giant step */
    long interval;
    iv++;
    interval=(long)iv*MULT;
    p=interval+1;
    marks(interval);
    fvw=x2/z2;
    addition(x1,z1,x2+z2,x2-z2,xt,zt,x,z);
    x1=x2;
    z1=z2;
    x2=x;
    z2=z;
    return 1;   
}

int main()
{  /*  factoring program using Lenstras Elliptic Curve method */
    int phase,m,k,nc,pos,btch;
    long i,pa;
    Big n,t;
    ZZn tt,u,v;
    mip=&precision;
    gprime(LIMIT1);
    for (m=1;m<=MULT/2;m+=2) 
        if (igcd(MULT,m)==1) cp[m]=TRUE;
        else                 cp[m]=FALSE;
    cout << "input number to be factored\n";
    cin >> n;
    if (prime(n))
    {
        cout << "this number is prime!\n";
        return 0;
    }
    modulo(n);                 /* do all arithmetic mod n */
    for (nc=1,k=6;k<100;k++)
    { /* try a new curve */
        u=k*k-5;
        v=4*k;
        x=u*u*u;
        z=v*v*v;

        ak=((v-u)*(v-u)*(v-u)*(3*u+v))/(16*u*u*u*v);

        phase=1;
        p=0;
        i=0;
        btch=50;
        cout << "phase 1 - trying all primes less than " << LIMIT1;
        cout << "\nprime= " << setw(8) << p;
        forever
        { /* main loop */
            if (phase==1)
            {
                p=mip->PRIMES[i];
                if (mip->PRIMES[i+1]==0)
                { /* now change gear */
                    phase=2;
                    cout << "\nphase 2 - trying last prime less than ";
                    cout << LIMIT2 << "\nprime= " << setw(8) << p;
                    next_phase();
                    btch*=100;
                    i++;
                    continue;
                }
                pa=p;
                while ((LIMIT1/p) > pa) pa*=p;
                ellipse(x,z,(int)pa,x1,z1,x2,z2);
                x=x1;
                q=z=z1;
            }
            else
            { /* looking for last large prime factor of (p+1+d) */
                p+=2;
                pos=p%MULT;
                if (pos>MULT/2) pos=giant_step();
                if (!cp[pos]) continue;
        /* if neither interval +/- pos is prime, don't bother */
                if (!Plus[pos] && !Minus[pos]) continue;
                q*=(fvw-fu[pos]);        /* batch gcds */
            }
            if (i++%btch==0)
            { /* try for a solution */
                cout << "\b\b\b\b\b\b\b\b" << setw(8) << p << flush;
                t=gcd(q,n);
                if (t==1)
                {
                    if (p>LIMIT2) break;
                    else continue;
                }
                if (t==n)
                {
                    cout << "\ndegenerate case";
                    break;
                }
                if (prime(t)) cout << "\nprime factor     " << t;
                else          cout << "\ncomposite factor " << t;
                n/=t;
                if (prime(n)) cout << "\nprime factor     " << n;
                else          cout << "\ncomposite factor " << n;
                cout << endl;
                return 0;
            }
        }
        if (nc>NCURVES) break;
        cout << "\ntrying a different curve " << nc << "\n";
    } 
    cout << "\nfailed to factor\n";
    return 0;
}

