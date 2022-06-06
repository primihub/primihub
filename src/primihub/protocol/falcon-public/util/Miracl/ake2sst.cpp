/*
   Scott's AKE Client/Server testbed

   See http://eprint.iacr.org/2002/164

   Compile as 
   cl /O2 /GX /DZZNS=16 ake2sst.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib
   using COMBA build

   Super-Singular curve - Tate pairing

   Requires file k2ss.ecs which contains details of supersingular 
   elliptic curve, with order divisible by q=2^159+2^17+1, and security 
   multiplier k=2. The prime p is 512 bits

   CHANGES: Use twisted curve to avoid ECn2 arithmetic completely
            Use Lucas functions to evaluate powers.
            Output of tate pairing is now a ZZn (half-size)

  
  Modified to prevent sub-group confinement attack

*/

#include <iostream>
#include <fstream>
#include "ecn.h"
#include <ctime>
#include "zzn2.h"

using namespace std;

Miracl precision(16,0); 

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

ZZn2 line(ECn& A,ECn& C,ECn& B,int type,ZZn& slope,ZZn& ex1,ZZn& ex2,ZZn& a,ZZn& d)
{ 
    ZZn2 w;
    ZZn x,y,z,t,n=a;

#ifdef AFFINE
    extract(A,x,y);
    n+=x; n*=slope;  
    w.set(y,-d); w-=n;

#endif
#ifdef PROJECTIVE
     if (type==MR_ADD)
     {
        ZZn x2,y2,x3,z3;
        extract(B,x2,y2);
        extract(C,x3,x3,z3);
        w.set(slope*(a+x2)-z3*y2,z3*d);
        return w;
     }    
     if (type==MR_DOUBLE)
     {
        ZZn x,y,x3,z3;
        extract(A,x,y);
        extract(C,x3,x3,z3);
        w.set(-(slope*ex2)*a-slope*x+ex1,-(z3*ex2)*d);
        return w;
     }
#endif
    return w;
}

//
// Add A=A+B  (or A=A+A) 
// Bump up num
//

ZZn2 g(ECn& A,ECn& B,ZZn& a,ZZn& d)
{
    int type;
    ZZn  lam,extra1,extra2;
    ECn P=A;
    big ptr,ex1,ex2;

// Evaluate line from A - lam is line slope

    type=A.add(B,&ptr,&ex1,&ex2);
    if (!type)   return (ZZn2)1; 
    lam=ptr;   // in projective case slope = lam/A.z
    extra1=ex1;
    extra2=ex2;
    return line(P,A,B,type,lam,extra1,extra2,a,d);
}

//
// Tate Pairing - note denominator elimination has been applied
//
// P is a point of order q. Q(x,y) is a point of order q. 
// Note that P is a point on the curve over Fp, Q(x,y) a point on the 
// curve E(Fp^2) -> Q([Qx,0],[0,Qy])
// Here we have morphed Q onto the twisted curve E'(Fp) 
//

BOOL tate(ECn& P,ECn& Q,Big& q,ZZn& r)
{ 
    int i,nb,qnr;
    ZZn2 res;
    ZZn a,d;
    Big p,x,y,n;
    ECn A;

    p=get_modulus();

// Note that q is fixed - q.P=2^17*(2^142.P + P) + P

    normalise(Q);  // make sure z=1
    extract(Q,a,d);

    qnr=get_mip()->qnr;
    if (qnr==-2)
    {
        a=a/2;   /* Convert off twist */
	    d=d/4;
    }

    normalise(P);
    A=P;           // remember A

    n=q-1;
    nb=bits(n);

    res=1;

    for (i=nb-2;i>=0;i--)
    {
        res*=res;    // 2 modmul
        res*=g(A,A,a,d); 
 
        if (bit(n,i)) 
            res*=g(A,P,a,d);  // executed just once
    }

    if (A != -P || res.iszero()) return FALSE;
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
    ifstream common("k2ss.ecs");      // elliptic curve parameters
    miracl* mip=&precision;
    ECn Alice,Bob,sA,sB;
    ECn Server,sS;
    ZZn res,sp,ap,bp;
    Big r,a,b,s,ss,p,q,x,y,B,cof;
    int i,nbits,A,qnr;
    time_t seed;

    common >> nbits;
    mip->IOBASE=16;
    common >> p;
    common >> A;
    common >> B;
    common >> cof;
    common >> q;

//cout << "p= " << p << endl;
//cout << "q%cof= " << q*cof << endl;

    time(&seed);
    irand((long)seed);

//
// initialise twisted curve...
// Server ID is hashed to points on this
//

    modulo(p);
    qnr=mip->qnr;
#ifdef AFFINE
    ecurve(qnr*qnr*A,qnr*qnr*qnr*B,p,MR_AFFINE);
#endif
#ifdef PROJECTIVE
    ecurve(qnr*qnr*A,qnr*qnr*qnr*B,p,MR_PROJECTIVE);
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

    sA=ss*Alice;
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

    if (!tate(Bob,sA,q,res)) cout << "Trouble" << endl;
    ap=powl(res,a);

    cout << "Alice  Key= " << H2(powl(bp,a)) << endl;
    cout << "Bob    Key= " << H2(powl(ap,b)) << endl;

    return 0;
}
