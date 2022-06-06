//
// Program to generate Modular Polynomials mod p, as required for fast
// implementations of the Schoof-Elkies-Atkins algorithm
// for counting points on an elliptic curve Y^2=X^3 + A.X + B mod p
//
// Implemented entirely from the description provided in:
// 1. "Distributed Computation of the number of points on an elliptic curve
//    over a finite prime field", Buchmann, Mueller, & Shoup, SFB 124-TP D5
//    Report 03/95, April 1995, Universitat des Saarlandes, and
// 2. "Counting the number of points on elliptic curves over finite fields
//    of characteristic greater than three", Lehmann, Maurer, Mueller & Shoup,
//    Proc. 1st Algorithmic Number Theory Symposium (ANTS), pp 60-70, 1994
//
// Both papers are available on the Web from Volker Mueller's home page
// www.informatik.th-darmstadt.de/TI/Mitarbeiter/vmueller.html
//
// Also strongly recommended is the book 
//
// 3. "Elliptic Curves in Cryptography"
//     by Blake, Seroussi and Smart, London Mathematical Society Lecture Note 
//     Series 265, Cambridge University Press. ISBN 0 521 65374 6 
//
// The programs's output for each prime in the range is a bivariate polynomial 
// in X and Y, which can optionally be stored to disk. Some informative output
// is generated just to convince you that it is still working, and to give an 
// idea of progress.
//
// This program is a composite of the "mueller" and "process" applications.
// It generates the modular polynomials, pre-reduced wrt to a specified prime
// modulus. This may be the only feasible way to do it on a small computer 
// system, for which the "mueller" application is too resource intensive. 
//
// Although less memory intensive than "mueller", problems may still arise.
// See mueller.cpp for a description of the -s2, -s3 and -s6 flags
//
// .pol file format 
// <modulus>,<prime>,<1st coef>,<1st power of X>,<1st power of Y>,<2nd coef>...
// Each polynomial ends wih zero powers of X and Y.
//
// For example
// modpol -d -f 2#512 0 500 -o test512.pol
//
// If appending to a file with the -a flag, make sure and use the same 
// prime modulus as used to create the file originally - no check is made
//
// generates the test512.pol file directly, given the range 0 to 500 and 
// using the first prime modulus it can find less than 2^512. This file can 
// then be used directly with the "sea" application
//

#include <iostream>
#include <fstream>
#include <cstring>
#include <iomanip>
#include "ps_zzn.h"      // power series class

using namespace std;

extern int psN;          // power series are modulo x^psN

BOOL fout;
BOOL append;
Miracl precision=20;

ofstream mueller;

// Code to parse formula in command line
// This code isn't mine, but its public domain
// Shamefully I forget the source
//
// NOTE: It may be necessary on some platforms to change the operators * and #
//

#if defined(unix)
#define TIMES '.'
#define RAISE '^'
#else
#define TIMES '*'
#define RAISE '#'
#endif

Big tt;
static char *ss;

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

void eval (void)
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
        while (*ss==' ')
        ss++;
        if (*ss=='-')    /* Unary minus */
        {
        ss++;
        minus=1;
        }
        else
        minus=0;
        while (*ss==' ')
        ss++;
        if (*ss=='(' || *ss=='[' || *ss=='{')    /* Number is subexpression */
        {
        ss++;
        eval ();
        n=tt;
        }
        else            /* Number is decimal value */
        {
        for (i=0;ss[i]>='0' && ss[i]<='9';i++)
                ;
        if (!i)         /* No digits found */
        {
                cout <<  "Error - invalid number" << endl;
                exit (20);
        }
        op=ss[i];
        ss[i]=0;
        n=atoi(ss);
        ss+=i;
        *ss=op;
        }
        if (minus) n=-n;
        do
        op=*ss++;
        while (op==' ');
        if (op==0 || op==')' || op==']' || op=='}')
        {
        eval_power (oldn[2],n,oldop[2]);
        eval_product (oldn[1],n,oldop[1]);
        eval_sum (oldn[0],n,oldop[0]);
        tt=n;
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

//
// When summing the Zk^n 0<=k<L (1. page 3, top), most terms cancel out,  
// leaving only every L-th term
//

Ps_ZZn phase(Ps_ZZn &z, int L,int off)
{ // Keep L times every L-th element in the Power Series
    Ps_ZZn w;
    term_ps_zzn *pos=NULL;
    int i,k;
    k=off+z.first();
    for (i=off;i<psN;i+=L,k+=L)
    {
        pos=w.addterm(L*z.coeff(k),k,pos);
    }
    return w;
}

void mueller_pol(int L,int s)
{ // Calculate Modular Polynomial for prime L 
  // s is smallest int such that s*(L-1)/12 is integer
    int i,j,n,v;
  
    Ps_ZZn klein,flt,zlt,x,y,z,f,jlt[500],c[1000],ps[1000];


// First calculate v, and hence psN - number of terms in Power Series
// 2. page 5 1st para

    cout << "preliminaries" << flush;

    v=s*(L-1)/12;
    psN=v+2;

//
// calculate Klein=j(tau) from its definition
// Numerator x... 
//
// 1. page 2
//
    for (n=1;n<psN;n++)
    {
        Ps_ZZn a,b,t;
        a.addterm((ZZn)n*n*n,n);     // a=n^3*x^n
        b.addterm((ZZn)1,0);
        b.addterm((ZZn)-1,n);
        t=a/b;
        x+=t;
    }
    x=(ZZn)240*x;
    x.addterm((ZZn)1,0);

    x=pow(x,3);
// Denominator y...
    y=eta();
    y=pow(y,24);

    klein=x/y;
    cout << "." << flush;
    klein.divxn(1);         // divides power series by x^parameter

    psN*=L;
//    cout << "psN= " << psN << endl;
    klein=power(klein,L);   // this substitutes x^L for x in the power series
    cout << "." << flush;

// Find Fl(t), Numerator z= Dedekind eta function 
// This has a simple repeating pattern of coefficients, and so costs nothing
// to calculate 1. page 2 bottom
    

    z=eta();
// Denominator y=n(Lt)...
    y=power(z,L);
    y=(ZZn)1/y;       // y has only psN/L terms.
    cout << "." << flush;
    z*=y;            // z has psN terms 
    flt=pow(z,2*s);   // ^2*s - expensive
    cout << "." << flush;
    flt.divxn(v);     // times x^-v

    ZZn w=pow((ZZn)L,s);
    y=power(flt,L);
    cout << "." << flush;
    zlt=w/y;          // l^s/Fl(lt) - cheap - psN/L terms in power series
    cout << "." << endl;
    y.clear();
    x.clear();

    ps[0]=L+1;
//
// Calculate Power Sums. Note that f and flt are very large objects
// with psN terms. Most other power series are in "compressed" form
// with "only" psN/L terms
//
// 1. page 3
//

    cout << "Power Sum   = " << flush;
    z=1;
    f=1;
    for (i=1;i<=L+1;i++)
    {
        cout << setw(3) << i << flush;
        f*=flt;    // expensive. In place multiplication discourages C++
                   // from moving large objects about
        z=z*zlt;   // cheap
        ps[i]=phase(f,L,(i*v)%L)+z;
        cout << "\b\b\b" << flush;
    }

    cout << setw(3) << L+1 << endl;

    f.clear();
    z.clear();
    flt.clear();
    zlt.clear();

    cout << "Coefficient = " << flush;
    c[0]=1;
//
// Newton's Identities - Calculate coefficients from Power Sums
//
// from a Web page somewhere and 3. page 54
//
    for (i=1;i<=L+1;i++)
    {
        cout << setw(3) << i << flush;
        c[i]=0;
        for (j=1;j<=i;j++)
              c[i]+=(ps[j]*c[i-j]);   // cheap, but lots of them
        c[i]=(-c[i])/i;
        cout << "\b\b\b" << flush;
    }

    cout << setw(3) << L+1 << endl;

    for (i=0;i<=L+1;i++) ps[i].clear(); // reclaim space
//
// Get powers of j(Lt)^i, i=1 to v
// These will be needed to determine the exponent of Y in each
// coefficient of the Modular Polynomial
//
    jlt[0]=1;
    jlt[1]=klein;

    for (i=2;i<=v;i++)
        jlt[i]=jlt[i-1]*klein;    // cheap
//
// Find Modular Polynomial, format it, and output
//
// 2. page 5, middle "Hl(X) = ..."
//
    cout << "\nG" << L << "(X,Y) = X^" << L+1 ;

    if (fout) 
    {
        mueller << L << endl;
        mueller << 1 << "\n" << L+1 << "\n" << 0 << endl;
    }

    for (i=1;i<=L+1;i++)
    {
        ZZn cf;
        BOOL brackets,first;
        first=TRUE;
        brackets=FALSE;
        z=c[i];            

// idea is to reduce this to an integer
// by subtracting j(Lt)^k as necessary
// The power of k required is then
// the coefficient of Y^k in G(X,Y)
// The first coefficient of c[i] tells us which j(Lt)^k to try

        if (z.first()!=0) 
        {
            brackets=TRUE;
            cout << "+(" ;
        }
   // coefficient may be a polynomial in Y
        while (z.first()!=0)    
        {
            int j=(-z.first()/L);    // index into jlt
            cf=z.coeff(z.first());   // get coefficient to be cancelled
            if (fout) mueller << cf << "\n" << L+1-i << "\n" << j << endl;
            if (cf==0) break;
            z-=(jlt[j]*cf);
            if (!first || !brackets) cout << "+";
            first=FALSE;
            if (cf==1) cout << "Y";
            else cout << cf << "*Y";
            if (j!=1) cout << "^" << j;
        }
        cf=z.coeff(0);
        if (fout) mueller << cf << "\n" << L+1-i << "\n" << 0 << endl;
        if (brackets) 
        {
            cout << "+" << cf << ")*X";
            if (i!=L) cout << "^" << L+1-i ;
        }
        else
        {
            if (i==L+1) 
            {
                cout << "+" << cf;
                continue;
            }
            if (cf!=0)
            {
                if (cf==1) cout << "+X";
                else cout << "+" << cf << "*X";
                if (i!=L) cout << "^" << L+1-i ;
            }
        }
  // all other coefficients should now be zero
        if (z.coeff(L)!=0) 
        { // check next coefficient is zero
            cout << "\n\n Sanity Check Failed " << endl;
            exit(0);
        }
    }
    for (i=0;i<=L+1;i++) c[i].clear(); // reclaim space
    for (i=0;i<=v;i++) jlt[i].clear();
    cout << endl;

    fft_reset();
}

int main(int argc,char **argv)
{
    Big p;
    miracl *mip=get_mip();
    int i,j,s,lo,hi,sp,ip,skip;
    int primes[200];
    BOOL dir,gotP,gothi,gotlo;
    argv++; argc--;
    int Base;

    if (argc<1)
    {
        cout << "Incorrect usage" << endl;
        cout << "Program generates Modular Polynomials, for use by fast Schoof-Elkies-Atkins" << endl;
        cout << "program for counting points on an elliptic curve" << endl;
        cout << "modpol <prime modulus P> <low number> <high number>" << endl;
        cout << "OR" << endl;
        cout << "modpol <formula for P>   <low number> <high number>" << endl;
        cout << "where the numbers define a range. The program will find the" << endl;
        cout << "Modular Polynomials for primes in this range wrt the specified modulus" << endl;
        cout << "To input P in Hex, precede with -h" << endl;
        cout << "To search downwards for a prime, use flag -d" << endl;
        cout << "NOTE: Program is both memory and time intensive" << endl;
        cout << "To skip \"difficult\" primes, use -s2, -s3 or -s6" << endl;
        cout << "where -s2 skips most and -s6 skips least" << endl;
        cout << "To output polynomials to a file use flag -o <filename>" << endl;
#if defined(unix)
        cout << "e.g. modpol -f 2^192-2^64-1 0 150 -o p192.pol" << endl;
#else
        cout << "e.g. modpol -f 2#192-2#64-1 0 150 -o p192.pol" << endl;
#endif
        cout << "Alternatively to append to a file use flag -a <filename>" << endl;
        cout << "See source code file for details" << endl;
        cout << "\nFreeware from Certivox, Dublin, Ireland" << endl;
        cout << "Full C++ source code and MIRACL multiprecision library available" << endl;
        cout << "email mscott@indigo.ie" << endl;

        return 0;
    }
    if (argc<3)
    {
        cout << "Error in command line" << endl;
        return 0;
    }
    ip=0;
    skip=12;
    fout=FALSE;
    dir=gotP=gothi=gotlo=FALSE;
    append=FALSE;
    Base=10;
    while (ip<argc)
    {
        if (!gotP && strcmp(argv[ip],"-f")==0)
        {
            ip++;
            if (!gotP && ip<argc)
            {

                ss=argv[ip++];
                tt=0;
                eval();
                p=tt;
                gotP=TRUE;
                continue;
            }
            else
            {
                cout << "Error in command line" << endl;
                return 0;
            }
        }

        if (strcmp(argv[ip],"-d")==0)
        {
            ip++;
            dir=TRUE;
            continue;
        }
        if (skip==12 && strcmp(argv[ip],"-s2")==0)
        {
            ip++;
            skip=2;
            continue;
        }
        if (skip==12 && strcmp(argv[ip],"-s3")==0)
        {
            ip++;
            skip=3;
            continue;
        }
        if (skip==12 && strcmp(argv[ip],"-s6")==0)
        {
            ip++;
            skip=6;
            continue;
        }

        if (!fout && strcmp(argv[ip],"-o")==0)
        {
            ip++;
            if (ip<argc)
            {
                fout=TRUE;
                append=FALSE;
                mueller.open(argv[ip++]);
                continue;
            }
            else
            {
                cout << "Error in command line" << endl;
                return 0;
            }
        }
        if (!fout && strcmp(argv[ip],"-a")==0)
        {
            ip++;
            if (ip<argc)
            {
                fout=TRUE;
                append=TRUE;
                mueller.open(argv[ip++],ios::app);
                continue;
            }
            else
            {
                cout << "Error in command line" << endl;
                return 0;
            }
        }
        if (strcmp(argv[ip],"-h")==0)
        {
            ip++;
            Base=16;
            continue;
        }
        if (!gotP)
        {
            mip->IOBASE=Base; 
            p=argv[ip++];
            mip->IOBASE=10;
            gotP=TRUE;
            continue;
        }
        if (!gotlo)
        {
            lo=atoi(argv[ip++]);
            gotlo=TRUE;
            continue;
        }
        if (!gothi)
        {
            hi=atoi(argv[ip++]);
            gothi=TRUE;
            continue;
        }
        cout << "Error in command line" << endl;
        return 0;
    }
    if (!gothi || !gotlo)
    {
        cout << "Error in command line" << endl;
        return 0;
    }
    if (lo>hi || hi>1000)
    {
        cout << "Invalid range specified" << endl;
        return 0;
    }

    gprime(1000);         // get all primes < 1000
    for (i=0;;i++)
    {
        sp=mip->PRIMES[i];
        primes[i]=sp;
        if (sp==0) break;
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
    cout << "P mod 24 = " << p%24 << endl;
    cout << "P is " << bits(p) << " bits long" << endl;

    if (fout && !append) mueller << p << endl;

    modulo(p);               // Set prime modulus for ZZn type

    for (j=0,i=1;;i++)       // lets go
    { 
        sp=primes[i];
        if (sp==0) break;
        if (sp<lo) continue;
        if (sp>hi) break;
        for (s=1;;s++)
            if (s*(sp-1)%12==0) break;
        if (s>=skip) continue;
        j++;
        cout << endl;
        cout << "prime " << j << " = " << sp << " (s=" << s << ")" << endl; 
        mueller_pol(sp,s); 
    }
    cout << endl;
    if (j==0) cout << "No primes processed in the specified range" << endl;
    if (j==1) cout << "One prime processed in the specified range" << endl;
    if (j>1) cout << j << " primes processed in the specified range" << endl;
    return 0;   
}

