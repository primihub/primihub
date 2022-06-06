rem MIRACL - IBM PC/MS-DOS Version 4.0
rem This batch files creates miracl.lib from its component parts
rem using the Microsoft 32-bit compiler, (as used in Windows '95
rem and Windows NT), and the Microsoft LIB librarian utility
rem Also included are the commands to create some of the example programs
rem
rem Read your compiler documentation for further information
rem 
rem Invoke as "ms32doit". It is assumed that paths have been correctly set up
rem to the compiler, librarian and linker.
rem
rem Provided mainly as a guide for creating a batch file tailored
rem specifically to your own configuration.
rem
rem Note - the module mrmuldv.c is not needed if MR_NOASM is defined
rem
rem NOTE - the flag /MT should be used for multithreaded applications
rem
rem Compile MIRACL modules
copy mirdef.h32 mirdef.h
cl /c /O2 /W3 mrcore.c
cl /c /O2 /W3 mrarth0.c
cl /c /O2 /W3 mrarth1.c
cl /c /O2 /W3 mrarth2.c
cl /c /O2 /W3 mralloc.c
cl /c /O2 /W3 mrsmall.c
cl /c /O2 /W3 mrio1.c
cl /c /O2 /W3 mrio2.c
cl /c /O2 /W3 mrgcd.c
cl /c /O2 /W3 mrjack.c
cl /c /O2 /W3 mrxgcd.c
cl /c /O2 /W3 mrarth3.c
cl /c /O2 /W3 mrbits.c
cl /c /O2 /W3 mrrand.c
cl /c /O2 /W3 mrprime.c
cl /c /O2 /W3 mrcrt.c
cl /c /O2 /W3 mrscrt.c
cl /c /O2 /W3 mrmonty.c
cl /c /O2 /W3 mrpower.c
cl /c /O2 /W3 mrsroot.c
cl /c /O2 /W3 mrcurve.c
cl /c /O2 /W3 mrfast.c
cl /c /O2 /W3 mrlucas.c
cl /c /O2 /W3 mrzzn2.c
cl /c /O2 /W3 mrzzn2b.c
cl /c /O2 /W3 mrzzn3.c
cl /c /O2 /W3 mrzzn4.c
cl /c /O2 /W3 mrshs.c
cl /c /O2 /W3 mrshs256.c
cl /c /O2 /W3 mrshs512.c
cl /c /O2 /W3 mraes.c
cl /c /O2 /W3 mrgcm.c
cl /c /O2 /W3 mrstrong.c
cl /c /O2 /W3 mrbrick.c
cl /c /O2 /W3 mrebrick.c
cl /c /O2 /W3 mrgf2m.c
cl /c /O2 /W3 mrec2m.c
cl /c /O2 /W3 mrecn2.c
cl /c /O2 /W3 mrflash.c
cl /c /O2 /W3 mrfrnd.c
cl /c /O2 /W3 mrdouble.c
cl /c /O2 /W3 mrround.c
cl /c /O2 /W3 mrbuild.c
cl /c /O2 /W3 mrflsh1.c
cl /c /O2 /W3 mrpi.c
cl /c /O2 /W3 mrflsh2.c
cl /c /O2 /W3 mrflsh3.c
cl /c /O2 /W3 mrflsh4.c
copy mrmuldv.c32 mrmuldv.c
cl /c /O2 /W3 mrmuldv.c

rem
rem Create library 'miracl.lib'
del miracl.lib

lib /OUT:miracl.lib mrflsh4.obj mrflsh3.obj mrflsh2.obj mrpi.obj mrflsh1.obj
lib /OUT:miracl.lib miracl.lib mrdouble.obj mrflash.obj mrfrnd.obj mrround.obj mrbuild.obj
lib /OUT:miracl.lib miracl.lib mrio2.obj mrio1.obj mrrand.obj mrprime.obj mrcrt.obj mrscrt.obj mrfast.obj 
lib /OUT:miracl.lib miracl.lib mrjack.obj mrxgcd.obj mrgcd.obj  mrarth3.obj mrarth2.obj mrpower.obj mrsroot.obj
lib /OUT:miracl.lib miracl.lib mrmonty.obj mralloc.obj mrarth1.obj mrarth0.obj mrsmall.obj mrcore.obj mrmuldv.obj
lib /OUT:miracl.lib miracl.lib mrcurve.obj mrshs.obj mraes.obj mrlucas.obj mrstrong.obj mrbrick.obj mrbits.obj 
lib /OUT:miracl.lib miracl.lib mrshs256.obj mrshs512.obj mrebrick.obj mrgf2m.obj mrec2m.obj mrzzn2.obj mrzzn3.obj mrzzn4.obj
lib /OUT:miracl.lib miracl.lib mrecn2.obj mrzzn2b.obj mrgcm.obj

del mr*.obj
rem
cl /c /O2 /W3 /GX big.cpp
cl /c /O2 /W3 /GX zzn.cpp
cl /c /O2 /W3 /GX ecn.cpp
cl /c /O2 /W3 /GX ec2.cpp
rem Compile and link example programs
cl /c /O2 /W3 /GX brute.cpp
link brute.obj big.obj miracl.lib
cl /c /O2 /W3 /GX brent.cpp
link brent.obj big.obj zzn.obj miracl.lib
cl /c /O2 /W3 /GX pk-demo.cpp
link pk-demo.obj big.obj ecn.obj miracl.lib
cl /c /O2 /W3 bmark.c
link bmark.obj miracl.lib
cl /c /O2 /W3 /GX flash.cpp


