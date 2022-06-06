/*

   Scott's AKE Client/Server testbed

   See http://eprint.iacr.org/2002/164

   Compile as 
   cl /O2 /GX /DZZNS=16 ake2cpt2.cpp ecn2.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib
   using COMBA build

   Cocks-Pinch curve - Type 2 Tate pairing

   Requires file k2.ecs which contains details of non-supersingular 
   elliptic curve, with order divisible by q=2^159+2^17+1, and security 
   multiplier k=2. The prime p is 512 bits

   CHANGES: This version uses a Type 2 pairing
            Use Lucas functions to evaluate powers.
            Output of tate pairing is now a ZZn (half-size)

  Modified to prevent sub-group confinement attack

  For a Type 2 curve ecap(P,Q) = e(Trace(P),Q)

  ecap(P,Q) = ecap(Q,P)

*/

#include <iostream>
#include <fstream>
#include "ecn.h"
#include <ctime>
#include "zzn2.h"
#include "ecn2.h"

using namespace std;

Miracl precision(32,0); 

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

//
// Line from A to destination C. Let A=(x,y)
// Line Y-slope.X-c=0, through A, so intercept c=y-slope.x
// Line Y-slope.X-y+slope.x = (Y-y)-slope.(X-x) = 0
// Now evaluate at Q -> return (Qy-y)-slope.(Qx-x)
//

ZZn2 line(ECn& A,ECn& C,ZZn& slope,ZZn2& Qx,ZZn2& Qy)
{ 
    ZZn2 w=Qy,n=Qx;
    ZZn x,y,z,t;

#ifdef AFFINE
    extract(A,x,y);
    n-=x; n*=slope;  
    w-=y; w-=n;

#endif
#ifdef PROJECTIVE
    extract(A,x,y,z);
    x*=z; t=z; z*=z; z*=t;
    n*=z; 
    n-=x;    
    w*=z;; w-=y;
    extract(C,x,y,z);   // only need z - its the denominator of the slope
    w*=z; 
    n*=slope;
    w-=n;  

#endif
    return w;
}

/*

ZZn2 vertical(ECn& A,ZZn2& Qx)
{
    ZZn2 n=Qx;
    ZZn x,y,z;
#ifdef AFFINE
    extract(A,x,y);
    n-=x;
#endif
#ifdef PROJECTIVE
    extract(A,x,y,z);
    z*=z;                    
    n*=z; n-=x; 
#endif
    return n;
}

*/

//
// Add A=A+B  (or A=A+A) 
// Bump up num
//

ZZn2 g(ECn& A,ECn& B,ZZn2& Qx,ZZn2& Qy)
{
    int type;
    ZZn  lam;
    ECn P=A;
    big ptr;

// Evaluate line from A - lam is line slope
    type=A.add(B,&ptr);
    if (!type)   return (ZZn2)1; 
    lam=ptr;   // in projective case slope = lam/A.z

    return line(P,A,lam,Qx,Qy) /* *conj(vertical(A,Qx)) */ ;
}

ECn2 Trace2(ECn2 P)
{
    ECn R;
    ECn2 Q;
    ZZn2 x,y;
    Big X,Y;
    P.get(x,y);
    x=conj(x); y=conj(y);
    Q.set(x,y);
    P+=Q;
    P.norm();
    return P;
}

//
// Tate Pairing - note denominator elimination has been applied
//
// P is a point of order q. Q(x,y) is a point of order q. 
// Note that P is a point on the curve over Fp, Q(x,y) a general point on the 
// curve E(Fp^2)
//

BOOL tate(ECn& P,ECn2& Q,Big& q,ZZn& r)
{ 
    int i,nb,qnr;
    ZZn2 res;
    ZZn2 Qx,Qy;
    Big p,x,y;
    ECn A;
    ECn2 NQ,TQ;

    p=get_modulus();

// Note that q is fixed - q.P=2^17*(2^142.P + P) + P

// Now set Q = kQ-Tr(Q) so we can use denominator elimination
    normalise(P);
    Q.norm();
    NQ=Q;
 
    NQ=2*NQ;
    TQ=Trace2(Q);
    NQ-=TQ;
    NQ.get(Qx,Qy);

    A=P;           // remember A  
    nb=bits(q);
    res=1;

    for (i=nb-2;i>=0;i--)
    {
        res*=res;    // 2 modmul
        res*=g(A,A,Qx,Qy); 
 
        if (bit(q,i)) 
            res*=g(A,P,Qx,Qy);  // executed just once
    }

    if (!A.iszero() || res.iszero()) return FALSE;
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

// hash to a *general* point of order q on E(F_p^2)

ECn2 hash2(char *ID)
{
    ECn2 T;
    ZZn2 x;
    Big x0,y0=1;  // Note y0 !=0

    x0=H1(ID);
    do
    {
        x.set(x0,y0);
        x0+=1;
    }
    while (!is_on_curve(x));
    T.set(x);

    return T;
}     

// Trace function

ECn Trace(ECn2 P)
{
    ECn R;
    ECn2 Q;
    ZZn2 x,y;
    Big X,Y;
    P.get(x,y);
    x=conj(x); y=conj(y);
    Q.set(x,y);
    P+=Q;

    P.get(x,y);
    X=real(x);
    Y=real(y);
    R.set(X,Y);
    return R;
}

/* Note that if #E(Fp)  = p+1-t
           then #E(Fp2) = (p+1-t)(p+1+t) (a multiple of #E(Fp))
           (Weil's Theorem)
 */

int main()
{
    ifstream common("k2.ecs");      // elliptic curve parameters
    miracl* mip=&precision;
    ECn2 Alice,Bob,sA,sB,Ps,Pt;
    ECn2 Server,sS;
    ECn T;
    ZZn res,sp,ap,bp;
    Big r,a,b,s,ss,p,q,n,x,y,B,cof,cf1,cf2,t;
    int nbits,A,qnr;
    time_t seed;

    common >> nbits;
    mip->IOBASE=16;
    common >> p;
    common >> A;
    common >> B;
    common >> cof;
    common >> q;

    time(&seed);
//    seed=1;
    irand((long)seed);

//
// initialise twisted curve...
// Server ID is hashed to points on this
//

    modulo(p);

    mip->IOBASE=16;
    t=p+1-cof*q;

    cf1=(p+1-t)/q;
    cf2=(p+1+t)/q;

// initialise curve...

#ifdef AFFINE
    ecurve(A,B,p,MR_AFFINE);
#endif
#ifdef PROJECTIVE
    ecurve(A,B,p,MR_PROJECTIVE);
#endif

// hash Identity to curve point

    ss=rand(q);    // TA's super-secret 

    cout << "Mapping Server ID to point on curve" << endl;
    Server=hash2((char *)"Server");


    cout << "Server visits trusted authority" << endl;
    sS=ss*Server; 
    sS*=cf1; sS*=cf2;

    cout << "Mapping Alice & Bob ID's to points on curve" << endl;

    Alice=hash2((char *)"Alice");
    Bob=hash2((char *)"Bob");

// Alice, Bob are points of order q

    cout << "Alice and Bob visit Trusted Authority" << endl;

    sA=ss*Alice;
    sA*=cf1; sA*=cf2;
    sB=ss*Bob;
    sB*=cf1; sB*=cf2;

    cout << "Alice and Server Key exchange" << endl;

    a=rand(q);   // Alice's random number
    s=rand(q);   // Server's random number

    T=Trace(sA); T*=cf1;
    if (!tate(T,Server,q,res)) cout << "Trouble" << endl; 

    if (powl(res,q)!=(ZZn)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    ap=powl(res,a);

    T=Trace(Alice); T*=cf1;
    if (!tate(T,sS,q,res))  cout << "Trouble" << endl;

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

    T=Trace(sB); T*=cf1;
    if (!tate(T,Server,q,res)) cout << "Trouble" << endl;
    if (powl(res,q)!=(ZZn)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    bp=powl(res,b);

    T=Trace(Bob); T*=cf1;
    if (!tate(T,sS,q,res)) cout << "Trouble" << endl;
    sp=powl(res,s);

    cout << "Bob    Key= " << H2(powl(sp,b)) << endl;
    cout << "Server Key= " << H2(powl(bp,s)) << endl;

    cout << "Alice and Bob's attempted Key exchange" << endl;

    T=Trace(sB); T*=cf1;
    if (!tate(T,Alice,q,res)) cout << "Trouble" << endl;
    if (powl(res,q)!=(ZZn)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    bp=powl(res,b);

    T=Trace(Bob); T*=cf1;
    if (!tate(T,sA,q,res)) cout << "Trouble" << endl;
    ap=powl(res,a);

    cout << "Alice  Key= " << H2(powl(bp,a)) << endl;
    cout << "Bob    Key= " << H2(powl(ap,b)) << endl;

    return 0;
}
