/*
 * Program to find Freeman pairing-friendly elliptic curves with embedding degree k=10
 *
 * See http://www.hpl.hp.com/techreports/2005/HPL-2005-155.html
 *
 * Compile as (for example)
 * cl /O2 /GX freeman.cpp big.cpp miracl.lib
 *
 * Outputs curves to a file freeman.dat
 *
 * Mike Scott 6/10/2005
 *
 */

#include <cstring>
#include <iostream>
#include <fstream>
#include <iomanip>
#include "big.h"

using namespace std;

// Solve the Pell equation

int pell(int max,Big D,Big F,Big& X,Big& Y,Big *SX,Big *SY,BOOL& complete)
{
    int i,j,ns;
    BOOL SMALLD;
    Big A,P,Q,SD,G,B,OG,OB,NG,NB,T;

    complete=FALSE;
    SMALLD=FALSE;
    if (D<F*F) SMALLD=TRUE;

    SD=sqrt(D);

    if (SD*SD==D) return 0;
    P=0; Q=1;
    OG=-P;
    OB=1;
    G=Q;
    B=0;
    ns=0;
    X=Y=0;
       
    for (i=1;;i++)
    {
        A=(P+SD)/Q;
        P=A*Q-P;
        Q=(D-P*P)/Q;
        NG=A*G+OG;       // G is getting bigger all the time....
        NB=A*B+OB;
        OG=G; OB=B;
        G=NG; B=NB;
        if (!SMALLD && bits(G)>max) return ns; 
                                    // abort - these are only solutions
        T=G*G-D*B*B;
        if (T == F/4) 
        {
            SX[ns]=2*G; SY[ns]=2*B;
            ns++;
        }
            
        if (T == F)
        {
            SX[ns]=G; SY[ns]=B;
            ns++;
        }
        if (i>0 && Q==1) break;
    }

    if (i%2==1)
    for (j=0;j<i;j++)
    {
        A=(P+SD)/Q;
        P=A*Q-P;
        Q=(D-P*P)/Q;
        NG=A*G+OG;
        NB=A*B+OB;
        OG=G; OB=B;
        G=NG; B=NB;
        if (!SMALLD && bits(G)>max) return ns;

        T=G*G-D*B*B;
        if (T == F/4) 
        {
            SX[ns]=2*G; SY[ns]=2*B;
            ns++;
        }
        if (T == F) 
        {
            SX[ns]=G; SY[ns]=B;
            ns++;
        }
    }

    complete=TRUE;   // we got to the end....
    X=G; Y=B;        // minimal solution of x^2-dy^2=1
                     // can be used to find more solutions....

    if (SMALLD)
    { // too small - do it the hard way
        Big ylim1,ylim2,R;
        ns=0;
        if (F<0)
        {
            ylim1=sqrt(-F/D);
            ylim2=sqrt(-F*(X+1)/(2*D));
        }
        else
        {
            ylim1=0;
            ylim2=sqrt(F*(X-1)/(2*D));
        }

// This might take too long...
// Should really implement LMM method here

        if (ylim2-ylim1>300) 
        {
            cout << "." << flush;  
            ylim2=ylim1+300;
        }
        for (B=ylim1;B<ylim2;B+=1)
        {
            R=F+D*B*B;
            if (R<0) continue;
            G=sqrt(R);
            if (G*G==R)
            {
                SX[ns]=G; SY[ns]=B;
                ns++;
            }
        }
    }

    return ns;
}

void multiply(Big& td,Big& a,Big& b,Big& c,Big& d)
{ // (c+td.d) = (a+td.b)(c+td.d)
    Big t;
    t=a*c+b*d*td;
    d=c*b+a*d;
    c=t;
}

BOOL squfree(int d)
{ // check if d is square-free
    int i,s;
    miracl *mip=get_mip();

    for (i=0;;i++)
    {
        s=mip->PRIMES[i];
        if ((d%(s*s))==0) return FALSE;
        if ((s*s)>d) break;
    }

    return TRUE;
}

void results(BOOL fout,ofstream& ofile,int d,Big p,Big cf,Big ord)
{
    cout << "*** Found one - p=" << bits(p) << "-bits ord=" << bits(ord) << "-bits" << endl;
    cout << "D= " << d << endl;
    cout << "p= " << p << endl;
    cout << "ord= " << ord << endl;
    cout << "cf= " << cf << endl << endl;
     
    if (fout)
    {
        ofile << "*** Found one: p=" << bits(p) << "-bits ord=" << bits(ord) << "-bits p%8= " << p%8 << endl;
        ofile << "D= " << d << endl;
        ofile << "p= " << p << endl;
        ofile << "ord= " << ord << endl;
        ofile << "cf= " << cf << endl;
		ofile << "cm " << p << " -D" << d << " -K" << cf << endl << endl; 
    }
}

int main(int argc,char **argv)
{
    ofstream ofile;
    int MIN_SECURITY,MAX_SECURITY,MIN_D,MAX_D,MIN_P;
    int ip,j,d,d15,m,mm,solutions,mnt,start,N,precision,max;
    BOOL fout,complete;

// Defaults

    fout=TRUE;
    precision=100;
	ofile.open("freeman.dat");

    miracl *mip=mirsys(precision,0);
    Big F,T,M,D,W,K,C,mmax,td,X,Y,x,y,w,xn,yn,t0,u0,p,k,nrp,ord,R;
    Big SX[20],SY[20];
            
	start=1;
	max=276;   // 10+5120/20
	gprime(100000);
	MAX_D=2000000000; // as far as I can reasonably go....

    for (d=start;d<=MAX_D;d++)
    {
// D must be square-free
		if (d%120!=43 && d%120!=67) continue;
	  
        if (!squfree(d)) continue;

        td=(Big)d*15;
		F=-20;

        solutions=pell(max,td,F,t0,u0,SX,SY,complete); 
		if (!solutions) continue;
      
        for (j=0;j<solutions;j++)
        {
            X=SX[j];
            Y=SY[j];
					
			forever
			{
				if (4*bits(X)>512) break;
				if ((X-5)%15==0)
				{
					x=(X-5)/15;
					p=25*x*x*x*x+25*x*x*x+25*x*x+10*x+3;
					if (bits(p)>148)
					{				
						if (prime(p)) 
						{		
							nrp=25*x*x*x*x+25*x*x*x+15*x*x+5*x+1;
							ord=trial_divide(nrp);
							if (prime(ord) && bits(ord)>148) 
								results(fout,ofile,d,p,nrp/ord,ord);
						}
					}
				}
						
				if ((-X-5)%15==0)
				{
					x=(-X-5)/15;
					p=25*x*x*x*x+25*x*x*x+25*x*x+10*x+3;
					if (bits(p)>148)
					{	
						if (prime(p)) 
						{
							nrp=25*x*x*x*x+25*x*x*x+15*x*x+5*x+1;
							ord=trial_divide(nrp);
							if (prime(ord) && bits(ord)>148) 
								results(fout,ofile,d,p,nrp/ord,ord);
						}
					}
				} 
				if (!complete) break;  // no more solutions
				multiply(td,t0,u0,X,Y);
			}
		}
	}		
	return 0;
}

