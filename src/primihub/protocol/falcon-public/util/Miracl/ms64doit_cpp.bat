rem MIRACL Build Batch File
rem This file builds an entirely C++ version of the library
rem This batch files creates miracl.lib from its component parts
rem using the Microsoft 64-bit compiler, and the Microsoft LIB librarian utility
rem Also included are the commands to create some of the example programs
rem
rem Read your compiler documentation for further information
rem 
rem Invoke as "ms64doit_cpp". It is assumed that paths have been correctly set up
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
copy mirdef.wpp mirdef.h
cl /c /O2 /W3 /Tp mrcore.c
cl /c /O2 /W3 /Tp mrarth0.c
cl /c /O2 /W3 /Tp mrarth1.c
cl /c /O2 /W3 /Tp mrarth2.c
cl /c /O2 /W3 /Tp mralloc.c
cl /c /O2 /W3 /Tp mrsmall.c
cl /c /O2 /W3 /Tp mrio1.c
cl /c /O2 /W3 /Tp mrio2.c
cl /c /O2 /W3 /Tp mrgcd.c
cl /c /O2 /W3 /Tp mrjack.c
cl /c /O2 /W3 /Tp mrxgcd.c
cl /c /O2 /W3 /Tp mrarth3.c
cl /c /O2 /W3 /Tp mrbits.c
cl /c /O2 /W3 /Tp mrrand.c
cl /c /O2 /W3 /Tp mrprime.c
cl /c /O2 /W3 /Tp mrcrt.c
cl /c /O2 /W3 /Tp mrscrt.c
cl /c /O2 /W3 /Tp mrmonty.c
cl /c /O2 /W3 /Tp mrpower.c
cl /c /O2 /W3 /Tp mrsroot.c
cl /c /O2 /W3 /Tp mrcurve.c
cl /c /O2 /W3 /Tp mrfast.c
cl /c /O2 /W3 /Tp mrlucas.c
cl /c /O2 /W3 /Tp mrzzn2.c
cl /c /O2 /W3 /Tp mrzzn2b.c
cl /c /O2 /W3 /Tp mrzzn3.c
cl /c /O2 /W3 /Tp mrzzn4.c
cl /c /O2 /W3 /Tp mrshs.c
cl /c /O2 /W3 /Tp mrshs256.c
cl /c /O2 /W3 /Tp mrshs512.c
cl /c /O2 /W3 /Tp mraes.c
cl /c /O2 /W3 /Tp mrgcm.c
cl /c /O2 /W3 /Tp mrstrong.c
cl /c /O2 /W3 /Tp mrbrick.c
cl /c /O2 /W3 /Tp mrebrick.c
cl /c /O2 /W3 /Tp mrgf2m.c
cl /c /O2 /W3 /Tp mrec2m.c
cl /c /O2 /W3 /Tp mrecn2.c
cl /c /O2 /W3 /Tp mrflash.c
cl /c /O2 /W3 /Tp mrfrnd.c
cl /c /O2 /W3 /Tp mrdouble.c
cl /c /O2 /W3 /Tp mrround.c
cl /c /O2 /W3 /Tp mrbuild.c
cl /c /O2 /W3 /Tp mrflsh1.c
cl /c /O2 /W3 /Tp mrpi.c
cl /c /O2 /W3 /Tp mrflsh2.c
cl /c /O2 /W3 /Tp mrflsh3.c
cl /c /O2 /W3 /Tp mrflsh4.c
copy mrmuldv.w64 mrmuldv.c
cl /c /O2 /W3 /Tp mrmuldv.c
cl /c /O2 /W3 /EHsc big.cpp
cl /c /O2 /W3 /EHsc zzn.cpp
cl /c /O2 /W3 /EHsc ecn.cpp
cl /c /O2 /W3 /EHsc ec2.cpp
cl /c /O2 /W3 /EHsc flash.cpp

rem
rem Create library 'miracl.lib'
rem Note that now C++ modules can be put in here as well.
del miracl.lib

lib /OUT:miracl.lib mrflsh4.obj mrflsh3.obj mrflsh2.obj mrpi.obj mrflsh1.obj
lib /OUT:miracl.lib miracl.lib mrdouble.obj mrflash.obj mrfrnd.obj mrround.obj mrbuild.obj
lib /OUT:miracl.lib miracl.lib mrio2.obj mrio1.obj mrrand.obj mrprime.obj mrcrt.obj mrscrt.obj mrfast.obj 
lib /OUT:miracl.lib miracl.lib mrjack.obj mrxgcd.obj mrgcd.obj  mrarth3.obj mrarth2.obj mrpower.obj mrsroot.obj
lib /OUT:miracl.lib miracl.lib mrmonty.obj mralloc.obj mrarth1.obj mrarth0.obj mrsmall.obj mrcore.obj mrmuldv.obj
lib /OUT:miracl.lib miracl.lib mrcurve.obj mrshs.obj mraes.obj mrlucas.obj mrstrong.obj mrbrick.obj mrbits.obj 
lib /OUT:miracl.lib miracl.lib mrshs256.obj mrshs512.obj mrebrick.obj mrgf2m.obj mrec2m.obj mrzzn2.obj mrzzn3.obj mrzzn4.obj
lib /OUT:miracl.lib miracl.lib mrecn2.obj mrzzn2b.obj mrgcm.obj
lib /OUT:miracl.lib miracl.lib big.obj zzn.obj ecn.obj ec2.obj flash.obj

del mr*.obj
rem

rem Compile and link example programs
cl /O2 /W3 /EHsc brute.cpp miracl.lib
cl /O2 /W3 /EHsc brent.cpp miracl.lib
cl /O2 /W3 /EHsc pk-demo.cpp miracl.lib
cl /O2 /W3 /Tp bmark.c miracl.lib
cl /O2 /W3 /EHsc sample.cpp miracl.lib
