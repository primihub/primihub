rem MIRACL - IBM PC/MS-DOS Version 4.0
rem This batch files creates the large memory model version of miracl.lib 
rem from its component parts using the Borland C++ V3.1 (or greater) compiler
rem Also included are the commands to create some of the example programs
rem Use with typically mirdef.h16
rem
rem Read your compiler documentation for further information
rem 
rem Invoke as "bcldoit". It is assumed that paths have been correctly set up to
rem the compiler, librarian and linker.
rem
rem Provided mainly as a guide for creating a batch file tailored
rem specifically to your own configuration.
rem
rem Note - the module mrmuldv.c is not needed if MR_NOASM is defined
rem
rem Compile MIRACL modules
bcc -ml -c -O mrcore.c
bcc -ml -c -O mrarth0.c
bcc -ml -c -O mrarth1.c
bcc -ml -c -O mrarth2.c
bcc -ml -c -O mralloc.c
bcc -ml -c -O mrsmall.c
bcc -ml -c -O mrio1.c
bcc -ml -c -O mrio2.c
bcc -ml -c -O mrgcd.c
bcc -ml -c -O mrjack.c
bcc -ml -c -O mrxgcd.c
bcc -ml -c -O mrarth3.c
bcc -ml -c -O mrbits.c
bcc -ml -c -O mrrand.c
bcc -ml -c -O mrprime.c
bcc -ml -c -O mrcrt.c
bcc -ml -c -O mrscrt.c
bcc -ml -c -O mrmonty.c
bcc -ml -c -O mrpower.c
bcc -ml -c -O mrsroot.c
bcc -ml -c -O mrcurve.c
bcc -ml -c -O mrfast.c
bcc -ml -c -O mrlucas.c
bcc -ml -c -O mrzzn2.c
bcc -ml -c -O mrzzn3.c
bcc -ml -c -O mrzzn4.c
bcc -ml -c -O mrecn2.c
bcc -ml -c -O mrshs.c
bcc -ml -c -O mrshs256.c
bcc -ml -c -O mraes.c
bcc -ml -c -O mrgcm.c
bcc -ml -c -O mrstrong.c
bcc -ml -c -O mrbrick.c
bcc -ml -c -O mrebrick.c
bcc -ml -c -O mrec2m.c
bcc -ml -c -O mrgf2m.c
bcc -ml -c -O mrflash.c
bcc -ml -c -O mrfrnd.c
bcc -ml -c -O mrdouble.c
bcc -ml -c -O mrround.c
bcc -ml -c -O mrbuild.c
bcc -ml -c -O mrflsh1.c
bcc -ml -c -O mrpi.c
bcc -ml -c -O mrflsh2.c
bcc -ml -c -O mrflsh3.c
bcc -ml -c -O mrflsh4.c
rem 
rem Assemble mrmuldv.c - use inline assembly version
bcc -ml -c mrmuldv.c
rem
rem Create library 'miracl.lib'
del miracl.lib
tlib miracl
tlib miracl +mrflsh4+mrflsh3+mrflsh2+mrpi+mrflsh1+mrfrnd+mrround+mrbuild
tlib miracl +mrdouble+mrflash
tlib miracl +mrio2+mrio1+mrrand+mrprime+mrcrt+mrscrt+mrfast+mrgcm
tlib miracl +mrjack+mrxgcd+mrgcd+mrarth3+mrarth2+mrpower+mrsroot+mrbits+mrecn2
tlib miracl +mrmonty+mralloc+mrarth1+mrarth0+mrsmall+mrcore+mrmuldv+mrzzn2+mrzzn3+mrzzn4
tlib miracl +mrcurve+mrshs+mrshs256+mraes+mrlucas+mrstrong+mrbrick+mrebrick+mrec2m+mrgf2m
del mr*.obj
rem
rem Compile and link C++ versions of example programs where possible
bcc -ml -c big
bcc -ml -c zzn
bcc -ml -c crt
bcc -ml brute big.obj miracl.lib
bcc -ml brent big.obj zzn.obj miracl.lib
bcc -ml pollard big.obj zzn.obj miracl.lib
bcc -ml genkey big.obj miracl.lib
bcc -ml encode big.obj miracl.lib
bcc -ml decode crt.obj big.obj miracl.lib 
bcc -ml enciph.c miracl.lib
bcc -ml deciph.c miracl.lib 
bcc -ml palin big.obj miracl.lib
bcc -ml dssetup big.obj miracl.lib
bcc -ml dssign big.obj miracl.lib
bcc -ml dssver big.obj miracl.lib
bcc -ml dssgen big.obj miracl.lib
bcc -ml -c flash
bcc -ml sample flash.obj miracl.lib
bcc -ml ratcalc.c miracl.lib

