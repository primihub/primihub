/*
 *   Program to factor big numbers using Pomerance-Silverman-Montgomery  
 *   multiple polynomial quadratic sieve.
 *   See "The Multiple Polynomial Quadratic Sieve", R.D. Silverman,
 *   Math. Comp. Vol. 48, 177, Jan. 1987, pp329-339
 *
 *   Requires: big.cpp
 */

#include <iostream>
#include <iomanip>
#include <cmath>
#include "big.h"

using namespace std;

#define SSIZE 1000000     /* Maximum sieve size            */

Miracl precision=(30);          /* number of bytes per big */

static int pk[]={0,1,2,3,5,6,7,10,11,13,14,15,17,0};
static Big NN,DD,DG,TT,RR,VV,PP,IG,AA,BB;
static Big *x,*y,*z,*w;
static int *epr,*r1,*r2,*rp,*pr;
static int *b,*e,*hash;
static unsigned int **EE,**G;
static unsigned char *logp,*sieve;
static int mm,mlf,jj,nbts,nlp,lp,hmod,hmod2;
static BOOL partial;
static miracl *mip;

int knuth(int mm,int *epr,Big& N,Big& D)
{ /* Input number to be factored N, find best multiplier k  *
   * set D=k.N */
    Big T;
    double fks,dp,top;
    BOOL found;
    int i,j,bk,nk,kk,rem,p;

    top=(-10.0e0);
    found=FALSE;
    nk=0;
    bk=0;
    epr[0]=1;
    epr[1]=2;

    do
    { /* search for best Knuth-Schroepel multiplier */
        kk=pk[++nk];
        if (kk==0)
        { /* finished */
            kk=pk[bk];
            found=TRUE;
        }
        D=kk*N;
        fks=log(2.0e0)/(2.0e0);
        rem=D%8;
        if (rem==1) fks*=(4.0e0);
        if (rem==5) fks*=(2.0e0);
        fks-=log((double)kk)/(2.0e0);
        i=0;
        j=1;
        while (j<mm)
        { /* select small primes */
            p=mip->PRIMES[++i];
            rem=D%p;
            if (spmd(rem,(p-1)/2,p)<=1) /* x = spmd(a,b,c) = a^b mod c */
            { /* use only if Jacobi symbol = 0 or 1 */
                epr[++j]=p;
                dp=(double)p;
                if (kk%p==0) fks+=log(dp)/dp;
                else         fks+=2*log(dp)/(dp-1.0e0);
            }
        }
        if (fks>top)
        { /* find biggest fks */
            top=fks;
            bk=nk;
        }
    } while (!found);
    return kk;
}

int initv()
{ /* initialize */
    Big T;
    double dp;
    int i,k,digits,pak,maxp;

    nbts=8*sizeof(int);

    cout << "input number to be factored N= \n";
    cin >> NN;
    if (prime(NN))
    {
        cout << "this number is prime!\n";
        return (-1);
    }
    T=NN;
    digits=1;                   /* digits in N */
    while ((T/=10)>0) digits++;

    if (digits<10) mm=digits;               
    else mm=25;
    if (digits>20) mm=(digits*digits*digits*digits)/4096;
    dp=(double)2*(mm+100);          /* number of primes to generate */

    maxp=(int)(dp*(log(dp*log(dp)))); /* Rossers upper bound */
    gprime(maxp);

    epr=(int *)mr_alloc(mm+1,sizeof(int));

    k=knuth(mm,epr,NN,DD);

    RR=sqrt(DD);

    if (RR*RR==DD)
    {
        cout << k << "N is a perfect square!" << endl;
        cout << "factors are" << endl;
        if (prime(RR)) cout << "prime factor     ";
        else           cout << "composite factor ";
        cout << RR << endl;
        NN=NN/RR;
        if (prime(NN)) cout << "prime factor     ";
        else           cout << "composite factor ";
        cout << NN << endl;
        return (-1);
    }
    cout << "using multiplier k= " << k;
    cout << "\nand " << mm << " small primes as factor base\n";
    gprime(0);   /* reclaim PRIMES space */

    mlf=2*mm;    

    r1=(int *)mr_alloc((mm+1),sizeof(int));
    r2=(int *)mr_alloc((mm+1),sizeof(int));
    rp=(int *)mr_alloc((mm+1),sizeof(int));
    b=(int *)mr_alloc((mm+1),sizeof(int));
    e=(int *)mr_alloc((mm+1),sizeof(int));

    logp=(unsigned char *)mr_alloc(mm+1,1);

    pr=(int *)mr_alloc((mlf+1),sizeof(int));
    hash=(int *)mr_alloc(2*mlf+1,sizeof(int));

    sieve=(unsigned char *)mr_alloc(SSIZE+1,1);

    x=new Big[mm+1];
    y=new Big[mm+1];
    z=new Big[mlf+1];
    w=new Big[mlf+1];

    EE=(unsigned int **)mr_alloc(mm+1,sizeof(unsigned int *));
    G=(unsigned int **) mr_alloc(mlf+1,sizeof(unsigned int *));


    pak=1+mm/(8*sizeof(int));

    for (i=0;i<=mm;i++)
    { 
        x[i]=0;
        y[i]=0;
        b[i]=(-1);
        EE[i]=(unsigned int *)mr_alloc(pak,sizeof(int));
    }
    for (i=0;i<=mlf;i++)
    {
        z[i]=0;
        w[i]=0;
        G[i]=(unsigned int *)mr_alloc(pak,sizeof(int));
    }

    return 1;
}

BOOL gotcha(Big& NN,Big& P)
{ /* use new factorisation */
    Big XX,YY,T;
    int r,j,i,k,n,rb,had,hp;
    BOOL found;
    found=TRUE;
    if (partial)
    { /* check partial factorisation for usefulness */
        had=lp%hmod;
        forever
        { /* hash search for matching large prime */
            hp=hash[had];
            if (hp<0)
            { /* failed to find match */
                found=FALSE;
                break;
            }
            if (pr[hp]==lp) break; /* hash hit! */
            had=(had+(hmod2-lp%hmod2))%hmod;
        }
        if (!found && nlp>=mlf) return FALSE;
    }
    XX=P;
    YY=1;
    for (k=1;k<=mm;k++)
    { /* build up square part in YY  *
       * reducing e[k] to 0s and 1s */
        if (e[k]<2) continue;
        r=e[k]/2;
        e[k]%=2;
        T=epr[k];
        YY*=pow(T,r);
    }
    if (partial)
    { /* factored with large prime */
        if (!found)
        { /* store new partial factorization   */
            hash[had]=nlp;
            pr[nlp]=lp;
            z[nlp]=XX;
            w[nlp]=YY;
            for (n=0,rb=0,j=0;j<=mm;j++)
            {
                G[nlp][n]|=((e[j]&1)<<rb);
                if (++rb==nbts) n++,rb=0;
            }
            nlp++;         
        }
        if (found)
        { /* match found so use as factorization  */
            cout << "\b\b\b\b\b*";
            XX=(XX*z[hp])%NN;
            YY=(YY*w[hp])%NN;
            for (n=0,rb=0,j=0;j<=mm;j++)
            {
                e[j]+=((G[hp][n]>>rb)&1);
                if (e[j]==2)
                {
                    YY=(YY*epr[j])%NN;
                    e[j]=0;
                }
                if (++rb==nbts) n++,rb=0;
            }
            YY=(YY*lp)%NN; 
        }
    }
    else cout << "\b\b\b\b\b " << flush;
    if (found)
    {
        for (k=mm;k>=0;k--)
        { /* use new factorization in search for solution */
            if (e[k]%2==0) continue;
            if (b[k]<0)
            { /* no solution this time */
                found=FALSE;
                break;
            }
            i=b[k];
            XX=(XX*x[i])%NN;    /* This is very inefficient -  */
            YY=(YY*y[i])%NN;    /* There must be a better way! */
            for (n=0,rb=0,j=0;j<=mm;j++)
            { /* Gaussian elimination */
                e[j]+=((EE[i][n]>>rb)&1);
                if (++rb==nbts) n++,rb=0;
            }
        }
        for (j=0;j<=mm;j++)
        { /* update YY */
            if (e[j]<2) continue;
            T=epr[j];
            YY=(YY*pow(T,e[j]/2,NN))%NN;  /* x = pow(a,b,c) = a^b mod c */
        }
        if (!found)
        { /* store details in EE, x and y for later  */
            b[k]=jj;
            x[jj]=XX;
            y[jj]=YY;
            for (n=0,rb=0,j=0;j<=mm;j++)
            {
                EE[jj][n]|=((e[j]&1)<<rb);
                if (++rb==nbts) n++,rb=0;
            }
            jj++;
            cout << setw(4) << jj; 
        }
    }
    if (found)
    { /* check for false alarm */
        P=XX+YY;
        cout << "\ntrying...\n";
        if (XX==YY || P==NN) found=FALSE;
        if (!found) cout << "working... " << setw(4) << jj << flush;
    }
    return found;
}

BOOL factored(long lptr,Big& T)
{ /* factor quadratic residue T */
    BOOL facted;
    int i,j,r,st;  
    partial=FALSE;
    facted=FALSE;
    for (j=1;j<=mm;j++)
    { /* now attempt complete factorisation of T */
        r=lptr%epr[j];
        if (r<0) r+=epr[j];
        if (r!=r1[j] && r!=r2[j]) continue;
        while (T%epr[j]==0)
        { /* cast out epr[j] */
            e[j]++;
            T/=epr[j];
        }
        st=toint(T);      /*  st = T as an int; st=TOOBIG if too big */
        if (st==1)
        {
           facted=TRUE;
           break;
        }
        if ((T/epr[j])<=epr[j])
        { /* st is prime < epr[mm]^2 */
            if (st==MR_TOOBIG || (st/epr[mm])>(1+mlf/50)) break;
            if (st<=epr[mm])
                for (i=j;i<=mm;i++)
                if (st==epr[i])
                {
                    e[i]++;
                    facted=TRUE;
                    break;
                }
            if (facted) break;
            lp=st;  /* factored with large prime */
            partial=TRUE;
            facted=TRUE;
            break;
        }
    }
    return facted;
} 

void new_poly()
{ /* form the next polynomial */
    int i,r,s,s1,s2;
   
    r=mip->NTRY;        /* MR_NTRY is global - number of trys at proving */
    mip->NTRY=1;        /* a probable prime  */
    do
    { /* looking for suitable prime DG = 3 mod 4 */
        do DG+=4; while(!prime(DG));
        TT=(DG-1)/2;
        TT=pow(DD,TT,DG);  /*  check DD is quad residue */
    } while (TT!=1);
    mip->NTRY=r;
    TT=(DG+1)/4;
    BB=pow(DD,TT,DG);
    TT=(DD-BB*BB)/DG;
    AA=inverse(2*BB,DG);
    AA=(AA*TT)%DG;
    BB=AA*DG+BB;       /* BB^2 = DD mod DG^2  */
    AA=DG*DG;
    IG=inverse(DG,DD); /*  IG = 1/DG mod DD  */ 
    r1[0]=r2[0]=0;
    for (i=1;i<=mm;i++) 
    { /* find roots of quadratic mod each prime */
        s=BB%epr[i];
        r=AA%epr[i];
        r=invers(r,epr[i]);      /* r = 1/AA mod p */
        s1=(epr[i]-s+rp[i]);
        s2=(epr[i]-s+epr[i]-rp[i]);
        r1[i]=smul(s1,r,epr[i]);  /* s1 = s1*r mod epr[i] */
        r2[i]=smul(s2,r,epr[i]);  
    }
}


int main()
{ /* factoring via quadratic sieve */
    unsigned int i,j,a,*SV;
    unsigned char logpi;
    int k,S,r,s1,s2,NS,logm,ptr,threshold,epri;
    long M,la,lptr;
    mip=&precision;
    if (initv()<0) return 0;
    hmod=2*mlf+1;               /* set up hash table */
    TT=hmod;
    while (!prime(TT)) TT-=2;
    hmod=toint(TT);
    hmod2=hmod-2;
    for (k=0;k<hmod;k++) hash[k]=(-1);

    M=50*(long)mm;
    NS=M/SSIZE;
    if (M%SSIZE!=0) NS++;
    M=SSIZE*(long)NS;
    logm=0;
    la=M;
    while ((la/=2)>0) logm++;   /* logm = log(M) */

    rp[0]=logp[0]=0;
    for (k=1;k<=mm;k++)
    { /* find root mod each prime, and approx log of each prime */
        r=DD%epr[k];
        rp[k]=sqrmp(r,epr[k]);     /* = square root of r mod epr[k] */
        logp[k]=0;
        r=epr[k];
        while((r/=2)>0) logp[k]++;
    }
    r=DD%8;             /* take special care of 2  */
    if (r==5) logp[1]++;
    if (r==1) logp[1]+=2;
    threshold=logm-2*logp[mm];
 
    jj=0;
    nlp=0;

    TT=RR;
    while ((TT/=2)>0) threshold++;   /*  add in number of bits in RR */
    DG=sqrt(DD*2);
    DG=sqrt(DG/M);
    if (DG%2==0) ++DG;
    if (DG%4==1) DG+=2;
    cout << "working...    0" << flush;
    forever
    { /* try a new polynomial  */

        new_poly();

        for (ptr=(-NS);ptr<NS;ptr++)
        { /* sieve over next period */
            la=(long)ptr*SSIZE;
            SV=(unsigned int *)sieve;
            for (i=0;i<SSIZE/sizeof(int);i++) *SV++=0;
            for (k=1;k<=mm;k++)
            { /* sieving with each prime */
                epri=epr[k];
                logpi=logp[k];
                r=la%epri;
                s1=(r1[k]-r)%epri;
                if (s1<0) s1+=epri;
                s2=(r2[k]-r)%epri;
                if (s2<0) s2+=epri;
                for (j=s1;j<SSIZE;j+=epri) sieve[j]+=logpi;
                if (s1==s2) continue;
                for (j=s2;j<SSIZE;j+=epri) sieve[j]+=logpi;
            }

            for (a=0;a<SSIZE;a++)
            {  /* main loop - look for fully factored residues */
                if (sieve[a]<threshold) continue;
                lptr=la+a;
                S=0;
                TT=AA*lptr+BB;
                PP=(TT*IG)%DD;             /*  PP = (AAx + BB)/G */
                if (PP<0) PP+=DD;
                VV=(PP*PP)%DD;             /*  VV = PP^2 mod kN  */
                if (TT<0) TT=(-TT);
                if (TT<RR) S=1;            /*  check for -ve VV  */
                if (S==1) VV=DD-VV;
                e[0]=S;
                for (k=1;k<=mm;k++) e[k]=0;
                if (!factored(lptr,VV)) continue;
                if (gotcha(NN,PP))
                { /* factors found! */
                    PP=gcd(PP,NN);
                    cout << "factors are";
                    if (prime(PP)) cout << "\nprime factor     " << PP;
                    else           cout << "\ncomposite factor " << PP;
                    NN/=PP;
                    if (prime(NN)) cout << "\nprime factor     " << NN;
                    else           cout << "\ncomposite factor " << NN;
                    cout << endl;
                    return 0;
                }
            }
        }
    }
}

