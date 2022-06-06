// Apply the GLV algorithm to decompose k (for use by GLS elliptic curves)
// See "Faster Point Multiplication on Elliptic Curves with efficient Endomorphisms", 
// by Gallant, Lambert and Vanstone".
// cl /O2 /GX glv.cpp big.cpp miracl.lib
// August 2008

#include <iostream>
#include "big.h"

using namespace std;

Miracl precision(50,0);

char *group="3FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFE09C5F010948D9D930E79156D8BA3CAF5";

int main()
{
    int i,mkb,tmkb;
    miracl *mip=&precision;
    mip->IOBASE=16;
    Big r=group;
    Big lambda,p;    
    Big u,v,u1,u2,u3,v1,v2,v3,t1,t2,t3,q,s,rm,tm,rm1,tm1,rm2,tm2,V1[2],V2[2],det,k,b1,b2,t,V[2],U[2],e;

// offline calculations

    cout << "r=      " << r << endl;
    lambda=sqrt(r-1,r);    // endomorphism corresponds to sqrt(-1) mod r
    cout << "lambda= " << lambda << endl;

    p=pow((Big)2,127)-1;
    t="3204F5AE088C39A7";

    cout << "p^2+1-2*p+t*t= " << p*p+1-2*p+t*t << endl;

    cout << "t+(p-1)*lambda mod r = " << (t+(p-1)*lambda)%r << endl;
    cout << "(p-1) - lambda*t mod r = " << ((p-1)-lambda*t)%r << endl;

    u1=1; u2=0; u3=r;
    v1=0; v2=1; v3=lambda;
    s=sqrt(r);
    
    cout << "s=       " << s << endl;
    cout << "2^127-2= " << pow((Big)2,127)-2 << endl;
    while (u3>s)
    {
        q=u3/v3;
        t1=u1-q*v1; t2=u2-q*v2; t3=u3-q*v3;
        u1=v1, u2=v2; u3=v3;
        v1=t1; v2=t2; v3=t3;
    }
    cout << "u3= " << u3 << endl;
    cout << "u2= " << u2 << endl;
    cout << "u1= " << u1 << endl;
    rm=u3; tm=-u2;
    q=u3/v3;
    t1=u1-q*v1; t2=u2-q*v2; t3=u3-q*v3;
    u1=v1, u2=v2; u3=v3;
    v1=t1; v2=t2; v3=t3;
    rm1=u3; tm1=-u2;
    q=u3/v3;
    t1=u1-q*v1; t2=u2-q*v2; t3=u3-q*v3;
    u1=v1, u2=v2; u3=v3;
    v1=t1; v2=t2; v3=t3;
    rm2=u3; tm2=-u2;
    V1[0]=rm1; V1[1]=tm1;

    cout << "(V1[0]+V1[1]*lambda)%r= " << (V1[0]+V1[1]*lambda)%r << endl;

    if (rm*rm+tm*tm < rm2*rm2+tm2*tm2)
    {
        V2[0]=rm; V2[1]=tm;
    }
    else
    {
        V2[0]=rm2; V2[1]=tm2;
    }

    cout << "(V2[0]+V2[1]*lambda)%r= " << (V2[0]+V2[1]*lambda)%r << endl;


    cout << "v1[0]= " << V1[0] << endl;
    cout << "v1[1]= " << V1[1] << endl;
    cout << "v2[0]= " << V2[0] << endl;
    cout << "v2[1]= " << V2[1] << endl;

    det=V1[0]*V2[1]-V2[0]*V1[1];
    det=-det;
    cout << "det= " << det << endl;

// online calculation - input V1, V2, determinant, and multiplier k which will be decomposed into k0 and k1
    tmkb=0;
    for (i=0;i<10;i++)
    {
        k=randbits(254);

        t=-(V2[1]*k);
        b1=t/det;
        if (t%det>(det/2)) b1+=1; // don't bother with "rounding to nearest"
    
        t=-(-V1[1]*k);
        b2=t/det;
        if (t%det>(det/2)) b2+=1;
 
    cout << "b1= " << b1 << endl;
    cout << "b2= " << b2 << endl;

        V[0]=b1*V1[0]+b2*V2[0];
        V[1]=b1*V1[1]+b2*V2[1];

        U[0]=k-V[0];
        U[1]=-V[1];

        cout << "k0= " << (U[0]) << " bits= " << bits(U[0]) << endl;
        cout << "k1= " << (U[1]) << " bits= " << bits(U[1]) << endl;
        e=(U[0]+U[1]*lambda)%r;
        if (e<0) e+=r;
        cout << "k=             " << k << endl;
        cout << "(k0+k1*lam)%r= " << e << endl;
        if (k!=e) break;
        if (bits(U[0])>126 || bits(U[1])>126) break;
        mkb=bits(U[0]);
        if (bits(U[1])>mkb) mkb=bits(U[1]);
        tmkb+=mkb;
       // if (U[0]<0 || U[1]<0) break;
    }
    cout << "tmkb= " << tmkb << endl;
    return 0;
}
