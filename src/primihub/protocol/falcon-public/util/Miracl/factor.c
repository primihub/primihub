/*
 *   Program to factor Integers
 *
 *   Current parameters assume a large 32-bit address space is available.
 *
 *   This program is cobbled together from the implementations of various
 *   factoring algorithms better described in:-
 *
 *   brute.c/cpp     - brute force division by small primes
 *   brent.c/cpp     - Pollard Rho method as improved by Brent
 *   pollard.c/cpp   - Pollard (p-1) method
 *   williams.c/cpp  - Williams (p+1) method
 *   lenstra.c/cpp   - Lenstra's Elliptic Curve method
 *   qsieve.c/cpp    - The Multiple polynomial quadratic sieve
 *
 *   Note that the .cpp C++ implementations are easier to follow
 *
 *   NOTE: The quadratic sieve program requires a lot of memory for 
 *   bigger numbers. It may fail if you system cannot provide the memory
 *   requested.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "miracl.h"

#define LIMIT 15000
#define BTRIES 1000
#define MULT 2310      /* 2*3*5*7*11    */
#define NEXT 13        /* .. next prime */
#define mr_min(a,b) ((a) < (b)? (a) : (b))

static big *fu;
static BOOL *cp,*plus,*minus;
big n;
FILE *output;
static BOOL suppress=FALSE;
static int PADDING;
static miracl *mip;

void brute(void)
{ /* find factors by brute force division */
    big x,y;
    int m,p;
    gprime(LIMIT);
    x=mirvar(0);
    y=mirvar(0);
    m=0;
    p=mip->PRIMES[0];
    forever
    { /* try division by each prime in turn */
        if (subdiv(n,p,y)==0)
        { /* factor found */
            copy(y,n);
            if (!suppress) printf("PRIME FACTOR     ");
            fprintf(output,"%d\n",p);
            if (size(n)==1) exit(0);
            continue;
        }
        if (size(y)<=p) 
        { /* must be prime */
            if (!suppress) printf("PRIME FACTOR     ");
            cotnum(n,output);
            exit(0);
        }
        p=mip->PRIMES[++m];
        if (p==0) break;
    }
    if (isprime(n)) 
    {
        if (!suppress) printf("PRIME FACTOR     ");
        cotnum(n,output);
        exit(0);
    }
    mr_free(x);
    mr_free(y);
    gprime(0);
}

void brent(void)
{  /*  factoring program using Brents method */
    long k,r,i,m,iter;
    big x,y,ys,z,q,c3,t;
    x=mirvar(0);
    y=mirvar(0);
    ys=mirvar(0);
    z=mirvar(0);
    q=mirvar(0);
    c3=mirvar(3);
    t=mirvar(0);
    m=10;
    r=1;
    iter=0;
    do
    {
        if (!suppress) printf("iterations=%5ld",iter);
        convert(1,q);
        do
        {
            copy(y,x);
            for (i=1;i<=r;i++)
                mad(y,y,c3,n,n,y);
            k=0;
            do
            {
                iter++;
                if (iter>BTRIES)
                {
                    if (!suppress) 
                    {
                         printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
                         fflush(stdout);
                    }
                    mr_free(t);
                    mr_free(x);
                    mr_free(y);
                    mr_free(ys);
                    mr_free(z);
                    mr_free(q);
                    mr_free(c3);             
                    return;
                }
                if (iter%10==0) if (!suppress) 
                {
                    printf("\b\b\b\b\b%5ld",iter);
                    fflush(stdout);
                }
                copy(y,ys);
                for (i=1;i<=mr_min(m,r-k);i++)
                {
                    mad(y,y,c3,n,n,y);
                    subtract(y,x,z);
                    mad(z,q,q,n,n,q);
                }
                egcd(q,n,z);
                k+=m;
            } while (k<r && size(z)==1);
            r*=2;
        } while (size(z)==1);
        if (compare(z,n)==0) do 
        { /* back-track */
            mad(ys,ys,c3,n,n,ys);
            subtract(ys,x,z);
        } while (egcd(z,n,z)==1);
        if (!suppress) 
        {
            printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
            fflush(stdout);
        }
        forever
        {
            if (isprime(z))
            {
                if (!suppress) printf("PRIME FACTOR     ");
                cotnum(z,output);
            }
            else
            { /* if end of factorisation - pass it on... */
                if (compare(z,n)==0) return;
              /* you will have to come back to it */
                if (!suppress) printf("COMPOSITE FACTOR ");
                else printf("& ");
                cotnum(z,output);
            }
       
            divide(n,z,n);
            divide(y,n,n);

            copy(n,t);
            divide(t,z,z);
            if (size(t)==0) continue;
            break;

        }
        if (size(n)==1) exit(0);
    } while (!isprime(n));      
    if (!suppress) printf("PRIME FACTOR     ");
    cotnum(n,output);
    exit(0);
}

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

void pollard(int lim1,long lim2)
{ /* factoring program using Pollards (p-1) method */
    long i,p,pa,interval;
    int phase,m,pos,btch,iv;
    big t,b,bw,bvw,bd,bp,q;
    t=mirvar(0);
    b=mirvar(0);
    q=mirvar(0);
    bw=mirvar(0);
    bvw=mirvar(0);
    bd=mirvar(0);
    bp=mirvar(0);
    gprime(lim1);
    for (m=1;m<=MULT/2;m+=2)
        if (igcd(MULT,m)==1)
        {
            fu[m]=mirvar(0);
            cp[m]=TRUE;
        }
        else cp[m]=FALSE;
    phase=1;
    p=0;
    btch=50;
    i=0;
    convert(3,b);
    if (!suppress) printf("phase 1 - trying all primes less than %d\n",lim1);
    if (!suppress) printf("prime= %8ld",p);
    forever
    {
        if (phase==1)
        { /* looking for all factors of (p-1) < lim1 */
            p=mip->PRIMES[i];
            if (mip->PRIMES[i+1]==0)
            {
                 phase=2;
                 if (!suppress)
                 {
                     printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
                     printf("phase 2 - trying last prime less than %ld\n"
                        ,lim2);
                     printf("prime= %8ld",p);
                 }
                 power(b,8,n,bw);
                 convert(1,t);
                 copy(b,bp);
                 copy(b,fu[1]);
                 for (m=3;m<=MULT/2;m+=2)
                 { /* store fu[m] = b^(m*m) */
                     mad(t,bw,bw,n,n,t);
                     mad(bp,t,t,n,n,bp);
                     if (cp[m]) copy(bp,fu[m]);
                 }
                 power(b,MULT,n,t);
                 power(t,MULT,n,t);
                 mad(t,t,t,n,n,bd);        /* bd=b^(2*MULT*MULT) */
                 iv=p/MULT;
                 if (p%MULT>MULT/2) iv++;
                 interval=(long)iv*MULT;
                 p=interval+1;
                 marks(interval);
                 power(t,2*iv-1,n,bw);
                 power(t,iv,n,bvw);
                 power(bvw,iv,n,bvw);      /* bvw = b^(MULT*MULT*iv*iv) */
                 subtract(bvw,fu[p%MULT],q);
                 btch*=100;
                 i++;
                 continue;
            }
            pa=p;
            while ((lim1/p) > pa) pa*=p;
            power(b,(int)pa,n,b);
            decr(b,1,q);
        }
        else
        { /* looking for last PRIME FACTOR of (p-1) < lim2 */
            p+=2;
            pos=p%MULT;
            if (pos>MULT/2) 
            { /* increment giant step */
                iv++;
                interval=(long)iv*MULT;
                p=interval+1;
                marks(interval);
                pos=1;
                mad(bw,bd,bd,n,n,bw);
                mad(bvw,bw,bw,n,n,bvw);
            }
            if (!cp[pos]) continue;

        /* if neither interval+/-pos is prime, don't bother */
            if (!plus[pos] && !minus[pos]) continue;

            subtract(bvw,fu[pos],t);
            mad(q,t,t,n,n,q);  /* batching gcds */
        }
        if (i++%btch==0)
        { /* try for a solution */
            if (!suppress)
            {
                printf("\b\b\b\b\b\b\b\b%8ld",p);
                fflush(stdout);
            }
            egcd(q,n,t);
            if (size(t)==1)
            {
                if (p>lim2) 
                {
                    if (!suppress)
                    {
                        printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
                        fflush(stdout);
                    }
                    break;
                }
                else continue;
            }
            if (compare(t,n)==0)
            {
                if (!suppress) 
                {
                    printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
                    printf("degenerate case\n");
                }
                break;
            }
            if (!suppress)
            {
                printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
                if (isprime(t)) printf("PRIME FACTOR     ");
                else            printf("COMPOSITE FACTOR ");
            }
            else if (!isprime(t)) printf("& ");
            cotnum(t,output);
            divide(n,t,n);
            if (isprime(n))
            {
                if (!suppress) printf("PRIME FACTOR     ");
                cotnum(n,output);
                exit(0);
            }
            break;
        }
    }
    gprime(0);
    mr_free(t);
    mr_free(b);
    mr_free(q);
    mr_free(bw);
    mr_free(bvw);
    mr_free(bd);
    mr_free(bp);
    for (m=1;m<=MULT/2;m+=2)
        if (igcd(MULT,m)==1) mr_free(fu[m]);
}

void williams(int lim1,long lim2,int ntrys)
{  /*  factoring program using Williams (p+1) method */
    int k,phase,m,nt,iv,pos,btch;
    long i,p,pa,interval;
    big b,q,fp,fvw,fd,fn,t;
    b=mirvar(0);
    q=mirvar(0);
    t=mirvar(0);
    fp=mirvar(0);
    fvw=mirvar(0);
    fd=mirvar(0);
    fn=mirvar(0);
    gprime(lim1);
    

    for (m=1;m<=MULT/2;m+=2)
        if (igcd(MULT,m)==1)
        {
            fu[m]=mirvar(0);
            cp[m]=TRUE;
        }
        else cp[m]=FALSE;
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
        if (!suppress) printf("phase 1 - trying all primes less than %d\n",lim1);
        if (!suppress) printf("prime= %8ld",p);
        forever
        { /* main loop */
            if (phase==1)
            { /* looking for all factors of p+1 < lim1 */
                p=mip->PRIMES[i];
                if (mip->PRIMES[i+1]==0)
                { /* now change gear */
                    phase=2;
                    if (!suppress)
                    {
                        printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
                        printf("phase 2 - trying last prime less than %ld\n"
                           ,lim2);
                        printf("prime= %8ld",p);
                    }
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
                    iv=p/MULT;
                    if (p%MULT>MULT/2) iv++;
                    interval=(long)iv*MULT;
                    p=interval+1;
                    marks(interval);
                    convert(iv,t);
                    lucas(fd,t,n,fp,fvw);
                    negify(fp,fp);
                    subtract(fvw,fu[p%MULT],q);
                    btch*=100;
                    i++;
                    continue;
                }
                pa=p;
                while ((lim1/p) > pa) pa*=p;
                convert((int)pa,t);
                lucas(b,t,n,fp,q);
                copy(q,b);
                decr(q,2,q);
            }
            else
            { /* phase 2 - looking for last large PRIME FACTOR of (p+1) */
                p+=2;
                pos=p%MULT;
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
                if (!suppress)
                {
                     printf("\b\b\b\b\b\b\b\b%8ld",p);
                     fflush(stdout);
                }
                egcd(q,n,t);
                if (size(t)==1)
                {
                    if (p>lim2) 
                    {
                        if (!suppress)
                        {
                             printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
                             fflush(stdout);
                        }
                        break;
                    }
                    else continue;
                }
                if (compare(t,n)==0)
                {
                    if (!suppress) 
                    {
                        printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
                        printf("degenerate case\n");
                    }
                    break;
                }
                if (!suppress)
                {
                    printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
                    if (isprime(t)) printf("PRIME FACTOR     ");
                    else            printf("COMPOSITE FACTOR ");
                }
                else if (!isprime(t)) printf("& ");
                cotnum(t,output);
                divide(n,t,n);
                if (isprime(n)) 
                {
                    if (!suppress) printf("PRIME FACTOR     ");
                    cotnum(n,output);
                    exit(0);
                }
                nt=ntrys;
                break;
            }
        } 
        if (nt>=ntrys) break;
    }
    gprime(0);
    mr_free(b);
    mr_free(q);
    mr_free(t);
    mr_free(fp);
    mr_free(fvw);
    mr_free(fd);
    mr_free(fn);
    for (m=1;m<=MULT/2;m+=2)
        if (igcd(MULT,m)==1) mr_free(fu[m]);
}


static big ak,t,ww,s1,d1,s2,d2;

void duplication(big sum,big diff,big x,big z)
{ /* double a point on the curve P(x,z)=2.P(x1,z1) */
    nres_modmult(sum,sum,t);
    nres_modmult(diff,diff,z);
    nres_modmult(t,z,x);          /* x = sum^2.diff^2 */
    nres_modsub(t,z,t);           /* t = sum^2-diff^2 */
    nres_modmult(ak,t,ww);
    nres_modadd(z,ww,z);           /* z = ak*t +diff^2 */
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

int lenstra(int lim1,long lim2,int nc,int kurve,int ncurves)
{  /*  factoring program using Lenstras Elliptic Curve method */
    int phase,m,iv,pos,btch,u,v,ncr;
    long i,p,pa,interval;
    big q,x,z,a,x1,z1,x2,z2,xt,zt,fvw;
    q=mirvar(0);
    x=mirvar(0);
    z=mirvar(0);
    a=mirvar(0);
    x1=mirvar(0);
    z1=mirvar(0);
    x2=mirvar(0);
    z2=mirvar(0);
    s1=mirvar(0);
    d1=mirvar(0);
    s2=mirvar(0);
    d2=mirvar(0);
    ak=mirvar(0);
    xt=mirvar(0);
    zt=mirvar(0);
    fvw=mirvar(0);
    t=mirvar(0);
    ww=mirvar(0);
    gprime(lim1);

    for (m=1;m<=MULT/2;m+=2)
        if (igcd(MULT,m)==1)
        {
            fu[m]=mirvar(0);
            cp[m]=TRUE;
        }
        else cp[m]=FALSE;


    prepare_monty(n);
    /* try a new curve */             
    /* generating an elliptic curve */
        ncr=nc;
        u=kurve*kurve-5;
        v=4*kurve;

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

        phase=1;
        p=0;
        i=0;
        btch=50;
        if (!suppress) printf("curve %3d phase 1 - trying all primes less than %d\n",nc,lim1);
        if (!suppress) printf("prime= %8ld",p);
        ncr++;
        forever
        { /* main loop */
            if (phase==1)
            {
                p=mip->PRIMES[i];
                if (mip->PRIMES[i+1]==0)
                { /* now change gear */
                    phase=2;
                    if (!suppress) 
                    {
                        printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
                        printf("          phase 2 - trying last prime less than %ld\n",
                            lim2);
                        printf("prime= %8ld",p);
                    }
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
                    marks(interval);
                    ellipse(x,z,iv,x1,z1,x2,z2); /* (x1,z1) = iv.MULT.P */
                    nres_moddiv(x1,z1,fvw);                /* fvw = x1/z1 */
                    nres_modsub(fvw,fu[p%MULT],q);
                    btch*=100;
                    i++;
                    continue;
                }
                pa=p;
                while ((lim1/p) > pa) pa*=p;
                ellipse(x,z,(int)pa,x1,z1,x2,z2);
                copy(x1,x);
                copy(z1,z);
                copy(z,q);
            }
            else
            { /* phase 2 - looking for last large PRIME FACTOR of (p+1+d) */
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
                if (!suppress) 
                {
                    printf("\b\b\b\b\b\b\b\b%8ld",p);
                    fflush(stdout);
                }
                egcd(q,n,t);
                if (size(t)==1)
                {
                    if (p>lim2) 
                    {
                        if (!suppress) 
                        {
                            printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
                            fflush(stdout);
                        }
                        break;
                    }
                    else continue;
                }
                if (compare(t,n)==0)
                {
                    if (!suppress) 
                    {
                        printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
                        printf("degenerate case\n");
                    }
                    break;
                }

                if (!suppress) 
                {
                    printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
                    if (isprime(t)) printf("PRIME FACTOR     ");
                    else            printf("COMPOSITE FACTOR ");
                }
                else if (!isprime(t)) printf("& ");
                cotnum(t,output);
                divide(n,t,n);
                if (isprime(n))
                {
                    if (!suppress) printf("PRIME FACTOR     ");
                    cotnum(n,output);
                    exit(0);
                }
                ncr=ncurves+1;
                break;
            }
        }
 
    gprime(0);
    mr_free(ww);
    mr_free(t);
    mr_free(q);
    mr_free(x);
    mr_free(z);
    mr_free(a);
    mr_free(x1);
    mr_free(z1);
    mr_free(x2);
    mr_free(z2);
    mr_free(s1);
    mr_free(d1);
    mr_free(s2);
    mr_free(d2);
    mr_free(ak);
    mr_free(xt);
    mr_free(zt);
    mr_free(fvw);
    for (m=1;m<=MULT/2;m+=2)
        if (igcd(MULT,m)==1) mr_free(fu[m]);
    return ncr;
}

void do_lenstra(int lim1,long lim2,int ncurves)
{
    int nc=1;
    int kurve=5;

    for (nc=1;nc<=ncurves;)
    {
        kurve++;
        nc=lenstra(lim1,lim2,nc,kurve,ncurves);
    }
}

#define SSIZE 100000    /* Maximum sieve size            */

static big NN,TT,DD,RR,VV,PP,XX,YY,DG,IG,AA,BB;
static big *x,*y,*z,*w;
static unsigned int **EE,**G;
static int *epr,*r1,*r2,*rp,*b,*pr,*e,*hash;
static unsigned char *logp,*sieve;
static int mm,mlf,jj,nbts,nlp,lp,hmod,hmod2;
static BOOL partial;

int knuth(int mm,int *epr,big N,big D)
{ /* Input number to be factored N and find best multiplier k  *
   * for use over a factor base epr[] of size mm.  Set D=k.N.  */
    double dp,fks,top;
    BOOL found;
    int i,j,bk,nk,kk,r,p;
    static int K[]={0,1,2,3,5,6,7,10,11,13,14,15,17,0};
    top=(-10.0e0);
    found=FALSE;
    nk=0;
    bk=0;
    epr[0]=1;
    epr[1]=2;
    do
    { /* search for best Knuth-Schroepel multiplier */
        kk=K[++nk];
        if (kk==0)
        { /* finished */
            kk=K[bk];
            found=TRUE;
        }
        premult(N,kk,D);
        fks=log(2.0e0)/(2.0e0);
        r=remain(D,8);
        if (r==1) fks*=(4.0e0);
        if (r==5) fks*=(2.0e0);
        fks-=log((double)kk)/(2.0e0);
        i=0;
        j=1;
        while (j<mm)
        { /* select small primes */
            p=mip->PRIMES[++i];
            r=remain(D,p);
            if (spmd(r,(p-1)/2,p)<=1)
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

BOOL factored(long lptr,big T)
{ /* factor quadratic residue */
    BOOL facted;
    int i,j,r,st;  
    partial=FALSE;
    facted=FALSE;
    for (j=1;j<=mm;j++)
    { /* now attempt complete factorisation of T */
        r=(int)(lptr%epr[j]);
        if (r<0) r+=epr[j];
        if (r!=r1[j] && r!=r2[j]) continue;
        while (subdiv(T,epr[j],XX)==0)
        { /* cast out epr[j] */
            e[j]++;
            copy(XX,T);
        }
        st=size(T);
        if (st==1)
        {
           facted=TRUE;
           break;
        }
        if (size(XX)<=epr[j])
        { /* st is prime < epr[mm]^2 */
            if (st>=MR_TOOBIG || (st/epr[mm])>(1+mlf/50)) break;
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

BOOL gotcha(void)
{ /* use new factorisation */
    int r,j,i,k,n,rb,had,hp;
    unsigned int t;
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
    copy(PP,XX);
    convert(1,YY);
    for (k=1;k<=mm;k++)
    { /* build up square part in YY  *
       * reducing e[k] to 0s and 1s */
        if (e[k]<2) continue;
        r=e[k]/2;
        e[k]%=2;
        expint(epr[k],r,TT);
        multiply(TT,YY,YY);
    }
/* debug only
    cotnum(XX,stdout);
    cotnum(YY,stdout);
    if (e[0]==1) printf("-1");
    else printf("1");
    for (k=1;k<=mm;k++)
    {
        if (e[k]==0) continue;
        printf(".%d",epr[k]);
    }
    if (partial) printf(".%d\n",lp);
    else printf("\n");
*/
    if (partial)
    { /* factored with large prime */
        if (!found)
        { /* store new partial factorization */
            hash[had]=nlp;
            pr[nlp]=lp;
            copy(XX,z[nlp]);
            copy(YY,w[nlp]);
            for (n=0,rb=0,j=0;j<=mm;j++)
            {
                G[nlp][n]|=((e[j]&1)<<rb);
                if (++rb==nbts) n++,rb=0;
            }
            nlp++;
        }
        if (found)
        { /* match found so use as factorization */
            if (!suppress)
            {
                printf("\b\b\b\b\b\b*");
                fflush(stdout);
            }
            mad(XX,z[hp],XX,NN,NN,XX);
            mad(YY,w[hp],YY,NN,NN,YY);
            for (n=0,rb=0,j=0;j<=mm;j++)
            {
                t=(G[hp][n]>>rb);
                e[j]+=(t&1);
                if (e[j]==2)
                {
                    premult(YY,epr[j],YY);
                    divide(YY,NN,NN);
                    e[j]=0;
                }
                if (++rb==nbts) n++,rb=0;
            }
            premult(YY,lp,YY);
            divide(YY,NN,NN);
        }
    }
    else if (!suppress)
    {
        printf("\b\b\b\b\b\b ");
        fflush(stdout);
    }
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
            mad(XX,x[i],XX,NN,NN,XX);    /* This is very inefficient -  */
            mad(YY,y[i],YY,NN,NN,YY);    /* There must be a better way! */
            for (n=0,rb=0,j=0;j<=mm;j++)
            { /* Gaussian elimination */
                t=(EE[i][n]>>rb);
                e[j]+=(t&1);
                if (++rb==nbts) n++,rb=0;
            }
        }
        for (j=0;j<=mm;j++)
        { /* update YY */
            if (e[j]<2) continue;
            convert(epr[j],TT);
            power(TT,e[j]/2,NN,TT);
            mad(YY,TT,YY,NN,NN,YY);
        }
        if (!found)
        { /* store details in E, x and y for later */
            b[k]=jj;
            copy(XX,x[jj]);
            copy(YY,y[jj]);
            for (n=0,rb=0,j=0;j<=mm;j++)
            {
                EE[jj][n]|=((e[j]&1)<<rb);
                if (++rb==nbts) n++,rb=0;
            }
            jj++;
            if (!suppress) printf("%5d",jj);
        }
    }
    if (found)
    { /* check for false alarm */
        if (!suppress) printf("\ntrying...\n");
        add(XX,YY,TT);
        if (compare(XX,YY)==0 || compare(TT,NN)==0) found=FALSE;
        if (!found) if(!suppress) printf("working... %5d",jj);
    }
    return found;
}

int initv(int d)
{ /* initialize big numbers and arrays */
    int i,j,pak,k,maxp;
    double dp;

    NN=mirvar(0); 
    TT=mirvar(0);
    DD=mirvar(0);
    RR=mirvar(0);
    VV=mirvar(0);
    PP=mirvar(0);
    XX=mirvar(0);
    YY=mirvar(0);
    DG=mirvar(0);
    IG=mirvar(0);
    AA=mirvar(0);
    BB=mirvar(0);

    nbts=8*sizeof(int);

    copy(n,NN);

/* determine mm - optimal size of factor base */

    if (d<8) mm=d;
    else mm=25;
    if (d>20) mm=(d*d*d*d)/4096;

/* only half the primes (on average) wil be used, so generate twice as
   many (+ a bit for luck) */

    dp=(double)2*(double)(mm+100);  /* number of primes to generate */
    maxp=(int)(dp*(log(dp*log(dp)))); /* Rossers upper bound */
    gprime(maxp);

    epr=(int *)mr_alloc(mm+1,sizeof(int));
  
    k=knuth(mm,epr,NN,DD);

    if (nroot(DD,2,RR))
    {
        if (!suppress)
        {
            printf("%dN is a perfect square!\n",k);
            if (isprime(RR)) printf("PRIME FACTOR     ");
            else             printf("COMPOSITE FACTOR ");
        }
        else if (!isprime(RR)) printf("& ");
        cotnum(RR,output);
        divide(NN,RR,NN);
        if (!suppress)
        {
            if (isprime(NN)) printf("PRIME FACTOR     ");
            else             printf("COMPOSITE FACTOR ");
        }
        else if (!isprime(NN)) printf("& ");
        cotnum(NN,output);
        return (-1);
    }
    if(!suppress)
    {
        printf("using multiplier k= %d and %d small primes as factor base\n",k,mm);
    }
    gprime(0);   /* reclaim PRIMES space */

    mlf=2*mm;

/* now get space for arrays */

    r1=(int *)mr_alloc((mm+1),sizeof(int));
    r2=(int *)mr_alloc((mm+1),sizeof(int));
    rp=(int *)mr_alloc((mm+1),sizeof(int));
    b=(int *)mr_alloc((mm+1),sizeof(int));
    e=(int *)mr_alloc((mm+1),sizeof(int));

    logp=(unsigned char *)mr_alloc(mm+1,1);

    pr=(int *)mr_alloc((mlf+1),sizeof(int));
    hash=(int *)mr_alloc((2*mlf+1),sizeof(int));

    sieve=(unsigned char *)mr_alloc(SSIZE+1,1); 

    x=(big *)mr_alloc(mm+1,sizeof(big *));
    y=(big *)mr_alloc(mm+1,sizeof(big *));
    z=(big *)mr_alloc(mlf+1,sizeof(big *));
    w=(big *)mr_alloc(mlf+1,sizeof(big *));

    for (i=0;i<=mm;i++)
    {
        x[i]=mirvar(0);
        y[i]=mirvar(0);
    }
    for (i=0;i<=mlf;i++)
    {
        z[i]=mirvar(0);
        w[i]=mirvar(0);
    }

    EE=(unsigned int **)mr_alloc(mm+1,sizeof(int *));
    G=(unsigned int **)mr_alloc(mlf+1,sizeof(int *));
    
    pak=1+mm/(MR_IBITS);
    for (i=0;i<=mm;i++)
    { 
        b[i]=(-1);
        EE[i]=(unsigned int *)mr_alloc(pak,sizeof(int));
    }
    
    mip->ERCON=TRUE;
    for (i=0;i<=mlf;i++)
    {
        G[i]=(unsigned int *)mr_alloc(pak,sizeof(int));
        if (G[i]==NULL)
        { /* Out of space - try a quick fix */
            mlf=mm;
            for (j=mm+1;j<i;j++) mr_free(G[j]);
            break;
        }
       
    }
    mip->ERCON=FALSE;
    mip->ERNUM=0;
    return 1;
}
 
int qsieve(int d)
{ /* factoring via quadratic sieve */
    unsigned int i,j,a,*SV;
    unsigned char logpi;
    int k,S,r,s1,s2,s,NS,logm,ptr,threshold,epri;
    long M,la,lptr;

    if (initv(d)<0) exit(0);

    hmod=2*mlf+1;               /* set up hash table */
    convert(hmod,TT);
    while (!isprime(TT)) decr(TT,2,TT);
    hmod=size(TT);
    hmod2=hmod-2;
    for (k=0;k<hmod;k++) hash[k]=(-1);

    M=50*(long)mm;
    NS=(int)(M/SSIZE);
    if (M%SSIZE!=0) NS++;
    M=SSIZE*(long)NS;
    logm=0;
    la=M;
    while ((la/=2)>0) logm++;   /* logm = log(M) */
    rp[0]=logp[0]=0;
    for (k=1;k<=mm;k++)
    { /* find root mod each prime, and approx log of each prime */
        r=subdiv(DD,epr[k],TT);
        rp[k]=sqrmp(r,epr[k]);
        logp[k]=0;
        r=epr[k];
        while((r/=2)>0) logp[k]++;
    }
    r=subdiv(DD,8,TT);    /* take special care of 2 */
    if (r==5) logp[1]++;
    if (r==1) logp[1]+=2;

    threshold=logm+logb2(RR)-2*logp[mm];

    jj=0;
    nlp=0;
    premult(DD,2,DG);
    nroot(DG,2,DG);
    
    lgconv(M,TT);
    divide(DG,TT,DG);
    nroot(DG,2,DG);
    if (subdiv(DG,2,TT)==0) incr(DG,1,DG);
    if (subdiv(DG,4,TT)==1) incr(DG,2,DG);
    if (!suppress) printf("working...     0");
    forever
    { /* try a new polynomial */
        r=mip->NTRY;
        mip->NTRY=1;         /* speed up search for prime */
        do
        { /* looking for suitable prime DG = 3 mod 4 */
            do {
               incr(DG,4,DG);
            } while(!isprime(DG));
            decr(DG,1,TT);
            subdiv(TT,2,TT);
            powmod(DD,TT,DG,TT);  /* check D is quad residue */
        } while (size(TT)!=1);
        mip->NTRY=r;
        incr(DG,1,TT);
        subdiv(TT,4,TT);
        powmod(DD,TT,DG,BB);
        negify(DD,TT);
        mad(BB,BB,TT,DG,TT,TT);
        negify(TT,TT);
        premult(BB,2,AA);
        xgcd(AA,DG,AA,AA,AA);
        mad(AA,TT,TT,DG,DG,AA);
        multiply(AA,DG,TT);
        add(BB,TT,BB);        /* BB^2 = DD mod DG^2 */
        multiply(DG,DG,AA);   /* AA = DG*DG         */
        xgcd(DG,DD,IG,IG,IG); /* IG = 1/DG mod DD  */

        r1[0]=r2[0]=0;
        for (k=1;k<=mm;k++) 
        { /* find roots of quadratic mod each prime */
            s=subdiv(BB,epr[k],TT);
            r=subdiv(AA,epr[k],TT);
            r=invers(r,epr[k]);     /* r = 1/AA mod p */
            s1=(epr[k]-s+rp[k]);
            s2=(epr[k]-s+epr[k]-rp[k]);
            r1[k]=smul(s1,r,epr[k]);
            r2[k]=smul(s2,r,epr[k]);
        }

        for (ptr=(-NS);ptr<NS;ptr++)
        { /* sieve over next period */
            la=(long)ptr*SSIZE;
            SV=(unsigned int *)sieve;
            for (i=0;i<SSIZE/sizeof(int);i++) *SV++=0;
            for (k=1;k<=mm;k++)
            { /* sieving with each prime */
                epri=epr[k];
                logpi=logp[k];
                r=(int)(la%epr[k]);
                s1=(r1[k]-r)%epri;
                if (s1<0) s1+=epri;
                s2=(r2[k]-r)%epri;
                if (s2<0) s2+=epri;
            /* these loops are time-critical */
   
                for (j=s1;j<SSIZE;j+=epri) sieve[j]+=logpi;
                if (s1==s2) continue;
                for (j=s2;j<SSIZE;j+=epri) sieve[j]+=logpi;
            }

            for (a=0;a<SSIZE;a++)
            { /* main loop - look for fully factored residues */
                if (sieve[a]<threshold) continue;
                lptr=la+a;
                lgconv(lptr,TT);
                S=0;
                multiply(AA,TT,TT);           /* TT = AAx + BB      */
                add(TT,BB,TT);
                mad(TT,IG,TT,DD,DD,PP);       /* PP = (AAx + BB)/G  */
                if (size(PP)<0) add(PP,DD,PP);
                mad(PP,PP,PP,DD,DD,VV);       /* VV = PP^2 mod kN  */
                absol(TT,TT);
                if (compare(TT,RR)<0) S=1;    /* check for -ve VV */
                if (S==1) subtract(DD,VV,VV);
                copy(VV,TT);
                e[0]=S;
                for (k=1;k<=mm;k++) e[k]=0;
                if (!factored(lptr,TT)) continue;
                if (gotcha())
                { /* factors found! */
                    egcd(TT,NN,PP);
                    if (!suppress)
                    {
                        if (isprime(PP)) printf("PRIME FACTOR     ");
                        else             printf("COMPOSITE FACTOR ");
                    }
                    else if (!isprime(PP)) printf("& ");

                    cotnum(PP,output);
                    divide(NN,PP,NN);
                    if (!suppress)
                    {
                        if (isprime(NN)) printf("PRIME FACTOR     ");
                        else             printf("COMPOSITE FACTOR ");
                    }
                    else if (!isprime(NN)) printf("& ");

                    cotnum(NN,output);
                    exit(0);
                }
            }
        }
    }
}


/* Code to parse formula in command line
   This code isn't mine, but its public domain
   Shamefully I forget the source
 
  NOTE: It may be necessary on some platforms to change the operators * and #
*/

#if defined(unix)
#define TIMES '.'
#define RAISE '^'
#else
#define TIMES '*'
#define RAISE '#'
#endif

int digits(void)
{ /* size of n */
    int d;
    t=mirvar(0);
    d=0;
    copy(n,t);
    while (size(t)!=0)
    {
        subdiv(t,10,t);
        d++;
    }
    mr_free(t);
    return d;
}

static char *s;

void eval_power (big oldn,big n,char op)
{
        if (op) power(oldn,size(n),n,n);
}

void eval_product (big oldn,big n,char op)
{
        switch (op)
        {
        case TIMES:
                multiply(n,oldn,n);
                break;
        case '/':
                copy(oldn,t);
                divide(t,n,t);
                copy(t,n);
                break;
        case '%':
                copy(oldn,t);
                divide(t,n,n);
                copy(t,n);
        }
}

void eval_sum (big oldn,big n,char op)
{
        switch (op)
        {
        case '+':
                add(n,oldn,n);
                break;
        case '-':
                subtract(oldn,n,n);
        }
}

void eval (void)
{
        big oldn[3];
        big n;
        int i;
        char oldop[3];
        char op;
        char minus;
        n=mirvar(0);
        for (i=0;i<3;i++)
        {
            oldop[i]=0;
            oldn[i]=mirvar(0);
        }
LOOP:
        while (*s==' ')
        s++;
        if (*s=='-')    /* Unary minus */
        {
        s++;
        minus=1;
        }
        else
        minus=0;
        while (*s==' ')
        s++;
        if (*s=='(' || *s=='[' || *s=='{')    /* Number is subexpression */
        {
        s++;
        eval ();
        copy(t,n);     
        }
        else            /* Number is decimal value */
        {
        for (i=0;s[i]>='0' && s[i]<='9';i++)
                ;
        if (!i)         /* No digits found */
        {
                printf ("Error - invalid number\n");
                exit (20);
        }
        op=s[i];
        s[i]=0;
        lgconv(atol(s),n);
        s+=i;
        *s=op;
        }
        if (minus) negify(n,n);
        do
        op=*s++;
        while (op==' ');
        if (op==0 || op==')' || op==']' || op=='}')
        {
        eval_power (oldn[2],n,oldop[2]);
        eval_product (oldn[1],n,oldop[1]);
        eval_sum (oldn[0],n,oldop[0]);
        copy(n,t);
        mr_free(n);
        for (i=0;i<2;i++) mr_free(oldn[i]);
        return;
        }
        else
        {
        if (op==RAISE)
        {
                eval_power (oldn[2],n,oldop[2]);
                copy(n,oldn[2]);
                oldop[2]=RAISE;
        }
        else
        {
                if (op==TIMES || op=='/' || op=='%')
                {
                eval_power (oldn[2],n,oldop[2]);
                oldop[2]=0;
                eval_product (oldn[1],n,oldop[1]);
                copy(n,oldn[1]);
                oldop[1]=op;
                }
                else
                {
                if (op=='+' || op=='-')
                {
                        eval_power (oldn[2],n,oldop[2]);
                        oldop[2]=0;
                        eval_product (oldn[1],n,oldop[1]);
                        oldop[1]=0;
                        eval_sum (oldn[0],n,oldop[0]);
                        copy(n,oldn[0]);
                        oldop[0]=op;
                }
                else    /* Error - invalid operator */
                {
                        printf ("Error - invalid operator\n");
                        exit (20);
                }
                }
        }
        }
        goto LOOP;
}

int main(int argc,char **argv)
{
    FILE *ifile;
    int ip,b,d=250;
    argv++;argc--;
    if (argc<1)
    {
      printf("Incorrect Usage\n");
      printf("factor <number>\n");
      printf("OR\n");
      printf("factor -f <formula>\n");
      printf("e.g. factor 999999999999999999999999999999999999999999999999999997\n");
#if defined(unix)
      printf("or   factor -f 10^100-19\n\n");
#else
      printf("or   factor -f 10#100-19\n\n");
#endif
      printf("To suppress the commentary, use flag -s\n");
      printf("To input from a file, use flag -i <filename>\n");
      printf("To output to a file, use flag -o <filename>\n");
      printf("To set max. number size, set -dn, where n is number of decimal digits\n");
      printf("(Default is -d150). Must be first flag and before number.\n");
#if defined(unix)
      printf("e.g. factor -d200 -f 10^200-1 -s -o factors.dat\n\n");
#else
      printf("e.g. factor -d200 -f 10#200-1 -s -o factors.dat\n\n");
#endif
      printf("Freeware from Certivox, Dublin, Ireland\n");
      printf("Full C source code and MIRACL multiprecision library available\n");
      printf("Email to mscott@indigo.ie for details\n");
      return 0;
    }

    b=(d*45)/100;
#ifndef MR_NOFULLWIDTH
    mip=mirsys(-b,0);
#else
    mip=mirsys(-b,MAXBASE);
#endif

    mip->NTRY=100;
    n=mirvar(0);

    ip=0;
    output=stdout;

    while (ip<argc)
    {
        if (strncmp(argv[ip],"-d",2)==0)
        {
            d=atoi(argv[ip++]+2);
            mr_free(n);
            mirexit();
            if (d<20) d=20;
            b=(d*45)/100;
#ifndef MR_NOFULLWIDTH
            mip=mirsys(-b,0);
#else
            mip=mirsys(-b,MAXBASE);
#endif
            mip->NTRY=100;
            n=mirvar(0);
            continue;

        }
        if (strcmp(argv[ip],"-f")==0)
        {
            ip++;
            s=argv[ip++];
            t=mirvar(0);
            eval();
            copy(t,n);
            mr_free(t);
            cotnum(n,stdout);
            continue;
        }
        if (strcmp(argv[ip],"-s")==0)
        {
            ip++;
            suppress=TRUE;
            continue;
        }
        if (strcmp(argv[ip],"-i")==0)
        {
            ip++;
            ifile=fopen(argv[ip++],"rt");
            if (ifile==NULL) break;
            cinnum(n,ifile);
            cotnum(n,stdout);
            continue;
        }
        if (strcmp(argv[ip],"-o")==0)
        {
            ip++;
            output=fopen(argv[ip++],"wt");
            continue;
        }
        cinstr(n,argv[ip++]);
    }

    if (size(n)==0) 
    {
        printf("No number to factor!\n");
        return 0;
    }
    if (size(n)<0)
    {
        printf("Positive numbers only!\n");
        return 0;
    }
    if (isprime(n))
    {
        cotnum(n,output);
        printf("this number is prime!\n");
        return 0;
    }

    if (!suppress) printf("first trying brute force division by small primes\n");
    brute();
    if (!suppress) printf("now trying %d iterations of brent's method\n",BTRIES);
    brent();
    fu= (big *)mr_alloc((1+MULT/2),sizeof(big));
    cp=(BOOL *)mr_alloc((1+MULT/2),sizeof(BOOL));
    plus=(BOOL *)mr_alloc((1+MULT/2),sizeof(BOOL));
    minus=(BOOL *)mr_alloc((1+MULT/2),sizeof(BOOL));

    if (digits()>25)
    {
        if (!suppress) printf("now trying william's (p+1) method\n");
        williams(10000,1000000L,1);
        if (!suppress) printf("now trying pollard's (p-1) method\n");
        pollard(100000,5000000L);
    }
    if (digits()>35)
    {
        if (!suppress) printf("now trying lenstra's method using 10 curves\n");
        do_lenstra(20000,2000000L,10);
        if (digits()>64) 
        {
             if (!suppress) printf("now trying 80 more curves\n");
             do_lenstra(20000,2000000L,80);
        }
        if (digits()>72)
        {
             if (!suppress) printf("trying 300 last curves\n");
             do_lenstra(50000,5000000,300);
        } 
    }  

    mr_free(minus);
    mr_free(plus);
    mr_free(cp);
    mr_free(fu);
    if (digits()<100)
    {
        if (!suppress) printf("finally - the multiple polynomial quadratic sieve - with large prime (*)\n");
        qsieve(digits());
    }
    if (!suppress) printf("I give up          \nCOMPOSITE FACTOR ");
    else printf("& ");
    cotnum(n,output);
    return 0;
}

