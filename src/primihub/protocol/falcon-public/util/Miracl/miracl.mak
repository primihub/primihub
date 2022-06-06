# UNIX Makefile for MIRACL Software Multi-Precision Math Lib.
#
#          Contributed by John Kennedy
#          Updated by M.Scott 15/7/97
#
#  Notes:  This is a "bare-bones" makefile for compiling
#          the MIRACL libs and C demo programs on a UNIX
#          system using the GNU C compiler. Please examine the CFLAGS and 
#          other configuration definitions to make sure they are
#          appropriate for your environment.  Particularly the
#          CC, LIBS, and CFLAGS definitions.  Also read the discussion
#          below about assembly language vs. portable C lib
#          support. Use with mirdef.h32 (typically) 
#          Some MIRACL builds might need certain extra modules included (e.g. 
#          mrkcm.c), and certain modules ommitted (e.g. mrmuldv.s)
#
#          BEFORE running this make file, assembly mrmuldv.s (if you are 
#          using one), as
#     
#   as mrmuldv.s -o mrmuldv.o
#
#   Invoke as "make -f miracl.mak"
#
#   Use at your own risk!
#
#  Note that this make file does not make the C++ programs.
#  To build for example the pk-demo.cpp application, proceed as follows
#   
#  g++ -I. -c crt.cpp
#  g++ -I. -c ecn.cpp
#  g++ -I. -c big.cpp
#  g++ -I. pk-demo.cpp crt.o ecn.o big.o -I/usr/lib miracl.a \
#          -lm -o pk-demo
# 
#  or perhaps that should be gpp rather than g++
#
#
CC = gcc
AR = ar

SHELL = /bin/sh

#OPTIMIZE FLAGS
CFLAGS = -c -O2 -I. -I/usr/lib


.KEEP_STATE :

MIRPROGS = hail factor fact genprime brick brent brute deciph decode dssetup \
 dssgen dssign dssver ecsgen ecsign ecsver enciph encode hilbert index \
kangaroo mersenne pollard qsieve roots williams palin genkey identity sample \
lenstra pk-demo factor


MIRACL = mrflsh4 mrflsh3 mrflsh2 mrpi mrflsh1 mrio2 mrio1 mrdouble mrflash \
mrrand mrprime mrcrt mrcurve mrshs mrshs256 mraes mrgcm mrstrong mrbrick mrebrick mrgf2m mrec2m \
mrscrt mrfast mrjack mrfrnd mrxgcd mrgcd mrround mrbuild mrarth3 mrbits mrarth2 \
mrlucas mrzzn2 mrzzn2b mrzzn3 mrecn2 mrmonty mrpower mrsroot mralloc mrarth1 mrarth0 mrsmall mrcore mrmuldv

# Try one of these two ....

LIBS = miracl.a  /usr/lib/libm.a

# LIBS = miracl.a  -lm

MIROBJS = mrflsh4.o mrflsh3.o mrflsh2.o mrpi.o mrflsh1.o mrio2.o mrio1.o \
mrdouble.o mrflash.o mrrand.o mrprime.o mrcrt.o mrscrt.o mrfast.o mrjack.o \
mrfrnd.o mrxgcd.o mrgcd.o mrstrong.o mrbrick.o mrebrick.o mrcurve.o mrshs256.o mrshs.o \
mraes.o mrgcm.o mrround.o mrbuild.o mrarth3.o mrbits.o mrarth2.o mrpower.o mrsroot.o mrec2m.o mrgf2m.o \
mrlucas.o mrzzn2.o mrzzn2b.o mrzzn3.o mrecn2.o mrmonty.o mralloc.o mrarth1.o mrarth0.o mrsmall.o mrcore.o \
mrmuldv.o 

# NOTE: THE ASSEMBLY SOURCE SHOULD BE PLACED IN 'mrmuldv.s'.  
#       IF YOU DON'T HAVE ASSEMBLY CODE, COMMENT OUT THE
#       'mrmuldv.o LINE AND DELETE THE '\' CONTINUATION CHARACTER
#       ON THE LINE ABOVE. 
#
#       Ideally FOR A 32-BIT MACHINE LIKE A SUN SPARCstation, USE mirdef.h32 
#       (but change ENDIAN definition) for mirdef.h with an assembly language 
#       mrmuldv.s
#
#       Alternatively omit mrmuldv. This will be slower. 
#               
#       See mrmuldv.any for advice on creating a mrmuldv.s assembly 
#       language module. In most cases the muldvd() function is the most 
#       time critical, so even if only this one can be written in assembly
#       language, it will be worth it. It may be necessary then to create
#       two files, mrmuldv.c for muldvm() and muldiv(), and a seperate 
#       mrmuldvd.s This make file will then have to modified to accomodate
#       this change.  
#
ALL: miracl.a  $(MIRPROGS)

miracl.a: $(MIROBJS)
#	$(RM) $@
	$(AR) crv $@ $(MIROBJS)

#
# DEPENDENCIES
#

miracl.h: mirdef.h

mrcore.o: mrcore.c mirdef.h miracl.h
mrarth0.o: mrarth0.c miracl.h
mrarth1.o: mrarth1.c miracl.h
mrarth2.o: mrarth2.c miracl.h
mralloc.o: mralloc.c miracl.h
mrmonty.o: mrmonty.c miracl.h
mrpower.o: mrpower.c miracl.h
mrsroot.o: mrsroot.c miracl.h
mrlucas.o: mrlucas.c miracl.h
mrzzn2.o: mrzzn2.c miracl.h
mrzzn2b.o: mrzzn2b.c miracl.h
mrzzn3.o: mrzzn3.c miracl.h
mrecn2.o: mrecn2.c miracl.h
mrsmall.o: mrsmall.c miracl.h
mrround.o: mrround.c miracl.h
mrio1.o: mrio1.c miracl.h
mrio2.o: mrio2.c miracl.h
mrgcd.o: mrgcd.c miracl.h
mrjack.o: mrjack.c miracl.h
mrxgcd.o: mrxgcd.c miracl.h
mrarth3.o: mrarth3.c miracl.h
mrbits.o: mrbits.c miracl.h
mrrand.o: mrrand.c miracl.h
mrprime.o: mrprime.c miracl.h
mrcrt.o: mrcrt.c miracl.h
mrscrt.o: mrscrt.c miracl.h
mrshs.o: mrshs.c miracl.h
mrshs256: mrshs256.c miracl.h
mraes.o: mraes.c miracl.h
mrgcm.o: mrgcm.c miracl.h
mrstrong.o: mrstrong.c miracl.h
mrbrick.o: mrbrick.c miracl.h
mrebrick.o: mrebrick.c miracl.h
mrec2m.o: mrec2m.c miracl.h
mrgf2m.o: mrgf2m.c miracl.h
mrcurve.o: mrcurve.c miracl.h
mrfast.o: mrfast.c miracl.h
mrfrnd.o: mrfrnd.c miracl.h
mrflash.o: mrflash.c miracl.h
mrdouble.o: mrdouble.c miracl.h
mrbuild.o: mrbuild.c miracl.h
mrflsh1.o: mrflsh1.c miracl.h
mrpi.o: mrpi.c miracl.h
mrflsh2.o: mrflsh2.c miracl.h
mrflsh3.o: mrflsh3.c miracl.h
mrflsh4.o: mrflsh4.c miracl.h
mrmuldv.o: mrmuldv.s

#DEMO PROGRAMS

hail: hail.o miracl.h
factor: factor.o miracl.h
fact: fact.o miracl.h
palin: palin.o miracl.h
genkey: genkey.o miracl.h
genprime: genprime.o miracl.h
identity: identity.o miracl.h
sample: sample.o miracl.h
pk-demo: pk-demo.o miracl.h
brent: brent.o miracl.h
brick: brick.o miracl.h
brute: brute.o miracl.h
deciph: deciph.o miracl.h
decode: decode.o miracl.h
dssetup: dssetup.o miracl.h
dssgen: dssgen.o miracl.h
dssign: dssign.o miracl.h
dssver: dssver.o miracl.h
ecsgen: ecsgen.o miracl.h
ecsign: ecsign.o miracl.h
ecsver: ecsver.o miracl.h
enciph: enciph.o miracl.h
encode: encode.o miracl.h
hilbert: hilbert.o miracl.h
index: index.o miracl.h
kangaroo: kangaroo.o miracl.h
lenstra: lenstra.o miracl.h
mersenne: mersenne.o miracl.h
pollard: pollard.o miracl.h
qsieve: qsieve.o miracl.h
roots: roots.o miracl.h
williams: williams.o miracl.h

hail:	hail.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
palin: palin.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
identity: identity.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
factor:	factor.o  $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
fact:	fact.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
genprime: genprime.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
genkey: genkey.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
lenstra: lenstra.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
pk-demo: pk-demo.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
brick: brick.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
brent: brent.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
brute: brute.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
deciph: deciph.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
decode: decode.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
dssetup: dssetup.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
dssgen: dssgen.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
dssign: dssign.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
dssver: dssver.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
ecsgen: ecsgen.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
ecsign: ecsign.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
ecsver: ecsver.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
enciph: enciph.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
encode: encode.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
hilbert: hilbert.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
sample: sample.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
index: index.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
kangaroo: kangaroo.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
mersenne: mersenne.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
pollard: pollard.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
qsieve: qsieve.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
roots: roots.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)
williams: williams.o $(LIBS)
	$(CC) -o $@ $@.o $(LIBS)


clean:
	-rm -f core a.out tags
tags:
	ctags *.c *.h



