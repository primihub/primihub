/*
 * program to calculate modquare/modmult, inverse/modmult,
 * jacobi/modmult and modadd/modmult ratios
 */

#include <stdio.h>
#include "miracl.h"
#include <time.h>

#define MIN_TIME 15.0

int sizes[]={160,256,512,640};
int num_sizes=4;

int main()
{
    time_t seed;
    int i,j,k,bits;
    long iterations;
    big x,nx,y,ny,n,w;
    clock_t start;
    double square_time,mult_time,xgcd_time,jac_time,add_time;

#ifndef MR_NOFULLWIDTH
    mirsys(80,0);
#else
    mirsys(80,MAXBASE);
#endif
    x=mirvar(0);
    nx=mirvar(0);
    y=mirvar(0);
    ny=mirvar(0);
    n=mirvar(0);
    w=mirvar(0);
    
    printf("MIRACL - %d bit version\n",MIRACL);
#ifdef MR_LITTLE_ENDIAN
    printf("Little Endian processor\n");
#endif
#ifdef MR_BIG_ENDIAN
    printf("Big Endian processor\n");
#endif
#ifdef MR_NOASM
    printf("C-Only Version of MIRACL\n");
#else
    printf("Using some assembly language\n");
#endif
#ifdef MR_STRIPPED_DOWN
    printf("Stripped down version of MIRACL - no error messages\n");
#endif
#ifdef MR_KCM
    k=MR_KCM*MIRACL;
    printf("Using KCM method \n");
    printf("Optimized for %d, %d, %d, %d...etc. bit moduli\n",k,k*2,k*4,k*8);
#endif
#ifdef MR_COMBA
    k=MR_COMBA*MIRACL;
    printf("Using COMBA method \n");
    printf("Optimized for %d bit moduli\n",k);
#endif
#ifdef MR_PENTIUM
    printf("Floating-point co-processor arithmetic used for Pentium\n");
#endif
#ifndef MR_KCM
#ifndef MR_COMBA
#ifndef MR_PENTIUM
    printf("No special optimizations\n");
#endif
#endif
#endif

#ifdef MR_NOFULLWIDTH
    printf("No Fullwidth base possible\n");
#endif

    printf("NOTE: times are elapsed real-times - so make sure nothing else is running!\n\n");

    time(&seed);
    irand((unsigned long)seed);
    printf("Calculating Modsquare/Modmult, Inverse/Modmult, Jacobi/Modmult Modadd/Modmult ratios\n");
    printf("Please Wait......\n");
    for (j=0;j<num_sizes;j++)
    {
        bits=sizes[j];
        bigbits(bits,n);
        if (remain(n,2)==0) incr(n,1,n);
        bigrand(n,x);
        bigrand(n,y);
        while (egcd(x,n,w)!=1) incr(n,2,n);
        prepare_monty(n);
        nres(x,nx); nres(y,ny);

        iterations=0;
        start=clock();
        do {
            for (i=0;i<1000;i++) nres_modmult(nx,ny,w);
            iterations++;
            mult_time=(clock()-start)/(double)CLOCKS_PER_SEC;
        } while (mult_time<MIN_TIME);
/* printf("before mult_time= %lf\n",mult_time); */
        mult_time=1000.0*mult_time/iterations;
/*
printf("iterations= %d\n",iterations);
printf("after mult_time= %lf\n",mult_time);
*/

        iterations=0;
        start=clock();
        do {
            for (i=0;i<1000;i++) nres_modmult(nx,nx,w);
            iterations++;
            square_time=(clock()-start)/(double)CLOCKS_PER_SEC;
        } while (square_time<MIN_TIME);
        square_time=1000.0*square_time/iterations;

        iterations=0;
        start=clock();
        do {
            for (i=0;i<1000;i++) jack(x,n);
            iterations++;
            jac_time=(clock()-start)/(double)CLOCKS_PER_SEC;
        } while (jac_time<MIN_TIME);
        jac_time=1000.0*jac_time/iterations;

        iterations=0;
        start=clock();
        do {
            for (i=0;i<1000;i++) xgcd(x,n,w,w,w);
            iterations++;
            xgcd_time=(clock()-start)/(double)CLOCKS_PER_SEC;
        } while (xgcd_time<MIN_TIME);
        xgcd_time=1000.0*xgcd_time/iterations;
  
        iterations=0;
        start=clock();
        do {
            for (i=0;i<1000;i++) nres_modadd(nx,ny,w);
            iterations++;
            add_time=(clock()-start)/(double)CLOCKS_PER_SEC;
        } while (add_time<MIN_TIME);
        add_time=1000.0*add_time/iterations;
 		
        printf("%d bits: modmult %.2lfuS modsqr %.2lfuS inverse %.2lfuS jacobi %.2lfuS modadd %.2lfuS \n",bits,mult_time,square_time,xgcd_time,jac_time,add_time);

        printf("S/M ratio= %.2lf\n",square_time/mult_time);
        printf("I/M ratio= %.2lf\n",xgcd_time/mult_time);
        printf("J/M ratio= %.2lf\n",jac_time/mult_time);
        printf("A/M ratio= %.2lf\n",add_time/mult_time);

    }

    return 0;
}

