/*
 *
 * cp_pair.cpp
 *
 * Cocks-Pinch curve, Tate pairing embedding degree 2, ideal for security level AES-80
 *
 * Provides high level interface to pairing functions
 * 
 * GT=pairing(G2,G1)
 *
 * This is calculated on a Pairing Friendly Curve (PFC), which must first be defined.
 *
 * G1 is a point over the base field, G2 is a point on the quadratic twist
 * GT is a finite field point over the 2nd extension, where 2 is the embedding degree.
 *
 */

#define MR_PAIRING_CP
#include "pairing_3.h"

// Cocks-Pinch curve parameters, A,B and n, where p=3 mod 4
// AES_SECURITY=80 bit curve
// Curve E:y^2=x^3-3x+B, #E=COF*order, modulus p

static char MODtext[]="8D5006492B424C09D2FEBE717EE382A57EBE3A352FC383E1AC79F21DDB43706CFB192333A7E9CF644636332E83D90A1E56EFBAE8715AA07883483F8267E80ED3";
static char Btext[]="609993837367998001C95B87A6BA872135E26906DB4C192D6E038486177A3EDF6C50B9BB20DF881F2BD05842F598F3E037B362DBF89F0A62E5871D41D951BF8E";
static char COFtext[]="11AA00C9256849813A5FD7CE2FDC7054AFD7809E7F7FD948C4B9C1C1E76FFEFF4ECAB83C950112DECB41D6EDA";

void read_only_error(void)
{
	cout << "Attempt to write to read-only object" << endl; 
	exit(0);
}

// Using SHA256 as basic hash algorithm
//
// Hash function
// 

#define HASH_LEN 32

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

void PFC::start_hash(void)
{
	shs256_init(&SH);
}

Big PFC::finish_hash_to_group(void)
{
	Big hash;
	char s[HASH_LEN];
    shs256_hash(&SH,s);
    hash=from_binary(HASH_LEN,s);
	return hash%(*ord);
}

void PFC::add_to_hash(const GT& x)
{ // compress it and add
	ZZn2 u=x.g;
	Big a;
	int m;

	u.get(a);
 
    while (a>0)
    {
        m=a%256;
        shs256_process(&SH,m);
        a/=256;
    }
}

void PFC::add_to_hash(const G1& x)
{
	Big a,X,Y;
	int i,m;
	x.g.get(X,Y);
	a=X;
    while (a>0)
    {
        m=a%256;
        shs256_process(&SH,m);
        a/=256;
    }
	a=Y;
    while (a>0)
    {
        m=a%256;
        shs256_process(&SH,m);
        a/=256;
    }
}

void PFC::add_to_hash(const G2& x)
{
	Big a,X,Y;
	int i,m;
	x.g.get(X,Y);
	a=X;
    while (a>0)
    {
        m=a%256;
        shs256_process(&SH,m);
        a/=256;
    }
	a=Y;
    while (a>0)
    {
        m=a%256;
        shs256_process(&SH,m);
        a/=256;
    }
}

void PFC::add_to_hash(const Big& x)
{
	int m;
	Big a=x;
    while (a>0)
    {
        m=a%256;
        shs256_process(&SH,m);
        a/=256;
    }
}

Big H2(ZZn2 y)
{ // Hash and compress an Fp2 to a big number
    sha256 sh;
    Big a,h;
    char s[HASH_LEN];
    int m;

    shs256_init(&sh);
	y.get(a);
   
    while (a>0)
    {
        m=a%256;
        shs256_process(&sh,m);
        a/=256;
    }
    shs256_hash(&sh,s);
    h=from_binary(HASH_LEN,s);
    return h;
}

#ifndef MR_AFFINE_ONLY

void force(ZZn& x,ZZn& y,ZZn& z,ECn& A)
{  // A=(x,y,z)
    copy(getbig(x),A.get_point()->X);
    copy(getbig(y),A.get_point()->Y);
    copy(getbig(z),A.get_point()->Z);
    A.get_point()->marker=MR_EPOINT_GENERAL;
}

void extract(ECn &A, ZZn& x,ZZn& y,ZZn& z)
{ // (x,y,z) <- A
    big t;
    x=(A.get_point())->X;
    y=(A.get_point())->Y;
    t=(A.get_point())->Z;
    if (A.get_status()!=MR_EPOINT_GENERAL) z=1;
    else                                   z=t;
}

#endif

void force(ZZn& x,ZZn& y,ECn& A)
{ // A=(x,y)
    copy(getbig(x),A.get_point()->X);
    copy(getbig(y),A.get_point()->Y);
    A.get_point()->marker=MR_EPOINT_NORMALIZED;
}

void extract(ECn& A,ZZn& x,ZZn& y)
{ // (x,y) <- A
    x=(A.get_point())->X;
    y=(A.get_point())->Y;
}

void extractZ(ECn& A,ZZn& z)
{ 
    big t;
    t=(A.get_point())->Z;
    if (A.get_status()!=MR_EPOINT_GENERAL) z=1;
    else                                   z=t;
}

//
// Line from A to destination C. Let A=(x,y)
// Line Y-slope.X-c=0, through A, so intercept c=y-slope.x
// Line Y-slope.X-y+slope.x = (Y-y)-slope.(X-x) = 0
// Now evaluate at Q -> return (Qy-y)-slope.(Qx-x)
//

ZZn2 line(ECn& A,ECn& C,ECn& B,int type,ZZn& slope,ZZn& ex1,ZZn& ex2,ZZn& Px,ZZn& Py)
{ 
    ZZn2 w;
    ZZn x,y,z3;

	extractZ(C,z3);
    if (type==MR_ADD)
    {
       extract(B,x,y);
       w.set(slope*(x+Px)-z3*y,z3*Py);
    } 
    if (type==MR_DOUBLE)
    { 
       extract(A,x,y);
       w.set(-(slope*ex2)*Px-slope*x+ex1,-(z3*ex2)*Py);
    }

/*
    extract(A,x,y,z);     
    x*=z; t=z; z*=z; z*=t;       // 9 ZZn muls   
    n*=z; n+=x; n*=slope;
	d*=z; w.set(-y,d);
    extractZ(C,z3);

    w*=z3; w+=n;
*/

//	w.set(Px*z*z*z*slope+slope*x*z-y*z3,Py*z*z*z*z3);
    return w;
}

//
// Add A=A+B  (or A=A+A) 
// Return line function value
//

ZZn2 g(ECn& A,ECn& B,ZZn& Px,ZZn& Py)
{
    int type;
    ZZn  lam,extra1,extra2;
    ZZn2 u;
    ECn P=A;
    big ptr,ex1,ex2;

    type=A.add(B,&ptr,&ex1,&ex2);
    if (!type) return (ZZn2)1;
    lam=ptr;
	extra1=ex1;
	extra2=ex2;
    
    return line(P,A,B,type,lam,extra1,extra2,Px,Py);
}

// if multiples of G2 can be precalculated, its a lot faster!

ZZn2 gp(ZZn* ptable,int &j,ZZn& Px,ZZn& Py)
{
	ZZn2 w;
	w.set(ptable[j]*Px+ptable[j+1],Py);
	j+=2;
	return w;
}

//
// Spill precomputation on pairing to byte array
//

int PFC::spill(G2& w,char *& bytes)
{
	int i,j,n=2*(bits(*ord-1)-2+ham(*ord));
	int bytes_per_big=(MIRACL/8)*(get_mip()->nib-1);
	int len=n*bytes_per_big;
	Big x;
	if (w.ptable==NULL) return 0;

	bytes=new char[len];
	for (i=j=0;i<n;i++)
	{
		x=w.ptable[i];
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
	}

	delete [] w.ptable; 
	w.ptable=NULL;
	return len;
}

//
// Restore precomputation on pairing to byte array
//

void PFC::restore(char * bytes,G2& w)
{
	int i,j,n=2*(bits(*ord-1)-2+ham(*ord));
	int bytes_per_big=(MIRACL/8)*(get_mip()->nib-1);
	int len=n*bytes_per_big;
	Big x;

	if (w.ptable!=NULL) return;

	w.ptable=new ZZn[n];
	for (i=j=0;i<n;i++)
	{
		x=from_binary(bytes_per_big,&bytes[j]);
		w.ptable[i]=x;
		j+=bytes_per_big;
	}
	for (i=0;i<len;i++) bytes[i]=0;
	delete [] bytes;
}


// precompute G2 table for pairing

int PFC::precomp_for_pairing(G2& w)
{
	int i,j,nb,len;
	ECn A,Q,B;
	ZZn lam,x,y;
	big ptr;
	Big iters=*ord-1;
	
	A=w.g;
	normalise(A);
	B=A;
	nb=bits(iters);
	j=0;
	len=2*(nb-2+ham(*ord));
	w.ptable=new ZZn[len];
	get_mip()->coord=MR_AFFINE;
    for (i=nb-2;i>=0;i--)
    {
		Q=A;
// Evaluate line from A to A+B
		A.add(A,&ptr);
		lam=ptr;
		extract(Q,x,y);
		w.ptable[j++]=lam; w.ptable[j++]=lam*x-y; 

		if (bit(iters,i)==1)
		{
			Q=A;
			A.add(B,&ptr);
			lam=ptr;
			extract(Q,x,y);
			w.ptable[j++]=lam; w.ptable[j++]=lam*x-y; 
		}
    }
	get_mip()->coord=MR_PROJECTIVE;
	return len;
}

GT PFC::multi_miller(int n,G2** QQ,G1** PP)
{
	GT z;
    ZZn *Px,*Py;
	int i,j,*k,nb;
    ECn *Q,*A;
	ECn P;
    ZZn2 res;
	Big iters=*ord-1;

	Px=new ZZn[n];
	Py=new ZZn[n];
	Q=new ECn[n];
	A=new ECn[n];
	k=new int[n];

    nb=bits(iters);
	res=1;  

	for (j=0;j<n;j++)
	{
		k[j]=0;
		P=PP[j]->g; normalise(P); Q[j]=QQ[j]->g; normalise(Q[j]);
		extract(P,Px[j],Py[j]);
	}

	for (j=0;j<n;j++) A[j]=Q[j];

	for (i=nb-2;i>=0;i--)
	{
		res*=res;
		for (j=0;j<n;j++)
		{
			if (QQ[j]->ptable==NULL)
				res*=g(A[j],A[j],Px[j],Py[j]);
			else
				res*=gp(QQ[j]->ptable,k[j],Px[j],Py[j]);
		}
		if (bit(iters,i)==1)
			for (j=0;j<n;j++) 
			{
				if (QQ[j]->ptable==NULL)
					res*=g(A[j],Q[j],Px[j],Py[j]);
				else
					res*=gp(QQ[j]->ptable,k[j],Px[j],Py[j]);
			}
		if (res.iszero()) return 0;  
	}

	delete [] k;
	delete [] A;
	delete [] Q;
	delete [] Py;
	delete [] Px;

	z.g=res;
	return z;
}

//
// Tate Pairing G1 x G1 -> GT
//
// P and Q are points of order q in G1. 
//

GT PFC::miller_loop(const G2& QQ,const G1& PP)
{ 
	GT z;
    int i,j,n,nb,nbw,nzs;
    ECn A,Q;
	ECn P;
	ZZn Px,Py;
	BOOL precomp;
    ZZn2 res;
	Big iters=*ord-1; // can omit last addition

	P=PP.g; Q=QQ.g;
	precomp=FALSE;
	if (QQ.ptable!=NULL) precomp=TRUE;

	normalise(P);
	normalise(Q);
	extract(P,Px,Py);
	//Px=-Px;

    res=1;  
    A=Q;    // reset A
    nb=bits(iters);

	j=0;

    for (i=nb-2;i>=0;i--)
    {
		res*=res;
		if (precomp) res*=gp(QQ.ptable,j,Px,Py);
		else         res*=g(A,A,Px,Py);

		if (bit(iters,i)==1)
		{
			if (precomp) res*=gp(QQ.ptable,j,Px,Py);
			else         res*=g(A,Q,Px,Py);
		}
    }

	z.g=res;
	return z;
}

GT PFC::final_exp(const GT& z)
{
	GT y;
	ZZn2 res;

	res=z.g;
	res=conj(res)/res;
    res=pow(res,(*mod+1)/(*ord));   // raise to power of (p^2-1)/q

    y.g=res;

	return y;
}

PFC::PFC(int s, csprng *rng)
{
	int mod_bits,words;

	if (s!=80)
	{
		cout << "No suitable curve available" << endl;
		exit(0);
	}

	mod_bits=512;

	if (mod_bits%MIRACL==0)
		words=(mod_bits/MIRACL);
	else
		words=(mod_bits/MIRACL)+1;

#ifdef MR_SIMPLE_BASE
	miracl *mip=mirsys((MIRACL/4)*words,16);
#else
	miracl *mip=mirsys(words,0); 
	mip->IOBASE=16;
#endif

	B=new Big;
	mod=new Big;
	ord=new Big;
	cof=new Big;
	npoints=new Big;
	trace=new Big;

	*B=Btext;

	*cof=COFtext;
	*ord=pow((Big)2,159)+pow((Big)2,17)+1;
	*npoints=*cof*(*ord);

	S=s;
	*mod=MODtext;
	*trace=*mod+1-*npoints;

	ecurve(-3,*B,*mod,MR_PROJECTIVE);

	RNG=rng;
}

PFC::~PFC()
{
	delete B;
	delete mod;
	delete ord;
	delete cof;
	delete npoints;
	delete trace;
	mirexit();
}

G1 PFC::mult(const G1& w,const Big& k)
{
	G1 z;
	if (w.mtable!=NULL)
	{ // we have precomputed values

		Big e=k;
		if (k<0) e=-e;

		int i,j,t=w.mtbits; // MR_ROUNDUP(2*S,WINDOW_SIZE); 
		j=recode(e,t,WINDOW_SIZE,t-1);
		z.g=w.mtable[j];
		for (i=t-2;i>=0;i--)
		{
			j=recode(e,t,WINDOW_SIZE,i);
			z.g+=z.g;
			if (j>0) z.g+=w.mtable[j];
		}
		if (k<0) z.g=-z.g;
	}
	else
	{
		z.g=w.g;
		z.g*=k;
	}
	return z;
}

G2 PFC::mult(const G2& w,const Big& k)
{
	G2 z;
	if (w.mtable!=NULL)
	{ // we have precomputed values

		Big e=k;
		if (k<0) e=-e;

		int i,j,t=w.mtbits; // MR_ROUNDUP(2*S,WINDOW_SIZE); 
		j=recode(e,t,WINDOW_SIZE,t-1);
		z.g=w.mtable[j];
		for (i=t-2;i>=0;i--)
		{
			j=recode(e,t,WINDOW_SIZE,i);
			z.g+=z.g;
			if (j>0) z.g+=w.mtable[j];
		}
		if (k<0) z.g=-z.g;
	}
	else
	{
		z.g=w.g;
		z.g*=k;
	}
	return z;
}

GT PFC::power(const GT& w,const Big& k)
{
	GT z;
	Big e=k;
	if (k<0) e=-e;
	if (w.etable!=NULL)
	{ // precomputation is available
		int i,j,t=w.etbits;  //MR_ROUNDUP(2*S,WINDOW_SIZE); 
		j=recode(e,t,WINDOW_SIZE,t-1);
		z.g=w.etable[j];
		for (i=t-2;i>=0;i--)
		{
			j=recode(e,t,WINDOW_SIZE,i);
			z.g*=z.g;
			if (j>0) z.g*=w.etable[j];
		}
	}
	else
	{
		z.g=powu(w.g,e);
	}
	if (k<0) z.g=conj(z.g);
	return z;
}

// random group member

void PFC::random(Big& w)
{
	if (RNG==NULL) w=rand(*ord);
	else w=strong_rand(RNG,*ord);
}

// random AES key

void PFC::rankey(Big& k)
{
	if (RNG==NULL)	k=rand(S,2);
	else k=strong_rand(RNG,S,2);
}

// Can be done deterministicly

void PFC::hash_and_map(G1& w,char *ID)
{
    Big x0=H1(ID);
    while (!w.g.set(x0,x0)) x0+=1;
	w.g*=*cof;
}

void PFC::hash_and_map(G2& w,char *ID)
{
    Big x0=H1(ID);
	*B=-(*B);
	ecurve((Big)-3,*B,*mod,MR_PROJECTIVE);  // move to twist
    while (!w.g.set(x0,x0)) x0+=1;
	w.g*=(*mod+1+*trace)/(*ord);
	*B=-(*B);
	ecurve((Big)-3,*B,*mod,MR_PROJECTIVE);  // move back
}

void PFC::random(G1& w)
{
	Big x0;
	if (RNG==NULL) x0=rand(*mod);
	else x0=strong_rand(RNG,*mod);
	while (!w.g.set(x0,x0)) x0+=1;
	w.g*=*cof;
}

void PFC::random(G2& w)
{
	Big x0;
	if (RNG==NULL) x0=rand(*mod);
	else x0=strong_rand(RNG,*mod);

	*B=-(*B);
	ecurve((Big)-3,*B,*mod,MR_PROJECTIVE);  // move to twist
    while (!w.g.set(x0,x0)) x0+=1;
	w.g*=(*mod+1+*trace)/(*ord);
	*B=-(*B);
	ecurve((Big)-3,*B,*mod,MR_PROJECTIVE);  // move back
}

Big PFC::hash_to_aes_key(const GT& w)
{
	Big m=pow((Big)2,S);
	return H2(w.g)%m;
}

Big PFC::hash_to_group(char *ID)
{
	Big m=H1(ID);
	return m%(*ord);
}

GT operator*(const GT& x,const GT& y)
{
	GT z=x;
	z.g*=y.g;
	return z; 
}

GT operator/(const GT& x,const GT& y)
{
	GT z=x;
	z.g*=conj(y.g); // elements in GT are unitary
	return z; 
}

//
// spill precomputation on GT to byte array
//

int GT::spill(char *& bytes)
{
	int i,j,n=(1<<WINDOW_SIZE);
	int bytes_per_big=(MIRACL/8)*(get_mip()->nib-1);
	int len=n*2*bytes_per_big;
	Big x,y;

	if (etable==NULL) return 0;

	bytes=new char[len];
	for (i=j=0;i<n;i++)
	{
		etable[i].get(x,y);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
	}
	delete [] etable; 
	etable=NULL;
	return len;
}

//
// restore precomputation for GT from byte array
//

void GT::restore(char *bytes)
{
	int i,j,n=(1<<WINDOW_SIZE);
	int bytes_per_big=(MIRACL/8)*(get_mip()->nib-1);
	int len=n*2*bytes_per_big;
	Big x,y;
	if (etable!=NULL) return;

	etable=new ZZn2[1<<WINDOW_SIZE];
	for (i=j=0;i<n;i++)
	{
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		etable[i].set(x,y);
	}
	delete [] bytes;
}

G1 operator+(const G1& x,const G1& y)
{
	G1 z=x;
	z.g+=y.g;
	return z;
}

G1 operator-(const G1& x)
{
	G1 z=x;
	z.g=-z.g;
	return z;
}

//
// spill precomputation on G1 to byte array
//

int G1::spill(char *& bytes)
{
	int i,j,n=(1<<WINDOW_SIZE);
	int bytes_per_big=(MIRACL/8)*(get_mip()->nib-1);
	int len=n*2*bytes_per_big;
	Big x,y;

	if (mtable==NULL) return 0;

	bytes=new char[len];
	for (i=j=0;i<n;i++)
	{
		mtable[i].get(x,y);
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
	}
	delete [] mtable; 
	mtable=NULL;
	return len;
}

//
// restore precomputation for G1 from byte array
//

void G1::restore(char *bytes)
{
	int i,j,n=(1<<WINDOW_SIZE);
	int bytes_per_big=(MIRACL/8)*(get_mip()->nib-1);
	int len=n*2*bytes_per_big;
	Big x,y;
	if (mtable!=NULL) return;

	mtable=new ECn[1<<WINDOW_SIZE];
	for (i=j=0;i<n;i++)
	{
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		mtable[i].set(x,y);
	}
	delete [] bytes;
}


G2 operator+(const G2& x,const G2& y)
{
	G2 z=x;
	z.g+=y.g;
	return z;
}

G2 operator-(const G2& x)
{
	G2 z=x;
	z.g=-z.g;
	return z;
}

//
// spill precomputation on G2 to byte array
//

int G2::spill(char *& bytes)
{
	int i,j,n=(1<<WINDOW_SIZE);
	int bytes_per_big=(MIRACL/8)*(get_mip()->nib-1);
	int len=n*2*bytes_per_big;
	Big x,y;

	if (mtable==NULL) return 0;

	bytes=new char[len];

	for (i=j=0;i<n;i++)
	{
		mtable[i].get(x,y);		
		to_binary(x,bytes_per_big,&bytes[j],TRUE);
		x=from_binary(bytes_per_big,&bytes[j]);

		j+=bytes_per_big;
		to_binary(y,bytes_per_big,&bytes[j],TRUE);
		j+=bytes_per_big;
	}
	delete [] mtable; 
	mtable=NULL;
	return len;
}

//
// restore precomputation for G2 from byte array
//

void G2::restore(char *bytes)
{
	int i,j,n=(1<<WINDOW_SIZE);
	int bytes_per_big=(MIRACL/8)*(get_mip()->nib-1);
	int len=n*2*bytes_per_big;
	Big x,y,B;
	if (mtable!=NULL) return;

	mtable=new ECn[1<<WINDOW_SIZE];
	B=getB();
	B=-B;
	ecurve((Big)-3,B,get_modulus(),MR_PROJECTIVE);  // move to twist	
	for (i=j=0;i<n;i++)
	{
		x=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;
		y=from_binary(bytes_per_big,&bytes[j]);
		j+=bytes_per_big;

		mtable[i].set(x,y);

	}
	B=-B;
	ecurve((Big)-3,B,get_modulus(),MR_PROJECTIVE);  // move back
	delete [] bytes;
}

BOOL PFC::member(const GT& z)
{
	ZZn2 r=z.g;

	if (pow(r,*ord)!=(ZZn2)1) return FALSE;

	return TRUE;
}

GT PFC::pairing(const G2& x,const G1& y)
{
	GT z;
	z=miller_loop(x,y);
	z=final_exp(z);
	return z;
}

GT PFC::multi_pairing(int n,G2 **y,G1 **x)
{
	GT z;
	z=multi_miller(n,y,x);
	z=final_exp(z);
	return z;

}

int PFC::precomp_for_mult(G1& w,BOOL small)
{
	ECn v=w.g;
	int i,j,k,bp,is,t;
	if (small) t=MR_ROUNDUP(2*S,WINDOW_SIZE);
	else       t=MR_ROUNDUP(bits(*ord),WINDOW_SIZE);
	w.mtable=new ECn[1<<WINDOW_SIZE];
	w.mtable[1]=v;
	w.mtbits=t;
	for (j=0;j<t;j++)
        v+=v;
    k=1;
    for (i=2;i<(1<<WINDOW_SIZE);i++)
    {
        if (i==(1<<k))
        {
            k++;
			normalise(v);
			w.mtable[i]=v;     
            for (j=0;j<t;j++)
				v+=v;
            continue;
        }
        bp=1;
        for (j=0;j<k;j++)
        {
            if (i&bp)
			{
				is=1<<j;
				w.mtable[i]+=w.mtable[is];
			}
            bp<<=1;
        }
        normalise(w.mtable[i]);
    }
	return (1<<WINDOW_SIZE);
}

int PFC::precomp_for_mult(G2& w,BOOL small)
{
	ECn v;
	int i,j,k,bp,is,t;
	if (small) t=MR_ROUNDUP(2*S,WINDOW_SIZE);
	else       t=MR_ROUNDUP(bits(*ord),WINDOW_SIZE);
	normalise(w.g);
	v=w.g;
	w.mtable=new ECn[1<<WINDOW_SIZE];
	w.mtable[1]=v;
	w.mtbits=t;
	for (j=0;j<t;j++)
        v+=v;
    k=1;
    for (i=2;i<(1<<WINDOW_SIZE);i++)
    {
        if (i==(1<<k))
        {
            k++;
			normalise(v);
			w.mtable[i]=v;     
            for (j=0;j<t;j++)
				v+=v;
            continue;
        }
        bp=1;
        for (j=0;j<k;j++)
        {
            if (i&bp)
			{
				is=1<<j;
				w.mtable[i]+=w.mtable[is];
			}
            bp<<=1;
        }
        normalise(w.mtable[i]);
    }
	return (1<<WINDOW_SIZE);
}

int PFC::precomp_for_power(GT& w,BOOL small)
{
	ZZn2 v=w.g;
	int i,j,k,bp,is,t;
	if (small) t=MR_ROUNDUP(2*S,WINDOW_SIZE);
	else       t=MR_ROUNDUP(bits(*ord),WINDOW_SIZE);
	w.etable=new ZZn2[1<<WINDOW_SIZE];
	w.etable[0]=1;
	w.etable[1]=v;
	w.etbits=t;
	for (j=0;j<t;j++)
        v*=v;
    k=1;

    for (i=2;i<(1<<WINDOW_SIZE);i++)
    {
        if (i==(1<<k))
        {
            k++;
			w.etable[i]=v;     
            for (j=0;j<t;j++)
				v*=v;
            continue;
        }
        bp=1;
		w.etable[i]=1;
        for (j=0;j<k;j++)
        {
            if (i&bp)
			{
				is=1<<j;
				w.etable[i]*=w.etable[is];
			}
            bp<<=1;
        }
    }
	return (1<<WINDOW_SIZE);
}
