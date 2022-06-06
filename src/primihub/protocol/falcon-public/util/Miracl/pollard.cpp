/*
 *   Program to factor big numbers using Pollards (p-1) method.
 *   Works when for some prime divisor p of n, p-1 has only
 *   small factors.
 *   See "Speeding the Pollard and Elliptic Curve Methods"
 *   by Peter Montgomery, Math. Comp. Vol. 48 Jan. 1987 pp243-264
 *
 *   Requires: big.cpp zzn.cpp
 */

#include <iostream>
#include <iomanip>
#include "zzn.h"

using namespace std;

#define LIMIT1 10000   /* must be int, and > MULT/2 */
#define LIMIT2 2000000L /* may be long */
#define MULT   2310     /* must be int, product of small primes 2.3.. */
#define NEXT   13           /* next small prime */

Miracl precision=50;  /* number of ints per ZZn */

miracl *mip;
static long p;
static int iv;
static ZZn b,bw,bvw,bd,q,bu[1+MULT/2];
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


void next_phase()
{ /* now changing gear */
    ZZn bp,t;
    long interval;
    bw=pow(b,8);
    t=1;
    bp=bu[1]=b;
    for (int m=3;m<=MULT/2;m+=2)
    { /* store bu[m] = b^(m*m) */
        t*=bw;
        bp*=t;
        if (cp[m]) bu[m]=bp;
    }
    t=pow(b,MULT);
    t=pow(t,MULT);
    bd=t*t;            /* bd = b^(2*MULT*MULT) */
    iv=p/MULT;
    if (p%MULT>MULT/2) iv++;
    interval=(long)iv*MULT;
    p=interval+1;
    marks(interval);
    bw=pow(t,(2*iv-1));
    bvw=pow(t,iv);
    bvw=pow(bvw,iv);   /* bvw = b^(MULT*MULT*iv*iv) */
    q=bvw-bu[p%MULT];
}

int giant_step()
{ /* increment giant step */
    long interval;
    iv++;
    interval=(long)iv*MULT;
    p=interval+1;
    marks(interval);
    bw*=bd;
    bvw*=bw;
    return 1;
}

int main()
{  /*  factoring program using Pollards (p-1) method */
    int phase,m,pos,btch;
    long i,pa;
    Big n,t;
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
    modulo(n);                    /* do all arithmetic mod n */
    phase=1;
    p=0;
    btch=50;
    i=0;
    b=2;
    cout << "phase 1 - trying all primes less than " << LIMIT1;
    cout << "\nprime= " << setw(8) << p;
    forever
    { /* main loop */
        if (phase==1)
        { /* looking for all factors of (p-1) < LIMIT1 */
            p=mip->PRIMES[i];
            if (mip->PRIMES[i+1]==0)
            {
                phase=2;
                cout << "\nphase 2 - trying last prime less than ";
                cout  << LIMIT2 << "\nprime= " << setw(8) << p;
                next_phase();
                btch*=100;
                i++;
                continue;
            }
            pa=p;
            while ((LIMIT1/p) > pa) pa*=p;
            b=pow(b,(int)pa);      /* b = b^pa mod n */
            q=b-1;
        }
        else
        { /* looking for last prime factor of (p-1) < LIMIT2 */
            p+=2;
            pos=p%MULT;
            if (pos>MULT/2) pos=giant_step();

        /* if neither interval+/-pos is prime, don't bother */
                if (!Plus[pos] && !Minus[pos]) continue;
            if (!cp[pos]) continue;
            q*=(bvw-bu[pos]);        /* batch gcd's in q */
        }
        if (i++%btch==0)
        { /* try for a solution */
            cout << "\b\b\b\b\b\b\b\b" << setw(8) << p << flush;
            t=gcd((Big)q,n);
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
            if (prime(t)) cout << "\nprime factor      " << t;
            else          cout << "\ncomposite factor  " << t;
            n/=t;
            if (prime(n)) cout << "\nprime factor      " << n;
            else          cout << "\ncomposite factor  " << n;
            cout << endl;
            return 0;
        }
    } 
    cout << "\nfailed to factor\n";
    return 0;
}

