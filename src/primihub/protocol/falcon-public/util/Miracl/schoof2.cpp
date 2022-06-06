// Schoof's Original Algorithm - GF(2^m) Version!
// Mike Scott March 2000   mike@compapp.dcu.ie
//
// Counts points on GF(2^m) Elliptic Curve, y^2+xy = x^3+Ax^2+B a prerequisite
// for implementation of Elliptic Curve Cryptography
//
// The self-contained Windows Command Prompt executable for this program may 
// obtained from ftp://ftp.computing.dcu.ie/pub/crypto/schoof2.exe 
//
// Basic algorithm is due to Schoof
// "Elliptic Curves Over Finite Fields and the Computation of Square Roots 
// mod p", Rene Schoof, Math. Comp., Vol. 44 pp 483-494
// Another very useful reference particularly for GF(2^m) curves is
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
// This approach is only feasible due to the use of fast multiplication
// methods for large degree polynomial multiplication
// **
//
// Ref "Monte Carlo Methods for Index Computation"
// by J.M. Pollard in Math. Comp. Vol. 32 1978 pp 918-924
//
// An "ideal" curve is defined as one with with 
// (i)  2*(a prime) number of points if trace(A)=1, or
// (ii) 4*(a prime) number of points if trace(A)=0.
//
// Note that the number of points on the curve is invariant if we substitute 
// B by B^2 (this is trivial, but I haven't seen it anywhere).
//
// Proof: if (x,y) is a point on 
// y^2+xy=x^3+Ax^2+B
// Then clearly (X,Y) = (x^2,y^2) is a point on
// y^4+x^2.y^2=x^6+Ax^4+B^2  (square both sides)
//
// so Y^2+XY=X^3+AX^2+B^2
//
// When using the "schoof2" program, the -s option is particularly useful
// and allows automatic search for an "ideal" curve. If a curve order is
// exactly divisible by a small prime >2, that curve is immediately abandoned, 
// and the program moves on to the next, incrementing the B parameter of 
// the curve. This is a fairly arbitrary but simple way of moving on to 
// the "next" curve (but note the point made above about B -> B^2). 
//
// NOTE: The output file can be used directly with for example the ECDSA
// programs IF AND ONLY IF an ideal curve is found. If you wish to use
// a less-than-ideal curve, you will first have to factor NP completely, and
// find a random point of large prime order. 
//
// This implementation is free. No terms, no conditions. It requires 
// version 4.30 or greater of the MIRACL library (a Shareware, Commercial 
// product, but free for non-profit making use), 
// available from ftp://ftp.computing.dcu.ie/pub/crypto/miracl.zip 
//
// However this program may be used (unmodified) to generate curves for 
// commercial use.
//
// Revision history 
//
// Timings for test curve Y^2+XY=X^3+X^2+52 mod 2^191
// Pentium III 450MHz
//
// Rev. 0 - 45 minutes
// Rev. 1 - 41 minutes
// Rev. 2 - 36 minutes
//
// Note that a small speed-up can be obtained by using an integer-only 
// build of MIRACL. See mirdef.hio
//
// Pentium III 450MHz
//
// GF(2^191)  - 36 minutes
// GF(2^223)  - 1.9 hours
// GF(2^251)  - 4   hours
// GF(2^283)  - 12.3 hours
//
//

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>
#include "ec2.h"         // GF(2^m) Elliptic Curve Class
#include "crt.h"         // Chinese Remainder Theorem Class

// poly2.h implements polynomial arithmetic. Karatsuba multiplication is 
// used for maximum speed, as the polynomials get very big. For example when 
// searching for the curve cardinality mod the prime 31, the polynomials are 
// of degree (31*31-1)/2 = 480. But all that gruesome detail is hidden away.
//
// poly2mod.h implements polynomial arithmetic wrt to a preset polynomial 
// modulus. This looks a bit neater. Function setmod() sets the modulus 
// to be used.

#include "poly2.h"
#include "poly2mod.h"

using namespace std;

Miracl precision=12;            // max. 12x32 bits per big number

Poly2Mod MFX,XX;

Big B;
Big D;
Big A;

void elliptic_dup(Poly2Mod& X,Poly2Mod& Y,Poly2Mod& Yy,Poly2Mod& Z)
{ // point doubling
    Poly2Mod W1,W2,W3,W4,W5y,W5;

    W1=(Z*Z);   // Z^2
    W2=X*X;     // X^2
    W3=W2*W2;   // X^4

    W4=X*W1;    // =Z

    X=(X+D*W1);
    X=X*X;
    X=X*X;

    W5=W4+W2+Y*Z;
    W5y=Yy*Z;

    Z=W4;
    
    Y=W3*Z+W5*X;
    Yy=W5y*X;
}

//
// This is addition formula for two distinct points on an elliptic curve
// Works with projective coordinates which are automatically reduced wrt a 
// polynomial modulus
// Remember that the expression for the Y coordinate of each point 
// is of the form A(x)+B(x).y
// We know Y^2=X^3+AX^2+B+XY, but we don't have an explicit expression for Y
// So if Y^2 ever crops up - substitute for it. 

void elliptic_add(Poly2Mod& XT,Poly2Mod& XTy,Poly2Mod& YT,Poly2Mod& YTy,Poly2Mod& ZT,Poly2Mod& X,Poly2Mod& Y,Poly2Mod& Yy)
{ // add (X,Y,Z) to (XT,YT,ZT) on an elliptic curve 
  // The point Y is of the form A(x)+B(x).y

    Poly2Mod ZT2,ZT3,W1,W2,W3,W4,W5,W5y,W6,W6y,W7,W8,W8y,W9,W9y;

    ZT2=(ZT*ZT);

    W3=XT+X*ZT2;

    ZT3=ZT2*ZT;
    W5=ZT3*Y;
    W5y=ZT3*Yy;

    W6=YT+W5;
    W6y=YTy+W5y;

    if (iszero(W3))
    {
        if (iszero(W6) && iszero(W6y))
        { // should have doubled!
            elliptic_dup(XT,YT,YTy,ZT);
            XTy=0;
            return;
        }

        ZT.clear();
        return;
    }


    ZT*=W3;

    W8=W6*X+ZT*Y;
    W8y=W6y*X+ZT*Yy;

    W9=W6+ZT;

    W1=ZT*ZT;
    W2=W6y*W6y;
    
    XT=A*W1+W3*W3*W3+W6*W9+W2*MFX;
    XTy=W9*W6y+W6y*W6+W2*XX;

    W2=W6y*XTy;

    YT=W9*XT+W2*MFX+W8*W1;
    YTy=XT*W6y+W9*XTy+W2*XX+W1*W8y;
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

EC2 wild[STORE],tame[STORE];
Big wdist[STORE],tdist[STORE];
int wname[STORE],tname[STORE];

Big kangaroo(Big p,Big order,Big ordermod,int TR,BOOL &found)
{
    EC2 ZERO,K[2*HERD],TE[2*HERD],X,P,G,table[50],trap;
    Big start[2*HERD],txc,wxc,mean,leaps,upper,lower,middle,a,b,x,y,n,w,t,nrp;
    int i,jj,j,m,sp,nw,nt,cw,ct,k,distinguished;
    Big D[2*HERD],s,distance[50],real_order;
    BOOL bad,collision,abort;

    found=FALSE;
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

        distinguished++;        // powers of 2 create "resonances" with 2^m
                                // so bump it up to odd.
    
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
        if (G!=ZERO || nrp%2==1)
        {
            cout << "Sanity Check 1 Failed. Please report to mike@compapp.dcu.ie" << endl;
            exit(0);
        }

        if (prime(nrp/2))
        {
            cout << "NP is 2*Prime!" << endl;
            cout << "NP= 2*" << nrp/2 << endl;
            found=TRUE;
            break;
        }
        if (nrp%4==0)
        {
            if (prime(nrp/4) && TR==0)
            {
                cout << "NP is 4*Prime!" << endl;
                cout << "NP= 4*" << nrp/4 << endl;
                found=TRUE;
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

int qpow(int x,int y)
{ // quick and dirty power function
    int r=x;
    for (int i=1;i<y;i++) r*=x;
    return r;
}

int main(int argc,char **argv)
{
    ofstream ofile;
    int TR,M,a,b,c,low,lower,ip,lp,i,j,jj,m,n,nl,L,k,tau,lambda,cf;
    mr_utype t[100];
    Big aa,bb,p,nrp,x,y,d,s;
    Poly2Mod X2,XP,YP,YPy,XP2,XPP,YPP,YPPy,TT,SX,XK[600];
    Poly2Mod Pf[600],P2f[600],P3f[600];
    Poly2 Fl,X,G,P[100],P2[100],P3[100],FX,Y4;
    EC2 GG;
    miracl *mip=&precision;
    BOOL eigen,found,escape,search,fout,gotM,gotA,gotB,gota,gotb,gotc;
    static BOOL permisso[600];
    int Base;

    argv++; argc--;
    if (argc<1)
    {
        cout << "Incorrect Usage" << endl;
        cout << "Program finds the number of points (NP) on an Elliptic curve" << endl;
        cout << "which is defined over the Galois field GF(2^M)" << endl;
        cout << "The Elliptic Curve has the equation Y^2 +XY = X^3 + AX^2 + B " << endl;
        cout << "A suitable trinomial or Pentanomial basis must" << endl;
        cout << "also be specified t^m+t^a+1 or t^m+t^a+t^b+t^c+1" << endl;
        cout << "These can be found in e.g. the IEEE P1363 standard" << endl;
        cout << "or can be generated using the MIRACL findbase example program" << endl;
        cout << "schoof2 <A> <B> <M> <a> <b> <c>" << endl << endl;
        cout << "e.g. schoof2 1 52 191 9" << endl << endl;
        cout << "To input A and B in Hex, precede with -h" << endl;
        cout << "To output to a file, use flag -o <filename>" << endl;
        cout << "To search for NP a near-prime, incrementing B, use flag -s" << endl;
        cout << "\nFreeware from Certivox, Dublin, Ireland" << endl;
        cout << "Full C++ source code and MIRACL multiprecision library available" << endl;
        cout << "email mscott@indigo.ie" << endl;
        return 0;
    }

    ip=0;
    gprime(10000);                 // generate small primes < 1000
    search=fout=gotM=gotA=gotB=gota=gotb=gotc=FALSE;
    M=a=b=c=0;

// Interpret command line
    Base=10;
    while (ip<argc)
    {
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
        if (!gotA) 
        {
            mip->IOBASE=Base;
            aa=argv[ip++]; 
            mip->IOBASE=10;
            gotA=TRUE;
            continue;
        }
        if (!gotB) 
        {
            mip->IOBASE=Base;
            bb=argv[ip++];
            mip->IOBASE=10;
            gotB=TRUE;
            continue;
        }
        if (!gotM) 
        {
            M=atoi(argv[ip++]);
            gotM=TRUE;
            continue;
        }
        if (!gota)
        {
            a=atoi(argv[ip++]);
            gota=TRUE;
            continue;
        } 
        if (!gotb)
        {
            b=atoi(argv[ip++]);
            gotb=TRUE;
            continue;
        } 
        if (!gotc)
        {
            c=atoi(argv[ip++]);
            gotc=TRUE;
            continue;
        } 
        cout << "Error in command line" << endl;
        return 0;
    }    

    if ((!gotM || !gotA || !gotB) || a==0)
    {
        cout << "Error in command line" << endl;
        return 0;
    }

// loop for "-s" search option
    p=pow((Big)2,M);
    forever {
 
    A=aa;
    if (bb>=p) bb%=p;
  
    B=bb;
    if (!ecurve2(M,a,b,c,A,B,TRUE,MR_AFFINE))    // initialise Elliptic Curve
    {
        cout << "Illegal Curve Parameters" << endl;
        return 0;
    }

//    D=mip->C;
    GF2m At=B;
    for (i=1;i<M-1;i++) At*=At;
    D=At;

    At=A;
    TR=trace(At);

// The elliptic curve as a Polynomial

    FX=0;
    FX.addterm(B,0);
    FX.addterm((GF2m)A,2);

    FX.addterm((GF2m)1,3);

    X=0; X.addterm(1,1);

    cout << "Counting the number of points (NP) on the curve" << endl;
    cout << "y^2 + xy = " << FX << " mod 2^" << M << endl;   

    if (B==0)
    {
        cout << "Not Allowed! B = 0" << endl;
        if (search) {bb+=1; continue; }
        else return 0;
    }

    if (M<12)
    { // do it the simple way
        nrp=2;
        x=1;
        while (x< (1<<M))
        { /* check if two points exist for each x */
            if (GG.set(x,0)) nrp+=2;
            x+=1;
        }
 
        do 
        {
            x=rand(p);
        } while (!GG.set(x,x));

        GG*=nrp;
        if (!GG.iszero())
        {
            cout << "Sanity Check 2 Failed. Please report to mike@compapp.dcu.ie" << endl;
            exit(0);
        }

        if (prime(nrp/2))
        {
            cout << "NP is 2*Prime!" << endl;
            cout << "NP= 2*" << nrp/2 << endl;
            break;
        }
        if (nrp%4==0)
        {
            if (A==0 && prime(nrp/4))
            {
                cout << "NP is 4*Prime!" << endl;
                cout << "NP= 4*" << nrp/4 << endl;
                break;
            }
        }
        cout << "NP= " << nrp << endl;
        if (search) {bb+=1; continue; }

        break;
        
    } 
    else if (M<56) 
    {
        nrp=kangaroo(p,(Big)0,(Big)2,TR,found);
        if (found) break;
        if (search) {bb+=1; continue; }
        break;
    }

    if (M<=100) d=pow((Big)2,48);  
    if (M>100 && M<=120) d=pow((Big)2,56); 
    if (M>120 && M<=160) d=pow((Big)2,64); 
    if (M>160 && M<=200) d=pow((Big)2,72);
    if (M>200) d=pow((Big)2,80);
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
            if (p<tp  || (cp==2 && p<16*tp) )
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
            int p=qpow(l[i],pp[i]);
            for (j=0;l[j]<p && j<nl;j++) ;
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
//     S   p        p

// where S is start_prime, and p marks the largest prime powers in the range
// CRT uses primes starting from S, but small primes are kept in anyway, 
// as they allow quick abort if searching for prime NP.

// Calculate Divisor Polynomials 
// Set the first few by hand....

    P[0]=0; P[1]=1; P[2]=0; P[3]=0; P[4]=0;

    P2[1]=1; P3[1]=1;

    P[2].addterm(1,1);

    P2[2]=P[2]*P[2];
    P3[2]=P2[2]*P[2];

    P[3].addterm(B,0);   
    P[3].addterm(1,3);
    P[3].addterm(1,4);  

    P2[3]=P[3]*P[3];
    P3[3]=P2[3]*P[3];

    P[4].addterm(B,2);
    P[4].addterm(1,6);

    P2[4]=P[4]*P[4];
    P3[4]=P2[4]*P[4];

    lower=5;     // next one to be calculated

    t[0]=0;

    Poly2Mod zero,one,XT,XTy,YT,YTy,ZT,XL,YL,ZL,ZL2,ZT2;
    one=1;                  // polynomial = 1
    zero=0;                 // polynomial = 0
    
    Crt CRT(nl-start_prime,&l[start_prime]);  // initialise for application of 
                                              // chinese remainder thereom

// now look for trace%prime for prime=3,5,7,11 etc
// actual trace is found by combining these via CRT

    l[0]=4;
    if (TR==0) l[0]=8;

    escape=FALSE;
    for (i=0;i<nl;i++)      
    {
        lp=l[i];       // next prime
        k=p%lp;

// generation of Divisor polynomials as needed
// See Schoof p. 485

        if (lp<=8 || lp%2!=0)
        {
            for (j=lower;j<=lp+1;j++)
            { // different for even and odd 
                if (j%2==1)
                { 
                    n=(j-1)/2;
                    P[j]=P[n+2]*P3[n]+P3[n+1]*P[n-1];
                }
                else
                {
                    n=j/2;
                    P[j]=P[n]*(P[n+2]*P2[n-1]+P[n-2]*P2[n+1]);
                    P[j]=divxn(P[j],1);
                }
                if (j <= 1+(L+1)/2)
                { // precalculate for later
                    P2[j]=P[j]*P[j];
                    P3[j]=P2[j]*P[j];
                }
            }

            if (lp+2>lower) lower=lp+2;
        }    
        for (tau=0;tau<=lp/2;tau++) permisso[tau]=TRUE;

        eigen=FALSE;
        if (lp%2==0)
        { // its 2^c
            Poly2 G,G2,PI;
            GF2m S;
            int t=lp,c=0;
            while (t!=1) {t/=2; c++;}

//
// When lp is a power of 2, we can construct a root of the Division Polynomial
// directly. See Menezes P.107. This is much quicker. Then the eigenvalue
// heuristic can be used.
//

            S=sqrt(sqrt((GF2m)B));
            G=X+S;
            G2=G*G;
            PI=1;

            for (jj=2;jj<c;jj++)
            {
                S=sqrt(S);
                G=G2+S*X*PI;
                PI=PI*G2;
                G2=G*G;
            }  
            eigen=TRUE;
            Fl=G;          // G is a root of degree lp/4
        }
        else
        {
            Poly2Mod Xcoord,batch;
            setmod(P[lp]);
            MFX=FX;
            XX=X;
            XP2=1;
  
//
// Calculate X^P. Save intermediates, as they can be used to
// calculate Y^P (if we need to.. )
//

            cout << "X^P " << flush;
            for (jj=0;jj<M-1;jj++)
            {
                XP2*=XX;
                XP2*=XP2;
                XK[jj]=XP2;
            }
            XP=XP2*(XX*XX);

// Eigenvalue search - see Menezes
// Batch the GCDs as they are slow. 
// This gives us product of both eigenvalues - a polynomial of degree (lp-1)
// But thats a lot better than (lp^2-1)/2

            if (prime((Big)lp))
            {
                batch=1;
                cout << "\b\b\b\bGCD " << flush;  
                for (tau=1;tau<=(lp-1)/2;tau++)
                {
                    Xcoord=(XP+XX)*P2[tau]+(Poly2Mod)P[tau-1]*P[tau+1];
                    batch*=Xcoord;
                }
                Fl=gcd(batch);
                if (degree(Fl)==(lp-1)) eigen=TRUE;
            }
            cout << "\b\b\b\b";
        }   

        if (eigen)
        { // eigenvalue found!
            Poly2Mod one;
            setmod(Fl);
            one=1;
            MFX=FX;
            XX=X;
            YP=MFX;
            XP2=1;

// Find X^P and Y^P together wrt new small modulus

            cout << "Y^P" << flush;

            for (jj=0;jj<M-1;jj++)
            {
                XP2*=XX;
                XP2*=XP2;
                YP*=YP;
                YP+=(XP2*MFX);
            }
            YPy=XP2*XX;
            XP=YPy*XX;
            cout << "\b\b\b";

            cout << "NP mod " << lp << " = " << flush;
            SX=inverse(XX);
            for (jj=0;jj<5;jj++)
            {
                Pf[jj]=(Poly2Mod)P[jj];
                P2f[jj]=(Poly2Mod)P2[jj];
                P3f[jj]=(Poly2Mod)P3[jj];
            }         

// if (lp%2==0) cout << "\n GCD(XX,Fl)= " << gcd(XX) << endl;

            low=5;
            for (lambda=1;lambda<=lp/2;lambda++)
            { // eigenvalue search
                Poly2Mod Tx,Hx,Ax,Bx,Pf3,Pft;
                tau=(lambda+invers(lambda,lp)*p)%lp;

                cout << setw(4) << (p+1-tau)%lp << flush;
                
                for (jj=low;jj<=lambda+2;jj++)
                { 
                    if (jj%2==1)
                    {
                        n=(jj-1)/2;
                        Pf[jj]=Pf[n+2]*P3f[n]+P3f[n+1]*Pf[n-1];
                    }
                    else
                    {
                        n=jj/2;
                        Pf[jj]=SX*Pf[n]*(Pf[n+2]*P2f[n-1]+Pf[n-2]*P2f[n+1]);
                    }
                    P2f[jj]=Pf[jj]*Pf[jj];
                    P3f[jj]=P2f[jj]*Pf[jj];
                }
                if (lambda+3>low) low=lambda+3;

                Pft=Pf[lambda-1]*Pf[lambda+1];

                Tx=(XP+XX)*P2f[lambda]+Pft;

                if (degree(gcd(Tx))!=0)
                { // Got it! Now check Y-coord for correct sign
                    if (lambda==1)
                    {
                        Ax=YP;
                        Bx=YPy+one;
                    }
                    else
                    {
                        Pf3=P3f[lambda];
                        Pft*=Pf[lambda];

                        Ax=XX*Pf3*(YP+XX)+Pf[lambda-2]*P2f[lambda+1]+(XX*XX+XX)*Pft;  
                        Bx=XX*Pf3*(YPy+one)+Pft; 
                    }              

                    Hx=Ax*Ax+XX*Ax*Bx+MFX*(Bx*Bx); // substitue y into curve

                    if (degree(gcd(Hx))==0)
                    { // its the other one
                        tau=(lp-tau)%lp;
                        cout << "\b\b\b\b";
                        cout << setw(4) << (p+1-tau)%lp << flush;
                    }
                    t[i]=tau;
                    if ((p+1-tau)%lp==0)
                    {
                        cout << " ***" << endl;
                        if (search) escape=TRUE;
                    }
                    else cout << endl;
                    break;
                }
                cout << "\b\b\b\b";
            }
            for (jj=0;jj<low;jj++)
            {
                Pf[jj].clear();
                P2f[jj].clear();
                P2f[jj].clear();
            }  
            if (escape) break;
            continue;
        }   

 // no eigenvalue found, but some tau values can be eliminated...       

        if (prime((Big)lp))
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
                jj=(2*sqrmp(p%lp,lp))%lp;
                for (tau=0;tau<=lp/2;tau++) permisso[tau]=FALSE;
                if (jj<=lp/2) permisso[jj]=TRUE;
                else          permisso[lp-jj]=TRUE;
            }
        }
        else
        { // prime power
            for (jj=0;jj<start_prime;jj++)
                if (lp%l[jj]==0)
                {
                    for (tau=0;tau<=lp/2;tau++)
                    {
                        permisso[tau]=FALSE;
                        if (tau%l[jj]==t[jj])      permisso[tau]=TRUE;
                        if ((lp-tau)%l[jj]==t[jj]) permisso[tau]=TRUE;
                    }
                    break;
                }  
        }

// These next are time-consuming calculations of Y^P, X^PP and Y^PP

        cout << "Y^P " << flush;
        YP=MFX;
        for (jj=0;jj<M-1;jj++)
        {
            XP2=XK[jj];     // use values stored during generation of X^P
            XK[jj].clear();
            YP*=YP;
            YP+=(XP2*MFX);
        }
        YPy=XP2*XX;
        cout << "\b\b\b\bX^PP" << flush;

// Composition is faster for smaller lp

        if (lp<40)
        {
            TT=compose(YPy,XP);
            XPP=XP*TT;
            cout << "\b\b\b\bY^PP" << flush;
            YPP=compose(YP,XP)+YP*TT;
            YPPy=TT*YPy;
        }
        else
        { // XPP and YPP are calculated together
            YPP=YP;
            for (jj=0;jj<M;jj++)
            {
                XP2*=XX;
                XP2*=XP2;
                YPP*=YPP;
                YPP+=(XP2*MFX);  
                if (jj==M/2) cout << "\b\b\b\bY^PP" << flush;
            }
            YPPy=XP2*XX;
            XPP=YPPy*XX;
        }
        cout << "\b\b\b\b";

        Poly2Mod Pk,P2k,P3k,PkP1,PkM1,PkP2,Pt;
        Pk=P[k]; PkP1=P[k+1]; PkM1=P[k-1]; PkP2=P[k+2];

//
// This is Schoof's algorithm
//
// Now looking for the value of tau which satisfies 
// (X^PP,Y^PP) + k.(X,Y) =  tau.(X^P,Y^P)
// 
// Note that (X,Y) are rational polynomial expressions for points on
// an elliptic curve, so "+" means elliptic curve point addition
// 
// Note also that Y is of the form A(x)+B(x).y. After the addition
// the X coordinate of the sum will also be of this form.
//
// k.(X,Y) can be found directly from Divisor polynomials
// Schoof Prop (2.2)
//
// Points are converted to projective (X,Y,Z) form
// Observe that (X/Z^2,Y/Z^3,1) is the same
// point in projective co-ordinates as (X,Y,Z)
//

        if (k==1)
        { // easy case
            XT=XX;
            YT=0;
            YTy=one;
            ZT=one;
        }
        else
        {
            P2k=Pk*Pk;
            P3k=P2k*Pk;
            Pt=PkP1*PkM1;
            X2=XX*XX;
            XT=X2*(XX*P2k+Pt);
            Pt*=Pk;
            YT=X2*(X2*P3k+P[k-2]*PkP1*PkP1+(X2+XX)*Pt);
            YTy=X2*(XX*P3k+Pt);  
            ZT=XX*Pk;
        }

        elliptic_add(XT,XTy,YT,YTy,ZT,XPP,YPP,YPPy);
// 
// Test for Schoof's case 1 - LHS (XT,YT,ZT) is point at infinity
//

        cout << "NP mod " << lp << " = " << flush;
        if (iszero(ZT))
        { // Is it zero point? (XPP,YPP) = - K(X,Y)
            t[i]=0;
            cout << setw(4) << (p+1)%lp;
            if ((p+1)%lp==0)
            {      
                cout << " ***" << endl;
                if (search) {escape=TRUE; break;}
            }
            else cout << endl;
            continue;         
        }

        Poly2Mod XP3,XP4,XP6;
        Poly2Mod ZT2,ZT3,ZT2XP;

        ZT2=ZT*ZT;
        ZT3=ZT2*ZT;
        ZT2XP=XP*ZT2;
        XP2=XP*XP;
        XP3=XP*XP2;
        XP4=XP2*XP2;
        XP6=XP3*XP3;

        SX=inverse(XP);   // we need 1/XP mod Fl

        Pf[0]=0; Pf[1]=1; Pf[2]=XP;
        P2f[1]=1; P3f[1]=1;
        P2f[2]=Pf[2]*Pf[2];
        P3f[2]=P2f[2]*Pf[2];

        Pf[3]=XP4+XP3+B;
        P2f[3]=Pf[3]*Pf[3];
        P3f[3]=P2f[3]*Pf[3];

        Pf[4]=XP6+B*XP2;
        P2f[4]=Pf[4]*Pf[4];
        P3f[4]=P2f[4]*Pf[4];
         
        low=5;

        for (tau=1;tau<=lp/2;tau++)
        {
            int res=0;
            Poly2Mod Hx,Ax,Bx,Rx,Tx,Ry,Ty,Pt;
            if (!permisso[tau]) continue;

            cout << setw(4) << (p+1-tau)%lp << flush;     

            for (jj=low;jj<=tau+2;jj++)
            { // different for odd and even
                if (jj%2==1)
                { // 2 mod-muls/
                    n=(jj-1)/2;
                    Pf[jj]=Pf[n+2]*P3f[n]+P3f[n+1]*Pf[n-1];
                }
                else
                { // 4 mod-muls
                    n=jj/2;
                    Pf[jj]=SX*Pf[n]*(Pf[n+2]*P2f[n-1]+Pf[n-2]*P2f[n+1]);
                }
                P2f[jj]=Pf[jj]*Pf[jj];    // square
                if (jj<=1+(1+(lp/2))/2) P3f[jj]=P2f[jj]*Pf[jj];   // cube
            } 
            if (tau+3>low) low=tau+3;

            if (tau==1)
            { // easy case
                Ax=ZT2XP+XT;
                Bx=XTy;
            }
            else
            {
                Pt=Pf[tau-1]*Pf[tau+1];
                Ax=ZT2*(XP*P2f[tau]+Pt)+P2f[tau]*XT;
                Bx=P2f[tau]*XTy;
            }
               
            Hx=Ax*Ax+XX*Ax*Bx+MFX*(Bx*Bx);

            if (iszero(Hx))   // NOTE: GCD not needed
            { // found it. Now compare Y coordinates to determine sign

                if (tau==1)
                {
                    Ax=YT+ZT3*YP;
                    Bx=YTy+ZT3*YPy;
                }
                else
                {
                    Tx=XP*P2f[tau]*Pf[tau];
                    Pt*=Pf[tau];
                    Ax=YT*Tx+ZT3*(Tx*(XP+YP)+(XP2+XP+YP)*Pt+Pf[tau-2]*P2f[tau+1]);
                    Bx=YTy*Tx+ZT3*YPy*(Tx+Pt);                                                                
                }

                Hx=Ax*Ax+XX*Ax*Bx+MFX*(Bx*Bx);  // substitute into curve
                if (!iszero(Hx))
                { // its the other one
                    tau=(lp-tau)%lp;
                    cout << "\b\b\b\b";
                    cout << setw(4) << (p+1-tau)%lp << flush;
                }
                t[i]=tau;
                if ((p+1-tau)%lp==0)
                {
                    cout << " ***" << endl;
                    if (search) escape=TRUE;
                }
                else cout << endl;
                break;
            }
            cout << "\b\b\b\b";
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

    if (escape) {bb+=1; continue;}
    Big order,ordermod;
    ordermod=1; for (i=0;i<nl-start_prime;i++) ordermod*=l[start_prime+i];
    order=(p+1-CRT.eval(&t[start_prime]))%ordermod;    // get order mod product of primes

    nrp=kangaroo(p,order,ordermod,TR,found);

    if (!found && search) {bb+=1; continue; }
    else break;

    }

    if (fout) 
    {
        cf=1;      // set co-factor=1
        if (found)
        { // An "ideal" curve was found 
            if (TR==1)
            {
                nrp/=2;
                cf=2;
            }
            else
            {
                nrp/=4;
                cf=4;
            }
        }

    // generate a random point on the curve 
    // point will be of prime order for "ideal" curve, otherwise any point
        forever
        {
            EC2 P;
            do {
                x=rand(p);
            } while (!GG.set(x,x));
            if (!found) break;
            P=GG;
            
            P*=(Big)cf;
            if (P.iszero()) continue;
            P=GG;
            P*=nrp;
            if (!P.iszero()) continue;
            break;
        }

        GG.get(x,y);
        ofile << M << endl;
        mip->IOBASE=16;
        ofile << aa << endl;
        ofile << bb << endl;
        ofile << nrp << endl;
        ofile << x << endl;
        ofile << y << endl;
        mip->IOBASE=10;
        ofile << a << endl;
        ofile << b << endl;
        ofile << c << endl;
    }

    if (p==nrp) 
    {
        cout << "WARNING: Curve is anomalous" << endl;
        return 0;
    }

// check MOV condition for curves of Cryptographic interest
//    if (M<128) return 0;

    nrp/=2;
    if (nrp%2==0) nrp/=2;
    
    d=1;
    for (i=0;i<50;i++)
    {
        d=modmult(d,p,nrp);
        if (d==1) 
        {
           if (i==1 || prime(nrp))   cout << "WARNING: Curve fails MOV condition, k= " << i << endl;
           else                      cout << "WARNING: Curve fails MOV condition, k<= " << i << endl;
           return 0;
        }
    }

    return 0;
}

