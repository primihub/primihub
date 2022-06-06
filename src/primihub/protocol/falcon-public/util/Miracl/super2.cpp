//
// Simple Program to find number of points on supersingular curves over GF(2^p)
//
// Either y^2+y=x^3+x 
// or     y^2+y=x^3+x+1
//
// See "Elliptic Curve Public Key Cryptosystems", Menezes, 
// Kluwer Academic Publishers, Chapter 3
//
// Requires: big.cpp ec2.cpp
//
//

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>
#include "big.h"
#include "ec2.h"

using namespace std;

Miracl precision=50;            // max. 50x32 bits per big number

int main(int argc,char **argv)
{
    ofstream ofile;
    int A,B,a,b,c,M,ip,m,i;
    Big nrp,q,r2q,x,y,f,p,r,cof,d;
    BOOL fout,gota,gotb,gotc,gotA,gotB,gotM,large;
    miracl *mip=&precision;
    EC2 P,Q,T,KP,KQ;

    gprime(100000);

    argv++; argc--;
    if (argc<1)
    {
        cout << "Incorrect Usage" << endl;
        cout << "Program finds the number of points (NP) on a Super-singular" << endl; 
        cout << "Elliptic curve which is defined over the Galois field GF(2^M)" << endl;
        cout << "The Elliptic Curve has the equation Y^2 +Y = X^3 + AX + B " << endl;
        cout << "(A,B) must be (1,0) or (1,1), M must be odd" << endl;
        cout << "A suitable trinomial or Pentanomial basis must" << endl;
        cout << "also be specified t^m+t^a+1 or t^m+t^a+t^b+t^c+1" << endl;
        cout << "These can be found in e.g. the IEEE P1363 standard" << endl;
        cout << "super2 <A> <B> <M> <a> <b> <c>" << endl << endl;
        cout << "e.g. super2 1 1 191 9" << endl << endl;
        cout << "To output to a file, use flag -o <filename>" << endl;
        cout << "\nFreeware from Certivox, Dublin, Ireland" << endl;
        cout << "Full C++ source code and MIRACL multiprecision library available" << endl;
        cout << "email mscott@indigo.ie" << endl;
        return 0;
    }

    fout=gotM=gotA=gotB=gota=gotb=gotc=FALSE;
    M=a=b=c=0;

    ip=0;
// Interpret command line
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
        if (!gotA) 
        {
            A=atoi(argv[ip++]);
            gotA=TRUE;
            continue;
        }
        if (!gotB) 
        {
            B=atoi(argv[ip++]);
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

    if ((!gotM || !gotA || !gotB) || a==0 || B<0 || A!=1 || B>1 || M%2!=1)
    {
        cout << "Error in command line" << endl;
        return 0;
    }

    if (!ecurve2(-M,a,b,c,(Big)A,(Big)B,TRUE,MR_AFFINE))
    {
        cout << "Illegal Curve Parameters" << endl;
        return 0;
    }

    m=M%8;

    q=pow((Big)2,M);
    r2q=pow((Big)2,(M+1)/2);

    if (B==0)
    {
        if (m==1 || m==7) nrp=q+1+r2q;
        else              nrp=q+1-r2q;
    }
    else
    {
        if (m==1 || m==7) nrp=q+1-r2q;
        else              nrp=q+1+r2q;
    }                     

    cout << "NP= " << nrp << endl;
    cof=1; large=false;   // indicates large prime factor found

    if (prime(nrp))
    {
        cout << "NP is Prime!" << endl;
        p=nrp;
        large=TRUE;
    }
    else  
    {
        cout << "NP= " ;
        p=nrp;
        for (i=0;;i++)
        {
            f=mip->PRIMES[i];
            if (f==0) break; 
            if (p%f==0)
            {
                cof*=f;
                p/=f;
                if (p==1) break;
                cout << f << "." ;
                continue;
            }
            if ((p/f)<=f)
            {
                cout << p << endl;
                p=1;
                break;
            } 
        }
        if (p>1) 
        {
            if (prime(p)) 
            {
                 cout << "prime" << endl;
                 large=TRUE;
            }
            else cout << "composite" << endl;
        }
    } 
    do {
         x=rand(q);
    } while (!P.set(x,x));

    T=P;
    T*=nrp;

    if (!T.iszero())
    {
        cout << "nrp*P!=O - something is wrong!" << endl;
        return 0;
    }
    
    P*=cof;
    
    if (large)
    { // check MOV condition
        d=1;
        for (i=1;i<=4;i++)
        {
            d=modmult(d,q,p);
            if (d==1)
            {
                cout << "Security Multiplier= " << i << endl;
                break;
            }
        }    
    }

    if (fout && large)
    {
    
        P.get(x,y);
        ofile << -M << endl;
        ofile << A << endl;
        ofile << B << endl;
        mip->IOBASE=16;
        ofile << nrp << endl;
        ofile << x << endl;
        ofile << y << endl;
        mip->IOBASE=10;
        ofile << a << endl;
        ofile << b << endl;
        ofile << c << endl;
    }
    return 0;
}

