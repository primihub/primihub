//
// cl /O2 /GX bls24.cpp zzn4.cpp zzn2.cpp zzn.cpp ecn4.cpp ecn.cpp big.cpp miracl.lib
// Program to generate BLS k=24 pairing friendly curves.
//

#include <iostream>
#include "ecn4.h"
#include "ecn.h"
#include "zzn.h"

Miracl precision(200,0);

int main()
{
	miracl *mip=&precision;
	Big s,x,q,p,t,A,B,cf,X,Y,sru,n,best_s,f,tau[5],TAU;
	Big T,P,F,m1,m2,m3,m4;
	BOOL got_one;
	ECn W;
	ECn4 Q;
	ZZn4 XX,YY,r;	
	ZZn2 xi;
	int i,ns,sign,best_ham=1000;
	mip->IOBASE=16;
	s="E000000000000000";   

	ns=1;
	
	forever
	{
		s+=1;
		for (sign=1;sign<=2;sign++)
		{

			if (sign==1) x=s;
			else         x=-s;

			if (x<0 || ham(x)>7) continue; // filter out difficult or poor solutions

			t=1+x;
			p=1+x+x*x-pow(x,4)+2*pow(x,5)-pow(x,6)+pow(x,8)-2*pow(x,9)+pow(x,10);
			q=1-pow(x,4)+pow(x,8);
	
			if (p%3!=0) continue;
			p/=3;
			
			if (p%8==1) continue;
			if (!prime(p)) continue;
			if (!prime(q)) continue;

			modulo(p);
			if (p%8==5) xi.set(0,1);
			if (p%8==3) xi.set(1,1);
			if (p%8==7) xi.set(2,1);

// make sure its irreducible
			if (pow(xi,(p*p-1)/2)==1) {/*cout << "Failed - not a square" << endl; */ continue;}
			if (pow(xi,(p*p-1)/3)==1) {/*cout << "Failed - not a cube" << endl; */ continue;}  // make sure that x^6-c is irreducible

			n=p+1-t;
			cf=n/q;

			tau[0]=2;  // count points on twist over extension p^4
			tau[1]=t;
			for (i=1;i<4;i++ ) tau[i+1]=t*tau[i]-p*tau[i-1];
			P=p*p*p*p;
			TAU=tau[4];

			F=(4*P-TAU*TAU)/3;
			F=sqrt(F);
		
			m2=P+1-(3*F+TAU)/2;

		//	cout << "m2%q= " << m2%q << endl;

			B=1;   // find curve equation
	
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

			mip->TWIST=MR_SEXTIC_M;  // is it an M-type twist...?
			do
			{
				r=randn4();
			} while (!Q.set(r)); 
			got_one=FALSE;

			Q*=(m2/q);

			if ((q*Q).iszero()) got_one=TRUE;

//			cout << "m1*Q= " << m1*Q << endl;
//			cout << "m1%q= " << m1%q << endl;
			else
			{
				mip->TWIST=MR_SEXTIC_D;  // no, so it must be D-type.
				do
				{
					r=randn4();
				} while (!Q.set(r)); 
			
				Q*=(m2/q);

				if ((q*Q).iszero()) got_one=TRUE;
			}
			if (!got_one) {cout << "Bad twist" << endl; exit(0);}  // Huh?
	
			if (mip->TWIST==MR_SEXTIC_M) continue;  // not interested just now

			cout << "solution " << ns << endl;
		
			cout << "x= " << x << " ham(x)= " << ham(x) << endl;
			cout << "x%72= " << x%72 << endl;
			cout << "p= " << p << " bits(p)= " << bits(p) << endl;
			cout << "q= " << q << " bits(q)= " << bits(q) << endl;
			cout << "n= " << n << endl;
			cout << "t= " << t << endl;
			cout << "cf= " << cf << endl;
			

			cout << "W= " << W << endl;
			cout << "q*W= " << q*W << endl;
			mip->IOBASE=10;
			cout << "E(Fp): y^2=x^3+" << B << endl;
			cout << "(p-1)%24= " << (p-1)%24 << endl;
			cout << "p%8= " << p%8 << endl;
			mip->IOBASE=16;
			if (mip->TWIST==MR_SEXTIC_M) cout << "Twist type M" << endl;
			if (mip->TWIST==MR_SEXTIC_D) cout << "Twist type D" << endl;

			Q*=q;
			cout << "check - if right twist should be O - q*Q= " << Q << endl;	
						
			if (ham(x)<best_ham) {best_ham=ham(x);best_s=s;}	
			cout << "So far minimum hamming weight of x= " << best_ham << endl;
			cout << "for seed= " << best_s << endl;
						
			cout << endl;
			ns++;
		}
	}

	return 0;
}
