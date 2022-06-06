/*

   Scott's AKE Client/Server testbed

   See http://eprint.iacr.org/2002/164

   Compile as 
   cl /O2 /GX /DZZNS=16 ake4mntt.cpp zzn4.cpp zzn2.cpp ecn2.cpp big.cpp zzn.cpp 
   ecn.cpp miracl.lib
   Fastest using COMBA build for 160-bit mod-mul

   The file k4mnt.ecs is required 
   Security is G155/F640 (155-bit group, 640-bit field)

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

Miracl precision(5,0);  

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

ZZn4 line(ECn& A,ECn& C,ECn& B,int type,ZZn& slope,ZZn& ex1,ZZn& ex2,ZZn2& Qx,ZZn2& Qy)
{ 
    ZZn4 w;
#ifdef AFFINE
    ZZn2 m=Qx;
    ZZn x,y
    extract(A,x,y);
    m-=x; m*=slope;  
    w.set((ZZn2)-y,Qy); w-=m;
#endif
#ifdef PROJECTIVE
    if (type==MR_ADD)
    {
        ZZn x2,y2,x3,z3;
        extract(B,x2,y2);
        extract(C,x3,x3,z3);
        w.set(slope*(Qx-x2)+z3*y2,-z3*Qy);
    } 
    if (type==MR_DOUBLE)
    {
        ZZn x,y,x3,z3;
        extract(A,x,y);
        extract(C,x3,x3,z3);
        w.set((slope*ex2)*Qx-slope*x+ex1,-(z3*ex2)*Qy);
    }                   
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
    ZZn  lam,extra1,extra2;
    big ptr,ex1,ex2;
    ECn P=A;

// Evaluate line from A
    type=A.add(B,&ptr,&ex1,&ex2);
    if (!type)   return (ZZn4)1; 
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
// extension field Fp^2
//

BOOL tate(ECn& P,ECn2 Q,Big& q,ZZn2 &Fr,Big cof,ZZn2& r)
{ 
    int i,j,n,nb,nbw,nzs;
    ECn A,P2,t[8];
    ZZn4 w,hc,z2n,zn[8],res;
    ZZn2 Qx,Qy;

#ifdef MR_COUNT_OPS
fpc=fpa=fpx=0;
#endif  

    Q.get(Qx,Qy);
    Qx=txd(Qx);
    Qy=txd(txd(Qy));

    normalise(P);

    res=zn[0]=1;  
    t[0]=P2=A=P;
    z2n=g(P2,P2,Qx,Qy);     // P2=P+P

    normalise(P2);



//
// Build windowing table
//
    for (i=1;i<8;i++)
    {           
        hc=g(A,P2,Qx,Qy);      
        t[i]=A;         
        zn[i]=z2n*zn[i-1]*hc;   
    } 
    multi_norm(8,t);  // make t points Affine

    A=P;
    nb=bits(q);
    for (i=nb-2;i>=0;i-=(nbw+nzs))
    {
        n=window(q,i,&nbw,&nzs,4);  // standard MIRACL windowing
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
    }

    if (!A.iszero()) return FALSE;

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
    X=pow(X,(p-1)/2);
}

int main()
{
    ifstream common("k4mnt.ecs");      // elliptic curve parameters
    miracl* mip=&precision;
    ECn Alice,Bob,sA,sB;
    ECn2 Server,sS;
    ZZn2 res,sp,ap,bp,Fr;
    Big a,b,s,ss,p,q,r,B,delta,fr;
                                       
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
 
    cout << "Initialised... " << endl;
 
    time(&seed);
    irand((long)seed);

#ifdef AFFINE
    ecurve(A,B,p,MR_AFFINE);
#endif
#ifdef PROJECTIVE
    ecurve(A,B,p,MR_PROJECTIVE);
#endif

 //   set_frobenius_constant(Fr);

    Fr=(ZZn2)fr; 
    mip->IOBASE=16;
    mip->TWIST=MR_QUADRATIC;   // map Server to point on twisted curve E(Fp2)

	//cout << "p= " << p << endl;
	//cout << "delta*delta+delta+1= " << delta*delta-delta+1 << endl;

// hash Identities to curve point

    ss=rand(q);    // TA's super-secret 

    cout << "Mapping Server ID to point" << endl;
    Server=hash2((char *)"Server");

    cout << "Mapping Alice & Bob ID's to points" << endl;
    Alice=hash_and_map((char *)"Alice",COF);
//cout << "Server= " << Server << endl;
//cout << "Alice= " << Alice << endl;
    Bob=  hash_and_map((char *)"Robert",COF);
    cout << "Alice, Bob and the Server visit Trusted Authority" << endl; 

    sS=ss*Server; 
    sA=ss*Alice; 
    sB=ss*Bob; 

    cout << "Alice and Server Key Exchange" << endl;

    a=rand(q);   // Alice's random number
    s=rand(q);   // Server's random number

    if (!tate(sA,Server,q,Fr,delta,res)) cout << "Trouble" << endl;

    if (powl(res,q)!=(ZZn2)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    ap=powl(res,a);

    if (!tate(Alice,sS,q,Fr,delta,res)) cout << "Trouble" << endl;
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

    if (!tate(sB,Server,q,Fr,delta,res)) cout << "Trouble" << endl;
    if (powl(res,q)!=(ZZn2)1)
    {
        cout << "Wrong group order - aborting" << endl;
        exit(0);
    }
    bp=powl(res,b);

    if (!tate(Bob,sS,q,Fr,delta,res)) cout << "Trouble" << endl;
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
