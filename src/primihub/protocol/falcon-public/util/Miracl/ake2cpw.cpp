/*
   Scott's AKE Client/Server testbed

   See http://eprint.iacr.org/2002/164

   Compile as 
   cl /O2 /GX /DBIGS=18 ake2cpw.cpp zzn2.cpp big.cpp monty.cpp elliptic.cpp miracl.lib
   using COMBA build

   Cocks-Pinch curve - Weil pairing

   Requires file k2.ecs which contains details of non-supersingular 
   elliptic curve, with order divisible by q=2^159+2^17+1, and security 
   multiplier k=2. The prime p is 512 bits

   NOTE: Key exchange bandwidth could be reduced by halve using ideas from 
         "Doing more with Fewer Bits", Brouwer, Pellikaan & Verheul, Asiacrypt 
         '99

*/

#include <iostream>
#include <fstream>
#include <ctime>
#include "ecn.h"
#include "zzn.h"
#include "zzn2.h"

using namespace std;

Miracl precision(34,0); 

// Using SHA-512 as basic hash algorithm

#define HASH_LEN 64

//
// Weil Pairing Code
//
//
// Extract ECn point in internal ZZn format
//

void extract(ECn& A,ZZn& x,ZZn& y)
{ 
    x=(A.get_point())->X;
    y=(A.get_point())->Y;
}

ZZn2 g(ECn& A,ECn& B,ECn& C,ECn& D,ECn& P,ECn& Q)
{
    ZZn x,y,Ax,Ay,Cx,Cy,lam1,lam2;
    ZZn2 u,w;
    big ptr1,ptr2;

    extract(A,Ax,Ay);
    extract(C,Cx,Cy);

    double_add(B,A,D,C,ptr1,ptr2); // adds B to A and D to C
                                   // uses Montgomery's trick
                                   // returns line slopes in ptr1 and ptr2

    if (A.iszero() || C.iszero()) return (ZZn2)1;
    if (ptr1==NULL || ptr2==NULL) return (ZZn2)0;

//
// Recall that Q and C are "really" of the form  [(-x,0),(0,y)]
// The slope of the real curve is -i*slope of the twist
// [(iQy-Ay) - m1(-Qx-Ax)]/(Py-iCy)+i.m2(Px+Cx)
// Instead of division, calculate conjugate and multiply (remember Fermat!)
//

    lam1=ptr1;
    lam2=ptr2;

    extract(Q,x,y);

    Ax+=x;             // numerator
    Ax*=lam1;
    u.set(Ax-Ay,y);

    extract(P,x,y);

    Cx+=x;             // denominator
    Cx*=lam2;

    w.set(y,Cy-Cx);    // conjugate trick !
                       // and don't forget the -i on the slope!
    return (w*u);
} 

//
// New Weil Pairing - note denominator elimination has been applied
//
// nw(P,Q) = [m(P,Q)/m(Q,P)]^(p-1)
//
// P(x,y) is a point of order q. Q(x,y) is a point of order q. 
//

BOOL nw(ECn& P,ECn& Q,Big& q,ZZn& r)
{ 
    ZZn2 m=1;
    int i,nb;
    ECn  A=P;
    ECn  B=Q;

    nb=bits(q);
    for (i=nb-2;i>=0;i--)
    { // one loop !
        m*=m;
        m*=g(A,A,B,B,P,Q);
        if (bit(q,i))
            m*=g(A,P,B,Q,P,Q);
    }

    m=conj(m)/m;     // raise to power of (p-1)

    if (!A.iszero() || m.iszero()) return FALSE;
    if (m.isunity()) return FALSE;

    r=real(m);
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

// Hash and map a Client Identity to a curve point E_(Fp)

ECn hash_and_map(char *ID,Big cof)
{
    ECn Q;
    Big x0=H1(ID);

    while (!Q.set(x0)) x0+=1;
    Q*=cof;

    return Q;
}

/* Note that if #E(Fp)  = p+1-t
           then #E(Fp2) = (p+1-t)(p+1+t) (a multiple of #E(Fp))
           (Weil's Theorem)
 */

int main()
{
    ifstream common("k2.ecs");      // elliptic curve parameters
    miracl* mip=&precision;
    ECn Alice,Bob,sA,sB;
    ECn B2,Server,sS;
    ZZn res,sp,ap,bp;
    Big t,r,a,b,s,ss,p,q,x,y,B,cof,cf,cf2;
    int bits,A;
    time_t seed;

    common >> bits;
    mip->IOBASE=16;
    common >> p;
    common >> A;
    common >> B;
    common >> cof;
    common >> q;

    t=p+1-cof*q;

    cf= (p+1-t)/q;        // q divides p+1 (for k=2 condition)
    cf2=(p+1+t)/q;      // and therefore also divides t (as it divides r)

// this co-factor is in fact not needed....

    time(&seed);
    irand((long)seed);


    mip->IOBASE=16;

// hash Identities to curve point

    ss=rand(q);    // TA's super-secret 

    cout << "Mapping Server ID to point" << endl;

    ecurve(A,-B,p,MR_AFFINE);     // twist curve

    Server=hash_and_map((char *)"Server",cf2);

    cout << "Mapping Alice & Bob ID's to points" << endl;

    ecurve(A,B,p,MR_AFFINE);

    Alice=hash_and_map((char *)"Alice",cf);
    Bob=  hash_and_map((char *)"Robert",cf);

// Alice, Bob are points of order q
// Server does not need to be (its order is a multiple of q)

    cout << "Alice, Bob and the Server visit Trusted Authority" << endl;

    sS=ss*Server; 
    sA=ss*Alice;
    sB=ss*Bob;

    cout << "Alice and Server Key exchange" << endl;

    a=rand(q);   // Alice's random number
    s=rand(q);   // Server's random number

    if (!nw(sA,Server,q,res)) cout << "Trouble" << endl; 

    ap=powl(res,a);

    if (!nw(Alice,sS,q,res))  cout << "Trouble" << endl;
    sp=powl(res,s);

    cout << "Alice  Key= " << H2(powl(sp,a)) << endl;
    cout << "Server Key= " << H2(powl(ap,s)) << endl;

    cout << "Bob and Server Key exchange" << endl;

    b=rand(q);   // Bob's random number
    s=rand(q);   // Server's random number

    if (!nw(sB,Server,q,res)) cout << "Trouble" << endl;
    bp=powl(res,b);

    if (!nw(Bob,sS,q,res)) cout << "Trouble" << endl;
    sp=powl(res,s);

    cout << "Bob's  Key= " << H2(powl(sp,b)) << endl;
    cout << "Server Key= " << H2(powl(bp,s)) << endl;

    return 0;
}

