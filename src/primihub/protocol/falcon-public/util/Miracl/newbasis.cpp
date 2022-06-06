//
// cl /O2 /GX newbasis.cpp poly2.cpp gf2m.cpp big.cpp miracl.lib
//
// Program to convert a GF(2^m) value from one irreducible polynomial 
// representation to another
//
//

#include <iostream>
#include "poly2.h"

using namespace std;

Miracl precision(50,0);

int main(int argc,char **argv)
{
    miracl *mip=&precision;
    Poly2 g,c,h,ut;
    GF2m u,f,w[600],r;
    Big t[600],a,d;
    Variable x;
    int i,ii,m,b,n,a1,b1,c1,a2,b2,c2,Base,ip;

    argv++; argc--;
    if (argc!=8 && argc!=9)
    {
        cout << "Incorrect Usage" << endl;
        cout << "Program converts a value from one polynomial basis to another" << endl;
        cout << "newbasis <n> <m> <a1> <b1> <c1> <a2> <b2> <c2>" << endl;
        cout << "for decimal <n>, OR" << endl;
        cout << "newbasis -h <n> <m> <a1> <b1> <c1> <a2> <b2> <c2> " << endl;
        cout << "for <n> in hex" << endl;
        cout << "converts <n> from basis x^m+x^a1+x^b1+x^c1+1 to basis x^m+x^a2+x^b2+x^c2+1" << endl;
        return 0;
    }
    ip=0;
    m=a1=b1=c1=a2=b2=c2=0;
    mip->IOBASE=10;
    if (strcmp(argv[0],"-h")==0) {mip->IOBASE=16; ip=1;}
    
    a=argv[ip];
    mip->IOBASE=10;

    m=atoi(argv[ip+1]);
    a1=atoi(argv[ip+2]);
    b1=atoi(argv[ip+3]);
    c1=atoi(argv[ip+4]);
    a2=atoi(argv[ip+5]);
    b2=atoi(argv[ip+6]);
    c2=atoi(argv[ip+7]);

    prepare_basis(m,a2,b2,c2,TRUE);              // convert to this basis..
    g=pow2(x,m)+pow2(x,a1)+pow2(x,b1)+pow2(x,c1)+1;  // from this basis
 
// IEEE-1363 Algorithm A5.6, find a Root in GF (2m) of the Irreducible Binary Polynomial

    irand(6L);

    cout << "Finding root of irreducible polynomial...." << endl;
    while (degree(g)>1)
    {
        u=random2();
        ut.addterm(u,1);
        c=ut;
        for (i=1;i<m;i++)
            c=(c*c+ut)%g;
        h=gcd(c,g);
        if (degree(h)==0 || degree(h)==degree(g)) continue;
        if (2*degree(h)>degree(g)) g=g/h;
        else                       g=h;
    }
    
    r=g.coeff(0);

// Now compute change-of-basis matrix - A7.3
// This matrix t[.] can be precomputed and stored

    cout << "Now computing change of basis matrix..." << endl;

    w[0]=1;
    for (i=1;i<m;i++)
        w[i]=w[i-1]*r;

    for (n=m-1;n>=0;n--)
    {
        t[n]=0;
        for (i=m-1;i>=0;i--)
        {
             b=bit((Big)w[i],n);
             t[n]=(t[n]<<1)+b;
        }
    }

// Do the conversion  

    cout << "Finally convert the value to new representation" << endl;

    d=0;
    for (i=m-1;i>=0;i--)
    {
        b=ham(land(a,t[i]))%2;
        d=(d<<1)+b;
    }

    f=d;

    cout << (Big)f << " decimal" << endl;
    mip->IOBASE=16;
    cout << (Big)f << " hex" << endl;

	return 0;
}
