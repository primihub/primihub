//
// Program to find Brezing & Weng pairing friendly curves
// http://eprint.iacr.org/2003/143 
//
// Finds families of elliptic curves with groups of points of order r over the prime field p with 
// embedding degree k, where k>2. The program tries to find curves with minimum rho=log(p)/log(r), 
// as such curves may be optimal for fast implementation. The total number of points on the curve is 
// p+1-t, where t is the trace.
//
// To get an actual curve, substitute for x, (ensure p(x) is prime and that r(x) has a large prime factor) 
// and use the CM program to find actual curve parameters.
//
// This method is based on the fact that (t(x)-1) should be a k-th root of unity mod r(x), and r(x) 
// only has k-th roots of unity if it is a divisor of phi_k(t(x)-1), as x^k=1 mod Phi_k(x)
// A simple way to generate t(x) and r(x) is to choose r(x) as Phi_{nk}(x) and then t(x)-1=x^n is a 
// k-th root of unity. However this method is not exhaustive, and there could be better solutions. 
// (That is there may be suitable r(x) which are not of the form Phi_{nk}(x), so these curves are not
// necessarily optimal).
//
// Omega indicates the degree of loop reduction possible compared to the Tate pairing, with the Ate 
// or Eta pairing
//
// This program uses the NTL number theoretic library available from http://www.shoup.net/ntl/
//
// Nov. 2007 - modified to search for the best Ate or Eta Omega
// 
// Mike Scott (2007)
//

#include <fstream>
#include "NTL/ZZXFactoring.h"

NTL_CLIENT
using namespace std;

#define POWER
#define MAXK 64
#define BM 10       // these limits have been tested as sufficient for K<=64
#define BD 20

void output(ZZX& p)
{
    int i,leading=1;
    ZZ c;
    if (p==0) 
    {
        cout << "0" << endl;
        return;
    }
    int len=p.rep.length();

    if (p.rep[0]!=0) 
    {
        leading=0;
        cout << p.rep[0];
    }
    
    if (len==1) return;

    c=p.rep[1];
    if (c<0)
    {
        c=-c;
        cout << "-";
    }
    else if (c>0 && !leading)
        cout << "+";
    if (c!=0)
    {
        leading=0;
        if (c==1)
            cout << "x";
        else
            cout << c << "*x";
    }

    if (len==2) return;
    
    for (i=2;i<len;i++)
    {
        c=p.rep[i];
        if (c<0)
        {
            c=-c;
            cout << "-";
        }
        else if (c>0 && !leading)
            cout << "+";
        if (c!=0)
        {
            leading=0;
            if (c!=1) cout << c << "*";
            if (i==2) cout << "x*x";
            else
#ifdef POWER                
                cout << "pow(x," << i << ")";
#else
                cout << "x^" << i;
#endif
        }
    }
}

void outputfactors(ZZX &f)
{
    int i;
    ZZ c;
    vec_pair_ZZX_long factors;
    factor(c,factors,f);
    if (c!=1) cout << c << ".";
    for (i=0;i<factors.length();i++)
    {
        cout << "("; output(factors[i].a); cout << ")";
        if (factors[i].b>1) cout << "^" << factors[i].b;
    }
}

//
// Composition. Give g(x) return f(x)=g(b(x))
//

ZZX compose(ZZX &g,ZZX &b)
{
    ZZX c;
    int i,d=deg(g);

    vec_ZZX table(INIT_SIZE,d+1);
    table(0)=1;
    for (i=1;i<=d;i++) table(i)=(table(i-1)*b);
    clear(c);

    for (i=0;i<=d;i++)
    {
        c+=g.rep[i]*table(i);
        table(i).kill();
    }

    return c;
}

int omega(ZZX &p,ZZX &pkr)
{ // input phi(k)/r 
    ZZX t,f=pkr;
    int biggest=0;

    while (deg(f)>=0)
    {
        t=f%p;
        if (deg(t)>biggest) biggest=deg(t);
        f/=p;
    }
    return biggest;
}

// evaluate f(x)

ZZ eval(ZZX &f,int x)
{
    ZZ y,xx;
    y=0;
    xx=1;
    for (int i=0;i<=deg(f);i++)
    {
        y+=coeff(f,i)*xx;
        xx*=x;
    }
    return y;
}

unsigned int igcd(unsigned int x,unsigned int y)
{ /* integer GCD, returns GCD of x and y */
    unsigned int r;
    if (y==0) return x;
    while ((r=x%y)!=0)
        x=y,y=r;
    return y;
}

// for given x evaluate f(x) mod m

int evalmod(ZZX &f,int x,int m)
{
    int y,xx;
    y=0;
    xx=1;
    for (int i=0;i<=deg(f);i++)
    {
        y+=(to_long(coeff(f,i))%m)*xx;
        y%=m;
        xx=(x*xx)%m;
    }
    if (y<0) y+=m;
   
    return y;
}

// check if f(x)/m has integer solutions

int solutions(ZZX &f,int m)
{
    int s=0;
    for (int x=0;x<m;x+=1)
        if (evalmod(f,x,m)==0) s++;
    
    return s;
}

// check p(x)/W for irreducibility

int irreducible(ZZX &p,long W)
{
    int x;
    ZZ pp,sr;

// This is a little dodgy....

    for (x=10;x<500;x++)
    {
        pp=eval(p,x);
        if (pp%W!=0) continue;
        pp/=W;    
        sr=SqrRoot(pp);
        if (sr*sr==pp) continue;
        if (ProbPrime(pp,4)) return 1;

        pp=eval(p,-x);
        if (pp%W!=0) continue;
        pp/=W;  
        sr=SqrRoot(pp);
        if (sr*sr==pp) continue;
        if (ProbPrime(pp,4)) return 1;
    }
    return 0;
}

int negative(ZZX &f)
{
    if (LeadCoeff(f)<0 && deg(f)%2==0) return 1;
    return 0;
}

// check a polynomial p(x)/W for a probable prime generator
// and remove any common factor between p and W

int isap(ZZX& p,long& W)
{
    int couldbe,mpj,cc;
    ZZ c,pp;

    c=content(p);
    p/=c;
    cc=to_long(c);
    
    if (W%cc!=0) return 0;
	W/=cc;

    if (!solutions(p,W))   return 0;  
    if (!irreducible(p,W)) return 0;

    return 1;
}

// Check d for square-free

int squarefree(int d)
{
    int sqr=0;
    for (int i=2;;i++)
    {
        if (d%(i*i)==0)
        {
            sqr=1;
            break;
        }
        if (i*i>d) break;
    }
    if (sqr) return 0;
    return 1;
}

// basis for prime p

ZZX rib(long p,long n,ZZX& phi)
{
    int j;
    ZZ r;
    ZZX s,t,b;
    ZZX zeta,z,iz,izeta,f,inf;

    b=1;
    SetCoeff(zeta,n/p,1);
    z=zeta;

    t=InvTrunc(phi,n/p);
    izeta=(1-phi*t)/zeta;
    iz=izeta;

    for (j=1;j<=(p-1)/2;j++)
    { 
        b=MulMod(b,(z-iz)%phi,phi);
        z*=zeta;
        iz*=izeta;
    }
    return b;
}

//
// square root of -d in Q(x), D is negative 
// Murphy & Fitzpatrick
// http://eprint.iacr.org/2005/302
//

int sqrt(ZZX& r,long n,long d,ZZX& phi)
{
    long p,dd;
    ZZX zeta4,zeta8;
    if (d<=0 || (d>1 && n%d!=0)) return 0;
    if (d%2==0 && n%8!=0) return 0;
    if (d%4!=3 && n%4!=0) return 0;

    if (n%4==0) {SetCoeff(zeta4,n/4,1);zeta4%=phi;}
    if (n%8==0) {SetCoeff(zeta8,n/8,1);zeta8%=phi;}

    if (d==1)
    {
        r=zeta4;
        return 1;
    }

    r=1;
    dd=d;
    for (p=2;p<=dd;p++)
    {
        if (d%p!=0) continue;
        dd/=p;
        if (p==2)
        {
            r=MulMod(zeta8,r,phi);
            r=MulMod((1+zeta4),r,phi);          
        }
        else
        {
            r=MulMod(rib(p,n,phi),r,phi);
        }
    }
    if (d%4==1) r=MulMod(zeta4,r,phi);
    if (d%2==0 && (4-(d/2)%4)%4==1) r=MulMod(zeta4,r,phi); 

    return 1;    
}

int main(int argc,char **argv)
{
    int nn,i,j,m,K,mode,min_K,max_K,fp,fast;
    long ww,d,W,x,bd,bn;
//    ZZ m;
    ZZX h,g,a,b,p,r,ff,q,kg;
    ZZX pru,w,T,lambda;
    int twist,small_ate,small_eta;
    unsigned int rho_n,rho_d,best_rho_n,best_rho_d,omega_n,omega_d,best_omega_n,best_omega_d,gcd;
    unsigned int delta_n,delta_d,best_delta_n,best_delta_d;
    argv++; argc--;

    fast=0;
    if (argc!=1)  {K=0; mode=0;}
    else {K=atoi(argv[0]); mode=1; if (K<0) {K=-K; fast=1;}}
   
    if (K!=0 && (K<3 || K>MAXK)) return 0;
	
// generate cyclotomic polynomials

    vec_ZZX phi(INIT_SIZE, BM*MAXK);
    for (i = 1; i <= BM*MAXK; i++) 
    {
      ZZX t;
      t = 1;
      for (j = 1; j <= i-1; j++)
         if (i % j == 0)
            t *= phi(j);
      phi(i) = (ZZX(i, 1) - 1)/t;  // ZZX(i, a) == X^i * a
    }

	if (mode==0) 
	{
        cout << "Finds best Brezing and Weng families of pairing friendly elliptic curves" << endl;
		cout << "To find all individual curves - bandw K, where 3<=K<=" << MAXK << endl;
        cout << "To just see best curves (smallest rho found so far) = bandw -K" << endl;
        cout << "A smaller omega means a shorter Miller loop for ETA or ATE pairing" << endl;
        cout << "Note that sometimes the ETA pairing is possible, as well as ATE" << endl;
 //       cout << "A smaller delta means a faster Ate pairing" << endl;
		min_K=3; 
		max_K=MAXK;
	}
	else {min_K=max_K=K;}

	for (K=min_K;K<=max_K; K++)
	{
		best_rho_n=2; best_rho_d=1; bd=0; bn=0;
        best_omega_n=1; best_omega_d=1;
//        best_delta_n=1; best_delta_d=1;
// Try r(x) as Phi_{nK}(x) - why? Because thats what B&W did.
		for (nn=1;nn<=BM;nn++)
		{
			r=phi(nn*K);
 
// set K-th root of unity
   
            clear(g);
            SetCoeff(g,nn,1);            
			kg=g%r;

// Try for small discriminants...

			for (d=1;d<=BD;d++)
			{    
				if (!squarefree(d)) continue;
				W=4*d;

// find square root of -d mod r

                if (!sqrt(h,nn*K,d,r)) continue;

// try for all the other K-th roots... 
				g=kg;
				for (j=1;j<K;j++)
				{    
					if (igcd(j,K)!=1)
					{ // wrong order...
					    g=MulMod(g,kg,r);
					    continue;
					}
             
					a=(g+1); 
					b=MulMod((a-2)%r,h,r);					
					p=d*a*a+b*b;
					ww=W;
                    rho_n=deg(p); rho_d=deg(r);
					gcd=igcd(rho_n,rho_d);
					rho_n/=gcd;	rho_d/=gcd;

                    if ((!fast && mode==1) || rho_n*best_rho_d<best_rho_n*rho_d)
                    {

					    if (isap(p,ww)) 
					    {
                      //      omega_n=omega(p,compose(phi(K),p)/r); omega_d=deg(p); gcd=igcd(omega_n,omega_d);
                      //      omega_n/=gcd; omega_d/=gcd;
                            omega_n=deg(a); omega_d=deg(r); gcd=igcd(omega_n,omega_d);
                            omega_n/=gcd; omega_d/=gcd;

						    if (mode==1)
						    {
							    cout << "\nK= " << K << " D= " << d <<  " zeta_{" << K*nn << "}" << endl;
							    cout << "#define TRACE(x) ";output(a);cout << endl;
							    cout << "#define PRIME(x) (";output(p);cout << ")/" << ww << endl;  
							    cout << "#define ORDER(x) ";output(r);cout << endl;
                                cout << "#define WW " << ww << endl;
							    cout << "rho= " << rho_n << "/" << rho_d << endl;
                            }
                               
                             w=g; 
                             small_ate=deg(r);

                             for (m=1;m<K;m++)
                             {  
                             //     cout << "m= " << m << "w= ";output(w);cout << endl;
                                if (!negative(w) && deg(w)<small_ate)
                                {
                                    small_ate=deg(w);
                                    T=w;
                                }
                                w=MulMod(w,g,r);
                             }
                             omega_n=small_ate; omega_d=deg(r); gcd=igcd(omega_n,omega_d);
                             omega_n/=gcd; omega_d/=gcd;

                             if (small_ate<deg(r) && mode==1)
                             {
                                 cout << "Best ATE Loop=";output(T); 
                                 cout << ", omega= " << omega_d << "/" << omega_n << endl;
                             }

                             twist=0;
                             if (K%3==0 && d==3) twist=3;
                             if (K%4==0 && d==1) twist=4;
                             if (K%6==0 && d==3) twist=6;

                             if (twist)
                             {   
                                 lambda=g;
                                 for (m=1;m<(K/twist);m++) lambda=MulMod(lambda,g,r);
                                 w=lambda; 
                                 small_eta=deg(r);
                              
                                 for (m=1;m<twist;m++)
                                 {
                                     if (!negative(w) && deg(w)<small_eta)
                                     {
                                         small_eta=deg(w);
                                         T=w;
                                     }
                                     w=MulMod(w,lambda,r);
                                 }
                                 if (small_eta<deg(r) && mode==1)
                                 {
                                     cout << "Best ETA Loop=";output(T); 
                                     cout << ", omega= " << omega_d/igcd(small_eta,omega_d) << "/" << small_eta/igcd(small_eta,omega_d) << endl;
                                 }
                                 if (small_eta<=small_ate)
                                 {
                                     omega_n=small_eta; omega_d=deg(r); gcd=igcd(omega_n,omega_d);
                                     omega_n/=gcd; omega_d/=gcd;     
                                 }
                           //     cout << "delta= " << delta_n << "/" << delta_d << endl << endl;
						    }
						    if (rho_n*best_rho_d<best_rho_n*rho_d)
						    {
							    best_rho_n=rho_n;
							    best_rho_d=rho_d;
                                best_omega_n=omega_n;
                                best_omega_d=omega_d;
                             //   best_delta_n=delta_n;
                             //   best_delta_d=delta_d;

							    bd=d;
                                bn=nn;
						    }
                            else if (rho_n*best_rho_d==best_rho_n*rho_d && omega_n*best_omega_d < best_omega_n*omega_d)
                            {
							    best_rho_n=rho_n;
							    best_rho_d=rho_d;
                                best_omega_n=omega_n;
                                best_omega_d=omega_d;
                          //      best_delta_n=delta_n;
                          //      best_delta_d=delta_d;

							    bd=d;
                                bn=nn;
                            }
                        }
					}
					g=MulMod(g,kg,r);
				}
			}
		}
		cout << "For K= " << K << " with D= " << bd << " best rho= " << best_rho_n << "/" << best_rho_d << " omega= " << best_omega_d << "/" << best_omega_n <<  /* " multiplier= " << bn << */ endl;
	}
    return 0;
}
