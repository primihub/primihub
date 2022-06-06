rem MIRACL - IBM PC/MS-DOS Version 4.0
rem This batch files creates miracl.lib from its component parts
rem using the Microsoft 16-bit compiler, the Microsoft
rem Macro assembler and the Microsoft LIB librarian utility
rem Also included are the commands to create some of the example programs
rem
rem Read your compiler documentation for further information
rem 
rem Invoke as "msldoit". It is assumed that paths have been correctly set up
rem to the compiler, macro assembler, librarian and linker.
rem
rem Provided mainly as a guide for creating a batch file tailored
rem specifically to your own configuration.
rem
rem Note - the module mrmuldv.c is not needed if MR_NOASM is defined
rem
rem Compile MIRACL modules
cl /AL /O2 /c mrcore.c
cl /AL /O2 /c mrarth0.c
cl /AL /O2 /c mrarth1.c
cl /AL /O2 /c mrarth2.c
cl /AL /O2 /c mralloc.c
cl /AL /O2 /c mrsmall.c
cl /AL /O2 /c mrio1.c
cl /AL /O2 /c mrio2.c
cl /AL /O2 /c mrgcd.c
cl /AL /O2 /c mrjack.c
cl /AL /O2 /c mrxgcd.c
cl /AL /O2 /c mrarth3.c
cl /AL /O2 /c mrbits.c
cl /AL /O2 /c mrrand.c
cl /AL /O2 /c mrprime.c
cl /AL /O2 /c mrcrt.c
cl /AL /O2 /c mrscrt.c
cl /AL /O2 /c mrmonty.c
cl /AL /O2 /c mrpower.c
cl /AL /O2 /c mrsroot.c
cl /AL /O2 /c mrcurve.c
cl /AL /O2 /c mrfast.c
cl /AL /O2 /c mrshs.c
cl /AL /O2 /c mrshs256.c
cl /AL /O2 /c mraes.c
cl /AL /O2 /c mrgcm.c
cl /AL /O2 /c mrlucas.c
cl /AL /O2 /c mrzzn2.c
cl /AL /O2 /c mrzzn3.c
cl /AL /O2 /c mrzzn4.c
cl /AL /O2 /c mrecn2.c
cl /AL /O2 /c mrstrong.c
cl /AL /O2 /c mrbrick.c
cl /AL /O2 /c mrebrick.c
cl /AL /O2 /c mrgf2m.c
cl /AL /O2 /c mrec2m.c
cl /AL /O2 /c mrflash.c
cl /AL /O2 /c mrfrnd.c
cl /AL /O2 /c mrdouble.c
cl /AL /O2 /c mrround.c
cl /AL /O2 /c mrbuild.c
cl /AL /O2 /c mrflsh1.c
cl /AL /O2 /c mrpi.c
cl /AL /O2 /c mrflsh2.c
cl /AL /O2 /c mrflsh3.c
cl /AL /O2 /c mrflsh4.c
rem 
rem Assemble mrmuldv.c - use inline assembly version
cl /AL /O2 /c mrmuldv.c
rem
rem Create library 'miracl.lib'
del miracl.lib
lib miracl;
lib miracl +mrflsh4+mrflsh3+mrflsh2+mrpi+mrflsh1;
lib miracl +mrdouble+mrflash+mrfrnd+mrround+mrbuild;
lib miracl +mrio2+mrio1+mrrand+mrprime+mrcrt+mrscrt+mrfast+mrgcm+mrzzn4;
lib miracl +mrjack+mrxgcd+mrgcd+mrarth3+mrarth2+mrebrick+mrpower+mrsroot+mrbits;
lib miracl +mrmonty+mralloc+mrarth1+mrarth0+mrsmall+mrcore+mrmuldv+mrzzn2+mrzzn3+mrecn2;
lib miracl +mrcurve+mrshs+mrshs256+mraes+mrlucas+mrstrong+mrbrick+mrec2m+mrgf2m;
del mr*.obj
rem
cl /AL /O2 /c big.cpp
cl /AL /O2 /c zzn.cpp
cl /AL /O2 /c crt.cpp
rem Compile and link example programs
cl /AL /O2 /c brute.cpp
link brute+big,,,miracl;
cl /AL /O2 /c genkey.cpp
link genkey+big,,,miracl;
cl /AL /O2 /c encode.cpp 
link encode+big,,,miracl;
cl /AL /O2 /c decode.cpp 
link decode+big+crt,,,miracl;
cl /AL /O2 /c brent.cpp 
link brent+big+zzn,,,miracl;
cl /AL /O2 /c pollard.cpp 
link pollard+big+zzn,,,miracl;
cl /AL /O2 /c pk-demo.cpp
link pk-demo+big+crt,,,miracl;
cl /AL /O2 /c flash.cpp
cl /AL /O2 /c sample.cpp 
link sample+flash,,,miracl;

