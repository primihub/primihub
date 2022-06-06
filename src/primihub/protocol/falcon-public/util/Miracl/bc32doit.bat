rem MIRACL - IBM PC/MS-DOS Version 4.0
rem This batch files creates 80386 version of miracl.lib from its component
rem parts using the 32-bit Borland C++ V4.02 (or greater) compiler BCC32
rem
rem Read your compiler documentation for further information
rem 
rem Invoke as "bc32doit" from a command prompt (MS-DOS) window. It is assumed 
rem that paths have been correctly set up to the compiler, librarian, linker
rem and assembler. Make sure that the compiler can find the MIRACL header 
rem files mirdef.h/miracl.h/big.h etc.
rem The simplest way to do this is to put them into the \bcXX\include\ 
rem directory where the standard C header files such as stdio.h are kept. 
rem
rem Provided mainly as a guide for creating a batch file tailored
rem specifically to your own configuration.
rem
rem Note - the module mrmuldv.c is not needed if MR_NOASM is defined
rem Note - one of the modules mrkcm.c mrcomba.c mr87v.c or mr87f.c may be 
rem        required to implement special methods for modular exponentiation
rem Note - If config.c was used to generate mirdef.h - then only those modules 
rem        listed in miracl.lst should be included in the library build
rem Note - If building C only version (MR_NOASM), remove the -B flag, as 
rem        this invokes the assembler TASM
rem
rem Compile MIRACL modules
copy mirdef.h32 mirdef.h
bcc32  -c -O2 mrcore.c
bcc32  -c -O2 mrarth0.c
bcc32  -c -O2 -B mrarth1.c
bcc32  -c -O2 -B mrarth2.c
bcc32  -c -O2 mralloc.c
bcc32  -c -O2 mrsmall.c
bcc32  -c -O2 mrio1.c
bcc32  -c -O2 mrio2.c
bcc32  -c -O2 mrgcd.c
bcc32  -c -O2 mrjack.c
bcc32  -c -O2 mrxgcd.c
bcc32  -c -O2 mrarth3.c
bcc32  -c -O2 mrbits.c
bcc32  -c -O2 mrrand.c
bcc32  -c -O2 mrprime.c
bcc32  -c -O2 mrcrt.c
bcc32  -c -O2 -B mrscrt.c
bcc32  -c -O2 -B mrmonty.c
bcc32  -c -O2 mrpower.c
bcc32  -c -O2 mrsroot.c
bcc32  -c -O2 mrcurve.c
bcc32  -c -O2 -B mrfast.c
bcc32  -c -O2 mrshs.c
bcc32  -c -O2 mrshs256.c
bcc32  -c -O2 mrshs512.c
bcc32  -c -O2 mraes.c
bcc32  -c -O2 mrgcm.c
bcc32  -c -O2 mrstrong.c
bcc32  -c -O2 mrlucas.c
bcc32  -c -O2 mrzzn2.c
bcc32  -c -O2 mrzzn2b.c
bcc32  -c -O2 mrzzn3.c
bcc32  -c -O2 mrzzn4.c
bcc32  -c -O2 mrbrick.c
bcc32  -c -O2 mrebrick.c
bcc32  -c -O2 mrec2m.c
bcc32  -c -O2 mrecn2.c
bcc32  -c -O2 mrgf2m.c
bcc32  -c -O2 mrflash.c
bcc32  -c -O2 mrfrnd.c
bcc32  -c -O2 mrdouble.c
bcc32  -c -O2 mrround.c
bcc32  -c -O2 mrbuild.c
bcc32  -c -O2 mrflsh1.c
bcc32  -c -O2 mrpi.c
bcc32  -c -O2 mrflsh2.c
bcc32  -c -O2 mrflsh3.c
bcc32  -c -O2 mrflsh4.c
rem bcc32  -c -O2 -B mrkcm.c
rem 
rem Assemble mrmuldv.c ; be careful to use correct version from mrmuldv.any
copy mrmuldv.c32 mrmuldv.c
bcc32  -c -B mrmuldv.c          
rem
rem Create library 'miracl.lib'
del miracl.lib
tlib miracl
tlib miracl +mrflsh4+mrflsh3+mrflsh2+mrpi+mrflsh1
tlib miracl +mrdouble+mrflash+mrfrnd+mrround+mrbuild
tlib miracl +mrio2+mrio1+mrrand+mrprime+mrcrt+mrscrt+mrfast+mrbits
tlib miracl +mrjack+mrxgcd+mrgcd+mrarth3+mrarth2+mrpower+mrsroot
tlib miracl +mrmonty+mralloc+mrarth1+mrarth0+mrsmall+mrcore+mrmuldv
tlib miracl +mrcurve+mrshs+mraes+mrlucas+mrstrong+mrbrick+mrshs256+mrgcm
tlib miracl +mrshs512+mrebrick+mrec2m+mrgf2m+mrzzn2+mrzzn3+mrecn2+mrzzn2b+mrzzn4
rem tlib miracl +mrkcm
del mr*.obj
bcc32  -c -O2 big
bcc32  -c -O2 crt
bcc32  -c -O2 zzn
bcc32  -c -O2 ecn
bcc32  -c -O2 ec2
bcc32  brent big.obj zzn.obj miracl.lib
bcc32  pk-demo big.obj crt.obj ecn.obj miracl.lib
bcc32  bmark.c miracl.lib
bcc32  -c -O2 flash
bcc32  sample flash.obj miracl.lib
bcc32  ecsgen  ecn.obj big.obj miracl.lib
bcc32  ecsign  ecn.obj big.obj miracl.lib
bcc32  ecsver  ecn.obj big.obj miracl.lib

