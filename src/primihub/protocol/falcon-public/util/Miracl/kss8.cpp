//
// cl /O2 /GX kss8.cpp zzn2.cpp zzn.cpp ecn2.cpp ecn.cpp big.cpp miracl.lib
// Program to generate KSS k=8 curves.
//
// Twist is represented as y^2=x^y+iA, and the untwisting formula is
//
// E' -> E (x,y) -> i^{1/2}.x/i,i^{1/4}.y/i
//

#include <iostream>
#include "ecn2.h"
#include "ecn.h"

Miracl precision(50,0);

int main()
{
	miracl *mip=&precision;
	Big s,x,q,p,t,A,B,cf,X,Y,sru,n,best_s,f;
	Big T,P,F,m1,m2;
	ECn W;
	ECn2 Q;
	ZZn2 r;
	mip->IOBASE=16;
	int i,ns,sign,best_ham=1000;
    sign=1;  // 1= positive, 2=negative for +/- x solutions
	s="1400000000000000";
	ns=1;
	for (i=0;i<100;i++)
	{
		forever
		{
			forever
			{
				sign=3-sign;    // always looking for +ve x solutions.
				if (sign==1) s+=1;
				if (sign==1) x=5+30*s;
				else         x=5-30*s;

				t=(2*pow(x,3) - 11*x + 15)/15;
				q=(pow(x,4) - 8*pow(x,2) + 25)/450;
				cf=(5*x*x+10*x+25)/2;
				n=cf*q;
				p=cf*q+t-1;  // avoids overflow..

				if (p%8!=5) continue;  // p will be 1 mod 4
				if (!prime(q)) continue;
				if (!prime(p)) continue;
		
				break;
			}

			T=t*t-2*p;
			P=p*p;
			F=(4*P-T*T);

			F=sqrt(F);
			m1=P+1-F;  // Wrong Curve
 
			m2=P+1+F;

			A=0; B=0;
			forever
			{
				A+=1;
				do
				{
					X=rand(p);
					Y=sqrt(X*X*X+A*X,p);
				} while (Y==0);
        
				ecurve(A,B,p,MR_AFFINE);

				W.set(X,Y);
				W*=cf;
				if ((q*W).iszero()) break;
			}

			mip->TWIST=MR_QUARTIC_M;
			do
			{
				r=randn2();
			} while (!Q.set(r)); 
  
			Q*=(m2/q);
 
			if ((q*Q).iszero()) break;

			mip->TWIST=MR_QUARTIC_D;
			do
			{
				r=randn2();
			} while (!Q.set(r)); 
  
			Q*=(m2/q);
 
			if ((q*Q).iszero()) break;

			cout << "Something wrong!" << endl;
			exit(0);
		}
		cout << "solution " << ns << endl;
		cout << "irreducible polynomial = X^4 - [0,1]" << endl;

		if (sign==1)
		{
			cout << "s= +" << s << endl;
			cout << "s%12= " << s%12 << endl;
		}
		else
		{
			cout << "s= -" << s << endl;
			cout << "s%12= " << 12-(s%12) << endl;
		}
		cout << "x= " << x << " ham(x)= " << ham(x) << endl;
		cout << "p= " << p << " bits(p)= " << bits(p) << endl;
		cout << "q= " << q << " bits(q)= " << bits(q) << endl;
		cout << "n= " << n << endl;
		cout << "t= " << t << endl;
		cout << "cf= " << cf << endl;

		//cout << "W= " << W << endl;
		cout << "q*W= " << q*W << endl;
		mip->IOBASE=10;
		cout << "E(Fp): y^2=x^3+" << A << "x" << endl;
		mip->IOBASE=16;
		if (mip->TWIST==MR_QUARTIC_M) cout << "Twist type M" << endl;
		if (mip->TWIST==MR_QUARTIC_D) cout << "Twist type D" << endl;

		//cout << "Q= " << Q << endl;
		Q*=q;
		cout << "check - if right twist should be O - q*Q= " << Q << endl;	
		if (ham(x)<best_ham) {best_ham=ham(x);best_s=s;}	
		cout << "So far minimum hamming weight of x= " << best_ham << endl;
		cout << "for seed= " << best_s << endl << endl;
		ns++;
	}
	return 0;
}
