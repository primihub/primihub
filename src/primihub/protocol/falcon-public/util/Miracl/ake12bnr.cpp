/*

   Scott's AKE Client/Server testbed

   See http://eprint.iacr.org/2002/164

   Compile as 
   cl /O2 /GX /DZZNS=8 ake12bnr.cpp zzn12.cpp zzn6a.cpp ecn2.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib
   using COMBA build

   Using g++, compile as

   g++ -O2 -DZZNS=4  ake12bnr.cpp zzn12.cpp zzn6a.cpp ecn2.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.a -o ake12bnr

   Barreto-Naehrig curve - R-ate pairing 

   The curve generated is generated from a 64-bit x parameter

   See "Efficient and Generalized pairing Computation on Abelian Varieties", by E. Lee, H-S Lee and C-M Park
   Cryptology ePrint Archive: Report 2008/040

   NOTE: Irreducible polynomial is of the form x^6+(2+sqrt(-1))

   See bn.cpp for a program to generate suitable BN curves

   Modified to prevent sub-group confinement attack

   This is implemented using a 2-3-2 tower
*/

#include <iostream>
#include <fstream>
#include <string.h>
#include "ecn.h"
#include <ctime>
#include "ecn2.h"
#include "zzn12.h"

// cofactor - number of points on curve=CF.q

using namespace std;

#ifdef MR_COUNT_OPS
extern "C"
{
    int fpc=0;
    int fpa=0;
    int fpx=0;
	int fpm2=0;
	int fpi2=0;
}
#endif

Miracl precision(8,0); 

#ifdef MR_AFFINE_ONLY
    #define AFFINE
#else
    #define PROJECTIVE
#endif

// Using SHA-256 as basic hash algorithm

#define HASH_LEN 32

//
// R-ate Pairing Code
//

void set_frobenius_constant(ZZn2 &X)
{
    Big p=get_modulus();
    switch (get_mip()->pmod8)
    {
    case 5:
         X.set((Big)0,(Big)1); // = (sqrt(-2)^(p-1)/2     
         break;
    case 3:                     // = (1+sqrt(-1))^(p-1)/2                                
         X.set((Big)1,(Big)1);      
         break;
   case 7: 
         X.set((Big)2,(Big)1); // = (2+sqrt(-1))^(p-1)/2
    default: break;
    }
    X=pow(X,(p-1)/6);
}

//
// Line from A to destination C. Let A=(x,y)
// Line Y-slope.X-c=0, through A, so intercept c=y-slope.x
// Line Y-slope.X-y+slope.x = (Y-y)-slope.(X-x) = 0
// Now evaluate at Q -> return (Qy-y)-slope.(Qx-x)
//

ZZn12 line(ECn2& A,ECn2& C,ECn2&B,ZZn2& slope,ZZn2& extra,BOOL Doubling,ZZn& Qx,ZZn& Qy)
{
     ZZn12 w;
     ZZn6 nn,dd;
     ZZn2 X,Y;

#ifdef AFFINE
     A.get(X,Y);

     dd.set(slope*Qx,Y-slope*X);
     nn.set((ZZn2)-Qy);
     w.set(nn,dd);

#endif
#ifdef PROJECTIVE
    ZZn2 Z3;

    C.getZ(Z3);

// Thanks to A. Menezes for pointing out this optimization...
    if (Doubling)
    {
        ZZn2 Z,ZZ;
        A.get(X,Y,Z);
        ZZ=Z; ZZ*=ZZ;
        dd.set(-(ZZ*slope)*Qx,slope*X-extra);
        nn.set((Z3*ZZ)*Qy);
    }
    else
    {
        ZZn2 X2,Y2;
        B.get(X2,Y2);
        dd.set(-slope*Qx,slope*X2-Y2*Z3);
        nn.set(Z3*Qy);  
    }
    w.set(nn,dd);
#endif
//cout << "w= " << w << endl;
     return w;
}

//
// fast multiplication by p-1+t
// We know F^2-tF+p = 0
// So p.S=t.F(S)-F^2(S), where F is Frobenius Endomorphism
// So (p-1+t).S = t(F(S)+S)-F^2(S)-S
// This is just multiplication by t, which is half size of (p-1+t)
//

void cofactor(ECn2& S,ZZn2 &F,Big& t)
{
    ZZn2 x,y,w,z;
    ZZn6 h,l,W;
    ECn2 K,T;

    K=S;
	z=F;
	w=F*F;
    S.get(x,y);
    x=w*conj(x);
    y=z*w*conj(y);
    S.set(x,y);
    x=w*conj(x);
    y=z*w*conj(y);
    T.set(x,y);

    S+=K;
    S*=t;

    S-=T;
    S-=K;
	S.norm();


// First "untwist" the point A to (X,Y) where X,Y in F_p^{12}

/*
    K=S;
    ZZn12 X,Y,X2,Y2;
    S.get(x,y);

    h.clear();
    l.set1(x);
    X.set(l,h);

    l.clear();
    h.set1(y);
    Y.set(l,h);


// Apply the Frobenius..
    X.powq(F);
    Y.powq(F);

    X2=X; X2.powq(F);
    Y2=Y; Y2.powq(F);

// Now "twist" it back to S

    X.get(l,h);
    l.get1(x);

    Y.get(l,h);
    h.get1(y);
    S.set(x,y);

// untwist unto T

    X2.get(l,h);
    l.get1(x);

    Y2.get(l,h);
    h.get1(y);

    T.set(x,y);

    S+=K;
    S*=t;
    S-=T;
    S-=K;
*/
}

void q_power_frobenius(ECn2 &A,ZZn2 &F)
{ 
// Fast multiplication of A by q (for Trace-Zero group members only)
    ZZn2 x,y,z,w,r;

// Faster method

#ifdef AFFINE
    A.get(x,y);
#else
    A.get(x,y,z);
#endif

	w=F*F;
	r=F;

    x=w*conj(x);
    y=r*w*conj(y);

#ifdef AFFINE
    A.set(x,y);
#else
    z=conj(z);
    A.set(x,y,z);
#endif


/*

// First "untwist" the point A to (X,Y) where X,Y in F_p^{12}
    A.get(x,y);
    h.clear();
    l.set1(x);
    X.set(l,h);

    l.clear();
    h.set1(y);
    Y.set(l,h);

// Apply the Frobenius..

    X.powq(F);
    Y.powq(F);

// Now "twist" it back to A

    X.get(l,h);
    l.get1(x);

    Y.get(l,h);
    h.get1(y);

    A.set(x,y);

*/
}

//
// Add A=A+B  (or A=A+A) 
// Return line function value
//

ZZn12 g(ECn2& A,ECn2& B,ZZn& Qx,ZZn& Qy)
{
    ZZn2 lam,extra;
    ZZn12 r;
    ECn2 P=A;
    BOOL Doubling;

// Evaluate line from A
    Doubling=A.add(B,lam,extra);

    if (A.iszero())   return (ZZn12)1; 
    r=line(P,A,B,lam,extra,Doubling,Qx,Qy);

    return r;
}

//
// R-ate Pairing - note denominator elimination has been applied
//
// P is a point of order q. Q(x,y) is a point of order q. 
// Note that P is a point on the sextic twist of the curve over Fp^2, Q(x,y) is a point on the 
// curve over the base field Fp
//

BOOL fast_pairing(ECn2& P,ZZn& Qx,ZZn& Qy,Big &x,ZZn2 &X,ZZn6& res)
{ 
    ECn2 A,KA;
    ZZn2 AX,AY;
    int i,nb;
    Big n;
    ZZn12 r,w;
    ZZn12 t0,t1;
    ZZn12 x0,x1,x2,x3,x4,x5;

#ifdef MR_COUNT_OPS
fpc=fpa=fpx=0;
#endif

    if (x>0) n=6*x+2;
    else     n=-3-6*x;

    A=P;
    nb=bits(n);
    r=1;

// Short Miller loop


    for (i=nb-2;i>=0;i--)
    {
        r*=r; 
		r*=g(A,A,Qx,Qy);
        if (bit(n,i)) 
            r*=g(A,P,Qx,Qy);
    }

// a small amount of extra work..
    t0=r;
    KA=A;
    if (x>0)
    {
        r*=g(A,P,Qx,Qy);
        r.powq(X);
        r*=t0;
    }
    else
    {
        r.powq(X);
        r*=t0;
        r*=g(KA,P,Qx,Qy);
    }

    q_power_frobenius(A,X);  // A*=(t-1)

    KA.norm();
    r*=g(A,KA,Qx,Qy);

#ifdef MR_COUNT_OPS
cout << "Miller fpc= " << fpc << endl;
cout << "Miller fpa= " << fpa << endl;
fpa=fpc=fpx=0;
#endif
    if (r.iszero()) return FALSE;

// The final exponentiation

    t0=r;

    r.conj();
    r/=t0;    // r^(p^6-1)

    r.mark_as_unitary();  // from now on all inverses are just conjugates !!

    t0=r;

    r.powq(X); 

	r.powq(X);
    r*=t0;    // r^[(p^6-1)*(p^2+1)]

// Newer new idea...
// See "On the final exponentiation for calculating pairings on ordinary elliptic curves" 
// Michael Scott and Naomi Benger and Manuel Charlemagne and Luis J. Dominguez Perez and Ezekiel J. Kachisa 

    t1=pow(r,-x);  // x is sparse..

    t0=r;    t0.powq(X);
    x0=t0;   x0.powq(X);

    x0*=(r*t0);
    x0.powq(X);

    x1=inverse(r);  // just a conjugation!

    x3=t1; x3.powq(X);
    x4=t1;

    t1=pow(t1,-x);

    x2=t1; x2.powq(X); 
    x4/=x2;
   
    x2.powq(X);

    x5=inverse(t1);
	
    t0=pow(t1,-x);
    t1=t0; t1.powq(X); t0*=t1;

    t0*=t0;
    t0*=x4;
    t0*=x5;
    t1=x3*x5;
    t1*=t0;
    t0*=x2;
    t1*=t1;
    t1*=t0;
    t1*=t1;
    t0=t1*x1;
    t1*=x0;
    t0*=t0;
    t0*=t1;

#ifdef MR_COUNT_OPS
cout << "FE fpc= " << fpc << endl;
cout << "FE fpa= " << fpa << endl;
cout << "FE fpx= " << fpx << endl;
fpa=fpc=fpx=0;
#endif

    res= real(t0);                    // compress to half size...

    return TRUE;

/*

// New idea..

//1. Calculate a=1/r^(6x+5)
//2. Calculate b=a^p using Frobenius
//3. Calculate c=ab
//4. Calculate r^p, r^{p^2} and r^{p^3} using Frobenius
//5. Calculate final exponentiation as 
// r^{p^3}.[c.(r^p)^2.r^{p^2}]^{6x^2+1).c.(r^p.r)^9.a.r^4
//
// Does not require multi-exponentiation, but total exponent length is the same.
// Also does not need precomputation (x is sparse). 
//

    if (x<0) 
        a=pow(r,-5-6*x);
    else
        a=inverse(pow(r,5+6*x));  // inverses are "free" for unitary values

    b=a; b.powq(X);
    b*=a;
    rp=r; rp.powq(X);
    a*=b;
    w=r; w*=w; w*=w;
    a*=w;
    c=rp*r; w=c; w*=w; w*=w; w*=w; w*=c;
    a*=w; w=(rp*rp);

    rp.powq(X);
    w*=(b*rp);

    c=pow(w,x);
    r=w*pow(c,6*x);  //    r=pow(w,6*x*x+1);   // time consuming bit...

    rp.powq(X); a*=rp;
    r*=a;

#ifdef MR_COUNT_OPS
cout << "FE fpc= " << fpc << endl;
cout << "FE fpa= " << fpa << endl;
cout << "FE fpx= " << fpx << endl;
fpa=fpc=fpx=0;
#endif
    res= real(r);                    // compress to half size...

    return TRUE;

*/
}

//
// ecap(.) function
//

BOOL ecap(ECn2& P,ECn& Q,Big& x,ZZn2 &X,ZZn6& r)
{
    BOOL Ok;
    Big xx,yy;
    ZZn Qx,Qy;

    P.norm();
    Q.get(xx,yy); Qx=xx; Qy=yy;

    Ok=fast_pairing(P,Qx,Qy,x,X,r);

    if (Ok) return TRUE;
    return FALSE;
}

//
// Hash functions
// 

Big H1(char *string)
{ // Hash a zero-terminated string to a number < modulus
    Big h,p;
    char s[HASH_LEN];
    int i,j; 
    sha256 sh;

    shs256_init(&sh);

    for (i=0;;i++)
    {
        if (string[i]==0) break;
        shs256_process(&sh,string[i]);
    }
    shs256_hash(&sh,s);
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

Big H2(ZZn6 x)
{ // Hash an Fp6 to a big number
    sha256 sh;
    ZZn2 u,v,w;
    ZZn h,l;
    Big a,hash,p,xx[6];
    char s[HASH_LEN];
    int i,j,m;

    shs256_init(&sh);
    x.get(u,v,w);
    u.get(l,h);
    xx[0]=l; xx[1]=h;
    v.get(l,h);
    xx[2]=l; xx[3]=h;
    w.get(l,h);
    xx[4]=l; xx[5]=h;

    for (i=0;i<6;i++)
    {
        a=xx[i];
        while (a>0)
        {
            m=a%256;
            shs256_process(&sh,m);
            a/=256;
        }
    }
    shs256_hash(&sh,s);
    hash=from_binary(HASH_LEN,s);
    return hash;
}

// Hash and map a Server Identity to a curve point E_(Fp2)

ECn2 hash_and_map2(char *ID)
{
    int i;
    ECn2 S;
    ZZn2 X;
 
    Big x0=H1(ID);

    forever
    {
        x0+=1;
        X.set((ZZn)0,(ZZn)x0);
//cout << "X= " << X << endl;
        if (!S.set(X)) continue;
        break;
    }

//    cout << "S= " << S << endl;
    return S;
}     

// Hash and map a Client Identity to a curve point E_(Fp) of order q

ECn hash_and_map(char *ID)
{
    ECn Q;
    Big x0=H1(ID);

    while (!Q.set(x0,x0)) x0+=1;
   
    return Q;
}

int main()
{
    miracl* mip=&precision;
    ECn Alice,Bob,sA,sB;
    ECn2 Server,sS;
    ZZn6 sp,ap,bp,res;
	ZZn2 X;
 //   ZZn12 Y;
    Big a,b,s,ss,p,q,x,y,cf,t;
    int i,bits,A,B;
    time_t seed;

    mip->IOBASE=16;
//    x= (char *)"-600000000000219B";  // found by BN.CPP B=3
//    B=3;
        x=(char *)"-4080000000000001"; // Nogami et al.'s curve  B=22
        B=22;
    p=36*pow(x,4)+36*pow(x,3)+24*x*x+6*x+1;
    t=6*x*x+1;
    q=p+1-t;
    cf=p-1+t;
    modulo(p);

	set_frobenius_constant(X);

    cout << "Initialised... " << endl;
//    cout << "sqrt(-3)= " << sqrt((ZZn)-3,p) << endl;
    cout << "p= " << p << endl;

    time(&seed);
    irand((long)seed);

#ifdef AFFINE
    ecurve((Big)0,(Big)B,p,MR_AFFINE);
#endif
#ifdef PROJECTIVE
    ecurve((Big)0,(Big)B,p,MR_PROJECTIVE);
#endif

    mip->IOBASE=16;
    mip->TWIST=MR_SEXTIC_D;   // map Server to point on twisted curve E(Fp2)

    ss=rand(q);    // TA's super-secret 

    Server=hash_and_map2((char *)"Server");

    cofactor(Server,X,t);   // fast multiplication by cf

    cout << "Mapping Alice & Bob ID's to points" << endl;
    Alice=hash_and_map((char *)"Alice");
    Bob=  hash_and_map((char *)"Robert");

    sS=ss*Server; 
//for (i=0;i<10000;i++)
    sA=ss*Alice; 
    sB=ss*Bob; 

    cout << "Alice and Server Key Exchange" << endl;

    a=rand(q);   // Alice's random number
    s=rand(q);   // Server's random number

//for (i=0;i<10000;i++)

    if (!ecap(Server,sA,x,X,res)) cout << "Trouble" << endl;
    if (powl(res,q)!=(ZZn6)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    ap=powl(res,a);

    if (!ecap(sS,Alice,x,X,res)) cout << "Trouble" << endl;
    if (powl(res,q)!=(ZZn6)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    sp=powl(res,s);

    cout << "Alice  Key= " << H2(powl(sp,a)) << endl;
    cout << "Server Key= " << H2(powl(ap,s)) << endl;

    cout << "Bob and Server Key Exchange" << endl;

    b=rand(q);   // Bob's random number
    s=rand(q);   // Server's random number

    if (!ecap(Server,sB,x,X,res)) cout << "Trouble" << endl;
    if (powl(res,q)!=(ZZn6)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    bp=powl(res,b);

    if (!ecap(sS,Bob,x,X,res)) cout << "Trouble" << endl;
    if (powl(res,q)!=(ZZn6)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    sp=powl(res,s);

    cout << "Bob's  Key= " << H2(powl(sp,b)) << endl;
    cout << "Server Key= " << H2(powl(bp,s)) << endl;

    return 0;
}

