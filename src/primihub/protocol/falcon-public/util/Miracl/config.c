
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
 *   MIRACL utility program to automatically generate a mirdef.h file 
 *
 *   config.c
 *
 *   Compile WITHOUT any optimization
 *
 *   Run this with your computer/compiler configuration. It will
 *   attempt to create best-fit mirdef.h file for compiling the MIRACL 
 *   library. 
 *
 *   Generated are mirdef.tst - the suggested mirdef.h file, and 
 *   miracl.lst which tells which modules should be included in the library 
 *   build.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int answer(void)
{
    char ch;
    scanf("%c",&ch);
    getchar();
    if (ch=='Y' || ch=='y') return 1;
    return 0;
}

int main()
{
    int chosen,chosen2,utlen,dllen,flsh,stripped,standard,nofull,port,mant,r,userlen;
    int nbits,i,b,dlong,threaded,choice,selected,special,sb,ab;
    int chosen64,no64,step_size,double_type,rounding,lmant,dmant,static_build,maxsize;
    int maxbase,bitsinchar,llsize,nio,edwards,double_length_type;
    int qlong,qllen;
    long end;
    unsigned char byte;
    char *ptr;
    char longlong[20];
    double s,magic;
    double x,y,eps;
    long double lx,ly,leps;
    FILE *fp,*fpl;

    bitsinchar=0;
    byte=1;
    while (byte!=0) {byte<<=1; bitsinchar++;}

    printf("This program attempts to generate a mirdef.h file from your\n");
    printf("responses to some simple questions, and by some internal tests\n");
    printf("of its own.\n\n");
    printf("The most fundamental decision is that of the 'underlying type'\n");
    printf("that is the C data type to be used to store each digit of a\n");
    printf("big number. You input the number of bits in this type, and \n");
    printf("this program finds a suitable candidate (usually short, int or \n");
    printf("long). Typical values would be 16, 32 or perhaps 64. \n");
    printf("The bigger the better, but a good starting point would be to\n");
    printf("enter the native wordlength of your computer\n\n");
    printf("For your information:-\n");
    printf("The size of a char  is %d bits\n",bitsinchar);
    printf("The size of a short is %ld bits\n",bitsinchar*sizeof(short));
    printf("The size of an int  is %ld bits\n",bitsinchar*sizeof(int));
    printf("The size of a long  is %ld bits\n",bitsinchar*sizeof(long));

    eps=1.0;
    for (dmant=0;;dmant++)
    { /* IMPORTANT TO FOOL OPTIMIZER!!!!!! */
        x=1.0+eps;
        y=1.0;
        if (x==y) break;
        eps/=2.0;
    }

    printf("The size of a double is %ld bits, its mantissa is %d bits\n",bitsinchar*sizeof(double),dmant);

    leps=1.0; 
    for (lmant=0;;lmant++) 
    { /* IMPORTANT TO FOOL OPTIMIZER!!!!!! */ 
        lx=1.0+leps; 
        ly=1.0; 
        if (lx==ly) break; 
        leps/=2.0; 
    } 
    printf("The size of a long double is %ld bits, its mantissa is %d bits\n",bitsinchar*sizeof(long double),lmant);


    fp=fopen("mirdef.tst","wt");
    fprintf(fp,"/*\n *   MIRACL compiler/hardware definitions - mirdef.h\n */\n");
    end=1;
    ptr=(char *)(&end);

    if ((*ptr) == 1)
    {
         printf("\n    Little-endian processor detected\n\n");
         fprintf(fp,"#define MR_LITTLE_ENDIAN\n");
    }
    else 
    {
         printf("    Big-endian processor detected\n\n");
         fprintf(fp,"#define MR_BIG_ENDIAN\n");

    }
    printf("A double can be used as the underlying type. In rare circumstances\n");
    printf(" this may be optimal. NOT recommended!\n\n");
    
    printf("Do you wish to use a double as the underlying type? (Y/N)?");
    double_type=answer();
    nofull=0;
    chosen=0;
    chosen64=0;
    llsize=-1; /* no check for long long yet */
    no64=0;
    if (double_type)
    {
      fprintf(fp,"#define MIRACL %ld\n",bitsinchar*sizeof(double));
      fprintf(fp,"#define mr_utype double\n");
      fprintf(fp,"#define mr_dltype long double\n");
      fprintf(fp,"#define MR_NOFULLWIDTH\n");
      fprintf(fp,"#define MR_FP\n");
      nofull=1;
    }
    else
    {
      printf("\nNow enter number of bits in underlying type= ");
      scanf("%d",&utlen);
      getchar();
      printf("\n");
      if (utlen!=8 && utlen!=16 && utlen!=32 && utlen!=64) 
      {
        printf("Non-standard system - this program cannot help!\n");
        fclose(fp);
        exit(0);
      }

      fprintf(fp,"#define MIRACL %d\n",utlen);

      if (utlen==bitsinchar)
      {
        printf("    underlying type is a char\n");
        fprintf(fp,"#define mr_utype char\n");
        chosen=1;
      }
      if (!chosen && utlen==bitsinchar*sizeof(short))
      {
        printf("    underlying type is a short\n");
        fprintf(fp,"#define mr_utype short\n");
        chosen=1;
      }
      if (!chosen && utlen==bitsinchar*sizeof(int))
      {
        printf("    underlying type is an int\n");
        fprintf(fp,"#define mr_utype int\n");
        chosen=1;
      }  
      if (!chosen && utlen==bitsinchar*sizeof(long))
      {
        printf("    underlying type is a long\n");
        fprintf(fp,"#define mr_utype long\n");
        chosen=1;
      }
      dlong=0;
      if (!chosen && utlen==2*bitsinchar*sizeof(long))
      {
        printf("\nDoes compiler support a %d bit unsigned type? (Y/N)?", 
              utlen);
        dlong=answer();
        if (dlong)
        {
            llsize=utlen;  /* there is a long long */
            printf("Is it called long long? (Y/N)?");
            if (!answer())
            {
                printf("Enter its name : ");
                scanf("%s",longlong);
                getchar();
            }
            else strcpy(longlong,"long long");
            printf("    underlying type is a %s\n",longlong);
            fprintf(fp,"#define mr_utype %s\n",longlong);
            chosen=1;
            if (64==utlen && !chosen64)
            { 
                printf("    64 bit unsigned type is an unsigned %s\n",longlong);
                fprintf(fp,"#define mr_unsign64 unsigned %s\n",longlong);
                chosen64=1;
            }
        }
        else
        {
            llsize=0; /* there is no long long */
            if (utlen==64) no64=1; /* there is no 64-bit type */
        }
      }

      if (!chosen)
      {
        printf("No suitable underlying type could be found\n");
        fclose(fp);
        exit(0);
      }
    }

    fprintf(fp,"#define MR_IBITS %ld\n",bitsinchar*sizeof(int));
    fprintf(fp,"#define MR_LBITS %ld\n",bitsinchar*sizeof(long)); 

/* Now try to find 32-bit unsigned types */

    chosen=0;
    if (32==bitsinchar*sizeof(unsigned short))
    {
        printf("    32 bit unsigned type is a unsigned short\n");
        fprintf(fp,"#define mr_unsign32 unsigned short\n");
        chosen=1;
    }
    if (!chosen && 32==bitsinchar*sizeof(unsigned int))
    {
        printf("    32 bit unsigned type is an unsigned int\n");
        fprintf(fp,"#define mr_unsign32 unsigned int\n");
        chosen=1;
    }
    if (!chosen && 32==bitsinchar*sizeof(long))
    {
        printf("    32 bit unsigned type is an unsigned long\n");
        fprintf(fp,"#define mr_unsign32 unsigned long\n");
        chosen=1;
    }

    if (!chosen)
    {
        printf("No suitable 32 bit unsigned type could be found\n");
        fclose(fp);
        exit(0);
    }

/* are some of the built-in types 64-bit? */

    if (64==bitsinchar*sizeof(int))
    {
        printf("    64 bit unsigned type is an unsigned int\n");
        fprintf(fp,"#define mr_unsign64 unsigned int\n");
        chosen64=1;
    }
    if (!chosen64 && 64==bitsinchar*sizeof(long))
    {
        printf("    64 bit unsigned type is an unsigned long\n");
        fprintf(fp,"#define mr_unsign64 unsigned long\n");
        chosen64=1;
    }
    if (!chosen64 && llsize>0 && 64==llsize)
    {
        printf("    64 bit unsigned type is a %s\n",longlong);
        fprintf(fp,"#define mr_unsign64 %s\n",longlong);
        chosen64=1;
    }

/* Now try to find a double length type */
    dlong=0;
	dllen=0;
	double_length_type=0;
    if (!double_type)
    {
      dllen=2*utlen;
      chosen=0;
      if (!chosen && dllen==bitsinchar*sizeof(short))
      {
        printf("    double length type is a short\n");
        fprintf(fp,"#define mr_dltype short\n");
		double_length_type=1;
        if (sizeof(short)==sizeof(int)) fprintf(fp,"#define MR_DLTYPE_IS_INT\n");
        chosen=1;
      }
      if (!chosen && dllen==bitsinchar*sizeof(int))
      {
        printf("    double length type is an int\n");
        fprintf(fp,"#define mr_dltype int\n");
		double_length_type=1;
        fprintf(fp,"#define MR_DLTYPE_IS_INT\n");
        chosen=1;
      }
      if (!chosen && dllen==bitsinchar*sizeof(long))
      {
        printf("    double length type is a long\n");
        fprintf(fp,"#define mr_dltype long\n");
		double_length_type=1;
        fprintf(fp,"#define MR_DLTYPE_IS_LONG\n");
        chosen=1;
      }
      if (!chosen && llsize>0 && dllen==llsize)
      {
           printf("    double length type is a %s\n",longlong);
           fprintf(fp,"#define mr_dltype %s\n",longlong);
		   double_length_type=1;
           chosen=1;
      }
      if (!chosen && dllen==2*bitsinchar*sizeof(long))
      {
        printf("\nDoes compiler support a %d bit integer type? (Y/N)?", 
              dllen);
        dlong=answer();
       
        if (dlong)
        {
            printf("Is it called long long? (Y/N)?");
            if (!answer())
            {
                printf("Enter its name : ");
                scanf("%s",longlong);
                getchar();
            }
            else strcpy(longlong,"long long");

            printf("    double length type is a %s\n",longlong);
            fprintf(fp,"#define mr_dltype %s\n",longlong);
			double_length_type=1;
            chosen=1;
            if (64==dllen && !chosen64)
            { 
                printf("    64 bit unsigned type is an unsigned %s\n",longlong);
                fprintf(fp,"#define mr_unsign64 unsigned %s\n",longlong);
                chosen64=1;
            }
        }
        else
        {
            if (dllen==64) no64=1; /* there is no 64-bit type */
        }
      }
    }
    else dlong=1;

/* Now try to find a double length type */
    qlong=0;

    if (!double_type)
    {
      qllen=4*utlen;
      chosen2=0;

      if (!chosen2 && qllen==bitsinchar*sizeof(long))
      {
        printf("    quad length type is a long\n");
        fprintf(fp,"#define mr_qltype long\n");
        chosen2=1;
      }
      if (!chosen2 && llsize>0 && dllen==llsize)
      {
           printf("    quad length type is a %s\n",longlong);
           fprintf(fp,"#define mr_qltype %s\n",longlong);
           chosen2=1;
      }
      if (!chosen2 && qllen==2*bitsinchar*sizeof(long))
      {
        printf("\nDoes compiler support a %d bit integer type? (Y/N)?", 
              qllen);
        qlong=answer();
       
        if (qlong)
        {
            printf("Is it called long long? (Y/N)?");
            if (!answer())
            {
                printf("Enter its name : ");
                scanf("%s",longlong);
                getchar();
            }
            else strcpy(longlong,"long long");

            printf("    quad length type is a %s\n",longlong);
            fprintf(fp,"#define mr_qltype %s\n",longlong);
            chosen2=1;
            if (64==qllen && !chosen64)
            { 
                printf("    64 bit unsigned type is an unsigned %s\n",longlong);
                fprintf(fp,"#define mr_unsign64 unsigned %s\n",longlong);
                chosen64=1;
            }
        }
        else
        {
            if (qllen==64) no64=1; /* there is no 64-bit type */
        }
      }
    }
    else qlong=1;


    if (!no64 && !chosen64)
    { /* Still no 64-bit type, but not ruled out either */
        printf("\nDoes compiler support a 64-bit integer type? (Y/N)?");
        dlong=answer();
       
        if (dlong)
        {
            printf("Is it called long long? (Y/N)?");
            if (!answer())
            {
                printf("Enter its name : ");
                scanf("%s",longlong);
                getchar();
            }
            else strcpy(longlong,"long long");

            printf("    64-bit type is a %s\n",longlong);
            fprintf(fp,"#define mr_unsign64 unsigned %s\n",longlong);
            chosen64=1;
        }
    }

    fpl=fopen("miracl.lst","wt");
    ab=0;
    static_build=0;
    if (!double_type)
    {
        printf("\nFor very constrained environments it is possible to build a version of MIRACL\n");
        printf("which does not require a heap. Not recommended for beginners.\n");
        printf("Some routines are not available in this mode and the max length of Big\n");
        printf("variables is fixed at compile time\n");
        printf("Do you want a no-heap version of the MIRACL C library? (Y/N)? ");
        static_build=answer();
        if (static_build)
        {
            printf("Please enter the max size of your Big numbers in bits= ");
            scanf("%d",&nbits);
            getchar();
            maxsize=nbits/utlen;
            if ((nbits%utlen)!=0) maxsize++;
            fprintf(fp,"#define MR_STATIC %d\n",maxsize);
            fprintf(fp,"#define MR_ALWAYS_BINARY\n");
            ab=1;
        }
    }
    fprintf(fpl,"mrcore.c\n");
    fprintf(fpl,"mrarth0.c\n");
    fprintf(fpl,"mrarth1.c\n");
    fprintf(fpl,"mrarth2.c\n");
    fprintf(fpl,"mrsmall.c\n");
    fprintf(fpl,"mrio1.c\n");
    fprintf(fpl,"mrio2.c\n");
    fprintf(fpl,"mrgcd.c\n");
    fprintf(fpl,"mrjack.c\n");
    fprintf(fpl,"mrxgcd.c\n");
    fprintf(fpl,"mrarth3.c\n");
    fprintf(fpl,"mrbits.c\n");
    fprintf(fpl,"mrrand.c\n");
    fprintf(fpl,"mrprime.c\n");
    fprintf(fpl,"mrcrt.c\n");
    fprintf(fpl,"mrscrt.c\n");
    fprintf(fpl,"mrmonty.c\n");
    fprintf(fpl,"mrpower.c\n");
    fprintf(fpl,"mrsroot.c\n");
    fprintf(fpl,"mrlucas.c\n");
    fprintf(fpl,"mrshs.c\n");
    fprintf(fpl,"mrshs256.c\n");
    fprintf(fpl,"mraes.c\n");
	fprintf(fpl,"mrgcm.c\n");
    fprintf(fpl,"mrstrong.c\n");
    fprintf(fpl,"mrcurve.c\n");
    fprintf(fpl,"mrbrick.c\n");
    fprintf(fpl,"mrebrick.c\n");
    fprintf(fpl,"mrzzn2.c\n");
    fprintf(fpl,"mrzzn2b.c\n");
    fprintf(fpl,"mrzzn3.c\n");
    fprintf(fpl,"mrecn2.c\n");

    if (!static_build)
    {
        fprintf(fpl,"mrfast.c\n");
        fprintf(fpl,"mralloc.c\n");
    }
    if (chosen64) fprintf(fpl,"mrshs512.c\n");

    port=0;
    if (chosen && double_length_type)
    {
        printf("\nDo you want a C-only version of MIRACL (Y/N)?");
        port=answer();
        if (port) fprintf(fp,"#define MR_NOASM\n");
    }
    rounding=0;
    if (double_type)
    {
#ifdef __TURBOC__
        rounding=1;
#endif        
#ifdef _MSC_VER
        rounding=1;
#endif        
#ifdef __GNUC__
        rounding=1;
#endif        
        if (!rounding)
        {
          printf("It will help if rounding control can be exercised on doubles\n");
          printf("Can you implement this in mrarth1.c??  (Y/N)?");
          if (answer()) rounding=1;
        }
        if (rounding) 
        {
            fprintf(fp,"#define MR_FP_ROUNDING\n");
            magic=1.0;
            for (i=0;i<lmant-2;i++) magic*=2.0;
            magic+=2*magic;
            fprintf(fp,"#define MR_MAGIC %lf\n",magic);
        } 
    }

    printf("\nDo you want support for flash arithmetic? (Y/N)?");
    flsh=answer();
    if (flsh)
    { /* calculate size of mantissa in bits */
        eps=1.0;
        for (mant=0;;mant++)
        { /* IMPORTANT TO FOOL OPTIMIZER!!!!!! */
            x=1.0+eps;
            y=1.0;
            if (x==y) break;
            eps/=2.0;
        }
        mant--;
        fprintf(fp,"#define MR_FLASH %d\n",mant);  

        fprintf(fpl,"mrflash.c\n");
        fprintf(fpl,"mrfrnd.c\n");
        fprintf(fpl,"mrdouble.c\n");
        fprintf(fpl,"mrround.c\n");
        fprintf(fpl,"mrbuild.c\n");
        fprintf(fpl,"mrflsh1.c\n");
        fprintf(fpl,"mrpi.c\n");
        fprintf(fpl,"mrflsh2.c\n");
        fprintf(fpl,"mrflsh3.c\n");
        fprintf(fpl,"mrflsh4.c\n");

    }

    printf("Do you want stripped-down version (smaller - no error messages) (Y/N)?");
    stripped=answer();
    if (stripped) fprintf(fp,"#define MR_STRIPPED_DOWN\n");
    
     printf("Do you want multi-threaded version of MIRACL\n");
     printf("Not recommended for program development - read the manual (Y/N)?");
     threaded=answer();

     if (threaded)
     {
      printf("Do you want generic portable threading support (C only - No C++) (Y/N)?");
      choice=answer();
      if (choice) fprintf(fp,"#define MR_GENERIC_MT\n");
      if (!choice && !static_build) 
      {
          printf("Do you want multi-threaded support for C++ in MS Windows (Y/N)?");
          choice=answer();
          if (choice) fprintf(fp,"#define MR_WINDOWS_MT\n");
      }
      if (!choice && !static_build)
      {
          printf("Do you want multi-threaded support for C++ in Unix (Y/N)?");
          choice=answer();
          if (choice) fprintf(fp,"#define MR_UNIX_MT\n");
      }
      if (!choice && !static_build)
      {
          printf("Do you want multi-threaded support for C++ using openMP (Y/N)?");
          choice=answer();
          if (choice) fprintf(fp,"#define MR_OPENMP_MT\n");
      }
     }
    
    nio=1;
    printf("Does your development environment support standard screen/keyboard I/O?\n");
    printf("(It doesn't for example in MS Windows, and embedded applications)\n");
    printf("If in doubt, answer Yes (Y/N)?");
    standard=answer();
    if (!standard) fprintf(fp,"#define MR_NO_STANDARD_IO\n");
    else nio=0;

    printf("Does your development environment support standard file I/O?\n");
    printf("(It doesn't for example in an embedded application)\n");
    printf("If in doubt, answer Yes (Y/N)?");
    standard=answer();
    if (!standard) fprintf(fp,"#define MR_NO_FILE_IO\n");
    else nio=0;

    if (!static_build && !nofull)
    {
        printf("\n\nDo you for some reason NOT want to use a full-width number base?\n");
        printf("\nYou may not if your processor instruction set does not support\n");
        printf("%d-bit UNSIGNED multiply and divide instructions.\n",utlen);
        printf("If NOT then a full-width number base will be difficult and \n");
        printf("slow to implement, which is a pity, because its normally faster\n"); 
        printf("If for some other reason you don't want to use a full-width\n");
        printf("number base, (abnormal handling of integer overflow or no muldvd()\n");
        printf("/muldvd2()/muldvm() available?), answer Yes\n");
        printf("If in doubt answer No\n");
        printf("\nAnswer (Y/N)?");
        nofull=answer();
        if (nofull) 
        {
            printf("\nRemember to use mirsys(...,MAXBASE), or somesuch, in your programs\n");
            printf("as mirsys(...,0); will generate an 'Illegal Number base' error\n");
            fprintf(fp,"#define MR_NOFULLWIDTH\n");
        }
    }

    if (!static_build)
    {
     printf("\nAlways using a power-of-2 (or 0) as a number base reduces code space\n");
     printf("and will also be a little faster. This is recommended.\n"); 
     printf("\nWill all of your programs use a power-of-2 as a number base (Y/N)?");
     if (answer())
     {
         fprintf(fp,"#define MR_ALWAYS_BINARY\n");
         ab=1;
     }
    }

    selected=special=0;
    if (!nofull)
    {
     fprintf(fpl,"mrec2m.c\n");
     fprintf(fpl,"mrgf2m.c\n");

     printf("\nDo you want to create a Comba fixed size multiplier\n");
     printf("for binary polynomial multiplication. This requires that\n");
     printf("your processor supports a special binary multiplication instruction\n");
     printf("which it almost certainly does not....\n");
     printf("Useful particularly for Elliptic Curve cryptosystems over GF(2^m).\n");
     printf("\nDefault to No. Answer (Y/N)?");  
     r=answer();
     if (r)
     {
      step_size=0;
      while (step_size<2 || step_size>32)
      {
         printf("Enter field size in bits = ");
         scanf("%d",&nbits);
         getchar();
         step_size=nbits/utlen;
         if ((nbits%utlen)!=0) step_size++;
      }

      printf("\nTo create the file MRCOMBA2.C you must next execute\n");
      if (port) printf("MEX %d C MRCOMBA2\n",step_size);
      else
      {
       printf("MEX %d <file> MRCOMBA2\n",step_size);
       printf("where <file> is the name of the macro .mcs file (e.g. smartmips)\n");
      }

      fprintf(fp,"#define MR_COMBA2 %d\n",step_size);
      fprintf(fpl,"mrcomba2.c\n");

      printf("\nSpecial routines for polynomial multiplication will now \n");
      printf("automatically be invoked \n");
     }
     fprintf(fp,"#define MAXBASE ((mr_small)1<<(MIRACL-1))\n");
    }
    else
    {
     if (!double_type) fprintf(fp,"#define MAXBASE ((mr_small)1<<(MIRACL-2))\n");
    }

     
     printf("\nDo you wish to use the Karatsuba/Comba/Montgomery method\n");
     printf("for modular arithmetic - as used by exponentiation\n");
     printf("cryptosystems like RSA.\n"); 
     if (port)
     {
      printf("This method may be faster than the standard method when\n");
      printf("using larger moduli, or if your processor has no \n");
      printf("unsigned integer multiply/divide instruction in its\n");
      printf("instruction set. This is true of some popular RISC computers\n");
     }
     else
     {
      printf("This method is probably fastest om most processors which\n");
      printf("which support unsigned mul and a carry flag\n");
      printf("NOTE: your compiler must support in-line assembly,\n");
      printf("and you must be able to supply a suitable .mcs file\n");
      printf("like, for example, ms86.mcs for pentium processors\n");
     }
     printf("\nAnswer (Y/N)?");
     r=answer();
     if (r)
     {
      printf("\nThis method can only be used with moduli whose length in\n");
      printf("bits can be represented as %d*(step size)*2^n, for any value\n",utlen);
      printf("of n. For example if you input a step size of 8, then \n");
      printf("moduli of 256, 512, 1024 bits etc will use this fast code\n");
      if (nofull) printf("(assuming a full-length base)\n");
      
      if (port)
       printf("In this case case a step size of about 4 is probably optimal\n");
      else
      {
       printf("The best step size can be determined by experiment, but\n");
       printf("larger step sizes generate more code. For the Pentium 8 is \n"); 
       printf("about optimal. For the Pentium Pro/Pentium II 16 is better.\n"); 
       if (!nofull) printf("If in doubt, set to 8\n"); 
      }
      step_size=0;
      while (step_size<2 || step_size>16)
      {
          printf("Enter step size = ");
          scanf("%d",&step_size);
          getchar();
      }

      printf("\nTo create the file MRKCM.C you must next execute\n");
      if (port) printf("MEX %d C MRKCM\n",step_size);
      else
      {
       printf("MEX %d <file> MRKCM\n",step_size);
       printf("where <file> is the name of the macro .mcs file (e.g. ms86)\n");
      }

      printf("\nSpecial routines for modular multiplication will now be\n");
      printf("automatically be invoked when, for example, powmod() is called\n");
      printf("\nRemember to use a full-width base in your programs\n");
      printf("by calling mirsys(..,0) or mirsys(..,256) at the start of the program\n");
      
      fprintf(fp,"#define MR_KCM %d\n",step_size);          
      fprintf(fpl,"mrkcm.c\n");
      selected=1;
     }
     else
     {
      printf("\nDo you want to create a Comba fixed size modular\n");
      printf("multiplier, for faster modular multiplication with\n");
      printf("smaller moduli. Can generate a lot of code \n");
      printf("Useful particularly for Elliptic Curve cryptosystems over GF(p).\n");
      printf("\nAnswer (Y/N)?");  
      r=answer();
      if (r)
      {
       step_size=0;
       while (step_size<2 || step_size>32)
       {
           printf("Enter modulus size in bits = ");
           scanf("%d",&nbits);
           getchar();
           if (nofull)
           {
              printf("Enter base size in bits = ");
              scanf("%d",&userlen);
              getchar();
           } 
           else userlen=utlen;
           step_size=nbits/userlen;
           if ((nbits%userlen)!=0) step_size++;
       }
       if (!nofull && (nbits%utlen)==0)
       {
        printf("\nDo you wish to use a \"special\" fast method\n");
        printf("for modular reduction, for a particular modulus\n");
        printf("Two types are supported - Generalised Mersenne Primes\n");
        printf("also known as Solinas primes\n");
        printf("(many already supported - easy to add more - see mrcomba.tpl)\n");
        printf("and Pseudo Mersenne Primes (automatically supported)\n");
        printf("If in any doubt answer No (Y/N)?");
        r=answer();
        if (r) 
        {
         special=1;
         printf("\nIs your modulus a Generalized Mersenne Prime (Y/N)?");
         r=answer();
         if (r) special=1;
         else
         {
           printf("\nIs your modulus a \"Pseudo Mersenne\" Prime of the form\n");
           printf("2^%d-c where c fits in a single word (Y/N)?",nbits);
           r=answer();
           if (r) special=2;
         }
        }
       }
       printf("\nTo create the file MRCOMBA.C you must next execute\n");
       if (port) printf("MEX %d C MRCOMBA\n",step_size);
       else
       {
        printf("MEX %d <file> MRCOMBA\n",step_size);
        printf("where <file> is the name of the macro .mcs file (e.g. ms86)\n");
       }

       fprintf(fp,"#define MR_COMBA %d\n",step_size);
       if (special) fprintf(fp,"#define MR_SPECIAL\n");
       if (special==1) fprintf(fp,"#define MR_GENERALIZED_MERSENNE\n");
       if (special==2) fprintf(fp,"#define MR_PSEUDO_MERSENNE\n");
       if (special) fprintf(fp,"#define MR_NO_LAZY_REDUCTION\n");

       fprintf(fpl,"mrcomba.c\n");

       printf("\nSpecial routines for modular multiplication will now \n");
       printf("automatically be invoked for this modulus\n");
       printf("\nRemember to use a full-width base in your programs\n");
       printf("by calling mirsys(..,0) or mirsys(,..,256) at the start of the program\n");
       selected=1;
      }

     }
    

    if (double_type)
    {
     maxbase=0;
#ifdef __TURBOC__ 
     if (!port && !selected)
     {
      printf("\nDoes your computer have a Pentium processor\n");
      printf("and do you wish to exploit its built-in FP coprocessor\n");
      printf("NOTE: this may not be optimal for Pentium Pro or Pentium II\n");
      printf("Supported only for 80x86 processors, and Borland C Compilers\n");
      printf("This is a little experimental - so use with care\n");
      printf("Answer (Y/N)?");
      r=answer();
      if (r) 
      {
          printf("Enter (maximum) modulus size in bits = ");
          scanf("%d",&nbits);
          getchar();
          b=31;
          do {
                  b--;
                  r=64-b-b;
                  s=1.0;
                  for (i=0;i<r;i++) s*=2.0;
                  s*=b;
          } while (s<=2*nbits);
          s=1; for (i=0;i<b;i++) s*=2;
          printf("\nDo you wish to generate variable length looping code, or\n");
          printf("fixed length unrolled code? The former can be used with any\n");
          printf("modulus less than the maximum size specified above. The latter will\n");
          printf("only work with a fixed modulus of that size, but is usually a bit\n");
          printf("faster, although it can generate a *lot* of code for larger moduli.\n"); 

          printf("\nAnswer Yes for looping code(Y/N)?");
          r=answer();
          if (r)
          {
           fprintf(fp,"#define MR_PENTIUM -%d\n",nbits/b+1);

           fprintf(fpl,"mr87v.c\n");

           printf("Make sure to compile and link into your program the module MR87V.C\n");
           }
          else
          {
           fprintf(fp,"#define MR_PENTIUM %d\n",nbits/b+1);
           fprintf(fpl,"mr87f.c\n");
           printf("Make sure to compile and link into your program the module MR87F.C\n");
 
          }
          printf("\nSpecial fast routines for modular multiplication will now be\n");
          printf("automatically be invoked when, for example, powmod() is called\n");
          printf("\nIt is *vital* to use the appropriate number base, so\n");
          printf("you *must* now call mirsys(...,MAXBASE) at the start of your program\n");
          fprintf(fp,"#define MAXBASE %lf\n",s);
          maxbase=1;
      }
     }
#endif
     if (!maxbase)
     {
         s=1.0; 
         for (i=0;i<dmant-1;i++) /* extra bit "spare" so that 2 can be added */
         {
             if (i+i+1>=lmant) break;
             s*=2.0;
         }  
         fprintf(fp,"#define MAXBASE %lf\n",s);
     }
    }
    fprintf(fp,"#define MR_BITSINCHAR %d\n",bitsinchar);

    printf("\nDo you want to save space by using a smaller but slightly slower \n");
    printf("AES implementation. Default to No. (Y/N)?");
    r=answer();
    if (r)
    {
        fprintf(fp,"#define MR_SMALL_AES\n");
    }
    
    edwards=0;
    printf("\nDo you want to use Edwards paramaterization of elliptic curves over Fp\n");
    printf("This is faster for basic Elliptic Curve cryptography (but does not support\n");
    printf("Pairing-based Cryptography and some applications). Default to No. (Y/N)?");
    r=answer();
    if (r)
    {
        edwards=1;
        fprintf(fp,"#define MR_EDWARDS\n");
    }

    if (!edwards)
    {
        printf("\nDo you want to save space by using only affine coordinates \n");
        printf("for elliptic curve cryptography. Default to No. (Y/N)?");
        r=answer();
        if (r)
        {
            fprintf(fp,"#define MR_AFFINE_ONLY\n");
        }
    }
    printf("\nDo you want to save space by not using point compression \n");
    printf("for EC(p) elliptic curve cryptography. Default to No. (Y/N)?");
    r=answer();
    if (r)
        fprintf(fp,"#define MR_NOSUPPORT_COMPRESSION\n");
 
    printf("\nDo you want to save space by not supporting special code \n");
    printf("for EC double-addition, as required for ECDSA signature \n");
    printf("verification, or any multi-addition of points. Default to No. (Y/N)?");
    r=answer();
    if (r)
        fprintf(fp,"#define MR_NO_ECC_MULTIADD\n");


    printf("\nDo you want to save RAM by using a smaller sliding window \n");
    printf("for all elliptic curve cryptography. Default to No. (Y/N)?");
    r=answer();
    if (r)
        fprintf(fp,"#define MR_SMALL_EWINDOW\n");

    if (!special)
    {
    printf("\nDo you want to save some space by supressing Lazy Reduction? \n");
    printf("(as used for ZZn2 arithmetic). Default to No. (Y/N)?");
    r=answer();
    if (r)
        fprintf(fp,"#define MR_NO_LAZY_REDUCTION\n");
    }

    printf("\nDo you NOT want to use the built in random number generator?\n");
    printf("Removing it saves space, and maybe you have your own source\n");
    printf("of randomness? Default to No. (Y/N)?");
    r=answer();
    if (r)
        fprintf(fp,"#define MR_NO_RAND\n");

    sb=0;
    if (!nofull && ab)
    {
      printf("\nDo you want to save space by only using a simple number base?\n");
      printf("(the number base in mirsys(.) must be 0 or must divide 2^U\n");
      printf("exactly, where U is number of bits in the underlying type)\n");
      printf("NOTE: no number base changes possible\n");
      printf("Default to No. (Y/N)?");
      r=answer();
      if (r)
      {
          sb=1;
          fprintf(fp,"#define MR_SIMPLE_BASE\n");
      }
    } 
    if (sb)
    {
      printf("\nDo you want to save space by only using simple I/O? \n");
      printf("(Input from ROM, and Input/Output as binary bytes only) \n");
      printf("(However crude HEX-only output is supported) \n");
      printf("Default to No. (Y/N)?");
      r=answer();
      if (r)
          fprintf(fp,"#define MR_SIMPLE_IO\n");
    }

    if (!double_type)
    {
     printf("\nDo you want to save space by NOT supporting KOBLITZ curves \n");
     printf("for EC(2^m) elliptic curve cryptography. Default to No. (Y/N)?");
     r=answer();
     if (r)
         fprintf(fp,"#define MR_NOKOBLITZ\n");

     printf("\nDo you want to save space by NOT supporting SUPERSINGULAR curves \n");
     printf("for EC(2^m) elliptic curve cryptography. Default to No. (Y/N)?");
     r=answer();
     if (r)
     {
         fprintf(fp,"#define MR_NO_SS\n");
     }
    }

    printf("\nDo you want to enable a Double Precision big type. See doubig.txt\n");
    printf("for more information. Default to No. (Y/N)?");
    r=answer();
    if (r)
    {
         fprintf(fp,"#define MR_DOUBLE_BIG\n");
    }

	printf("\nDo you want to compile MIRACL as a C++ library, rather than a C library?\n");
    printf("Default to No. (Y/N)?");
    r=answer();
    if (r)
    {
         fprintf(fp,"#define MR_CPP\n");
    }

	printf("\nDo you want to avoid the use of compiler intrinsics?\n");
    printf("Default to No. (Y/N)?");
    r=answer();
    if (r)
    {
         fprintf(fp,"#define MR_NO_INTRINSICS\n");
    }

    if (!port)
    {
        if (!dlong) printf("\nYou must now provide an assembly language file mrmuldv.c,\n");
        else printf("\nYou must now provide an assembly or C file mrmuldv.c,\n");
        if (!nofull)
        printf("containing implementations of muldiv(), muldvd(), muldvd2() and muldvm()\n");
        else
        {
            printf("containing an implementation of muldiv()\n");
            if (rounding) printf("..and imuldiv()\n");
        }
        if (!dlong)
            printf("Check mrmuldv.any - an assembly language version may be\n");
        else
            printf("Check mrmuldv.any - a C or assembly language version is\n");
        printf("there already\n");
      
        fprintf(fpl,"mrmuldv.c\n");

    }
    printf("\nA file mirdef.tst has been generated. If you are happy with it,\n");
    printf("rename it to mirdef.h and use for compiling the MIRACL library.\n");

    printf("A file miracl.lst has been generated that includes all the \n");
    printf("files to be included in this build of the MIRACL library.\n");

    fprintf(fpl,"\nCompile the above with -O2 optimization\n");
    if (threaded) 
      fprintf(fpl,"Also use appropriate flag for multi-threaded compilation\n");

    if (!port)
    {
      fprintf(fpl,"Note that mrmuldv.c file may be pure assembly, so may \n");
      fprintf(fpl,"be renamed to mrmuldv.asm or mrmuldv.s, and assembled \n");
      fprintf(fpl,"rather than compiled\n");
    }
    fclose(fp);
    fclose(fpl);

    return 0;
}

