/*

   Scott's AKE Client/Server testbed

   See http://eprint.iacr.org/2002/164

   Compile as 
   cl /O2 /GX /DZZNS=16 ake6fsta.cpp zzn6a.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib
   using COMBA build

   Freeman-Scott-Teske curve - Ate pairing

   This version uses the sextic twist and the Ate pairing
   No elliptic curve operations over an extension field!
  
   p=1+3*x+3*x*x+9*pow(x,3)+27*pow(x,4)
   r=1+3*x+9*x*x
   t=2+3*x

   For this curve k=6, rho=2 (which is bad...)
   p is 512 bits, 6p = 3072 bits, r is 256 bits

   For this program p MUST be 13 mod 24

   final exponent = (1+9*x^3)*p^0 + 3x^2*p^1

   Modified to prevent sub-group confinement attack

*/

#include <iostream>
#include <fstream>
#include <string.h>
#include <ctime>
#include "ecn.h"
#include "zzn6a.h"

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
    X=pow(Fr,(p-1)/3);
}

//
// Line from A to destination C. Let A=(x,y)
// Line Y-slope.X-c=0, through A, so intercept c=y-slope.x
// Line Y-slope.X-y+slope.x = (Y-y)-slope.(X-x) = 0
// Now evaluate at Q -> return (Qy-y)-slope.(Qx-x)
//

ZZn6 line(ECn& A,ECn& C,ZZn& slope,ECn& Q)
{ 
    ZZn6 n;
    ZZn2 p;
    ZZn x,y,z,t,Qx,Qy; 
    extract(Q,Qx,Qy);
#ifdef AFFINE
    extract(A,x,y);
	n.set(-Qx,0,x);
	p.set(-Qy,y);
	n*=slope; n=tx(n); 
	n-=p;
#endif
#ifdef PROJECTIVE
    extract(A,x,y,z);
    x*=z; t=z; z*=z; z*=t;
    Qx*=z; Qy*=z;
	p.set(-Qy,y);
	t=-Qx*slope;
	x*=slope;
	n.set(t,0,x);
	n=tx(n);
    extract(C,x,y,z);
    p*=z; n-=p;
#endif
    return n;
}

//
// Add A=A+B  (or A=A+A) 
// Evaluate line function
//

ZZn6 g(ECn& A,ECn& B,ECn& Q)
{
    int type;
    ZZn lam;
    big ptr;
    ECn P=A;

// Evaluate line from A
    type=A.add(B,&ptr);
    if (!type)   return (ZZn6)1; 
    else lam=ptr;

    return line(P,A,lam,Q);    
}

//
// Ate Pairing - note denominator elimination has been applied
//
// P is a point of order q. Q(x,y) is a point of order q. 
// Note that P is a point on the sextic twist curve over Fp, 
// Q(x,y) a point on the base curve 
//

BOOL fast_pairing(ECn& P,ECn& Q,Big& q,Big &x,ZZn2 &X, ZZn6 &res)
{ 
    int i,j,n,nb,nbw,nzs;
    ECn A;
    Big T=3*x+1;
    ZZn6 w,t;

    res=1;  
    A=P;
    normalise(Q);

    nb=bits(T);

// Miller Loop
    for (i=nb-2;i>=0;i--)
    {
        res*=res;
        t=g(A,A,Q);
        res*=t; 
        if (bit(T,i))
        {
            t=g(A,P,Q);
            res*=t;
        }
    }

    if (res.iszero()) return FALSE;

// Final Exponentiation
    w=res;                          
    w.powq(X);

	res*=w;                        // ^(p+1)

    w=res;

	w.conj();
    res=w/res;                     // ^(p^3-1)

    res.mark_as_unitary();

    t=w=res;
    w.powq(X);

    res=pow(res,3*x);              // specially tailored final exponentiation
    res*=w;
    res=pow(res,3*x);
    res=pow(res,x);
    res*=t;

    if (res==(ZZn6)1) return FALSE;
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

Big H2(ZZn6 y)
{ // Compress and Hash an Fp6 to a big number
    sha sh;
    Big a,h,p,xx[2];
    ZZn u,v,w;
	ZZn2 x;
    char s[HASH_LEN];
    int i,j,m;

    shs_init(&sh);
	y.get(x);
    x.get(u,v);
    xx[0]=u; xx[1]=v;
    for (i=0;i<2;i++)
    {
        a=xx[i];
        while (a>0)
        {
            m=a%256;
            shs_process(&sh,m);
            a/=256;
        }
    }
    shs_hash(&sh,s);
    h=from_binary(HASH_LEN,s);
    return h;
}

// Hash and map a Client Identity to a curve point E_(Fp)

ECn hash_and_map(char *ID,Big& cof)
{
    ECn Q;
    Big x0=H1(ID);
    forever
    {
        while (!Q.set(x0)) x0+=1;
        x0+=1;
        Q*=cof;
        if (!Q.iszero()) break;
    }
    return Q;
}

int main()
{
    miracl* mip=&precision;
    ECn Alice,Bob,sA,sB,Server,sS;
    ZZn6 res,sp,ap,bp;
	ZZn2 X;
    Big a,b,s,ss,p,q,x,y,B,cof,t,cf,cof1;
    int i,bits,A;
    time_t seed;

    mip->IOBASE=16;
	x="60000000000000000000000000012014"; // Low Hamming weight
	q=1+3*x+9*x*x;
	p=1+3*x+3*x*x+9*pow(x,3)+27*pow(x,4);	
	t=2+3*x;
	cof=(p+1-t)/q;
	A=0; B=1;

	cout << "Initialised... " << endl;
    cout << "p%24= " << p%24 << endl;

    time(&seed);
    irand((long)seed);
    mip->IOBASE=16;
    cof1=cof+1;

    ss=rand(q);    // TA's super-secret 

#ifdef AFFINE
    ecurve(A,B,p,MR_AFFINE);
#endif
#ifdef PROJECTIVE
    ecurve(A,B,p,MR_PROJECTIVE);
#endif

	set_frobenius_constant(X);

    cout << "Mapping Server ID to point" << endl;
    Server=hash_and_map((char *)"Server",cof);
    sS=ss*Server;

// sextic twist

#ifdef AFFINE
    ecurve(A,B*(p-1)/2,p,MR_AFFINE);
#endif
#ifdef PROJECTIVE
    ecurve(A,B*(p-1)/2,p,MR_PROJECTIVE);
#endif

    cout << "Mapping Alice & Bob ID's to points" << endl;
    Alice=hash_and_map((char *)"Alice",cof1);  // cof+1 is the co-factor for the sextic twist (?)
    Bob=  hash_and_map((char *)"Robert",cof1);

    cout << "Alice and Bob visit Trusted Authority" << endl; 

    sA=ss*Alice; 
    sB=ss*Bob; 

    cout << "Alice and Server Key Exchange" << endl;

    a=rand(q);   // Alice's random number
    s=rand(q);   // Server's random number

    if (!fast_pairing(sA,Server,q,x,X,res)) cout << "Trouble" << endl;
    if (pow(res,q)!=(ZZn6)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    ap=powu(res,a);

    if (!fast_pairing(Alice,sS,q,x,X,res)) cout << "Trouble" << endl;
    if (pow(res,q)!=(ZZn6)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    sp=powu(res,s);

    cout << "Alice  Key= " << H2(pow(sp,a)) << endl;
    cout << "Server Key= " << H2(pow(ap,s)) << endl;

    cout << "Bob and Server Key Exchange" << endl;

    b=rand(q);   // Bob's random number
    s=rand(q);   // Server's random number

    if (!fast_pairing(sB,Server,q,x,X,res)) cout << "Trouble" << endl;
    if (pow(res,q)!=(ZZn6)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    bp=powu(res,b);

    if (!fast_pairing(Bob,sS,q,x,X,res)) cout << "Trouble" << endl;
    if (pow(res,q)!=(ZZn6)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    sp=powu(res,s);

    cout << "Bob's  Key= " << H2(powu(sp,b)) << endl;
    cout << "Server Key= " << H2(powu(bp,s)) << endl;

    cout << "Alice and Bob's attempted Key exchange" << endl;

    if (!fast_pairing(Alice,sB,q,x,X,res)) cout << "Trouble" << endl;
    if (pow(res,q)!=(ZZn6)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    bp=powu(res,b);

    if (!fast_pairing(sA,Bob,q,x,X,res)) cout << "Trouble" << endl;
    if (pow(res,q)!=(ZZn6)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    ap=powu(res,a);

    cout << "Alice  Key= " << H2(powu(ap,b)) << endl;
    cout << "Bob's Key=  " << H2(powu(bp,a)) << endl;

    return 0;
}

