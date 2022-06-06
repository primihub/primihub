/*

   Scott's AKE Client/Server testbed

   See http://eprint.iacr.org/2002/164

   Compile as 
   cl /O2 /GX /DZZNS=5 ake6mntx.cpp zzn6a.cpp ecn3.cpp zzn3.cpp zzn2.cpp big.cpp zzn.cpp ecn.cpp miracl.lib
   using COMBA build

   MNT Curve - Tate pairing

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

#ifdef MR_COUNT_OPS
extern "C"
{
    int fpc,fpa,fpx,fpm2,fpi2;
}
#endif

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
    X=pow(X,(p-1)/3);
}

ZZn6 shuffle(const ZZn3 &first, const ZZn3 &second)
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

ZZn6 line(ECn& A,ECn& C,ECn& B,int type,ZZn& slope,ZZn& ex1,ZZn& ex2,ZZn3& Qx,ZZn3& Qy)
{
     ZZn6 w;
#ifdef AFFINE
     ZZn3 nn=Qx;
     ZZn x,y;
     extract(A,x,y);
     nn-=x; 
     nn*=slope;   
     nn+=y;
	 nn=-nn;
	 w=shuffle(nn,Qy);

#endif
#ifdef PROJECTIVE
     if (type==MR_ADD)
     {
        ZZn x2,y2,x3,z3;
        extract(B,x2,y2);
        extract(C,x3,x3,z3);
		w=shuffle(slope*(Qx-x2)+z3*y2,-z3*Qy);
     } 
     if (type==MR_DOUBLE)
     {
        ZZn x,y,x3,z3;
        extract(A,x,y);
        extract(C,x3,x3,z3);
		w=shuffle((slope*ex2)*Qx-slope*x+ex1,-(z3*ex2)*Qy);
     }

#endif
     return w;
}

//
// Add A=A+B  (or A=A+A) 
// Return line function value
//

ZZn6 g(ECn& A,ECn& B,ZZn3& Qx,ZZn3& Qy)
{
    ZZn  lam,extra1,extra2;
    int type;
    big ptr,ex1,ex2;
    ECn P=A;

// Evaluate line from A
    type=A.add(B,&ptr,&ex1,&ex2);
    if (!type) return (ZZn6)1;
    lam=ptr;
    extra1=ex1;
    extra2=ex2;
    return line(P,A,B,type,lam,extra1,extra2,Qx,Qy);
}

//
// Tate Pairing - note denominator elimination has been applied
//
// P is a point of order q. Q(x,y) is a point of order m.q. 
// Note that P is a point on the curve over Fp, Q(x,y) a point on the 
// twisted curve over the extension field Fp^3
//

#define WINDOW_SIZE 4
#define PRECOMP (1<<(WINDOW_SIZE-1))

BOOL fast_tate_pairing(ECn& P,ZZn3& Qx,ZZn3& Qy,Big &x,ZZn2& X,ZZn6& res)
{ 
    int i,j,n,nb,nbw,nzs;
    ECn A,P2,t[PRECOMP];
    ZZn6 w,hc,z2n,zn[PRECOMP];
	Big q=x*x-x+1;

    res=zn[0]=1;  

    t[0]=P2=A=P;
    z2n=g(P2,P2,Qx,Qy);     // P2=P+P
    normalise(P2);
//
// Build windowing table
//
    for (i=1;i<PRECOMP;i++)
    {           
        hc=g(A,P2,Qx,Qy);     
        t[i]=A;         
        zn[i]=z2n*zn[i-1]*hc;   
    }
	
    multi_norm(PRECOMP,t);  // make t points Affine

/*
    A=P;    // reset A
    nb=bits(q);

    for (i=nb-2;i>=0;i--)
    {
		res*=res;
		res*=g(A,A,Qx,Qy);
		if (bit(q,i)==1)
			res*=g(A,P,Qx,Qy);
        if (res.iszero()) return FALSE;  
    }
*/

	A=P;    // reset A
	nb=bits(q);
    for (i=nb-2;i>=0;i-=(nbw+nzs))
    { // windowing helps a little.. 
        n=window(q,i,&nbw,&nzs,WINDOW_SIZE);  // standard MIRACL windowing
        for (j=0;j<nbw;j++)
        {
            res*=res;    
            res*=g(A,A,Qx,Qy);
        }

        if (n>0)
        {
            res*=zn[n/2]; 
            res*=g(A,t[n/2],Qx,Qy);
        }  

        for (j=0;j<nzs;j++) 
        {
            res*=res; 
            res*=g(A,A,Qx,Qy); 
        }  
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

    if (res==(ZZn6)1) return FALSE;
    return TRUE;            
}

//
// ecap(.) function
//

BOOL ecap(ECn& P,ECn3& Q,Big& x,ZZn2 &X,ZZn6& res)
{
    BOOL Ok;
    ECn PP=P;
    ZZn3 Qx,Qy;
    int qnr=get_mip()->cnr;

    normalise(PP);
    Q.get(Qx,Qy);

// untwist    
    Qx=Qx/qnr;
    Qy=tx(Qy);
    Qy=Qy/(qnr*qnr);

#ifdef MR_COUNT_OPS
fpc=fpa=fpx=0;
#endif  

    Ok=fast_tate_pairing(PP,Qx,Qy,x,X,res);

#ifdef MR_COUNT_OPS
printf("After pairing fpc= %d fpa= %d fpx= %d\n",fpc,fpa,fpx);
fpa=fpc=fpx=0;
#endif

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

void q_power_frobenius(ECn3 &S,ZZn2& X)
{
	ZZn6 X1,X2,Y1,Y2;
	ZZn3 Sx,Sy,T;

	int qnr=get_mip()->cnr;

	S.get(Sx,Sy);

	// untwist    
    Sx=Sx/qnr;
    Sy=tx(Sy);
    Sy=Sy/(qnr*qnr);

	X1=shuffle(Sx,(ZZn3)0); Y1=shuffle((ZZn3)0,Sy);
	X1.powq(X); Y1.powq(X);
	unshuffle(X1,Sx,T); unshuffle(Y1,T,Sy);
	
	// twist
	Sx=qnr*Sx;
	Sy=txd(Sy*qnr*qnr);
	S.set(Sx,Sy);
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

// GLV + Galbraith-Scott

ZZn6 GT_pow(ZZn6& u,Big& k,Big& x,ZZn2& X)
{
	ZZn6 v=u;
	v.powq(X);
	v=powu(v,k/x,u,k%x);
	return v;
}

ECn3 G2_mul(ECn3& P,Big& k,Big& x,ZZn2& X)
{
	ECn3 V=P;
	q_power_frobenius(V,X);
	V=mul(V,k/x,P,k%x);
	return V;
}

int main()
{       
    miracl* mip=&precision;
    ECn Alice,Bob,sA,sB;
    ECn3 B6,Server,sS;
    ZZn6 sp,ap,bp;
	ZZn6 res;
	ZZn2 X;
    Big a,b,s,ss,p,q,x,y,B,cf,t,sru,T;
    int i,A;
    time_t seed;

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

#ifdef AFFINE
    ecurve(A,B,p,MR_AFFINE);
#endif
#ifdef PROJECTIVE
    ecurve(A,B,p,MR_PROJECTIVE);
#endif

	set_frobenius_constant(X);
	sru=pow((ZZn)-2,(p-1)/6);   // x^6+2 is irreducible
    set_zzn3(-2,sru);

    mip->IOBASE=16;
    mip->TWIST=MR_QUADRATIC;   // map Server to point on twisted curve E(Fp3)

    ss=rand(q);    // TA's super-secret 

    cout << "Mapping Server ID to point" << endl;
    Server=hash_and_map3((char *)"Server");
	cofactor(Server,x,X); 

    cout << "Mapping Alice & Bob ID's to points" << endl;
    Alice=hash_and_map((char *)"Alice");
    Bob=  hash_and_map((char *)"Robert");

    cout << "Alice, Bob and the Server visit Trusted Authority" << endl; 

	sS=G2_mul(Server,ss,x,X);
    sA=ss*Alice; 
    sB=ss*Bob; 

    cout << "Alice and Server Key Exchange" << endl;

    a=rand(q);   // Alice's random number
    s=rand(q);   // Server's random number

    if (!ecap(sA,Server,x,X,res)) cout << "Trouble" << endl;

	if (!member(res,x,X))
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
	ap=GT_pow(res,a,x,X);//powu(res,a);

    if (!ecap(Alice,sS,x,X,res)) cout << "Trouble" << endl;
   	if (!member(res,x,X))
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }

	sp=GT_pow(res,s,x,X);

    cout << "Alice  Key= " << H2(powu(sp,a)) << endl;
    cout << "Server Key= " << H2(powu(ap,s)) << endl;

    cout << "Bob and Server Key Exchange" << endl;

    b=rand(q);   // Bob's random number
    s=rand(q);   // Server's random number

    if (!ecap(sB,Server,x,X,res)) cout << "Trouble" << endl;
    if (!member(res,x,X))
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    bp=GT_pow(res,b,x,X);

    if (!ecap(Bob,sS,x,X,res)) cout << "Trouble" << endl;
    if (!member(res,x,X))
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    sp=GT_pow(res,s,x,X);

    cout << "Bob's  Key= " << H2(powu(sp,b)) << endl;
    cout << "Server Key= " << H2(powu(bp,s)) << endl;

    return 0;
}

