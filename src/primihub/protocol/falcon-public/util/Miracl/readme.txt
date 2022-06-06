IMPORTANT! See UPDATE.TXT for latest changes

Then read FIRST.TXT

The distribution media contains the following files

  README.TXT      -    This file
  FIRST.TXT       -    Read this next
  MSVISUAL.TXT    -    Microsoft Visual C++ V6.0 quick-start instructions
  VC2005.TXT      -    Microsoft Visual C++ V8.0 quick-start instructions
  BORLAND.TXT     -    Borland C quick-start instructions
  ARM.TXT         -    ARM processor advice
  SPARC.TXT       -    SPARC processor advice
  ITANIUM.TXT     -    ITANIUM processor advice
  FASTGF2M.TXT    -    How to speed up methods based on the field GF(2^m)
  WIN64.TXT       -    64-bit Windows advice
  DEVCPP.TXT      -    DEV-CPP quick-start instructions
  AMD64.TXT       -    AMD64 processor advice
  SSE2.TXT        -    SSE2 extensions advice
  PIC32.TXT       -    PIC32 processor advice
  POWERPC.TXT     -    POWERPC processor advice
  LINUX.TXT       -    Some advice for Linux users 
  CYGWIN.TXT      -    How to install MIRACL using Cygwin
  MANUAL.DOC      -    The Manual - read it!
  PROBLEMS.TXT    -    Known problems with MIRACL
  UPDATE.TXT      -    MIRACL Update History
  KCMCOMBA.TXT    -    Using super-fast techniques
  MAKEMCS.TXT     -    How to create your own .mcs file
  DOUBLE.TXT      -    Using a double underlying type
  FLOAT.TXT       -    Multiprecision floating-point
  DOUBIG.TXT      -    How to allow "Double Precision" big types
  EDWARDS.TXT     -    How to use Edwards curves for Elliptic curve crypto over Fp
  MANAGED.TXT     -    How to build a "managed code" version of MIRACL for .NET applications
  AESGCM.TXT      -    New support for AES-GCM mode of operation
  BLACKFIN.TXT    -    For information on Analog devices Blackfin processors
  CONFIG.C        -    Program to automatically generate a mirdef.h file 
  MEX.C           -    Program to insert fast macros into mrcomba.c/mrkcm.c 

In the subdirectory SOURCE

  MRMULDV.ANY  -    Contains assembly language versions of muldiv,muldvm,
                    muldvd and muldvd2
  MRMULDV.S    -    Version of the above for Linux i386 GCC
  MRMULDV.S64  -    Version of the above for Linux x86_64 GCC
  MRMULDV.W64  -    Version of the above for Windows x86-64 Visual Studio
  MRMULDV.GCC  -    GCC version of above - 32-bit
  MRMULDV.G64  -    GCC version of above - 64-bit
  MRMULDV.C    -    Version of the above for Win32
  MRMULDV.CCC  -    Standard C version
  MRMULDV.GPP  -    Version of the above for DJGPP GCC
  MR*.C        -    MIRACL library source files
  MRCOMBA.TPL  -    Template file for fast Comba method
  MRCOMBA2.TPL -    Template file for fast Comba method for GF2 arithmetic
  MRKCM.TPL    -    Template file for fast KCM method
  C.MCS        -    C macros for use with above
  C1.MCS       -    Alternate C macros
  C2.MCS       -    Alternate C macros
  CS.MCS       -    Alternate C macros
  CLMUL.MCS    -    MSC Intrinsic macros to use PCLMULQDQ instruction
  GCCLMUL.MCS  -    GCC assembly language to support PCLMULQDQ instruction 
  MIPS.MCS     -    Macros for MIPS32 processor
  SMARTMIP.MCS -    Macros for Smartmips smart-card processor
  MS86.MCS     -    Microsoft/Borland 80*86/Pentium macros for use with above
  GCC386.MCS   -    GCC compiler compatible Pentium macros
  ARM.MCS      -    ARM processor macros
  GCCARM.MCS   -    GCC compatable version of the above
  AVR.MCS      -    Atmel Atmega128 processor macros
  MSP430.MCS   -    TI msp430 support (uses hardware multiplier)
  BLACKFIN.MCS -    Analog Devices Blackfin processor support
  GCCMSP430.MCS -   GCC compatable version of the above
  SPARC32.MCS  -    32-bit Sparc processor macros
  SPARC64.MCS  -    64-bit Sparc processor macros
  ITANIUM.MCS  -    64-bit Itanium processor macros
  WIN64.MCS    -    64-bit x86-64 Windows macros
  AMD64.MCS    -    64-bit AMD64 procesor macros
  SSE2.MCS     -    Pentium 4 SSE2 instructions for Microsoft compiler
  GCCPPC.MCS   -    PowerPC processor macros
  GCCSSE2.MCS  -    Pentium 4 SSE2 instructions for GCC compiler
  BMARK.C      -    Benchmark program for Public Key methods
  IMRATIO.C    -    Benchmark program. Calculates S/M, I/M and J/M ratios over GF(p)
  IMRATIO2.C   -    Benchmark program. Calculates S/M and I/M ratios over GF(2^m)
  MERSENNE.C   -    Mersenne primes
  FACT.C       -    Factorials
  BRUTE.C      -    Brute-force factorisation
  BRENT.C      -    Brent-pollard factoring
  BRENT_MT.C   -    Example of generic Multi-Threading
  HAIL.C       -    Hailstone numbers
  PALIN.C      -    Palindromic numbers
  GENKEY.C     -    Generate Public and Private keys
  ENCODE.C     -    Encode using RSA method
  DECODE.C     -    Decode using RSA method
  ENCIPH.C     -    Encipher using Probabalistic method
  DECIPH.C     -    Decipher using Probabalistic method
  PK-DEMO.C    -    Demo of RSA/El Gamal/Diffie-Hellman/Elliptic Curve... 
  IDENTITY.C   -    ID based key exchange program
  HILBERT.C    -    Solve special system of equations
  SAMPLE.C     -    Example of Flash arithmetic
  ROOTS.C      -    Square roots
  POLLARD.C    -    Pollard's factoring method
  WILLIAMS.C   -    William's factoring method
  LENSTRA.C    -    Lenstra's factoring method
  QSIEVE.C     -    The Quadratic Sieve
  RATCALC.C    -    Rational Scientific Calculator
  FACTOR.C     -    Factoring Program source
  KANGAROO.C   -    Pollards Lambda method for discrete logs
  INDEX.C      -    Pollards rho method for discrete logs
  GENPRIME.C   -    Generates prime for above
  LIMLEE.C     -    Lim-Lee prime generation
  DSSETUP.C    -    Digital Signature Standard setup program
  DSSGEN.C     -    Digital Signature Standard key generator program
  DSSIGN.C     -    Digital Signature Standard signature program
  DSSVER.C     -    Digital Signature Standard verification program
  ECDH2M.C     -    Example EC Diffie-Hellman program for 32-bit constrained environments (static stack-only)
  ECDH2M16.C   -    16-bit version of the above
  ECDH2M8.c    -    8-bit version of the above
  ROMAKER2.C   -    Program to automatically generate ROMS for above programs
  ECDHP.C      -       ditto, over GF(p) - 32-bit
  ECDHP32.C    -       ditto, over GF(p) - Nice ARM example (32-bits)
  ECDHP8.C     -       ditto, over GF(p), 8-bit version
  ECDHP16.C    -       ditto, over GF(p), 16-bit version
  ROMAKER.C    -    Program to automatically generate ROMS for above programs
  ECSGEN.C     -    DSS (Elliptic Curve GF(p) variation) key generator program
  ECSIGN.C     -    DSS (Elliptic Curve GF(p) variation) signature program
  ECSVER.C     -    DSS (Elliptic Curve GF(p) variation) verification program
  ECSGEN_S.C   -    DSS (Elliptic Curve GF(p) variation) key generator program (static stack-only version)
  ECSIGN_S.C   -    DSS (Elliptic Curve GF(p) variation) signature program (static stack-only version)
  ECSVER_S.C   -    DSS (Elliptic Curve GF(p) variation) verification program (static stack-only version)
  ECSGEN2.C    -    DSS (Elliptic Curve GF(2^m) variation) key generator program
  ECSIGN2.C    -    DSS (Elliptic Curve GF(2^m) variation) signature program
  ECSVER2.C    -    DSS (Elliptic Curve GF(2^m) variation) verification program
  ECSGEN2S.C   -    DSS (Elliptic Curve GF(2^m) variation) key generator program (static stack-only version)
  ECSIGN2S.C   -    DSS (Elliptic Curve GF(2^m) variation) signature program (static stack-only version)
  ECSVER2S.C   -    DSS (Elliptic Curve GF(2^m) variation) verification program (static stack-only version)
  BRICK.C      -    Brickell's method for fast exponentiation
  EBRICK.C     -    Same for GF(p) Elliptic Curves
  EBRICK2.C    -    Same for GF(2^m) Elliptic Curves
  BIG.CPP      -    Big function implementations
  ZZN.CPP      -    ZZn function implementations
  ECN.CPP      -    ECn function implementations
  ECNZZN.CPP   -    ECn <-> ZZn functions
  EC2.CPP      -    EC2 function implementations
  GF2M.CPP     -    GF(2^m) function implementations
  CRT.CPP      -    Crt function implementations
  FLASH.CPP    -    Flash function implementations   
  FLOATING.CPP -    Float function implementations
  PAL_ENC.CPP  -    Paillier Homomorphic Encryption Program
  PAL_DEC.CPP  -    Paillier Homomorphic Decryption Program
  THREADWN.CPP -    Example of Windows Multi-threading
  THREADUX.CPP -    Example of Unix Multi-Threading
  THREADMP.CPP -    Example of openMP Multi-Threading
  FINDBASE.CPP -    Find irreducible polynomial for GF(2^m) programs
  IRP.CPP      -    Generates code to implement irreducible polynomial
  NEWBASIS.CPP -    Converts from one irreducible polynomial representation to another
  FACT.CPP     -    Example C++ source (uses BIG.H)
  HAIL.CPP     -            "                "
  PALIN.CPP    -            "                "
  BRUTE.CPP    -            "                "
  MERSENNE.CPP -            "                "
  QSIEVE.CPP   -            "                " 
  GENKEY.CPP   -            "                " 
  ENCODE.CPP   -            "                " 
  DECODE.CPP   -            "                "
  ENCIPH.CPP   -            "                " 
  DECIPH.CPP   -            "                "
  PK-DEMO.CPP  -            "                "
  LIMLEE.CPP   -            "                "
  DSSETUP.CPP  -            "                "
  DSSGEN.CPP   -            "                "
  DSSIGN.CPP   -            "                "
  DSSVER.CPP   -            "                "  
  KANGAROO.CPP -            "                "
  INDEX.CPP    -            "                "
  GENPRIME.CPP -            "                "
  BRICK.CPP    -            "                "
  EBRICK.CPP   -    Example C++ source (uses ECN.H)
  ECSGEN.CPP   -            "                "
  ECSIGN.CPP   -            "                "
  ECSVER.CPP   -            "                "  
  EBRICK2.CPP  -    Example C++ source (uses EC2.H)
  ECSGEN2.CPP  -            "                "
  ECSIGN2.CPP  -            "                "
  ECSVER2.CPP  -            "                "  
  POLLARD.CPP  -    Example C++ source (uses ZZN.H)
  WILLIAMS.CPP -            "                "  
  LENSTRA.CPP  -            "                "  
  BRENT.CPP    -            "                "
  SAMPLE.CPP   -    Example C++ source (uses FLASH.H)
  ROOTS.CPP    -            "                "
  HILBERT.CPP  -            "                "
  FSAMPLE.CPP  -    Example C++ source (uses FLOATING.H)
  CARDANO.CPP  -    Example C++ source (uses ZZn2.H)
  BP160.ECS    -    Brainpool 160-bit GF(p) curve
  BPT160.ECS   -    Brainpool 160-bit GF(p) twisted curve
  SECP160/192/224/256/521.ecs - Parameter files for some standard GF(p) elliptic curves
  NIST163/233/283/571.ecs  -    Parameter files for standard GF(2^m) elliptic curves
  KOB163/233/283/571.ecs   -    Parameter files for GF(2^m) Koblitz curves
  EDWARDS.ECS              - Parameter file for an Edwards GF(p) elliptic curve

  Note how readable the C++ versions of the example programs look.

  In the subdirectory SOURCE\CURVE

  CM.CPP       -  Complex Multiplication - creates elliptic curves
  VARIABLE.H   -  Dummy Variable class
  POLY.H       -  Polynomial Class definition, elements from ZZn
  POLY.CPP     -  Polynomial Arithmetic with ZZn coefficients
  POLY2.H      -  Polynomial Class definition, elements from GF(2^m)
  POLY2.CPP    -  Polynomial Arithmetic with GF(2^m) coefficients
  FLPOLY.H     -  Polynomial Class definition, float elements
  FLPOLY.CPP   -  Polynomial arithmetic with float coefficients
  COMPLEX.H    -  Complex Float class definition
  COMPLEX.CPP  -  Complex Float class arithmetic
  CM.TXT       -  How to build the CM application
  POLYMOD.H    -  Polynomials mod a Polynomial - Class Definition
  POLYMOD.CPP  -  ZZn Polynomial arithmetic wrt a Polynomial Modulus
  POLY2MOD.H   -  Polynomials mod a Polynomial - Class Definition
  POLY2MOD.CPP -  GF(2^m) Polynomial arithmetic wrt a Polynomial Modulus
  TRANS.CPP    -  A simple utility to convert elliptic curve to Weierstrass
  SCHOOF.CPP   -  Schoof's method for counting points on a GF(p) elliptic curve
  SCHOOF2.CPP  -  Schoof's method for counting points on a GF(2^m) elliptic curve
  SCHOOF.TXT   -  How to build the schoof Application
  SCHOOF2.TXT  -  How to build the schoof2 Application
  PS_BIG.H     -  Power series with Big coefficients - Class Definition
  PS_BIG.CPP   -  Power Series Arithmetic
  PS_ZZN.H     -  Power series with ZZN coefficients - Class Definition
  PS_ZZN.CPP   -  Power Series Arithmetic
  POLYXY.H     -  Bivariate Polynomials - Class Definition
  POLYXY.CPP   -  Bivariate Polynomilas - Implementation
  POLY2XY.H    -  Bivariate Polynomials - Class Definition
  POLY2XY.CPP  -  Bivariate Polynomilas - Implementation
  MUELLER.CPP  -  Program to generate Modular Polynomials
  PROCESS.CPP  -  Program to process Modular Polynomials wrt a prime modulus
  SEA.CPP      -  Schoof-Elkies-Atkin-Mueller algorithm
  SEA.TXT      -  How to build the MUELLER/PROCESS/SEA applications
  WEIL.CPP     -  Calculates number of points on curve over extension field
  GLV.CPP      -  Calculates a GLV decomposition

  In the subdirectory SOURCE\P1363

  P1363.H      - P1363 Header File
  P1363.C      - P1363 implementation file
  TEST1363.c   - test driver for P1363 implementation
  RSA.C        - quick start RSA application

  In the subdirectory SOURCE\CURVE\PAIRING

  BLS_SIGN.CPP - Boneh-Lynn-Shacham signature
  BLS_VER.CPP  - Boneh-Lynn-Shacham signature verification
  GF2M4X.H     - GF(2^4m) arithmetic - Header file
  GF2M4X.CPP   - GF(2^4m) arithmetic - Implementation file
  GF2M6X.H     - GF(2^6m) arithmetic - Header file
  GF2M6X.CPP   - GF(2^6m) arithmetic - Implementation file
  GF2M12X.H    - GF(2^12m) arithmetic - Header file
  GF2M12X.CPP  - GF(2^12m) arithmetic - Implementation file
  SF2M12X.H    - GF(2^12m) special extension arithmetic - Header file
  SF2M12X.CPP  - GF(2^12m) special extension arithmetic - Implementation file
  ECN2.H       - Elliptic curves over Fp2 - Header file
  ECN2.CPP     - Elliptic curves over Fp2 - Implementation file
  ECN4.H       - Elliptic curves over Fp4 - Header file
  ECN4.CPP     - Elliptic curves over Fp4 - Implementation file
  ECN6.H       - Elliptic curves over Fp6 - Header file
  ECN6.CPP     - Elliptic curves over Fp6 - Implementation file
  ZZN2.H       - Fp2 arithmetic - Header file
  ZZN2.CPP     - Fp2 arithmetic - Implementation file
  ZZN3.H       - Fp3 arithmetic - Header file
  ZZN3.CPP     - Fp3 arithmetic - Implementation file
  ZZN4.H       - Fp4 arithmetic - Header file
  ZZN4.CPP     - Fp4 arithmetic - Implementation file
  ZZN8.H       - Fp8 arithmetic - Header file
  ZZN8.CPP     - Fp8 arithmetic - Implementation file
  ECN3.H/.CPP  - Elliptic curves over Fp3
  ZZN6.H/.CPP  - Fp6 arithmetic - 2 over 3
  ZZN6a.H/.CPP - Fp6 arithmetic - 3 over 2
  ZZN12.H/.CPP - Fp12 arithmetic - 2 over 3 over 2
  ZZN12a.H/.CPP- Fp12 arithmetic - 3 over 2 over 2
  ZZN12b.H/.CPP- Fp12 arithmetic - 2 over 2 over 3
  ZZN18.H/.CPP - Fp18 arithmetic - 3 over 2 over 3
  ZZN24.H/.CPP - Fp24 arithmetic - 3 over 2 over 2 over 2
  ZZN36.H/.CPP - Fp36 arithmetic - 3 over 2 over 2 over 3
  MNT.CPP      - Program to generate MNT elliptic curves
  MNT.ECS      - Non-supersingular curve, k=6, created by CM from MNT output
  FREEMAN.CPP  - Program to generate k=10 Freeman curves
  FOLKLORE.CPP - program to create pairing-friendly non-SS curves
  IRRED.CPP    - Finds irreducible polynomial - Experimental!
  AKE.TXT      - Some explanation for these programs. 
  AKE6MNTT.CPP - Authenticated Key Exchange, MNT k=6 curve - Experimental! 1-3-6 tower
  AKE6MNTX.CPP - Authenticated Key Exchange, MNT k=6 curve - Experimental! Uses "compositum" tower.
  AKE6MNTA.CPP - Authenticated Key Exchange, MNT k=6 curve - Experimental! Uses "compositum" tower.
  AKE6MNTT.C   - Partial C version of the above
  AKE4MNTT.CPP - Authenticated Key Exchange, k=4  - Experimental!
  AKE4MNTT.C   - Partial C version of the above
  AKE4MNTA.CPP - Authenticated Key Exchange, k=4  - Experimental! - Ate pairing
  AKE4MNTT.C   - Partial C version of the above
  AKE6FSTA.CPP - Authenticated Key Exchange, k=6, Ate pairing, sextic twist!
  BN.CPP       - Program to generate BN k=12 curves
  KSS8.CPP     - Program to generate KSS k=8 curves
  KSS18.CPP    - Program to generate KSS k=18 curves
  BLS12.CPP    - Program to generate BLS k=12 curves
  BLS24.CPP    - Program to generate BLS k=24 curves
  AKE12BNE.CPP - Authenticated Key Exchange, k=12, BN curve  - Experimental!
  AKE12BNA.CPP - Authenticated Key Exchange, k=12, BN curve, Ate pairing, sextic twist!
  AKE12BNR.CPP - Authenticated Key Exchange, k=12, BN curve, R-ate pairing, sextic twist! 1-2-6-12 tower
  AKE12BNX.CPP - Authenticated Key Exchange, k=12, BN curve, R-ate pairing, sextic twist! 1-2-4-12 tower
  AKE12BLSA.CPP - Authenticated Key Exchange, k=12, BLS curve, ate pairing, sextic twist!
  AKE18KSSX.CPP - Authenticated Key Exchange, k=18, KSS curve, R-ate pairing, sextic twist!
  AKE24BLSA.CPP - Authenticated Key Exchange, k=18, BLS curve, ate pairing, sextic twist!
  AKE2CPT.CPP  - Same as above, but k=2 Cocks-Pinch curve
  AKE2SST.CPP  - Same as above, but uses a supersingular curve
  AKE4CPT.CPP  - Same as above, but k=4
  AKEW4.CPP    - Variation on the above
  AKE8CPT.CPP  - Same as above, but k=8 based
  AKE8BWT.CPP  - Variation on the above - uses Brezing-Weng curve
  K2.ECS       - Non-supersingular curve, k=2 
  K2SS.ECS     - Supersingular curve, k=2
  K4.ECS       - Non-supersingular curve, k=4 
  K4MNT.ECS    - MNT k=4 curve
  K8.ECS       - Non-supersingular curve, k=8 
  WENG.ECS     - Non-supersingular curve, k=8
  DL.CPP       - Duursma-Lee Char 2 pairings
  DL2.CPP      - Truncnated-loop eta_T char 2 pairings
  ETAT271.c    - C version of eta_T pairing
  BANDW.CPP    - Brezing & Weng curves
  AKE2NSST.CPP - Faster k=2 key exchange program - Not Superingular Curves
  AKE2CPW.CPP  - Uses New Weil pairing, k=2
  AKE1KMT.CPP  - Key Exchange program, for k=1 Koblitz-Menezes curve
  MAKE_K1.CPP  - Creates curves for the above
  XK1.ECS      - k=1 curve details
  PAIRINGS.TXT - Details of pairing-based resources
  PAIRING_1.H  - High level type 1 pairings interface header
  PAIRING_3.H  - High level type 3 pairings interface header
  AKE.CPP      - Pairing-based Authentiated Key Exchange
  BLS.CPP      - Pairing-based Short Signature Scheme
  BMC.CPP      - Pairing-based Signcryption scheme
  BLMQ.CPP     - Another pairing-based signcryption scheme
  BB1.CPP      - Boneh & Boyen BB1 IBE
  FUZZY.CPP    - Sahai & Waters Fuzzy IBE
  PEKS.CPP     - PKE with keyword search
  HIBE.CPP     - Hierarchical IBE (Lewko & Waters)
  SK_1.CPP     - Sakai-Kasahara IBE - type 1 pairing
  SK_3.CPP     - Sakai-Kasahara IBE - type 3 pairing
  CPABE.CPP    - Waters Attribute Based Cryptography
  SOK.CPP      - Sakai-Ohgishi-Kasahara key exchange (Type 1 pairing)
  BGW.CPP      - Boneh-Gentry-Waters Broadcast encryption
  IPE.CPP      - Inner Product Predicate Encryption
  WANG.CPP     - Key Exchange Protocol
  CP_PAIR      - AES-80 security from a Cocks-Pinch k=2 curve
  MNT_PAIR.CPP  - AES-80 security from an MNT k=6 curve
  BN_PAIR.CPP   - AES-128 security from a BN k=12 curve
  KSS_PAIR.CPP  - AES-192 security from a KSS k=18 curve
  BLS_PAIR.CPP  - AES-256 security from a BLS k=24 curve
  SS2_PAIR.CPP  - AES-80/128 security from a Supersingular k=4 curve over GF(2^m)
  SSP_PAIR.CPP  - AES-80/128 security from a Supersingular k=2 curve over GF(p)

  In the subdirectory INCLUDE
  
  MIRDEF.H16   -    Standard hardware specific header file for 16-bit computer
  MIRDEF.H32   -    Header file for full 32-bit Computer
  MIRDEF.H     -    Same as above
  MIRDEF.H64   -    Header file for full 64-bit Computer
  MIRDEF.W64   -    Header file for Microsoft 64-bit compiler
  MIRDEF.HPC   -    Header file for pseudo-32 bit computer
  MIRDEF.HAF   -    Header file for 16 bit use of 32 bit computer
  MIRDEF.HIO   -    Integer-Only 32-bit header file
  MIRDEF.HPP   -    Header file for full 64-bit Computer - C++ Library Build - see linux64_cpp
  MIRACL.H     -    Main MIRACL header
  BIG.H        -    C++ header for 'big' numbers
  FLASH.H      -    C++ header for 'flash' numbers
  FLOATING.H   -    C++ header for 'float' numbers
  ZZN.H        -    C++ header for 'big' numbers mod n
  CRT.H        -    C++ header for chinese remainder thereom
  ECN.H        -    C++ header for GF(p) Elliptic Curves
  ECNZZN.H     -    ECn <-> ZZn functions
  EC2.H        -    C++ header for GF(2^m) Elliptic Curves 
  GF2M.H       -    C++ header for GF(2^m)
  BRICK.H      -    C++ header for Brickell's method
  EBRICK.H     -    C++ header for Brickell's method (Elliptic Curve GF(p) version)
  EBRICK2.H    -    C++ header for Brickell's method (Elliptic Curve GF(2^m) version)

  In the subdirectory LIB

  *DOIT.BAT    -    Batch files for constructing libraries and sample progs.
  MIRACL.MAK   -    John Kennedy's UNIX make file
  LINUX        -    Linux bash script to build library - 32-bit
  LINUX64      -    Linux bash script to build library - 64-bit - C Library Build
  LINUX64_CPP  -    Linux bash script to build library - 64-bit - C++ Library Build
 

  To build the standard library on a standard OS, dump all of the MIRACL files
  into a single directory, and move to that directory

  (1) If using Windows and the Microsoft compiler, open a command window, 
      ensure paths are set correctly by executing the microsoft-supplied
      vcvars32.bat or vcvars64.bat, and then execute either ms32doit.bat
      or ms64doit.bat, depending on your system being 32-bits or 64-bits

  (2) If using Linux, open a terminal window and simply execute either 
      "bash linux", or "bash linux64" for 64-bit systems

  Otherwise 

  (1) Determine which of 
      mirdef.h64/mirdef.w64/mirdef.h32/mirdef.h16/mirdef.haf/mirdef.hpc etc 
      is suitable for you, and/or compile and run config.c to automatically 
      generate a suitable mirdef.h.

  (2) If for performance reasons a non-portable version is to be built,
      select suitable assembly language routines from mrmuldv.any, or
      write them yourself (send us a copy!). Even better - produce a
      .mcs file for the processor and use either the KCM or Comba method.

  (3) Compile and link together the mr*.c components into an object library.
      Also assemble and link in the assemble language component from 
      mrmuldv.any (if needed).  

  In the subdirectory EXE some precompiled example programs

  FACT.EXE     -    Factorial program
  ENCIPH.EXE   -    Enciphering program
  DECIPH.EXE   -    Deciphering program
  PUBLIC.KEY   -    Public key for use by enciphering program
  PRIVATE.KEY  -    Private key for use by deciphering program

  In the sub-directory FREE some FREEWARE 32-bit IBM PC Command prompt 
  specific applications. CM.EXE is free as well, but omitted here for space
  reasons. 
  
  READ.TXT     -    Read this first
  RATCALC.EXE  -    Rational Calculator
  FACTOR.EXE   -    General purpose Factoring Program  (80386+ only)
                    For maximum speed this is compiled as a true 32-bit
                    and runs in a 32-bit DOS Window

  These files (ONLY!!) are FREEWARE, and may be freely copied 
  and distributed, unmodified. Copyright remains with CertiVox. 

