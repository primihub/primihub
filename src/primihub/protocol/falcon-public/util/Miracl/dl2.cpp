/* Even Faster Duursma-Lee char 2 Tate pairing based on eta_T pairing */
/* cl /O2 /GX dl2.cpp ec2.cpp gf2m4x.cpp gf2m.cpp big.cpp miracl.lib  */
/* Half sized loop so nearly twice as fast! */
/* 14th March 2005 */

#include <iostream>
#include <ctime>

#include "gf2m.h"
#include "gf2m4x.h"
#include "ec2.h"

// set TYPE = 1 if B=0 && (M=1 or 7 mod 8), else TYPE = 2
// set TYPE = 1 if B=1 && (M=3 or 5 mod 8), else TYPE = 2 

// some example curves to play with

//#define M 283
//#define T 249
//#define U 219
//#define V 27
//#define B 1
//#define TYPE 1
//#define CF 1

//#define M 367
//#define T 21
//#define U 0
//#define V 0
//#define B 1
//#define TYPE 2
//#define CF 1

//#define M 379
//#define T 317
//#define U 315
//#define V 283
//#define B 1
//#define TYPE 1
//#define CF 1

#define M 1223
#define T 255
#define U 0
#define V 0
#define B 0
#define TYPE 1
#define CF 5

//#define M 271
//#define T 207
//#define U 175
//#define V 111
//#define B 0
//#define TYPE 1
//#define CF 487805

//#define M 353 
//#define B 1
//#define T 215
//#define U 0
//#define V 0
//#define TYPE 2
//#define CF 1

//#define M 271
//#define U 0
//#define V 0
//#define T 201
//#define B 0
//#define TYPE 1
//#define CF 487805

#define IMOD4 ((M+1)/2)%4

//#define XX (IMOD4%2)
//#define YY (IMOD4/2)
//#define NXX (1-XX)

using namespace std;

Miracl precision(40,0);

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
// Loop unrolled x2 for speed
//

GF2m4x tate(EC2& P,EC2& Q)
{ 
    GF2m xp,yp,xq,yq,t;
    GF2m4x miller,w,u,u0,u1,v,f,res;
    int i,m=M;         

    normalise(P); normalise(Q);
    extract(P,xp,yp);
    extract(Q,xq,yq);
 
// first calculate the contribution of adding P or -P to 2^[(m+1)/2].P
//
// Note that 2^[(m+1)/2].Point(x,y) = Point(x^2+1,x^2+y^2) or something similar....
// Line slope is x or x+1 (thanks Steven!)
//
// Then the truncated loop, four flavours...

#if IMOD4 == 1
	                                                      //              (X=1)
                                                          //              (Y=0)   
	t=xp;                                                 // 0            (X+1)
	f.set(t*(xp+xq+1)+yq+yp+B,t+xq+1,t+xq,0);             // 0            (Y)


    miller=1;
    for (i=0;i<(m-3)/2;i+=2)
    {

        t=xp+1; xp=sqrt(xp); yp=sqrt(yp);                 // 1            (X)
        u0.set(t*(xp+xq+1)+yp+yq,t+xq+1,t+xq,0);          // 1   0        (X) ((X+1)*(xp+1)+Y)
		xq*=xq; yq*=yq;

        t=xp+1; xp=sqrt(xp); yp=sqrt(yp);
        u1.set(t*(xp+xq+1)+yp+yq,t+xq+1,t+xq,0);
	    xq*=xq; yq*=yq;

	    u=mul(u0,u1);
        miller*=u;
    }

// final step

    t=xp+1; xp=sqrt(xp); yp=sqrt(yp);
    u.set(t*(xp+xq+1)+yp+yq,t+xq+1,t+xq,0);
    miller*=u;

#endif

#if IMOD4 == 0
														  //              (X=0)
                                                          //              (Y=0)
	t=xp+1;                                               // 1            (X+1)
    f.set(t*(xq+xp+1)+yq+yp+B,t+xq+1,t+xq,0);             // 0            (Y)
    miller=1;
  
    for (i=0;i<(m-1)/2;i+=2)  
    {
// loop is unrolled x 2 
        t=xp; xp=sqrt(xp); yp=sqrt(yp);                   // 0            (X)
        u0.set(t*(xp+xq)+yp+yq+xp+1,t+xq+1,t+xq,0);       // 0  xp+1      (X)  ((X+1)*(xp+1)+Y
	    xq*=xq; yq*=yq;

        t=xp; xp=sqrt(xp); yp=sqrt(yp);
        u1.set(t*(xp+xq)+yp+yq+xp+1,t+xq+1,t+xq,0);
	    xq*=xq; yq*=yq;

	    u=mul(u0,u1);
        miller*=u;
    }

#endif

#if IMOD4 == 2
														  //              (X=0)                                                         //              (Y=1)
    t=xp+1;                                               // 1            (X+1)
    f.set(t*(xq+xp+1)+yq+yp+B+1,t+xq+1,t+xq,0);           // 1            (Y)
    miller=1;
    for (i=0;i<(m-1)/2;i+=2)
    {

        t=xp;  xp=sqrt(xp); yp=sqrt(yp);                 // 0            (X)
        u0.set(t*(xp+xq)+yp+yq+xp,t+xq+1,t+xq,0);         // 0   xp+0     (X)  ((X+1)*(xp+1)+Y)
	    xq*=xq; yq*=yq;

        t=xp;  xp=sqrt(xp); yp=sqrt(yp);
        u1.set(t*(xp+xq)+yp+yq+xp,t+xq+1,t+xq,0);
	    xq*=xq; yq*=yq;

        u=mul(u0,u1);
        miller*=u;
    }

#endif

#if IMOD4 == 3
                                                          //              (X=1)                                                        //              (Y=1)
    t=xp;                                                 // 0            (X+1)
    f.set(t*(xq+xp+1)+yq+yp+B+1,t+xq+1,t+xq,0);           // 1            (Y)

    miller=1;
    for (i=0;i<(m-3)/2;i+=2)
    {

        t=xp+1; xp=sqrt(xp); yp=sqrt(yp);                 // 1            (X)
        u0.set(t*(xp+xq+1)+yp+yq+1,t+xq+1,t+xq,0);        // 1   1        (X) ((X+1)*(xp+1)+Y)
	    xq*=xq; yq*=yq;

        t=xp+1; xp=sqrt(xp); yp=sqrt(yp);
        u1.set(t*(xp+xq+1)+yp+yq+1,t+xq+1,t+xq,0);
	    xq*=xq; yq*=yq;

        u=mul(u0,u1);
        miller*=u;
    }

// final step

    t=xp+1; xp=sqrt(xp); yp=sqrt(yp);
    u.set(t*(xp+xq+1)+yp+yq+1,t+xq+1,t+xq,0);
    miller*=u;

#endif

    miller*=f;

// raising to the power (2^m-2^[m+1)/2]+1)(2^[(m+1)/2]+1)(2^(2m)-1) (TYPE 2)
// or (2^m+2^[(m+1)/2]+1)(2^[(m+1)/2]-1)(2^(2m)-1) (TYPE 1)
// 6 Frobenius, 4 big field muls...

    u=v=w=miller;
    for (i=0;i<(m+1)/2;i++) u*=u;

#if TYPE == 1   

    u.powq();
    w.powq();
    v=w;
    w.powq();
    res=w;
    w.powq();
    w*=u;
    w*=miller;
    res*=v;
    u.powq();
    u.powq();
    res*=u;

#else

    u.powq();
    v.powq();
    w=u*v;
    v.powq();
    w*=v;
    v.powq();
    u.powq();
    u.powq();
    res=v*u;
    res*=miller;
  
#endif

    res/=w;
    return res;            
}

int main()
{
    EC2 P,Q,W;
    Big bx,s,r,x,y,order;
    GF2m4x res; 
    time_t seed;
	int i;
    miracl *mip=&precision;

    time(&seed);
    irand((long)seed);

    if (!ecurve2(-M,T,U,V,(Big)1,(Big)B,TRUE,MR_PROJECTIVE)) // -M indicates Super-Singular
    {
        cout << "Problem with the curve" << endl;
        return 0;
    }

// Curve order = 2^M+2^[(M+1)/2]+1 or 2^M-2^[(M+1)/2]+1 is nearly prime     

    cout << "IMOD4= " << IMOD4 << endl;
    cout << "M%8= " << M%8 << endl; 

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

   /*  for (int i=0;i<10000;i++)  */

    P*=CF;  // cofactor multiplication
    Q*=CF;

//	order=pow((Big)2,M)-pow((Big)2,(M+1)/2)+1;
//	P*=order;
//	cout << "P= " << P << endl;
//	exit(0);
/*
mip->IOBASE=16;
cout << "P= " << P << endl;
cout << "Q= " << Q << endl;

Big ddd=pow((Big)2,32);
Big sx,sy;


P.get(x,y);
sx=x;
sy=y;

while (x>0)
{
    cout << "0x" << hex << x%ddd << ",";
    x/=ddd;
}
cout << endl;

while (y>0)
{
    cout << "0x" << hex << y%ddd << ",";
    y/=ddd;
}
cout << endl;

ddd=256;
x=sx;
y=sy;

while (x>0)
{
    cout << "0x" << hex << x%ddd << ",";
    x/=ddd;
}
cout << endl;

while (y>0)
{
    cout << "0x" << hex << y%ddd << ",";
    y/=ddd;
}
cout << endl;


Q.get(x,y);
sx=x;
sy=y;
ddd=pow((Big)2,32);

while (x>0)
{
    cout << "0x" << hex << x%ddd << ",";
    x/=ddd;
}
cout << endl;

while (y>0)
{
    cout << "0x" << hex << y%ddd << ",";
    y/=ddd;
}
cout << endl;

ddd=256;
x=sx;
y=sy;

while (x>0)
{
    cout << "0x" << hex << x%ddd << ",";
    x/=ddd;
}
cout << endl;

while (y>0)
{
    cout << "0x" << hex << y%ddd << ",";
    y/=ddd;
}
cout << endl;



exit(0);
*/
//for (i=0;i<1000;i++)
    res=tate(P,Q);

    s=rand(256,2);
    r=rand(256,2);
    res=pow(res,s); res=pow(res,r);
//mip->IOBASE=16;
    cout << "e(P,Q)^sr= " << res << endl;

    P*=s;
    Q*=r;

    res=tate(Q,P);

    cout << "e(sP,rQ)=  " << res << endl;

//Big q=pow((Big)2,M)-pow((Big)2,(M+1)/2)+1;

//cout << pow(res,q) << endl;

    return 0;
}
