rem MIRACL - IBM PC/MS-DOS Version 4.0
rem This batch files creates miracl.lib from its component parts
rem using the Microsoft 32-bit compiler, (as used in Windows '95
rem and Windows NT), and the Microsoft LIB librarian utility
rem Also included are the commands to create some of the example programs
rem
rem Read your compiler documentation for further information
rem 
rem Invoke as "msiodoit". It is assumed that paths have been correctly set up
rem to the compiler, librarian and linker.
rem
rem NOTE: This batch file will build an Intger-Only version of the MIRACL 
rem library, without support for FLASH arithmetic
rem
rem Provided mainly as a guide for creating a batch file tailored
rem specifically to your own configuration.
rem
rem Note - the module mrmuldv.c is not needed if MR_NOASM is defined
rem
rem
rem Compile MIRACL modules
cl /c /O2 mrcore.c
cl /c /O2 mrarth0.c
cl /c /O2 mrarth1.c
cl /c /O2 mrarth2.c
cl /c /O2 mralloc.c
cl /c /O2 mrsmall.c
cl /c /O2 mrio1.c
cl /c /O2 mrio2.c
cl /c /O2 mrgcd.c
cl /c /O2 mrjack.c
cl /c /O2 mrxgcd.c
cl /c /O2 mrarth3.c
cl /c /O2 mrbits.c
cl /c /O2 mrrand.c
cl /c /O2 mrprime.c
cl /c /O2 mrcrt.c
cl /c /O2 mrscrt.c
cl /c /O2 mrmonty.c
cl /c /O2 mrpower.c
cl /c /O2 mrsroot.c
cl /c /O2 mrcurve.c
cl /c /O2 mrfast.c
cl /c /O2 mrlucas.c
cl /c /O2 mrzzn2.c
cl /c /O2 mrzzn3.c
cl /c /O2 mrzzn4.c
cl /c /O2 mrecn2.c
cl /c /O2 mrshs.c
cl /c /O2 mrshs256.c
cl /c /O2 mraes.c
cl /c /O2 mrgcm.c
cl /c /O2 mrstrong.c
cl /c /O2 mrbrick.c
cl /c /O2 mrebrick.c
cl /c /O2 mrec2m.c
cl /c /O2 mrgf2m.c
cl /c /O2 mrmuldv.c

rem
rem Create library 'miracl.lib'
del miracl.lib

lib /OUT:miracl.lib mrio2.obj mrio1.obj mrrand.obj mrprime.obj mrcrt.obj mrscrt.obj mrfast.obj mrecn2.obj mrzzn4.obj
lib /OUT:miracl.lib miracl.lib mrjack.obj mrxgcd.obj mrgcd.obj  mrarth3.obj mrarth2.obj mrpower.obj mrsroot.obj
lib /OUT:miracl.lib miracl.lib mrmonty.obj mralloc.obj mrarth1.obj mrarth0.obj mrsmall.obj mrcore.obj mrmuldv.obj
lib /OUT:miracl.lib miracl.lib mrcurve.obj mrshs.obj mraes.obj mrlucas.obj mrstrong.obj mrbrick.obj mrbits.obj
lib /OUT:miracl.lib miracl.lib mrshs256.obj mrebrick.obj mrec2m.obj mrgf2m.obj mrzzn2.obj mrzzn3.obj mrgcm.obj
del mr*.obj
rem
cl /c /O2 /GX big.cpp
cl /c /O2 /GX zzn.cpp
cl /c /O2 /GX crt.cpp
cl /c /O2 /GX ecn.cpp
cl /c /O2 /GX ec2.cpp
rem Compile and link example programs
cl /c /O2 /GX brute.cpp
link brute.obj big.obj miracl.lib
cl /c /O2 /GX brent.cpp
link brent.obj big.obj zzn.obj miracl.lib
cl /c /O2 /GX pk-demo.cpp
link pk-demo.obj big.obj crt.obj ecn.obj miracl.lib
cl /c /O2 /GX bmark.c
link bmark.obj miracl.lib


