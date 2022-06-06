//
// cl /O2 /GX kss18.cpp zzn3.cpp zzn.cpp ecn3.cpp ecn.cpp big.cpp miracl.lib
// Program to generate KSS k=18 curves.
//

#include <iostream>
#include "ecn3.h"
#include "ecn.h"

Miracl precision(50,0);

int main()
{
	miracl *mip=&precision;
	Big s,x,q,p,t,A,B,cf,X,Y,sru,n,best_s,f;
	Big T,P,F,m1,m2;
	ECn W;
	ECn3 Q;
	ZZn3 r;
	mip->IOBASE=16;
	int i,ns,sign,best_ham=1000;

    sign=1;  // 1= positive, 2=negative for +/- s solutions
	//s="808000000000000";
	s=(char *)"800000000000000";

	ns=1;
	for (i=0;i<100;i++)
	{
		forever
		{
			forever
			{
				sign=3-sign; 
				if (sign==1) s+=1;
				if (sign==1) x=14+42*s;
				else         x=14-42*s;

				t=(pow(x,4) + 16*x + 7)/7;
//	p=(pow(x,8) + 5*pow(x,7) + 7*pow(x,6) + 37*pow(x,5) + 188*pow(x,4) + 259*pow(x,3) + 343*pow(x,2) + 1763*x + 2401)/21;
				q=(pow(x,6) + 37*pow(x,3) + 343)/343;
				if (!prime(q)) continue;
				cf=(49*x*x+245*x+343)/3;
				n=cf*q;
				p=cf*q+t-1;  // avoids overflow..

				if (p%8!=5) continue;
				
				
				if (!prime(p)) continue;
				modulo(p);
				if (pow((ZZn)-2,(p-1)/3)==1) continue;
		
				break;
			}

			T=t*t*t-3*p*t;
			P=p*p*p;
			F=(4*P-T*T)/3;

			F=sqrt(F);

			m1=P+1-(-3*F+T)/2;  // Wrong Curve
 
			m2=P+1-(3*F+T)/2;

			B=1;
	
			forever
			{
				B+=1;
				if (B==2)
				{
					X=-1; Y=1;
				}
				else if (B==3)
				{
					X=1; Y=2;
				}
				else if (B==8)
				{
					X=1; Y=3;
				}
				else if (B==15)
				{
					X=1; Y=4;
				}
				else
				{
					do
					{
						X=rand(p);
						Y=sqrt(X*X*X+B,p);
					} while (Y==0);
				}
        
				ecurve(0,B,p,MR_AFFINE);
	

				W.set(X,Y);
				W*=cf;
				if ((q*W).iszero()) break;
			}
	
/* This always works! 
			B=2;
			ecurve(0,B,p,MR_AFFINE);
			W.set(-1,1);
			W*=cf;
*/
			sru=pow((ZZn)-2,(p-1)/6);   // x^6+2 is irreducible
			set_zzn3(-2,sru);

			mip->TWIST=MR_SEXTIC_M;
			do
			{
				r=randn3();
			} while (!Q.set(r)); 
  
			Q*=(m2/q);

			if ((q*Q).iszero()) break;

//			cout << "m1*Q= " << m1*Q << endl;
//			cout << "m1%q= " << m1%q << endl;

			mip->TWIST=MR_SEXTIC_D;
			do
			{
				r=randn3();
			} while (!Q.set(r)); 
  
			Q*=(m2/q);

			if ((q*Q).iszero()) break;	
			
			cout << "Something wrong" << endl;
			exit(0);
		}
        cout << "solution " << ns << endl;
		cout << "X^18+2 is irreducible" << endl;
		
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

		cout << "W= " << W << endl;
		cout << "q*W= " << q*W << endl;
		mip->IOBASE=10;
		cout << "E(Fp): y^2=x^3+" << B << endl;
		cout << "(p-1)%18= " << (p-1)%18 << endl;
		mip->IOBASE=16;
		if (mip->TWIST==MR_SEXTIC_M) cout << "Twist type M" << endl;
		if (mip->TWIST==MR_SEXTIC_D) cout << "Twist type D" << endl;

		m2/=q;
	//	cout << "cf0= " << p-m2%p << endl; m2/=p;
	//	cout << "cf1= " << m2%p << endl; m2/=p;
	//	cout << "cf2= " << m2 << endl;

		cout << "Q= " << Q << endl;
		Q*=q;
		cout << "check - if right twist should be O - q*Q= " << Q << endl;	
		if (ham(x)<best_ham) {best_ham=ham(x);best_s=s;}	
		cout << "So far minimum hamming weight of x= " << best_ham << endl;
		cout << "for seed= " << best_s << endl << endl;
		ns++;
		
	}
	return 0;
}
