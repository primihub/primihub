/*
   Scott's AKE Client/Server testbed

   See http://eprint.iacr.org/2002/164

   Compile as 
   cl /O2 /GX /DZZNS=16 ake4cpt.cpp zzn4.cpp zzn2.cpp ecn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib
   Fastest using COMBA build for 512-bit mod-mul

   Cocks-Pinch curve - Tate pairing

   The file k4.ecs is required 
   Security is G192/F2048 (192-bit group, 2048-bit field)

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

Miracl precision(16,0);  

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

    x*=z;
	t=z; z*=z; z*=t;          
    x*=slope; 
	t=slope*z;
    m*=t;
	m-=x; t=z;
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
	ZZn4 val;
    big ptr;
    ECn P=A;

// Evaluate line from A
    type=A.add(B,&ptr);
    if (!type)   return (ZZn4)1; 
    lam=ptr;
    val=line(P,A,lam,Qx,Qy);
	//cout << "val= " << val << endl;
	//exit(0);
	return val;
}

//
// Tate Pairing - note denominator elimination has been applied
//
// P is a point of order q. Q(x,y) is a point of order m.q. 
// Note that P is a point on the curve over Fp, Q(x,y) a point on the 
// extension field Fp^2
//

BOOL tate(ECn& P,ECn2 Q,Big& q,Big *cf,ZZn2 &Fr,ZZn2& r)
{ 
    int i,nb;
    ECn A;
    ZZn4 w,res;
    ZZn4 a[2];
    ZZn2 Qx,Qy;
//    ZZn4 X,Y;

    Q.get(Qx,Qy);

    Qx=txd(Qx);
    Qy=txd(txd(Qy));

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

	w.conj(); // ^(p^2-1)
   
    res=w/res;

    res.mark_as_unitary();

    a[0]=a[1]=res; 
    a[0].powq(Fr);
    res=pow(2,a,cf);

    r=real(res);  // compression

//    r=powl(real(res),cf[0]*get_modulus()+cf[1]);    // ^(p*p+1)/q

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
   case 7: 
         X.set((Big)2,(Big)1);  // = (2+sqrt(-1))^(p-1)/2
    default: break;
    }
    X=pow(X,(p-1)/2);
}

int main()
{
    ifstream common("k4.ecs");      // elliptic curve parameters
    miracl* mip=&precision;
    ECn Alice,Bob,sA,sB;
    ECn2 Server,sS;
    ZZn2 res,sp,ap,bp,Fr;
    Big a,b,s,ss,p,q,r,B,cof;
    Big cf[2];
                                       
    int bits,A;
    time_t seed;

    common >> bits;
    mip->IOBASE=16;
    common >> p;
    common >> A;
    common >> B;
    common >> cof;   // #E/q
    common >> q;     // low hamming weight q
    common >> cf[0];    // [(p^2+1)/q]/p
    common >> cf[1];    // [(p^2+1)/q]%p
   
    cout << "Initialised... " << p%8 << endl;

    time(&seed);
    irand(1L /*(long)seed*/);

#ifdef AFFINE
    ecurve(A,B,p,MR_AFFINE);
#endif
#ifdef PROJECTIVE
    ecurve(A,B,p,MR_PROJECTIVE);
#endif

    set_frobenius_constant(Fr);

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
    
	if (!tate(sA,Server,q,cf,Fr,res)) cout << "Trouble" << endl;

    if (powl(res,q)!=(ZZn2)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    ap=powl(res,a);

    if (!tate(Alice,sS,q,cf,Fr,res)) cout << "Trouble" << endl;
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

    if (!tate(sB,Server,q,cf,Fr,res)) cout << "Trouble" << endl;
    if (powl(res,q)!=(ZZn2)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    bp=powl(res,b);

    if (!tate(Bob,sS,q,cf,Fr,res)) cout << "Trouble" << endl;
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
