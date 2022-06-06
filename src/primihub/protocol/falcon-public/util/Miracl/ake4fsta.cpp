/*

   Scott's AKE Client/Server testbed

   See http://eprint.iacr.org/2002/164

   Compile as 
   cl /O2 /GX /DZZNS=8 ake4fsta.cpp zzn4.cpp zzn2.cpp ecn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib
   Fastest using COMBA build for 256-bit mod-mul

   Freeman-Scott-Teske Curve - Ate pairing

   The file nk4.ecs is required 
   Security is G160/F1024 (160-bit group, 1024-bit field)

   Modified to prevent sub-group confinement attack

   NOTE: assumes p = 5 mod 8, p is 256-bits

   **** NEW **** Based on the observation by R. Granger and D. Page and N.P. Smart  in "High Security 
   Pairing-Based Cryptography Revisited" that multi-exponentiation can be used for the final exponentiation
   of the Tate pairing, we suggest the Power Pairing, which calculates E(P,Q,e) = e(P,Q)^e, where the 
   exponentiation by e is basically for free, as it can be folded into the multi-exponentiation.

   **** NEW **** ATE pairing implementation - see The Eta Pairing revisited by Hess, Smart and Vercauteren

*/

#include <iostream>
#include <fstream>
#include <string.h>
#include "ecn.h"
#include <ctime>
#include "ecn2.h"
#include "zzn4.h"

using namespace std;

Miracl precision(16,0);   

// Using SHA-1 as basic hash algorithm

#define HASH_LEN 20

//
// Define one or the other of these
//
// Which is faster depends on the I/M ratio - See imratio.c
// Roughly if I/M ratio > 16 use PROJECTIVE, otherwise use AFFINE
//


// This program works with AFFINE only..

#define AFFINE

//
// Ate Pairing Code
//
// Extract ECn point in internal ZZn format
//

void extract(ECn& A,ZZn& x,ZZn& y)
{ 
    x=(A.get_point())->X;
    y=(A.get_point())->Y;
}

//
// Get x/i^2, y/i^4, where i is 4th root of -2
//

void untwist(ECn2& P,ZZn2& U,ZZn2& V)
{
    P.get(U,V);
    U=-tx(U)/2;
    V=-V/2;
}

//
// Line from A to destination C. Let A=(x,y)
// Line Y-slope.X-c=0, through A, so intercept c=y-slope.x
// Line Y-slope.X-y+slope.x = (Y-y)-slope.(X-x) = 0
// Now evaluate at Q -> return (Qy-y)-slope.(Qx-x)
//

ZZn4 line(ECn2& A,ZZn2& lam,ZZn& Qx,ZZn& Qy)
{ 
    ZZn4 w;
    ZZn2 x,y,z,t;

    untwist(A,x,y);

    w.set(ZZn2(0,Qy),tx(y)-lam*(x-Qx));

    return w;
}

//
// Add A=A+B  (or A=A+A) 
// Bump up num
//


ZZn4 g(ECn2& A,ECn2& B,ZZn& Qx,ZZn& Qy)
{
    ZZn2  lam;
    ECn2 P=A;

// Evaluate line from A
    A.add(B,lam);
    if (A.iszero())   return (ZZn4)1; 

//cout << "lam= " << lam << endl;

    return line(P,lam,Qx,Qy);
}

//
// Ate Pairing - note denominator elimination has been applied
//
// P is a point of order q. Q(x,y) is a point of order m.q. 
// Note that P is a point on the curve over Fp, Q(x,y) a point on the 
// extension field Fp^2
//
// New! Power Pairing calculates E(P,Q,e) = e(P,Q)^e at no extra cost!
//

BOOL power_pairing(ECn2& P,ECn Q,Big& T,Big *cf,ZZn2 &Fr,Big &e,Big q,ZZn2& r)
{ 
    int i,nb;
    ECn2 A;
    ZZn4 w,res,a[2];
    ZZn Qx,Qy;
    Big carry,ex[2],p=get_modulus();

    extract(Q,Qx,Qy);

    res=1;  

/* Left to right method  */
    A=P;
    nb=bits(T);
    for (i=nb-2;i>=0;i--)
    {
        res*=res;           
        res*=g(A,A,Qx,Qy); 
        if (bit(T,i))
            res*=g(A,P,Qx,Qy);
    }

//    if (!A.iszero() || res.iszero()) return FALSE;

    w=res;
    w.powq(Fr); w.powq(Fr);  // ^(p^2-1)
    res=w/res;

 
    res.mark_as_unitary();

    if (e.isone())
    {
        ex[0]=cf[0];
        ex[1]=cf[1];
    }
    else
    { // cf *= e
        carry=mad(cf[1],e,(Big)0,p,ex[1]);
        mad(cf[0],e,carry,p,ex[0]);
    }

    a[0]=a[1]=res;
    a[0].powq(Fr);
    res=pow(2,a,ex);

    r=real(res);    // compression

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

Big H2(ZZn2 x)
{ // Hash an Fp2 to a big number
    sha sh;
    Big a,u,v;
    char s[HASH_LEN];
    int m;

    shs_init(&sh);
    x.get(u,v);

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

// Hash and map a Server Identity to a curve point E(Fp2)

ECn2 hash2(char *ID,Big cof2)
{
    ECn2 T;
    ZZn2 x;
    Big x0,y0=0;
    x0=H1(ID);
    do
    {
        x.set(x0,y0);
        x0+=1;
    }
    while (!is_on_curve(x));
    T.set(x);
    T*=cof2;
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
         X.set((Big)0,(Big)1); // = (sqrt(-2)^(p-1)/2     
         break;
    case 3:                     // = (1+sqrt(-1))^(p-1)/2
         X.set((Big)1,(Big)1);           
         break;
    case 7:                     // = (2+sqrt(-1))^(p-1)/2
         X.set((Big)2,(Big)1);    
         break;
    default: break;
    }
    X=pow(X,(p-1)/2);
}

int main()
{
    ifstream common("nk4.ecs");      // elliptic curve parameters
    miracl* mip=&precision;
    ECn Alice,Bob,sA,sB;
    ECn2 Server,sS;
    ZZn2 res,sp,ap,bp,wa,wb,w1,w2,Fr;
    ZZn ww;
    ZZn4 w;
    Big a,b,s,ss,p,q,r,B,cof,n,t1;
    Big cf[2];

    int i,bitz,A;
    time_t seed;

    cout << "Started" << endl;
    common >> bitz;
    mip->IOBASE=16;
    common >> p;
    common >> A;
    common >> B;
    common >> cof;   // #E/q
    common >> q;     // low hamming weight q
    common >> cf[0];    // [(p^2+1)/q]/p
    common >> cf[1];    // [(p^2+1)/q]%p

    cout << "Initialised... " << p%8 << endl;

    Big t=p+1-cof*q;
    Big cof2=(p*p+1)/q+(t*t-2*p)/q;

    t1=p-cof*q;  // t-1

    time(&seed);
    irand((long)seed);

    ecurve(A,B,p,MR_AFFINE);

    set_frobenius_constant(Fr);

    mip->IOBASE=16;
    mip->TWIST=TRUE;   // map Server to point on twisted curve E(Fp2)

// hash Identities to curve point

    ss=rand(q);    // TA's super-secret 

    cout << "Mapping Server ID to point" << endl;
    Server=hash2((char *)"Server",cof2);

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

    if (!power_pairing(Server,sA,t1,cf,Fr,a,q,res)) cout << "Trouble" << endl;
    
    if (powl(res,q)!=(ZZn2)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }

    ap=res;

    if (!power_pairing(sS,Alice,t1,cf,Fr,s,q,res)) cout << "Trouble" << endl;
    if (powl(res,q)!=(ZZn2)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }

    sp=res;

    cout << "Alice  Key= " << H2(powl(sp,a)) << endl;
    cout << "Server Key= " << H2(powl(ap,s)) << endl;

    cout << "Bob and Server Key Exchange" << endl;

    b=rand(q);   // Bob's random number
    s=rand(q);   // Server's random number

    if (!power_pairing(Server,sB,t1,cf,Fr,b,q,res)) cout << "Trouble" << endl;
    if (powl(res,q)!=(ZZn2)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    bp=res;

    if (!power_pairing(sS,Bob,t1,cf,Fr,s,q,res)) cout << "Trouble" << endl;
    if (powl(res,q)!=(ZZn2)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    sp=res;

    cout << "Bob's  Key= " << H2(powl(sp,b)) << endl;
    cout << "Server Key= " << H2(powl(bp,s)) << endl;

    return 0;
}

