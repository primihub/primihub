
/***************************************************************************
                                                                           *
Copyright 2013 CertiVox IOM Ltd.                                           *
                                                                           *
This file is part of CertiVox MIRACL Crypto SDK.                           *
                                                                           *
The CertiVox MIRACL Crypto SDK provides developers with an                 *
extensive and efficient set of cryptographic functions.                    *
For further information about its features and functionalities please      *
refer to http://www.certivox.com                                           *
                                                                           *
* The CertiVox MIRACL Crypto SDK is free software: you can                 *
  redistribute it and/or modify it under the terms of the                  *
  GNU Affero General Public License as published by the                    *
  Free Software Foundation, either version 3 of the License,               *
  or (at your option) any later version.                                   *
                                                                           *
* The CertiVox MIRACL Crypto SDK is distributed in the hope                *
  that it will be useful, but WITHOUT ANY WARRANTY; without even the       *
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. *
  See the GNU Affero General Public License for more details.              *
                                                                           *
* You should have received a copy of the GNU Affero General Public         *
  License along with CertiVox MIRACL Crypto SDK.                           *
  If not, see <http://www.gnu.org/licenses/>.                              *
                                                                           *
You can be released from the requirements of the license by purchasing     *
a commercial license. Buying such a license is mandatory as soon as you    *
develop commercial activities involving the CertiVox MIRACL Crypto SDK     *
without disclosing the source code of your own applications, or shipping   *
the CertiVox MIRACL Crypto SDK with a closed source product.               *
                                                                           *
***************************************************************************/
/*
 *   Benchmarking program for PK implementations
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "miracl.h"

/* define minimum duration of each timing, and min. number of iterations */

#define MIN_TIME 10.0
#define MIN_ITERS 20 

/* define fixed precomputation window size */

#define WINDOW 8

/* random "safe" primes - (p-1)/2 is also prime */

char p512[]="A89BmxRFLAnMTGV1EofBF3t9vxHwLw3upSiJQqGrSSJanNwAWm4qeIpR0QZos81Cb0T3GSB8Vvioo2ShdHeocZl";
char p1024[]="33pn5XYfRZ6oa1SgeSZ0gLXbIHYKsAL2vf2hMPp4BShBUUwVqJSaZMHBtYRr2C8CtD2ql3cKco8tsbol9KiiW0kmgYdmX2OYuDirwVHBXU6iarsuWLsFI8f9IcXF5mQUhhIfNL1UgB9iOopI4DZJdaAkweMrr0L7H6DTcJCv4uOG8l";
char p2048[]="9JhODtckdgHoisG3BF7icLO1W2kQN8uERdD45ta8ECK2pSl74xmjtptZhoFRXLCn8SHJtmwXTuf6aUbUUGsT6dE8GMWSkdg3qN4owcJE6wuCUiKKDOrsUEaFA6GCaSoHrHd6upEOTFJrSt5JZvvPUmZExbgTtVkZaM3EHVO5hhmaOglEXNmWbQlSZR57EPH4VS5nYPHsj3YEqtQjBxOg509VY3Efa3WCBXSILEksrBCdxBFeboPQ2ImO8gt52UX68ClTq4hUO7HltCJ8DEXT0QitGp5G39H3EGlBM7a1Pto1XRctShgDCJkKtedRvCTHJ81IaLUM2QRgVvY2oAUfU6DpqPl";


/* 160-bit Elliptic Curve A= -3 (1,y) is of prime order r wrt prime p */

char b160[]="547961736808018748879088091015409822321903727720";
char y160[]="1184021062507719516935416374276431034553065993786";
char p160[]="1243254415344564576487568858887587143562341624873";
char r160[]="1243254415344564576487570064860738948886682236669";


/* 192-bit Elliptic Curve A= -3 (1,y) is of prime order r wrt prime p */

char b192[]="4061049254666112630970447728594959377821841236338949398359";
char y192[]="939373580274738592696031201994651073677369517020051213856";
char p192[]="4361274637164371634176431764172114141371368173651736587859";
char r192[]="4361274637164371634176431764042976768701814568420333347189";

/* 224-bit Elliptic Curve A= -3 (1,y) is of prime order r wrt prime p */

char b224[]="17383927112623192126321700675122043803151281370446907580591543997888";
char y224[]="6566202929975094781252846334642707436688198986599754639429350077046";
char p224[]="26237462376427386428736423786423764364625346524653462546544347644653";
char r224[]="26237462376427386428736423786423773752689811507809031319417547459991";

/* 256-bit Elliptic Curve A= -3 (1,y) is of prime order r wrt prime p */

char b256[]="25389140340672155341527372976612393184553582461816899055687141548002290977046";
char y256[]="51289739734510562976895380525256763300476168821636300126346201758371757118206";
char p256[]="115324781748134865946503563657643838352352623747656242345890742746828256867467";
char r256[]="115324781748134865946503563657643838352221626521810006206950260876359658535911";

#ifndef MR_FP

/* Elliptic Curve wrt GF(2^163). This is NIST standard Curve */ 

int  A163=1;
char B163[]="20A601907B8C953CA1481EB10512F78744A3205FD";
char x163[]="3F0EBA16286A2D57EA0991168D4994637E8343E36";
char y163[]="D51FBC6C71A0094FA2CDD545B11C5C0C797324F1";
int  m163=163;
int  a163=7;
int  b163=6;
int  c163=3;
char r163[]="5846006549323611672814742442876390689256843201587";
int cf163=2;

/* Elliptic Curve wrt GF(2^163). NIST Koblitz Curve */

int KA163=1;
char KB163[]="1";
char Kx163[]="396C30B475EF87A2B37CA911D272DE90E109CA80F";
char Ky163[]="3947D0E4C8BB41DC3BABB142D2923A253D6E76391";

/* Elliptic Curve wrt GF(2^233). This is NIST standard Curve */ 

int  A233=1;
char B233[]="66647EDE6C332C7F8C0923BB58213B333B20E9CE4281FE115F7D8F90AD";
char x233[]="FAC9DFCBAC8313BB2139F1BB755FEF65BC391F8B36F8F8EB7371FD558B";
char y233[]="1006A08A41903350678E58528BEBF8A0BEFF867A7CA36716F7E01F81052";
int  m233=233;
int  a233=74;
int  b233=0;
int  c233=0;
char r233[]="6901746346790563787434755862277025555839812737345013555379383634485463";
int cf233=2;

/* Elliptic Curve wrt GF(2^233). This is NIST Koblitz Curve */ 

int  KA233=0;
char KB233[]="1";
char Kx233[]="17232ba853a7e731af129f22ff4149563a419c26bf50a4c9d6eefad6126";
char Ky233[]="1db537dece819b7f70f555a67c427a8cd9bf18aeb9b56e0c11056fae6a3";

/* Elliptic Curve wrt GF(2^283). This is NIST standard Curve */ 

int  A283=1;
char B283[]="27B680AC8B8596DA5A4AF8A19A0303FCA97FD7645309FA2A581485AF6263E313B79A2F5";
char x283[]="5F939258DB7DD90E1934F8C70B0DFEC2EED25B8557EAC9C80E2E198F8CDBECD86B12053";
char y283[]="3676854FE24141CB98FE6D4B20D02B4516FF702350EDDB0826779C813F0DF45BE8112F4";
int  m283=283;
int  a283=12;/* 119; these are faster.. */
int  b283=7; /* 97; */
int  c283=5; /* 93; */
char r283[]="7770675568902916283677847627294075626569625924376904889109196526770044277787378692871"; 
int cf283=2;

/* Elliptic Curve wrt GF(2^283). This is NIST Koblitz Curve */ 

int  KA283=0;
char KB283[]="1";
char Kx283[]="503213f78ca44883f1a3b8162f188e553cd265f23c1567a16876913b0c2ac2458492836";
char Ky283[]="1ccda380f1c9e318d90f95d07e5426fe87e45c0e8184698e45962364e34116177dd2259";

/* Elliptic Curve wrt GF(2^571). This is NIST standard Curve */

int  A571=1;
char B571[]="02F40E7E2221F295DE297117B7F3D62F5C6A97FFCB8CEFF1CD6BA8CE4A9A18AD84FFABBD8EFA59332BE7AD6756A66E294AFD185A78FF12AA520E4DE739BACA0C7FFEFF7F2955727A";
char x571[]="0303001D34B856296C16C0D40D3CD7750A93D1D2955FA80AA5F40FC8DB7B2ABDBDE53950F4C0D293CDD711A35B67FB1499AE60038614F1394ABFA3B4C850D927E1E7769C8EEC2D19";
char y571[]="037BF27342DA639B6DCCFFFEB73D69D78C6C27A6009CBBCA1980F8533921E8A684423E43BAB08A576291AF8F461BB2A8B3531D2F0485C19B16E2F1516E23DD3C1A4827AF1B8AC15B";
int  m571=571;
int  a571=10;
int  b571=5;
int  c571=2;
int  cf571=2;

/* Elliptic Curve wrt GF(2^571). This is NIST Koblitz Curve */ 

int  KA571=0;
char KB571[]="1";
char Kx571[]="026EB7A859923FBC82189631F8103FE4AC9CA2970012D5D46024804801841CA44370958493B205E647DA304DB4CEB08CBBD1BA39494776FB988B47174DCA88C7E2945283A01C8972";
char Ky571[]="0349DC807F4FBF374F4AEADE3BCA95314DD58CEC9F307A54FFC61EFC006D8A2C9D4979C0AC44AEA74FBEBBB9F772AEDCB620B01A7BA7AF1B320430C8591984F601CD4C143EF1C7A3";

#endif

void primemod(int bits,big p)
{
    do {
        printf("%d bit prime.....\n",bits);
        bigbits(bits,p);
        nxprime(p,p);
    } while (logb2(p)!=bits);
}

double powers(int gb,int eb,big p)
{
    int iterations=0;
    big g,e,w;
    clock_t start;
    double elapsed;
    char *mem;

    mem=(char *)memalloc(3);
    g=mirvar_mem(mem,0);
    e=mirvar_mem(mem,1);
    w=mirvar_mem(mem,2);

    bigbits(gb,g);
    bigbits(eb,e);
    start=clock();

    do {
       powmod(g,e,p,w);
       iterations++;
       elapsed=(clock()-start)/(double)CLOCKS_PER_SEC;
    } while (elapsed<MIN_TIME || iterations<MIN_ITERS);

    elapsed=1000.0*elapsed/iterations;
    printf("R - %8d iterations of %4d/%4d ",iterations,gb,eb);
    printf(" %8.2lf ms per iteration\n",elapsed);

    memkill(mem,3);
 
    return elapsed;
}

double mults(int eb,epoint *g)
{
    big e;
    int iterations=0;
    clock_t start;
    double elapsed;
    epoint *w,*r;
    char *mem1;
    char *mem2;

    mem1=(char *)memalloc(1);
    mem2=(char *)ecp_memalloc(2);

    e=mirvar_mem(mem1,0);
    w=epoint_init_mem(mem2,0);
    r=epoint_init_mem(mem2,1);
    
    bigbits(eb,e);
    ecurve_mult(e,g,r);   /* generate a random point on the curve */
    bigbits(eb,e);
    start=clock();

    do {
       ecurve_mult(e,r,w);
       iterations++;
       elapsed=(clock()-start)/(double)CLOCKS_PER_SEC;
    } while (elapsed<MIN_TIME || iterations<MIN_ITERS);

    elapsed=1000.0*elapsed/iterations;
    printf("ER - %8d iterations             ",iterations);
    printf(" %8.2lf ms per iteration\n",elapsed);

    memkill(mem1,1);
    ecp_memkill(mem2,2);
    return elapsed;
}

#ifndef MR_FP

double mults2(int eb,epoint *g)
{
    big e;
    int iterations=0;
    clock_t start;
    double elapsed;
    epoint *w;
    epoint *r;
    char *mem1;
    char *mem2;

    mem1=(char *)memalloc(1);
    mem2=(char *)ecp_memalloc(2);

    e=mirvar_mem(mem1,0);
    w=epoint_init_mem(mem2,0);
    r=epoint_init_mem(mem2,1);

    bigbits(eb,e);
    ecurve2_mult(e,g,r);   /* generate a random point on the curve */
    bigbits(eb,e);
    start=clock();

    do {
       ecurve2_mult(e,r,w);
       iterations++;
       elapsed=(clock()-start)/(double)CLOCKS_PER_SEC;
    } while (elapsed<MIN_TIME || iterations<MIN_ITERS);

    elapsed=1000.0*elapsed/iterations;
    printf("ER - %8d iterations             ",iterations);
    printf(" %8.2lf ms per iteration\n",elapsed);

    memkill(mem1,1);
    ecp_memkill(mem2,2);
    
    return elapsed;
}

#endif

double powers_small_base(int g,int eb,big p)
{
    int iterations=0;
    big e,w;
    clock_t start;
    double elapsed;
    char *mem;

    mem=(char *)memalloc(2);

    e=mirvar_mem(mem,0);
    w=mirvar_mem(mem,1);
    bigbits(eb,e);
    start=clock();

    do {
        powltr(g,e,p,w);
        iterations++;
        elapsed=(clock()-start)/(double)CLOCKS_PER_SEC;
    } while (elapsed<MIN_TIME || iterations<MIN_ITERS);

    elapsed=1000.0*elapsed/iterations;
    printf("S - %8d iterations of  g=%d/%4d ",iterations,g,eb);
    printf(" %8.2lf ms per iteration\n",elapsed);

    memkill(mem,2);
    return elapsed;
}

double powers_double(int gb,int eb,big p)
{
    int iterations=0;
    clock_t start;
    double elapsed;
    big g1,e1,g2,e2,w;
    char *mem;

    mem=(char *)memalloc(5);
    g1=mirvar_mem(mem,0);
    e1=mirvar_mem(mem,1);
    g2=mirvar_mem(mem,2);
    e2=mirvar_mem(mem,3);
    w=mirvar_mem(mem,4);
    bigbits(gb,g1);
    bigbits(gb,g2);
    bigbits(eb,e1);
    bigbits(eb,e2);
    start=clock();
    do {
        powmod2(g1,e1,g2,e2,p,w);
        iterations++;
        elapsed=(clock()-start)/(double)CLOCKS_PER_SEC;
    } while (elapsed<MIN_TIME || iterations<MIN_ITERS);

    elapsed=1000.0*elapsed/iterations;
    printf("D - %8d iterations of %4d/%4d ",iterations,gb,eb);
    printf(" %8.2lf ms per iteration\n",elapsed);

    memkill(mem,4);

    return elapsed;
}

double mult_double(int eb,epoint *g)
{
    big e1,e2;
    int iterations=0;
    clock_t start;
    double elapsed;
    char *mem1;
    char *mem2;
    epoint *w;
    epoint *r1;
    epoint *r2;

    mem1=(char *)memalloc(2);
    mem2=(char *)ecp_memalloc(3);

    e1=mirvar_mem(mem1,0);
    e2=mirvar_mem(mem1,1);
    w=epoint_init_mem(mem2,0);
    r1=epoint_init_mem(mem2,1);
    r2=epoint_init_mem(mem2,2);

    bigbits(eb,e1);
    ecurve_mult(e1,g,r1);   /* generate a random point on the curve */
    bigbits(eb,e2);
    ecurve_mult(e2,g,r2);   /* generate a random point on the curve */
    bigbits(eb,e1);
    bigbits(eb,e2);
    start=clock();

    do {
       ecurve_mult2(e1,r1,e2,r2,w);
       iterations++;
       elapsed=(clock()-start)/(double)CLOCKS_PER_SEC;
    } while (elapsed<MIN_TIME || iterations<MIN_ITERS);

    elapsed=1000.0*elapsed/iterations;
    printf("ED - %8d iterations             ",iterations);
    printf(" %8.2lf ms per iteration\n",elapsed);

    ecp_memkill(mem2,3);
    memkill(mem1,2);

    return elapsed;
}

#ifndef MR_FP

double mult2_double(int eb,epoint *g)
{
    big e1,e2;
    int iterations=0;
    clock_t start;
    double elapsed;
    char *mem1;
    char *mem2;
    epoint *w;
    epoint *r1;
    epoint *r2;

    mem1=(char *)memalloc(2);
    mem2=(char *)ecp_memalloc(3);

    e1=mirvar_mem(mem1,0);
    e2=mirvar_mem(mem1,1);
    w=epoint_init_mem(mem2,0);
    r1=epoint_init_mem(mem2,1);
    r2=epoint_init_mem(mem2,2);

    bigbits(eb,e1);
    ecurve2_mult(e1,g,r1);   /* generate a random point on the curve */
    bigbits(eb,e2);
    ecurve2_mult(e2,g,r2);   /* generate a random point on the curve */
    bigbits(eb,e1);
    bigbits(eb,e2);
    start=clock();

    do {
       ecurve2_mult2(e1,r1,e2,r2,w);
       iterations++;
       elapsed=(clock()-start)/(double)CLOCKS_PER_SEC;
    } while (elapsed<MIN_TIME || iterations<MIN_ITERS);

    elapsed=1000.0*elapsed/iterations;
    printf("ED - %8d iterations             ",iterations);
    printf(" %8.2lf ms per iteration\n",elapsed);

    ecp_memkill(mem2,3);
    memkill(mem1,2);

    return elapsed;
}

#endif

double powers_precomp(int gb,int eb,big p)
{
    int iterations=0;
    clock_t start;
    double elapsed;
    brick binst;
    big g,e,w;
    char *mem;

    mem=(char *)memalloc(3);
    g=mirvar_mem(mem,0);
    e=mirvar_mem(mem,1);
    w=mirvar_mem(mem,2);
    bigbits(gb,g);
   
    brick_init(&binst,g,p,WINDOW,eb);
 
    bigbits(eb,e);

    start=clock();
    do {
        pow_brick(&binst,e,w);
        iterations++;
        elapsed=(clock()-start)/(double)CLOCKS_PER_SEC;
    } while (elapsed<MIN_TIME || iterations<MIN_ITERS);

    elapsed=1000.0*elapsed/iterations;
    printf("P - %8d iterations of %4d/%4d ",iterations,gb,eb);
    printf(" %8.2lf ms per iteration\n",elapsed);

    brick_end(&binst);
   
    memkill(mem,3);

    return elapsed;
}

double mult_precomp(int eb,big x,big y,big a,big b,big p)
{
    big e,c,d;
    int iterations=0;
    ebrick binst;
    clock_t start;
    double elapsed;
    char *mem;

    mem=(char *)memalloc(3);
    e=mirvar_mem(mem,0);
    c=mirvar_mem(mem,1);
    d=mirvar_mem(mem,2);
    ebrick_init(&binst,x,y,a,b,p,WINDOW,eb);
    bigbits(eb,e);
    start=clock();

    do {
       mul_brick(&binst,e,c,d);
       iterations++;
       elapsed=(clock()-start)/(double)CLOCKS_PER_SEC;
    } while (elapsed<MIN_TIME || iterations<MIN_ITERS);

    elapsed=1000.0*elapsed/iterations;
    printf("EP - %8d iterations             ",iterations);
    printf(" %8.2lf ms per iteration\n",elapsed);

    ebrick_end(&binst);
    memkill(mem,3);
   
    return elapsed;
}

#ifndef MR_FP

double mult2_precomp(int eb,big x,big y,big a2,big a6,int M,int A,int B,int C)
{
    big e,c,d;
    int iterations=0;
    ebrick2 binst;
    clock_t start;
    double elapsed;
    char *mem;

    mem=(char *)memalloc(3);
    e=mirvar_mem(mem,0);
    c=mirvar_mem(mem,1);
    d=mirvar_mem(mem,2);
    ebrick2_init(&binst,x,y,a2,a6,M,A,B,C,WINDOW,eb);
    bigbits(eb,e);
    start=clock();

    do {
       mul2_brick(&binst,e,c,d);
       iterations++;
       elapsed=(clock()-start)/(double)CLOCKS_PER_SEC;
    } while (elapsed<MIN_TIME || iterations<MIN_ITERS);

    elapsed=1000.0*elapsed/iterations;
    printf("EP - %8d iterations             ",iterations);
    printf(" %8.2lf ms per iteration\n",elapsed);

    ebrick2_end(&binst);
    memkill(mem,3);
   
    return elapsed;
}

#endif

double powers_small_exp(int gb,long ex,big p)
{
    int iterations=0;
    big g,e,w;
    clock_t start;
    double elapsed;
    char *mem;

    mem=(char *)memalloc(3);
    g=mirvar_mem(mem,0);
    e=mirvar_mem(mem,1);
    w=mirvar_mem(mem,2);
    bigbits(gb,g);
    start=clock();
    lgconv(ex,e);
    do {
       power(g,ex,p,w);
       iterations++;
       elapsed=(clock()-start)/(double)CLOCKS_PER_SEC;
    } while (elapsed<MIN_TIME || iterations<MIN_ITERS);

    elapsed=1000.0*elapsed/iterations;
    if (ex==257L)
        printf("V - %8d iterations of %4d/e=F3 ",iterations,gb);
    if (ex==65537L)
        printf("V - %8d iterations of %4d/e=F4 ",iterations,gb);
    if (ex!=257L && ex!=65537L)
        printf("V - %8d iterations of %4d/e=%2ld ",iterations,gb,ex);
    printf(" %8.2lf ms per iteration\n",elapsed);
    memkill(mem,3);
   
    return elapsed;
}

int main()
{
    int j,k;
    big a,b,x,y,p,A2;
    time_t seed;
    epoint *g;
    double tr1,tr2,ts,tv1,tv2,tp,td;
#ifndef MR_NOFULLWIDTH
    miracl *mip=mirsys(300,0);
#else
    miracl *mip=mirsys(300,MAXBASE);
#endif
    p=mirvar(0);
    a=mirvar(-3);
    b=mirvar(0);
    x=mirvar(1);
    y=mirvar(0);
    A2=mirvar(0);
    mip->IOBASE=60;

    time(&seed);
    irand((long)seed);

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
    printf("Precomputation uses fixed Window size = %d\n",WINDOW);
    printf("So %d values are precomputed and stored\n",(1<<WINDOW));
#ifdef MR_NOFULLWIDTH
    printf("No Fullwidth base possible\n");
#else
    printf("NOTE: No optimizations/assembly language apply to GF(2^m) Elliptic Curves\n");
#endif

    printf("NOTE: times are elapsed real-times - so make sure nothing else is running!\n\n");
    printf("Modular exponentiation benchmarks - calculating g^e mod p\n");
    printf("From these figures it should be possible to roughly estimate the time\n");
    printf("required for your favourite PK algorithm, RSA, DSA, DH, etc.\n");
    printf("Key R - random base bits/random exponent bits \n");
    printf("    V - random base bits/(small exponent e)   \n");
    printf("    S - (small base g)  /random exponent bits \n");
    printf("    P - exponentiation with precomputation (fixed base g)\n");
    printf("    D - double exponentiation g^e.a^b mod p\n");
   
    printf("F3 = 257, F4 = 65537\n");
    printf("RSA - Rivest-Shamir-Adleman\n");
    printf("DH  - Diffie Hellman Key exchange\n");
    printf("DSA - Digital Signature Algorithm\n");

    printf("\n512 bit prime....\n");
    cinstr(p,p512);

    k=512;
    j=160;

    tr1=powers(k,j,p);
    td=powers_double(k,j,p);
    tr2=powers(k,k,p);
    ts=powers_small_base(3,j,p);
    tp=powers_precomp(k,j,p);

    printf("\n");
    printf("%4d bit RSA decryption               %8.2lf ms \n",2*k,2*tr2);
    printf("%4d bit DH %d bit exponent:-\n",k,j);
    printf("         offline, no precomputation   %8.2lf ms \n",tr1);
    printf("         offline, small base          %8.2lf ms \n",ts);
    printf("         offline, w. precomputation   %8.2lf ms \n",tp);
    printf("         online                       %8.2lf ms \n",tr1);                           
    printf("%4d bit DSA %d bit exponent:-\n",k,j);
    printf("         signature no precomputation  %8.2lf ms \n",tr1);
    printf("         signature w. precomputation  %8.2lf ms \n",tp);
    printf("         verification                 %8.2lf ms \n",td);
          
    printf("\n1024 bit prime....\n");
    cinstr(p,p1024);        

    k=1024; j=160;
    tr1=powers(k,j,p);
    td=powers_double(k,j,p);
    tr2=powers(k,k,p);
    tv1=powers_small_exp(k,3,p);
    tv2=powers_small_exp(k,65537L,p);
    ts=powers_small_base(3,j,p);
    tp=powers_precomp(k,j,p);

    printf("\n");
    printf("%4d bit RSA decryption               %8.2lf ms \n",2*k,2*tr2);
    printf("%4d bit RSA encryption e=3           %8.2lf ms \n",k,tv1);
    printf("%4d bit RSA encryption e=65537       %8.2lf ms \n",k,tv2);
    printf("%4d bit DH %d bit exponent:-\n",k,j);
    printf("         offline, no precomputation   %8.2lf ms \n",tr1);
    printf("         offline, small base          %8.2lf ms \n",ts);
    printf("         offline, w. precomputation   %8.2lf ms \n",tp);
    printf("         online                       %8.2lf ms \n",tr1);                           
    printf("%4d bit DSA %d bit exponent:-\n",k,j);
    printf("         signature no precomputation  %8.2lf ms \n",tr1);
    printf("         signature w. precomputation  %8.2lf ms \n",tp);
    printf("         verification                 %8.2lf ms \n",td);

    printf("\n2048 bit prime....\n");
    cinstr(p,p2048);

    k=2048; j=256;

    tr1=powers(k,j,p);
    td=powers_double(k,j,p);
    powers(k,k,p);
    tv1=powers_small_exp(k,3,p);
    tv2=powers_small_exp(k,65537L,p);
    ts=powers_small_base(3,j,p);
    tp=powers_precomp(k,j,p);

    printf("\n");
    printf("%4d bit RSA encryption e=3           %8.2lf ms \n",k,tv1);
    printf("%4d bit RSA encryption e=65537       %8.2lf ms \n",k,tv2);
    printf("%4d bit DH %d bit exponent:-\n",k,j);
    printf("         offline, no precomputation   %8.2lf ms \n",tr1);
    printf("         offline, small base          %8.2lf ms \n",ts);
    printf("         offline, w. precomputation   %8.2lf ms \n",tp);
    printf("         online                       %8.2lf ms \n",tr1);                           
    printf("%4d bit DSA %d bit exponent:-\n",k,j);
    printf("         signature no precomputation  %8.2lf ms \n",tr1);
    printf("         signature w. precomputation  %8.2lf ms \n",tp);
    printf("         verification                 %8.2lf ms \n",td);
  
    printf("\n");
    printf("Elliptic Curve point multiplication benchmarks - calculating r.P\n");
    printf("From these figures it should be possible to roughly estimate the time\n");
    printf("required for your favourite EC PK algorithm, ECDSA, ECDH, etc.\n");
    printf("Key - ER - Elliptic Curve point multiplication r.P\n");
    printf("      ED - Elliptic Curve double multiplication r.P + s.Q\n");
    printf("      EP - Elliptic Curve multiplication with precomputation\n");
    printf("EC    - Elliptic curve GF(p) - p of no special form \n");
    printf("ECDH  - Diffie Hellman Key exchange\n");
    printf("ECDSA - Digital Signature Algorithm\n");

    mip->IOBASE=10;

    printf("\n160 bit GF(p) Elliptic Curve....\n");
    k=160;
    cinstr(p,p160);
    cinstr(b,b160);
    cinstr(y,y160);

    ecurve_init(a,b,p,MR_PROJECTIVE);
    g=epoint_init();
    if (!epoint_set(x,y,0,g))
    {
        printf("This is not a point on the curve!\n");
        exit(0);
    }

    tr1=mults(k,g);
    td=mult_double(k,g);
    tp=mult_precomp(k,x,y,a,b,p);

    printf("\n");
    printf("%4d bit ECDH :-\n",k);
    printf("         offline, no precomputation   %8.2lf ms \n",tr1);
    printf("         offline, w. precomputation   %8.2lf ms \n",tp);
    printf("         online                       %8.2lf ms \n",tr1);                           
    printf("%4d bit ECDSA :-\n",k);
    printf("         signature no precomputation  %8.2lf ms \n",tr1);
    printf("         signature w. precomputation  %8.2lf ms \n",tp);
    printf("         verification                 %8.2lf ms \n",td);

    printf("\n192 bit GF(p) Elliptic Curve....\n");
    k=192;
    cinstr(p,p192);
    cinstr(b,b192);
    cinstr(y,y192);

    ecurve_init(a,b,p,MR_PROJECTIVE);
    g=epoint_init();
    if (!epoint_set(x,y,0,g))
    {            
        printf("This is not a point on the curve!\n");
        exit(0);
    }


    tr1=mults(k,g);
    td=mult_double(k,g);
    tp=mult_precomp(k,x,y,a,b,p);

    printf("\n");
    printf("%4d bit ECDH :-\n",k);
    printf("         offline, no precomputation   %8.2lf ms \n",tr1);
    printf("         offline, w. precomputation   %8.2lf ms \n",tp);
    printf("         online                       %8.2lf ms \n",tr1);                           
    printf("%4d bit ECDSA :-\n",k);
    printf("         signature no precomputation  %8.2lf ms \n",tr1);
    printf("         signature w. precomputation  %8.2lf ms \n",tp);
    printf("         verification                 %8.2lf ms \n",td);

    printf("\n224 bit GF(p) Elliptic Curve....\n");
    k=224;
    cinstr(p,p224);
    cinstr(b,b224);
    cinstr(y,y224);

    ecurve_init(a,b,p,MR_PROJECTIVE);
    g=epoint_init();
    if (!epoint_set(x,y,0,g))
    {            
        printf("This is not a point on the curve!\n");
        exit(0);
    }

    tr1=mults(k,g);
    td=mult_double(k,g);
    tp=mult_precomp(k,x,y,a,b,p);

    printf("\n");
    printf("%4d bit ECDH :-\n",k);
    printf("         offline, no precomputation   %8.2lf ms \n",tr1);
    printf("         offline, w. precomputation   %8.2lf ms \n",tp);
    printf("         online                       %8.2lf ms \n",tr1);                           
    printf("%4d bit ECDSA :-\n",k);
    printf("         signature no precomputation  %8.2lf ms \n",tr1);
    printf("         signature w. precomputation  %8.2lf ms \n",tp);
    printf("         verification                 %8.2lf ms \n",td);

    printf("\n256 bit GF(p) Elliptic Curve....\n");
    k=256;
    cinstr(p,p256);
    cinstr(b,b256);
    cinstr(y,y256);

    ecurve_init(a,b,p,MR_PROJECTIVE);
    g=epoint_init();
    if (!epoint_set(x,y,0,g))
    {            
        printf("This is not a point on the curve!\n");
        exit(0);
    }

    tr1=mults(k,g);
    td=mult_double(k,g);
    tp=mult_precomp(k,x,y,a,b,p);

    printf("\n");
    printf("%4d bit ECDH :-\n",k);
    printf("         offline, no precomputation   %8.2lf ms \n",tr1);
    printf("         offline, w. precomputation   %8.2lf ms \n",tp);
    printf("         online                       %8.2lf ms \n",tr1);                           
    printf("%4d bit ECDSA :-\n",k);
    printf("         signature no precomputation  %8.2lf ms \n",tr1);
    printf("         signature w. precomputation  %8.2lf ms \n",tp);
    printf("         verification                 %8.2lf ms \n",td);

#ifndef MR_FP

    printf("\n163 bit GF(2^m) Elliptic Curve....\n");
    k=163;
    mip->IOBASE=16;
    cinstr(b,B163);
    cinstr(x,x163);
    cinstr(y,y163);
    mip->IOBASE=10;
    convert(A163,A2);
    ecurve2_init(m163,a163,b163,c163,A2,b,FALSE,MR_PROJECTIVE);
    g=epoint_init();
    if (!epoint2_set(x,y,0,g))
    {            
        printf("This is not a point on the curve!\n");
        exit(0);
    }

    tr1=mults2(k,g);
    td=mult2_double(k,g);
    tp=mult2_precomp(k,x,y,A2,b,m163,a163,b163,c163);

    printf("\n");
    printf("%4d bit ECDH :-\n",k);
    printf("         offline, no precomputation   %8.2lf ms \n",tr1);
    printf("         offline, w. precomputation   %8.2lf ms \n",tp);
    printf("         online                       %8.2lf ms \n",tr1);                           
    printf("%4d bit ECDSA :-\n",k);
    printf("         signature no precomputation  %8.2lf ms \n",tr1);
    printf("         signature w. precomputation  %8.2lf ms \n",tp);
    printf("         verification                 %8.2lf ms \n",td);

    printf("\n163 bit GF(2^m) Koblitz Elliptic Curve....\n");
    k=163;
    mip->IOBASE=16;
    cinstr(b,KB163);
    cinstr(x,Kx163);
    cinstr(y,Ky163);
    mip->IOBASE=10;
    convert(KA163,A2);
    ecurve2_init(m163,a163,b163,c163,A2,b,FALSE,MR_PROJECTIVE);
    g=epoint_init();
    if (!epoint2_set(x,y,0,g))
    {            
        printf("This is not a point on the curve!\n");
        exit(0);
    }

    tr1=mults2(k,g);
    td=mult2_double(k,g);
    tp=mult2_precomp(k,x,y,A2,b,m163,a163,b163,c163);

    printf("\n");
    printf("%4d bit ECDH :-\n",k);
    printf("         offline, no precomputation   %8.2lf ms \n",tr1);
    printf("         offline, w. precomputation   %8.2lf ms \n",tp);
    printf("         online                       %8.2lf ms \n",tr1);                           
    printf("%4d bit ECDSA :-\n",k);
    printf("         signature no precomputation  %8.2lf ms \n",tr1);
    printf("         signature w. precomputation  %8.2lf ms \n",tp);
    printf("         verification                 %8.2lf ms \n",td);

    printf("\n233 bit GF(2^m) Elliptic Curve....\n");
    k=233;
    mip->IOBASE=16;
    cinstr(b,B233);
    cinstr(x,x233);
    cinstr(y,y233);
    mip->IOBASE=10;
    convert(A233,A2);
    ecurve2_init(m233,a233,b233,c233,A2,b,FALSE,MR_PROJECTIVE);
    g=epoint_init();
    if (!epoint2_set(x,y,0,g))
    {            
        printf("This is not a point on the curve!\n");
        exit(0);
    }

    tr1=mults2(k,g);
    td=mult2_double(k,g);
    tp=mult2_precomp(k,x,y,A2,b,m233,a233,b233,c233);

    printf("\n");
    printf("%4d bit ECDH :-\n",k);
    printf("         offline, no precomputation   %8.2lf ms \n",tr1);
    printf("         offline, w. precomputation   %8.2lf ms \n",tp);
    printf("         online                       %8.2lf ms \n",tr1);                           
    printf("%4d bit ECDSA :-\n",k);
    printf("         signature no precomputation  %8.2lf ms \n",tr1);
    printf("         signature w. precomputation  %8.2lf ms \n",tp);
    printf("         verification                 %8.2lf ms \n",td);

    printf("\n233 bit GF(2^m) Koblitz Elliptic Curve....\n");
    k=233;
    mip->IOBASE=16;
    cinstr(b,KB233);
    cinstr(x,Kx233);
    cinstr(y,Ky233);
    mip->IOBASE=10;
    convert(KA233,A2);
    ecurve2_init(m233,a233,b233,c233,A2,b,FALSE,MR_PROJECTIVE);
    g=epoint_init();
    if (!epoint2_set(x,y,0,g))
    {            
        printf("This is not a point on the curve!\n");
        exit(0);
    }

    tr1=mults2(k,g);
    td=mult2_double(k,g);
    tp=mult2_precomp(k,x,y,A2,b,m233,a233,b233,c233);

    printf("\n");
    printf("%4d bit ECDH :-\n",k);
    printf("         offline, no precomputation   %8.2lf ms \n",tr1);
    printf("         offline, w. precomputation   %8.2lf ms \n",tp);
    printf("         online                       %8.2lf ms \n",tr1);                           
    printf("%4d bit ECDSA :-\n",k);
    printf("         signature no precomputation  %8.2lf ms \n",tr1);
    printf("         signature w. precomputation  %8.2lf ms \n",tp);
    printf("         verification                 %8.2lf ms \n",td);


    printf("\n283 bit GF(2^m) Elliptic Curve....\n");
    k=283;
    mip->IOBASE=16;
    cinstr(b,B283);
    cinstr(x,x283);
    cinstr(y,y283);
    mip->IOBASE=10;

    convert(A283,A2);
    ecurve2_init(m283,a283,b283,c283,A2,b,FALSE,MR_PROJECTIVE);
    g=epoint_init();
    if (!epoint2_set(x,y,0,g))
    {            
        printf("This is not a point on the curve!\n");
        exit(0);
    }

    tr1=mults2(k,g);
    td=mult2_double(k,g);
    tp=mult2_precomp(k,x,y,A2,b,m283,a283,b283,c283);

    printf("\n");
    printf("%4d bit ECDH :-\n",k);
    printf("         offline, no precomputation   %8.2lf ms \n",tr1);
    printf("         offline, w. precomputation   %8.2lf ms \n",tp);
    printf("         online                       %8.2lf ms \n",tr1);                           
    printf("%4d bit ECDSA :-\n",k);
    printf("         signature no precomputation  %8.2lf ms \n",tr1);
    printf("         signature w. precomputation  %8.2lf ms \n",tp);
    printf("         verification                 %8.2lf ms \n",td);

    printf("\n283 bit GF(2^m) Koblitz Elliptic Curve....\n");
    k=283;
    mip->IOBASE=16;
    cinstr(b,KB283);
    cinstr(x,Kx283);
    cinstr(y,Ky283);
    mip->IOBASE=10;

    convert(KA283,A2);
    ecurve2_init(m283,a283,b283,c283,A2,b,FALSE,MR_PROJECTIVE);
    g=epoint_init();
    if (!epoint2_set(x,y,0,g))
    {            
        printf("This is not a point on the curve!\n");
        exit(0);
    }

    tr1=mults2(k,g);
    td=mult2_double(k,g);
    tp=mult2_precomp(k,x,y,A2,b,m283,a283,b283,c283);

    printf("\n");
    printf("%4d bit ECDH :-\n",k);
    printf("         offline, no precomputation   %8.2lf ms \n",tr1);
    printf("         offline, w. precomputation   %8.2lf ms \n",tp);
    printf("         online                       %8.2lf ms \n",tr1);                           
    printf("%4d bit ECDSA :-\n",k);
    printf("         signature no precomputation  %8.2lf ms \n",tr1);
    printf("         signature w. precomputation  %8.2lf ms \n",tp);
    printf("         verification                 %8.2lf ms \n",td);

    printf("\n571 bit GF(2^m) Elliptic Curve....\n");
    k=571;
    mip->IOBASE=16;
    cinstr(b,B571);
    cinstr(x,x571);
    cinstr(y,y571);
    mip->IOBASE=10;

    convert(A571,A2);
    ecurve2_init(m571,a571,b571,c571,A2,b,FALSE,MR_PROJECTIVE);
    g=epoint_init();
    if (!epoint2_set(x,y,0,g))
    {            
        printf("This is not a point on the curve!\n");
        exit(0);
    }

    tr1=mults2(k,g);
    td=mult2_double(k,g);
    tp=mult2_precomp(k,x,y,A2,b,m571,a571,b571,c571);

    printf("\n");
    printf("%4d bit ECDH :-\n",k);
    printf("         offline, no precomputation   %8.2lf ms \n",tr1);
    printf("         offline, w. precomputation   %8.2lf ms \n",tp);
    printf("         online                       %8.2lf ms \n",tr1);                           
    printf("%4d bit ECDSA :-\n",k);
    printf("         signature no precomputation  %8.2lf ms \n",tr1);
    printf("         signature w. precomputation  %8.2lf ms \n",tp);
    printf("         verification                 %8.2lf ms \n",td);

    printf("\n571 bit GF(2^m) Koblitz Elliptic Curve....\n");
    k=571;
    mip->IOBASE=16;
    cinstr(b,KB571);
    cinstr(x,Kx571);
    cinstr(y,Ky571);
    mip->IOBASE=10;

    convert(KA571,A2);
    ecurve2_init(m571,a571,b571,c571,A2,b,FALSE,MR_PROJECTIVE);
    g=epoint_init();
    if (!epoint2_set(x,y,0,g))
    {            
        printf("This is not a point on the curve!\n");
        exit(0);
    }

    tr1=mults2(k,g);
    td=mult2_double(k,g);
    tp=mult2_precomp(k,x,y,A2,b,m571,a571,b571,c571);

    printf("\n");
    printf("%4d bit ECDH :-\n",k);
    printf("         offline, no precomputation   %8.2lf ms \n",tr1);
    printf("         offline, w. precomputation   %8.2lf ms \n",tp);
    printf("         online                       %8.2lf ms \n",tr1);                           
    printf("%4d bit ECDSA :-\n",k);
    printf("         signature no precomputation  %8.2lf ms \n",tr1);
    printf("         signature w. precomputation  %8.2lf ms \n",tp);
    printf("         verification                 %8.2lf ms \n",td);

#endif
    return 0;
}

