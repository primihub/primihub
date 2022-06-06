del *.exe
del miracl.a
copy mirdef.lnx mirdef.h
mingw32-gcc -c -m32 -O2 mrcore.c
mingw32-gcc -c -m32 -O2 mrarth0.c
mingw32-gcc -c -m32 -O2 mrarth1.c
mingw32-gcc -c -m32 -O2 mrarth2.c
mingw32-gcc -c -m32 -O2 mralloc.c
mingw32-gcc -c -m32 -O2 mrsmall.c
mingw32-gcc -c -m32 -O2 mrio1.c
mingw32-gcc -c -m32 -O2 mrio2.c
mingw32-gcc -c -m32 -O2 mrgcd.c
mingw32-gcc -c -m32 -O2 mrjack.c
mingw32-gcc -c -m32 -O2 mrxgcd.c
mingw32-gcc -c -m32 -O2 mrarth3.c
mingw32-gcc -c -m32 -O2 mrbits.c
mingw32-gcc -c -m32 -O2 mrrand.c
mingw32-gcc -c -m32 -O2 mrprime.c
mingw32-gcc -c -m32 -O2 mrcrt.c
mingw32-gcc -c -m32 -O2 mrscrt.c
mingw32-gcc -c -m32 -O2 mrmonty.c
mingw32-gcc -c -m32 -O2 mrpower.c
mingw32-gcc -c -m32 -O2 mrsroot.c
mingw32-gcc -c -m32 -O2 mrcurve.c
mingw32-gcc -c -m32 -O2 mrfast.c
mingw32-gcc -c -m32 -O2 mrshs.c
mingw32-gcc -c -m32 -O2 mrshs256.c
mingw32-gcc -c -m32 -O2 mrshs512.c
mingw32-gcc -c -m32 -O2 mraes.c
mingw32-gcc -c -m32 -O2 mrgcm.c
mingw32-gcc -c -m32 -O2 mrlucas.c
mingw32-gcc -c -m32 -O2 mrzzn2.c
mingw32-gcc -c -m32 -O2 mrzzn2b.c
mingw32-gcc -c -m32 -O2 mrzzn3.c
mingw32-gcc -c -m32 -O2 mrzzn4.c
mingw32-gcc -c -m32 -O2 mrecn2.c
mingw32-gcc -c -m32 -O2 mrstrong.c
mingw32-gcc -c -m32 -O2 mrbrick.c
mingw32-gcc -c -m32 -O2 mrebrick.c
mingw32-gcc -c -m32 -O2 mrec2m.c
mingw32-gcc -c -m32 -O2 mrgf2m.c
mingw32-gcc -c -m32 -O2 mrflash.c
mingw32-gcc -c -m32 -O2 mrfrnd.c
mingw32-gcc -c -m32 -O2 mrdouble.c
mingw32-gcc -c -m32 -O2 mrround.c
mingw32-gcc -c -m32 -O2 mrbuild.c
mingw32-gcc -c -m32 -O2 mrflsh1.c
mingw32-gcc -c -m32 -O2 mrpi.c
mingw32-gcc -c -m32 -O2 mrflsh2.c
mingw32-gcc -c -m32 -O2 mrflsh3.c
mingw32-gcc -c -m32 -O2 mrflsh4.c
copy mrmuldv.gcc mrmuldv.c
mingw32-gcc -c -m32 -O2 mrmuldv.c
ar rc miracl.a mrcore.o mrarth0.o mrarth1.o mrarth2.o mralloc.o mrsmall.o mrzzn2.o mrzzn3.o
ar r miracl.a mrio1.o mrio2.o mrjack.o mrgcd.o mrxgcd.o mrarth3.o mrbits.o mrecn2.o mrzzn4.o
ar r miracl.a mrrand.o mrprime.o mrcrt.o mrscrt.o mrmonty.o mrcurve.o mrsroot.o mrzzn2b.o
ar r miracl.a mrpower.o mrfast.o mrshs.o mrshs256.o mraes.o mrlucas.o mrstrong.o mrgcm.o     
ar r miracl.a mrflash.o mrfrnd.o mrdouble.o mrround.o mrbuild.o
ar r miracl.a mrflsh1.o mrpi.o mrflsh2.o mrflsh3.o mrflsh4.o 
ar r miracl.a mrbrick.o mrebrick.o mrec2m.o mrgf2m.o mrmuldv.o mrshs512.o
del mr*.o
mingw32-gcc -m32 -O2 bmark.c miracl.a -o bmark
mingw32-gcc -m32 -O2 fact.c miracl.a -o fact
mingw32-g++ -c -m32 -O2 big.cpp
mingw32-g++ -c -m32 -O2 zzn.cpp
mingw32-g++ -c -m32 -O2 ecn.cpp
mingw32-g++ -c -m32 -O2 ec2.cpp
mingw32-g++ -c -m32 -O2 crt.cpp
mingw32-g++ -m32 -O2 mersenne.cpp big.o miracl.a -o mersenne
mingw32-g++ -m32 -O2 brent.cpp big.o zzn.o miracl.a -o brent
mingw32-g++ -c -m32 -O2 flash.cpp
mingw32-g++ -m32 -O2 sample.cpp flash.o miracl.a -o sample
mingw32-g++ -m32 -O2 ecsgen.cpp ecn.o big.o miracl.a -o ecsgen
mingw32-g++ -m32 -O2 ecsign.cpp ecn.o big.o miracl.a -o ecsign
mingw32-g++ -m32 -O2 ecsver.cpp ecn.o big.o miracl.a -o ecsver
mingw32-g++ -m32 -O2 pk-demo.cpp ecn.o big.o miracl.a -o pk-demo
mingw32-g++ -c -m32 -O2 polymod.cpp
mingw32-g++ -c -m32 -O2 poly.cpp
mingw32-g++ -m32 -O2 schoof.cpp polymod.o poly.o ecn.o crt.o zzn.o big.o miracl.a -o schoof
