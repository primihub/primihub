/* Fast Duursma-Lee char 2 Tate pairing */
/* See paper by Barreto, Galbraith O'hEigeartaigh and Scott - http://eprint.iacr.org/2004/375 */
/* cl /O2 /GX /DGF2MS=10 dl.cpp ec2.cpp gf2m4x.cpp gf2m.cpp big.cpp miracl.lib  */

#include <iostream>
#include <ctime>

#include "gf2m.h"
#include "gf2m4x.h"
#include "ec2.h"

#define M 313
#define T 121
#define U 0
#define V 0
#define B 1
#define TYPE 2

// #define M 101
// #define T 35
// #define U 31
// #define V 3
// #define B 0
// #define TYPE 2

// #define M 107
// #define T 37
// #define U 33
// #define V 23
// #define B 1
// #define TYPE 1

// #define M 271
// #define T 58
// #define U 0
// #define V 0
// #define B 0
// #define TYPE 1

using namespace std;

Miracl precision(15,0);

//
// Extract ECn point in internal GF2m format
//

void extract(EC2& A,GF2m& x,GF2m& y)
{ 
    x=(A.get_point())->X;
    y=(A.get_point())->Y;
}

//
// Tate Pairing - note miller -> miller variable
//

GF2m4x tate(EC2& P,EC2& Q)
{ 
    GF2m xp,yp,xq,yq,t,w;
    GF2m4x miller,u0,u1,u,res;
    int i,m=M;         

    normalise(P);
    normalise(Q);

    extract(P,xp,yp);
    extract(Q,xq,yq);

    miller=1;

// loop is unrolled x 2

    for (i=0;i<(m-1);i+=2)
    {
        t=xp*xp;
	    u0.set((t+1)*(xp+xq)+yp+yq+t,t+xq,t+xq+1,0);
        xp=t; yp*=yp; xq=sqrt(xq); yq=sqrt(yq);

        t=xp*xp;
	    u1.set((t+1)*(xp+xq)+yp+yq+t,t+xq,t+xq+1,0);
        xp=t; yp*=yp; xq=sqrt(xq); yq=sqrt(yq);

        u=mul(u0,u1);

        miller*=u;
    }

// final step

    t=xp*xp;
    u.set((t+1)*(xp+xq)+yp+yq+t,t+xq,t+xq+1,0);
    xp=t; yp*=yp; xq=sqrt(xq); yq=sqrt(yq);
    miller*=u;
    
    res=miller;

// final exponentiation to q^2-1

    res.powq();    // raise to the power of q=2^m using Frobenius 
    res.powq();

    res/=miller;   // one inversion     

    return res;            
}

int main()
{
    EC2 P,Q;
    Big bx,by,s,r;
    GF2m4x res; 
    time_t seed;

    time(&seed);
    irand((long)seed);

    if (!ecurve2(-M,T,U,V,(Big)1,(Big)B,TRUE,MR_PROJECTIVE))
    {
        cout << "Problem with the curve" << endl;
        return 0;
    }

// Curve order = 2^M+2^[(M+1)/2]+1 or 2^M-2^[(M+1)/2]+1 is prime     

    forever 
    {
        bx=rand(M,2);
        if (P.set(bx,bx)) break;
    }

    forever 
    {
        bx=rand(M,2);
        if (Q.set(bx,bx)) break;
    }

    res=tate(P,Q);

    s=rand(200,2);
    r=rand(200,2);
    res=pow(res,s*r);

    cout << "e(P,Q)^sr= " << res << endl;

    P*=s;
    Q*=r;

    res=tate(P,Q);

    cout << "e(sP,rQ)=  " << res << endl;

    return 0;
}
