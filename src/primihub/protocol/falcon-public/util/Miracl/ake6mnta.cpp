/*

   Scott's AKE Client/Server testbed

   See http://eprint.iacr.org/2002/164

   Compile as 
   cl /O2 /GX /DZZNS=5 ake6mnta.cpp zzn6a.cpp ecn3.cpp zzn3.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib
   using COMBA build

   MNT Curve - ate pairing

   Thanks to Drew Sutherland for providing the MNT curve

   Irreducible binomial MUST be of the form x^6+2. This excludes many of the curves
   found using the mnt utility!

   Modified to prevent sub-group confinement attack

   NOTE: Key exchange bandwidth could be reduced further using ideas from
         "Doing more with Fewer Bits", Brouwer, Pellikaan & Verheul, Asiacrypt 
         '99

   NOTE: This version uses a "compositum". That is the ZZn6 class is a cubic tower over ZZn2, but can
   also be considered as a quadratic tower over ZZn3. The routine shuffle converts from one form to the other.
   The former is fastest for ZZn6 arithmetic, the latter form is required for handling the second parameter
   to the pairing, which is on the quadratic twist E(Fp3)

*/

#include <iostream>
#include <fstream>
#include <string.h>
#include "ecn.h"
#include <ctime>
#include "ecn3.h"
#include "zzn6a.h"

Miracl precision(5,0); 

#define AFFINE
//#define PROJECTIVE

#ifdef MR_COUNT_OPS
extern "C"
{
    int fpc,fpa,fpx,fpm2,fpi2,fpaq,fpsq,fpmq;
}
#endif

// Using SHA-1 as basic hash algorithm

#define HASH_LEN 20

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
    X=pow(X,(p-1)/3);
}

ZZn6 shuffle(ZZn3 &first, ZZn3 &second)
{ // shuffle from a pair ZZn3's to three ZZn2's, as required by ZZn6
	ZZn6 w;
	ZZn x0,x1,x2,x3,x4,x5;
	ZZn2 t0,t1,t2;
	first.get(x0,x2,x4);
	second.get(x1,x3,x5);
	t0.set(x0,x3);
	t1.set(x1,x4);
	t2.set(x2,x5);
	w.set(t0,t1,t2);
	return w;
}

void unshuffle(ZZn6 &S,ZZn3 &first,ZZn3 &second)
{ // unshuffle a ZZn6 into two ZZn3's 
	ZZn x0,x1,x2,x3,x4,x5;
	ZZn2 t0,t1,t2;
	S.get(t0,t1,t2);
	t0.get(x0,x3);
	t1.get(x1,x4);
	t2.get(x2,x5);
	first.set(x0,x2,x4);
	second.set(x1,x3,x5);
}

//
// Line from A to destination C. Let A=(x,y)
// Line Y-slope.X-c=0, through A, so intercept c=y-slope.x
// Line Y-slope.X-y+slope.x = (Y-y)-slope.(X-x) = 0
// Now evaluate at Q -> return (Qy-y)-slope.(Qx-x)
//

ZZn6 line(ECn3& A,ECn3& C,ECn3& B,int type,ZZn3& slope,ZZn3& ex1,ZZn3& ex2,ZZn& Px,ZZn& Py)
{
    ZZn6 w;
	ZZn3 d;
#ifdef AFFINE
    ZZn3 x,y;
    A.get(x,y);
    d.set1(Py);
	w=shuffle(y-slope*(Px+x),d);

#endif
#ifdef PROJECTIVE
	ZZn3 x,y,z,z3,t;
	C.getZ(z3);
	d.set1(Py);

	if (type==MR_ADD)
	{ // exploit that B is in affine
		ZZn3 x2,y2;
		B.get(x2,y2);
		y2*=z3; d*=z3; 
		w=shuffle(y2-slope*(Px+x2),d);
	}
	if (type==MR_DOUBLE)
	{ // use extra information from point doubling
		A.get(x,y,z);
		w=shuffle(ex1-slope*(Px*ex2+x),d*z3*ex2);	
	}
#endif

    return w;
}

//
// Add A=A+B  (or A=A+A) 
// Return line function value
//

ZZn6 g(ECn3& A,ECn3& B,ZZn& Px,ZZn& Py)
{
    BOOL type;
    ZZn3 lam,ex1,ex2;
    ECn3 Q=A;

// Evaluate line from A to A+B
    type=A.add(B,lam,&ex1,&ex2);

    return line(Q,A,B,type,lam,ex1,ex2,Px,Py);
}

//
// ate Pairing - note denominator elimination has been applied
//
// Q(x,y) is a point of order q. 
// Note that P is a point on the curve over Fp, Q(x,y) a point on the 
// twisted curve over the extension field Fp^3
//

BOOL ate(ECn3& Q,ECn& P,Big &x,ZZn2& X,ZZn6& res)
{ 
    int i,j,n,nb,nbw,nzs;
    ECn3 A;
	ZZn Px,Py;
    ZZn6 w;
	Big q=x*x-x+1;

#ifdef MR_COUNT_OPS
fpc=fpa=fpx=0;
#endif  

	normalise(P);
#ifdef PROJECTIVE
	Q.norm();
#endif
	extract(P,Px,Py);

    Px+=Px;  // because x^6+2 is irreducible.. simplifies line function calculation
    Py+=Py; 

    res=1;  

    A=Q;    // reset A
    nb=bits(x);
	res.mark_as_miller();

    for (i=nb-2;i>=0;i--)
    {
		res*=res;
		res*=g(A,A,Px,Py);
		if (bit(x,i)==1)
			res*=g(A,Q,Px,Py);
        if (res.iszero()) return FALSE;  
    }

#ifdef MR_COUNT_OPS
printf("After Miller  fpc= %d fpa= %d fpx= %d\n",fpc,fpa,fpx);
#endif
  //  if (!A.iszero() || res.iszero()) return FALSE;

    w=res;   
    w.powq(X);
    res*=w;                        // ^(p+1)

    w=res;
    w.powq(X); w.powq(X); w.powq(X);
    res=w/res;                     // ^(p^3-1)

// exploit the clever "trick" for a half-length exponentiation!

    res.mark_as_unitary();

    w=res;
    res.powq(X);  // res*=res;  // res=pow(res,CF);
 
    if (x<0) res/=powu(w,-x);
    else res*=powu(w,x);
#ifdef MR_COUNT_OPS
printf("After pairing fpc= %d fpa= %d fpx= %d\n",fpc,fpa,fpx);
fpa=fpc=fpx=0;
#endif

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
{ // Hash and compress an Fp6 to a big number
    sha sh;
    ZZn u,v,w;
	ZZn2 x;
    Big a,h,p,xx[2];
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

// Hash and map a Server Identity to a curve point E_(Fp3)

ECn3 hash_and_map3(char *ID)
{
    int i;
    ECn3 S;
    ZZn3 X;
 
    Big x0=H1(ID);
    forever
    {
        x0+=1;
        X.set2((ZZn)x0);
        if (!S.set(X)) continue;

        break;
    }

//    cout << "S= " << S << endl;
    return S;
}     

// Hash and map a Client Identity to a curve point E_(Fp) of order q

ECn hash_and_map(char *ID)
{
    ECn Q;
    Big x0=H1(ID);

    while (!Q.set(x0,x0)) x0+=1;
  
    return Q;
}

BOOL member(ZZn6 r,Big &x,ZZn2 &X)
{ // check its an element of order q
	ZZn6 w=r;
	w.powq(X);
	if (x<0) r=powu(inverse(r),-x);
	else     r=powu(r,x);
	if (r==w) return TRUE;
	return FALSE;
}

// Use Scott et al. idea - http://eprint.iacr.org/2008/530.pdf

void cofactor(ECn3 &S,Big &x, ZZn2& X)
{ // S=Phi(2xP)+phi^2(2xP)
	ZZn6 X1,X2,Y1,Y2;
	ZZn3 Sx,Sy,T;
	ECn3 S2;
	int qnr=get_mip()->cnr;

	S*=x; S+=S; // hard work done here

	S.get(Sx,Sy);

	// untwist    
    Sx=Sx/qnr;
    Sy=tx(Sy);
    Sy=Sy/(qnr*qnr);

	X1=shuffle(Sx,(ZZn3)0); Y1=shuffle((ZZn3)0,Sy);
	X1.powq(X); Y1.powq(X);
	X2=X1; Y2=Y1;
	X2.powq(X); Y2.powq(X);
	unshuffle(X1,Sx,T); unshuffle(Y1,T,Sy);
	
	// twist
	Sx=qnr*Sx;
	Sy=txd(Sy*qnr*qnr);
	S.set(Sx,Sy);
	unshuffle(X2,Sx,T); unshuffle(Y2,T,Sy);

	//twist (again, like we did last summer...)
	Sx=qnr*Sx;
	Sy=txd(Sy*qnr*qnr);
	S2.set(Sx,Sy);
	S+=S2;
}

int main()
{
    miracl* mip=&precision;
    ECn Alice,Bob,sA,sB;
    ECn3 B6,Server,sS;
    ZZn6 sp,ap,bp;
	ZZn6 res,XX,YY;
	ZZn2 X;
	ZZn3 Qx,Qy;
    Big a,b,s,ss,p,q,x,y,B,cf,t,sru,T;
    int i,A;
    time_t seed;
    int qnr;

	mip->IOBASE=16;
	x="-D285DA0CFEF02F06F812"; // MNT elliptic curve parameters (Thanks to Drew Sutherland)
	p=x*x+1;
	q=x*x-x+1;
	t=x+1;
	cf=x*x+x+1;

	T=t-1;
//    cout << "t-1= " << T << endl;
//    cout << "p%24= " << p%24 << endl;

    time(&seed);
    irand((long)seed);

	A=-3;
	B="77479D33943B5B1F590B54258B72F316B3261D45";

    ecurve(A,B,p,MR_PROJECTIVE);

	set_frobenius_constant(X);
	sru=pow((ZZn)-2,(p-1)/6);   // x^6+2 is irreducible
    set_zzn3(-2,sru);

    mip->IOBASE=16;
    mip->TWIST=MR_QUADRATIC;   // map Server to point on twisted curve E(Fp3)
	//See ftp://ftp.computing.dcu.ie/pub/resources/crypto/twists.pdf

    ss=rand(q);    // TA's super-secret 

    cout << "Mapping Server ID to point" << endl;
    Server=hash_and_map3((char *)"Server");

// Multiply by the cofactor - thank you NTL!
//	Server*=(p-1);
//	Server*=(p+1+t);

	cofactor(Server,x,X);  

    cout << "Mapping Alice & Bob ID's to points" << endl;
    Alice=hash_and_map((char *)"Alice");
    Bob=  hash_and_map((char *)"Robert");

    cout << "Alice, Bob and the Server visit Trusted Authority" << endl; 

    sS=ss*Server; 
    sA=ss*Alice; 
    sB=ss*Bob; 

    cout << "Alice and Server Key Exchange" << endl;

    a=rand(q);   // Alice's random number
    s=rand(q);   // Server's random number

    if (!ate(Server,sA,x,X,res)) cout << "Trouble" << endl;
	if (!member(res,x,X))
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
	ap=powu(res,a);

    if (!ate(sS,Alice,x,X,res)) cout << "Trouble" << endl;
   	if (!member(res,x,X))
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }

	sp=powu(res,s);

    cout << "Alice  Key= " << H2(powu(sp,a)) << endl;
    cout << "Server Key= " << H2(powu(ap,s)) << endl;

    cout << "Bob and Server Key Exchange" << endl;

    b=rand(q);   // Bob's random number
    s=rand(q);   // Server's random number

    if (!ate(Server,sB,x,X,res)) cout << "Trouble" << endl;
    if (!member(res,x,X))
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    bp=powu(res,b);

    if (!ate(sS,Bob,x,X,res)) cout << "Trouble" << endl;
    if (!member(res,x,X))
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    sp=powu(res,s);

    cout << "Bob's  Key= " << H2(powu(sp,b)) << endl;
    cout << "Server Key= " << H2(powu(bp,s)) << endl;

    return 0;
}

