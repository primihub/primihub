//
// cl /O2 /GX bn.cpp zzn2.cpp zzn.cpp ecn2.cpp ecn.cpp big.cpp miracl.lib
// Program to generate BN curves for use by ake12*.cpp
//

#include <iostream>
#include "big.h"
#include "ecn.h"
#include "ecn2.h"
//#include "zzn12.h"

using namespace std;

Miracl precision=100;

int main()
{
    int i,ns;
    int sign;
    BOOL ontwist;
    Big m1,m2,n,p,t,x,cube,y,b,eta,w,cf[4],X,Y;
    Big PP,TT,FF;
    miracl*mip=&precision;
    ECn P;
    ECn2 Q,T;
    ZZn2 xi,r;

    x=pow((Big)2,62)+pow((Big)2,55)-1;  // x is this size to get right size for t, p and n
                                        // x is low hamming weight
    mip->IOBASE=16;
    sign=1;  // 1= positive, 2=negative for +/- x solutions
    ns=1;
    for (i=0;i<100;i++)
    {
        forever
        {
			forever
			{
				sign=3-sign;    // always looking for +ve x solutions.
				if (sign==1) x+=1;
         
				if (sign==1) p=36*pow(x,4)+36*pow(x,3)+24*x*x+6*x+1;
				else         p=36*pow(x,4)-36*pow(x,3)+24*x*x-6*x+1;
 //cout << "x= " << x << " p%8= " << p%8 << " p%6= " << p%6 << " p%9= " << p%9 << endl;
 //if (prime(p)) cout << "p is prime" << endl;
 
 // Now check congruence conditions
				if (p%8==1) continue;
 //           if (p%9==1) continue;
            
				if (p%8==7 && (p%5==1 || p%5==4)) continue;
              
				if (!prime(p)) continue;

				modulo(p);
				if (p%8==5) xi.set(0,1);
				if (p%8==3) xi.set(1,1);
				if (p%8==7) xi.set(2,1);

// make sure its irreducible
				if (pow(xi,(p*p-1)/2)==1) {/*cout << "Failed - not a square" << endl; */ continue;}
				if (pow(xi,(p*p-1)/3)==1) {/*cout << "Failed - not a cube" << endl;*/  continue;}  // make sure that x^6-c is irreducible

				t=6*x*x+1;
				n=p+1-t;
				if (prime(n)) break;
			}     
        
			cf[3]=1;
			cf[2]=6*x*x+1;
			cf[1]=36*x*x*x-18*x*x+12*x+1;
			cf[0]=36*x*x*x-30*x*x+18*x-2;

// find number of points on sextic twist..

			TT=t*t-2*p;
			PP=p*p;

			FF=(4*PP-TT*TT)/3;
			FF=sqrt(FF);

    //    m1=PP+1-(-3*FF+TT)/2;  // 2 possibilities... This is the wrong curve.
			m2=PP+1-(3*FF+TT)/2;

			b=1;
			forever
			{
				b+=1;
				if (b==2)
				{
					X=-1; Y=1;
				}
				else if (b==3)
				{
					X=1; Y=2;
				}
				else if (b==8)
				{
					X=1; Y=3;
				}
				else if (b==15)
				{
					X=1; Y=4;
				}
				else
				{
					do
					{
						X=rand(p);
						Y=sqrt(X*X*X+b,p);
					} while (Y==0);
				}

				ecurve(0,b,p,MR_AFFINE);

				P.set(X,Y);
				if ((n*P).iszero()) break;
			}

			mip->TWIST=MR_SEXTIC_M;

			do
			{
				r=randn2();
			} while (!Q.set(r));
        
			Q*=m2/n;
			if ((n*Q).iszero()) break;
			
			mip->TWIST=MR_SEXTIC_D;

			do
			{
				r=randn2();
			} while (!Q.set(r));
        
			Q*=m2/n;
			if ((n*Q).iszero()) break;

			cout << "Something Wrong" << endl;
			exit(0);
		}
// Note that this program only produces BN curves for which y^2=x^3+B/D yields the correct twist
// It would be possible to modify to find curves on the other twist y^2=x^3+B*D- but a lot of changes required!


        cout << "solution " << ns << endl;
        cout << "irreducible polynomial = X^6 - " << xi << endl;
        
        if (sign==1)
		{
			cout << "x mod 12 = " << x%12 << endl;
			cout << "x= +" << x << endl;
		}
        else
		{
			cout << "x mod 12 = " << 12-(x%12) << endl;
			cout << "x= -" << x << endl;
		}
		cout << "p=" << p << endl;
        cout << "p mod 72 = " << p%72 << endl;

        cout << "bits(p)= " << bits(p) << endl;
        cout << "n=" << n << endl;
        cout << "t=" << t << endl;
 
 		if (x>0)
			cout << "ham(6*x+2)= " << ham(6*x+2) << " (small is better for R-ate pairing)" << endl;
		else
			cout << "ham(6*x+2)= " << ham(-(6*x+2)) << " (small is better for R-ate pairing)" << endl;

        cout << "bits(t)= " << bits(t) << endl;
        cout << "ham(t-1) = " << ham(t-1) << " (small is better for Ate pairing)" << endl;
       
        cout << "bits(n)= " << bits(n) << endl;
        cout << "ham(n-1) = " << ham(n-1) << " (small is better for Tate pairing)" << endl;
		mip->IOBASE=10;
        cout << "E(Fp): y^2=x^3+" << b << endl;
		mip->IOBASE=16;
		if (mip->TWIST==MR_SEXTIC_M) cout << "Twist type M" << endl;
		if (mip->TWIST==MR_SEXTIC_D) cout << "Twist type D" << endl;

        cout << "Point P= " << P << endl;
        cout << "Point Q= " << Q << endl;
        T=n*Q;
        cout << "check - if right twist should be O - n*Q= " << T << endl << endl;
        ns++;

    }
    return 0;
}

