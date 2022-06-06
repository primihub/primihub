/*
 * program to generate MNT Elliptic curves
 *
 * Compile as (for example)
 * cl /O2 /GX mnt.cpp big.cpp miracl.lib
 *
 * See Miyaji, Nakabayashi & Takano "New explicit conditions of elliptic curve 
 * traces for FR-reduction", IEICE Trans. Fundementals., Vol. E84A, No. 5,May 
 * 2001
 *
 * Invoke as mnt N where k= 3,4 or 6
 *
 * Security level is preportional to the difficulty of the associated 
 * discrete log problem in bits. e.g. if p is 160 bits and k=6, then 
 * discrete log problem is 160*6=960 bits.
 *
 * To find actual elliptic curve use CM utility
 * For example from output:-
 *
 * *** Found one - 165 bits
 * D= 998
 * p= 28795013727143986241793993458326894706197949151527
 * NP= 28795013727143986241793989663922216291941998151802
 *   = 78*369166842655692131305051149537464311435153822459
 *  (a 159-bit prime)
 *
 * Execute
 * cm 28795013727143986241793993458326894706197949151527 -D998 -K78
 *
 * New! Now also finds "extended" MNT curves. By allowing a co-factor > 1
 * many more curves will be found.
 *
 * However this can cause a problem for our Pell code. A "." indicates that 
 * only a partial search was possible.
 *
 * Mike Scott 4/07/2002
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

void results(BOOL fout,ofstream& ofile,int d,Big p,Big nrp,Big ord,BOOL prime_ord)
{
    Big cf=nrp/ord;

    cout << "*** Found one - " << bits(p) << " bits" << endl;
    cout << " D= " << d << endl;
    cout << " p= " << p << " p%24= " << p%24 << endl;
    cout << "NP= " << nrp << endl;
    if (cf>1) cout << "  = " << cf << "*" << ord << endl;
    if (prime_ord) 
    {
        if (cf==1)
            cout << "    (a " << bits(ord) << "-bit prime) !\n" << endl;
        else
            cout << "    (a " << bits(ord) << "-bit prime)\n" << endl;

    }
    else cout << endl;
     
    if (fout)
    {
        ofile << "*** Found one - " << bits(p) << " bits" << endl;
        ofile << " D= " << d << endl;
        ofile << " p= " << p << " p%24= " << p%24 << endl;
        ofile << "NP= " << nrp << endl;
        if (cf>1) ofile << "  = " << cf << "*" << ord << endl;
        if (prime_ord) 
        {
            if (cf==1)
                ofile << "    (a " << bits(ord) << "-bit prime) !" << endl;
            else 
                ofile << "    (a " << bits(ord) << "-bit prime)" << endl;
            ofile << " cm " << p << " -D" << d << " -K" << cf << endl << endl; 
        }
        else ofile << endl;
    }
}

int main(int argc,char **argv)
{
    ofstream ofile;
    int MIN_SECURITY,MAX_SECURITY,MIN_D,MAX_D,MIN_P;
    int ip,j,d,m,mm,solutions,mnt,start,N,precision,max;
    BOOL fout,complete;

// Defaults
    MIN_SECURITY=768;
    MAX_SECURITY=1536;
    MIN_D=1;
    MAX_D=10000;
    MIN_P=128;
    fout=FALSE;
    precision=100;
    m=1;

    argc--; argv++;
    if (argc<1)
    {
        cout << "Wrong number of parameters" << endl;
        cout << "Utility to find MNT elliptic curves" << endl; 
        cout << "The output from this program must be processed by the" << endl;
        cout << "CM (Complex Multiplication) utility to find the actual" << endl;
        cout << "curve parameters" << endl;
        cout << "See comments at start of source file mnt.cpp for more details" << endl;
        cout << "mnt k, where k=3,4 or 6 is the security multiplier" << endl;
        cout << "Flags allowed, e.g. mnt 6 -c2 -d100 -D1000 -m1000 -M2000" << endl;
        cout << "-c: cofactor (number of points on curve will be multiple of cofactor)" << endl;
        cout << "-d: min value of D, -D: max value of D" << endl;
        cout << "-m: min security level, -M: max security level (RSA equivalent bits)" << endl; 
        cout << "-p: min prime size/field size in bits" << endl;
        cout << "Increase co-factor for more curves" << endl;
        cout << "Defaults -c1 -d0 -D10000 -m768 -M1536 -p128" << endl;
        cout << "To output to a file, use flag -o <filename>" << endl;
        exit(0);
    }

    N=atoi(argv[0]);
    if (N!=3 && N!=4 && N!=6)
    {
        cout << "Wrong parameter - should be 3,4 or 6" << endl;
        exit(0);
    }

    ip=1;
    while (ip<argc)
    { // look for flags
        if (strncmp(argv[ip],"-d",2)==0)
        {
            MIN_D=atoi(argv[ip]+2);
            ip++;
            continue;
        }
        if (strncmp(argv[ip],"-D",2)==0)
        {
            MAX_D=atoi(argv[ip]+2);
            ip++;
            continue;
 
        }
        if (strncmp(argv[ip],"-m",2)==0)
        {
            MIN_SECURITY=atoi(argv[ip]+2);
            ip++;
            continue;
        }
        if (strncmp(argv[ip],"-M",2)==0)
        {
            MAX_SECURITY=atoi(argv[ip]+2);
            ip++;
            continue;
        }
        if (strncmp(argv[ip],"-p",2)==0)
        {
            MIN_P=atoi(argv[ip]+2);
            ip++;
            continue;
        }
        if (strncmp(argv[ip],"-c",2)==0)
        {
            m=atoi(argv[ip]+2);
            ip++;
            continue;
        }
        if (strcmp(argv[ip],"-o")==0)
        {
            ip++;
            if (ip<argc)
            {
                fout=TRUE;
                ofile.open(argv[ip++]);
                continue;
            }
            else
            {
                cout << "Error in command line" << endl;
                return 0;
            }

        }
        cout << "Error in command line" << endl;
        return 0;
    }

    if (m<1 || m>8)
    {
        cout << "Co-factor must be > 0 and <= 8" << endl;
        exit(0);
    }

    miracl *mip=mirsys(precision,0);
    Big F,T,M,D,W,K,C,mmax,td,X,Y,x,y,w,xn,yn,t0,u0,p,k,nrp,ord,R;
    Big SX[20],SY[20];

    start=1;
    if (1<MIN_D) start=MIN_D;
    mmax=m;
    mnt=0;
    for (M=1;M<=mmax;M+=1)
    {
        for (R=1;;R+=1)
        {
            if (N==4)
            { // R=1,2,5,... 1 or 2 mod 4
                if (R%4!=1 && R%4!=2) continue;
                if (R%3==0) continue;   
                if (R>=4*M) break;     // Hasse limit
                if (gcd(R,M)!=1) continue;
                T=R*R-4*M*M;
            }
            else
            { // R=1,3,7,9,13,...  1 or 3 mod 6
                if (R%6!=1 && R%6!=3) continue;
                if (R%5==0 || R%2==0) continue;
                if (R>=4*M) break;     // Hasse limit
                if (gcd(R,M)!=1) continue;
                if (N==3) T=(R+M)*(R+M)-4*M*M;
                else      T=(R-M)*(R-M)-4*M*M;
            }

// Check that quadratic polynomial for p does not factor
// i.e.  b^2-4ac is not a square
// The prime p will be of the form M*Phi_N(x)/R + x

            if (T>=0)
            {   
                W=sqrt(T);
                if (W*W==T) continue;  // p cannot be prime - has factors 
            }

// There are probably more conditions like these .....

            if (N==6)
            {
                if (M==4 && R==3) continue; // p cannot be prime - multiple of 3
                if (R>3 && (R/3)%3==0) continue;
            } 

            cout << "\nM= " << M << " R= " << R << endl << endl;
            if (fout) ofile << "\nM= " << M << " R= " << R << endl << endl;

            K=(4*M-R);

            if (N==3) C=2*M+R;
            if (N==4) C=R;
            if (N==6) C=-(2*M-R);

            F=C*C-K*K;

// CM equation is R.D.V^2 = 4*M*Phi_N(x)-R*(x-1)^2
// Substitute x=(X-C)/K (to get rid of x term)
// Pell equation is then X^2-R*K*D.V^2 = F

// Our Pell code runs into problems if F is too big 

// This hack makes F a bit smaller - better for Pell
// If M is even x must be odd...

            mm=1;
            if (M%2==0 && F%4==0)
            {
                mm=2; F/=4;
            }

            max=10+MAX_SECURITY/(2*N);
            gprime(100000);            // > 2^16
          

            for (d=start;d<=MAX_D;d++)
            {
// D must be square-free
                if (!squfree(d)) continue;
                td=R*K*d;
                solutions=pell(max,td,F,t0,u0,SX,SY,complete); 

                if (!solutions) continue;
                for (j=0;j<solutions;j++)
                {
                    X=SX[j];
                    Y=SY[j];

                    forever
                    {
                        if (bits(mm*X) > max) break;   // primes are too big
               
                        if ((mm*X-C)%K == 0)
                        {
                            x=(mm*X-C)/K;
                            if (N==3) p=x*x+x+1;
                            if (N==4) p=x*x+1;
                            if (N==6) p=x*x-x+1;
                            if (R==1 || p%R==0)
                            { 
                                p=(M*p)/R+x;
                                if (N*bits(p) > MAX_SECURITY) break;
                                if (bits(p)>=MIN_P && N*bits(p)>= MIN_SECURITY && prime(p)) 
                                {
                                    nrp=p+1-(x+1);
                                    ord=trial_divide(nrp);
                                    if (prime(ord)) 
                                    {   
                                        if (bits(ord)>=MIN_P) 
                                        {
                                            mnt++;  
                                            results(fout,ofile,d,p,nrp,ord,TRUE);
                                        }
                                    }
                                //    else
                                //    { // ord has at least a 16-bit factor...
                                //        if (bits(ord)>=MIN_P+16) 
                                //        {
                                //            mnt++;
                                //            results(fout,ofile,d,p,nrp,ord,FALSE);
                                //        }
                                //    }
                                }
                            }
                        }
                        if ((-mm*X-C)%K == 0)
                        {
                            x=(-mm*X-C)/K;
                            if (N==3) p=x*x+x+1;
                            if (N==4) p=x*x+1;
                            if (N==6) p=x*x-x+1;
                            if (R==1 || p%R==0)
                            { 
                                p=(M*p)/R+x;
                                if (N*bits(p) > MAX_SECURITY) break;
                                if (bits(p)>=MIN_P && N*bits(p)>= MIN_SECURITY && prime(p)) 
                                {
                                    nrp=p+1-(x+1);
                                    ord=trial_divide(nrp);
                                    if (prime(ord)) 
                                    {   
                                        if (bits(ord)>=MIN_P) 
                                        {
                                            results(fout,ofile,d,p,nrp,ord,TRUE);
                                            mnt++;
                                        }
                                    }
                                 //   else
                                 //   {
                                 //       if (bits(ord)>=MIN_P+16) 
                                 //       {
                                 //           results(fout,ofile,d,p,nrp,ord,FALSE);
                                 //           mnt++;
                                 //       }
                                 //   }
                                }
                            }
                        }
                        if (!complete) break;  // no more solutions
                        multiply(td,t0,u0,X,Y);
                    }
                }
            }
        }
    }
    if (fout) ofile << mnt << " MNT elliptic curves found in this range" << endl;
    cout << mnt << " MNT elliptic curves found in this range" << endl;
    return 0;
}

