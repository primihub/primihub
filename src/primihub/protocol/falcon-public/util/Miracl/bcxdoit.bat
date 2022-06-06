rem MIRACL - IBM PC/MS-DOS Version 4.0
rem This batch files creates large memory model 80386 version of miracl.lib 
rem from its component parts using the Borland C++ V3.1 (or greater) compiler
rem Use typically with mirdef.hpc
rem
rem Read your compiler documentation for further information
rem 
rem Invoke as "bcxdoit". It is assumed that paths have been correctly set up to
rem the compiler, librarian and linker.
rem
rem Provided mainly as a guide for creating a batch file tailored
rem specifically to your own configuration.
rem
rem Note - the module mrmuldv.c is not needed if MR_NOASM is defined
rem Note - one of the modules mrkcm.c mrcomba.c mr87v.c or mr87f.c may be 
rem        required to implement special methods for modular exponentiation
rem
rem Compile MIRACL modules
bcc -ml -c -3 -O mrcore.c
bcc -ml -c -3 -O mrarth0.c
bcc -ml -c -3 -O -B mrarth1.c
bcc -ml -c -3 -O -B mrarth2.c
bcc -ml -c -3 -O mralloc.c
bcc -ml -c -3 -O mrsmall.c
bcc -ml -c -3 -O mrio1.c
bcc -ml -c -3 -O mrio2.c
bcc -ml -c -3 -O mrgcd.c
bcc -ml -c -3 -O mrjack.c
bcc -ml -c -3 -O mrxgcd.c
bcc -ml -c -3 -O mrarth3.c
bcc -ml -c -3 -O mrbits.c
bcc -ml -c -3 -O mrrand.c
bcc -ml -c -3 -O mrprime.c
bcc -ml -c -3 -O mrcrt.c
bcc -ml -c -3 -O mrscrt.c
bcc -ml -c -3 -O -B mrmonty.c
bcc -ml -c -3 -O mrpower.c
bcc -ml -c -3 -O mrsroot.c
bcc -ml -c -3 -O mrcurve.c
bcc -ml -c -3 -O mrfast.c
bcc -ml -c -3 -O mrshs.c
bcc -ml -c -3 -O mrshs256.c
bcc -ml -c -3 -O mraes.c
bcc -ml -c -3 -O mrgcm.c
bcc -ml -c -3 -O mrlucas.c
bcc -ml -c -3 -O mrzzn2.c
bcc -ml -c -3 -O mrzzn3.c
bcc -ml -c -3 -O mrzzn4.c
bcc -ml -c -3 -O mrecn2.c
bcc -ml -c -3 -O mrstrong.c
bcc -ml -c -3 -O mrbrick.c
bcc -ml -c -3 -O mrebrick.c
bcc -ml -c -3 -O mrec2m.c
bcc -ml -c -3 -O mrgf2m.c
bcc -ml -c -3 -O mrflash.c
bcc -ml -c -3 -O mrfrnd.c
bcc -ml -c -3 -O mrdouble.c
bcc -ml -c -3 -O mrround.c
bcc -ml -c -3 -O mrbuild.c
bcc -ml -c -3 -O mrflsh1.c
bcc -ml -c -3 -O mrpi.c
bcc -ml -c -3 -O mrflsh2.c
bcc -ml -c -3 -O mrflsh3.c
bcc -ml -c -3 -O mrflsh4.c
rem 
rem Assemble mrmuldv.c ; use inline assembly version
bcc -ml -c -3 -B mrmuldv.c
rem tasm -ml mrmuldv.c          
rem
rem Create library 'miracl.lib'
del miracl.lib
tlib miracl
tlib miracl +mrflsh4+mrflsh3+mrflsh2+mrpi+mrflsh1
tlib miracl +mrdouble+mrflash+mrfrnd+mrround+mrbuild
tlib miracl +mrio2+mrio1+mrrand+mrprime+mrcrt+mrscrt+mrfast+mrgcm
tlib miracl +mrjack+mrxgcd+mrgcd+mrarth3+mrarth2+mrpower+mrsroot+mrbits+mrecn2
tlib miracl +mrmonty+mralloc+mrarth1+mrarth0+mrsmall+mrcore+mrmuldv+mrzzn2+mrzzn3+mrzzn4
tlib miracl +mrcurve+mrshs+mrshs256+mraes+mrlucas+mrstrong+mrbrick+mrebrick+mrgf2m+mrec2m
del mr*.obj
bcc -ml -c -3 -O big
bcc -ml -c -3 -O crt
bcc -ml -c -3 -O zzn
rem compile a couple of example programs
bcc -ml brent big.obj zzn.obj miracl.lib
bcc -ml -c -3 -O flash
bcc -ml sample flash.obj miracl.lib

