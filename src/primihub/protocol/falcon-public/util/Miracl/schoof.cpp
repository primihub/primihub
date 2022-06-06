// Schoof's Original Algorithm!
// Mike Scott June 1999   mike@compapp.dcu.ie
// Counts points on GF(p) Elliptic Curve, y^2=x^3+Ax+B a prerequisite 
// for implemention of  Elliptic Curve Cryptography
//
// cl /O2 /GX schoof.cpp ecn.cpp zzn.cpp big.cpp crt.cpp poly.cpp polymod.cpp miracl.lib
// g++ -O2 schoof.cpp ecn.cpp zzn.cpp big.cpp crt.cpp poly.cpp polymod.cpp miracl.a -o schoof
//
// !!!!!!!!!!!!!!!!!!!!!!!!
// NOTE! September 1999. This program has been rendered effectively obsolete 
// by the implementation of the superior Schoof-Elkies-Atkin Algorithm
// This has O(log(p)^5) complexity compared with O(log(p)^6), and so will
// always be faster.
//
// Read the comments at the head of the file 
// ftp://ftp.computing.dcu.ie/pub/crypto/sea.cpp
// for more details
//
// However this program is self-contained and hence easier to use.
// It is quite adequate for counting points on up to 256 bit curves (that is 
// for primes p < 256 bits in length) assuming a reasonably powerful PC
// It also works for the smallest curves, and so is ideal for educational
// use
//
// It is now straightforward to create an executable that runs under Linux.
// See ftp://ftp.computing.dcu.ie/pub/crypto/README for details
//
// For elliptic curves defined over GF(2^m), see the utility schoof2.exe
//
// !!!!!!!!!!!!!!!!!!!!!!!!
//
// The self-contained Windows Command Prompt executable for this program may 
// obtained from ftp://ftp.computing.dcu.ie/pub/crypto/schoof.exe 
//
// Basic algorithm is due to Schoof
// "Elliptic Curves Over Finite Fields and the Computation of Square Roots 
// mod p", Rene Schoof, Math. Comp., Vol. 44 pp 483-494
// Expression for Mod 2 Cardinality from "Counting Points on Elliptic
// Curves over Finite Fields", Rene Schoof, Jl. de Theorie des Nombres
// de Bordeaux 7 (1995) pp 219-254
// Another useful reference is
// "Elliptic Curve Public Key Cryptosystems", Menezes, 
// Kluwer Academic Publishers, Chapter 7
// Thanks are due to Richard Crandall for the tip about using prime powers
//
// **
// This program implements Schoof's original algorithm, augmented by
// the use of prime powers. By finding the Number of Points mod the product
// of many small primes and large prime powers, the final search for NP is 
// greatly speeded up.
// Final phase search uses Pollard Lambda ("kangaroo") algorithm
// This final phase effectively stretches the range of Schoof's
// algorithm by about 70-80 bits.
// This approach is only feasible due to the use of fast FFT methods
// for large degree polynomial multiplication
// **
//
// Ref "Monte Carlo Methods for Index Computation"
// by J.M. Pollard in Math. Comp. Vol. 32 1978 pp 918-924
// Ref "A New Polynomial Factorisation Algorithm and its implementation",
// Victor Shoup, Jl. Symbolic Computation, 1996 
//
//
// An "ideal" curve is defined as one with with prime number of points.
//
// When using the "schoof" program, the -s option is particularly useful
// and allows automatic search for an "ideal" curve. If a curve order is
// exactly divisible by a small prime, that curve is immediately abandoned, 
// and the program moves on to the next, incrementing the B parameter of 
// the curve. This is a fairly arbitrary but simple way of moving on to 
// the "next" curve. 
//
// NOTE: The output file can be used directly with for example the ECDSA
// programs IF AND ONLY IF an ideal curve is found. If you wish to use
// a less-than-ideal curve, you will first have to factor NP completely, and
// find a random point of large prime order.
//
// This implementation is free. No terms, no conditions. It requires 
// version 4.24 or greater of the MIRACL library (a Shareware, Commercial 
// product, but free for non-profit making use), 
// available from ftp://ftp.computing.dcu.ie/pub/crypto/miracl.zip 
//
// However this program may be used (unmodified) to generate curves for 
// commercial use.
//
// 32-bit build only
//
// Revision history
//
// Rev. 1 June 1999       - included prime powers
// Rev. 2 June 1999       - tweaked some inner loops
// Rev. 3 July 1999       - changed from rational to projective coordinates
//                          power windowing helps X^PP, Y^PP calculations
// Rev. 4 August 1999     - suppressed unnecessary creation of ZZn temporaries 
//                          in poly.cpp and polymod.cpp
// Rev. 5 September 1999  - Use fast modular composition to calculate X^PP 
//                          Half required size of ZZn's
//                          More & Faster Kangaroos!   
// Rev. 6 October 1999    - Revamped Poly class
//                          25% Faster technique for finding tau mod lp
// Rev. 7 November 1999   - Calculation of Y^PP eliminated!
//
// Rev. 8 November 1999   - Find Y^PP using composition - faster tau search
// Rev. 9 December 1999   - Various optimisations
// Rev. 10 March 2000     - Eigenvalue Heuristic introduced
//
// Timings for test curve Y^2=X^3-3X+49 mod 65112*2^144-1 (160 bits)
// Pentium Pro 180MHz
//
// Rev. 0  - 100 minutes
// Rev. 1  - 67 minutes
// Rev. 2  - 57 minutes
// Rev. 3  - 51 minutes
// Rev. 4  - 46 minutes
// Rev. 5  - 31 minutes
// Rev. 6  - 25 minutes
// Rev. 7  - 22 minutes
// Rev. 8  - 21 minutes
// Rev. 9  - 18 minutes
// Rev. 10 - 13 minutes
//
// 160 bit curve -  13 minutes
// 192 bit curve -  60 minutes
// 224 bit curve - 176 minutes
// 256 bit curve - 355 minutes
//
// This execution time can be related directly to 
// the O(log(p)^6) complexity of the algorithm as implemented here.
//
// Note that a small speed-up can be obtained by using an integer-only 
// build of MIRACL. See mirdef.hio
//
//

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>
#include "ecn.h"    // Elliptic Curve Class
#include "crt.h"         // Chinese Remainder Theorem Class

// poly.h implements polynomial arithmetic. FFT methods are used for maximum
// speed, as the polynomials get very big. For example when searching for the
// curve cardinality mod the prime 31, the polynomials are of degree (31*31-1)/2
// = 480. But all that gruesome detail is hidden away.
//
// polymod.h implements polynomial arithmetic wrt to a preset poynomial 
// modulus. This looks a bit neater. Function setmod() sets the modulus 
// to be used. Again fast FFT methods are used.

#include "poly.h"
#include "polymod.h"

using namespace std;

#ifndef MR_NOFULLWIDTH
Miracl precision=10;            // max. 10x32 bits per big number
#else
Miracl precision(10,MAXBASE);
#endif

PolyMod MY2,MY4;

ZZn A,B;         // Here ZZn are integers mod the prime p
                 // Montgomery representation is used internally

BOOL Edwards=FALSE;

// Elliptic curve Point duplication formula

void elliptic_dup(PolyMod& X,PolyMod& Y,PolyMod& Z)
{ // (X,Y,Z)=2.(X,Y,Z)
    PolyMod W1,W2,W3,W4;

    W2=Z*Z;           // 1
    if (A==(ZZn)(-3))
    {
        W4=3*(X-W2)*(X+W2); // 2  (and save 1)  
    }
    else
    {
        W3=A*(W2*W2);     // 2
        W1=X*X;           // 3
        W4=3*W1+W3;
    }
    Z*=(2*Y);         // 4   Z has an implied y
    W2=MY2*(Y*Y);     // 5
    W3=4*X*W2;        // 6
    W1=W4*W4;         // 7
    X=W1-2*W3;
    W2*=W2;           // 8
    W2*=8;
    W3-=X;
    W3*=W4;          // 9
    Y=W3-W2;    
    X*=MY2;          // fix up - move implied y from Z to Y
    Y*=MY2;
    Z*=MY2;
}

//
// This is addition formula for two distinct points on an elliptic curve
// Works with projective coordinates which are automatically reduced wrt a 
// polynomial modulus
// Remember that the expression for the Y coordinate of each point 
// (a function of X) is implicitly multiplied by Y.
// We know Y^2=X^3+AX+B, but we don't have an expression for Y
// So if Y^2 ever crops up - substitute for it 

void elliptic_add(PolyMod& XT,PolyMod& YT,PolyMod& ZT,PolyMod& X,PolyMod& Y,PolyMod& Z)
{ // add (X,Y,Z) to (XT,YT,ZT) on an elliptic curve 

    PolyMod W1,W2,W3,W4,W5,W6;

    if (!isone(Z))
    { // slower if Z!=1
        W3=Z*Z;    // 1x
        W1=XT*W3;  // 2x
        W3*=Z;     // 3x
        W2=YT*W3;  // 4x
    }
    else
    {
        W1=XT;
        W2=YT;      // W2 has an implied y
    }
    W6=ZT*ZT;       // 1
    W4=X*W6;        // 2
    W6*=ZT;         // 3
    W5=Y*W6;        // 4 W5 has an implied y

    W1-=W4;
    W2-=W5;
    if (iszero(W1))
    {
        if (iszero(W2)) 
        { // should have doubled
            elliptic_dup(XT,YT,ZT);
            return;
        }
        else
        { // point at infinity
            ZT.clear();
            return;    
        }
    }

    W4=W1+2*W4;     
    W5=W2+2*W5;     

    ZT*=W1;       // 5

    if (!isone(Z)) ZT*=Z; // 5x

    W6=W1*W1;       // 6
    W1*=W6;         // 7
    W6*=W4;         // 8
    W4=MY2*(W2*W2);   // 9 Substitute for Y^2

    XT=W4-W6;

    W6=W6-2*XT;
    W2*=W6;        // 10
    W1*=W5;        // 11
    W5=W2-W1;

    YT=W5/(ZZn)2;   
}

//
//   Program to compute the order of a point on an elliptic curve
//   using Pollard's lambda method for catching kangaroos. 
//
//   As a way of counting points on an elliptic curve, this
//   has complexity O(p^(1/4))
//
//   However Schoof puts a spring in the step of the kangaroos
//   allowing them to make bigger jumps, and lowering overall complexity
//   to O(p^(1/4)/sqrt(L)) where L is the product of the Schoof primes
//
//   See "Monte Carlo Methods for Index Computation"
//   by J.M. Pollard in Math. Comp. Vol. 32 1978 pp 918-924
//
//   This code has been considerably speeded up using ideas from
//   "Parallel Collision Search with Cryptographic Applications", J. Crypto., 
//   Vol. 12, 1-28, 1999
//

#define STORE 80
#define HERD 5

ECn wild[STORE],tame[STORE];
Big wdist[STORE],tdist[STORE];
int wname[STORE],tname[STORE];

Big kangaroo(Big p,Big order,Big ordermod)
{
    ECn ZERO,K[2*HERD],TE[2*HERD],X,P,G,table[50],trap;
    Big start[2*HERD],txc,wxc,mean,leaps,upper,lower,middle,a,b,x,y,n,w,t,nrp;
    int i,jj,j,m,sp,nw,nt,cw,ct,k,distinguished;
    Big D[2*HERD],s,distance[50],real_order;
    BOOL bad,collision,abort;
    forever
    {

// find a random point on the curve
        do
        {
            x=rand(p);
        } while (!P.set(x,x));

        lower=p+1-2*sqrt(p)-3; // lower limit of search
        upper=p+1+2*sqrt(p)+3; // upper limit of search

        w=1+(upper-lower)/ordermod;
        leaps=sqrt(w);
        mean=HERD*leaps/2;      // ideal mean for set S=1/2*w^(0.5)
        distinguished=1<<(bits(leaps/16));
        for (s=1,m=1;;m++)
        { /* find table size */
            distance[m-1]=s*ordermod;
            s*=2;
            if ((2*s/m)>mean) break;
        }
        table[0]=ordermod*P;
        for (i=1;i<m;i++)
        { // double last entry
            table[i]=table[i-1];
            table[i]+=table[i-1];
        }

        middle=(upper+lower)/2;
        if (ordermod>1)
            middle+=(ordermod+order-middle%ordermod);

        for (i=0;i<HERD;i++) start[i]=middle+13*ordermod*i;
        for (i=0;i<HERD;i++) start[HERD+i]=13*ordermod*i;

        for (i=0;i<2*HERD;i++) 
        {
            K[i]=start[i]*P;         // on your marks
            D[i]=0;                  // distance counter
        }
        cout << "Releasing " << HERD << " Tame and " << HERD << " Wild Kangaroos\n";

        nt=0; nw=0; cw=0; ct=0;
        collision=FALSE;  abort=FALSE;
        forever
        { 
            for (jj=0;jj<HERD;jj++)
            {
                K[jj].get(txc);
                i=txc%m;  /* random function */

                if (txc%distinguished==0)
                {
                    if (nt>=STORE)
                    {
                        abort=TRUE;
                        break;
                    }
                    cout << "." << flush;
                    tame[nt]=K[jj];
                    tdist[nt]=D[jj];
                    tname[nt]=jj;
                    for (k=0;k<nw;k++)
                    {
                        if (wild[k]==tame[nt])
                        {
                            ct=nt; cw=k;
                            collision=TRUE;
                            break;
                        }
                    }
                    if (collision) break;
                    nt++;
                }
                D[jj]+=distance[i];
                TE[jj]=table[i];
            }
            if (collision || abort) break;
            for (jj=HERD;jj<2*HERD;jj++)
            {
                K[jj].get(wxc);
                j=wxc%m;

                if (wxc%distinguished==0)
                {
                    if (nw>=STORE)
                    {
                        abort=TRUE;
                        break;
                    }
                    cout << "." << flush;
                    wild[nw]=K[jj];
                    wdist[nw]=D[jj];
                    wname[nw]=jj;
                    for (k=0;k<nt;k++)
                    {
                        if (tame[k]==wild[nw]) 
                        {
                            ct=k; cw=nw;
                            collision=TRUE;
                            break;
                        }
                    }
                    if (collision) break;
                    nw++;
                }
                D[jj]+=distance[j];
                TE[jj]=table[j];
            }
            if (collision || abort) break;

            multi_add(2*HERD,TE,K);   // jump together - its faster
        }
        cout << endl;
        if (abort)
        {
            cout << "Failed - this should not happen! - trying again" << endl;
            continue;
        }
        nrp=start[tname[ct]]-start[wname[cw]]+tdist[ct]-wdist[cw];
        G=P;
        G*=nrp;
        if (G!=ZERO)
        {
            cout << "Sanity Check Failed. Please report to mike@compapp.dcu.ie" << endl;
            exit(0);
        }

		if (Edwards)
		{
			if (prime(nrp/4))
			{
				cout << "NP/4= " << nrp/4 << endl;
				cout << "NP/4 is Prime!" << endl;
				break;
			}
		}
		else
		{
			if (prime(nrp))
			{
				cout << "NP= " << nrp << endl;
				cout << "NP is Prime!" << endl;
				break;
			}
		}

// final checks....
        real_order=nrp; i=0; 
        forever
        {
            sp=get_mip()->PRIMES[i];
            if (sp==0) break;
            if (real_order%sp==0)
            {
                G=P;
                G*=(real_order/sp);
                if (G==ZERO) 
                {
                    real_order/=sp;
                    continue;
                }
            }
            i++;
        }   
        if (real_order <= 4*sqrt(p))
        { 
            cout << "Low Order point used - trying again" << endl;
            continue;
        }
        real_order=nrp;
        for (i=0;(sp=get_mip()->PRIMES[i])!=0;i++)
            while (real_order%sp==0) real_order/=sp;
        if (real_order==1)
        { // all factors of nrp were considered
            cout << "NP= " << nrp << endl;
            break;
        }
        if (prime(real_order))
        { // all factors of NP except for one last one....
            G=P;
            G*=(nrp/real_order);
            if (G==ZERO) 
            {
                cout << "Failed - trying again" << endl;
                continue;
            }
            else 
            {
                cout << "NP= " << nrp << endl;
                break;
            }
        }
// Couldn't be bothered factoring nrp completely
// Probably not an interesting curve for Cryptographic purposes anyway.....
// But if 20 random points are all "killed" by nrp, its almost
// certain to be the true NP, and not a multiple of a small order.

        bad=FALSE;
        for (i=0;i<20;i++)
        { 
            do
            {
                x=rand(p);
            } while (!P.set(x,x));
            G=P;
            G*=nrp;
            if (G!=ZERO)
            {
                bad=TRUE;
                break;
            }
        }
        if (bad)
        {
            cout << "Failed - trying again" << endl;
            continue;                       
        }
        cout << "NP is composite and not ideal for Cryptographic use" << endl;
        cout << "NP= " << nrp << " (probably)" << endl;
        break;
    }
    return nrp;
}

// Code to parse formula in command line
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

mr_utype qpow(mr_utype x,int y)
{ // quick and dirty power function
    mr_utype r=x;
    for (int i=1;i<y;i++) r*=x;
    return r;
}

int main(int argc,char **argv)
{
    ofstream ofile;
    int low,lower,ip,pbits,lp,i,j,jj,m,n,nl,L,k,tau,lambda;
    mr_utype t[100];
    Big a,b,p,nrp,x,y,d,s;
    PolyMod XX,XP,YP,XPP,YPP;
    PolyMod Pf[100],P2f[100],P3f[100];
    Poly G,P[100],P2[100],P3[100],Y2,Y4,Fl;
    miracl *mip=&precision;
    BOOL escape,search,fout,dir,gotP,gotA,gotB,eigen,anomalous;
    BOOL permisso[100];
    ZZn delta,j_invariant;
    ZZn EB,EA,T,T1,T3,A2,A4,AZ,AW;
    int Base; 

    argv++; argc--;
    if (argc<1)
    {
        cout << "Incorrect Usage" << endl;
        cout << "Program finds the number of points (NP) on an Elliptic curve" << endl;
        cout << "which is defined over the Galois field GF(P), P a prime" << endl;
        cout << "The Elliptic Curve has the equation Y^2 = X^3 + AX + B mod P" << endl;
		cout << "(Or use flag -E for Inverted Edwards coordinates X^2+AY^2=X^2.Y^2+B mod P)" << endl;		
        cout << "schoof <prime number P> <A> <B>" << endl;
        cout << "OR" << endl;
        cout << "schoof -f <formula for P> <A> <B>" << endl;
#if defined(unix)
        cout << "e.g. schoof -f 2^192-2^64-1 -3 35317045537" << endl;
#else
        cout << "e.g. schoof -f 2#192-2#64-1 -3 35317045537" << endl;
#endif
        cout << "To output to a file, use flag -o <filename>" << endl;
        cout << "To search downwards for a prime, use flag -d" << endl;
        cout << "To input P, A and B in Hex, precede with -h" << endl;
        cout << "To search for NP prime incrementing B, use flag -s" << endl;
		cout << "(For Edwards curve the search is for NP=4*prime)" << endl;
        cout << "\nFreeware from Certivox, Dublin, Ireland" << endl;
        cout << "Full C++ source code and MIRACL multiprecision library available" << endl;
        cout << "Also faster Schoof-Elkies-Atkin implementation" << endl;
        cout << "email mscott@indigo.ie" << endl;
        return 0;
    }

    ip=0;
    gprime(10000);                 // generate small primes < 1000
    search=fout=dir=gotP=gotA=gotB=FALSE;
    p=0; a=0; b=0;

// Interpret command line
    Base=10;
    while (ip<argc)
    {
        if (strcmp(argv[ip],"-f")==0)
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
        if (strcmp(argv[ip],"-o")==0)
        {
            ip++;
            if (ip<argc)
            {
                fout=TRUE;
                ofile.open(argv[ip++]);
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
        if (strcmp(argv[ip],"-E")==0)
        {
            ip++;
            Edwards=TRUE;
            continue;
        }

        if (strcmp(argv[ip],"-s")==0)
        {
            ip++;
            search=TRUE;
            continue;
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
        if (!gotA) 
        {
            mip->IOBASE=Base;
            a=argv[ip++];
            mip->IOBASE=10;
            gotA=TRUE;
            continue;
        }
        if (!gotB) 
        {
            mip->IOBASE=Base;
            b=argv[ip++];
            mip->IOBASE=10;
            gotB=TRUE;
            continue;
        }
        cout << "Error in command line" << endl;
        return 0;
    }    

    if (!gotP || !gotA || !gotB)
    {
        cout << "Error in command line" << endl;
        return 0;
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
    pbits=bits(p);
    cout << "P mod 24 = " << p%24 << endl;
    cout << "P is " << pbits << " bits long" << endl;

// loop for "-s" search option

    forever {

    fft_reset();   // reset FFT tables


	if (Edwards)
	{
		modulo(p);
		EB=b;
		EA=a;
		AZ=(ZZn)1/(EA-EB);
		A2=2*(EA+EB)/(EA-EB);
		A4=1; AW=1;

		AW*=AZ; A2*=AZ; A4*=AZ;

		A4*=AW;

		T=4*A2;
		T1=3*T;
		T3=18*36*(2*A4);

		A=T3-3*T1*T1;
		B=-T1*T3+2*T1*T1*T1;
		ecurve((Big)A,(Big)B,p,MR_AFFINE);    // initialise Elliptic Curve

	}
	else
	{
		ecurve(a,b,p,MR_AFFINE);    // initialise Elliptic Curve
		A=a;
		B=b;
	}

// The elliptic curve as a Polynomial

    Y2=0;
    Y2.addterm(B,0);
    Y2.addterm(A,1);
    Y2.addterm((ZZn)1,3);

    Y4=Y2*Y2;
    cout << "Counting the number of points (NP) on the curve" << endl;
	if (Edwards)
	{
		cout << "X^2+" << EA << "*Y^2=X^2*Y^2+" << EB << endl;
		cout << "Equivalent to Weierstrass form" << endl;
	}
    cout << "y^2= " << Y2 << " mod " << p << endl;   

    delta=-16*(4*A*A*A+27*B*B);
    if (delta==0)
    {
        cout << "Not Allowed! 4A^3+27B^2 = 0" << endl;
        if (search) {b+=1; continue; }
        else return 0;
    }
    anomalous=FALSE;
    j_invariant=(-1728*64*A*A*A)/delta;

    cout << "j-invariant= " << j_invariant << endl;

    if (j_invariant==0 || j_invariant==1728)
    {
        anomalous=TRUE;
        cout << "Warning: j-invariant is " << j_invariant << endl;
    }
    if (pbits<14)
    { // do it the simple way
        nrp=1;
        x=0;
        while (x<p)
        {
            nrp+=1+jacobi((x*x*x+(Big)A*x+(Big)B)%p,p);
            x+=1;
        }
		if (Edwards)
		{
			cout << "NP/4= " << nrp/4 << endl;
			if (prime(nrp/4)) cout << "NP/4 is Prime!" << endl;
			else if (search) {b+=1; continue; }
		}
		else
		{
			cout << "NP= " << nrp << endl;
			if (prime(nrp)) cout << "NP is Prime!" << endl;
			else if (search) {b+=1; continue; }
		}
        break;
    } 
    if (pbits<56) 
    { // do it with kangaroos
        nrp=kangaroo(p,(Big)0,(Big)1);
		if (Edwards)
		{
			if (!prime(nrp/4) && search) {b+=1; continue; }
		}
		else
		{
			if (!prime(nrp) && search) {b+=1; continue; }
		}
        break;
    }

    if (pbits<=100) d=pow((Big)2,48);
    if (pbits>100 && pbits<=120) d=pow((Big)2,56);
    if (pbits>120 && pbits<=140) d=pow((Big)2,64);
    if (pbits>140 && pbits<=200) d=pow((Big)2,72);
    if (pbits>200) d=pow((Big)2,80);
    
/*
    if (pbits<200) d=pow((Big)2,72);
    else           d=pow((Big)2,80);
*/

    d=sqrt(p/d);
    if (d<256) d=256;

    mr_utype l[100];
    int pp[100];   // primes and powers 

// see how many primes will be needed
// l[.] is the prime, pp[.] is the power

    for (s=1,nl=0;s<=d;nl++)
    {
        int tp=mip->PRIMES[nl];
        pp[nl]=1;     // every prime included once...
        s*=tp;
        for (i=0;i<nl;i++)
        { // if a new prime power is now in range, include its contribution
            int cp=mip->PRIMES[i];
            int p=qpow(cp,pp[i]+1);
            if (p<tp)
            { // new largest prime power
                s*=cp;
                pp[i]++;
            } 
        }
    }  
    L=mip->PRIMES[nl-1];

    cout << nl << " primes used (plus largest prime powers), largest is " << L << endl;

    for (i=0;i<nl;i++)
        l[i]=mip->PRIMES[i];

    int start_prime;   // start of primes & largest prime powers
    for (i=0;;i++)
    {
        if (pp[i]!=1)
        {
            mr_utype p=qpow(l[i],pp[i]);
            for (j=0;l[j]<p;j++) ;
            nl++;
            for (m=nl-1;m>j;m--) l[m]=l[m-1];
            l[j]=p;   // insert largest prime power in table
        }
        else 
        {
            start_prime=i;
            break;
        } 
    }

// table of primes and prime powers now looks like:-
// 2 3 5 7 9 11 13 16 17 19 ....
//       S p        p

// where S is start_prime, and p marks the largest prime powers in the range
// CRT uses primes starting from S, but small primes are kept in anyway, 
// as they allow quick abort if searching for prime NP.

// Calculate Divisor Polynomials - Schoof 1985 p.485
// Set the first few by hand....
    
    P[1]=1; P[2]=2; P[3]=0; P[4]=0;

    P2[1]=1; P3[1]=1;

    P2[2]=P[2]*P[2];
    P3[2]=P2[2]*P[2];

    P[3].addterm(-(A*A),0); P[3].addterm(12*B,1);
    P[3].addterm(6*A,2)   ; P[3].addterm((ZZn)3,4);

    P2[3]=P[3]*P[3];
    P3[3]=P2[3]*P[3];

    P[4].addterm((ZZn)(-4)*(8*B*B+A*A*A),0);
    P[4].addterm((ZZn)(-16)*(A*B),1);
    P[4].addterm((ZZn)(-20)*(A*A),2);
    P[4].addterm((ZZn)80*B,3);
    P[4].addterm((ZZn)20*A,4);
    P[4].addterm((ZZn)4,6);

    P2[4]=P[4]*P[4];
    P3[4]=P2[4]*P[4];

    lower=5;     // next one to be calculated

  // Finding the order modulo 2
  // If GCD(X^P-X,X^3+AX+B) == 1 , trace=1 mod 2, else trace=0 mod 2

    XX=0;
    XX.addterm((ZZn)1,1);

    setmod(Y2);
    XP=pow(XX,p);
    G=gcd(XP-XX);         
    t[0]=0;
    if (isone(G)) t[0]=1;
    cout << "NP mod 2 =   " << (p+1-(int)t[0])%2;
    if ((p+1-(int)t[0])%2==0)
    {                                  
        cout << " ***" << endl;
        if (search && !Edwards) {b+=1; continue; }
    }
    else cout << endl;

    PolyMod one,XT,YT,ZT,XL,YL,ZL,ZL2,ZT2,ZT3;
    one=1;                  // polynomial = 1
    
    Crt CRT(nl-start_prime,&l[start_prime]);  // initialise for application of the 
                                              // chinese remainder thereom

// now look for trace%prime for prime=3,5,7,11 etc
// actual trace is found by combining these via CRT

    escape=FALSE;
    for (i=1;i<nl;i++)      
    {
        lp=l[i];       // next prime
        k=p%lp;

// generation of Divisor polynomials as needed
// See Schoof p. 485

        for (j=lower;j<=lp+1;j++)
        { // different for even and odd 
            if (j%2==1)
            { 
                n=(j-1)/2;
                if (n%2==0)
                    P[j]=P[n+2]*P3[n]*Y4-P3[n+1]*P[n-1];
                else
                    P[j]=P[n+2]*P3[n]-Y4*P3[n+1]*P[n-1];
            }
            else
            {
                n=j/2;
                P[j]=P[n]*(P[n+2]*P2[n-1]-P[n-2]*P2[n+1])/(ZZn)2;
            }
            if (j <= 1+(L+1)/2)
            { // precalculate for later
                P2[j]=P[j]*P[j];
                P3[j]=P2[j]*P[j];
            }
        }

        if (lp+2>lower) lower=lp+2;

        for (tau=0;tau<=lp/2;tau++) permisso[tau]=TRUE;

        setmod(P[lp]);
        MY2=Y2;
        MY4=Y4;
// These next are time-consuming calculations of X^P, Y^P, X^(P*P) and Y^(P*P)

        cout << "X^P " << flush;
        XP=pow(XX,p);

// Eigenvalue search - see Menezes
// Batch the GCDs as they are slow. 
// This gives us product of both eigenvalues - a polynomial of degree (lp-1)
// But thats a lot better than (lp^2-1)/2
        eigen=FALSE;
        if (!anomalous && prime((Big)lp))
        {
            PolyMod Xcoord,batch;
            batch=1;
            cout << "\b\b\b\bGCD " << flush;
            for (tau=1;tau<=(lp-1)/2;tau++)
            {
                if (tau%2==0)
                    Xcoord=(XP-XX)*P2[tau]*MY2+(PolyMod)P[tau-1]*P[tau+1];
                else
                    Xcoord=(XP-XX)*P2[tau]+(PolyMod)P[tau-1]*P[tau+1]*MY2;
                batch*=Xcoord;     
            }
            Fl=gcd(batch);  // just one GCD!
            if (degree(Fl)==(lp-1)) eigen=TRUE;
        }

        if (eigen)
        {
            setmod(Fl);
            MY2=Y2;
            MY4=Y4;

//
// Only the Y-coordinate is calculated. No need for X^P !
//
            cout << "\b\b\b\bY^P" << flush;
            YP=pow(MY2,(p-1)/2);
            cout << "\b\b\b";

//
// Now looking for value of lambda which satisfies
// (X^P,Y^P) = lambda.(XX,YY). 
//
// Note that it appears to be sufficient to only compare the Y coordinates (!?)
//
            cout << "NP mod " << lp << " = " << flush;
            Pf[0]=0; P2f[0]=0; P3f[0]=0;
            Pf[1]=1; P2f[1]=1; P3f[1]=1;    
            low=2;
            for (lambda=1;lambda<=(lp-1)/2;lambda++)
            {
                int res=0;
                PolyMod Ry,Ty;
                tau=(lambda+invers(lambda,lp)*p)%lp;

                cout << setw(3) << (p+1-tau)%lp << flush; 

// Get Divisor Polynomials as needed - this time mod the new (small) modulus Fl

                for (jj=low;jj<=lambda+2;jj++)
                    Pf[jj]=(PolyMod)P[jj]; 
                if (lambda+3>low) low=lambda+3;

// compare Y-coordinates - 5 polynomial mod-muls required

                P2f[lambda+1]=Pf[lambda+1]*Pf[lambda+1];
                P3f[lambda]=P2f[lambda]*Pf[lambda];
                if (lambda%2==0)
                {
                    Ry=(Pf[lambda+2]*P2f[lambda-1]-Pf[lambda-2]*P2f[lambda+1])/4;
                    Ty=MY4*YP*P3f[lambda];
                }
                else
                {
                    if (lambda==1) Ry=(Pf[lambda+2]*P2f[lambda-1]+P2f[lambda+1])/4;
                    else           Ry=(Pf[lambda+2]*P2f[lambda-1]-Pf[lambda-2]*P2f[lambda+1])/4;
                    Ty=YP*P3f[lambda];
                }

                if (degree(gcd(Ty-Ry))!=0) res=1;
                if (degree(gcd(Ty+Ry))!=0) res=2;
                if (res!=0) 
                {  // has it doubled, or become point at infinity?
                    if (res==2)
                    { // it doubled - wrong sign
                        tau=(lp-tau)%lp;
                        cout << "\b\b\b";
                        cout << setw(3) << (p+1-tau)%lp << flush; 
                    }
                    t[i]=tau;
                    if ((p+1-tau)%lp==0)
                    {
                        cout << " ***" << endl;
                        if (search && (!Edwards || lp!=4)) escape=TRUE;
                    }
                    else cout << endl;
                    break;
                }
                cout << "\b\b\b";
            }
            for (jj=0;jj<low;jj++)
            {  
                Pf[jj].clear();
                P2f[jj].clear();
                P3f[jj].clear();
            }
            if (escape) break;
            continue;
        }

 // no eigenvalue found, but some tau values can be eliminated...

        if (!anomalous && prime((Big)lp))
        {
            if (degree(Fl)==0) 
            {
                for (tau=0;tau<=lp/2;tau++)
                {
                    jj=(lp+tau*tau-(4*p)%lp)%lp;
                    if (jac(jj,lp)!=(-1)) permisso[tau]=FALSE;
                }
            }
            else
            { // Fl==P[lp] so tau=+/- sqrt(p) mod lp
                jj=(int)(2*sqrmp((p%lp),lp))%lp;
                for (tau=0;tau<=lp/2;tau++) permisso[tau]=FALSE;
                if (jj<=lp/2) permisso[jj]=TRUE;
                else          permisso[lp-jj]=TRUE;
            }
        }
        if (!prime((Big)lp))
        { // prime power
            for (jj=0;jj<start_prime;jj++)
                if (lp%(int)l[jj]==0)
                {
                    for (tau=0;tau<=lp/2;tau++)
                    {
                        permisso[tau]=FALSE;
                        if (tau%(int)l[jj]==(int)t[jj])      permisso[tau]=TRUE;
                        if ((lp-tau)%(int)l[jj]==(int)t[jj]) permisso[tau]=TRUE;
                    }
                    break;
                }  
        }

        cout << "\b\b\b\bY^P " << flush;
        YP=pow(MY2,(p-1)/2);
        cout << "\b\b\b\bX^PP" << flush;
       

        if (lp<40)
            XPP=compose(XP,XP);               // This is faster!
        else XPP=pow(XP,p);

        cout << "\b\b\b\bY^PP" << flush;
        if (lp<40)
            YPP=YP*compose(YP,XP);            // This is faster!
        else YPP=pow(YP,p+1);
        cout << "\b\b\b\b";

        PolyMod Pk,P2k,PkP1,PkM1,PkP2;
        Pk=P[k]; PkP1=P[k+1]; PkM1=P[k-1]; PkP2=P[k+2];

        P2k=(Pk*Pk);
//
// This is Schoof's algorithm, stripped to its bare essentials
//
// Now looking for the value of tau which satisfies 
// (X^PP,Y^PP) + k.(X,Y) =  tau.(X^P,Y^P)
// 
// Note that (X,Y) are rational polynomial expressions for points on
// an elliptic curve, so "+" means elliptic curve point addition
// 
// k.(X,Y) can be found directly from Divisor polynomials
// Schoof Prop (2.2)
//
// Points are converted to projective (X,Y,Z) form
// This is faster (x2). Observe that (X/Z^2,Y/Z^3,1) is the same
// point in projective co-ordinates as (X,Y,Z)
//

        if (k%2==0)
        {
            XT=XX*MY2*P2k-PkM1*PkP1;
            YT=(PkP2*PkM1*PkM1-P[k-2]*PkP1*PkP1)/4;
            XT*=MY2;      // fix up, so that Y has implicit y multiplier
            YT*=MY2;      // rather than Z
            ZT=MY2*Pk;
        }
        else
        {
            XT=(XX*P2k-MY2*PkM1*PkP1);
            if (k==1) YT=(PkP2*PkM1*PkM1+PkP1*PkP1)/4;
            else      YT=(PkP2*PkM1*PkM1-P[k-2]*PkP1*PkP1)/4;
            ZT=Pk;
        }

        elliptic_add(XT,YT,ZT,XPP,YPP,one);
// 
// Test for Schoof's case 1 - LHS (XT,YT,ZT) is point at infinity
//

        cout << "NP mod " << lp << " = " << flush;
        if (iszero(ZT))
        { // Is it zero point? (XPP,YPP) = - K(X,Y)
            t[i]=0;
            cout << setw(3) << (p+1)%lp;
            if ((p+1)%lp==0)
            {      
                cout << " ***" << endl;
                if (search && (!Edwards || lp!=4)) {escape=TRUE; break;}
            }
            else cout << endl;
            continue;         
        }

 // try all candidates one after the other

        PolyMod XP2,XP3,XP4,XP6,YP2,YP4;
        PolyMod ZT2XP,ZT2YP2,XPYP2,XTYP2,ZT3YP;

        ZT2=ZT*ZT;
        ZT3=ZT2*ZT;
        XP2=XP*XP;
        XP3=XP*XP2;
        XP4=XP2*XP2;
        XP6=XP3*XP3;
        YP2=MY2*(YP*YP);
        YP4=YP2*YP2;

        ZT2XP=ZT2*XP; ZT2YP2=ZT2*YP2; XPYP2=XP*YP2; XTYP2=XT*YP2; ZT3YP=ZT3*YP;

        Pf[0]=0; Pf[1]=1; Pf[2]=2;
        P2f[1]=1; P3f[1]=1;
        P2f[2]=Pf[2]*Pf[2];
        P3f[2]=P2f[2]*Pf[2];
    
        Pf[3]=3*XP4+6*A*XP2+12*B*XP-A*A;
        P2f[3]=Pf[3]*Pf[3];
        P3f[3]=P2f[3]*Pf[3];

        Pf[4]=(4*XP6+20*A*XP4+80*B*XP3-20*A*A*XP2-16*A*B*XP-32*B*B-4*A*A*A);
        P2f[4]=Pf[4]*Pf[4];
        P3f[4]=P2f[4]*Pf[4];
        low=5;

        for (tau=1;tau<=lp/2;tau++)
        {
            int res=0;
            PolyMod Rx,Tx,Ry,Ty;
            if (!permisso[tau]) continue;

            cout << setw(3) << (p+1-tau)%lp << flush;     

            for (jj=low;jj<=tau+2;jj++)
            { // different for odd and even
                if (jj%2==1)
                { /* 3 mod-muls */
                    n=(jj-1)/2;
                    if (n%2==0)
                       Pf[jj]=Pf[n+2]*P3f[n]*YP4-P3f[n+1]*Pf[n-1];
                    else
                       Pf[jj]=Pf[n+2]*P3f[n]-YP4*P3f[n+1]*Pf[n-1];
                }
                else
                { /* 3 mod-muls */
                    n=jj/2;
                    Pf[jj]=Pf[n]*(Pf[n+2]*P2f[n-1]-Pf[n-2]*P2f[n+1])/(ZZn)2;
                }
                P2f[jj]=Pf[jj]*Pf[jj];                            // square
                if (jj<=1+(1+(lp/2))/2) P3f[jj]=P2f[jj]*Pf[jj];   // cube
            } 
            if (tau+3>low) low=tau+3;

            if (tau%2==0)
            { // 4 mod-muls
                Rx=ZT2*(XPYP2*P2f[tau]-Pf[tau-1]*Pf[tau+1]);
                Tx=XTYP2*P2f[tau];
            }
            else
            { // 4 mod-muls
                Rx=(ZT2XP*P2f[tau]-ZT2YP2*Pf[tau-1]*Pf[tau+1]);
                Tx=XT*P2f[tau];
            }
            if (iszero(Rx-Tx))
            { // we have a result. Now compare Y's
                if (tau%2==0)
                {
                    Ry=ZT3YP*(Pf[tau+2]*P2f[tau-1]-Pf[tau-2]*P2f[tau+1]);
                    Ty=4*YT*YP4*P2f[tau]*Pf[tau];
                }
                else
                {
                    if (tau==1) Ry=ZT3YP*(Pf[tau+2]*P2f[tau-1]+P2f[tau+1]);
                    else        Ry=ZT3YP*(Pf[tau+2]*P2f[tau-1]-Pf[tau-2]*P2f[tau+1]);
                    Ty=4*YT*P2f[tau]*Pf[tau];
                }

                if (iszero(Ry-Ty)) res=1;
                else               res=2;
            }

            if (res!=0) 
            {  // has it doubled, or become point at infinity?
                if (res==2)
                { // it doubled - wrong sign
                    tau=lp-tau;
                    cout << "\b\b\b";
                    cout << setw(3) << (p+1-tau)%lp << flush; 
                }
                t[i]=tau;
                if ((p+1-tau)%lp==0)
                {
                    cout << " ***" << endl;
                    if (search && (!Edwards || lp!=4)) escape=TRUE;
                }
                else cout << endl;
                break;
            }
            cout << "\b\b\b";
        }
        for (jj=0;jj<low;jj++)
        {
            Pf[jj].clear();
            P2f[jj].clear();
            P3f[jj].clear();
        }
        if (escape) break;
    }
    Modulus.clear();

    for (i=0;i<=L+1;i++) 
    {
        P[i].clear(); // reclaim space
        P2[i].clear();
        P3[i].clear();
    }

    if (escape) {b+=1; continue;}
    Big order,ordermod;
    ordermod=1; for (i=0;i<nl-start_prime;i++) ordermod*=(int)l[start_prime+i];
    order=(p+1-CRT.eval(&t[start_prime]))%ordermod;    // get order mod product of primes

    nrp=kangaroo(p,order,ordermod);

	if (Edwards)
	{
		if (!prime(nrp/4) && search) {b+=1; continue; }
		else break;
	}
	else
	{
		if (!prime(nrp) && search) {b+=1; continue; }
		else break;
    }
	}
    if (fout) 
    {
        ECn P;
        ofile << bits(p) << endl;
        mip->IOBASE=16;
        ofile << p << endl;

        ofile << a << endl;
        ofile << b << endl;
    // generate a random point on the curve 
    // point will be of prime order for "ideal" curve, otherwise any point
		if (!Edwards)
		{
			do {
				x=rand(p);
			} while (!P.set(x,x));
			P.get(x,y);
			ofile << nrp << endl;
		}
		else
		{
			ZZn X,Y,Z,R,TA,TB,TC,TD,TE;
			forever
			{
				X=randn();
				R=(X*X-EB)/(X*X-EA);
				if (!qr(R))continue;
				Y=sqrt(R);
				break;
			}
			Z=1;
// double point twice (4*P)
			for (i=0;i<2;i++)
			{
				TA = X*X;
				TB = Y*Y;
				TC = TA+TB;
				TD = TA-TB;
				TE = (X+Y)*(X+Y)-TC;
			
				X = TC*TD;
				Y = TE*(TC-2*EB*Z*Z);
				Z = TD*TE;
			}
			X/=Z;
			Y/=Z;
			x=X;
			y=Y;
			ofile << nrp/4 << endl;
		}
		ofile << x << endl;
		ofile << y << endl;
        mip->IOBASE=10;
    }
    if (p==nrp) 
    {
        cout << "WARNING: Curve is anomalous" << endl;
        return 0;
    }

    if (p+1==nrp)
    {
        cout << "WARNING: Curve is supersingular" << endl;
    }

// check MOV condition for curves of Cryptographic interest
// if (pbits<128) return 0;

    d=1;
    for (i=1;i<50;i++)
    {
        d=modmult(d,p,nrp);
        if (d==1) 
        {
           if (i==1 || prime(nrp)) cout << "WARNING: Curve fails MOV condition - K = " << i << endl;
           else                    cout << "WARNING: Curve fails MOV condition - K <= " << i << endl;

           return 0;
        }
    }
    
    return 0;
}


