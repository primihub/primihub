/*

   Scott's AKE Client/Server testbed

   See http://eprint.iacr.org/2002/164

   Compile as 
   cl /O2 /GX /DZZNS=8 ake8bwt.cpp zzn8.cpp zzn4.cpp zzn2.cpp ecn4.cpp big.cpp zzn.cpp  ecn.cpp miracl.lib
   Fastest using COMBA build for 256-bit mod-mul

   Brezing-Weng Curve - Tate pairing

   The file weng.ecs is required.
   This curve was constructed using the method described in 
   http://eprint.iacr.org/2003/143/
 
   Security is 192/2048

   Modified to prevent sub-group confinement attack

   NOTE: assumes p = 5 mod 8, p is 256-bits

   **** NEW **** Based on the observation by R. Granger and D. Page and N.P. Smart  in "High Security 
   Pairing-Based Cryptography Revisited" that multi-exponentiation can be used for the final exponentiation
   of the Tate pairing, we suggest the Power Pairing, which calculates E(P,Q,e) = e(P,Q)^e, where the 
   exponentiation by e is basically for free, as it can be folded into the multi-exponentiation.

   NOTE: Irreducible polynomial is x^8+2 : p = 5 mod 8

*/

#include <iostream>
#include <fstream>
#include <string.h>
#include "ecn.h"
#include <ctime>
#include "ecn4.h"
#include "zzn8.h"

using namespace std;

Miracl precision(8,0); 

// Using SHA-1 as basic hash algorithm

#define HASH_LEN 20

//
// Define one or the other of these
//
// Which is faster depends on the I/M ratio - See imratio.c
// Roughly if I/M ratio > 16 use PROJECTIVE, otherwise use AFFINE
//

#ifdef MR_AFFINE_ONLY
    #define AFFINE
#else
    #define PROJECTIVE
#endif

//
// Tate Pairing Code
//
// Extract ECn point in internal ZZn format
//

void extract(ECn& A,ZZn& x,ZZn& y)
{ 
    x=(A.get_point())->X;
    y=(A.get_point())->Y;
}

#ifdef PROJECTIVE
void extract(ECn& A,ZZn& x,ZZn& y,ZZn& z)
{ 
    big t;
    x=(A.get_point())->X;
    y=(A.get_point())->Y;
    t=(A.get_point())->Z;
    if (A.get_status()!=MR_EPOINT_GENERAL) z=1;
    else                                   z=t;
}
#endif

//
// Line from A to destination C. Let A=(x,y)
// Line Y-slope.X-c=0, through A, so intercept c=y-slope.x
// Line Y-slope.X-y+slope.x = (Y-y)-slope.(X-x) = 0
// Now evaluate at Q -> return (Qy-y)-slope.(Qx-x)
//

ZZn8 line(ECn& A,ECn& C,ZZn& slope,ZZn4& Qx,ZZn4& Qy)
{ 
    ZZn8 w;
    ZZn4 m=Qx;
    ZZn x,y,z,t;
#ifdef AFFINE
    extract(A,x,y);
    m-=x; m*=slope;  
    w.set((ZZn4)-y,Qy); w-=m;
#endif
#ifdef PROJECTIVE
    extract(A,x,y,z);
    x*=z; t=z; z*=z; z*=t; 

    x*=slope; t=slope*z;
    m*=t; m-=x; t=z;
    extract(C,x,x,z);
    m+=(z*y); t*=z;
    w.set(m,-Qy*t);

#endif
    return w;
}

//
// Add A=A+B  (or A=A+A) 
// Bump up num
//

void g(ECn& A,ECn& B,ZZn4& Qx,ZZn4& Qy,ZZn8& num,BOOL first)
{
    int type;
    ZZn  lam;
    ZZn8 u;
    big ptr;
    ECn P=A;

// Evaluate line from A

    type=A.add(B,&ptr);
    if (!type)   return; 

    lam=ptr;
    u=line(P,A,lam,Qx,Qy);
    if (first) num= u;
    else       num*=u;
}

//
// Tate Pairing - note denominator elimination has been applied
//
// P is a point of order q. Q(x,y) is a point of order m.q. 
// Note that P is a point on the curve over Fp, Q(x,y) a point on the 
// extension field Fp^4
//
// New! Power Pairing calculates E(P,Q,e) = e(P,Q)^e at no extra cost!
//
//

BOOL power_tate(ECn& P,ECn4 Q,Big& q,Big *cf,ZZn2 &Fr,Big &e,ZZn4& r)
{ 
    int i,nb,j,n,nbw,nzs;
    ECn A,P2,t[8];
    ZZn8 w,res,hc,z2n,zn[8],a[4];
    ZZn4 Qx,Qy;
    ZZn2 x,y;
    Big p=get_modulus();
    Big carry,ex[4];

    Q.get(Qx,Qy);
                    // convert from twist 
    Qx=tx(Qx);
    Qx.get(x,y);
    Qx.set(tx(x),tx(y));
    Qx=-Qx/2;       // Qx=-2.i^6.Qx, i is 8th root of -2
    Qy.get(x,y);
    Qy.set(tx(x),tx(y));
    Qy=-Qy/2;       // Qy=-2.i^4.Qx

    res=zn[0]=1;

/* Left to right method  */
    t[0]=P2=A=P;

    g(P2,P2,Qx,Qy,z2n,TRUE);

    normalise(P2);

    for (i=1;i<8;i++)
    {
        g(A,P2,Qx,Qy,hc,TRUE);
        t[i]=A;
        zn[i]=z2n*zn[i-1]*hc;
    }

    multi_norm(8,t); 
    A=P;

    nb=bits(q);

    for (i=nb-2;i>=0;i-=(nbw+nzs))
    {
        n=window(q,i,&nbw,&nzs,4);
        for (j=0;j<nbw;j++)
        {
            res*=res;           
            g(A,A,Qx,Qy,res,FALSE);
        } 
        if (n>0)
        {
            res*=zn[n/2];
            g(A,t[n/2],Qx,Qy,res,FALSE);
        }
        for (j=0;j<nzs;j++)
        {
            res*=res;
            g(A,A,Qx,Qy,res,FALSE);
        }  
    }

    if (!A.iszero() || res.iszero()) return FALSE;
 
    w=res;

    w.powq(Fr); w.powq(Fr);  // ^(p^4-1)
    w.powq(Fr); w.powq(Fr);  
    res=w/res;

    res.mark_as_unitary();

    a[3]=res;
    a[2]=a[3]; a[2].powq(Fr);
    a[1]=a[2]; a[1].powq(Fr);
    a[0]=a[1]; a[0].powq(Fr);


    if (e.isone()) for (i=0;i<4;i++) ex[i]=cf[i];
    else
    { // cf *= e
        carry=0;
        for (i=3;i>=0;i--)
            carry=mad(cf[i],e,carry,p,ex[i]);
    }

    res=pow(4,a,ex);
    r=real(res); // compression

 //   r=powl(real(res),cf[0]*p*p*p+cf[1]*p*p+cf[2]*p+cf[3]);    // ^(p*p*p*p+1)/q

    if (r.isunity()) return FALSE;
    return TRUE;            
}

//
// Hash functions
// 

Big H1(char *string)
{ // Hash a zero-terminated string to a number < modulus
    Big h,p;
    char s[HASH_LEN];
    int i,j; 
    sha sh;

    shs_init(&sh);

    for (i=0;;i++)
    {
        if (string[i]==0) break;
        shs_process(&sh,string[i]);
    }
    shs_hash(&sh,s);
    p=get_modulus();
    h=1; j=0; i=1;
    forever
    {
        h*=256; 
        if (j==HASH_LEN)  {h+=i++; j=0;}
        else         h+=s[j++];
        if (h>=p) break;
    }
    h%=p;
    return h;
}

Big H4(ZZn4 x)
{ // Hash an Fp2 to a big number
    sha sh;
    Big a,u,v;
    ZZn2 X,Y;
    char s[HASH_LEN];
    int m;

    shs_init(&sh);
    x.get(X,Y);

    X.get(u,v);

    a=u;
    while (a>0)
    {
        m=a%256;
        shs_process(&sh,m);
        a/=256;
    }
    a=v;
    while (a>0)
    {
        m=a%256;
        shs_process(&sh,m);
        a/=256;
    }

    Y.get(u,v);

    a=u;
    while (a>0)
    {
        m=a%256;
        shs_process(&sh,m);
        a/=256;
    }
    a=v;
    while (a>0)
    {
        m=a%256;
        shs_process(&sh,m);
        a/=256;
    }
    shs_hash(&sh,s);
    a=from_binary(HASH_LEN,s);
    return a;
}

// Hash and map a Server Identity to a curve point E(Fp4)

ECn4 hash4(char *ID)
{
    ECn4 T;
    ZZn4 x;
    ZZn2 X,Y;
    Big x0,y0;

    x0=y0=1;
    X.set(x0,y0);

    y0=1;
    x0=H1(ID);
    do
    {
        Y.set(x0,y0);
        x.set(X,Y);
        x0+=1;
    }
    while (!is_on_curve(x)) ;
    T.set(x);

//    while (!T.set(x));
    return T;
}     

// Hash and map a Client Identity to a curve point E(Fp)

ECn hash_and_map(char *ID,Big cof)
{
    ECn Q;
    Big x0=H1(ID);

    while (!is_on_curve(x0)) x0+=1;
    Q.set(x0);  // Make sure its on E(F_p)

    Q*=cof;
    return Q;
}

void set_frobenius_constant(ZZn2 &X)
{
    Big p=get_modulus();
    switch (get_mip()->pmod8)
    {
    case 5:
         X.set((Big)0,(Big)1); // = (sqrt(sqrt(-2))^(p-1)/4  
		 X=pow(X,(p-1)/4);
         break;
    case 3:  
		 X.set((Big)1,(Big)1);
		 X=pow(X,(p-3)/4); 
		 break;
    case 7:   
		 X.set((Big)2,(Big)1);
		 X=pow(X,(p-3)/4);     // note that 4 does not divide p-1, so this is the best we can do...
    default: break;
    }
}

int main()
{
    ifstream common("weng.ecs");      // elliptic curve parameters
    miracl* mip=&precision;
    ECn Alice,Bob,sA,sB;
    ECn4 Server,sS;
    ZZn4 res,sp,ap,bp;
	ZZn2 Fr;
    Big a,b,s,ss,p,q,r,B,cof,t,ii;
    int i,bitz,A;
    time_t seed;
    Big cf[4];

    cout << "Started" << endl;
    common >> bitz;
    mip->IOBASE=16;
    common >> p;
    common >> A;
    common >> B;
    common >> cof;
    common >> q;
    common >> cf[0];
    common >> cf[1];
    common >> cf[2];
    common >> cf[3];

    cout << "Initialised... " << p%24 << endl;

// cout << "ham(q)= " << ham(q) << endl;

//    q*=(Big)"5B51";

//    cout << "q= " << q << endl;
/*
ii=1;
forever
{
    t=ii*q;
    t=4*t-3;
    if (sqrt(t)*sqrt(t)==t) break;
    ii=ii+1;
}
cout << "ii= " << ii << endl;
*/
    time(&seed);
    irand((long)seed);

#ifdef AFFINE
    ecurve(A,B,p,MR_AFFINE);
#endif
#ifdef PROJECTIVE
    ecurve(A,B,p,MR_PROJECTIVE);
#endif

    set_frobenius_constant(Fr);
    
    mip->IOBASE=16;
    mip->TWIST=MR_QUADRATIC;  // map Server to point on twisted curve E(Fp2)

// hash Identities to curve point

    ss=rand(q);    // TA's super-secret 

    cout << "Mapping Server ID to point" << endl;
    Server=hash4((char *)"Server");

    cout << "Mapping Alice & Bob ID's to points" << endl;
    Alice=hash_and_map((char *)"Alice",cof);

    Bob=  hash_and_map((char *)"Robert",cof);

    cout << "Alice, Bob and the Server visit Trusted Authority" << endl; 

    sS=ss*Server; 

    sA=ss*Alice; 
    sB=ss*Bob; 

    cout << "Alice and Server Key Exchange" << endl;

    a=rand(q);   // Alice's random number
    s=rand(q);   // Server's random number

    if (!power_tate(sA,Server,q,cf,Fr,a,res)) cout << "Trouble" << endl;

    if (powl(res,q)!=(ZZn4)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
//    ap=powl(res,a);
    ap=res;

    if (!power_tate(Alice,sS,q,cf,Fr,s,res)) cout << "Trouble" << endl;
    if (powl(res,q)!=(ZZn4)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
//    sp=powl(res,s);
    sp=res;

    cout << "Alice  Key= " << H4(powl(sp,a)) << endl;
    cout << "Server Key= " << H4(powl(ap,s)) << endl;

    cout << "Bob and Server Key Exchange" << endl;

    b=rand(q);   // Bob's random number
    s=rand(q);   // Server's random number

    if (!power_tate(sB,Server,q,cf,Fr,b,res)) cout << "Trouble" << endl;
    if (powl(res,q)!=(ZZn4)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
//    bp=powl(res,b);
    bp=res;
    
    if (!power_tate(Bob,sS,q,cf,Fr,s,res)) cout << "Trouble" << endl;
    if (powl(res,q)!=(ZZn4)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
//    sp=powl(res,s);
    sp=res;

    cout << "Bob's  Key= " << H4(powl(sp,b)) << endl;
    cout << "Server Key= " << H4(powl(bp,s)) << endl;

    return 0;
}

