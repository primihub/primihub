/*

   Scott's AKE Client/Server testbed

   See http://eprint.iacr.org/2002/164

   Compile as 
   cl /O2 /GX /DZZNS=6 ake12blsa.cpp zzn12a.cpp zzn4.cpp ecn2.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib
   using COMBA build

   Barreto-Lynn-Scott Curve - Ate pairing

   The curve generated is generated from a 64-bit x parameter
   This version implements that Ate pairing

   This is implemented on the Barreto-Lynn-Scott k=12, rho=1.5 pairing friendly curve

   NOTE: Irreducible polynomial is of the form x^6+sqrt(-2)

   See bls12.cpp for a program to generate suitable curves

   Modified to prevent sub-group confinement attack
*/

#include <iostream>
#include <fstream>
#include <string.h>
#include "ecn.h"
#include <ctime>
#include "ecn2.h"
#include "zzn12a.h"

using namespace std;

Miracl precision(6,0); 
/*
extern "C"
{
    int fpc=0;
    int fpa=0;
    int fpx=0;
}
*/

#ifdef MR_AFFINE_ONLY
    #define AFFINE
#else
    #define PROJECTIVE
#endif

// Using SHA-256 as basic hash algorithm

#define HASH_LEN 32

//
// Ate Pairing Code
//

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

ZZn12 Frobenius(ZZn12 P, ZZn2 &X, int k)
{
  ZZn12 Q=P;
  for (int i=0; i<k; i++)
    Q.powq(X);
  return Q;
}

void endomorph(ECn &A,ZZn &Beta)
{ // apply endomorphism P(x,y) = (Beta*x,y) where Beta is cube root of unity
  // Actually (Beta*x,-y) =  x^2.P
	ZZn x,y;
	x=(A.get_point())->X;
	y=(A.get_point())->Y;
	y=-y;
	x*=Beta;
	copy(getbig(x),(A.get_point())->X);
	copy(getbig(y),(A.get_point())->Y);
}

//
// This calculates p.A quickly using Frobenius
// 1. Extract A(x,y) from twisted curve to point on curve over full extension, as X=i^2.x and Y=i^3.y
// where i=NR^(1/k)
// 2. Using Frobenius calculate (X^p,Y^p)
// 3. map back to twisted curve
// Here we simplify things by doing whole calculation on the twisted curve
//
//
// Note we have to be careful as in detail it depends on w where p=w mod k
// Its simplest if w=1, which in this case it is.
//

ECn2 psi(ECn2 &A,ZZn2 &F,int n)
{ 
	int i;
// Fast multiplication of A by q (for Trace-Zero group members only)
    ZZn2 x,y,z,w,r;
	ECn2 P=A;

#ifdef PROJECTIVE
    P.get(x,y,z);
#else
    P.get(x,y);
#endif

	w=F*F;
	r=F;
	for (i=0;i<n;i++)
	{
		x=w*conj(x);
		y=r*w*conj(y);
#ifdef PROJECTIVE
		z=conj(z);
#endif
	}
#ifdef PROJECTIVE
    P.set(x,y,z);
#else
    P.set(x,y);
#endif
	return P;
}


//
// Line from A to destination C. Let A=(x,y)
// Line Y-slope.X-c=0, through A, so intercept c=y-slope.x
// Line Y-slope.X-y+slope.x = (Y-y)-slope.(X-x) = 0
// Now evaluate at Q -> return (Qy-y)-slope.(Qx-x)
//

ZZn12 line(ECn2& A,ECn2& C,ECn2&B,ZZn2& slope,ZZn2& extra,BOOL Doubling,ZZn& Qx,ZZn& Qy)
{
     ZZn12 w;
     ZZn4 nn,dd;
     ZZn2 X,Y;

#ifdef AFFINE
     A.get(X,Y);

	 nn.set((ZZn2)-Qy,Y-slope*X);
	 dd.set(slope*Qx);
		   
     w.set(nn,dd);

#endif
#ifdef PROJECTIVE
    ZZn2 Z3;

    C.getZ(Z3);

// Thanks to A. Menezes for pointing out this optimization...
    if (Doubling)
    {
        ZZn2 Z,ZZ;
        A.get(X,Y,Z);
        ZZ=Z; ZZ*=ZZ;
		nn.set((Z3*ZZ)*Qy,slope*X-extra);
		dd.set(-(ZZ*slope)*Qx);        
    }
    else
    {
        ZZn2 X2,Y2;
        B.get(X2,Y2);
		nn.set(Z3*Qy,slope*X2-Y2*Z3);
		dd.set(-slope*Qx);       
    }
    w.set(nn,dd);
#endif

     return w;
}


//
// Add A=A+B  (or A=A+A) 
// Return line function value
//

ZZn12 g(ECn2& A,ECn2& B,ZZn& Qx,ZZn& Qy)
{
    ZZn2 lam,extra;
    ZZn12 r;
    ECn2 P=A;
    BOOL Doubling;

// Evaluate line from A
    Doubling=A.add(B,lam,extra);

    if (A.iszero())   return (ZZn12)1; 
    r=line(P,A,B,lam,extra,Doubling,Qx,Qy);

    return r;
}

void SoftExpo(ZZn12 &f3, ZZn2 &X){
  ZZn12 t0;
  int i;

  t0=f3; f3.conj(); f3/=t0;
  f3.mark_as_regular();
  t0=f3; f3.powq(X); f3.powq(X); f3*=t0;
  f3.mark_as_unitary();
}

ZZn12 HardExpo(ZZn12 &f3x0, ZZn2 &X, Big &x){
//vector=[ 1, 2, 3 ]
  ZZn12 r;
  ZZn12 xA;
  ZZn12 xB;
  ZZn12 t0;
  ZZn12 t1;
  ZZn12 f3x1;
  ZZn12 f3x2;
  ZZn12 f3x3;
  ZZn12 f3x4;
  ZZn12 f3x5;

  f3x1=pow(f3x0,x);
  f3x2=pow(f3x1,x);
  f3x3=pow(f3x2,x);
  f3x4=pow(f3x3,x);
  f3x5=pow(f3x4,x);

  xA=f3x2*inverse(f3x4)*Frobenius(f3x1,X,1)*Frobenius(inverse(f3x3),X,1)*Frobenius(inverse(f3x2),X,2)*Frobenius(inverse(f3x1),X,3);
  xB=f3x0;
  t0=xA*xB;
  xA=inverse(f3x1)*f3x5*Frobenius(inverse(f3x0),X,1)*Frobenius(f3x4,X,1)*Frobenius(f3x1,X,2)*Frobenius(f3x3,X,2)*Frobenius(f3x0,X,3)*Frobenius(f3x2,X,3);
  xB=f3x0;
  t1=xA*xB;
  t0=t0*t0;
  t0=t0*t1;

r=t0;
return r;

}

//
// Ate Pairing - note denominator elimination has been applied
//
// P is a point of order q. Q(x,y) is a point of order q. 
// Note that P is a point on the sextic twist of the curve over Fp^2, Q(x,y) is a point on the 
// curve over the base field Fp
//

BOOL fast_pairing(ECn2& P,ZZn& Qx,ZZn& Qy,Big &x,ZZn2 &X,ZZn12& res)
{ 
    ECn2 A;
    int i,nb;
    Big n;
    ZZn12 w,r;

    n=x;       // t-1
    A=P;           // remember A
 
    nb=bits(n);
    r=1;
//fpc=fpa=fpx=0;
    for (i=nb-2;i>=0;i--)
    {
        r*=r;   
        r*=g(A,A,Qx,Qy); 
 
        if (bit(n,i)) 
            r*=g(A,P,Qx,Qy);  
    }


    if (r.iszero()) return FALSE;

	SoftExpo(r,X);
	res=HardExpo(r,X,x);
             
    return TRUE;
}

//
// ecap(.) function
//

BOOL ecap(ECn2& P,ECn& Q,Big& x,ZZn2 &X,ZZn12& r)
{
    BOOL Ok;
    Big xx,yy;
    ZZn Qx,Qy;

    Q.get(xx,yy); Qx=xx; Qy=yy;
	P.norm();

    Ok=fast_pairing(P,Qx,Qy,x,X,r);

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

Big H2(ZZn12 x)
{ // Compress and hash an Fp12 to a big number
    sha256 sh;
    ZZn4 u;
    ZZn2 h,l;
    Big a,hash,p,xx[4];
    char s[HASH_LEN];
    int i,j,m;

    shs256_init(&sh);
    x.get(u);  // compress to single ZZn4
    u.get(l,h);
    xx[0]=real(l); xx[1]=imaginary(l); xx[2]=real(h); xx[3]=imaginary(h);
 
    for (i=0;i<4;i++)
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
    ECn2 S,SS;
    ZZn2 X;
 
    Big x0=H1(ID);

    forever
    {
        x0+=1;
        X.set((ZZn)1,(ZZn)x0);
        if (!S.set(X)) continue;
        break;
    }

    return S;
}     

// Hash and map a Client Identity to a curve point E_(Fp) of order q

ECn hash_and_map(char *ID,Big cf)
{
    ECn Q;
    Big x0=H1(ID);

    while (!Q.set(x0,x0)) x0+=1;
    Q*=cf;

    return Q;
}

// Use GLV endomorphism idea for multiplication in G1.

ECn G1_mult(ECn &P,Big &e,Big &x,ZZn &Beta)
{
//	return e*P;
	int i;
	ECn Q;
	Big x2,u[2];
	x2=x*x; 
	u[0]=e%x2; u[1]=e/x2;

	Q=P;
	endomorph(Q,Beta);

	Q=mul(u[0],P,u[1],Q);
	
	return Q;
}

//.. for multiplication in G2

ECn2 G2_mult(ECn2 &P,Big e,Big &x,ZZn2 &X)
{
//	return e*P;
	int i;
	ECn2 Q[4];
	Big u[4];

	for (i=0;i<4;i++) {u[i]=e%x; e/=x;}

	Q[0]=P;
	for (i=1;i<4;i++)
		Q[i]=psi(Q[i-1],X,1);

// simple multi-addition

	return mul(4,Q,u);
}

//.. and for exponentiation in GT

ZZn12 GT_pow(ZZn12 &res,Big e,Big &x,ZZn2 &X)
{
//	return pow(res,e);
	int i,j;
	ZZn12 Y[4];
	Big u[4];

	for (i=0;i<4;i++) {u[i]=e%x; e/=x;}

	Y[0]=res;
	for (i=1;i<4;i++)
		{Y[i]=Y[i-1]; Y[i].powq(X);}

// simple multi-exponentiation
	return pow(4,Y,u);
}

ECn2 HashG2(ECn2 &Qx0, Big &x, ZZn2 &X){
//vector=[ 1, 2, 4 ]
  ECn2 r;
  ECn2 xA;
  ECn2 xB;
  ECn2 xC;
  ECn2 t0;
  ECn2 Qx0_;
  ECn2 Qx1;
  ECn2 Qx1_;
  ECn2 Qx2;
  ECn2 Qx2_;
  ECn2 Qx3;
  ECn2 Qx3_;

  Qx0_=-(Qx0);
  Qx1=x*Qx0;
  Qx1_=-(Qx1);
  Qx2=x*Qx1;
  Qx2_=-(Qx2);
  Qx3=x*Qx2;
  Qx3_=-(Qx3);

  xA=Qx0;
  xB=Qx0;
  t0=xA+xB;
  xB=psi(Qx1,X,2);

  t0=t0+xB;
  t0+=t0;
  xB=Qx1_;
  xC=Qx2_;

  xB+=xC;
  xC=Qx3;
  xB+=xC;
  xC=psi(Qx0,X,1);
  xB+=xC;
  xC=psi(Qx1_,X,1);
  xB+=xC;
  xC=psi(Qx2_,X,1);
  xB+=xC;
  xC=psi(Qx3,X,1);
  xB+=xC;
  xC=psi(Qx0_,X,2);
  xB+=xC;
  xC=psi(Qx2_,X,2);
  xB+=xC;
  xB.norm();
  t0=t0+xB;

r=t0;
r.norm();
return r;

}

int main()
{
    miracl* mip=&precision;
    ECn Alice,Bob,sA,sB;
    ECn2 Server,sS;
    ZZn12 sp,ap,bp,res;
    ZZn2 X;
    Big a,b,s,ss,p,q,x,y,B,cf,t,cof;
	ZZn Beta;
    int i,bits,A;
    time_t seed;

    mip->IOBASE=16;
    x= (char *)"C000000000040405";  // found by BLS12.CPP
    
    p=(pow(x,6)-2*pow(x,5)+2*pow(x,3)+x+1)/3;
    t=x+1;
    q=pow(x,4)-x*x+1;
	cof=(p+1-t)/q;

  //  cf=9*((x-1)*(x-1)*(p+t)/3 + 1);

    modulo(p);
    set_frobenius_constant(X);

    cout << "Initialised... " << endl;

    time(&seed);
    irand((long)seed);

#ifdef AFFINE
    ecurve((Big)0,(Big)1,p,MR_AFFINE);
#endif
#ifdef PROJECTIVE
    ecurve((Big)0,(Big)1,p,MR_PROJECTIVE);
#endif

	Beta=pow((ZZn)2,(p-1)/3);
	Beta*=Beta;   // right cube root of unity

    mip->IOBASE=16;
    mip->TWIST=MR_SEXTIC_D;   // map Server to point on twisted curve E(Fp2)

    ss=rand(q);    // TA's super-secret 

    cout << "Mapping Server ID to point" << endl;
    Server=hash_and_map2((char *)"Server");
	Server=HashG2(Server,x,X);

    cout << "Mapping Alice & Bob ID's to points" << endl;
    Alice=hash_and_map((char *)"Alice",cof);
    Bob=  hash_and_map((char *)"Robert",cof);

    cout << "Alice, Bob and the Server visit Trusted Authority" << endl; 

	sS=G2_mult(Server,ss,x,X);
    sA=G1_mult(Alice,ss,x,Beta); 
    sB=G1_mult(Bob,ss,x,Beta); 

    cout << "Alice and Server Key Exchange" << endl;

    a=rand(q);   // Alice's random number
    s=rand(q);   // Server's random number

 //   for (i=0;i<1000;i++)
   
    if (!ecap(Server,sA,x,X,res)) cout << "Trouble" << endl;
    if (pow(res,q)!=(ZZn12)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    ap=GT_pow(res,a,x,X);

    if (!ecap(sS,Alice,x,X,res)) cout << "Trouble" << endl;
    if (pow(res,q)!=(ZZn12)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    sp=GT_pow(res,s,x,X);

    cout << "Alice  Key= " << H2(GT_pow(sp,a,x,X)) << endl;
    cout << "Server Key= " << H2(GT_pow(ap,s,x,X)) << endl;

    cout << "Bob and Server Key Exchange" << endl;

    b=rand(q);   // Bob's random number
    s=rand(q);   // Server's random number

    if (!ecap(Server,sB,x,X,res)) cout << "Trouble" << endl;
    if (pow(res,q)!=(ZZn12)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    bp=GT_pow(res,b,x,X);

    if (!ecap(sS,Bob,x,X,res)) cout << "Trouble" << endl;
    if (pow(res,q)!=(ZZn12)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    sp=GT_pow(res,s,x,X);

    cout << "Bob's  Key= " << H2(GT_pow(sp,b,x,X)) << endl;
    cout << "Server Key= " << H2(GT_pow(bp,s,x,X)) << endl;

    return 0;
}

