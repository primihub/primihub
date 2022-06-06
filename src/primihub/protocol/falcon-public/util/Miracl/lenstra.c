/*
 *   Program to factor big numbers using Lenstras elliptic curve method.
 *   Works when for some prime divisor p of n, p+1+d has only
 *   small factors, where d depends on the particular curve used.
 *   See "Speeding the Pollard and Elliptic Curve Methods"
 *   by Peter Montgomery, Math. Comp. Vol. 48 Jan. 1987 pp243-264
 */

#include <stdio.h>
#include <stdlib.h>
#include "miracl.h"

#define LIMIT1 10000        /* must be int, and > MULT/2 */
#define LIMIT2 1000000L     /* may be long */
#define MULT   2310         /* must be int, product of small primes 2.3... */
#define NEXT   13           /* next small prime */
#define NCURVES 200         /* no. of curves to try */

miracl *mip;
static big ak,t,w,s1,d1,s2,d2;
static BOOL plus[1+MULT/2],minus[1+MULT/2];

void marks(long start)
{ /* mark non-primes in this interval. Note    *
   * that those < NEXT are dealt with already  */
    int i,pr,j,k;
    for (j=1;j<=MULT/2;j+=2) plus[j]=minus[j]=TRUE;
    for (i=0;;i++)
    { /* mark in both directions */
        pr=mip->PRIMES[i];
        if (pr<NEXT) continue;
        if ((long)pr*pr>start) break;
        k=pr-start%pr;
        for (j=k;j<=MULT/2;j+=pr)
            plus[j]=FALSE;
        k=start%pr;
        for (j=k;j<=MULT/2;j+=pr)
            minus[j]=FALSE;
    }        
}

void duplication(big sum,big diff,big x,big z)
{ /* double a point on the curve P(x,z)=2.P(x1,z1) */
    nres_modmult(sum,sum,t);
    nres_modmult(diff,diff,z);
    nres_modmult(t,z,x);          /* x = sum^2.diff^2 */
    nres_modsub(t,z,t);           /* t = sum^2-diff^2 */
    nres_modmult(ak,t,w);
    nres_modadd(z,w,z);           /* z = ak*t +diff^2 */
    nres_modmult(z,t,z);          /* z = z.t          */
}

void addition(big xd,big zd,big sm1,big df1,big sm2,big df2,big x,big z)
{ /* add two points on the curve P(x,z)=P(x1,z1)+P(x2,z2) *
   * given their difference P(xd,zd)                      */
    nres_modmult(df2,sm1,x);
    nres_modmult(df1,sm2,z);
    nres_modadd(z,x,t);
    nres_modsub(z,x,z);
    nres_modmult(t,t,x);
    nres_modmult(x,zd,x);     /* x = zd.[df1.sm2+sm1.df2]^2 */
    nres_modmult(z,z,z);
    nres_modmult(z,xd,z);     /* z = xd.[df1.sm2-sm1.df2]^2 */
}

void ellipse(big x,big z,int r,big x1,big z1,big x2,big z2)
{ /* calculate point r.P(x,z) on curve */
    int k,rr;
    k=1;
    rr=r;
    copy(x,x1);            
    copy(z,z1);
    nres_modadd(x1,z1,s1);
    nres_modsub(x1,z1,d1);
    duplication(s1,d1,x2,z2);  /* generate 2.P */
    while ((rr/=2)>1) k*=2;
    while (k>0)
    { /* use binary method */
        nres_modadd(x1,z1,s1);         /* form sums and differences */
        nres_modsub(x1,z1,d1);    /* x+z and x-z for P1 and P2 */
        nres_modadd(x2,z2,s2);
        nres_modsub(x2,z2,d2);
        if ((r&k)==0)
        { /* double P(x1,z1) mP to 2mP */
            addition(x,z,s1,d1,s2,d2,x2,z2);
            duplication(s1,d1,x1,z1);
        }
        else
        { /* double P(x2,z2) (m+1)P to (2m+2)P */
            addition(x,z,s1,d1,s2,d2,x1,z1);
            duplication(s2,d2,x2,z2);
        }    
        k/=2;
    }
}

int main()
{  /*  factoring program using Lenstras Elliptic Curve method */
    int phase,m,k,nc,iv,pos,btch,u,v;
    long i,p,pa,interval;
    big q,x,z,a,x1,z1,x2,z2,xt,zt,n,fvw;
    static big fu[1+MULT/2];
    static BOOL cp[1+MULT/2];
    mip=mirsys(30,0);
    q=mirvar(0);
    x=mirvar(0);
    z=mirvar(0);
    a=mirvar(0);
    x1=mirvar(0);
    z1=mirvar(0);
    x2=mirvar(0);
    z2=mirvar(0);
    n=mirvar(0);
    t=mirvar(0);
    s1=mirvar(0);
    d1=mirvar(0);
    s2=mirvar(0);
    d2=mirvar(0);
    ak=mirvar(0);
    xt=mirvar(0);
    zt=mirvar(0);
    fvw=mirvar(0);
    w=mirvar(0);
    gprime(LIMIT1);
    for (m=1;m<=MULT/2;m+=2)
        if (igcd(MULT,m)==1)
        {
            fu[m]=mirvar(0);
            cp[m]=TRUE;
        }
        else cp[m]=FALSE;
    printf("input number to be factored\n");
    cinnum(n,stdin);

    if (isprime(n))
    {
        printf("this number is prime!\n");
        return 0;
    }
    prepare_monty(n);

    for (nc=1,k=6;k<100;k++)
    { /* try a new curve */
                             /* generating an elliptic curve */
        u=k*k-5;
        v=4*k;
        convert(u,x);  nres(x,x);
        convert(v,z);  nres(z,z);
        nres_modsub(z,x,a);   /* a=v-u */

        copy(x,t);
        nres_modmult(x,x,x);
        nres_modmult(x,t,x);  /* x=u^3 */

        copy(z,t);
        nres_modmult(z,z,z);
        nres_modmult(z,t,z);  /* z=v^3 */

        copy(a,t);
        nres_modmult(t,t,t);
        nres_modmult(t,a,t);  /* t=(v-u)^3 */

        convert(3*u,a); nres(a,a);
        convert(v,ak);  nres(ak,ak);
        nres_modadd(a,ak,a);
        nres_modmult(t,a,t);  /* t=(v-u)^3.(3u+v) */

        convert(u,a);  nres(a,a);
        copy(a,ak);
        nres_modmult(a,a,a);
        nres_modmult(a,ak,a);   /* a=u^3 */
        convert(v,ak); nres(ak,ak);
        nres_modmult(a,ak,a);   /* a=u^3.v */
        nres_premult(a,16,a);
        nres_moddiv(t,a,ak);     /* ak=(v-u)^3.(3u+v)/16u^3v */

        nc++;

        phase=1;
        p=0;
        i=0;
        btch=50;
        printf("phase 1 - trying all primes less than %d\n",LIMIT1);
        printf("prime= %8ld",p);
        forever
        { /* main loop */
            if (phase==1)
            {
                p=mip->PRIMES[i];
                if (mip->PRIMES[i+1]==0)
                { /* now change gear */
                    phase=2;
                    printf("\nphase 2 - trying last prime less than %ld\n",
                            LIMIT2);
                    printf("prime= %8ld",p);
                    copy(x,xt);
                    copy(z,zt);
                    nres_modadd(x,z,s2);
                    nres_modsub(x,z,d2);                    /*   P = (s2,d2) */
                    duplication(s2,d2,x,z);
                    nres_modadd(x,z,s1);
                    nres_modsub(x,z,d1);                    /* 2.P = (s1,d1) */

                    nres_moddiv(x1,z1,fu[1]);               /* fu[1] = x1/z1 */
                    
                    addition(x1,z1,s1,d1,s2,d2,x2,z2); /* 3.P = (x2,z2) */
                    for (m=5;m<=MULT/2;m+=2)
                    { /* calculate m.P = (x,z) and store fu[m] = x/z */
                        nres_modadd(x2,z2,s2);
                        nres_modsub(x2,z2,d2);
                        addition(x1,z1,s2,d2,s1,d1,x,z);
                        copy(x2,x1);
                        copy(z2,z1);
                        copy(x,x2);
                        copy(z,z2);
                        if (!cp[m]) continue;
                        copy(z2,fu[m]);
                        nres_moddiv(x2,fu[m],fu[m]);
                    }
                    ellipse(xt,zt,MULT,x,z,x2,z2);
                    nres_modadd(x,z,xt);
                    nres_modsub(x,z,zt);              /* MULT.P = (xt,zt) */
                    iv=(int)(p/MULT);
                    if (p%MULT>MULT/2) iv++;
                    interval=(long)iv*MULT; 
                    p=interval+1;
                    ellipse(x,z,iv,x1,z1,x2,z2); /* (x1,z1) = iv.MULT.P */
                    nres_moddiv(x1,z1,fvw);                /* fvw = x1/z1 */
                    nres_modsub(fvw,fu[p%MULT],q);
                    marks(interval);
                    btch*=100;
                    i++;
                    continue;
                }
                pa=p;
                while ((LIMIT1/p) > pa) pa*=p;
                ellipse(x,z,(int)pa,x1,z1,x2,z2);
                copy(x1,x);
                copy(z1,z);
                copy(z,q);
            }
            else
            { /* phase 2 - looking for last large prime factor of (p+1+d) */
                p+=2;
                pos=(int)(p%MULT);
                if (pos>MULT/2)
                { /* increment giant step */
                    iv++;
                    interval=(long)iv*MULT;
                    p=interval+1;
                    marks(interval);
                    pos=1;
                    nres_moddiv(x2,z2,fvw);
                    nres_modadd(x2,z2,s2);
                    nres_modsub(x2,z2,d2);
                    addition(x1,z1,s2,d2,xt,zt,x,z);
                    copy(x2,x1);
                    copy(z2,z1);
                    copy(x,x2);
                    copy(z,z2);
                }
                if (!cp[pos]) continue;

        /* if neither interval +/- pos is prime, don't bother */
                if (!plus[pos] && !minus[pos]) continue;
                nres_modsub(fvw,fu[pos],t);
                nres_modmult(q,t,q);
            }
            if (i++%btch==0)
            { /* try for a solution */
                printf("\b\b\b\b\b\b\b\b%8ld",p);
                fflush(stdout);
                egcd(q,n,t);
                if (size(t)==1)
                {
                    if (p>LIMIT2) break;
                    else continue;
                }
                if (compare(t,n)==0)
                {
                    printf("\ndegenerate case");
                    break;
                }
                printf("\nfactors are\n");
                if (isprime(t)) printf("prime factor     ");
                else            printf("composite factor ");
                cotnum(t,stdout);
                divide(n,t,n);
                if (isprime(n)) printf("prime factor     ");
                else            printf("composite factor ");
                cotnum(n,stdout);
                return 0;
            }
        }
        if (nc>NCURVES) break;
        printf("\ntrying a different curve %d\n",nc);
    } 
    printf("\nfailed to factor\n");
    return 0;
}

