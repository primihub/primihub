rem MIRACL - IBM PC/MS-DOS Version 4.0
rem This batch files creates 80386 version of miracl.lib from its component
rem parts using the 32-bit DJGPP C/C++ V2.01 (or greater) compiler GCC
rem
rem Read your compiler documentation for further information
rem 
rem Invoke as "gcdoit". It is assumed that paths have been correctly set up to
rem the compiler, librarian and linker.
rem
rem Provided mainly as a guide for creating a batch file tailored
rem specifically to your own configuration.
rem
rem Note - the module mrmuldv.c is not needed if MR_NOASM is defined
rem Note - the inline assember and the optimizer don't seem to co-exist 
rem         happily
rem 
rem Use with mirdef.h32 - but ensure 64-bit type is defined as long long
rem (not __int64)
rem
rem Compile MIRACL modules
del miracl.a
copy mirdef.lnx mirdef.h
gcc -c -O2 mrcore.c
gcc -c -O2 mrarth0.c
gcc -c -O2 mrarth1.c
gcc -c -O2 mrarth2.c
gcc -c -O2 mralloc.c
gcc -c -O2 mrsmall.c
gcc -c -O2 mrio1.c
gcc -c -O2 mrio2.c
gcc -c -O2 mrgcd.c
gcc -c -O2 mrjack.c
gcc -c -O2 mrxgcd.c
gcc -c -O2 mrarth3.c
gcc -c -O2 mrbits.c
gcc -c -O2 mrrand.c
gcc -c -O2 mrprime.c
gcc -c -O2 mrcrt.c
gcc -c -O2 mrscrt.c
gcc -c -O2 mrmonty.c
gcc -c -O2 mrpower.c
gcc -c -O2 mrsroot.c
gcc -c -O2 mrcurve.c
gcc -c -O2 mrfast.c
gcc -c -O2 mrshs.c
gcc -c -O2 mrshs256.c
gcc -c -O2 mrshs512.c
gcc -c -O2 mraes.c
gcc -c -O2 mrgcm.c
gcc -c -O2 mrlucas.c
gcc -c -O2 mrzzn2.c
gcc -c -O2 mrzzn2b.c
gcc -c -O2 mrzzn3.c
gcc -c -O2 mrzzn4.c
gcc -c -O2 mrstrong.c
gcc -c -O2 mrbrick.c
gcc -c -O2 mrebrick.c
gcc -c -O2 mrec2m.c
gcc -c -O2 mrecn2.c
gcc -c -O2 mrgf2m.c
gcc -c -O2 mrflash.c
gcc -c -O2 mrfrnd.c
gcc -c -O2 mrdouble.c
gcc -c -O2 mrround.c
gcc -c -O2 mrbuild.c
gcc -c -O2 mrflsh1.c
gcc -c -O2 mrpi.c
gcc -c -O2 mrflsh2.c
gcc -c -O2 mrflsh3.c
gcc -c -O2 mrflsh4.c
as mrmuldv.gpp -o mrmuldv.o

rem gcc -c -O2 -fomit-frame-pointer mrcomba.c

ar rc miracl.a mrcore.o mrarth0.o mrarth1.o mrarth2.o mralloc.o mrsmall.o mrgcm.o
ar r  miracl.a mrio1.o mrio2.o mrjack.o mrgcd.o mrxgcd.o mrarth3.o mrbits.o mrzzn2.o mrzzn3.o mrzzn4.o
ar r  miracl.a mrrand.o mrprime.o mrcrt.o mrscrt.o mrmonty.o mrcurve.o mrpower.o mrsroot.o
ar r  miracl.a mrfast.o mrshs.o mraes.o mrlucas.o mrstrong.o mrbrick.o mrecn2.o
ar r  miracl.a mrshs256.o mrshs512.o mrmuldv.o mrebrick.o mrgf2m.o mrec2m.o mrzzn2b.o
ar r  miracl.a mrdouble.o mrround.o mrbuild.o mrflsh1.o mrpi.o mrflsh2.o mrflsh3.o mrflsh4.o mrflash.o mrfrnd.o

del mr*.o
gpp  -c -O2 big.cpp
gpp  -c -O2 zzn.cpp
gpp  -c -O2 ecn.cpp
gpp  -c -O2 ec2.cpp
gpp  brent.cpp big.o zzn.o miracl.a -o brent.exe
gpp  -c -O2 flash.cpp
gpp  sample.cpp flash.o miracl.a -o sample.exe
gpp  ecsgen.cpp ecn.o big.o miracl.a -o ecsgen.exe
gpp  ecsign.cpp ecn.o big.o miracl.a -o ecsign.exe
gpp  ecsver.cpp ecn.o big.o miracl.a -o ecsver.exe

