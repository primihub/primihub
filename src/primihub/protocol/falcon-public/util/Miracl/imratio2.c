/*
 * program to calculate Square/Multiply, Sqrt/Multiply and Inverse/Multiply ratio
 * for fields F(2^m). Note that Square/Multiply can be improved by
 * optimizing the reduce2(.) in mrgf2m.c for a particular field. 
 * See the comments in the function.
 */

#include <stdio.h>
#include "miracl.h"
#include <time.h>

#define MIN_TIME 10.0

int  m[]={103,163,233,283,313,379,571};
int  a[]={9,   99,159,249,121,317,507};
int  b[]={0,   97,  0,219,  0,315,475};
int  c[]={0,    3,  0, 27,  0,283,417};

int main()
{
    time_t seed;
    int i,j,k,bits;
    long iterations;
    big x,y,w;
    clock_t start;
    double square_time,sqrt_time,mult_time,inverse_time;

#ifndef MR_NOFULLWIDTH
    mirsys(80,0);
#else
    mirsys(80,MAXBASE);
#endif
    x=mirvar(0);
    y=mirvar(0);
    w=mirvar(0);

    printf("MIRACL - %d bit version\n",MIRACL);
#ifdef MR_LITTLE_ENDIAN
    printf("Little Endian processor\n");
#endif
#ifdef MR_BIG_ENDIAN
    printf("Big Endian processor\n");
#endif

    printf("C-Only Version of MIRACL\n");

#ifdef MR_STRIPPED_DOWN
    printf("Stripped down version of MIRACL - no error messages\n");
#endif

    printf("NOTE: times are elapsed real-times - so make sure nothing else is running!\n\n");

    time(&seed);
    irand((unsigned long)seed);
    printf("Calculating Square/Multiply (S/M), Square Root/Multiply (R/M) and Inverse/Multiply (I/M) ratios\n");
    printf("Please Wait......\n");
 
    for (j=0;j<7;j++)
    {
        bits=m[j];
        if (!prepare_basis(m[j],a[j],b[j],c[j],TRUE))
        {
            printf("Problem\n");
            return 0;
        }
        rand2(x);
        rand2(y);

        iterations=0;
        start=clock();
        do {
            for (i=0;i<1000;i++) modmult2(x,y,w);
            iterations++;
            mult_time=(clock()-start)/(double)CLOCKS_PER_SEC;
        } while (mult_time<MIN_TIME);

        mult_time=1000.0*mult_time/iterations;

        iterations=0;
        start=clock();
        do {
            for (i=0;i<1000;i++) modsquare2(x,w);
            iterations++;
            square_time=(clock()-start)/(double)CLOCKS_PER_SEC;
        } while (square_time<MIN_TIME);
        square_time=1000.0*square_time/iterations;

        iterations=0;
        start=clock();
        do {
            for (i=0;i<1000;i++) sqroot2(x,w);
            iterations++;
            sqrt_time=(clock()-start)/(double)CLOCKS_PER_SEC;
        } while (sqrt_time<MIN_TIME);
        sqrt_time=1000.0*sqrt_time/iterations;

        iterations=0;
        start=clock();
        do {
            for (i=0;i<1000;i++) inverse2(x,w);
            iterations++;
            inverse_time=(clock()-start)/(double)CLOCKS_PER_SEC;
        } while (inverse_time<MIN_TIME);
        inverse_time=1000.0*inverse_time/iterations;
  
     
        printf("%d bits: multiply %.2lfuS square %.2lfuS sqrt %.2lfuS inverse %.2lfuS\n",bits,mult_time,square_time,sqrt_time,inverse_time);

        printf("S/M ratio= %.2lf\n",square_time/mult_time);
        printf("R/M ratio= %.2lf\n",sqrt_time/mult_time);
        printf("I/M ratio= %.2lf\n",inverse_time/mult_time);
    }
    return 0;
}

