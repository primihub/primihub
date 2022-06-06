/*

   Scott's AKE Client/Server testbed

   See http://eprint.iacr.org/2002/164

   Compile as 
   cl /O2 /GX /DZZNS=8 ake12bna.cpp zzn12.cpp zzn6a.cpp ecn2.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib
   using COMBA build

   Barreto-Naehrig Curve - Ate pairing

   The curve generated is generated from a 64-bit x parameter
   This version implements the Ate pairing

   See "Pairing-Friendly Elliptic Curves of Prime Order", by Paulo S. L. M. Barreto and Michael Naehrig,
   Cryptology ePrint Archive: Report 2005/133

   NOTE: Irreducible polynomial is of the form x^6+(1+sqrt(-2))

   See bn.cpp for a program to generate suitable BN curves

   Modified to prevent sub-group confinement attack
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
// Ate Pairing Code
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

ZZn12 line(ECn2& A,ECn2& C,ZZn2& slope,ZZn& Qx,ZZn& Qy)
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
    ZZn2 Z,Z2,ZZ,ZZZ;

    A.get(X,Y,Z);
    C.getZ(Z2);

    ZZ=Z*Z;
    ZZZ=ZZ*Z;

    dd.set((ZZZ*slope)*Qx,Z2*Y-Z*X*slope);
    nn.set((ZZn2)-(ZZZ*Z2)*Qy);
    w.set(nn,dd);

#endif

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

}

//
// Add A=A+B  (or A=A+A) 
// Return line function value
//

ZZn12 g(ECn2& A,ECn2& B,ZZn& Qx,ZZn& Qy)
{
    ZZn2 lam;
    ZZn12 r;
    ECn2 P=A;
 //   int fpcb=fpc;

// Evaluate line from A
    A.add(B,lam);
//cout << "point addition/doubling= " << fpc-fpcb << endl;
    if (A.iszero())   return (ZZn12)1; 
//fpcb=fpc;
    r=line(P,A,lam,Qx,Qy);
//cout << "line calculation= " << fpc-fpcb << endl;
//cout << "r= " << r << endl;
    return r;
}

//
// Ate Pairing - note denominator elimination has been applied
//
// P is a point of order q. Q(x,y) is a point of order q. 
// Note that P is a point on the sextic twist of the curve over Fp^2, Q(x,y) is a point on the 
// curve over the base field Fp
//

BOOL fast_pairing(ECn2& P,ZZn& Qx,ZZn& Qy,Big &x,ZZn2 &X,ZZn6& res)
{ 
    ECn2 A;
    int i,nb;
    Big n;
    ZZn12 w,r,a,b,c,rp;

    n=6*x*x;       // t-1
    A=P;           // remember A
 
    nb=bits(n);
    r=1;
#ifdef MR_COUNT_OPS
fpc=fpa=fpx=0;
#endif
    for (i=nb-2;i>=0;i--)
    {
        r*=r;  
        r*=g(A,A,Qx,Qy);
 
        if (bit(n,i)) 
            r*=g(A,P,Qx,Qy);  
    }
#ifdef MR_COUNT_OPS
cout << "Miller fpc= " << fpc << endl;
cout << "Miller fpa= " << fpa << endl;
cout << "Miller fpx= " << fpx << endl;
fpa=fpc=fpx=0;
#endif
    if (r.iszero()) return FALSE;

    w=r;

    r.conj();
    r/=w;    // r^(p^6-1)

    r.mark_as_unitary();

    w=r;
    r.powq(X); r.powq(X);
    r*=w;    // r^[(p^6-1)*(p^2+1)]

// New idea..

//1. Calculate a=r^(6x-5)
//2. Calculate b=a^p using Frobenius
//3. Calculate c=ab
//4. Calculate r^p, r^{p^2} and r^{p^3} using Frobenius
//5. Calculate final exponentiation as 
// r^{p^3}.[c.(r^p)^2.r^{p^2}]^(6x^2+1).c.(r^p.r)^9.a.r^4
//
// Does not require multi-exponentiation, but total exponent length is the same.
// Also does not need precomputation (x is sparse). 
//

    if (x>0) a=pow(r,6*x-5);
    else     a=inverse(pow(r,5-6*x)); // inverses are "free" for unitary values

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
    Big a,b,s,ss,p,q,x,y,B,cf,t;
    int i,bits,A;
    time_t seed;

    mip->IOBASE=16;
    x= (char *)"600000000000219B";  // found by BN.CPP
    
    p=36*pow(x,4)-36*pow(x,3)+24*x*x-6*x+1;
    t=6*x*x+1;
    q=p+1-t;
    cf=p-1+t;
    modulo(p);
    set_frobenius_constant(X);

    cout << "Initialised... " << endl;

    time(&seed);
    irand((long)seed);

#ifdef AFFINE
    ecurve((Big)0,(Big)3,p,MR_AFFINE);
#endif
#ifdef PROJECTIVE
    ecurve((Big)0,(Big)3,p,MR_PROJECTIVE);
#endif

    mip->IOBASE=16;
    mip->TWIST=MR_SEXTIC_D;   // map Server to point on twisted curve E(Fp2)

    ss=rand(q);    // TA's super-secret 

    cout << "Mapping Server ID to point" << endl;
    Server=hash_and_map2((char *)"Server");
    cofactor(Server,X,t);   // fast multiplication by cf

    cout << "Mapping Alice & Bob ID's to points" << endl;
    Alice=hash_and_map((char *)"Alice");
    Bob=  hash_and_map((char *)"Robert");

    cout << "Alice, Bob and the Server visit Trusted Authority" << endl; 

    sS=ss*Server; 
    sA=ss*Alice; 
    sB=ss*Bob; 

    cout << "Alice and Server Key Exchange" << endl;

    a=rand(q);   // Alice's random number
    s=rand(q);   // Server's random number

   // for (i=0;i<1000;i++)
   
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

