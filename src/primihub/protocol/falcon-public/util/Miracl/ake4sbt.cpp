/*

   Scott's AKE Client/Server testbed

   See http://eprint.iacr.org/2002/164

   Compile as 
   cl /O2 /GX /DZZNS=8 ake4sbt.cpp zzn4.cpp zzn2.cpp ecn2.cpp big.cpp zzn.cpp 
   ecn.cpp miracl.lib
   Fastest using COMBA build for 256-bit mod-mul

   Scott-Barreto Curve - Tate pairing

   The file kw4.ecs is required 
   Security is G160/F1024 (160-bit group, 1024-bit field)

   Modified to prevent sub-group confinement attack

   NOTE: assumes p = 3 mod 8, p is 256-bits

   **** NEW **** Based on the observation by R. Granger and D. Page and N.P. Smart  in "High Security 
   Pairing-Based Cryptography Revisited" that multi-exponentiation can be used for the final exponentiation
   of the Tate pairing, we suggest the Power Pairing, which calculates E(P,Q,e) = e(P,Q)^e, where the 
   exponentiation by e is basically for free, as it can be folded into the multi-exponentiation.

*/

#include <iostream>
#include <fstream>
#include <string.h>
#include "ecn.h"
#include <ctime>
#include "ecn2.h"
#include "zzn4.h"

using namespace std;

Miracl precision(12,0);   

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

ZZn4 line(ECn& A,ECn& C,ZZn& slope,ZZn2& Qx,ZZn2& Qy)
{ 
    ZZn4 w;
    ZZn2 m=Qx;
    ZZn x,y,z,t;
#ifdef AFFINE
    extract(A,x,y);
    m-=x; m*=slope;  
    w.set((ZZn2)-y,Qy); w-=m;
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

ZZn4 g(ECn& A,ECn& B,ZZn2& Qx,ZZn2& Qy)
{
    int type;
    ZZn  lam;
    big ptr;
    ECn P=A;

// Evaluate line from A
    type=A.add(B,&ptr);
    if (!type)   return (ZZn4)1; 
    lam=ptr;
    return line(P,A,lam,Qx,Qy);
}

//
// Tate Pairing - note denominator elimination has been applied
//
// P is a point of order q. Q(x,y) is a point of order m.q. 
// Note that P is a point on the curve over Fp, Q(x,y) a point on the 
// extension field Fp^2
//
// New! Power Pairing calculates E(P,Q,e) = e(P,Q)^e at no extra cost!
//

BOOL power_tate(ECn& P,ECn2 Q,Big& q,Big *cf,ZZn2 &Fr,Big &e,ZZn2& r)
{ 
    int i,nb;
    ECn A;
    ZZn4 w,res,a[2];
    ZZn2 Qx,Qy;
    Big carry,ex[2],p=get_modulus();
//    ZZn4 Y,X;

    Q.get(Qx,Qy);
//    Qx=-tx(Qx)/2;   // convert from twist to (x,0),(0,y)
//    Qy/=2;

    Qx=txd(Qx);
    Qy=txd(txd(Qy));

// cout << "Qx= " << Qx << endl;
// cout << "Qy= " << Qy << endl;

//    X.set(Qx,(ZZn2)0);
//    Y.set((ZZn2)0,Qy);

//    cout << "Y^2= " << Y*Y << endl;
//    cout << "X^3+AX+B= " << X*X*X+getA()*X+getB() << endl;
    
    res=1;  

/* Left to right method  */
    A=P;
    nb=bits(q);
    for (i=nb-2;i>=0;i--)
    {
        res*=res;           
        res*=g(A,A,Qx,Qy); 
        if (bit(q,i))
            res*=g(A,P,Qx,Qy);
    }

    if (!A.iszero() || res.iszero()) return FALSE;
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

ECn2 hash2(char *ID)
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

// cout << "T= " << T << endl;

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

ZZn2 get_frobenius_constant()
{
    ZZn2 Fr;
    Big p=get_modulus();
    switch (get_mip()->pmod8)
    {
    case 5:
         Fr.set((Big)0,(Big)1); // = (sqrt(-2)^(p-1)/2     
         break;
    case 3:                     // = (1+sqrt(-1))^(p-1)/2
    case 7:                     // = (1+sqrt(-2))^(p-1)/2
         Fr.set((Big)1,(Big)1);                                         
    default: break;
    }
    return pow(Fr,(p-1)/2);
}

#define COF 58

int main()
{
    ifstream common("kw4.ecs");      // elliptic curve parameters
    miracl* mip=&precision;
    ECn Alice,Bob,sA,sB;
    ECn2 Server,sS;
    ZZn2 res,sp,ap,bp,wa,wb,w1,w2;
    ZZn ww;
    ZZn4 w;
    ZZn2 Fr;
    Big a,b,s,ss,p,q,r,B,cof,t,qcof;
    Big cf[2];
                                    // this is read-only
                                    // and never copied.      
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
    cout << "cf= " << cf[0]*p+cf[1] << endl;

//
// Note: COF*q has a low hamming weight for this particular curve - so use this instead..
//

    qcof=q*COF;

    time(&seed);
    irand((long)seed);

#ifdef AFFINE
    ecurve(A,B,p,MR_AFFINE);
#endif
#ifdef PROJECTIVE
    ecurve(A,B,p,MR_PROJECTIVE);
#endif

    Fr=get_frobenius_constant();

// cout << "qnr= " << get_mip()->qnr << endl;

    mip->IOBASE=16;
    mip->TWIST=MR_QUADRATIC;   // map Server to point on twisted curve E(Fp2)

// hash Identities to curve point

    ss=rand(q);    // TA's super-secret 

    cout << "Mapping Server ID to point" << endl;
    Server=hash2((char *)"Server");

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

    if (!power_tate(sA,Server,qcof,cf,Fr,a,res)) cout << "Trouble" << endl;
    
    if (powl(res,q)!=(ZZn2)1)
    {
        cout << "res= " << res << endl;
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
//    ap=powl(res,a);
    ap=res;

    if (!power_tate(Alice,sS,qcof,cf,Fr,s,res)) cout << "Trouble" << endl;
    if (powl(res,q)!=(ZZn2)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
//    sp=powl(res,s);
    sp=res;

    cout << "Alice  Key= " << H2(powl(sp,a)) << endl;
    cout << "Server Key= " << H2(powl(ap,s)) << endl;

    cout << "Bob and Server Key Exchange" << endl;

    b=rand(q);   // Bob's random number
    s=rand(q);   // Server's random number

    if (!power_tate(sB,Server,qcof,cf,Fr,b,res)) cout << "Trouble" << endl;
    if (powl(res,q)!=(ZZn2)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
//    bp=powl(res,b);
    bp=res;

    if (!power_tate(Bob,sS,qcof,cf,Fr,s,res)) cout << "Trouble" << endl;
    if (powl(res,q)!=(ZZn2)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
//    sp=powl(res,s);
    sp=res;

    cout << "Bob's  Key= " << H2(powl(sp,b)) << endl;
    cout << "Server Key= " << H2(powl(bp,s)) << endl;

    return 0;
}

