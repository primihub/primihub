// 
// Generate random non-supersingular curves suitable for pairings...
// Security Multiplier 2,4 or 8
// Uses "folklore method" - actually due to Cocks & Pinch
//
// See Chapter 9 (by Steve Galbraith) in Advances in Elliptic Curve 
// Cryptography, edited by Blake, Seroussi and  Smart, Cambridge University 
// Press (published January 2005)
//
// cl /O2 /GX folklore.cpp big.cpp miracl.lib
//
// if k=2 prime p = 3 mod 4
//
// Creates batch file c.bat to generate curve using CM
// You MUST make sure that the CM utility builds the curve with the right 
// number of points (normally you get two choices for the given D).
//
// NOTE: The CM utility creates a file kn.ecs (where n=2,4 or 8)
// Then you must delete the last line from kn.ecs and manually insert the file 
// extra.ecs at the bottom. 
// The file extra.ecs is created by this program.
//

#include <iostream>
#include <fstream>
#include "big.h"
#include <ctime>

using namespace std;

Miracl precision(100,0);

BOOL sqr_free(Big m)
{
    Big r;
    for (r=2;;r+=1)
    {
        if (m%(r*r)==0) return FALSE;
        if (r*r > m) break;
    }
    return TRUE;
}

int main(int argc,char **argv)
{
    Big D;
    time_t seed;
    int i,e,k,nbits;
    Big s,j,p,r,g,b0,a,c,n,v,w,t;
    ofstream batch("c.bat");
    ofstream extra("extra.ecs");
    miracl *mip=&precision;
    BOOL correct;

    time(&seed);
    irand((long)seed);

    argc--; argv++;

    if (argc!=1)
    {
        cout << "Missing parameter" << endl;
        return 0;
    }
    e=atoi(argv[0]);

    if (e<1 || e>3)
    {
        cout << "Bad parameters" << endl;
        cout << "folklore N " << endl;
        cout << "where N is 1,2 or 3. Security multiplier k=2^N" << endl;
        return 0;
    }

// important parameters calculated here...

    k=1<<e;            // security multiplier - a power of 2 please

    if (k==8) r=pow((Big)2,223)+pow((Big)2,13)+1;
    if (k==4) r=pow((Big)2,191)+pow((Big)2,2)+1; 
    if (k==2) r=pow((Big)2,159)+pow((Big)2,17)+1;

                      // prime group order - (r-1) divisible by k

    nbits=512;        // size of modulus p

    if (!prime(r) || (r-1)%k!=0)
    {
        cout << "Bad group order" << endl;
        return 0;
    }

	cout << "If this freezes, just kill the program and try again.." << endl;

    D=rand(16,2);                  // D is random 16-bit number
    if (k==2)
    {
        while (D%4!=2) D+=1; 
        forever
        {
            D+=4;                  // D=2 mod 4
            if (!sqr_free(D)) continue;
            w=rand(96,2);          // for eventual 512-bit prime
            w=2*w+1;  
            t=2*w*r;
            if (bits(t*t)!=514) continue; 
        
            v=sqrt(-4*inverse(D,r),r);
            if (v.iszero()) continue;
            if (v%2==1) v=r-v;
            if (gcd(t/2,v/2)!=1) continue;
            if (gcd(t/2,D)!=1) continue;
            break;    
        }

        t/=2; v/=2;
        for (;;v+=r)
        {
            if (v%2==0) continue; 
            p=D*v*v+t*t;    // = 3 mod 4
            if (prime(p)) break;
        }
        n=p+1-(t*2);               // Number of points on curve
    }
    else
    {
      //  if (D%2==0) D+=1;          // D must be odd
        for (;;D+=1)
        {
            if (!sqr_free(D)) continue;
            if (jacobi(r-D,r)!=1) continue;

            g=pow(rand(r),(r-1)/k,r);
            s=g;
            correct=TRUE;
            for (i=0;i<e;i++)
            {                      // make sure its k-th root of unity
                if (s==1) correct=FALSE;
                s*=s; s%=r;
            }
            if (!correct) continue;     

            a=((g+1)*inverse((Big)2,r))%r;
            if (gcd(a,D)!=1) continue;
			if (a%2==0 && D%2==0) continue;
		//	if (a%4==0 && D%8==1) continue;
		//	if (a%4==2 && D%8==5) continue;
        //    if (a%4==0 && D%8!=5) continue; // conditions for p=5 mod 8
        //    if (a%4==2 && D%8!=1) continue;
            break;
        }

    
        b0=((a-1)*inverse(sqrt(r-D,r),r))%r;

        s=sqrt(rand(nbits,2)/(D*r*r))+1;

        for (j=s;;j+=1)
        {
            p=a*a+D*(b0+j*r)*(b0+j*r);
			//cout << p%8 << " a%4= " << a%4 << " D%8= " << D%8 << endl;
            if (p%8==1) continue;
			
			if (p%8==7 && ((p%5==1) || (p%5==4))) continue;
            if (prime(p)) break;
        }
        n=(p+1-2*a);               // Number of points on curve
    }
    cout << "D= " << D << endl;
    cout << "k= " << k << endl;
    mip->IOBASE=16;
    cout << "r= " << r << endl;
    mip->IOBASE=10;
    cout << "p= " << p << endl;
    cout << "bits(p)= " << bits(p) << endl;
    cout << "p%8= " << p%8 << endl;
    cout << "#E= " << n << endl;
    cout << "t= " << p+1-n << endl;

    for (i=1;i<k;i++)
    {
        c=(pow(p,i,r)-1)%r;
        if (c==0)
        {
            cout << "This should never happen - Failed" << endl;
            return 0;
        }
    }
    c=(pow(p,k,r)-1)%r;
    if (c!=0 || n%r!=0)
    {
        cout << "This should never happen - Failed" << endl;
        return 0;
    }

    batch << "cm " << p << " -K0 -D" << D << " -P7 -t -o k" << k << ".ecs" << endl;

    mip->IOBASE=16;
    extra << n/r << endl;
    extra << r << endl;

    
    if (k==4)
    {
//        extra << pow(p-2,(p-1)/4,p) << endl; 
        w=(pow(p,2)+1)/r;
        extra << w/p << endl;
        extra << w%p << endl;
        
    }
    if (k==8)
    {
//        extra << pow(p-2,(p-5)/8,p) << endl; 
        w=(pow(p,4)+1)/r;
        extra << w/(p*p*p) << endl;
        w%=(p*p*p);
        extra << w/(p*p) << endl;
        w%=(p*p);
        extra << w/p << endl;
        extra << w%p << endl;
    }

    return 0;
}

