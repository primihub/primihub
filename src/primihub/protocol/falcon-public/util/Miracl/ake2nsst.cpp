/*

   Scott's AKE Client/Server testbed

   See http://eprint.iacr.org/2002/164

   This implementation uses an NSS curve - see http://eprint.iacr.org/2005/252

   Compile as 
   cl /O2 /GX /DZZNS=16 ake2nsst.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib
   using COMBA build

   Not-Super Singular curve - Tate pairing

   Requires file bk2.ecs which contains details of non-supersingular 
   elliptic curve, with order divisible by q=r*r+r+1, where r=2^80+2^16, and security 
   multiplier k=2. The prime p is 512 bits

   CHANGES: Use twisted curve to avoid ECn2 arithmetic completely
            Use Lucas functions to evaluate powers.
            Output of tate pairing is now a ZZn (half-size)

   NOTE: Key exchange bandwidth could be reduced by halve using ideas from 
         "Doing more with Fewer Bits", Brouwer, Pellikaan & Verheul, Asiacrypt 
         '99

  Modified to prevent sub-group confinement attack

*/

#include <iostream>
#include <fstream>
#include "ecn.h"
#include <ctime>
#include "zzn2.h"

using namespace std;

Miracl precision(18,0); 

// Using SHA-512 as basic hash algorithm

#define HASH_LEN 64

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

ZZn beta;


//
// Line from A to destination C. Let A=(x,y)
// Line Y-slope.X-c=0, through A, so intercept c=y-slope.x
// Line Y-slope.X-y+slope.x = (Y-y)-slope.(X-x) = 0
// Now evaluate at Q -> return (Qy-y)-slope.(Qx-x)
//
// Calculate the "second half" values at the same time, and store them
//

ZZn2 line(ECn& A,ECn& C,ZZn& slope,ZZn& Qx,ZZn& Qy,ZZn& bQx,ZZn2& w2)
{ 
    ZZn2 w;
    ZZn x,y,z,t;
    ZZn a=Qx; ZZn d=Qy; ZZn a2=bQx;

#ifdef AFFINE
    extract(A,x,y);       
    a-=x; a*=slope;        a2-=x; a2*=slope;
    w.set(-y,d); w-=a;     w2.set(-y,-d); w2-=a2;

#endif
#ifdef PROJECTIVE
    extract(A,x,y,z);
    x*=z; t=z; z*=z; z*=t;
    a*=z;                  a2*=z;
    a-=x;                  a2-=x;   
    z*=d; w.set(-y,z);     w2.set(-y,-z);
    extract(C,x,y,z);   // only need z - its the denominator of the slope
    w*=z;                  w2*=z;
    a*=slope;              a2*=slope;
    w-=a;                  w2-=a2;
#endif
    return w;
}

//
// Add A=A+B  (or A=A+A) 
// Bump up num
//

ZZn2 g(ECn& A,ECn& B,ZZn& Qx,ZZn& Qy,ZZn& bQx,ZZn2& w2)
{
    int type;
    ZZn  lam;
    ECn P=A;
    big ptr;

// Evaluate line from A - lam is line slope
    type=A.add(B,&ptr);
    if (!type)    return (ZZn2)1;
    lam=ptr;   // in projective case slope = lam/A.z
    return line(P,A,lam,Qx,Qy,bQx,w2);    
}

ECn Phi(ECn A,ZZn beta)
{
    ZZn xx,yy;
  
    extract(A,xx,yy);
    xx=beta*xx; yy=yy;
    A.set(xx,yy);
    return A;
}

//
// Tate Pairing - note denominator elimination has been applied
//
// P is a point of order q. Q(x,y) is a point of order q. 
// Note that P is a point on the curve over Fp, Q(x,y) a point on the 
// curve E(Fp^2) -> Q([Qx,0],[0,Qy])
// Here we have morphed Q onto the twisted curve E'(Fp) 
//

BOOL tate(ECn& P,ECn Q,Big& q,ZZn& r)
{ 
    int i,nb,k;
    ZZn2 res,resb,res1,res2;
    ZZn2 store[82];
    ZZn xx,yy,Qx,Qy,bQx;
    Big p,x,y,n,lambda;
    ECn W,A,B;

    p=get_modulus();

    normalise(Q);  // make sure z=1
    extract(Q,Qx,Qy);
    Qx=-Qx;  // untwist
    bQx=beta*Qx;

    A=P;           // remember P
    res=res1=res2=1;
    k=0;
/*
    for (i=64;i>=1;i--)
    {
        res*=res;
        res*=g(A,A,Qx,Qy,bQx,store[k++]);
    }
    res*=g(A,P,Qx,Qy,bQx,store[k++]);
    for (i=16;i>=1;i--)
    {
        res*=res;
        res*=g(A,A,Qx,Qy,bQx,store[k++]);
    }
    res*=g(A,P,Qx,Qy,bQx,store[k++]);

    resb=res;

    k=0;

    for (i=64;i>=1;i--)
    {
        res*=res;
        res*=store[k++];                   //g(A,A,Q,beta,store[k++]);
    }
    res*=store[k++];                       //g(A,P,Q,beta,store[k++]);

    res*=resb;     // This is easy to overlook!

    for (i=16;i>=1;i--)
    {
        res*=res;
        res*=store[k++];                   // g(A,A,Q,beta,store[k++]);
    }
*/

//    lambda=pow((Big)2,80)+pow((Big)2,16);

    for (i=64;i>=1;i--)
    {
        res1*=res1;
        res2*=res2;
        res1*=g(A,A,Qx,Qy,bQx,store[0]);
        res2*=store[0];
    }
    res1*=g(A,P,Qx,Qy,bQx,store[0]);
    res2*=store[0];
    for (i=16;i>=1;i--)
    {
        res1*=res1;
        res2*=res2;
        res1*=g(A,A,Qx,Qy,bQx,store[0]);
        res2*=store[0];
    }

    res1*=g(A,P,Qx,Qy,bQx,store[0]);

//    res1=pow(res1,lambda);

    resb=res1;
    for (i=64;i>=1;i--) res1*=res1;
    res1*=resb;
    for (i=16;i>=1;i--) res1*=res1;

    res=res1*res2; 

    res=conj(res)/res;          // raise to power of (p-1)
    r=powl(real(res),(p+1)/q);  // raise to power of (p+1)/q

    if (r==1) return FALSE;
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
    sha512 sh;

    shs512_init(&sh);

    for (i=0;;i++)
    {
        if (string[i]==0) break;
        shs512_process(&sh,string[i]);
    }
    shs512_hash(&sh,s);
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

Big H2(ZZn x)
{ // Hash an Fp to a big number
    sha sh;
    Big a,h,p;
    char s[20];
    int m;

    shs_init(&sh);
    a=(Big)x;
    while (a>0)
    {
        m=a%256;
        shs_process(&sh,m);
        a/=256;
    }
    shs_hash(&sh,s);
    h=from_binary(20,s);
    return h;
}

// Hash an Identity to a curve point

ECn Hash(char *ID)
{
    ECn T;
    Big a=H1(ID);

    while (!is_on_curve(a)) a+=1;
    T.set(a);  // Make sure its on E(F_p)

    return T;
}     

// Hash an Identity to a curve point and map to point of order q

ECn hash_and_map(char *ID,Big cof)
{
    ECn T;
    Big a=H1(ID);

    while (!is_on_curve(a)) a+=1;
    T.set(a);

    T*=cof;

    return T;
}

/* Note that if #E(Fp)  = p+1-t
           then #E(Fp2) = (p+1-t)(p+1+t) (a multiple of #E(Fp))
           (Weil's Theorem)
 */

int main()
{
    ifstream common("bk2.ecs");      // elliptic curve parameters
    miracl* mip=&precision;
    ECn Alice,Bob,sA,sB;
    ECn Server,sS;
    ZZn res,sp,ap,bp;
    Big r,a,b,s,ss,p,q,x,y,B,cof;
    int i,nbits,A;
    time_t seed;

    common >> nbits;
    mip->IOBASE=16;
    common >> p;
    common >> A;
    common >> B;
    common >> cof;
    common >> q;

    time(&seed);
    irand((long)seed);

//    r=pow((Big)2,80)+pow((Big)2,16);
//    q=r*r+r+1 

// must get the right beta - recall that there are two solutions for lambda..
    i=3;
    do
    {
        beta=(ZZn)pow((Big)++i,(p-1)/3,p);
    } while (beta==1);
    
//
// initialise twisted curve...
// Server ID is hashed to points on this
//

#ifdef AFFINE
    ecurve(A,-B,p,MR_AFFINE);
#endif
#ifdef PROJECTIVE
    ecurve(A,-B,p,MR_PROJECTIVE);
#endif

    mip->IOBASE=16;

// hash Identity to curve point
// Server does not need to be of order q (its order is a multiple of q)

    ss=rand(q);    // TA's super-secret 

    cout << "Mapping Server ID to point on twisted curve" << endl;
    Server=Hash((char *)"Server");

    cout << "Server visits trusted authority" << endl;
    sS=ss*Server; 


// initialise curve...

#ifdef AFFINE
    ecurve(A,B,p,MR_AFFINE);
#endif
#ifdef PROJECTIVE
    ecurve(A,B,p,MR_PROJECTIVE);
#endif

    cout << "Mapping Alice & Bob ID's to points on curve" << endl;

    Alice=hash_and_map((char *)"Alice",cof);
    Bob=  hash_and_map((char *)"Robert",cof);

// Alice, Bob are points of order q

    cout << "Alice and Bob visit Trusted Authority" << endl;

//for (i=0;i<10000;i++)
//    sA=ss*Alice;

/*  Use GLV method - faster */
//cout << "sA= " << sA << endl;
    Big k1,k2,lambda;
    lambda=pow((Big)2,80)+pow((Big)2,16);
    k1=ss%lambda;
    k2=ss/lambda;
//for (i=0;i<100000;i++)
    sA=mul(k1,Alice,k2,Phi(Alice,beta));
//cout << "sA= " << mul(k1,Alice,k2,Phi(Alice,beta)) << endl;

    sB=ss*Bob;

    cout << "Alice and Server Key exchange" << endl;

    a=rand(q);   // Alice's random number
    s=rand(q);   // Server's random number

//for (i=0;i<10000;i++)
    if (!tate(sA,Server,q,res)) cout << "Trouble" << endl; 

    if (powl(res,q)!=(ZZn)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    ap=powl(res,a);

    if (!tate(Alice,sS,q,res))  cout << "Trouble" << endl;

    if (powl(res,q)!=(ZZn)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }

    sp=powl(res,s);

    cout << "Alice  Key= " << H2(powl(sp,a)) << endl;
    cout << "Server Key= " << H2(powl(ap,s)) << endl;

    cout << "Bob and Server Key exchange" << endl;

    b=rand(q);   // Bob's random number
    s=rand(q);   // Server's random number

    if (!tate(sB,Server,q,res)) cout << "Trouble" << endl;
    if (powl(res,q)!=(ZZn)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    bp=powl(res,b);

    if (!tate(Bob,sS,q,res)) cout << "Trouble" << endl;
    sp=powl(res,s);

    cout << "Bob    Key= " << H2(powl(sp,b)) << endl;
    cout << "Server Key= " << H2(powl(bp,s)) << endl;

    cout << "Alice and Bob's attempted Key exchange" << endl;

    if (!tate(Alice,sB,q,res)) cout << "Trouble" << endl;
    if (powl(res,q)!=(ZZn)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    bp=powl(res,b);

    if (!tate(sA,Bob,q,res)) cout << "Trouble" << endl;
    ap=powl(res,a);

    cout << "Alice  Key= " << H2(powl(bp,a)) << endl;
    cout << "Bob    Key= " << H2(powl(ap,b)) << endl;

    return 0;
}

