/*

   Scott's AKE Client/Server testbed

   See http://eprint.iacr.org/2002/164

   Compile as 
   cl /O2 /GX /DZZNS=8 ake12bne.cpp zzn12.cpp zzn6a.cpp ecn2.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib
   using COMBA build

   Barreto-Naehrig curve - Eta pairing

   The curve generated is generated from a 64-bit x parameter

   See "Pairing-Friendly Elliptic Curves of Prime Order", by Paulo S. L. M. Barreto and Michael Naehrig,
   Cryptology ePrint Archive: Report 2005/133
   
   NOTE: Irreducible polynomial is of the form x^6+(1+sqrt(-2))

   See bn.cpp for a program to generate suitable BN curves

   Modified to prevent sub-group confinement attack

   Nov. 2007. Now implements Eta or Tate pairing (Eta is faster)
   See "A Note on the Ate Pairing", by Chang-An Zhao, Fangguo Zhang and Jiwu Huang
   http://eprint.iacr.org/2007/247.pdf, and
   "On compressible pairings and their computation", by Naehrig & Barreto
   http://eprint.iacr.org/2007/429

   Select which pairing below.
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

Miracl precision(8,0); 

// Using SHA-256 as basic hash algorithm

#ifdef MR_COUNT_OPS
extern "C"
{
    int fpc,fpa,fpx,fpm2,fpi2;
}
#endif


#define HASH_LEN 32

//#define TATE
#define ETA


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

void extract(ECn& A,ZZn& x,ZZn& y,ZZn& z)
{ 
    big t;
    x=(A.get_point())->X;
    y=(A.get_point())->Y;
#ifndef MR_AFFINE_ONLY
    t=(A.get_point())->Z;
    if (A.get_status()!=MR_EPOINT_GENERAL) z=1;
    else                                   z=t;
#endif
}

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

ZZn12 line(ECn& A,ECn& C,ZZn& slope,ZZn2& Qx,ZZn2& Qy)
{
     ZZn12 w;
     ZZn6 nn,dd;
     ZZn x,y,z,t;

#ifdef AFFINE   
     dd.set1(Qy);
     extract(A,x,y);
     nn.set(slope*x-y,-slope*Qx);
     w.set(nn,dd);
#endif
#ifdef PROJECTIVE
     extract(A,x,y,z);
     x*=z; t=z; z*=z; z*=t;
     x*=slope; t=slope*z;
     nn.set1(Qx*t);
     nn-=x; t=z;
     extract(C,x,x,z);
     nn+=(z*y); t*=z;
     dd.set1(-t*Qy);
     w.set(nn,dd);
#endif
     return w;
}

//
// Add A=A+B  (or A=A+A) 
// Return line function value
//

ZZn12 g(ECn& A,ECn& B,ZZn2& Qx,ZZn2& Qy)
{
    int type;
    ZZn6 u;
    big ptr;
    ZZn12 r;
    ZZn lam;
    ECn P=A;

// Evaluate line from A
    type=A.add(B,&ptr);
    if (!type) return (ZZn12)1;
    lam=ptr;
    if (A.iszero())   return (ZZn12)1; 
    r=line(P,A,lam,Qx,Qy);
//cout << "r= " << r << endl;
    return r;
}

//
// Eta or Tate Pairing - note denominator elimination has been applied
//
// P is a point of order q. Q(x,y) is a point of order m.q. 
// Note that P is a point on the curve over Fp, Q(x,y) a point on the 
// sextic twist curve over the extension field Fp^2
//

BOOL fast_pairing(ECn& P,ZZn2& Qx,ZZn2& Qy,Big &x,ZZn2 &X,ZZn6& res)
{ 
    ECn A;
    int i,nb;
    Big n,p,q,t,rho;
    ZZn12 w,r,a,b,c,rp;
#ifdef MR_COUNT_OPS
fpc=fpa=fpx=0;
#endif 
    p=36*pow(x,4)-36*pow(x,3)+24*x*x-6*x+1;
    t=6*x*x+1;
    q=p+1-t;

    A=P;           // remember A
#ifdef TATE
    n=q-1;                   // Tate Pairing
#endif
#ifdef ETA
    n = ((t-1)*(t-1))%q - 1; // Eta pairing
#endif
    nb=bits(n);
    r=1;

    for (i=nb-2;i>=0;i--)
    {
        r*=r;   
        r*=g(A,A,Qx,Qy); 
 
        if (bit(n,i)) 
            r*=g(A,P,Qx,Qy);  
    }
#ifdef MR_COUNT_OPS
printf("After tate fpc= %d fpa= %d fpx= %d\n",fpc,fpa,fpx);
fpa=fpc=fpx=0;
#endif
#ifdef TATE
    if (A != -P || r.iszero()) return FALSE;
#endif
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
// r^{p^3}.[c.(r^p)^2.r^{p^2}]^{6x^2+1).c.(r^p.r)^9.a.r^4

//
// Does not require multi-exponentiation, but total exponent length is the same.
// Also does not need precomputation (x is sparse)

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
printf("After final exp fpc= %d fpa= %d fpx= %d\n",fpc,fpa,fpx);
#endif
    res= real(r);
    return TRUE;
}

//
// ecap(.) function
//

BOOL ecap(ECn& P,ECn2& Q,Big& x,ZZn2 &X,ZZn6& r)
{
    BOOL Ok;
    ECn PP=P;
    ZZn2 Qx,Qy;

    normalise(PP);
    Q.get(Qx,Qy);

    Ok=fast_pairing(PP,Qx,Qy,x,X,r);

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
    ECn2 B6,Server,sS;
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

//    for (i=0;i<10000;i++)
  
    if (!ecap(sA,Server,x,X,res)) cout << "Trouble" << endl;
    if (powl(res,q)!=(ZZn6)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    ap=powl(res,a);

    if (!ecap(Alice,sS,x,X,res)) cout << "Trouble" << endl;
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

    if (!ecap(sB,Server,x,X,res)) cout << "Trouble" << endl;
    if (powl(res,q)!=(ZZn6)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    bp=powl(res,b);

    if (!ecap(Bob,sS,x,X,res)) cout << "Trouble" << endl;
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

