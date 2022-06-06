/*
 *   Program to factor big numbers using Williams (p+1) method.
 *   Works when for some prime divisor p of n, p+1 has only
 *   small factors.
 *   See "Speeding the Pollard and Elliptic Curve Methods"
 *   by Peter Montgomery, Math. Comp. Vol. 48. Jan. 1987 pp243-264
 */

#include <stdio.h>
#include <stdlib.h>
#include "miracl.h"

#define LIMIT1 10000      /* must be int, and > MULT/2 */
#define LIMIT2 500000L    /* may be long */
#define MULT   2310       /* must be int, product of small primes 2.3.. */
#define NEXT   13         /* next small prime */
#define NTRYS  3          /* number of attempts */

static BOOL plus[1+MULT/2],minus[1+MULT/2];

miracl *mip;

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

int main()
{  /*  factoring program using Williams (p+1) method */
    int k,phase,m,nt,iv,pos,btch;
    long i,p,pa,interval;
    big b,q,n,fp,fvw,fd,fn,t;
    static big fu[1+MULT/2];
    static BOOL cp[1+MULT/2];
    mip=mirsys(30,0);
    b=mirvar(0);
    q=mirvar(0);
    n=mirvar(0);
    t=mirvar(0);
    fp=mirvar(0);
    fvw=mirvar(0);
    fd=mirvar(0);
    fn=mirvar(0);
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
    for (nt=0,k=3;k<10;k++)
    { /* try more than once for p+1 condition (may be p-1) */
        convert(k,b);              /* try b=3,4,5..        */
        convert((k*k-4),t);
        if (egcd(t,n,t)!=1) continue; /* check (b*b-4,n)!=0 */
        nt++;
        phase=1;
        p=0;
        btch=50;
        i=0;
        printf("phase 1 - trying all primes less than %d\n",LIMIT1);
        printf("prime= %8ld",p);
        forever
        { /* main loop */
            if (phase==1)
            { /* looking for all factors of p+1 < LIMIT1 */
                p=mip->PRIMES[i];
                if (mip->PRIMES[i+1]==0)
                { /* now change gear */
                    phase=2;
                    printf("\nphase 2 - trying last prime less than %ld\n"
                           ,LIMIT2);
                    printf("prime= %8ld",p);
                    copy(b,fu[1]);
                    copy(b,fp);
                    mad(b,b,b,n,n,fd);
                    decr(fd,2,fd);     
                    negify(b,t);
                    mad(fd,b,t,n,n,fn);
                    for (m=5;m<=MULT/2;m+=2)
                    { /* store fu[m] = Vm(b) */
                        negify(fp,t);
                        mad(fn,fd,t,n,n,t);
                        copy(fn,fp);
                        copy(t,fn);
                        if (!cp[m]) continue;
                        copy(t,fu[m]);
                    }
                    convert(MULT,t);    
                    lucas(b,t,n,fp,fd);
                    iv=(int)(p/MULT);
                    if (p%MULT>MULT/2) iv++;
                    interval=(long)iv*MULT;
                    p=interval+1;
                    convert(iv,t);
                    lucas(fd,t,n,fp,fvw);
                    negify(fp,fp);
                    subtract(fvw,fu[p%MULT],q);
                    marks(interval);
                    btch*=100;
                    i++;
                    continue;
                }
                pa=p;
                while ((LIMIT1/p) > pa) pa*=p;
                convert((int)pa,t);   
                lucas(b,t,n,fp,q);
                copy(q,b);
                decr(q,2,q);
            }
            else
            { /* phase 2 - looking for last large prime factor of (p+1) */
                p+=2;
                pos=(int)(p%MULT);
                if (pos>MULT/2)
                { /* increment giant step */
                    iv++;
                    interval=(long)iv*MULT;
                    p=interval+1;
                    marks(interval);
                    pos=1;
                    copy(fvw,t);
                    mad(fvw,fd,fp,n,n,fvw);
                    negify(t,fp);
                }
                if (!cp[pos]) continue;

        /* if neither interval+/-pos is prime, don't bother */
                if (!plus[pos] && !minus[pos]) continue;
                subtract(fvw,fu[pos],t);
                mad(q,t,t,n,n,q);  /* batching gcds */
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
                else          printf("composite factor ");
                cotnum(t,stdout);
                divide(n,t,n);
                if (isprime(n)) printf("prime factor     ");
                else          printf("composite factor ");
                cotnum(n,stdout);
                return 0;
            }
        } 
        if (nt>=NTRYS) break;
        printf("\ntrying again\n");
    }
    printf("\nfailed to factor\n");
    return 0;
}

