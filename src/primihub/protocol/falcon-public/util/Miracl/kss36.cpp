//
// cl /O2 /GX kss36.cpp zzn6.cpp zzn3.cpp zzn.cpp ecn6.cpp ecn.cpp big.cpp miracl.lib
// Program to generate KSS k=36 curves.
//

#include <iostream>
#include "ecn6.h"
#include "ecn.h"
#include "zzn.h"

Miracl precision(200,0);

int rc[]={287,308,497,539,728,749,0};  // allowed residue classes

int main()
{
	miracl *mip=&precision;
	Big s,x,q,p,t,A,B,cf,X,Y,sru,n,best_s,f;
	Big T,P,F,m1,m2;
	BOOL got_one;
	ECn W;
	ECn6 Q;
	ZZn6 r;
	mip->IOBASE=16;
	int i,j,ns,sign,best_ham=1000;

	s="2000000000";
	
	ns=1;
	//for (i=0;i<100;i++)
	//{
		forever
		{
			forever
			{
				s+=1;

				for (j=0;j<6;j++)
				{
					for (sign=1;sign<=2;sign++)
					{
		
						if (sign==1) x=rc[j]+777*s;
						else         x=rc[j]-777*s;


						t=(2*pow(x,7) + 757*x + 259)/259;
						p=(pow(x,14)-4*pow(x,13)+7*pow(x,12)+683*pow(x,8)-2510*pow(x,7)+4781*pow(x,6)+117649*pow(x,2)-386569*x + 823543)/28749;
						q=(pow(x,12) + 683*pow(x,6) + 117649)/161061481;
				
						if (x<0) continue;
						if (p%8!=5) continue;
				
						if (!prime(q)) continue;
						if (!prime(p)) continue;
						modulo(p);
						if (pow((ZZn)-2,(p-1)/3)==1) continue;
						

						n=p+1-t;
						cf=n/q;

// find number of points on sextic twist E(p^6)		

						T=t*t*t*t*t*t-6*p*t*t*t*t+9*p*p*t*t-2*p*p*p;  // t^6-6p*t^4+9p^2.t^2-2p^3

						P=p*p*p*p*p*p;
						F=(4*P-T*T)/3;
						
						F=sqrt(F);

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

						sru=pow((ZZn)-2,(p-1)/6);   // x^6+2 is irreducible
						set_zzn3(-2,sru);

						mip->TWIST=MR_SEXTIC_M;
						do
						{
							r=randn6();
						} while (!Q.set(r)); 
						got_one=FALSE;

						Q*=(m2/q);

						if ((q*Q).iszero()) got_one=TRUE;

//			cout << "m1*Q= " << m1*Q << endl;
//			cout << "m1%q= " << m1%q << endl;
						else
						{
							mip->TWIST=MR_SEXTIC_D;
							do
							{
								r=randn6();
							} while (!Q.set(r)); 
  
							Q*=(m2/q);

							if ((q*Q).iszero()) got_one=TRUE;
						}
						if (!got_one)
						{
							cout << "Something wrong" << endl;
							exit(0);
						}
						if (mip->TWIST==MR_SEXTIC_M) continue;

						cout << "solution " << ns << endl;
						cout << "X^36+2 is irreducible" << endl;
		
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
						cout << "residue class= 777x+" << rc[j] << endl;

						cout << "W= " << W << endl;
						cout << "q*W= " << q*W << endl;
						mip->IOBASE=10;
						cout << "E(Fp): y^2=x^3+" << B << endl;
						cout << "(p-1)%36= " << (p-1)%36 << endl;
						mip->IOBASE=16;
						if (mip->TWIST==MR_SEXTIC_M) cout << "Twist type M" << endl;
						if (mip->TWIST==MR_SEXTIC_D) cout << "Twist type D" << endl;

						m2/=q;
	//	cout << "cf0= " << p-m2%p << endl; m2/=p;
	//	cout << "cf1= " << m2%p << endl; m2/=p;
	//	cout << "cf2= " << m2 << endl;

	//					cout << "Q= " << Q << endl;
						Q*=q;
						cout << "check - if right twist should be O - q*Q= " << Q << endl;	
						
						if (ham(x)<best_ham) {best_ham=ham(x);best_s=s;}	
						cout << "So far minimum hamming weight of x= " << best_ham << endl;
						cout << "for seed= " << best_s << endl;
						
						cout << endl;
						ns++;

					}
				}

			}	
	}

	return 0;
}
