//
// cl /O2 /GX bls12.cpp zzn2.cpp zzn.cpp ecn2.cpp ecn.cpp big.cpp miracl.lib
// Program to generate Barreto-Lynn-Scott k=12 rho=1.5 curves for use by ake12blsa.cpp
//

#include <iostream>
#include "big.h"
#include "ecn.h"
#include "ecn2.h"

using namespace std;

Miracl precision=100;

int main()
{
    int ns,twist,best_ham=1000;
    int sign;
    BOOL got_one;
    Big cof,r,m1,m2,n,p,t,s,x,y,b,w,best_s;
    Big PP,TT,FF;
    miracl*mip=&precision;
    ECn P;
    ECn2 Q;
    ZZn2 xi;
    
    mip->IOBASE=16;

    s="C000000000000000";
			
    ns=1;
	forever
	{
		s+=1;
		for (sign=1;sign<=2;sign++)
		{
			if (sign==1) x=s;
			else         x=-s;
 
			if (x<0 || ham(x)>9) continue; // filter out difficult or poor solutions
         
			p=pow(x,6)-2*pow(x,5)+2*pow(x,3)+x+1;

			if (p%8==1) continue;
			if (p%3!=0) continue;   // check congruence conditions
			p/=3;
			if (p%8==7 && (p%5==1 || p%5==4)) continue;
   
			t=x+1;
				
			n=p+1-t;
			r=pow(x,4)-x*x+1;

			if (!prime(r))  continue;

			if (!prime(p)) continue;
			modulo(p);

			if (p%8==5) xi.set(0,1);
			if (p%8==3) xi.set(1,1);
			if (p%8==7) xi.set(2,1);
			if (pow(xi,(p*p-1)/2)==1) continue;
			if (pow(xi,(p*p-1)/3)==1) continue;  // make sure that x^6+c is irreducible
        
			cof=n/r;

// find number of points on sextic twist..

			TT=t*t-2*p;
			PP=p*p;

			FF=sqrt((4*PP-TT*TT)/3);

			m1=PP+1-(-3*FF+TT)/2;  // 2 possibilities...
		
//		m2=PP+1-(3*FF+TT)/2;   // wrong one

			b=0;
			forever
			{
				b+=1;
				ecurve(0,b,p,MR_AFFINE);
				while (!P.set(rand(p))) ;
				P*=cof;
				if ((r*P).iszero()) break; // right curve
			}
			got_one=FALSE;

			mip->TWIST=MR_SEXTIC_M;
			while (!Q.set(randn2())) ;

			Q*=(m1/r);

			if ((r*Q).iszero()) got_one=TRUE;
			else
			{
				mip->TWIST=MR_SEXTIC_D;
				while (!Q.set(randn2())) ;

				Q*=(m1/r);

				if ((r*Q).iszero()) got_one=TRUE;
			}
			if (!got_one) {cout << "Bad twist" << endl; exit(0);}  // Huh?

			if (mip->TWIST==MR_SEXTIC_M) continue;  // not interested just now

			cout << "solution " << ns << endl;
			cout << "irreducible polynomial = X^6 + " << xi << endl;
			cout << "p=" << p << endl;
			cout << "p mod 72 = " << p%72 << endl;
			cout << "x mod 72 = " << x%72 << endl;
	
			cout << "x= " << x << endl;
			cout << "bits(p)= " << bits(p) << endl;
			cout << "r=" << r << endl;
			cout << "t=" << t << endl;
			cout << "m1= " << m1 << endl;
			cout << "cof= " << m1/r << endl;
   
			cout << "bits(t)= " << bits(t) << endl;
			cout << "ham(t-1) = " << ham(t-1) << " (small is better for Ate pairing)" << endl;
			cout << "bits(r)= " << bits(r) << endl;
   
			cout << "p%8= " << p%8 << endl;
			mip->IOBASE=10;
			cout << "E(Fp): y^2=x^3+" << b << endl;
			mip->IOBASE=16;
			if (mip->TWIST==MR_SEXTIC_M) cout << "Twist type M" << endl;
			if (mip->TWIST==MR_SEXTIC_D) cout << "Twist type D" << endl;

			Q*=r;
			cout << "check - if right twist should be O - r*Q= " << Q << endl;	
			ns++;
			if (ham(x)<best_ham) {best_ham=ham(x);best_s=s;}	
			cout << "So far minimum hamming weight of x= " << best_ham << endl;
			cout << "for seed= " << best_s << endl;

			cout << endl;
		}
	}
    return 0;
}

