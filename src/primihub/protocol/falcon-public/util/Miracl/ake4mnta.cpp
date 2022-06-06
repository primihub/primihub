/*
   Scott's AKE Client/Server testbed

   See http://eprint.iacr.org/2002/164

   Compile as 
   cl /O2 /GX /DZZNS=16 ake4mnta.cpp zzn4.cpp zzn2.cpp ecn2.cpp big.cpp zzn.cpp 
   ecn.cpp miracl.lib
   Fastest using COMBA build for 160-bit mod-mul

   The file k4mnt.ecs is required 
   Security is G155/F640 (155-bit group, 640-bit field)

   Speeded up using ideas from 
   "Efficient Computation of Tate Pairing in Projective Coordinate over General Characteristic Fields" 
   by Sanjit Chatterjee1, Palash Sarkar1 and Rana Barua1 

   Modified to prevent sub-group confinement attack

*/

#include <iostream>
#include <fstream>
#include <string.h>
#include "ecn.h"
#include <ctime>
#include "ecn2.h"
#include "zzn4.h"

using namespace std;

Miracl precision(10,0);  

// Using SHA-1 as basic hash algorithm

#define HASH_LEN 20
#define COF 34

#ifdef MR_COUNT_OPS
extern "C"
{
    int fpc,fpa,fpx;
}
#endif

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

ZZn4 line(ECn2& A,ECn2& C,ECn2& B,BOOL Doubling,ZZn2& lam,ZZn2& extra1,ZZn2& extra2,ZZn& Qx,ZZn& Qy)
{ 
    ZZn4 w;
    ZZn2 z3;

#ifdef AFFINE

    ZZn2 x,y;
    A.get(x,y);
    w.set(ZZn2(0,Qy),lam*(tx(x)+Qx)-tx(y));

#endif

#ifdef PROJECTIVE
   
    C.getZ(z3);

    if (!Doubling)
    {
        ZZn2 x2,y2;
        B.get(x2,y2);
        w.set(tx(z3*Qy),lam*(tx(x2)+Qx)-z3*tx(y2));
    }
    else
    {
        ZZn2 x,y,z;
        A.get(x,y,z);
        w.set(tx(z3*extra2*Qy),lam*(extra2*Qx+tx(x))-tx(extra1));
    }
/*
         A.get(x,y,z);
         x=tx(x); y=tx(y);
         t=z; x*=z; z*=z; z*=t;
         w.set(z3*z*ZZn2(0,Qy),lam*(x+z*Qx)-y*z3);
*/
#endif

    return w;
}

ZZn4 g(ECn2& A,ECn2& B,ZZn& Qx,ZZn& Qy)
{
    BOOL Doubling;
    ZZn2  lam,extra1,extra2;
    ECn2 P=A;

    ZZn2 x,y,z;

// Evaluate line from A

    Doubling=A.add(B,lam,extra1,extra2);
    if (A.iszero())   return (ZZn4)1; 

    return line(P,A,B,Doubling,lam,extra1,extra2,Qx,Qy);
}

//
// Ate Pairing - note denominator elimination has been applied
//
// P is a point of order q. Q(x,y) is a point of order m.q. 
// Note that P is a point on the curve over Fp, Q(x,y) a point on the 
// extension field Fp^2
//

BOOL ate(ECn2& P,ECn Q,Big& t1,ZZn2 &Fr,Big cof,ZZn2& r)
{ 
    int i,j,n,nb;
    ECn2 A;
    ZZn4 w,res;
    ZZn Qx,Qy;

#ifdef MR_COUNT_OPS
fpc=fpa=fpx=0;
#endif  

    P.norm();
    normalise(Q);
    extract(Q,Qx,Qy);

    Qx+=Qx;  // because x^4+2 is irreducible..
    Qy+=Qy;

    res=1;  

/* Miller loop - Left to right method  */
    A=P;
    nb=bits(t1);
    for (i=nb-2;i>=0;i--)
    {
        res*=res;           
        res*=g(A,A,Qx,Qy); 
        if (bit(t1,i))
            res*=g(A,P,Qx,Qy);
    }

#ifdef MR_COUNT_OPS
printf("After Miller fpc= %d fpa= %d fpx= %d\n",fpc,fpa,fpx);
fpa=fpc=fpx=0;
#endif
    
    w=res;
    w.powq(Fr); w.powq(Fr);  // ^(p^2-1)
    res=w/res;

    res.mark_as_unitary();

    res*=res; w=res; res*=res; res*=res; res*=res; res*=res; res*=w;  // res=powu(res,34);


    w=res;
    res.powq(Fr);
    res*=powu(w,cof);

#ifdef MR_COUNT_OPS
printf("After Final exp. fpc= %d fpa= %d fpx= %d\n",fpc,fpa,fpx);
fpa=fpc=fpx=0;
#endif

    r=real(res);

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

//
// fast multiplication by p-1+t
// We know F^2-tF+p = 0
// So p.S=t.F(S)-F^2(S), where F is Frobenius Endomorphism
// So (p-1+t).S = t(F(S)+S)-F^2(S)-S
// This is just multiplication by t, which is half size of (p-1+t)
//

void cofactor(ECn2& S,ZZn2& Fr,Big& t)
{
    ECn2 T,K;
    ZZn2 x,y,tx,tty;

    K=S;
    S.get(x,y);
    x=txd(x);
    y=txd(txd(y)); // untwist

    x.conj();
    y.conj(); y*=Fr;  // ^p

    tx=txx(x); tty=txx(txx(y));
    S.set(tx,tty); // twist again

    x.conj();
    y.conj(); y*=Fr;  // ^(p^2)

    tx=txx(x); tty=txx(txx(y));
    T.set(tx,tty); // twist

    S+=K;
    S*=t;
    S-=T;
    S-=K;
}

int main()
{
    ifstream common("k4mnt.ecs");      // elliptic curve parameters
    miracl* mip=&precision;
    ECn Alice,Bob,sA,sB;
    ECn2 Server,sS,tP;
    ZZn2 res,sp,ap,bp,Fr,x,y;
    ZZn4 X,Y,X2,Y2;
    Big a,b,s,ss,p,q,r,B,delta,fr,t,t1;
                                       
    int bits,A;
    time_t seed;

    cout << "Started" << endl;
    common >> bits;
    mip->IOBASE=16;
    common >> p;
    common >> A;
    common >> B;
    common >> q >> delta;
    common >> fr;

    cout << "Initialised... " << p%24 << endl;
 
    t=p+1-COF*q;
    t1=p-COF*q; 

//cout << "p= " << p << endl;
//cout << "p%8= " << p%8 << endl;
cout << "t= " << t << endl;
//cout << "delta= " << delta << endl;

    time(&seed);
    irand((long)seed);

#ifdef AFFINE
    ecurve(A,B,p,MR_AFFINE);
#endif
#ifdef PROJECTIVE
    ecurve(A,B,p,MR_PROJECTIVE);
#endif

 //   Fr=get_frobenius_constant();

    Fr=(ZZn2)fr; 
    mip->IOBASE=16;
    mip->TWIST=MR_QUADRATIC;   // map Server to point on twisted curve E(Fp2)

// hash Identities to curve point

    ss=rand(q);    // TA's super-secret 

    cout << "Mapping Server ID to point" << endl;
    Server=hash2((char *)"Server");
    cofactor(Server,Fr,t);   // multiply by cofactor (p-1+t)
    Server*=COF;

    cout << "Mapping Alice & Bob ID's to points" << endl;
    Alice=hash_and_map((char *)"Alice",COF);

    Bob=  hash_and_map((char *)"Robert",COF);
    cout << "Alice, Bob and the Server visit Trusted Authority" << endl; 

    sS=ss*Server; 
    sA=ss*Alice; 
    sB=ss*Bob; 

    cout << "Alice and Server Key Exchange" << endl;

    a=rand(q);   // Alice's random number
    s=rand(q);   // Server's random number

    if (!ate(Server,sA,t1,Fr,delta,res)) cout << "Trouble" << endl;

    if (powl(res,q)!=(ZZn2)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }

    ap=powl(res,a);

    if (!ate(sS,Alice,t1,Fr,delta,res)) cout << "Trouble" << endl;
    if (powl(res,q)!=(ZZn2)1)
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

    if (!ate(Server,sB,t1,Fr,delta,res)) cout << "Trouble" << endl;
    if (powl(res,q)!=(ZZn2)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    bp=powl(res,b);

    if (!ate(sS,Bob,t1,Fr,delta,res)) cout << "Trouble" << endl;
    if (powl(res,q)!=(ZZn2)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    sp=powl(res,s);

    cout << "Bob's  Key= " << H2(powl(sp,b)) << endl;
    cout << "Server Key= " << H2(powl(bp,s)) << endl;

    return 0;
}
