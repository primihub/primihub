/*
 * MIRACL-based implementation of the IEEE 1363 standard 
 * Version 1.6 July 2010
 *
 * NEW 1.1 - Elliptic curve points now stored in a single OCTET as recommended
 *           in P1363a. Such octets will be 2*fs+1 in length, where fs is
 *           the field size. If compression is used they may be fs+1
 *
 * NEW 1.2 - Auxiliary functions completely re-written
 *           More primitives and functions implemented
 *           New mirvar_mem function used for extra speed 
 *
 * NEW 1.3 - Cryptographically strong RNG interface improved
 *
 * NEW 1.6 - Support for .NET applications. See below
 *
 * Implements most of the cryptographic primitives, message encoding methods, 
 * key derivation functions and auxiliary functions necessary for realising the 
 * recommended methods. Note that GF(p) and GF(2^m) Elliptic curves are
 * treated seperately.
 *
 * See the IEEE 1363 documentation for details
 * http://grouper.ieee.org/groups/1363/index.html
 * This IS the documentation for this implementation. Each P1363 function
 * has the same name, and same inputs/outputs as those described in the 
 * official 1363 documentation.
 *
 * The user is responsible for the initialisation and maintainance of 
 * "Octet strings" which store Cryptographic keys and application info.
 * These octet strings store big numbers in binary, from MSB down to LSB
 * NOTE:- Leading zeros can be suppressed! Use OCTET_PAD() to insert leading
 * zeros to make an octet a particular length, e.g. 20 bytes, 160 bits.
 * 
 * An instance of a Cryptographically Strong Random Number Generator
 * must be created and initialised by the user with
 * raw random data. This doesn't have to be high quality (e.g. mouse movement 
 * coordinates), but there must be enough of it for it to be unguessable. 
 * It is subsequently maintained internally by the system
 * The secure random number generation method used is based on that described 
 * in ftp://ftp.rsasecurity.com/pub/pdfs/bull-1.pdf
 *
 * The above is the recommended approach. 
 * Alternatvely when generating a key pair a random number can be specified 
 * directly as a private key, and this internal system by-passed. 
 * 
 * Domain and Public/Private key data is stored in structures defined in 
 * p1363.h. The first parameter in each function is a pointer to a function 
 * which will be called during time-consuming calculations - e.g. a Windows 
 * message pump. Negative return values indicate an error - see p1363.h for 
 * definitions. Most common MIRACL errors are mapped to these values.
 * Unlikely MIRACL errors are returned as -(1000+MEC), where MEC
 * is the MIRACL Error Code (see miracl.h)
 *
 * quick start:-
 * cl /O2 test1363.c p1363.c miracl.lib
 *
 * Recommended for use with a (32-bit) MIRACL library built 
 * from a mirdef.h file that looks something like this:-
 *
 * #define MIRACL 32
 * #define MR_LITTLE_ENDIAN
 * #define mr_utype int
 * #define MR_IBITS 32
 * #define MR_LBITS 32
 * #define mr_unsign32 unsigned int
 * #define MR_STRIPPED_DOWN
 * #define MR_GENERIC_MT
 * #define mr_dltype __int64
 * #define mr_unsign64 unsigned __int64
 * #define MAXBASE ((mr_small)1<<(MIRACL-1))
 *
 * Note that this is a multi-threaded build, ideal for a Cryptographic DLL
 * The compiler may need a multithreaded flag set (/MT ??) 
 *
 * The miracl instance variable ERCON is set to TRUE in each routine, so 
 * MIRACL does not attempt to report an error. Instead the variable ERNUM 
 * latches the last error, and if set causes each subsequent routine to 
 * "fall through".  
 *
 * Octet strings are not tested for overflow. In fact overflow will not 
 * happen as octets will be truncated - so buffer overflow attacks against 
 * this code will not succeed. To avoid unintentional overflow its best to 
 * initialise the length of most octets to that recommended and returned by 
 * the XXX_DOMAIN_INIT() routines. Note however that elliptic curve point  
 * octets should be initialised to "twice this length plus 1" (or only "this 
 * length plus 1" if compression is used)
 *
 * A test driver main() function is provided in test1363.c to test all the 
 * functions, and to implement many of the recommended methods, ECDSA, IFSSA, 
 * IFSSR, DLSSR, DLSSR-PV and ECIES 
 *
 * In Win32 MS C:-
 * cl /O2 /MT test1363.c p1363.c miracl.lib  /link /NODEFAULTLIB:libc.lib
 * and run the executable
 *
 * test1363         ; 
 * test1363 -c      ; uses EC point compression
 * test1363 -p      ; uses precomputation (window size = 8) for signature
 * test1363 -c -p   ; uses both precomputation and EC point compression
 *
 * To create a Windows P1363 DLL, compile as 
 * cl /O2 /W3 /MT /DMR_P1363_DLL /LD p1363.c miracl.lib /link /NODEFAULTLIB:libc.lib
 * To run the test program to use the DLL, compile as
 * cl /O2 /MT /DMR_P1363_DLL test1363.c p1363.lib
 * 
 * In .NET MS C:-
 * Build miracl library as described in managed.txt - but perhaps include 
 * #define MR_GENERIC_MT
 * #define MR_STRIPPED_DOWN
 *
 * To create a DLL compile as
 * cl /clr /O2 /DMR_P1363_DLL /LD /Tp p1363.c miracl.lib
 * To run the test program to use the DLL, compile as
 * cl /clr /O2 /DMR_P1363_DLL /Tp test1363.c p1363.lib
 *
 * Discrete Logarithm/GF(p) Elliptic Curve/GF(2^m) Elliptic Curve Domain 
 * details are read from the files common.dss, common.ecs and common2.ecs 
 * respectively. Suitable files are included in the standard MIRACL 
 * distribution. You can generate your own using for example the utilities 
 * dssetup.cpp, schoof.cpp and schoof2.cpp. But make sure that the group order
 * in each case is at least 160-bits, or the demo may fail.
 *
 * Note that for a full 1363 implementation various simple message-encoding 
 * regimes must be also implemented, as described in the P1363 and P1363a 
 * documentation. Many of these are now implemented here. Also suitable hashing
 * functions must be available. The SHA1/SHA256/SHA384 and SHA512 algorithms 
 * are recommended, and implemented in MIRACL 
 *
 * Note that some of the message encoding functions can take input from a file
 * rather than from a string. This is convenient for example if "signing" a 
 * large document. In these cases the parameters are typically "octet *x,FILE 
 * *fp", in which case either an OCTET string x can be specified OR a 
 * filename, but not both.
 *
 * Version 1.5
 * Miracl STATIC (no-heap version) supported. However octets (which are often 
 * variable length) are still allocated from the heap by default. To allocate 
 * fixed size octets from the stack edit p1363.h and define OCTET_FROM_STACK_SIZE
 * NOTE: Precomputation not directly supported in this mode 
 *
 * NOTE: It would be typical to break this file up and just use the parts that are needed
 * (For example if using Elliptic curve methods only, then the size of structures could be much smaller)
 *
 * NOTE: Certain methods employed in the IEEE-1363 may be subject to US patent protection.
 * Check http://grouper.ieee.org/groups/1363/index.html for more details
 *
 */

/* define this next to create Windows DLL */   

/* #define MR_1363_DLL */

#ifdef MR_P1363_DLL
#define P1363_DLL
#endif

#include <stdio.h>
#include <stdlib.h>
#include "p1363.h"

/* Hash IDs for supported EMSA3 hash functions. <hashIDlen>,<hash ID> */

static unsigned char SHA160ID[]={15,0x30,0x21,0x30,0x09,0x06,0x05,0x2b,0x0e,0x03,0x02,0x1a,0x05,0x00,0x04,0x14};
static unsigned char SHA256ID[]={19,0x30,0x21,0x30,0x0d,0x06,0x09,0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x01,0x05,0x00,0x04,0x20};
static unsigned char SHA384ID[]={19,0x30,0x21,0x30,0x0d,0x06,0x09,0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x02,0x05,0x00,0x04,0x30};
static unsigned char SHA512ID[]={19,0x30,0x21,0x30,0x0d,0x06,0x09,0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x03,0x05,0x00,0x04,0x40};

/* IDs for SHA1,RIPEMD,SHA256,SHA384 and SHA512 */
/* NOTE ripemd is not implemented               */

static unsigned char sha_id[]={0x33,0x31,0x34,0x36,0x35};

/* Internal functions */

static long filelength(FILE *fp)
{ /* return file length in reasonably portable fashion */
    long len;
    fseek(fp,0,SEEK_END);
    len=ftell(fp);
    fseek(fp,0,SEEK_SET);
    return len;
}

static void OS2FEP(_MIPD_ octet *s,big w)
{ /* internal:- converts from octet to big format */
    bytes_to_big(_MIPP_ s->len,s->val,w);
}

static void convert_big_octet(_MIPD_ big w,octet *s)
{ /* internal:- converts from big integer to octet format */
    s->len=(oltype)big_to_bytes(_MIPP_ 0,w,s->val,FALSE);
}

static void FE2OSP(_MIPD_ big w,int fsize,octet *s)
{ /* internal:- converts from big to field element format */
    s->len=(oltype)big_to_bytes(_MIPP_ fsize,w,s->val,TRUE);
}

static void EC2OSP(_MIPD_ big x,big y,int cbit,BOOL compressed,int fsize,octet *s)
{ /* internal:- converts from (x,y) to octet string */
    s->len=1; 
    if (compressed) s->val[0]=2+cbit; 
    else            s->val[0]=6+cbit;     /* hybrid */
    s->len+=(oltype)big_to_bytes(_MIPP_ fsize,x,&(s->val[1]),TRUE);
    if (!compressed) s->len+=(oltype)big_to_bytes(_MIPP_ fsize,y,&(s->val[fsize+1]),TRUE);
}

static BOOL OS2ECP(_MIPD_ octet *s,big x,big y,int fsize,int *bit)
{ /* converts from octet string to (x,y) point */

    bytes_to_big(_MIPP_ fsize,&(s->val[1]),x);
    if (s->val[0]&4)
    { /* uncompressed ..but don't put a y if not required */
        if(y!=NULL) bytes_to_big(_MIPP_ fsize,&(s->val[fsize+1]),y);
        return FALSE;
    }   
    else
    { /* compressed */
        *bit=s->val[0]&1;
        return TRUE;
    } 
}

/* return output length of hash function in bytes, also hash function input 
   block size if desired */

static int hash_params(int hash_type,int *block)
{
    switch (hash_type)
    {
    case SHA1:
        if (block!=NULL) *block=64;
        return 20;
    case SHA256:
        if (block!=NULL) *block=64;
        return 32;
#ifdef mr_unsign64
    case SHA384:
        if (block!=NULL) *block=128;
        return 48;
    case SHA512:
        if (block!=NULL) *block=128;
        return 64;   
#endif
    default:
        break;
    }
    return 0;
}

/*** Basic Octet string maintainance routines  ***/
/* initialize an octet string                    */
/* make it impossible to write beyond max length */

P1363_API BOOL OCTET_INIT(octet *w,int bytes)
{
    w->len=0;
    w->max=bytes; 
#ifndef OCTET_FROM_STACK_SIZE
    w->val=(char *)malloc(bytes);
    if (w->val!=NULL) return TRUE;
    else              return FALSE;
#else
    return TRUE;
#endif
}

P1363_API void OCTET_INIT_FROM_ARRAY(octet *w,int bytes,char *array)
{
	w->len=0;
	w->max=bytes;
	w->val=array;
}

/* clear an octet string */

P1363_API void OCTET_CLEAR(octet *w)
{
    int i;
    for (i=0;i<w->len;i++) w->val[i]=0;
    w->len=0;
}

/* XOR m with all of x */

P1363_API void OCTET_XOR_BYTE(octet *x,int m)
{
    int i;
    for (i=0;i<x->len;i++) x->val[i]^=m;
}

/* XOR common bytes of x with y */

P1363_API void OCTET_XOR(octet *x,octet *y)
{ /* xor first x->len bytes of y */

    int i;
    for (i=0;i<x->len && i<y->len;i++)
    {
        y->val[i]^=x->val[i];
    }
}

/* clear an octet */

P1363_API void OCTET_EMPTY(octet *w)
{
    w->len=0;
}

/* copy an octet string - truncates if no room */

P1363_API void OCTET_COPY(octet *x,octet *y)
{
    int i;
    OCTET_CLEAR(y);
    y->len=x->len;
    if (y->len>y->max) y->len=y->max;

    for (i=0;i<y->len;i++)
        y->val[i]=x->val[i];
}

/* compare 2 octet strings. 
 * If x==y return TRUE, else return FALSE */ 

P1363_API BOOL OCTET_COMPARE(octet *x,octet *y)
{
    int i;
    if (x->len>y->len) return FALSE;
    if (x->len<y->len) return FALSE;
    for (i=0;i<x->len;i++)
    {
        if (x->val[i]!=y->val[i]) return FALSE;
    }
    return TRUE;
}

/* check are first n bytes the same */

P1363_API BOOL OCTET_NCOMPARE(octet *x,octet *y,int n)
{
    int i;
    if (n>y->len || n>x->len) return FALSE;
    for (i=0;i<n;i++)
    {
        if (x->val[i]!=y->val[i]) return FALSE;
    }
    return TRUE;
}

/* Pads an octet with leading zeros - returns FALSE if not enough room */

P1363_API BOOL OCTET_PAD(octet *x,int len)
{
    int i,d;
    if (len>x->max || x->len>len) return FALSE;
    if (len==x->len) return TRUE;
    d=len - x->len;
    for (i=len-1;i>=d;i--)
        x->val[i]=x->val[i-d];

    for (i=0;i<d;i++) x->val[i]=0;
    x->len=(oltype)len;
    return TRUE;
}

/* Shift octet to the left by n bytes. Leftmost bytes disappear  */

P1363_API void OCTET_SHIFT_LEFT(octet *x,int n)
{
    int i;
    if (n>=x->len)
    {
        x->len=0;
        return;
    }
    x->len-=n;
    for (i=0;i<x->len;i++)
        x->val[i]=x->val[i+n];
}    

/* truncates x to n bytes and places the rest in y (if y is not NULL) */

P1363_API void OCTET_CHOP(octet *x,int n,octet *y)
{
    int i;
    if (n>=x->len)
    {
        if (y!=NULL) y->len=0;
        return;
    }
    if (y!=NULL) y->len=x->len-n;
    x->len=n; 

    if (y!=NULL)
    {
        for (i=0;i<y->len && i<y->max;i++) y->val[i]=x->val[i+n];
    }
}

/* Concatenates two octet strings */

P1363_API void OCTET_JOIN_OCTET(octet *x,octet *y)
{ /* y=y || x */
    int i,j;
    if (x==NULL) return;

    for (i=0;i<x->len;i++)
    {
        j=y->len+i;
        if (j>=y->max)
        {
            y->len=y->max;
            return;
        }
        y->val[j]=x->val[i];
    }
    y->len+=x->len;
}

/* OCTET_JOIN_LONG primitive */
/* appends long x of length len bytes to OCTET string */

P1363_API void OCTET_JOIN_LONG(long x, int len,octet *y) {
    int i,j,n;
    n=y->len+len;
    if (n>y->max || len<=0) return;
    for (i=y->len;i<n;i++) y->val[i]=0;
    y->len=n;

    i=y->len;
    while (x>0 && i>0)
    {
        i--;
        y->val[i]=x%256;
        x/=256;
    }
}

/* Convert C string to octet format - truncates if no room  */

P1363_API void OCTET_JOIN_STRING(char *s,octet *y)
{
    int i,j;
    i=y->len;
    j=0;
    while (s[j]!=0 && i<y->max)
    {
        y->val[i]=s[j];
        y->len++;
        i++;  j++;
    }
}

/* Append binary string to octet - truncates if no room */

P1363_API void OCTET_JOIN_BYTES(char *b,int len,octet *y)
{
    int i,j;
    i=y->len;
    for (j=0;j<len && i<y->max;j++) 
    {
        y->val[i]=b[j];
        y->len++;
        i++;
    }
}

/* Append byte to octet rep times */

P1363_API void OCTET_JOIN_BYTE(int ch,int rep,octet *y)
{
    int i,j;
    i=y->len;
    for (j=0;j<rep && i<y->max;j++) 
    {
        y->val[i]=ch;
        y->len++;
        i++;
    }
}

/* Copy len random bytes into octet */

P1363_API void OCTET_RAND(csprng *RNG,int len,octet *x)
{
    int i;
    if (len>x->max) len=x->max;
    x->len=len;

    for (i=0;i<len;i++) x->val[i]=strong_rng(RNG);
}

/* Output an octet string (Debug Only) */

P1363_API void OCTET_OUTPUT(octet *w)
{
    int i;
    unsigned char ch;
    for (i=0;i<w->len;i++)
    {
        ch=w->val[i];
        printf("%02x",ch);
    }
    printf("\n");
}              

P1363_API void OCTET_PRINT_STRING(octet *w)
{
    int i;
    unsigned char ch;
    for (i=0;i<w->len;i++)
    {
        ch=w->val[i];
        printf("%c",ch);
    }
}

/* Kill an octet string - Zeroise it for security */

P1363_API void OCTET_KILL(octet *w)
{
    int i;
    for (i=0;i<w->max;i++) w->val[i]=0;
    w->len=0;
    w->max=0;
#ifndef OCTET_FROM_STACK_SIZE
    free(w->val);
#endif
}

/*** P1363 Auxiliary Functions ***/ 

/* Internal - All Purpose Hash function */

/* Input is taken from 

1. octet p
2. pointer to 32-bit number n
3. octet x
4. file fp - opened by calling program
5. octet e

   in this order.

Returns TRUE if x and/or fp are empty

*/

static BOOL hash(octet *p,int *n,octet *x,FILE *fp,octet *e,int hash_type,octet *w)
{
    BOOL result=TRUE;
    int i,hlen,ch,c[4];
    sha256 sh32;
#ifdef mr_unsign64
    sha512 sh64;
#endif
    char hh[MAX_HASH_BYTES];

    if (n!=NULL)
    {
        c[0]=(*n>>24)&0xff;
        c[1]=(*n>>16)&0xff;
        c[2]=(*n>>8)&0xff;
        c[3]=(*n)&0xff;
    }

    hlen=hash_params(hash_type,NULL);
    if (hlen==0) 
    {
        OCTET_EMPTY(w);
        return result;
    }

    switch (hash_type)
    {
    case SHA1:
        shs_init(&sh32);
        if (p!=NULL)
            for (i=0;i<p->len;i++) shs_process(&sh32,p->val[i]);
        if (n!=NULL) 
           for (i=0;i<4;i++) shs_process(&sh32,c[i]);
        if (x!=NULL)
            for (i=0;i<x->len;i++) 
               { shs_process(&sh32,x->val[i]); result=FALSE; }
        if (fp!=NULL)
            while ((ch=fgetc(fp))!=EOF) 
                { shs_process(&sh32,ch); result=FALSE;}
        if (e!=NULL)
            for (i=0;i<e->len;i++) shs_process(&sh32,e->val[i]);
        shs_hash(&sh32,hh);
        break;
    case SHA256:
        shs256_init(&sh32);
        if (p!=NULL)
            for (i=0;i<p->len;i++) shs256_process(&sh32,p->val[i]);
        if (n!=NULL) 
           for (i=0;i<4;i++) shs256_process(&sh32,c[i]);
        if (x!=NULL)
            for (i=0;i<x->len;i++) 
                 { shs256_process(&sh32,x->val[i]); result=FALSE; }
        if (fp!=NULL)
            while ((ch=fgetc(fp))!=EOF) 
                 { shs256_process(&sh32,ch); result=FALSE; }
        if (e!=NULL)
            for (i=0;i<e->len;i++) shs256_process(&sh32,e->val[i]);
        shs256_hash(&sh32,hh);
        break;
#ifdef mr_unsign64
    case SHA384:
        shs384_init(&sh64);
        if (p!=NULL)
            for (i=0;i<p->len;i++) shs384_process(&sh64,p->val[i]);
        if (n!=NULL) 
           for (i=0;i<4;i++) shs384_process(&sh64,c[i]);
        if (x!=NULL)
            for (i=0;i<x->len;i++) 
                { shs384_process(&sh64,x->val[i]); result=FALSE; }
        if (fp!=NULL)
            while ((ch=fgetc(fp))!=EOF) 
                { shs384_process(&sh64,ch);  result=FALSE; }
        if (e!=NULL)
            for (i=0;i<e->len;i++) shs384_process(&sh64,e->val[i]);
        shs384_hash(&sh64,hh);
        break;
    case SHA512:
        shs512_init(&sh64);
        if (p!=NULL)
            for (i=0;i<p->len;i++) shs512_process(&sh64,p->val[i]);
        if (n!=NULL) 
           for (i=0;i<4;i++) shs512_process(&sh64,c[i]);
        if (x!=NULL)
            for (i=0;i<x->len;i++) 
                 { shs512_process(&sh64,x->val[i]); result=FALSE; }
        if (fp!=NULL)
            while ((ch=fgetc(fp))!=EOF) 
                 { shs512_process(&sh64,ch); result=FALSE; }
        if (e!=NULL)
            for (i=0;i<e->len;i++) shs512_process(&sh64,e->val[i]);
        shs512_hash(&sh64,hh);
        break;
#endif
    default:
        OCTET_EMPTY(w);
        return result;
    }
    OCTET_EMPTY(w);
    OCTET_JOIN_BYTES(hh,hlen,w);
    for (i=0;i<hlen;i++) hh[i]=0;
    return result;
}

/* General Purpose hash function */

P1363_API void HASH(int hash_type,octet *x,octet *y)
{
    hash(x,NULL,NULL,NULL,NULL,hash_type,y);
}

/* Initialise a Cryptographically Strong Random Number Generator from 
   an octet of raw random data */

P1363_API void CREATE_CSPRNG(csprng *RNG,octet *raw)
{
    strong_init(RNG,raw->len,raw->val,0L);
}

P1363_API void KILL_CSPRNG(csprng *RNG)
{
    strong_kill(RNG);
}

/* Mask Generation Function */

P1363_API void MGF1(octet *z,int olen,int hash_type,octet *mask)
{
    octet h;
    int counter,cthreshold;
    int hlen;

    hlen=hash_params(hash_type,NULL);
    if (hlen==0) return;
    OCTET_INIT(&h,hlen);
    OCTET_EMPTY(mask);

    cthreshold=MR_ROUNDUP(olen,hlen);

    for (counter=0;counter<cthreshold;counter++)
    {
        hash(z,&counter,NULL,NULL,NULL,hash_type,&h);
        if (mask->len+hlen>olen) OCTET_JOIN_BYTES(h.val,olen%hlen,mask);
        else                     OCTET_JOIN_OCTET(&h,mask);
    }
    OCTET_KILL(&h);
}

/*** P1363 recommended Message Encoding Methods ***/
/* message encoding methods for signatures with Appendix */

P1363_API BOOL EMSA1(octet *x,FILE *fp,int bits,int hash_type,octet *w)
{ /* w is a bits-length representative of the    * 
   * octet string message x (if x!=NULL)         *
   * or of the file fp (if fp!=NULL)             *
   * w must be initialised for at least 20 bytes */
    unsigned char c1,c2;
    int i,hlen;

    hash(NULL,NULL,x,fp,NULL,hash_type,w);
    hlen=hash_params(hash_type,NULL);
    if (hlen==0) return FALSE;

    if (bits<8*hlen)
    { 
        for (i=hlen-1;i>bits/8;i--) 
        { /* remove bytes off the end */
            w->val[i]=0;
            w->len--;
        }    
        bits=bits%8;
        if (bits!=0)
        {
            for (i=w->len-1;i>=0;i--)
            {
                c1=(unsigned char)w->val[i];
                c1>>=(8-bits);               
                if (i>0)
                {
                    c2=(unsigned char)w->val[i-1];
                    c2<<=bits;
                    c1|=c2;
                }
                w->val[i]=(char)c1;
            }
        }
    }
    return TRUE;
}

P1363_API BOOL EMSA2(octet *x,FILE *fp,int bits,int hash_type,octet *w)
{ /* w is a bits-length representative of the    * 
   * octet string message x (if x!=NULL)         *
   * and/or of the file fp (if fp!=NULL)         *
   * w must be initialised for at least 1+bits/8 *
   * bytes                                       */
    int lp,hlen;
    char id,p1;
    octet h;
    BOOL zero;
    hlen=hash_params(hash_type,NULL);
    if (hlen==0 || ((bits+1)%8)!=0) return FALSE;
    if (bits<8*hlen+31 || (x!=NULL && fp!=NULL)) return FALSE;

    id=sha_id[hash_type];

    OCTET_INIT(&h,hlen);
    zero=hash(NULL,NULL,x,fp,NULL,hash_type,&h);

    p1=0x4b;
    if (!zero) p1=0x6b;
    lp=((bits+1)/8)-hlen-4;

    OCTET_EMPTY(w);
    OCTET_JOIN_BYTE(p1,1,w);
    OCTET_JOIN_BYTE(0xbb,lp,w);
    OCTET_JOIN_BYTE(0xba,1,w);
    OCTET_JOIN_OCTET(&h,w);
    OCTET_JOIN_BYTE(id,1,w);
    OCTET_JOIN_BYTE(0xcc,1,w);

    OCTET_KILL(&h);
    return TRUE;
}

P1363_API BOOL EMSA3(octet *x,FILE *fp,int bits,int hash_type,octet *w)
{ /* w is a bits-length representative of the    * 
   * octet string message x (if x!=NULL)         *
   * or of the file fp (if fp!=NULL)             *
   * w must be initialised big enough to take    *
   * the result > hlen+hashIDlen+10 bytes        */
    int olen,hlen,hashIDlen;
    unsigned char *idptr;
    octet h;
    hlen=hash_params(hash_type,NULL);
    if (hlen==0) return FALSE;

    switch (hash_type)
    {
    case SHA1:
        hashIDlen=SHA160ID[0];
        idptr=&SHA160ID[1];
        break;
    case SHA256:
        hashIDlen=SHA256ID[0];
        idptr=&SHA256ID[1];
        break;
#ifdef mr_unsign64
    case SHA384:
        hashIDlen=SHA384ID[0];
        idptr=&SHA384ID[1];
        break;
    case SHA512:
        hashIDlen=SHA512ID[0];
        idptr=&SHA512ID[1];
        break;
#endif
    default:
        break;
    }
    olen=bits/8;
    if (olen<hashIDlen+hlen+10 || (x!=NULL && fp!=NULL)) return FALSE;

    OCTET_INIT(&h,hlen);
    hash(NULL,NULL,x,fp,NULL,hash_type,&h);

    OCTET_EMPTY(w);
    OCTET_JOIN_BYTE(0x01,1,w);
    OCTET_JOIN_BYTE(0xff,olen-hashIDlen-hlen-2,w);
    OCTET_JOIN_BYTE(0x00,1,w);
    OCTET_JOIN_BYTES((char *)idptr,hashIDlen,w);
    OCTET_JOIN_OCTET(&h,w);

    OCTET_KILL(&h);

    return TRUE;
}

/* message encoding methods for signatures with Recovery   */
/* Hash function ID is a mess - see see D5.2.2.2 Note 1 ?? */ 

P1363_API BOOL EMSR1_DECODE(BOOL hash_id,int r1len,int r2len,octet *f,octet *m2,FILE *fp,int bits,int hash_type,octet *v,int m1len,octet *m1)
{ /* m2 can come from an OCTET or from a file.... */
    int rlen,hlen,olen=bits/8;
    long m2len;
    BOOL result;
    octet hh,h,r;

    hlen=hash_params(hash_type,NULL);
    if (hlen==0) return FALSE;
    if (hash_id)
    {
        if (r1len>hlen+1 || r2len>hlen+1) return FALSE;
    }
    else
    {
        if (r1len>hlen || r2len>hlen) return FALSE;
    }

    if (fp==NULL)
    {   
        if (m2!=NULL) m2len=m2->len;
        else          m2len=0;
    }
    else
    {
        if (m2!=NULL) return FALSE;
        m2len=filelength(fp);     
    }

    if (m2len==0) rlen=r1len;
    else          rlen=r2len;

    if (bits<8*rlen) return FALSE;

    if (m1len > olen-rlen) return FALSE;

    OCTET_INIT(&r,rlen+m1len);
    OCTET_COPY(f,&r);
    if (!OCTET_PAD(&r,rlen+m1len))
    {
        OCTET_KILL(&r); 
        return FALSE;
    }
    OCTET_CHOP(&r,rlen,m1);
    OCTET_INIT(&hh,16+m1len);
    OCTET_INIT(&h,hlen+1);

    OCTET_JOIN_LONG((long)m1len,8,&hh);
    OCTET_JOIN_LONG(m2len,8,&hh);
    OCTET_JOIN_OCTET(m1,&hh);

    hash(&hh,NULL,m2,fp,v,hash_type,&h);

    if (hash_id)
         OCTET_JOIN_BYTE(sha_id[hash_type],1,&h);
    OCTET_CHOP(&h,rlen,NULL);

    result=OCTET_COMPARE(&h,&r);
    OCTET_KILL(&r);
    OCTET_KILL(&h);
    OCTET_KILL(&hh);
    return result;
}

P1363_API BOOL EMSR1_ENCODE(BOOL hash_id,int r1len,int r2len,octet *m1,octet *m2,FILE *fp,int bits,int hash_type,octet *v,octet *f)
{ /* m2 can come from an OCTET or from a file.... */
    int m1len,rlen,hlen,olen=bits/8;
    long m2len;
    octet hh,h;

    hlen=hash_params(hash_type,NULL);
    if (hlen==0) return FALSE;
    if (hash_id)
    {
        if (r1len!=hlen+1 || r2len!=hlen+1) return FALSE;
    }
    else
    {
        if (r1len>hlen || r2len>hlen) return FALSE;
    }

    m1len=m1->len;
    if (fp==NULL)
    {   
        if (m2!=NULL) m2len=m2->len;
        else          m2len=0;
    }
    else
    {
        if (m2!=NULL) return FALSE;
        m2len=filelength(fp); 
    }

    if (m2len==0) rlen=r1len;
    else          rlen=r2len;
    if (bits<8*rlen) return FALSE;

    if (m1len > olen-rlen) return FALSE;

    OCTET_INIT(&hh,16+m1len);
    OCTET_INIT(&h,hlen+1);

    OCTET_JOIN_LONG((long)m1len,8,&hh);
    OCTET_JOIN_LONG(m2len,8,&hh);
    OCTET_JOIN_OCTET(m1,&hh);

    hash(&hh,NULL,m2,fp,v,hash_type,&h);

    if (hash_id)
         OCTET_JOIN_BYTE(sha_id[hash_type],1,&h);
    OCTET_CHOP(&h,rlen,NULL);

    OCTET_EMPTY(f);
    OCTET_JOIN_OCTET(&h,f);
    OCTET_JOIN_OCTET(m1,f);

    OCTET_KILL(&h);
    OCTET_KILL(&hh);

    return TRUE;
}

/* EMSR2 - p is key derivation parameters - NULL by default *
 * sym=TRUE  - use symmetric encryption
 * sym=FALSE - use stream cipher
 * assumes KDF2 and AES. */

P1363_API BOOL EMSR2_DECODE(int padlen,BOOL sym,int hash_type,octet *p,octet *c,octet *v,octet *m)
{
    int i,len,clen;
    octet t,k;
    clen=c->len;
    if (clen<padlen) return FALSE;
    OCTET_INIT(&t,clen);
    if (sym)
    {
        OCTET_INIT(&k,16);
        if (!KDF2(v,p,16,hash_type,&k))
        {  
            OCTET_KILL(&t);
            OCTET_KILL(&k);
            return FALSE;  
        }
        if (!AES_CBC_IV0_DECRYPT(&k,c,NULL,&t,NULL))
        {  
            OCTET_KILL(&t);
            OCTET_KILL(&k);
            return FALSE;  
        }
    }
    else
    {
        OCTET_INIT(&k,clen);
        if (!KDF2(v,p,clen,hash_type,&k))
        {
            OCTET_KILL(&t);  
            OCTET_KILL(&k);
            return FALSE;
        }
        OCTET_COPY(c,&t);
        OCTET_XOR(&k,&t);
    }
    OCTET_KILL(&k);
    len=t.val[0];
    if (len!=padlen)
    { 
        OCTET_KILL(&t);
        return FALSE;
    }
    for (i=1;i<padlen;i++)
    {
        if (t.val[i]!=padlen || i>=t.len)
        { 
            OCTET_KILL(&t);
            return FALSE;
        }
    }
    OCTET_CHOP(&t,padlen,m);
    OCTET_KILL(&t);
   
    return TRUE;
}

P1363_API BOOL EMSR2_ENCODE(int padlen,BOOL sym,int hash_type,octet *p,octet *m,octet *v,octet *c)
{
    int mlen;
    octet k,t;

    if (padlen<1 || padlen>255) return FALSE;
    mlen=m->len;
    OCTET_INIT(&t,padlen+mlen);
    OCTET_JOIN_BYTE(padlen,padlen,&t);
    OCTET_JOIN_OCTET(m,&t);

    if (sym)
    {
        OCTET_INIT(&k,16);            /* AES Key */
        if (!KDF2(v,p,16,hash_type,&k)) 
        {
            OCTET_KILL(&t);
            OCTET_KILL(&k);
            return FALSE;
        }
        if (!AES_CBC_IV0_ENCRYPT(&k,&t,NULL,c,NULL))
        {
            OCTET_KILL(&t);  
            OCTET_KILL(&k);
            return FALSE;
        }
    }
    else
    {
        OCTET_INIT(&k,padlen+mlen);
        if (!KDF2(v,p,padlen+mlen,hash_type,&k))
        {
            OCTET_KILL(&t);  
            OCTET_KILL(&k);
            return FALSE;
        }
        OCTET_XOR(&k,&t);
        OCTET_COPY(&t,c);
    }
    OCTET_KILL(&t);
    OCTET_KILL(&k);

    return TRUE;
}


/* PSS Message Encoding */

P1363_API BOOL EMSA4_ENCODE(BOOL hash_id,int hash_type,int slen,csprng *RNG,int bits,octet *m,FILE *fp,octet *f)
{
    return EMSR3_ENCODE(hash_id,hash_type,slen,RNG,bits,NULL,m,fp,f);
}

P1363_API BOOL EMSA4_DECODE(BOOL hash_id,int hash_type,int slen,int bits,octet *f,octet *m,FILE *fp)
{
    return EMSR3_DECODE(hash_id,hash_type,slen,bits,f,m,fp,NULL);
}

/* PSS-R Message Encoding for signature with recovery */

P1363_API BOOL EMSR3_ENCODE(BOOL hash_id,int hash_type,int slen,csprng *RNG,int bits,octet *m1,octet *m2,FILE *fp,octet *f)
{
    int m1len,hlen,u,olen,t,zbytes,dblen;
    unsigned char mask;
    octet h,salt,md,db;
    u=1;
    if (hash_id) u=2;
    if (m1!=NULL) m1len=m1->len;
    else          m1len=0;

    hlen=hash_params(hash_type,NULL);
    if (hlen==0) return FALSE;

    olen=MR_ROUNDUP(bits,8);

    t=8*olen-bits;
    mask=0xFF;
    if (t>0) mask>>=t;  

    if (8*m1len>bits-8*slen-8*hlen-8*u-1) return FALSE;

    OCTET_INIT(&h,hlen);
    hash(NULL,NULL,m2,fp,NULL,hash_type,&h);

    OCTET_INIT(&salt,slen);
    OCTET_RAND(RNG,slen,&salt);

    OCTET_INIT(&md,8+m1len+hlen+slen);
    OCTET_JOIN_LONG(8*m1len,8,&md);
    OCTET_JOIN_OCTET(m1,&md);
    OCTET_JOIN_OCTET(&h,&md);
    OCTET_JOIN_OCTET(&salt,&md);

    hash(&md,NULL,NULL,NULL,NULL,hash_type,&h);

    OCTET_KILL(&md);
    zbytes=olen-m1len-slen-hlen-u;
    OCTET_INIT(&db,zbytes+m1len+slen);
    OCTET_JOIN_BYTE(0x00,zbytes-1,&db);
    OCTET_JOIN_BYTE(0x01,1,&db);
    OCTET_JOIN_OCTET(m1,&db);
    OCTET_JOIN_OCTET(&salt,&db);
    OCTET_KILL(&salt);

    dblen=olen-hlen-u;

    MGF1(&h,dblen,hash_type,f);

    OCTET_XOR(&db,f);
    f->val[0]&=mask;

    OCTET_JOIN_OCTET(&h,f);
    OCTET_KILL(&h);
    OCTET_KILL(&db);
    if (hash_id)
    {
        OCTET_JOIN_BYTE(sha_id[hash_type],1,f);
        OCTET_JOIN_BYTE(0xcc,1,f);
    }
    else
        OCTET_JOIN_BYTE(0xbc,1,f);
/* remove leading zeros 
    while (f->val[0]==0) OCTET_SHIFT_LEFT(f,1);     */

    return TRUE;
}

P1363_API BOOL EMSR3_DECODE(BOOL hash_id,int hash_type,int slen,int bits,octet *f,octet *m2,FILE *fp,octet *m1)           
{
    int k,t,u,hlen,dblen,m1len,olen;
    octet salt,h2,h,db,md,dbmask;
    unsigned char mask,ch;
    BOOL result;

    u=1;
    if (hash_id) u=2;

    hlen=hash_params(hash_type,NULL);
    if (hlen==0) return FALSE;
    if (bits<8*slen+8*hlen+8*u+1) return FALSE; 

    olen=MR_ROUNDUP(bits,8);
  
    t=8*olen-bits;
    mask=0xFF;
    if (t>0) mask>>=t; 

    if (!OCTET_PAD(f,olen)) return FALSE;
    if (hash_id)
    {
        if (f->val[olen-1]!=(char)0xcc) return FALSE;  
        if (f->val[olen-2]!=sha_id[hash_type]) return FALSE;
    }
    else
        if (f->val[olen-1]!=(char)0xbc) return FALSE;

    if (f->val[0]&(~mask)) return FALSE;

    OCTET_INIT(&h2,hlen);
    hash(NULL,NULL,m2,fp,NULL,hash_type,&h2);

    OCTET_INIT(&db,olen);
    OCTET_INIT(&h,hlen+2);

    dblen=olen-hlen-u;

    OCTET_COPY(f,&db);
    OCTET_CHOP(&db,dblen,&h);
    OCTET_CHOP(&h,hlen,NULL);

    OCTET_INIT(&dbmask,dblen);
    MGF1(&h,dblen,hash_type,&dbmask);

    OCTET_XOR(&dbmask,&db);
    OCTET_KILL(&dbmask);
    db.val[0]&=mask;
    OCTET_INIT(&salt,slen);
    OCTET_CHOP(&db,dblen-slen,&salt);
    k=0;
    while (k<db.len)
    {
        ch=db.val[k];
        if (ch!=0x00) break;
        k++;
    }

    if (k==db.len || ch!=0x01)
    {
        OCTET_KILL(&salt);
        OCTET_KILL(&h);
        OCTET_KILL(&db);
        OCTET_KILL(&h2);
        return FALSE;
    }
    OCTET_SHIFT_LEFT(&db,k+1);

    m1len=db.len;
    OCTET_INIT(&md,8+m1len+hlen+slen);    
    OCTET_JOIN_LONG(8*m1len,8,&md);
    OCTET_JOIN_OCTET(&db,&md);
    OCTET_JOIN_OCTET(&h2,&md);
    OCTET_JOIN_OCTET(&salt,&md);
    OCTET_KILL(&salt);
    hash(&md,NULL,NULL,NULL,NULL,hash_type,&h2);
    OCTET_KILL(&md);

    result=OCTET_COMPARE(&h,&h2);
    OCTET_KILL(&h2);
    OCTET_KILL(&h);
    if (!result)
    {
        OCTET_KILL(&db);
        return FALSE;
    }
    result=TRUE;
    if (m1!=NULL) OCTET_COPY(&db,m1);
    else if (db.len!=0) result=FALSE;
    OCTET_KILL(&db);
    return result;
}

/* OAEP Message Encoding for Encryption */

P1363_API BOOL EME1_ENCODE(octet *m,csprng *RNG,int bits,octet *p,int hash_type,octet *f)
{ 
    int slen,olen=bits/8;
    int mlen=m->len;
    int hlen,seedlen;
    octet dbmask,seed;
       
    hlen=seedlen=hash_params(hash_type,NULL);
    if (hlen==0 || mlen>olen-hlen-seedlen-1) return FALSE;
    if (m==f) return FALSE;  /* must be distinct octets */ 

    hash(p,NULL,NULL,NULL,NULL,hash_type,f);

    slen=olen-mlen-hlen-seedlen-1;      

    OCTET_JOIN_BYTE(0,slen,f);
    OCTET_JOIN_BYTE(0x1,1,f);
    OCTET_JOIN_OCTET(m,f);

    OCTET_INIT(&dbmask,olen-seedlen);

    OCTET_INIT(&seed,seedlen);
    OCTET_RAND(RNG,seedlen,&seed);

    MGF1(&seed,olen-seedlen,hash_type,&dbmask);
    OCTET_XOR(f,&dbmask);
    MGF1(&dbmask,seedlen,hash_type,f);
    OCTET_XOR(&seed,f);
    OCTET_JOIN_OCTET(&dbmask,f);
    OCTET_KILL(&seed);
    OCTET_KILL(&dbmask);

    return TRUE;
}

/* OAEP Message Decoding for Decryption */

P1363_API BOOL EME1_DECODE(octet *f,int bits,octet *p,int hash_type,octet *m)
{
    BOOL comp;
    int i,k,olen=bits/8;
    int hlen,seedlen;
    octet dbmask,seed,chash;
    int x,t;

    seedlen=hlen=hash_params(hash_type,NULL);
    if (hlen==0 || olen<seedlen+hlen+1) return FALSE;

    if (!OCTET_PAD(f,olen+1)) return FALSE;

    OCTET_INIT(&chash,hlen);
    hash(p,NULL,NULL,NULL,NULL,hash_type,&chash);

    OCTET_INIT(&dbmask,olen-seedlen);
    OCTET_INIT(&seed,seedlen);

    x=f->val[0];
    for (i=seedlen;i<olen;i++)
        dbmask.val[i-seedlen]=f->val[i+1]; 
    dbmask.len=olen-seedlen;

    MGF1(&dbmask,seedlen,hash_type,&seed);
    for (i=0;i<seedlen;i++) seed.val[i]^=f->val[i+1];
    MGF1(&seed,olen-seedlen,hash_type,m);
    OCTET_XOR(m,&dbmask);
    comp=OCTET_NCOMPARE(&chash,&dbmask,hlen);
    OCTET_SHIFT_LEFT(&dbmask,hlen);

    OCTET_KILL(&seed);
    OCTET_KILL(&chash);
    for (k=0;;k++)
    {
        if (k>=dbmask.len)
        {
            OCTET_KILL(&dbmask);
            return FALSE;
        }
        if (dbmask.val[k]!=0) break;
    }
    t=dbmask.val[k];
    if (!comp || x!=0 || t!=0x01) 
    {
        OCTET_KILL(&dbmask);
        return FALSE;
    }
    OCTET_SHIFT_LEFT(&dbmask,k+1);

    OCTET_COPY(&dbmask,m);
    OCTET_KILL(&dbmask);

    return TRUE;
}

/*** P1363 Key Derivation Functions ***/

P1363_API void KDF1(octet *z,octet *p,int hash_type,octet *k)
{
    hash(z,NULL,p,NULL,NULL,hash_type,k);
}

P1363_API BOOL KDF2(octet *z,octet *p,int olen,int hash_type,octet *k)
{
/* NOTE: the parameter olen is the length of the output k in bytes */

    int counter,cthreshold;
    int hlen;
    octet h;
    
    hlen=hash_params(hash_type,NULL);
    if (hlen==0) return FALSE;
    cthreshold=MR_ROUNDUP(olen,hlen);

    OCTET_EMPTY(k);
    OCTET_INIT(&h,hlen);
    for (counter=1;counter<=cthreshold;counter++)
    {
        hash(z,&counter,p,NULL,NULL,hash_type,&h);
        if (k->len+hlen>olen)  OCTET_JOIN_BYTES(h.val,olen%hlen,k);
        else                   OCTET_JOIN_OCTET(&h,k);
    }
    OCTET_KILL(&h);
    return TRUE;
}

/*** P1363 Message Authentication codes ***/

P1363_API BOOL MAC1(octet *m,FILE *fp,octet *k,int olen,int hash_type,octet *tag)
{
/* Input is either from an octet m, or a file fp.          *
 * olen is requested output length in bytes. k is the key  *
 * The output is the calculated tag */
    int hlen,b;
    octet h,k0;

    hlen=hash_params(hash_type,&b);
    if (hlen==0 || k->len<hlen/2) return FALSE;
    if (m!=NULL && fp!=NULL) return FALSE;
    if (olen<4 || olen>hlen) return FALSE;

    OCTET_INIT(&k0,b);
    OCTET_INIT(&h,hlen);

    if (k->len > b) hash(k,NULL,NULL,NULL,NULL,hash_type,&k0);
    else            OCTET_COPY(k,&k0);

    OCTET_JOIN_BYTE(0,b-k0.len,&k0);

    OCTET_XOR_BYTE(&k0,0x36);
    hash(&k0,NULL,m,fp,NULL,hash_type,&h);

    OCTET_XOR_BYTE(&k0,0x6a);   /* 0x6a = 0x36 ^ 0x5c */
    hash(&k0,NULL,&h,NULL,NULL,hash_type,&h);

    OCTET_EMPTY(tag);
    OCTET_JOIN_BYTES(h.val,olen,tag);

    OCTET_KILL(&h);
    OCTET_KILL(&k0);

    return TRUE;
}

P1363_API BOOL AES_CBC_IV0_DECRYPT(octet *k,octet *c,FILE *ifp,octet *m,FILE *ofp)
{
    aes a;
    int i,ipt,opt,ch;
    char buff[16];
    BOOL fin,bad;
    int padlen;
    ipt=opt=0;
    if (m!=NULL)
    {
        if (ofp!=NULL) return FALSE;
        OCTET_CLEAR(m);
    }
    if (c!=NULL)
    {
        if (ifp!=NULL) return FALSE;
        if (c->len==0) return FALSE;
        else ch=c->val[ipt++]; 
    }
    if (!aes_init(&a,MR_CBC,k->len,k->val,NULL)) return FALSE;
    if (ifp!=NULL)
    {
        ch=fgetc(ifp);
        if (ch==EOF) return FALSE; 
    }
    fin=FALSE;

    forever
    {
        for (i=0;i<16;i++)
        {
            if (c!=NULL) 
            {
                buff[i]=ch;      
                if (ipt>=c->len) {fin=TRUE; break;}  
                else ch=c->val[ipt++];  
            }
            if (ifp!=NULL) 
            { 
                buff[i]=ch;
                if ((ch=fgetc(ifp))==EOF) {fin=TRUE; break;}
            }
        }
        aes_decrypt(&a,buff);
        if (fin) break;
        for (i=0;i<16;i++)
        {
            if (m!=NULL) if (opt<m->max) m->val[opt++]=buff[i];
            if (ofp!=NULL) fputc(buff[i],ofp); 
        }
    }    
    aes_end(&a);
    bad=FALSE;
    padlen=buff[15];
    if (i!=15 || padlen<1 || padlen>16) bad=TRUE;
    if (padlen>=2 && padlen<=16)
        for (i=16-padlen;i<16;i++) if (buff[i]!=padlen) bad=TRUE;
    
    if (!bad) for (i=0;i<16-padlen;i++)
    {
        if (m!=NULL) if (opt<m->max) m->val[opt++]=buff[i];
        if (ofp!=NULL) fputc(buff[i],ofp); 
    }
    if (m!=NULL) m->len=opt;
    if (bad) return FALSE;
    return TRUE;
}

P1363_API BOOL AES_CBC_IV0_ENCRYPT(octet *k,octet *m,FILE *ifp,octet *c,FILE *ofp)
{ /* AES CBC encryption, with Null IV */
  /* Input is from either an octet string m, or from a file ifp, 
     output is to an octet string c, or a file ofp */
    aes a;
    int i,j,ipt,opt,ch;
    char buff[16];
    BOOL fin;
    int padlen;
    if (m!=NULL && ifp!=NULL) return FALSE;
    if (c!=NULL)
    {
        if (ofp!=NULL) return FALSE;
        OCTET_CLEAR(c);
    }
    if (!aes_init(&a,MR_CBC,k->len,k->val,NULL)) return FALSE;

    ipt=opt=0;
    fin=FALSE;
    forever
    {
        for (i=0;i<16;i++)
        {
            if (m!=NULL) 
            {
                if (ipt<m->len) buff[i]=m->val[ipt++];
                else {fin=TRUE; break;}
            }
            if (ifp!=NULL) 
            { 
                if ((ch=fgetc(ifp))!=EOF) buff[i]=ch;
                else {fin=TRUE; break;}
            }
        }
        if (fin) break;
        aes_encrypt(&a,buff);
        for (i=0;i<16;i++)
        {
            if (c!=NULL) if (opt<c->max) c->val[opt++]=buff[i];
            if (ofp!=NULL) fputc(buff[i],ofp); 
        }   
    }    

/* last block, filled up to i-th index */

    padlen=16-i;
    for (j=i;j<16;j++) buff[j]=padlen;
    aes_encrypt(&a,buff);
    for (i=0;i<16;i++)
    {
        if (c!=NULL) if (opt<c->max) c->val[opt++]=buff[i];
        if (ofp!=NULL) fputc(buff[i],ofp);
    }   
    aes_end(&a);
    if (c!=NULL) c->len=opt;    
    return TRUE;
}

/*** DL primitives - support functions ***/
/* Destroy the DL Domain structure */

P1363_API void DL_DOMAIN_KILL(dl_domain *DOM)
{
    OCTET_KILL(&DOM->Q);
    OCTET_KILL(&DOM->R);
    OCTET_KILL(&DOM->G);
    OCTET_KILL(&DOM->K);
    OCTET_KILL(&DOM->IK);
#ifndef MR_STATIC
    if (DOM->PC.window!=0) 
        brick_end(&DOM->PC); 
#endif
    DOM->words=DOM->fsize=0; DOM->H=DOM->rbits=0;
}

/* Initialise the DL domain structure
 * It is assumed that the DL domain details are obtained from a file
 * or from an array of strings 
 * multi-precision numbers are read in in Hex
 * A suitable file can be generated offline by the MIRACL example program
 * dssetup.c   
 * Set precompute=window size if a precomputed table is to be used to 
 * speed up the calculation g^x mod p 
 * Returns recommended number of bytes for use with octet strings */

P1363_API int DL_DOMAIN_INIT(dl_domain *DOM,char *fname,char **params,int precompute)
{ /* get domain details from specified file, OR from an array of strings   *
   * If input from a file, params=NULL, if input from strings, fname=NULL  *
   * returns filed size in bytes                                           *
   * precompute is 0 for no precomputation, or else window size for Comb method */ 
    FILE *fp;
    BOOL tt1,fileinput=TRUE;
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
#endif
    miracl *mr_mip;
    big q,r,g,k;
    int bits,rbits,bytes,rbytes,err,res=0;

#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(4)];
    memset(mem,0,MR_BIG_RESERVE(4));
#endif
    
    if (fname==NULL && params==NULL) return MR_P1363_DOMAIN_NOT_FOUND;
    if (fname==NULL) fileinput=FALSE;

    if (fileinput)
    {
        fp=fopen(fname,"rt");
        if (fp==NULL) return MR_P1363_DOMAIN_NOT_FOUND;
        fscanf(fp,"%d\n",&bits);
    }
    else
        sscanf(params[0],"%d\n",&bits); 

    DOM->words=MR_ROUNDUP(bits,MIRACL);
#ifdef MR_GENERIC_AND_STATIC  
	mr_mip=mirsys(&instance,MR_ROUNDUP(bits,4),16);
#else
    mr_mip=mirsys(MR_ROUNDUP(bits,4),16);
#endif
	if (mr_mip==NULL) 
    {
        if (fileinput) fclose(fp);
        return MR_P1363_OUT_OF_MEMORY;
    }

#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 4);
    if (mem==NULL)
    {
        if (fileinput) fclose(fp);
        res=MR_P1363_OUT_OF_MEMORY;
    }
#endif

    mr_mip->ERCON=TRUE;

    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem,0);
        r=mirvar_mem(_MIPP_ mem,1);
        g=mirvar_mem(_MIPP_ mem,2);
        k=mirvar_mem(_MIPP_ mem,3);

        bytes=MR_ROUNDUP(bits,8);

        if (fileinput)
        {
            innum(_MIPP_ q,fp);
            innum(_MIPP_ r,fp);
            innum(_MIPP_ g,fp);  
            fclose(fp);
        }
        else
        {
            instr(_MIPP_ q,params[1]);
            instr(_MIPP_ r,params[2]);
            instr(_MIPP_ g,params[3]);
        } 
         
        rbits=logb2(_MIPP_ r);       /* r is usually much smaller than q */
        rbytes=MR_ROUNDUP(rbits,8);

        OCTET_INIT(&DOM->Q,bytes);
        OCTET_INIT(&DOM->R,rbytes);
        OCTET_INIT(&DOM->G,bytes);
        OCTET_INIT(&DOM->K,bytes);
        OCTET_INIT(&DOM->IK,rbytes);
        DOM->H=(1+rbits)/2;
        DOM->fsize=bytes;
        DOM->rbits=rbits;
        decr(_MIPP_ q,1,k);
        divide(_MIPP_ k,r,k);    /* k=(q-1)/r */
    
        convert_big_octet(_MIPP_ q,&DOM->Q);
        convert_big_octet(_MIPP_ r,&DOM->R);
        convert_big_octet(_MIPP_ g,&DOM->G);
        convert_big_octet(_MIPP_ k,&DOM->K);
        xgcd(_MIPP_ k,r,k,k,k);
        convert_big_octet(_MIPP_ k,&DOM->IK);
        DOM->PC.window=0;
#ifndef MR_STATIC
        if (precompute)
            brick_init(_MIPP_ &DOM->PC,g,q,precompute,logb2(_MIPP_ r));
#endif
	}
#ifndef MR_STATIC
    memkill(_MIPP_ mem,4);
#else
    memset(mem,0,MR_BIG_RESERVE(4));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );

    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    if (res<0) return res;
    else return bytes;
}

/* validate DL domain details - good idea if you got them from
 * some-one else! */

P1363_API int DL_DOMAIN_VALIDATE(BOOL (*idle)(void),dl_domain *DOM)
{ /* do domain checks - IEEE P1363 A16.2 */
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,r,g,t;
    int err,res=0;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(4)];
    memset(mem,0,MR_BIG_RESERVE(4));
#endif

    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;

    mr_mip->ERCON=TRUE;
    mr_mip->NTRY=50;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 4);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif

	if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem,0);
        r=mirvar_mem(_MIPP_ mem,1);
        g=mirvar_mem(_MIPP_ mem,2);
        t=mirvar_mem(_MIPP_ mem,3);
        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->G,g);
        if (size(g)<2 || size(q)<=2 || size(r)<=2) res=MR_P1363_DOMAIN_ERROR; 
        if (compare(g,q)>=0) res=MR_P1363_DOMAIN_ERROR;
    }
    if (res==0)
    {
        gprime(_MIPP_ 10000);
        if (!isprime(_MIPP_ q)) res=MR_P1363_DOMAIN_ERROR;
    }
    if (res==0) 
    {
        if (!isprime(_MIPP_ r)) res=MR_P1363_DOMAIN_ERROR;
    }
    if (res==0)
    { /* is g of order r? */
       powmod(_MIPP_ g,r,q,t);
       if (size(t)!=1) res=MR_P1363_DOMAIN_ERROR;
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,4);
#else
    memset(mem,0,MR_BIG_RESERVE(4));
#endif
    err=mr_mip->ERNUM;

    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

/* Calculate a public/private DL key pair. W=g^S mod p 
 * where S is the private key and W the public key 
 * If RNG is NULL then the private key is provided externally in S
 * otherwise it is generated randomly internally
 */

P1363_API int DL_KEY_PAIR_GENERATE(BOOL (*idle)(void),dl_domain *DOM,csprng *RNG, octet *S,octet *W)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,r,g,s,w;
    int err,res=0;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(5)];
    memset(mem,0,MR_BIG_RESERVE(5));
#endif

    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 5);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem,0);
        r=mirvar_mem(_MIPP_ mem,1);
        g=mirvar_mem(_MIPP_ mem,2);
        s=mirvar_mem(_MIPP_ mem,3);
        w=mirvar_mem(_MIPP_ mem,4);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->G,g);
        if (RNG!=NULL)
        {
            strong_bigrand(_MIPP_ RNG,r,s);
        }
        else
        {
            OS2FEP(_MIPP_ S,s);
            divide(_MIPP_ s,r,r);
        }
        if (DOM->PC.window==0) 
            powmod(_MIPP_ g,s,q,w);
        else
            pow_brick(_MIPP_ &DOM->PC,s,w);

        if (RNG!=NULL) convert_big_octet(_MIPP_ s,S);
        FE2OSP(_MIPP_ w,DOM->fsize,W);
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,5);
#else
    memset(mem,0,MR_BIG_RESERVE(5));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

/* validate a DL public key. Set full=TRUE for fuller, 
 * but more time-consuming test */

P1363_API int DL_PUBLIC_KEY_VALIDATE(BOOL (*idle)(void),dl_domain *DOM,BOOL full,octet *W)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,r,w,t;
    int err,res=0;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(4)];
    memset(mem,0,MR_BIG_RESERVE(4));
#endif

    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 4);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem,0);
        r=mirvar_mem(_MIPP_ mem,1);
        w=mirvar_mem(_MIPP_ mem,2);
        t=mirvar_mem(_MIPP_ mem,3);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ W,w);
        if (size(w)<2 || compare(w,q)>=0) res=MR_P1363_INVALID_PUBLIC_KEY;
    }
    if (res==0 && full) 
    {
        powmod(_MIPP_ w,r,q,t);
        if (size(t)!=1) res=MR_P1363_INVALID_PUBLIC_KEY;
    }   
#ifndef MR_STATIC
    memkill(_MIPP_ mem,4);
#else
    memset(mem,0,MR_BIG_RESERVE(4));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

/*** P1363 DL primitives ***/
/* See P1363 documentation for specification */

P1363_API int DLSVDP_DH(BOOL (*idle)(void),dl_domain *DOM,octet *S,octet *WD,octet *Z) 
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,s,wd,z;
    int err,res=0;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(4)];
    memset(mem,0,MR_BIG_RESERVE(4));
#endif

    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 4);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem,0);
        s=mirvar_mem(_MIPP_ mem,1);
        wd=mirvar_mem(_MIPP_ mem,2);
        z=mirvar_mem(_MIPP_ mem,3);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ S,s);
        OS2FEP(_MIPP_ WD,wd);

        powmod(_MIPP_ wd,s,q,z);

        FE2OSP(_MIPP_ z,DOM->fsize,Z);
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,4);
#else
    memset(mem,0,MR_BIG_RESERVE(4));
#endif
    err=mr_mip->ERNUM;

    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int DLSVDP_DHC(BOOL (*idle)(void),dl_domain *DOM,octet *S,octet *WD,BOOL compatible,octet *Z)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,r,k,ik,s,wd,t,z;
    int sz;
    int err,res=0;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(8)];
    memset(mem,0,MR_BIG_RESERVE(8));
#endif


    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 8);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem,0);
        r=mirvar_mem(_MIPP_ mem,1);
        k=mirvar_mem(_MIPP_ mem,2);
        ik=mirvar_mem(_MIPP_ mem,3);
        s=mirvar_mem(_MIPP_ mem,4);
        wd=mirvar_mem(_MIPP_ mem,5);
        z=mirvar_mem(_MIPP_ mem,6);
        t=mirvar_mem(_MIPP_ mem,7);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->K,k);
        OS2FEP(_MIPP_ S,s);
        OS2FEP(_MIPP_ WD,wd);

        if (compatible)
        {
            OS2FEP(_MIPP_ &DOM->IK,ik);
            mad(_MIPP_ ik,s,ik,r,r,t);   /* t=s/k mod r */
        }
        else copy(s,t);
        multiply(_MIPP_ t,k,t);        /* kt */
        powmod(_MIPP_ wd,t,q,z);

        sz=size(z);
        if (sz==0 || sz==1) res=MR_P1363_INVALID_PUBLIC_KEY;
        else FE2OSP(_MIPP_ z,DOM->fsize,Z);
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,8);
#else
    memset(mem,0,MR_BIG_RESERVE(8));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int DLSVDP_MQV(BOOL (*idle)(void),dl_domain *DOM,octet *S,octet *U,octet *V,octet *WD,octet *VD,octet *Z)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    int h;
    int err,res=0;
    big q,r,s,u,v,wd,vd,e,t,td,z;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(11)];
    memset(mem,0,MR_BIG_RESERVE(11));
#endif

    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 11);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        r=mirvar_mem(_MIPP_ mem, 1);
        s=mirvar_mem(_MIPP_ mem, 2);
        u=mirvar_mem(_MIPP_ mem, 3);
        v=mirvar_mem(_MIPP_ mem, 4);
        wd=mirvar_mem(_MIPP_ mem, 5);
        vd=mirvar_mem(_MIPP_ mem, 6);
        e=mirvar_mem(_MIPP_ mem, 7);
        t=mirvar_mem(_MIPP_ mem, 8);
        td=mirvar_mem(_MIPP_ mem, 9);
        z=mirvar_mem(_MIPP_ mem, 10);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ S,s);
        OS2FEP(_MIPP_ U,u);
        OS2FEP(_MIPP_ V,v);
        OS2FEP(_MIPP_ WD,wd);
        OS2FEP(_MIPP_ VD,vd);

        h=DOM->H;
        expb2(_MIPP_ h,z);

        copy(v,t);
        divide(_MIPP_ t,z,z);
        add(_MIPP_ t,z,t);
        copy(vd,td);
        divide(_MIPP_ td,z,z);
        add(_MIPP_ td,z,td);

        mad(_MIPP_ t,s,u,r,r,e);
        mad(_MIPP_ e,td,td,r,r,t);
        powmod2(_MIPP_ vd,e,wd,t,q,z);

        if (size(z)==1) res= MR_P1363_ERROR;
        else FE2OSP(_MIPP_ z,DOM->fsize,Z);
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,11);
#else
    memset(mem,0,MR_BIG_RESERVE(11));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );

    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int DLSVDP_MQVC(BOOL (*idle)(void),dl_domain *DOM,octet *S,octet *U,octet *V,octet *WD,octet *VD,BOOL compatible,octet *Z)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    int sz,h;
    big q,r,s,u,v,wd,vd,e,t,td,z,k,ik;
    int err,res=0;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(13)];
    memset(mem,0,MR_BIG_RESERVE(13));
#endif

    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 13);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        r=mirvar_mem(_MIPP_ mem, 1);
        ik=mirvar_mem(_MIPP_ mem, 2);
        k=mirvar_mem(_MIPP_ mem, 3);
        s=mirvar_mem(_MIPP_ mem, 4);
        u=mirvar_mem(_MIPP_ mem, 5);
        v=mirvar_mem(_MIPP_ mem, 6);
        wd=mirvar_mem(_MIPP_ mem, 7);
        vd=mirvar_mem(_MIPP_ mem, 8);
        e=mirvar_mem(_MIPP_ mem, 9);
        t=mirvar_mem(_MIPP_ mem, 10);
        td=mirvar_mem(_MIPP_ mem, 11);
        z=mirvar_mem(_MIPP_ mem, 12);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->K,k);
    
        OS2FEP(_MIPP_ S,s);
        OS2FEP(_MIPP_ U,u);
        OS2FEP(_MIPP_ V,v);
        OS2FEP(_MIPP_ WD,wd);
        OS2FEP(_MIPP_ VD,vd);

        h=DOM->H;
        expb2(_MIPP_ h,z);

        copy(v,t);
        divide(_MIPP_ t,z,z);
        add(_MIPP_ t,z,t);
        copy(vd,td);
        divide(_MIPP_ td,z,z);
        add(_MIPP_ td,z,td);

        mad(_MIPP_ t,s,u,r,r,e);
        if (compatible) 
        {
            OS2FEP(_MIPP_ &DOM->IK,ik);
            mad(_MIPP_ e,ik,e,r,r,e);
        }
        mad(_MIPP_ e,k,e,r,r,e);
        mad(_MIPP_ e,td,td,r,r,t);
        powmod2(_MIPP_ vd,e,wd,t,q,z);

        sz=size(z);      
        if (sz==0 || sz==1) res=MR_P1363_INVALID_PUBLIC_KEY;
        FE2OSP(_MIPP_ z,DOM->fsize,Z);
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,13);
#else
    memset(mem,0,MR_BIG_RESERVE(13));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int DLVP_NR(BOOL (*idle)(void),dl_domain *DOM,octet *W,octet *C,octet *D,octet *F)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,r,g,w,f,c,d;
    int err,res=0;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(7)];
    memset(mem,0,MR_BIG_RESERVE(7));
#endif

    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 7);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        r=mirvar_mem(_MIPP_ mem, 1);
        g=mirvar_mem(_MIPP_ mem, 2);
        w=mirvar_mem(_MIPP_ mem, 3);
        f=mirvar_mem(_MIPP_ mem, 4);
        c=mirvar_mem(_MIPP_ mem, 5);
        d=mirvar_mem(_MIPP_ mem, 6);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->G,g);
        OS2FEP(_MIPP_ W,w);
        OS2FEP(_MIPP_ C,c);
        OS2FEP(_MIPP_ D,d);
        if (size(c)<1 || compare(c,r)>=0 || size(d)<0 || compare(d,r)>=0) 
            res=MR_P1363_INVALID;
    }
    if (res==0)
    {
        powmod2(_MIPP_ g,d,w,c,q,f);
        divide(_MIPP_ f,r,r);
        subtract(_MIPP_ c,f,f);
        if (size(f)<0) add(_MIPP_ f,r,f);
        convert_big_octet(_MIPP_ f,F);
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,7);
#else
    memset(mem,0,MR_BIG_RESERVE(7));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int DLSP_NR(BOOL (*idle)(void),dl_domain *DOM,csprng *RNG,octet *S,octet *F,octet *C,octet *D)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,r,g,s,f,c,d,u,v;
    int err,res=0;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(9)];
    memset(mem,0,MR_BIG_RESERVE(9));
#endif

    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 9);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        r=mirvar_mem(_MIPP_ mem, 1);
        g=mirvar_mem(_MIPP_ mem, 2);
        s=mirvar_mem(_MIPP_ mem, 3);
        f=mirvar_mem(_MIPP_ mem, 4);
        c=mirvar_mem(_MIPP_ mem, 5);
        d=mirvar_mem(_MIPP_ mem, 6);
        u=mirvar_mem(_MIPP_ mem, 7);
        v=mirvar_mem(_MIPP_ mem, 8);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->G,g);
        OS2FEP(_MIPP_ S,s);
        OS2FEP(_MIPP_ F,f);
        if (compare(f,r)>=0) res=MR_P1363_BAD_ASSUMPTION;
    }
    if (res==0)
    {
        do {
            if (mr_mip->ERNUM) break;
            strong_bigrand(_MIPP_ RNG,r,u);

            if (DOM->PC.window==0) 
                powmod(_MIPP_ g,u,q,v);
            else
                pow_brick(_MIPP_ &DOM->PC,u,v);

            add(_MIPP_ v,f,c);
            divide(_MIPP_ c,r,r);
        } while (size(c)==0);
    }
    if (res==0)
    {    
        mad(_MIPP_ s,c,s,r,r,d);
        subtract(_MIPP_ u,d,d);
        if (size(d)<0) add(_MIPP_ d,r,d);

        convert_big_octet(_MIPP_ c,C);
        convert_big_octet(_MIPP_ d,D);
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,9);
#else
    memset(mem,0,MR_BIG_RESERVE(9));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int DLVP_DSA(BOOL (*idle)(void),dl_domain *DOM,octet *W,octet *C,octet *D,octet *F)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,r,g,w,f,c,d,h2;
    int err,res=0;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(8)];
    memset(mem,0,MR_BIG_RESERVE(8));
#endif

    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 8);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        r=mirvar_mem(_MIPP_ mem, 1);
        g=mirvar_mem(_MIPP_ mem, 2);
        w=mirvar_mem(_MIPP_ mem, 3);
        f=mirvar_mem(_MIPP_ mem, 4);
        c=mirvar_mem(_MIPP_ mem, 5);
        d=mirvar_mem(_MIPP_ mem, 6);
        h2=mirvar_mem(_MIPP_ mem, 7);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->G,g);
        OS2FEP(_MIPP_ W,w);
        OS2FEP(_MIPP_ C,c);
        OS2FEP(_MIPP_ D,d);
        OS2FEP(_MIPP_ F,f);
        if (size(c)<1 || size(d)<1 || compare(c,r)>=0 || compare(d,r)>=0) 
            res=MR_P1363_INVALID;
    }
    if (res==0)
    {
        xgcd(_MIPP_ d,r,d,d,d);
        mad(_MIPP_ f,d,f,r,r,f);
        mad(_MIPP_ c,d,c,r,r,h2);
        powmod2(_MIPP_ g,f,w,h2,q,d);
        divide(_MIPP_ d,r,r);
        if (compare(d,c)!=0) res=MR_P1363_INVALID;
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,8);
#else
    memset(mem,0,MR_BIG_RESERVE(8));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int DLSP_DSA(BOOL (*idle)(void),dl_domain *DOM,csprng *RNG,octet *S,octet *F,octet *C,octet *D)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,r,g,s,f,c,d,u,v;
    int err,res=0;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(9)];
    memset(mem,0,MR_BIG_RESERVE(9));
#endif

    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=FALSE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 9);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        r=mirvar_mem(_MIPP_ mem, 1);
        g=mirvar_mem(_MIPP_ mem, 2);
        s=mirvar_mem(_MIPP_ mem, 3);
        f=mirvar_mem(_MIPP_ mem, 4);
        c=mirvar_mem(_MIPP_ mem, 5);
        d=mirvar_mem(_MIPP_ mem, 6);
        u=mirvar_mem(_MIPP_ mem, 7);
        v=mirvar_mem(_MIPP_ mem, 8);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->G,g);
        OS2FEP(_MIPP_ S,s);
        OS2FEP(_MIPP_ F,f);

        do {
            if (mr_mip->ERNUM) break;
            strong_bigrand(_MIPP_ RNG,r,u);

            if (DOM->PC.window==0) 
                powmod(_MIPP_ g,u,q,v);
            else
                pow_brick(_MIPP_ &DOM->PC,u,v);

            copy(v,c); 
            divide(_MIPP_ c,r,r);
            if (size(c)==0) continue;
            xgcd(_MIPP_ u,r,u,u,u);
            mad(_MIPP_ s,c,f,r,r,d);
            mad(_MIPP_ u,d,u,r,r,d);
        } while (size(d)==0);
        if (res==0)
        {
            convert_big_octet(_MIPP_ c,C);
            convert_big_octet(_MIPP_ d,D);
        }
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,9);
#else
    memset(mem,0,MR_BIG_RESERVE(9));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int DLPSP_NR2PV(BOOL (*idle)(void),dl_domain *DOM,csprng *RNG,octet *U,octet *V)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,r,g,u,v;
    int err,res=0;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(5)];
    memset(mem,0,MR_BIG_RESERVE(5));
#endif

    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=FALSE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 5);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        r=mirvar_mem(_MIPP_ mem, 1);
        g=mirvar_mem(_MIPP_ mem, 2);
        u=mirvar_mem(_MIPP_ mem, 3);
        v=mirvar_mem(_MIPP_ mem, 4);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->G,g);
        strong_bigrand(_MIPP_ RNG,r,u);

        if (DOM->PC.window==0) 
            powmod(_MIPP_ g,u,q,v);
        else
            pow_brick(_MIPP_ &DOM->PC,u,v);

        convert_big_octet(_MIPP_ u,U);
        FE2OSP(_MIPP_ v,DOM->fsize,V);
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,5);
#else
    memset(mem,0,MR_BIG_RESERVE(5));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int DLSP_NR2(BOOL (*idle)(void),dl_domain *DOM,octet *S,octet *U,octet *V,octet *F,octet *C,octet *D)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,r,u,v,c,d,s,f;
    int err,res=0;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(8)];
    memset(mem,0,MR_BIG_RESERVE(8));
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=FALSE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 8);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        r=mirvar_mem(_MIPP_ mem, 1);
        s=mirvar_mem(_MIPP_ mem, 2);
        f=mirvar_mem(_MIPP_ mem, 3);
        u=mirvar_mem(_MIPP_ mem, 4);
        v=mirvar_mem(_MIPP_ mem, 5);
        c=mirvar_mem(_MIPP_ mem, 6);
        d=mirvar_mem(_MIPP_ mem, 7);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ S,s);
        OS2FEP(_MIPP_ F,f);
        OS2FEP(_MIPP_ U,u);
        OS2FEP(_MIPP_ V,v);
        if (compare(f,r)>=0) res=MR_P1363_BAD_ASSUMPTION;
        if (size(u)<1 || compare(u,r)>=0) res=MR_P1363_BAD_ASSUMPTION;
        if (size(v)<1 || compare(v,q)>=0) res=MR_P1363_BAD_ASSUMPTION;
    }

    if (res==0)
    {
        add(_MIPP_ v,f,c);
        divide(_MIPP_ c,r,r);
        if (size(c)==0) res=MR_P1363_ERROR;
    }
    if (res==0)
    {
        mad(_MIPP_ s,c,s,r,r,d);
        subtract(_MIPP_ u,d,d);
        if (size(d)<0) add(_MIPP_ d,r,d);
 
        convert_big_octet(_MIPP_ c,C);
        convert_big_octet(_MIPP_ d,D);
    }

    OCTET_CLEAR(U);
#ifndef MR_STATIC
    memkill(_MIPP_ mem,8);
#else
    memset(mem,0,MR_BIG_RESERVE(8));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int DLVP_NR2(BOOL (*idle)(void),dl_domain *DOM,octet *W,octet *C,octet *D,octet *F,octet *V)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,r,g,c,d,w,f,v;
    int err,res=0;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(8)];
    memset(mem,0,MR_BIG_RESERVE(8));
#endif
 
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=FALSE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 8);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        r=mirvar_mem(_MIPP_ mem, 1);
        g=mirvar_mem(_MIPP_ mem, 2);
        w=mirvar_mem(_MIPP_ mem, 3);
        c=mirvar_mem(_MIPP_ mem, 4);
        d=mirvar_mem(_MIPP_ mem, 5);
        f=mirvar_mem(_MIPP_ mem, 6);
        v=mirvar_mem(_MIPP_ mem, 7);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->G,g);
        OS2FEP(_MIPP_ W,w);
        OS2FEP(_MIPP_ C,c);
        OS2FEP(_MIPP_ D,d);
        if (size(c)<1 || compare(c,r)>=0 || compare(d,r)>=0) res=MR_P1363_INVALID;
    }

    if (res==0)
    {
        powmod2(_MIPP_ g,d,w,c,q,v);
        copy(v,f);
        divide(_MIPP_ f,r,r);
        subtract(_MIPP_ c,f,f);
        if (size(f)<0) add(_MIPP_ f,r,f);
        convert_big_octet(_MIPP_ f,F);
        FE2OSP(_MIPP_ v,DOM->fsize,V);
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,8);
#else
    memset(mem,0,MR_BIG_RESERVE(8));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int DLSP_PV(BOOL (*idle)(void),dl_domain *DOM,octet *S,octet *U,octet *H,octet *D)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,r,h,u,d,s;
    int err,res=0;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(6)];
    memset(mem,0,MR_BIG_RESERVE(6));
#endif
 
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=FALSE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 6);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        r=mirvar_mem(_MIPP_ mem, 1);
        s=mirvar_mem(_MIPP_ mem, 2);
        u=mirvar_mem(_MIPP_ mem, 3);
        h=mirvar_mem(_MIPP_ mem, 4);
        d=mirvar_mem(_MIPP_ mem, 5);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ S,s);
        OS2FEP(_MIPP_ U,u);
        OS2FEP(_MIPP_ H,h);
        if (compare(u,r)>=0) res=MR_P1363_BAD_ASSUMPTION;
    }

    if (res==0)
    {
        mad(_MIPP_ s,h,s,r,r,d);
        subtract(_MIPP_ u,d,d);
        if (size(d)<0) add(_MIPP_ d,r,d);
 
        convert_big_octet(_MIPP_ d,D);
    }

    OCTET_CLEAR(U);
#ifndef MR_STATIC
    memkill(_MIPP_ mem,6);
#else
    memset(mem,0,MR_BIG_RESERVE(6));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int DLVP_PV(BOOL (*idle)(void),dl_domain *DOM,octet *W,octet *H,octet *D,octet *V)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,r,g,d,w,h,v;
    int err,res=0;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(7)];
    memset(mem,0,MR_BIG_RESERVE(7));
#endif
 
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=FALSE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 7);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        r=mirvar_mem(_MIPP_ mem, 1);
        g=mirvar_mem(_MIPP_ mem, 2);
        w=mirvar_mem(_MIPP_ mem, 3);
        h=mirvar_mem(_MIPP_ mem, 4);
        d=mirvar_mem(_MIPP_ mem, 5);
        v=mirvar_mem(_MIPP_ mem, 6);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->G,g);
        OS2FEP(_MIPP_ W,w);
        OS2FEP(_MIPP_ D,d);
        OS2FEP(_MIPP_ H,h);
        if (compare(d,r)>=0) res=MR_P1363_INVALID;
    }

    if (res==0)
    {
        powmod2(_MIPP_ g,d,w,h,q,v);
        FE2OSP(_MIPP_ v,DOM->fsize,V);
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,7);
#else
    memset(mem,0,MR_BIG_RESERVE(7));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

/*** EC GF(p) primitives - support functions ***/
/* destroy the EC GF(p) domain structure */

P1363_API void ECP_DOMAIN_KILL(ecp_domain *DOM)
{
    OCTET_KILL(&DOM->Q);
    OCTET_KILL(&DOM->A);
    OCTET_KILL(&DOM->B);
    OCTET_KILL(&DOM->R);
    OCTET_KILL(&DOM->Gx);
    OCTET_KILL(&DOM->Gy);
    OCTET_KILL(&DOM->K);
    OCTET_KILL(&DOM->IK);
#ifndef MR_STATIC
    if (DOM->PC.window!=0) 
        ebrick_end(&DOM->PC);  
#endif
    DOM->words=DOM->fsize=0; DOM->H=DOM->rbits=0;
}

/* Initialise the EC GF(p) domain structure
 * It is assumed that the EC domain details are obtained from a file
 * or from an array of strings 
 * multiprecision numbers are read in in Hex
 * A suitable file can be generated offline by the MIRACL example program
 * schoof.exe   
 * Set precompute=window size if a precomputed table is to be used to 
 * speed up the calculation x.G mod EC(p) 
 * Returns field size in bytes                                       */

P1363_API int ECP_DOMAIN_INIT(ecp_domain *DOM,char *fname,char **params,int precompute)
{ /* get domain details from specified file     */
  /* If input from a file, params=NULL, if input from strings, fname=NULL  */
  /* return max. size in bytes of octet strings */
  /* precompute is 0 for no precomputation, or else window size for Comb method */ 
    FILE *fp;
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
#endif
    miracl *mr_mip;
    BOOL fileinput=TRUE;
    big q,r,gx,gy,a,b,k;
    int bits,rbits,bytes,err,res=0;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(7)];
    memset(mem,0,MR_BIG_RESERVE(7));
#endif
 
    if (fname==NULL && params==NULL) return MR_P1363_DOMAIN_NOT_FOUND; 
    if (fname==NULL) fileinput=FALSE;

    if (fileinput)
    {
        fp=fopen(fname,"rt");
        if (fp==NULL) return MR_P1363_DOMAIN_NOT_FOUND;
        fscanf(fp,"%d\n",&bits);
    }
    else
        sscanf(params[0],"%d\n",&bits);
    
    DOM->words=MR_ROUNDUP(bits,MIRACL);
#ifdef MR_GENERIC_AND_STATIC
	mr_mip=mirsys(&instance,MR_ROUNDUP(bits,4),16);
#else
    mr_mip=mirsys(MR_ROUNDUP(bits,4),16);
#endif
	if (mr_mip==NULL) 
    {
        if (fileinput) fclose(fp);
        return MR_P1363_OUT_OF_MEMORY;
    }
    mr_mip->ERCON=TRUE;
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 7);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        a=mirvar_mem(_MIPP_ mem, 1);
        b=mirvar_mem(_MIPP_ mem, 2);
        r=mirvar_mem(_MIPP_ mem, 3);
        gx=mirvar_mem(_MIPP_ mem, 4);
        gy=mirvar_mem(_MIPP_ mem, 5);
        k=mirvar_mem(_MIPP_ mem, 6);

        bytes=MR_ROUNDUP(bits,8);
       
        if (fileinput)
        {
            innum(_MIPP_ q,fp);
            innum(_MIPP_ a,fp);
            innum(_MIPP_ b,fp);
            innum(_MIPP_ r,fp);
            innum(_MIPP_ gx,fp); 
            innum(_MIPP_ gy,fp); 
            fclose(fp);

        }
        else
        {
            instr(_MIPP_ q,params[1]);
            instr(_MIPP_ a,params[2]);
            instr(_MIPP_ b,params[3]);
            instr(_MIPP_ r,params[4]);
            instr(_MIPP_ gx,params[5]); 
            instr(_MIPP_ gy,params[6]); 
        }
        
        OCTET_INIT(&DOM->Q,bytes);
        OCTET_INIT(&DOM->A,bytes);
        OCTET_INIT(&DOM->B,bytes);
        OCTET_INIT(&DOM->R,bytes);
        OCTET_INIT(&DOM->Gx,bytes);
        OCTET_INIT(&DOM->Gy,bytes);
        OCTET_INIT(&DOM->K,bytes);
        OCTET_INIT(&DOM->IK,bytes);
        rbits=logb2(_MIPP_ r);
        DOM->H=(1+rbits)/2;
        DOM->fsize=bytes;
        DOM->rbits=rbits;
        nroot(_MIPP_ q,2,k);
        premult(_MIPP_ k,2,k);
        add(_MIPP_ k,q,k);
        incr(_MIPP_ k,3,k);

        divide(_MIPP_ k,r,k);    /* get co-factor k = (q+2q^0.5+3)/r */
        if (size(a)<0) add(_MIPP_ q,a,a);
        if (size(b)<0) add(_MIPP_ q,b,b);

        convert_big_octet(_MIPP_ q,&DOM->Q);
        convert_big_octet(_MIPP_ a,&DOM->A);
        convert_big_octet(_MIPP_ b,&DOM->B);
        convert_big_octet(_MIPP_ r,&DOM->R);
        convert_big_octet(_MIPP_ gx,&DOM->Gx);
        convert_big_octet(_MIPP_ gy,&DOM->Gy);

        convert_big_octet(_MIPP_ k,&DOM->K);
        xgcd(_MIPP_ k,r,k,k,k);
        convert_big_octet(_MIPP_ k,&DOM->IK);
        DOM->PC.window=0;
#ifndef MR_STATIC
        if (precompute) ebrick_init(_MIPP_ &DOM->PC,gx,gy,a,b,q,precompute,logb2(_MIPP_ r));
#endif
	}
#ifndef MR_STATIC
    memkill(_MIPP_ mem,7);
#else
    memset(mem,0,MR_BIG_RESERVE(7));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    if (res<0) return res;
    else return bytes;
}

/* validate EC GF(p) domain details - good idea if you got them from
 * some-one else! */

P1363_API int ECP_DOMAIN_VALIDATE(BOOL (*idle)(void),ecp_domain *DOM)
{ /* do domain checks - A16.8 */
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,a,b,r,gx,gy,t,w;
    epoint *G;
    int i,err,res=0;
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(8)];
    char mem1[MR_ECP_RESERVE(1)];
    memset(mem,0,MR_BIG_RESERVE(8));
    memset(mem1,0,MR_ECP_RESERVE(1));
#endif
 
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    
    mr_mip->ERCON=TRUE;
    mr_mip->NTRY=50;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 8);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 1);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        a=mirvar_mem(_MIPP_ mem, 1);
        b=mirvar_mem(_MIPP_ mem, 2);
        r=mirvar_mem(_MIPP_ mem, 3);
        gx=mirvar_mem(_MIPP_ mem, 4);
        gy=mirvar_mem(_MIPP_ mem, 5);
        t=mirvar_mem(_MIPP_ mem, 6);
        w=mirvar_mem(_MIPP_ mem, 7);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->A,a);
        OS2FEP(_MIPP_ &DOM->B,b);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->Gx,gx);
        OS2FEP(_MIPP_ &DOM->Gy,gy);

        nroot(_MIPP_ q,2,t);
        premult(_MIPP_ t,4,t);

        if (compare(r,t)<=0) res=MR_P1363_DOMAIN_ERROR;
        if (compare(b,q)>=0) res=MR_P1363_DOMAIN_ERROR; 
        if (compare(r,q)==0) res=MR_P1363_DOMAIN_ERROR;
        if (size(r)<3 || size(q)<3) res=MR_P1363_DOMAIN_ERROR;
    }

    if (res==0)
    {
        gprime(_MIPP_ 10000);
        if (!isprime(_MIPP_ r)) res=MR_P1363_DOMAIN_ERROR;
    }

    if (res==0)
    {
        if (!isprime(_MIPP_ q)) res=MR_P1363_DOMAIN_ERROR;
    }

    if (res==0)
    { /* check 4a^3+27b^2 !=0 mod q */
        mad(_MIPP_ b,b,b,q,q,t);
        premult(_MIPP_ t,27,t);
        mad(_MIPP_ a,a,a,q,q,w);
        mad(_MIPP_ w,a,a,q,q,w);
        premult(_MIPP_ w,4,w);

        add(_MIPP_ t,w,t);
        divide(_MIPP_ t,q,q);
        if (size(t)<0) add(_MIPP_ t,q,t);
        if (size(t)==0) res=MR_P1363_DOMAIN_ERROR;  
    }

    if (res==0)
    {
        ecurve_init(_MIPP_ a,b,q,MR_AFFINE);   
        G=epoint_init_mem(_MIPP_ mem1,0);

        if (!epoint_set(_MIPP_ gx,gy,0,G)) res=MR_P1363_DOMAIN_ERROR;

		if (G->marker==MR_EPOINT_INFINITY) res=MR_P1363_DOMAIN_ERROR;
        else
        {
            ecurve_mult(_MIPP_ r,G,G);
            if (G->marker!=MR_EPOINT_INFINITY) res=MR_P1363_DOMAIN_ERROR;
        }
    }

    if (res==0)
    { /* MOV conditon */
        convert(_MIPP_ 1,t);
        for (i=0;i<50;i++)
        {
            mad(_MIPP_ t,q,q,r,r,t);
            if (size(t)==1)
            {
                res=MR_P1363_DOMAIN_ERROR;
                break;
            }
        }
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,8);
    ecp_memkill(_MIPP_ mem1,1);
#else
    memset(mem,0,MR_BIG_RESERVE(8));
    memset(mem1,0,MR_ECP_RESERVE(1));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

/* Calculate a public/private EC GF(p) key pair. W=S.g mod EC(p),
 * where S is the secret key and W is the public key
 * Indicate if "point compression" is required for public key
 * If RNG is NULL then the private key is provided externally in S
 * otherwise it is generated randomly internally */

P1363_API int ECP_KEY_PAIR_GENERATE(BOOL (*idle)(void),ecp_domain *DOM,csprng *RNG,octet* S,BOOL compress,octet *W)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,a,b,r,gx,gy,s,wx,wy;
    epoint *G,*WP;
    int bit,err,res=0;
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(9)];
    char mem1[MR_ECP_RESERVE(2)];
    memset(mem,0,MR_BIG_RESERVE(9));
    memset(mem1,0,MR_ECP_RESERVE(2));
#endif
 
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 9);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 2);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        a=mirvar_mem(_MIPP_ mem, 1);
        b=mirvar_mem(_MIPP_ mem, 2);
        r=mirvar_mem(_MIPP_ mem, 3);
        gx=mirvar_mem(_MIPP_ mem, 4);
        gy=mirvar_mem(_MIPP_ mem, 5);
        s=mirvar_mem(_MIPP_ mem, 6);
        wx=mirvar_mem(_MIPP_ mem, 7);
        wy=mirvar_mem(_MIPP_ mem, 8);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->A,a);
        OS2FEP(_MIPP_ &DOM->B,b);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->Gx,gx);
        OS2FEP(_MIPP_ &DOM->Gy,gy);

        ecurve_init(_MIPP_ a,b,q,MR_PROJECTIVE);
        G=epoint_init_mem(_MIPP_ mem1,0);
        WP=epoint_init_mem(_MIPP_ mem1,1);
        epoint_set(_MIPP_ gx,gy,0,G);

        if (RNG!=NULL)
            strong_bigrand(_MIPP_ RNG,r,s);
        else
        {
            OS2FEP(_MIPP_ S,s);
            divide(_MIPP_ s,r,r);
        }
        if (DOM->PC.window==0)
        {
            ecurve_mult(_MIPP_ s,G,WP);        
            bit=epoint_get(_MIPP_ WP,wx,wy);
        }
        else
            bit=mul_brick(_MIPP_ &DOM->PC,s,wx,wy);
        if (RNG!=NULL) convert_big_octet(_MIPP_ s,S);
        
        EC2OSP(_MIPP_ wx,wy,bit,compress,DOM->fsize,W);

    }

#ifndef MR_STATIC
    memkill(_MIPP_ mem,9);
    ecp_memkill(_MIPP_ mem1,2);
#else
    memset(mem,0,MR_BIG_RESERVE(9));
    memset(mem1,0,MR_ECP_RESERVE(2));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

/* validate an EC GF(p) public key. Set full=TRUE for fuller, 
 * but more time-consuming test */

P1363_API int ECP_PUBLIC_KEY_VALIDATE(BOOL (*idle)(void),ecp_domain *DOM,BOOL full,octet *W)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,a,b,r,wx,wy;
    epoint *WP;
    BOOL valid,compressed;
    int bit,err,res=0;
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(6)];
    char mem1[MR_ECP_RESERVE(1)];
    memset(mem,0,MR_BIG_RESERVE(6));
    memset(mem1,0,MR_ECP_RESERVE(1));
#endif
 
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 6);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 1);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        a=mirvar_mem(_MIPP_ mem, 1);
        b=mirvar_mem(_MIPP_ mem, 2);
        r=mirvar_mem(_MIPP_ mem, 3);
        wx=mirvar_mem(_MIPP_ mem, 4);
        wy=mirvar_mem(_MIPP_ mem, 5);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->A,a);
        OS2FEP(_MIPP_ &DOM->B,b);
        OS2FEP(_MIPP_ &DOM->R,r);

        compressed=OS2ECP(_MIPP_ W,wx,wy,DOM->fsize,&bit);
        if (W->len<DOM->fsize+1 || W->len>2*DOM->fsize+1) res=MR_P1363_INVALID_PUBLIC_KEY;
        if (compare(wx,q)>=0 || compare (wy,q)>=0) res=MR_P1363_INVALID_PUBLIC_KEY; 
    }
    if (res==0)
    {
        ecurve_init(_MIPP_ a,b,q,MR_PROJECTIVE);
        WP=epoint_init_mem(_MIPP_ mem1,0);

        if (!compressed)
             valid=epoint_set(_MIPP_ wx,wy,0,WP);
        else valid=epoint_set(_MIPP_ wx,wx,bit,WP);

        if (!valid || WP->marker==MR_EPOINT_INFINITY) res=MR_P1363_INVALID_PUBLIC_KEY;
        if (res==0 && full)
        {
            ecurve_mult(_MIPP_ r,WP,WP);
            if (WP->marker!=MR_EPOINT_INFINITY) res=MR_P1363_INVALID_PUBLIC_KEY; 
        }
      
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,6);
    ecp_memkill(_MIPP_ mem1,1);
#else
    memset(mem,0,MR_BIG_RESERVE(6));
    memset(mem1,0,MR_ECP_RESERVE(1));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

/*** P1363 EC GF(p) primitives ***/
/* See P1363 documentation for specification */
/* Note the support for point compression */

P1363_API int ECPSVDP_DH(BOOL (*idle)(void),ecp_domain *DOM,octet *S,octet *WD,octet *Z)    
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,a,b,s,wx,wy,z;
    BOOL valid,compressed;
    epoint *W;
    int bit,err,res=0;
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(7)];
    char mem1[MR_ECP_RESERVE(1)];
    memset(mem,0,MR_BIG_RESERVE(7));
    memset(mem1,0,MR_ECP_RESERVE(1));
#endif
 
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 7);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 1);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        a=mirvar_mem(_MIPP_ mem, 1);
        b=mirvar_mem(_MIPP_ mem, 2);
        s=mirvar_mem(_MIPP_ mem, 3);
        wx=mirvar_mem(_MIPP_ mem, 4);
        wy=mirvar_mem(_MIPP_ mem, 5);
        z=mirvar_mem(_MIPP_ mem, 6);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->A,a);
        OS2FEP(_MIPP_ &DOM->B,b);
        OS2FEP(_MIPP_ S,s);

        ecurve_init(_MIPP_ a,b,q,MR_PROJECTIVE);
        W=epoint_init_mem(_MIPP_ mem1,0);

        compressed=OS2ECP(_MIPP_ WD,wx,wy,DOM->fsize,&bit);
       
        if (!compressed)
             valid=epoint_set(_MIPP_ wx,wy,0,W);
        else valid=epoint_set(_MIPP_ wx,wx,bit,W);

        if (!valid) res=MR_P1363_ERROR;
    
        ecurve_mult(_MIPP_ s,W,W);
        if (W->marker==MR_EPOINT_INFINITY) res=MR_P1363_ERROR; 
        else
        {
            epoint_get(_MIPP_ W,z,z);
            FE2OSP(_MIPP_ z,DOM->fsize,Z);
        }
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,7);
    ecp_memkill(_MIPP_ mem1,1);
#else
    memset(mem,0,MR_BIG_RESERVE(7));
    memset(mem1,0,MR_ECP_RESERVE(1));
#endif
 
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int ECPSVDP_DHC(BOOL (*idle)(void),ecp_domain *DOM,octet *S,octet *WD,BOOL compatible,octet *Z)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,a,b,r,s,wx,wy,z,k,ik,t;
    BOOL compressed,valid;
    epoint *W;
    int bit,err,res=0;
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(11)];
    char mem1[MR_ECP_RESERVE(1)];
    memset(mem,0,MR_BIG_RESERVE(11));
    memset(mem1,0,MR_ECP_RESERVE(1));
#endif
 
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 11);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 1);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        a=mirvar_mem(_MIPP_ mem, 1);
        b=mirvar_mem(_MIPP_ mem, 2);
        r=mirvar_mem(_MIPP_ mem, 3);
        s=mirvar_mem(_MIPP_ mem, 4);
        wx=mirvar_mem(_MIPP_ mem, 5);
        wy=mirvar_mem(_MIPP_ mem, 6);
        k=mirvar_mem(_MIPP_ mem, 7);
        ik=mirvar_mem(_MIPP_ mem, 8);
        z=mirvar_mem(_MIPP_ mem, 9);
        t=mirvar_mem(_MIPP_ mem, 10);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->A,a);
        OS2FEP(_MIPP_ &DOM->B,b);
        OS2FEP(_MIPP_ &DOM->K,k);
        OS2FEP(_MIPP_ S,s);

        if (compatible)
        {
            OS2FEP(_MIPP_ &DOM->IK,ik);
            mad(_MIPP_ ik,s,ik,r,r,t);   /* t=s/k mod r */
        }
        else copy(s,t);

        ecurve_init(_MIPP_ a,b,q,MR_PROJECTIVE);
        W=epoint_init_mem(_MIPP_ mem1,0);

        compressed=OS2ECP(_MIPP_ WD,wx,wy,DOM->fsize,&bit);

        if (!compressed)
             valid=epoint_set(_MIPP_ wx,wy,0,W);
        else valid=epoint_set(_MIPP_ wx,wx,bit,W);

        if (!valid) res=MR_P1363_ERROR;
        else
        {
            multiply(_MIPP_ t,k,t);
            ecurve_mult(_MIPP_ t,W,W);
            if (W->marker==MR_EPOINT_INFINITY) res=MR_P1363_ERROR;
            else
            { 
                epoint_get(_MIPP_ W,z,z);
                FE2OSP(_MIPP_ z,DOM->fsize,Z);
            }
        } 
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,11);
    ecp_memkill(_MIPP_ mem1,1);
#else
    memset(mem,0,MR_BIG_RESERVE(11));
    memset(mem1,0,MR_ECP_RESERVE(1));
#endif

    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int ECPSVDP_MQV(BOOL (*idle)(void),ecp_domain *DOM,octet *S,octet *U,octet *V,octet *WD,octet *VD,octet *Z)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,a,b,r,s,u,vx,wdx,wdy,vdx,vdy,z,e,t,td;
    epoint *P,*WDP,*VDP;
    BOOL compressed,valid;
    int bit,h,err,res=0;
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(15)];
    char mem1[MR_ECP_RESERVE(3)];
    memset(mem,0,MR_BIG_RESERVE(15));
    memset(mem1,0,MR_ECP_RESERVE(3));
#endif
 
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 15);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 3);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        a=mirvar_mem(_MIPP_ mem, 1);
        b=mirvar_mem(_MIPP_ mem, 2);
        r=mirvar_mem(_MIPP_ mem, 3);
        s=mirvar_mem(_MIPP_ mem, 4);
        u=mirvar_mem(_MIPP_ mem, 5);
        vx=mirvar_mem(_MIPP_ mem, 6);
        wdx=mirvar_mem(_MIPP_ mem, 7);
        wdy=mirvar_mem(_MIPP_ mem, 8);
        vdx=mirvar_mem(_MIPP_ mem, 9);
        vdy=mirvar_mem(_MIPP_ mem, 10);
        z=mirvar_mem(_MIPP_ mem, 11);
        e=mirvar_mem(_MIPP_ mem, 12);
        t=mirvar_mem(_MIPP_ mem, 13);
        td=mirvar_mem(_MIPP_ mem, 14);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->A,a);
        OS2FEP(_MIPP_ &DOM->B,b);

        ecurve_init(_MIPP_ a,b,q,MR_PROJECTIVE);
        
        OS2FEP(_MIPP_ S,s);
        OS2FEP(_MIPP_ U,u);

        P=epoint_init_mem(_MIPP_ mem1,0);
        OS2ECP(_MIPP_ V,vx,NULL,DOM->fsize,&bit); 
        
        WDP=epoint_init_mem(_MIPP_ mem1,1);

        compressed=OS2ECP(_MIPP_ WD,wdx,wdy,DOM->fsize,&bit);

        if (!compressed)
             valid=epoint_set(_MIPP_ wdx,wdy,0,WDP);
        else valid=epoint_set(_MIPP_ wdx,wdx,bit,WDP);

        if (!valid) res=MR_P1363_ERROR;
        else
        {
            VDP=epoint_init_mem(_MIPP_ mem1,2);
            compressed=OS2ECP(_MIPP_ VD,vdx,vdy,DOM->fsize,&bit);
            if (!compressed)
                 valid=epoint_set(_MIPP_ vdx,vdy,0,VDP);
            else valid=epoint_set(_MIPP_ vdx,vdx,bit,VDP);
            if (!valid) res=MR_P1363_ERROR;
            else
            {
                h=DOM->H;
                expb2(_MIPP_ h,z);
                copy(vx,t);
                divide(_MIPP_ t,z,z);
                add(_MIPP_ t,z,t);
                copy (vdx,td);
                divide(_MIPP_ td,z,z);
                add(_MIPP_ td,z,td);

                mad(_MIPP_ t,s,u,r,r,e);
                mad(_MIPP_ e,td,td,r,r,t);
                ecurve_mult2(_MIPP_ e,VDP,t,WDP,P); 
                if (P->marker==MR_EPOINT_INFINITY) res=MR_P1363_ERROR; 
                else
                { 
                    epoint_get(_MIPP_ P,z,z);
                    FE2OSP(_MIPP_ z,DOM->fsize,Z);
                }
            }  
        }
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,15);
    ecp_memkill(_MIPP_ mem1,3);
#else
    memset(mem,0,MR_BIG_RESERVE(15));
    memset(mem1,0,MR_ECP_RESERVE(3));
#endif

    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int ECPSVDP_MQVC(BOOL (*idle)(void),ecp_domain *DOM,octet *S,octet *U,octet *V,octet *WD,octet *VD,BOOL compatible,octet *Z)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,a,b,r,s,u,vx,wdx,wdy,vdx,vdy,z,e,t,td,k,ik;
    epoint *P,*WDP,*VDP;
    BOOL compressed,valid;
    int bit,h,err,res=0;
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(17)];
    char mem1[MR_ECP_RESERVE(3)];
    memset(mem,0,MR_BIG_RESERVE(17));
    memset(mem1,0,MR_ECP_RESERVE(3));
#endif
 
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 17);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 3);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        a=mirvar_mem(_MIPP_ mem, 1);
        b=mirvar_mem(_MIPP_ mem, 2);
        r=mirvar_mem(_MIPP_ mem, 3);
        ik=mirvar_mem(_MIPP_ mem, 4);
        k=mirvar_mem(_MIPP_ mem, 5);
        s=mirvar_mem(_MIPP_ mem, 6);
        u=mirvar_mem(_MIPP_ mem, 7);
        vx=mirvar_mem(_MIPP_ mem, 8);
        wdx=mirvar_mem(_MIPP_ mem, 9);
        wdy=mirvar_mem(_MIPP_ mem, 10);
        vdx=mirvar_mem(_MIPP_ mem, 11);
        vdy=mirvar_mem(_MIPP_ mem, 12);
        z=mirvar_mem(_MIPP_ mem, 13);
        e=mirvar_mem(_MIPP_ mem, 14);
        t=mirvar_mem(_MIPP_ mem, 15);
        td=mirvar_mem(_MIPP_ mem, 16);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->K,k); 
        OS2FEP(_MIPP_ &DOM->A,a);
        OS2FEP(_MIPP_ &DOM->B,b);

        ecurve_init(_MIPP_ a,b,q,MR_PROJECTIVE);
        
        OS2FEP(_MIPP_ S,s);
        OS2FEP(_MIPP_ U,u);

        P=epoint_init_mem(_MIPP_ mem1,0);

        OS2ECP(_MIPP_ V,vx,NULL,DOM->fsize,&bit);
        
        WDP=epoint_init_mem(_MIPP_ mem1,1);

        compressed=OS2ECP(_MIPP_ WD,wdx,wdy,DOM->fsize,&bit);
        if (!compressed)
             valid=epoint_set(_MIPP_ wdx,wdy,0,WDP);
        else valid=epoint_set(_MIPP_ wdx,wdx,bit,WDP);

        if (!valid) res=MR_P1363_ERROR;
        else
        {
            VDP=epoint_init_mem(_MIPP_ mem1,2);
            compressed=OS2ECP(_MIPP_ VD,vdx,vdy,DOM->fsize,&bit);
            if (!compressed)
                 valid=epoint_set(_MIPP_ vdx,vdy,0,VDP);
            else valid=epoint_set(_MIPP_ vdx,vdx,bit,VDP);
            if (!valid) res=MR_P1363_ERROR;
            else
            {
                h=DOM->H;
                expb2(_MIPP_ h,z);
                copy(vx,t);
                divide(_MIPP_ t,z,z);
                add(_MIPP_ t,z,t);
                copy (vdx,td);
                divide(_MIPP_ td,z,z);
                add(_MIPP_ td,z,td);
                mad(_MIPP_ t,s,u,r,r,e);
                if (compatible)
                {
                    OS2FEP(_MIPP_ &DOM->IK,ik);
                    mad(_MIPP_ e,ik,e,r,r,e);
                }
                mad(_MIPP_ e,k,e,r,r,e);
                mad(_MIPP_ e,td,td,r,r,t);
                ecurve_mult2(_MIPP_ e,VDP,t,WDP,P); 
                if (P->marker==MR_EPOINT_INFINITY) res=MR_P1363_INVALID_PUBLIC_KEY; 
                else
                {
                    epoint_get(_MIPP_ P,z,z);
                    FE2OSP(_MIPP_ z,DOM->fsize,Z);
                }
            }
        }
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,17);
    ecp_memkill(_MIPP_ mem1,3);
#else
    memset(mem,0,MR_BIG_RESERVE(17));
    memset(mem1,0,MR_ECP_RESERVE(3));
#endif

    err=mr_mip->ERNUM;

    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int ECPSP_NR(BOOL (*idle)(void),ecp_domain *DOM,csprng *RNG,octet *S,octet *F,octet *C,octet *D)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,a,b,gx,gy,r,s,f,c,d,u,vx;
    epoint *G,*V;
    int err,res=0;
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(12)];
    char mem1[MR_ECP_RESERVE(2)];
    memset(mem,0,MR_BIG_RESERVE(12));
    memset(mem1,0,MR_ECP_RESERVE(2));
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 12);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 2);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        a=mirvar_mem(_MIPP_ mem, 1);
        b=mirvar_mem(_MIPP_ mem, 2);
        gx=mirvar_mem(_MIPP_ mem, 3);
        gy=mirvar_mem(_MIPP_ mem, 4);
        r=mirvar_mem(_MIPP_ mem, 5);
        s=mirvar_mem(_MIPP_ mem, 6);
        f=mirvar_mem(_MIPP_ mem, 7);
        c=mirvar_mem(_MIPP_ mem, 8);
        d=mirvar_mem(_MIPP_ mem, 9);
        u=mirvar_mem(_MIPP_ mem, 10);
        vx=mirvar_mem(_MIPP_ mem,11);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->Gx,gx);
        OS2FEP(_MIPP_ &DOM->Gy,gy);
        OS2FEP(_MIPP_ &DOM->B,b);
        OS2FEP(_MIPP_ &DOM->A,a);
        OS2FEP(_MIPP_ S,s);
        OS2FEP(_MIPP_ F,f);
        if (compare(f,r)>=0) res=MR_P1363_BAD_ASSUMPTION;
    }
    if (res==0)
    {
        ecurve_init(_MIPP_ a,b,q,MR_PROJECTIVE);
        G=epoint_init_mem(_MIPP_ mem1,0);
        V=epoint_init_mem(_MIPP_ mem1,1);
        epoint_set(_MIPP_ gx,gy,0,G);

        do {
            if (mr_mip->ERNUM) break;

            strong_bigrand(_MIPP_ RNG,r,u);
            if (DOM->PC.window==0)
            {
                ecurve_mult(_MIPP_ u,G,V);        
                epoint_get(_MIPP_ V,vx,vx);
            }
            else
                mul_brick(_MIPP_ &DOM->PC,u,vx,vx);
              
            divide(_MIPP_ vx,r,r);
            add(_MIPP_ vx,f,c);
            divide(_MIPP_ c,r,r);
 
        } while (size(c)==0);

        if (res==0)
        {
            mad(_MIPP_ s,c,s,r,r,d);
            subtract(_MIPP_ u,d,d);
            if (size(d)<0) add(_MIPP_ d,r,d);
            convert_big_octet(_MIPP_ c,C);
            convert_big_octet(_MIPP_ d,D);
        }
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,12);
    ecp_memkill(_MIPP_ mem1,2);
#else
    memset(mem,0,MR_BIG_RESERVE(12));
    memset(mem1,0,MR_ECP_RESERVE(2));
#endif

    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int ECPVP_NR(BOOL (*idle)(void),ecp_domain *DOM,octet *W,octet *C,octet *D,octet *F)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,r,a,b,gx,gy,wx,wy,f,c,d;
    int bit,err,res=0;
    epoint *G,*WP,*P;
    BOOL compressed,valid; 
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(11)];
    char mem1[MR_ECP_RESERVE(3)];
    memset(mem,0,MR_BIG_RESERVE(11));
    memset(mem1,0,MR_ECP_RESERVE(3));
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 11);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 3);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        a=mirvar_mem(_MIPP_ mem, 1);
        b=mirvar_mem(_MIPP_ mem, 2);
        gx=mirvar_mem(_MIPP_ mem, 3);
        gy=mirvar_mem(_MIPP_ mem, 4);
        r=mirvar_mem(_MIPP_ mem, 5);
        wx=mirvar_mem(_MIPP_ mem, 6);
        wy=mirvar_mem(_MIPP_ mem, 7);
        f=mirvar_mem(_MIPP_ mem, 8);
        c=mirvar_mem(_MIPP_ mem, 9);
        d=mirvar_mem(_MIPP_ mem, 10);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->Gx,gx);
        OS2FEP(_MIPP_ &DOM->Gy,gy);
        OS2FEP(_MIPP_ &DOM->A,a);
        OS2FEP(_MIPP_ &DOM->B,b);
        OS2FEP(_MIPP_ C,c);
        OS2FEP(_MIPP_ D,d);
        if (size(c)<1 || compare(c,r)>=0 || size(d)<0 || compare(d,r)>=0) 
            res=MR_P1363_INVALID;
    }

    if (res==0)
    {
        ecurve_init(_MIPP_ a,b,q,MR_PROJECTIVE);
        G=epoint_init_mem(_MIPP_ mem1,0);
        WP=epoint_init_mem(_MIPP_ mem1,1);
        P=epoint_init_mem(_MIPP_ mem1,2);
        epoint_set(_MIPP_ gx,gy,0,G);

        compressed=OS2ECP(_MIPP_ W,wx,wy,DOM->fsize,&bit);
        if (!compressed)
             valid=epoint_set(_MIPP_ wx,wy,0,WP);
        else valid=epoint_set(_MIPP_ wx,wx,bit,WP);
        if (!valid) res=MR_P1363_ERROR;
        else
        {         
            ecurve_mult2(_MIPP_ d,G,c,WP,P);
            if (P->marker==MR_EPOINT_INFINITY) res=MR_P1363_INVALID;
            else
            {
                epoint_get(_MIPP_ P,f,f);
                divide(_MIPP_ f,r,r);
                subtract(_MIPP_ c,f,f);
                if (size(f)<0) add(_MIPP_ f,r,f);
                convert_big_octet(_MIPP_ f,F);
            }
        }
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,11);
    ecp_memkill(_MIPP_ mem1,3);
#else
    memset(mem,0,MR_BIG_RESERVE(11));
    memset(mem1,0,MR_ECP_RESERVE(3));
#endif

    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int ECPPSP_NR2PV(BOOL (*idle)(void),ecp_domain *DOM,csprng *RNG,octet *U,octet *V)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,a,b,gx,gy,r,u,vx;
    epoint *G,*VP;
    int err,res=0;
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(8)];
    char mem1[MR_ECP_RESERVE(2)];
    memset(mem,0,MR_BIG_RESERVE(8));
    memset(mem1,0,MR_ECP_RESERVE(2));
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 8);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 2);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        a=mirvar_mem(_MIPP_ mem, 1);
        b=mirvar_mem(_MIPP_ mem, 2);
        gx=mirvar_mem(_MIPP_ mem, 3);
        gy=mirvar_mem(_MIPP_ mem, 4);
        r=mirvar_mem(_MIPP_ mem, 5);
        u=mirvar_mem(_MIPP_ mem, 6);
        vx=mirvar_mem(_MIPP_ mem, 7);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->Gx,gx);
        OS2FEP(_MIPP_ &DOM->Gy,gy);
        OS2FEP(_MIPP_ &DOM->B,b);
        OS2FEP(_MIPP_ &DOM->A,a);

        ecurve_init(_MIPP_ a,b,q,MR_PROJECTIVE);
        G=epoint_init_mem(_MIPP_ mem1,0);
        VP=epoint_init_mem(_MIPP_ mem1,1);
        epoint_set(_MIPP_ gx,gy,0,G);

        strong_bigrand(_MIPP_ RNG,r,u);
        if (DOM->PC.window==0)
        {
            ecurve_mult(_MIPP_ u,G,VP);        
            epoint_get(_MIPP_ VP,vx,vx);
        }
        else
        mul_brick(_MIPP_ &DOM->PC,u,vx,vx);
              
        convert_big_octet(_MIPP_ u,U);
        FE2OSP(_MIPP_ vx,DOM->fsize,V);

    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,8);
    ecp_memkill(_MIPP_ mem1,2);
#else
    memset(mem,0,MR_BIG_RESERVE(8));
    memset(mem1,0,MR_ECP_RESERVE(2));
#endif

    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int ECPSP_NR2(BOOL (*idle)(void),ecp_domain *DOM,octet *S,octet *U,octet *V,octet *F,octet *C,octet *D)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,r,u,v,s,c,d,f;
    int err,res=0;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(8)];
    memset(mem,0,MR_BIG_RESERVE(8));
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 8);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        r=mirvar_mem(_MIPP_ mem, 1);
        u=mirvar_mem(_MIPP_ mem, 2);
        v=mirvar_mem(_MIPP_ mem, 3);
        s=mirvar_mem(_MIPP_ mem, 4);
        c=mirvar_mem(_MIPP_ mem, 5);
        d=mirvar_mem(_MIPP_ mem, 6);
        f=mirvar_mem(_MIPP_ mem, 7);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ S,s);
        OS2FEP(_MIPP_ F,f);
        OS2FEP(_MIPP_ U,u);
        OS2FEP(_MIPP_ V,v);
        if (size(u)<1 || compare(u,r)>=0)  res=MR_P1363_BAD_ASSUMPTION;
        if (size(v)<1 || compare(v,q)>=0) res=MR_P1363_BAD_ASSUMPTION;  
        if (compare(f,r)>=0) res=MR_P1363_BAD_ASSUMPTION; 
    }
    if (res==0)
    {
        add(_MIPP_ v,f,c);
        divide(_MIPP_ c,r,r);
        if (size(c)==0) res=MR_P1363_ERROR;
    }
    if (res==0)
    {
        mad(_MIPP_ s,c,s,r,r,d);
        subtract(_MIPP_ u,d,d);
        if (size(d)<0) add(_MIPP_ d,r,d);
 
        convert_big_octet(_MIPP_ c,C);
        convert_big_octet(_MIPP_ d,D);
    }

    OCTET_CLEAR(U);
#ifndef MR_STATIC
    memkill(_MIPP_ mem,8);
#else
    memset(mem,0,MR_BIG_RESERVE(8));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int ECPVP_NR2(BOOL (*idle)(void),ecp_domain *DOM,octet *W,octet *C,octet *D,octet *F,octet *V)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,r,a,b,gx,gy,v,wx,wy,c,d,f;
    epoint *G,*WP,*P;
    BOOL compressed,valid;
    int bit,err,res=0;
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(12)];
    char mem1[MR_ECP_RESERVE(3)];
    memset(mem,0,MR_BIG_RESERVE(12));
    memset(mem1,0,MR_ECP_RESERVE(3));
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 12);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 3);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        r=mirvar_mem(_MIPP_ mem, 1);
        a=mirvar_mem(_MIPP_ mem, 2);
        b=mirvar_mem(_MIPP_ mem, 3);
        gx=mirvar_mem(_MIPP_ mem, 4);
        gy=mirvar_mem(_MIPP_ mem, 5);
        v=mirvar_mem(_MIPP_ mem, 6);
        wx=mirvar_mem(_MIPP_ mem, 7);
        wy=mirvar_mem(_MIPP_ mem, 8);
        c=mirvar_mem(_MIPP_ mem, 9);
        d=mirvar_mem(_MIPP_ mem, 10);
        f=mirvar_mem(_MIPP_ mem, 11);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->Gx,gx);
        OS2FEP(_MIPP_ &DOM->Gy,gy);
        OS2FEP(_MIPP_ &DOM->B,b);
        OS2FEP(_MIPP_ &DOM->A,a);
        OS2FEP(_MIPP_ C,c);
        OS2FEP(_MIPP_ D,d);
        if (size(c)<1 || compare(c,r)>=0 || compare(d,r)>=0) res=MR_P1363_INVALID;
    }
    if (res==0)
    {
        ecurve_init(_MIPP_ a,b,q,MR_PROJECTIVE);
        G=epoint_init_mem(_MIPP_ mem1,0);   
        WP=epoint_init_mem(_MIPP_ mem1,1);
        P=epoint_init_mem(_MIPP_ mem1,2);   
        epoint_set(_MIPP_ gx,gy,0,G);

        compressed=OS2ECP(_MIPP_ W,wx,wy,DOM->fsize,&bit);
        if (!compressed)
             valid=epoint_set(_MIPP_ wx,wy,0,WP);
        else valid=epoint_set(_MIPP_ wx,wx,bit,WP);
        if (!valid) res=MR_P1363_ERROR;
        else
        {
            ecurve_mult2(_MIPP_ d,G,c,WP,P);
            if (P->marker==MR_EPOINT_INFINITY) res=MR_P1363_INVALID;
            else
            {
                epoint_get(_MIPP_ P,v,v);
                copy(v,f);
                divide(_MIPP_ f,r,r);
                subtract(_MIPP_ c,f,f);
                if (size(f)<0) add(_MIPP_ f,r,f);
                convert_big_octet(_MIPP_ f,F);
                FE2OSP(_MIPP_ v,DOM->fsize,V);
            }
        }
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,12);
    ecp_memkill(_MIPP_ mem1,3);
#else
    memset(mem,0,MR_BIG_RESERVE(12));
    memset(mem1,0,MR_ECP_RESERVE(3));
#endif

    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int ECPSP_PV(BOOL (*idle)(void),ecp_domain *DOM,octet *S,octet *U,octet *H,octet *D)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,r,u,s,d,h;
    int err,res=0;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(6)];
    memset(mem,0,MR_BIG_RESERVE(6));
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 6);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        r=mirvar_mem(_MIPP_ mem, 1);
        u=mirvar_mem(_MIPP_ mem, 2);
        s=mirvar_mem(_MIPP_ mem, 3);
        d=mirvar_mem(_MIPP_ mem, 4);
        h=mirvar_mem(_MIPP_ mem, 5);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ S,s);
        OS2FEP(_MIPP_ U,u);
        OS2FEP(_MIPP_ H,h);
        if (size(u)<1 || compare(u,r)>=0)  res=MR_P1363_BAD_ASSUMPTION;
    }

    if (res==0)
    {
        mad(_MIPP_ s,h,s,r,r,d);
        subtract(_MIPP_ u,d,d);
        if (size(d)<0) add(_MIPP_ d,r,d);
 
        convert_big_octet(_MIPP_ d,D);
    }

    OCTET_CLEAR(U);
#ifndef MR_STATIC
    memkill(_MIPP_ mem,6);
#else
    memset(mem,0,MR_BIG_RESERVE(6));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int ECPVP_PV(BOOL (*idle)(void),ecp_domain *DOM,octet *W,octet *H,octet *D,octet *V)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,r,a,b,gx,gy,v,wx,wy,d,h;
    epoint *G,*WP,*P;
    BOOL compressed,valid;
    int bit,err,res=0;
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(11)];
    char mem1[MR_ECP_RESERVE(3)];
    memset(mem,0,MR_BIG_RESERVE(11));
    memset(mem1,0,MR_ECP_RESERVE(3));
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 11);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 3);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        r=mirvar_mem(_MIPP_ mem, 1);
        a=mirvar_mem(_MIPP_ mem, 2);
        b=mirvar_mem(_MIPP_ mem, 3);
        gx=mirvar_mem(_MIPP_ mem, 4);
        gy=mirvar_mem(_MIPP_ mem, 5);
        v=mirvar_mem(_MIPP_ mem, 6);
        wx=mirvar_mem(_MIPP_ mem, 7);
        wy=mirvar_mem(_MIPP_ mem, 8);
        d=mirvar_mem(_MIPP_ mem, 9);
        h=mirvar_mem(_MIPP_ mem, 10);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->Gx,gx);
        OS2FEP(_MIPP_ &DOM->Gy,gy);
        OS2FEP(_MIPP_ &DOM->B,b);
        OS2FEP(_MIPP_ &DOM->A,a);
        OS2FEP(_MIPP_ H,h);
        OS2FEP(_MIPP_ D,d);
        if (compare(d,r)>=0) res=MR_P1363_INVALID;
    }
    if (res==0)
    {
        ecurve_init(_MIPP_ a,b,q,MR_PROJECTIVE);
        G=epoint_init_mem(_MIPP_ mem1,0);   
        WP=epoint_init_mem(_MIPP_ mem1,1);
        P=epoint_init_mem(_MIPP_ mem1,2);   
        epoint_set(_MIPP_ gx,gy,0,G);

        compressed=OS2ECP(_MIPP_ W,wx,wy,DOM->fsize,&bit);
        if (!compressed)
             valid=epoint_set(_MIPP_ wx,wy,0,WP);
        else valid=epoint_set(_MIPP_ wx,wx,bit,WP);
        if (!valid) res=MR_P1363_ERROR;
        else
        {
            ecurve_mult2(_MIPP_ d,G,h,WP,P);
            if (P->marker==MR_EPOINT_INFINITY) res=MR_P1363_INVALID;
            else
            {
                epoint_get(_MIPP_ P,v,v);
                FE2OSP(_MIPP_ v,DOM->fsize,V);
            }
        }
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,11);
    ecp_memkill(_MIPP_ mem1,3);
#else
    memset(mem,0,MR_BIG_RESERVE(11));
    memset(mem1,0,MR_ECP_RESERVE(3));
#endif

    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int ECPSP_DSA(BOOL (*idle)(void),ecp_domain *DOM,csprng *RNG,octet *S,octet *F,octet *C,octet *D)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,a,b,gx,gy,r,s,f,c,d,u,vx;
    epoint *G,*V;
    int err,res=0;
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(12)];
    char mem1[MR_ECP_RESERVE(2)];
    memset(mem,0,MR_BIG_RESERVE(12));
    memset(mem1,0,MR_ECP_RESERVE(2));
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 12);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 2);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        a=mirvar_mem(_MIPP_ mem, 1);
        b=mirvar_mem(_MIPP_ mem, 2);
        gx=mirvar_mem(_MIPP_ mem, 3);
        gy=mirvar_mem(_MIPP_ mem, 4);
        r=mirvar_mem(_MIPP_ mem, 5);
        s=mirvar_mem(_MIPP_ mem, 6);
        f=mirvar_mem(_MIPP_ mem, 7);
        c=mirvar_mem(_MIPP_ mem, 8);
        d=mirvar_mem(_MIPP_ mem, 9);
        u=mirvar_mem(_MIPP_ mem, 10);
        vx=mirvar_mem(_MIPP_ mem,11);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->Gx,gx);
        OS2FEP(_MIPP_ &DOM->Gy,gy);
        OS2FEP(_MIPP_ &DOM->A,a);
        OS2FEP(_MIPP_ &DOM->B,b);
        OS2FEP(_MIPP_ S,s);
        OS2FEP(_MIPP_ F,f);

        ecurve_init(_MIPP_ a,b,q,MR_PROJECTIVE);
        G=epoint_init_mem(_MIPP_ mem1,0);
        V=epoint_init_mem(_MIPP_ mem1,1);
        epoint_set(_MIPP_ gx,gy,0,G);

        do {
            if (mr_mip->ERNUM) break;
            strong_bigrand(_MIPP_ RNG,r,u);
            if (DOM->PC.window==0)
            {
                ecurve_mult(_MIPP_ u,G,V);        
                epoint_get(_MIPP_ V,vx,vx);
            }
            else
                mul_brick(_MIPP_ &DOM->PC,u,vx,vx);

            copy(vx,c); 
            divide(_MIPP_ c,r,r);
            if (size(c)==0) continue;
            xgcd(_MIPP_ u,r,u,u,u);
            mad(_MIPP_ s,c,f,r,r,d);
            mad(_MIPP_ u,d,u,r,r,d);
  
        } while (size(d)==0);
       
        if (res==0)
        {
            convert_big_octet(_MIPP_ c,C);
            convert_big_octet(_MIPP_ d,D);
        }
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,12);
    ecp_memkill(_MIPP_ mem1,2);
#else
    memset(mem,0,MR_BIG_RESERVE(12));
    memset(mem1,0,MR_ECP_RESERVE(2));
#endif

    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int ECPVP_DSA(BOOL (*idle)(void),ecp_domain *DOM,octet *W,octet *C,octet *D,octet *F)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,r,a,b,gx,gy,wx,wy,f,c,d,h2;
    int bit,err,res=0;
    epoint *G,*WP,*P;
    BOOL compressed,valid; 
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(12)];
    char mem1[MR_ECP_RESERVE(3)];
    memset(mem,0,MR_BIG_RESERVE(12));
    memset(mem1,0,MR_ECP_RESERVE(3));
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 12);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 3);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        a=mirvar_mem(_MIPP_ mem, 1);
        b=mirvar_mem(_MIPP_ mem, 2);
        gx=mirvar_mem(_MIPP_ mem, 3);
        gy=mirvar_mem(_MIPP_ mem, 4);
        r=mirvar_mem(_MIPP_ mem, 5);
        wx=mirvar_mem(_MIPP_ mem, 6);
        wy=mirvar_mem(_MIPP_ mem, 7);
        f=mirvar_mem(_MIPP_ mem, 8);
        c=mirvar_mem(_MIPP_ mem, 9);
        d=mirvar_mem(_MIPP_ mem, 10);
        h2=mirvar_mem(_MIPP_ mem,11);

        OS2FEP(_MIPP_ &DOM->Q,q);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->Gx,gx);
        OS2FEP(_MIPP_ &DOM->Gy,gy);
        OS2FEP(_MIPP_ &DOM->A,a);
        OS2FEP(_MIPP_ &DOM->B,b);
        OS2FEP(_MIPP_ C,c);
        OS2FEP(_MIPP_ D,d);
        OS2FEP(_MIPP_ F,f);
        if (size(c)<1 || compare(c,r)>=0 || size(d)<1 || compare(d,r)>=0) 
            res=MR_P1363_INVALID;
    }

    if (res==0)
    {
        xgcd(_MIPP_ d,r,d,d,d);
        mad(_MIPP_ f,d,f,r,r,f);
        mad(_MIPP_ c,d,c,r,r,h2);

        ecurve_init(_MIPP_ a,b,q,MR_PROJECTIVE);
        G=epoint_init_mem(_MIPP_ mem1,0);
        WP=epoint_init_mem(_MIPP_ mem1,1);
        P=epoint_init_mem(_MIPP_ mem1,2);
        epoint_set(_MIPP_ gx,gy,0,G);

        compressed=OS2ECP(_MIPP_ W,wx,wy,DOM->fsize,&bit);
        if (!compressed)
             valid=epoint_set(_MIPP_ wx,wy,0,WP);
        else valid=epoint_set(_MIPP_ wx,wx,bit,WP);
        if (!valid) res=MR_P1363_ERROR;
        else
        {
            ecurve_mult2(_MIPP_ f,G,h2,WP,P);
            if (P->marker==MR_EPOINT_INFINITY) res=MR_P1363_INVALID;
            else
            {
                epoint_get(_MIPP_ P,d,d);
                divide(_MIPP_ d,r,r);
                if (compare(d,c)!=0) res=MR_P1363_INVALID;
            }
        }
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,12);
    ecp_memkill(_MIPP_ mem1,3);
#else
    memset(mem,0,MR_BIG_RESERVE(12));
    memset(mem1,0,MR_ECP_RESERVE(3));
#endif

    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

/*** EC GF(2^m) primitives - support functions ***/
/* destroy the EC GF(2^m) domain structure */

P1363_API void EC2_DOMAIN_KILL(ec2_domain *DOM)
{
    OCTET_KILL(&DOM->A);
    OCTET_KILL(&DOM->B);
    OCTET_KILL(&DOM->R);
    OCTET_KILL(&DOM->Gx);
    OCTET_KILL(&DOM->Gy);
    OCTET_KILL(&DOM->K);
    OCTET_KILL(&DOM->IK);
#ifndef MR_STATIC
    if (DOM->PC.window!=0) 
        ebrick2_end(&DOM->PC);   
#endif

    DOM->words=DOM->fsize=0; DOM->H=DOM->rbits=0;
    DOM->a=0; DOM->b=0; DOM->c=0;
}

/* Initialise the EC GF(2^m) domain structure
 * It is assumed that the EC domain details are obtained from a file 
 * or from an array of strings
 * multiprecision numbers are read in in Hex
 * A suitable file can be generated offline by the MIRACL example program
 * schoof2.exe   
 * Set precompute=window size if a precomputed table is to be used to 
 * speed up the calculation x.G mod EC(2^m) 
 * Returns field size in bytes                                        */

P1363_API int EC2_DOMAIN_INIT(ec2_domain *DOM,char *fname,char **params,int precompute)
{ /* get domain details from specified file     */
  /* If input from a file, params=NULL, if input from strings, fname=NULL  */
  /* return max. size in bytes of octet strings */
    FILE *fp;
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
#endif
    miracl *mr_mip;
    BOOL fileinput=TRUE;
    big q,r,gx,gy,a,b,k;
    int aa,bb,cc,bits,rbits,bytes,err,res=0;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(7)];
    memset(mem,0,MR_BIG_RESERVE(7));
#endif
    if (fname==NULL && params==NULL) return MR_P1363_DOMAIN_NOT_FOUND;
    if (fname==NULL) fileinput=FALSE;

    if (fileinput)
    {
        fp=fopen(fname,"rt");
        if (fp==NULL) return MR_P1363_DOMAIN_NOT_FOUND;
        fscanf(fp,"%d\n",&bits);
    }
    else
        sscanf(params[0],"%d\n",&bits);

    DOM->words=MR_ROUNDUP(mr_abs(bits),MIRACL);
#ifdef MR_GENERIC_AND_STATIC
	mr_mip=mirsys(&instance,MR_ROUNDUP(mr_abs(bits),4),16);
#else
    mr_mip=mirsys(MR_ROUNDUP(mr_abs(bits),4),16);
#endif
	if (mr_mip==NULL) 
    {
        if (fileinput) fclose(fp);
        return MR_P1363_OUT_OF_MEMORY;
    }
    mr_mip->ERCON=TRUE;
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 7);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        r=mirvar_mem(_MIPP_ mem, 1);
        gx=mirvar_mem(_MIPP_ mem, 2);
        gy=mirvar_mem(_MIPP_ mem, 3);
        a=mirvar_mem(_MIPP_ mem, 4);
        b=mirvar_mem(_MIPP_ mem, 5);
        k=mirvar_mem(_MIPP_ mem, 6);

        bytes=MR_ROUNDUP(mr_abs(bits),8);

        if (fileinput)
        {
            innum(_MIPP_ a,fp);
            innum(_MIPP_ b,fp);
            innum(_MIPP_ r,fp);
            innum(_MIPP_ gx,fp);
            innum(_MIPP_ gy,fp);
            fscanf(fp,"%d\n",&aa);
            fscanf(fp,"%d\n",&bb);
            fscanf(fp,"%d\n",&cc);
            fclose(fp);
        }
        else
        {
            instr(_MIPP_ a,params[1]);
            instr(_MIPP_ b,params[2]);
            instr(_MIPP_ r,params[3]);
            instr(_MIPP_ gx,params[4]);
            instr(_MIPP_ gy,params[5]);
            sscanf(params[6],"%d\n",&aa);
            sscanf(params[7],"%d\n",&bb);
            sscanf(params[8],"%d\n",&cc);
        }
        DOM->M=bits;
        OCTET_INIT(&DOM->A,bytes);
        OCTET_INIT(&DOM->B,bytes);
        OCTET_INIT(&DOM->R,bytes);
        OCTET_INIT(&DOM->Gx,bytes);
        OCTET_INIT(&DOM->Gy,bytes);
        OCTET_INIT(&DOM->K,bytes);
        OCTET_INIT(&DOM->IK,bytes);
        rbits=logb2(_MIPP_ r);
        DOM->H=(1+rbits)/2;
        DOM->fsize=bytes;
        DOM->rbits=rbits; 
        DOM->a=aa; DOM->b=bb; DOM->c=cc;
        expb2(_MIPP_ mr_abs(bits),q);    /* q=2^m */

        nroot(_MIPP_ q,2,k);
        premult(_MIPP_ k,2,k);
        add(_MIPP_ k,q,k);
        incr(_MIPP_ k,3,k);

        divide(_MIPP_ k,r,k);        /* gets co-factor k */

        convert_big_octet(_MIPP_ a,&DOM->A);
        convert_big_octet(_MIPP_ b,&DOM->B);
        convert_big_octet(_MIPP_ r,&DOM->R);
        convert_big_octet(_MIPP_ gx,&DOM->Gx);
        convert_big_octet(_MIPP_ gy,&DOM->Gy);
        convert_big_octet(_MIPP_ k,&DOM->K);
        xgcd(_MIPP_ k,r,k,k,k);
        convert_big_octet(_MIPP_ k,&DOM->IK);

        DOM->PC.window=0;
#ifndef MR_STATIC
        if (precompute) 
            ebrick2_init(_MIPP_ &DOM->PC,gx,gy,a,b,bits,aa,bb,cc,precompute,logb2(_MIPP_ r));
#endif
	}
#ifndef MR_STATIC
    memkill(_MIPP_ mem,7);
#else
    memset(mem,0,MR_BIG_RESERVE(7));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    if (res<0) return res;
    else return bytes;
}

/* validate EC GF(2^m) domain details - good idea if you got them from
 * some-one else! */

P1363_API int EC2_DOMAIN_VALIDATE(BOOL (*idle)(void),ec2_domain *DOM)
{ /* do domain checks - A16.8 */
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big q,a,b,r,gx,gy,t;
    epoint *G;
    int i,aa,bb,cc,err,res=0;
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(7)];
    char mem1[MR_ECP_RESERVE(1)];
    memset(mem,0,MR_BIG_RESERVE(7));
    memset(mem1,0,MR_ECP_RESERVE(1));
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    
    mr_mip->ERCON=TRUE;
    mr_mip->NTRY=50;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 7);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 1);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        q=mirvar_mem(_MIPP_ mem, 0);
        a=mirvar_mem(_MIPP_ mem, 1);
        b=mirvar_mem(_MIPP_ mem, 2);
        r=mirvar_mem(_MIPP_ mem, 3);
        gx=mirvar_mem(_MIPP_ mem, 4);
        gy=mirvar_mem(_MIPP_ mem, 5);
        t=mirvar_mem(_MIPP_ mem, 6);

        expb2(_MIPP_ DOM->M,q);
        OS2FEP(_MIPP_ &DOM->A,a);
        OS2FEP(_MIPP_ &DOM->B,b);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->Gx,gx);
        OS2FEP(_MIPP_ &DOM->Gy,gy);
        aa=DOM->a;
        bb=DOM->b;
        cc=DOM->c;

        nroot(_MIPP_ q,2,t);
        premult(_MIPP_ t,4,t);

        if (compare(r,t)<=0) res=MR_P1363_DOMAIN_ERROR;
        if (size(r)<3 || size(b)==0) res=MR_P1363_DOMAIN_ERROR;
    }

    if (res==0)
    {
        gprime(_MIPP_ 10000);
        if (!isprime(_MIPP_ r)) res=MR_P1363_DOMAIN_ERROR;
    }
    if (res==0)
    {
        if (!ecurve2_init(_MIPP_ DOM->M,aa,bb,cc,a,b,TRUE,MR_AFFINE))
            res=MR_P1363_DOMAIN_ERROR;
    }
    if (res==0)
    {
        G=epoint_init_mem(_MIPP_ mem1,0);
        if (!epoint2_set(_MIPP_ gx,gy,0,G)) res=MR_P1363_DOMAIN_ERROR;
        if (G->marker==MR_EPOINT_INFINITY) res=MR_P1363_DOMAIN_ERROR;
        else
        { 
            ecurve2_mult(_MIPP_ r,G,G);
            if (G->marker!=MR_EPOINT_INFINITY) res=MR_P1363_DOMAIN_ERROR;
        }
    }                  
       
    if (res==0)
    { /* MOV conditon */
        convert(_MIPP_ 1,t);
        for (i=0;i<50;i++)
        {
            mad(_MIPP_ t,q,q,r,r,t);
            if (size(t)==1)
            {
                res=MR_P1363_DOMAIN_ERROR;
                break;
            }
        }
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,7);
    ecp_memkill(_MIPP_ mem1,1);
#else
    memset(mem,0,MR_BIG_RESERVE(7));
    memset(mem1,0,MR_ECP_RESERVE(1));
#endif

    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

/* validate an EC GF(2^m) public key W. Set full=TRUE for fuller, 
 * but more time-consuming test */

P1363_API int EC2_PUBLIC_KEY_VALIDATE(BOOL (*idle)(void),ec2_domain *DOM,BOOL full,octet *W)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big a,b,r,wx,wy;
    epoint *WP;
    BOOL compressed,valid;
    int bit,M,aa,bb,cc,err,res=0;
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(5)];
    char mem1[MR_ECP_RESERVE(1)];
    memset(mem,0,MR_BIG_RESERVE(5));
    memset(mem1,0,MR_ECP_RESERVE(1));
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 5);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 1);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        a=mirvar_mem(_MIPP_ mem, 0);
        b=mirvar_mem(_MIPP_ mem, 1);
        r=mirvar_mem(_MIPP_ mem, 2);
        wx=mirvar_mem(_MIPP_ mem, 3);
        wy=mirvar_mem(_MIPP_ mem, 4);

        M=DOM->M;
        aa=DOM->a;
        bb=DOM->b;
        cc=DOM->c;
        OS2FEP(_MIPP_ &DOM->A,a);
        OS2FEP(_MIPP_ &DOM->B,b);
        OS2FEP(_MIPP_ &DOM->R,r);

        compressed=OS2ECP(_MIPP_ W,wx,wy,DOM->fsize,&bit);
        if (W->len<DOM->fsize+1 || W->len>2*DOM->fsize+1) res=MR_P1363_INVALID_PUBLIC_KEY;
        if (logb2(_MIPP_ wx)>M || logb2(_MIPP_ wy)>M) res=MR_P1363_INVALID_PUBLIC_KEY;
    }
    if (res==0)
    {
        ecurve2_init(_MIPP_ M,aa,bb,cc,a,b,FALSE,MR_PROJECTIVE);
        WP=epoint_init_mem(_MIPP_ mem1,0);

        if (!compressed)
             valid=epoint2_set(_MIPP_ wx,wy,0,WP);
        else valid=epoint2_set(_MIPP_ wx,wx,bit,WP);

        if (!valid || WP->marker==MR_EPOINT_INFINITY) res=MR_P1363_INVALID_PUBLIC_KEY;
        if (res==0 && full)
        {
            ecurve2_mult(_MIPP_ r,WP,WP);
            if (WP->marker!=MR_EPOINT_INFINITY) res=MR_P1363_INVALID_PUBLIC_KEY; 
        }
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,5);
    ecp_memkill(_MIPP_ mem1,1);
#else
    memset(mem,0,MR_BIG_RESERVE(5));
    memset(mem1,0,MR_ECP_RESERVE(1));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

/* Calculate a public/private EC GF(2^m) key pair. W=S.g mod EC(2^m),
 * where S is the secret key and W is the public key
 * Returns a single bit which can be used for "point compression" 
 * If RNG is NULL then the private key is provided externally in S
 * otherwise it is generated randomly internally
 * (Set Wy to NULL if y-coordinate not needed) */

P1363_API int EC2_KEY_PAIR_GENERATE(BOOL (*idle)(void),ec2_domain *DOM,csprng *RNG,octet*S,BOOL compress,octet *W)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big a,b,r,gx,gy,s,wx,wy;
    epoint *G,*WP;
    int M,aa,bb,cc;
    int bit,err,res=0;
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(8)];
    char mem1[MR_ECP_RESERVE(2)];
    memset(mem,0,MR_BIG_RESERVE(8));
    memset(mem1,0,MR_ECP_RESERVE(2));
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 8);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 2);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        a=mirvar_mem(_MIPP_ mem, 0);
        b=mirvar_mem(_MIPP_ mem, 1);
        r=mirvar_mem(_MIPP_ mem, 2);
        gx=mirvar_mem(_MIPP_ mem, 3);
        gy=mirvar_mem(_MIPP_ mem, 4);
        s=mirvar_mem(_MIPP_ mem, 5);
        wx=mirvar_mem(_MIPP_ mem, 6);
        wy=mirvar_mem(_MIPP_ mem, 7);

        M=DOM->M;
        aa=DOM->a;
        bb=DOM->b;
        cc=DOM->c;

        OS2FEP(_MIPP_ &DOM->A,a);
        OS2FEP(_MIPP_ &DOM->B,b);
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->Gx,gx);
        OS2FEP(_MIPP_ &DOM->Gy,gy);

        ecurve2_init(_MIPP_ M,aa,bb,cc,a,b,FALSE,MR_PROJECTIVE);
        G=epoint_init_mem(_MIPP_ mem1,0);
        WP=epoint_init_mem(_MIPP_ mem1,1);
        epoint2_set(_MIPP_ gx,gy,0,G);

        if (RNG!=NULL)
        {
            strong_bigrand(_MIPP_ RNG,r,s);
        }
        else
        {
            OS2FEP(_MIPP_ S,s);
            divide(_MIPP_ s,r,r);
        }
        if (DOM->PC.window==0)
        {
            ecurve2_mult(_MIPP_ s,G,WP);        
            bit=epoint2_get(_MIPP_ WP,wx,wy);
        }
        else
            bit=mul2_brick(_MIPP_ &DOM->PC,s,wx,wy);
        if (RNG!=NULL) 
            convert_big_octet(_MIPP_ s,S);

        EC2OSP(_MIPP_ wx,wy,bit,compress,DOM->fsize,W);
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,8);
    ecp_memkill(_MIPP_ mem1,2);
#else
    memset(mem,0,MR_BIG_RESERVE(8));
    memset(mem1,0,MR_ECP_RESERVE(2));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

/*** P1363 EC GF(2^m) primitives ***/
/* See P1363 documentation for specification */
/* Note the support for point compression */

P1363_API int EC2SVDP_DH(BOOL (*idle)(void),ec2_domain *DOM,octet *S,octet *WD,octet *Z)    
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big a,b,s,wx,wy,z;
    BOOL compress,valid;
    epoint *WP;
    int bit,M,aa,bb,cc,err,res=0;
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(6)];
    char mem1[MR_ECP_RESERVE(1)];
    memset(mem,0,MR_BIG_RESERVE(6));
    memset(mem1,0,MR_ECP_RESERVE(1));
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 6);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 1);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        a=mirvar_mem(_MIPP_ mem, 0);
        b=mirvar_mem(_MIPP_ mem, 1);
        s=mirvar_mem(_MIPP_ mem, 2);
        wx=mirvar_mem(_MIPP_ mem, 3);
        wy=mirvar_mem(_MIPP_ mem, 4);
        z=mirvar_mem(_MIPP_ mem, 5);

        M=DOM->M;
        aa=DOM->a;
        bb=DOM->b;
        cc=DOM->c;
        
        OS2FEP(_MIPP_ &DOM->A,a);
        OS2FEP(_MIPP_ &DOM->B,b);
        OS2FEP(_MIPP_ S,s);

        ecurve2_init(_MIPP_ M,aa,bb,cc,a,b,FALSE,MR_PROJECTIVE);
        WP=epoint_init_mem(_MIPP_ mem1,0);

        compress=OS2ECP(_MIPP_ WD,wx,wy,DOM->fsize,&bit);

        if (!compress)
             valid=epoint2_set(_MIPP_ wx,wy,0,WP);
        else valid=epoint2_set(_MIPP_ wx,wx,bit,WP);

        if (!valid) res=MR_P1363_ERROR;
        else
        {
            ecurve2_mult(_MIPP_ s,WP,WP);
            if (WP->marker==MR_EPOINT_INFINITY) res=MR_P1363_ERROR; 
            else
            { 
                epoint2_get(_MIPP_ WP,z,z);
                convert_big_octet(_MIPP_ z,Z);
            }
        }
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,6);
    ecp_memkill(_MIPP_ mem1,1);
#else
    memset(mem,0,MR_BIG_RESERVE(6));
    memset(mem1,0,MR_ECP_RESERVE(1));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int EC2SVDP_DHC(BOOL (*idle)(void),ec2_domain *DOM,octet *S,octet *WD,BOOL compatible,octet *Z)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big a,b,r,s,wx,wy,z,k,ik,t;
    BOOL compress,valid;
    epoint *WP;
    int bit,M,aa,bb,cc,err,res=0;
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(10)];
    char mem1[MR_ECP_RESERVE(1)];
    memset(mem,0,MR_BIG_RESERVE(10));
    memset(mem1,0,MR_ECP_RESERVE(1));
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 10);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 1);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        a=mirvar_mem(_MIPP_ mem, 0);
        b=mirvar_mem(_MIPP_ mem, 1);
        r=mirvar_mem(_MIPP_ mem, 2);
        s=mirvar_mem(_MIPP_ mem, 3);
        wx=mirvar_mem(_MIPP_ mem,4);
        wy=mirvar_mem(_MIPP_ mem,5);
        k=mirvar_mem(_MIPP_ mem, 6);
        ik=mirvar_mem(_MIPP_ mem,7);
        z=mirvar_mem(_MIPP_ mem, 8);
        t=mirvar_mem(_MIPP_ mem, 9);

        M=DOM->M;
        aa=DOM->a; bb=DOM->b; cc=DOM->c;
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->A,a);
        OS2FEP(_MIPP_ &DOM->B,b);
        OS2FEP(_MIPP_ &DOM->K,k);

        OS2FEP(_MIPP_ S,s);

        if (compatible)
        {
            OS2FEP(_MIPP_ &DOM->IK,ik);
            mad(_MIPP_ ik,s,ik,r,r,t);   /* t=s/k mod r */
        }
        else copy(s,t);

        ecurve2_init(_MIPP_ M,aa,bb,cc,a,b,FALSE,MR_PROJECTIVE);
        WP=epoint_init_mem(_MIPP_ mem1,0);

        compress=OS2ECP(_MIPP_ WD,wx,wy,DOM->fsize,&bit);

        if (!compress)
             valid=epoint2_set(_MIPP_ wx,wy,0,WP);
        else valid=epoint2_set(_MIPP_ wx,wx,bit,WP);

        if (!valid) res=MR_P1363_ERROR;
        else
        {    
            multiply(_MIPP_ t,k,t);
            ecurve2_mult(_MIPP_ t,WP,WP);
            if (WP->marker==MR_EPOINT_INFINITY) res=MR_P1363_ERROR; 
            else
            {
                epoint2_get(_MIPP_ WP,z,z);
                convert_big_octet(_MIPP_ z,Z);
            }
        }
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,10);
    ecp_memkill(_MIPP_ mem1,1);
#else
    memset(mem,0,MR_BIG_RESERVE(10));
    memset(mem1,0,MR_ECP_RESERVE(1));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int EC2SVDP_MQV(BOOL (*idle)(void),ec2_domain *DOM,octet *S,octet *U,octet *V,octet *WD,octet *VD,octet *Z)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big a,b,r,s,u,vx,wdx,wdy,vdx,vdy,z,e,t,td;
    epoint *P,*WDP,*VDP;
    BOOL compress,valid;
    int bit,M,aa,bb,cc,h,err,res=0;
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(14)];
    char mem1[MR_ECP_RESERVE(3)];
    memset(mem,0,MR_BIG_RESERVE(14));
    memset(mem1,0,MR_ECP_RESERVE(3));
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 14);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 3);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        a=mirvar_mem(_MIPP_ mem, 0);
        b=mirvar_mem(_MIPP_ mem, 1);
        r=mirvar_mem(_MIPP_ mem, 2);
        s=mirvar_mem(_MIPP_ mem, 3);
        u=mirvar_mem(_MIPP_ mem, 4);
        vx=mirvar_mem(_MIPP_ mem, 5);
        wdx=mirvar_mem(_MIPP_ mem, 6);
        wdy=mirvar_mem(_MIPP_ mem, 7);
        vdx=mirvar_mem(_MIPP_ mem, 8);
        vdy=mirvar_mem(_MIPP_ mem, 9);
        z=mirvar_mem(_MIPP_ mem, 10);
        e=mirvar_mem(_MIPP_ mem, 11);
        t=mirvar_mem(_MIPP_ mem, 12);
        td=mirvar_mem(_MIPP_ mem, 13);

        M=DOM->M;
        aa=DOM->a; bb=DOM->b;  cc=DOM->c; 
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->B,b);
        OS2FEP(_MIPP_ &DOM->A,a);

        ecurve2_init(_MIPP_ M,aa,bb,cc,a,b,FALSE,MR_PROJECTIVE);
        
        OS2FEP(_MIPP_ S,s);
        OS2FEP(_MIPP_ U,u);

        P=epoint_init_mem(_MIPP_ mem1,0);

        OS2ECP(_MIPP_ V,vx,NULL,DOM->fsize,&bit);
        
        WDP=epoint_init_mem(_MIPP_ mem1,1);

        compress=OS2ECP(_MIPP_ WD,wdx,wdy,DOM->fsize,&bit);

        if (!compress)
             valid=epoint2_set(_MIPP_ wdx,wdy,0,WDP);
        else valid=epoint2_set(_MIPP_ wdx,wdx,bit,WDP);

        if (!valid) res=MR_P1363_ERROR;
        else
        {
            VDP=epoint_init_mem(_MIPP_ mem1,2);

            compress=OS2ECP(_MIPP_ VD,vdx,vdy,DOM->fsize,&bit);
            if (!compress)
                 valid=epoint2_set(_MIPP_ vdx,vdy,0,VDP);
            else valid=epoint2_set(_MIPP_ vdx,vdx,bit,VDP);
            if (!valid) res=MR_P1363_ERROR;
            else
            {
                h=DOM->H;
                expb2(_MIPP_ h,z);
                copy(vx,t);
                divide(_MIPP_ t,z,z);
                add(_MIPP_ t,z,t);
                copy (vdx,td);
                divide(_MIPP_ td,z,z);
                add(_MIPP_ td,z,td);
                mad(_MIPP_ t,s,u,r,r,e);
                mad(_MIPP_ e,td,td,r,r,t);
                ecurve2_mult2(_MIPP_ e,VDP,t,WDP,P); 
                if (P->marker==MR_EPOINT_INFINITY) res=MR_P1363_ERROR; 
                else
                {
                    epoint2_get(_MIPP_ P,z,z);
                    convert_big_octet(_MIPP_ z,Z);
                }
            }
        }
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,14);
    ecp_memkill(_MIPP_ mem1,3);
#else
    memset(mem,0,MR_BIG_RESERVE(14));
    memset(mem1,0,MR_ECP_RESERVE(3));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int EC2SVDP_MQVC(BOOL (*idle)(void),ec2_domain *DOM,octet *S,octet *U,octet *V,octet *WD,octet *VD,BOOL compatible,octet *Z)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big a,b,r,s,u,vx,wdx,wdy,vdx,vdy,z,e,t,td,k,ik;
    epoint *P,*WDP,*VDP;
    BOOL compress,valid;
    int bit,M,aa,bb,cc,h,err,res=0;
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(16)];
    char mem1[MR_ECP_RESERVE(3)];
    memset(mem,0,MR_BIG_RESERVE(16));
    memset(mem1,0,MR_ECP_RESERVE(3));
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 16);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 3);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        a=mirvar_mem(_MIPP_ mem, 0);
        b=mirvar_mem(_MIPP_ mem, 1);
        r=mirvar_mem(_MIPP_ mem, 2);
        ik=mirvar_mem(_MIPP_ mem, 3);
        k=mirvar_mem(_MIPP_ mem, 4);
        s=mirvar_mem(_MIPP_ mem, 5);
        u=mirvar_mem(_MIPP_ mem, 6);
        vx=mirvar_mem(_MIPP_ mem, 7);
        wdx=mirvar_mem(_MIPP_ mem, 8);
        wdy=mirvar_mem(_MIPP_ mem, 9);
        vdx=mirvar_mem(_MIPP_ mem, 10);
        vdy=mirvar_mem(_MIPP_ mem, 11);
        z=mirvar_mem(_MIPP_ mem, 12);
        e=mirvar_mem(_MIPP_ mem, 13);
        t=mirvar_mem(_MIPP_ mem, 14);
        td=mirvar_mem(_MIPP_ mem, 15);

        M=DOM->M;
        aa=DOM->a; bb=DOM->b; cc=DOM->c; 
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->K,k); 
        OS2FEP(_MIPP_ &DOM->B,b);
        OS2FEP(_MIPP_ &DOM->A,a);

        ecurve2_init(_MIPP_ M,aa,bb,cc,a,b,FALSE,MR_PROJECTIVE);
        
        OS2FEP(_MIPP_ S,s);
        OS2FEP(_MIPP_ U,u);

        P=epoint_init_mem(_MIPP_ mem1,0);
        OS2ECP(_MIPP_ V,vx,NULL,DOM->fsize,&bit);
        
        WDP=epoint_init_mem(_MIPP_ mem1,1);

        compress=OS2ECP(_MIPP_ WD,wdx,wdy,DOM->fsize,&bit);
        if (!compress)
             valid=epoint2_set(_MIPP_ wdx,wdy,0,WDP);
        else valid=epoint2_set(_MIPP_ wdx,wdx,bit,WDP);
        if (!valid) res=MR_P1363_ERROR;
        else
        { 
            VDP=epoint_init_mem(_MIPP_ mem1,2);
            compress=OS2ECP(_MIPP_ VD,vdx,vdy,DOM->fsize,&bit);
            if (!compress)
                 valid=epoint2_set(_MIPP_ vdx,vdy,0,VDP);
            else valid=epoint2_set(_MIPP_ vdx,vdx,bit,VDP);
            if (!valid) res=MR_P1363_ERROR;
            else
            {
                h=DOM->H;
                expb2(_MIPP_ h,z);
                copy(vx,t);
                divide(_MIPP_ t,z,z);
                add(_MIPP_ t,z,t);
                copy (vdx,td);
                divide(_MIPP_ td,z,z);
                add(_MIPP_ td,z,td);
                mad(_MIPP_ t,s,u,r,r,e);
                if (compatible)
                {
                    OS2FEP(_MIPP_ &DOM->IK,ik);
                    mad(_MIPP_ e,ik,e,r,r,e);
                }
                mad(_MIPP_ e,k,e,r,r,e);
                mad(_MIPP_ e,td,td,r,r,t);
                ecurve2_mult2(_MIPP_ e,VDP,t,WDP,P); 
                if (P->marker==MR_EPOINT_INFINITY) res=MR_P1363_INVALID_PUBLIC_KEY; 
                else
                {  
                    epoint2_get(_MIPP_ P,z,z);
                    convert_big_octet(_MIPP_ z,Z);
                }
            }
        }
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,16);
    ecp_memkill(_MIPP_ mem1,3);
#else
    memset(mem,0,MR_BIG_RESERVE(16));
    memset(mem1,0,MR_ECP_RESERVE(3));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int EC2SP_NR(BOOL (*idle)(void),ec2_domain *DOM,csprng *RNG,octet *S,octet *F,octet *C,octet *D)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big a,b,gx,gy,r,s,f,c,d,u,vx;
    epoint *G,*V;
    int M,aa,bb,cc,err,res=0;
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(11)];
    char mem1[MR_ECP_RESERVE(2)];
    memset(mem,0,MR_BIG_RESERVE(11));
    memset(mem1,0,MR_ECP_RESERVE(2));
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 11);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 2);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        a=mirvar_mem(_MIPP_ mem, 0);
        b=mirvar_mem(_MIPP_ mem, 1);
        gx=mirvar_mem(_MIPP_ mem, 2);
        gy=mirvar_mem(_MIPP_ mem, 3);
        r=mirvar_mem(_MIPP_ mem, 4);
        s=mirvar_mem(_MIPP_ mem, 5);
        f=mirvar_mem(_MIPP_ mem, 6);
        c=mirvar_mem(_MIPP_ mem, 7);
        d=mirvar_mem(_MIPP_ mem, 8);
        u=mirvar_mem(_MIPP_ mem, 9);
        vx=mirvar_mem(_MIPP_ mem, 10);

        M=DOM->M;
        aa=DOM->a; bb=DOM->b;  cc=DOM->c; 
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->Gx,gx);
        OS2FEP(_MIPP_ &DOM->Gy,gy);
        OS2FEP(_MIPP_ &DOM->B,b);
        OS2FEP(_MIPP_ &DOM->A,a);
        OS2FEP(_MIPP_ S,s);
        OS2FEP(_MIPP_ F,f);
        if (compare(f,r)>=0) res=MR_P1363_BAD_ASSUMPTION;
    }
    if (res==0)
    {
        ecurve2_init(_MIPP_ M,aa,bb,cc,a,b,FALSE,MR_PROJECTIVE);
        G=epoint_init_mem(_MIPP_ mem1,0);
        V=epoint_init_mem(_MIPP_ mem1,1);
        epoint2_set(_MIPP_ gx,gy,0,G);

        do {
            if (mr_mip->ERNUM) break;
            strong_bigrand(_MIPP_ RNG,r,u);
            if (DOM->PC.window==0)
            {
                ecurve2_mult(_MIPP_ u,G,V);        
                epoint2_get(_MIPP_ V,vx,vx);
            }
            else
                mul2_brick(_MIPP_ &DOM->PC,u,vx,vx);

            divide(_MIPP_ vx,r,r);
            add(_MIPP_ vx,f,c);
            divide(_MIPP_ c,r,r);
 
        } while (size(c)==0);
        if (res==0)
        {
            mad(_MIPP_ s,c,s,r,r,d);
            subtract(_MIPP_ u,d,d);
            if (size(d)<0) add(_MIPP_ d,r,d);
            convert_big_octet(_MIPP_ c,C);
            convert_big_octet(_MIPP_ d,D);
        }
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,11);
    ecp_memkill(_MIPP_ mem1,2);
#else
    memset(mem,0,MR_BIG_RESERVE(11));
    memset(mem1,0,MR_ECP_RESERVE(2));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int EC2VP_NR(BOOL (*idle)(void),ec2_domain *DOM,octet *W,octet *C,octet *D,octet *F)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big r,a,b,gx,gy,wx,wy,f,c,d;
    int bit,M,aa,bb,cc,err,res=0;
    epoint *G,*WP,*P;
    BOOL compress,valid; 
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(10)];
    char mem1[MR_ECP_RESERVE(3)];
    memset(mem,0,MR_BIG_RESERVE(10));
    memset(mem1,0,MR_ECP_RESERVE(3));
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 10);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 3);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        a=mirvar_mem(_MIPP_ mem, 0);
        b=mirvar_mem(_MIPP_ mem, 1);
        gx=mirvar_mem(_MIPP_ mem, 2);
        gy=mirvar_mem(_MIPP_ mem, 3);
        r=mirvar_mem(_MIPP_ mem, 4);
        wx=mirvar_mem(_MIPP_ mem, 5);
        wy=mirvar_mem(_MIPP_ mem, 6);
        f=mirvar_mem(_MIPP_ mem, 7);
        c=mirvar_mem(_MIPP_ mem, 8);
        d=mirvar_mem(_MIPP_ mem, 9);

        M=DOM->M;
        aa=DOM->a; bb=DOM->b;  cc=DOM->c; 
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->Gx,gx);
        OS2FEP(_MIPP_ &DOM->Gy,gy);
        OS2FEP(_MIPP_ &DOM->B,b);
        OS2FEP(_MIPP_ &DOM->A,a);
        OS2FEP(_MIPP_ C,c);
        OS2FEP(_MIPP_ D,d);
        if (size(c)<1 || compare(c,r)>=0 || size(d)<0 || compare(d,r)>=0) 
            res=MR_P1363_INVALID;
    }

    if (res==0)
    {
        ecurve2_init(_MIPP_ M,aa,bb,cc,a,b,FALSE,MR_PROJECTIVE);
        G=epoint_init_mem(_MIPP_ mem1,0);
        WP=epoint_init_mem(_MIPP_ mem1,1);
        P=epoint_init_mem(_MIPP_ mem1,2);
        epoint2_set(_MIPP_ gx,gy,0,G);

        compress=OS2ECP(_MIPP_ W,wx,wy,DOM->fsize,&bit);
        if (!compress)
             valid=epoint2_set(_MIPP_ wx,wy,0,WP);
        else valid=epoint2_set(_MIPP_ wx,wx,bit,WP);

        if (!valid) res=MR_P1363_ERROR;
        else
        {
            ecurve2_mult2(_MIPP_ d,G,c,WP,P);
            if (P->marker==MR_EPOINT_INFINITY) res=MR_P1363_INVALID;
            else
            {
                epoint2_get(_MIPP_ P,f,f);
                divide(_MIPP_ f,r,r);
                subtract(_MIPP_ c,f,f);
                if (size(f)<0) add(_MIPP_ f,r,f);
                convert_big_octet(_MIPP_ f,F);
             }
        }
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,10);
    ecp_memkill(_MIPP_ mem1,3);
#else
    memset(mem,0,MR_BIG_RESERVE(10));
    memset(mem1,0,MR_ECP_RESERVE(3));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int EC2SP_DSA(BOOL (*idle)(void),ec2_domain *DOM,csprng *RNG,octet *S,octet *F,octet *C,octet *D)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big a,b,gx,gy,r,s,f,c,d,u,vx;
    epoint *G,*V;
    int M,aa,bb,cc,err,res=0;
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(11)];
    char mem1[MR_ECP_RESERVE(2)];
    memset(mem,0,MR_BIG_RESERVE(11));
    memset(mem1,0,MR_ECP_RESERVE(2));
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 11);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 2);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        a=mirvar_mem(_MIPP_ mem, 0);
        b=mirvar_mem(_MIPP_ mem, 1);
        gx=mirvar_mem(_MIPP_ mem, 2);
        gy=mirvar_mem(_MIPP_ mem, 3);
        r=mirvar_mem(_MIPP_ mem, 4);
        s=mirvar_mem(_MIPP_ mem, 5);
        f=mirvar_mem(_MIPP_ mem, 6);
        c=mirvar_mem(_MIPP_ mem, 7);
        d=mirvar_mem(_MIPP_ mem, 8);
        u=mirvar_mem(_MIPP_ mem, 9);
        vx=mirvar_mem(_MIPP_ mem, 10);

        M=DOM->M;
        aa=DOM->a; bb=DOM->b;  cc=DOM->c; 
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->Gx,gx);
        OS2FEP(_MIPP_ &DOM->Gy,gy);
        OS2FEP(_MIPP_ &DOM->B,b);
        OS2FEP(_MIPP_ &DOM->A,a);
        OS2FEP(_MIPP_ S,s);
        OS2FEP(_MIPP_ F,f);

        ecurve2_init(_MIPP_ M,aa,bb,cc,a,b,FALSE,MR_PROJECTIVE);
        G=epoint_init_mem(_MIPP_ mem1,0);
        V=epoint_init_mem(_MIPP_ mem1,1);
        epoint2_set(_MIPP_ gx,gy,0,G);

        do {
            if (mr_mip->ERNUM) break;

            strong_bigrand(_MIPP_ RNG,r,u);
            if (DOM->PC.window==0)
            {
                ecurve2_mult(_MIPP_ u,G,V);        
                epoint2_get(_MIPP_ V,vx,vx);
            }
            else
                mul2_brick(_MIPP_ &DOM->PC,u,vx,vx);

            copy(vx,c); 
            divide(_MIPP_ c,r,r);
            if (size(c)==0) continue;

            xgcd(_MIPP_ u,r,u,u,u);
            mad(_MIPP_ s,c,f,r,r,d);
            mad(_MIPP_ u,d,u,r,r,d);
  
        } while (size(d)==0);
        if (res==0)
        {
            convert_big_octet(_MIPP_ c,C);
            convert_big_octet(_MIPP_ d,D);
        }
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,11);
    ecp_memkill(_MIPP_ mem1,2);
#else
    memset(mem,0,MR_BIG_RESERVE(11));
    memset(mem1,0,MR_ECP_RESERVE(2));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int EC2VP_DSA(BOOL (*idle)(void),ec2_domain *DOM,octet *W,octet *C,octet *D,octet *F)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big r,a,b,gx,gy,wx,wy,f,c,d,h2;
    int bit,M,aa,bb,cc,err,res=0;
    epoint *G,*WP,*P;
    BOOL compress,valid; 
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(11)];
    char mem1[MR_ECP_RESERVE(3)];
    memset(mem,0,MR_BIG_RESERVE(11));
    memset(mem1,0,MR_ECP_RESERVE(3));
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 11);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 3);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        a=mirvar_mem(_MIPP_ mem, 0);
        b=mirvar_mem(_MIPP_ mem, 1);
        gx=mirvar_mem(_MIPP_ mem, 2);
        gy=mirvar_mem(_MIPP_ mem, 3);
        r=mirvar_mem(_MIPP_ mem, 4);
        wx=mirvar_mem(_MIPP_ mem, 5);
        wy=mirvar_mem(_MIPP_ mem, 6);
        f=mirvar_mem(_MIPP_ mem, 7);
        c=mirvar_mem(_MIPP_ mem, 8);
        d=mirvar_mem(_MIPP_ mem, 9);
        h2=mirvar_mem(_MIPP_ mem, 10);

        M=DOM->M;
        aa=DOM->a; bb=DOM->b;  cc=DOM->c; 
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->Gx,gx);
        OS2FEP(_MIPP_ &DOM->Gy,gy);
        OS2FEP(_MIPP_ &DOM->B,b);
        OS2FEP(_MIPP_ &DOM->A,a);
        OS2FEP(_MIPP_ C,c);
        OS2FEP(_MIPP_ D,d);
        OS2FEP(_MIPP_ F,f);
        if (size(c)<1 || compare(c,r)>=0 || size(d)<1 || compare(d,r)>=0) 
            res=MR_P1363_INVALID;
    }

    if (res==0)
    {
        xgcd(_MIPP_ d,r,d,d,d);
        mad(_MIPP_ f,d,f,r,r,f);
        mad(_MIPP_ c,d,c,r,r,h2);

        ecurve2_init(_MIPP_ M,aa,bb,cc,a,b,FALSE,MR_PROJECTIVE);
        G=epoint_init_mem(_MIPP_ mem1,0);
        WP=epoint_init_mem(_MIPP_ mem1,1);
        P=epoint_init_mem(_MIPP_ mem1,2);
        epoint2_set(_MIPP_ gx,gy,0,G);

        compress=OS2ECP(_MIPP_ W,wx,wy,DOM->fsize,&bit);
        if (!compress)
             valid=epoint2_set(_MIPP_ wx,wy,0,WP);
        else valid=epoint2_set(_MIPP_ wx,wx,bit,WP);
        if (!valid) res=MR_P1363_ERROR;
        else
        {        
            ecurve2_mult2(_MIPP_ f,G,h2,WP,P);
            if (P->marker==MR_EPOINT_INFINITY) res=MR_P1363_INVALID;
            else
            {
                epoint2_get(_MIPP_ P,d,d);
                divide(_MIPP_ d,r,r);
                if (compare(d,c)!=0) res=MR_P1363_INVALID;
            }
        }
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,11);
    ecp_memkill(_MIPP_ mem1,3);
#else
    memset(mem,0,MR_BIG_RESERVE(11));
    memset(mem1,0,MR_ECP_RESERVE(3));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int EC2PSP_NR2PV(BOOL (*idle)(void),ec2_domain *DOM,csprng *RNG,octet *U,octet *V)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
	miracl *mr_mip=mirsys(DOM->words,0);
#endif
	big a,b,gx,gy,r,u,vx;
    epoint *G,*VP;
    int M,aa,bb,cc,err,res=0;
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(7)];
    char mem1[MR_ECP_RESERVE(2)];
    memset(mem,0,MR_BIG_RESERVE(7));
    memset(mem1,0,MR_ECP_RESERVE(2));
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 7);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 3);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        a=mirvar_mem(_MIPP_ mem, 0);
        b=mirvar_mem(_MIPP_ mem, 1);
        gx=mirvar_mem(_MIPP_ mem, 2);
        gy=mirvar_mem(_MIPP_ mem, 3);
        r=mirvar_mem(_MIPP_ mem, 4);
        u=mirvar_mem(_MIPP_ mem, 5);
        vx=mirvar_mem(_MIPP_ mem, 6);

        M=DOM->M;
        aa=DOM->a; bb=DOM->b;  cc=DOM->c; 
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->Gx,gx);
        OS2FEP(_MIPP_ &DOM->Gy,gy);
        OS2FEP(_MIPP_ &DOM->B,b);
        OS2FEP(_MIPP_ &DOM->A,a);

        ecurve2_init(_MIPP_ M,aa,bb,cc,a,b,FALSE,MR_PROJECTIVE);
        G=epoint_init_mem(_MIPP_ mem1,0);
        VP=epoint_init_mem(_MIPP_ mem1,1);
        epoint2_set(_MIPP_ gx,gy,0,G);

        strong_bigrand(_MIPP_ RNG,r,u);
        if (DOM->PC.window==0)
        {
            ecurve2_mult(_MIPP_ u,G,VP);        
            epoint2_get(_MIPP_ VP,vx,vx);
        }
        else
        mul2_brick(_MIPP_ &DOM->PC,u,vx,vx);
              
        convert_big_octet(_MIPP_ u,U);
        FE2OSP(_MIPP_ vx,DOM->fsize,V);

    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,7);
    ecp_memkill(_MIPP_ mem1,2);
#else
    memset(mem,0,MR_BIG_RESERVE(7));
    memset(mem1,0,MR_ECP_RESERVE(2));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int EC2SP_NR2(BOOL (*idle)(void),ec2_domain *DOM,octet *S,octet *U,octet *V,octet *F,octet *C,octet *D)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
    miracl *mr_mip=mirsys(DOM->words,0);
#endif
	big r,u,v,s,c,d,f;
    int err,res=0;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(7)];
    memset(mem,0,MR_BIG_RESERVE(7));
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 7);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        r=mirvar_mem(_MIPP_ mem, 0);
        u=mirvar_mem(_MIPP_ mem, 1);
        v=mirvar_mem(_MIPP_ mem, 2);
        s=mirvar_mem(_MIPP_ mem, 3);
        c=mirvar_mem(_MIPP_ mem, 4);
        d=mirvar_mem(_MIPP_ mem, 5);
        f=mirvar_mem(_MIPP_ mem, 6);
       
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ S,s);
        OS2FEP(_MIPP_ F,f);
        OS2FEP(_MIPP_ U,u);
        OS2FEP(_MIPP_ V,v);
        if (size(u)<1 || compare(u,r)>=0)  res=MR_P1363_BAD_ASSUMPTION;
        if (size(v)<1) res=MR_P1363_BAD_ASSUMPTION;  
        if (compare(f,r)>=0) res=MR_P1363_BAD_ASSUMPTION; 
    }
    if (res==0)
    {
        add(_MIPP_ v,f,c);
        divide(_MIPP_ c,r,r);
        if (size(c)==0) res=MR_P1363_ERROR;
    }
    if (res==0)
    {
        mad(_MIPP_ s,c,s,r,r,d);
        subtract(_MIPP_ u,d,d);
        if (size(d)<0) add(_MIPP_ d,r,d);
 
        convert_big_octet(_MIPP_ c,C);
        convert_big_octet(_MIPP_ d,D);
    }

    OCTET_CLEAR(U);
#ifndef MR_STATIC
    memkill(_MIPP_ mem,7);
#else
    memset(mem,0,MR_BIG_RESERVE(7));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int EC2VP_NR2(BOOL (*idle)(void),ec2_domain *DOM,octet *W,octet *C,octet *D,octet *F,octet *V)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
    miracl *mr_mip=mirsys(DOM->words,0);
#endif
    big r,a,b,gx,gy,v,wx,wy,c,d,f;
    epoint *G,*WP,*P;
    BOOL compressed,valid;
    int bit,M,aa,bb,cc,err,res=0;
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(11)];
    char mem1[MR_ECP_RESERVE(3)];
    memset(mem,0,MR_BIG_RESERVE(11));
    memset(mem1,0,MR_ECP_RESERVE(3));
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 11);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 3);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        r=mirvar_mem(_MIPP_ mem, 0);
        a=mirvar_mem(_MIPP_ mem, 1);
        b=mirvar_mem(_MIPP_ mem, 2);
        gx=mirvar_mem(_MIPP_ mem, 3);
        gy=mirvar_mem(_MIPP_ mem, 4);
        v=mirvar_mem(_MIPP_ mem, 5);
        wx=mirvar_mem(_MIPP_ mem, 6);
        wy=mirvar_mem(_MIPP_ mem, 7);
        c=mirvar_mem(_MIPP_ mem, 8);
        d=mirvar_mem(_MIPP_ mem, 9);
        f=mirvar_mem(_MIPP_ mem, 10);

        M=DOM->M;
        aa=DOM->a; bb=DOM->b;  cc=DOM->c; 
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->Gx,gx);
        OS2FEP(_MIPP_ &DOM->Gy,gy);
        OS2FEP(_MIPP_ &DOM->B,b);
        OS2FEP(_MIPP_ &DOM->A,a);
        OS2FEP(_MIPP_ C,c);
        OS2FEP(_MIPP_ D,d);
        if (size(c)<1 || compare(c,r)>=0 || compare(d,r)>=0) res=MR_P1363_INVALID;
    }
    if (res==0)
    {
        ecurve2_init(_MIPP_ M,aa,bb,cc,a,b,FALSE,MR_PROJECTIVE);
        G=epoint_init_mem(_MIPP_ mem1,0);   
        WP=epoint_init_mem(_MIPP_ mem1,1);
        P=epoint_init_mem(_MIPP_ mem1,2);   
        epoint2_set(_MIPP_ gx,gy,0,G);

        compressed=OS2ECP(_MIPP_ W,wx,wy,DOM->fsize,&bit);
        if (!compressed)
             valid=epoint2_set(_MIPP_ wx,wy,0,WP);
        else valid=epoint2_set(_MIPP_ wx,wx,bit,WP);
        if (!valid) res=MR_P1363_ERROR;
        else
        {
            ecurve2_mult2(_MIPP_ d,G,c,WP,P);
            if (P->marker==MR_EPOINT_INFINITY) res=MR_P1363_INVALID;
            else
            {
                epoint2_get(_MIPP_ P,v,v);
                copy(v,f);
                divide(_MIPP_ f,r,r);
                subtract(_MIPP_ c,f,f);
                if (size(f)<0) add(_MIPP_ f,r,f);
                convert_big_octet(_MIPP_ f,F);
                FE2OSP(_MIPP_ v,DOM->fsize,V);
            }
        }
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,11);
    ecp_memkill(_MIPP_ mem1,3);
#else
    memset(mem,0,MR_BIG_RESERVE(11));
    memset(mem1,0,MR_ECP_RESERVE(3));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int EC2SP_PV(BOOL (*idle)(void),ec2_domain *DOM,octet *S,octet *U,octet *H,octet *D)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
    miracl *mr_mip=mirsys(DOM->words,0);
#endif
	big r,u,s,d,h;
    int err,res=0;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(5)];
    memset(mem,0,MR_BIG_RESERVE(5));
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 5);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        r=mirvar_mem(_MIPP_ mem, 0);
        u=mirvar_mem(_MIPP_ mem, 1);
        s=mirvar_mem(_MIPP_ mem, 2);
        d=mirvar_mem(_MIPP_ mem, 3);
        h=mirvar_mem(_MIPP_ mem, 4);

        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ S,s);
        OS2FEP(_MIPP_ U,u);
        OS2FEP(_MIPP_ H,h);
        if (size(u)<1 || compare(u,r)>=0)  res=MR_P1363_BAD_ASSUMPTION;
    }

    if (res==0)
    {
        mad(_MIPP_ s,h,s,r,r,d);
        subtract(_MIPP_ u,d,d);
        if (size(d)<0) add(_MIPP_ d,r,d);
 
        convert_big_octet(_MIPP_ d,D);
    }

    OCTET_CLEAR(U);
#ifndef MR_STATIC
    memkill(_MIPP_ mem,5);
#else
    memset(mem,0,MR_BIG_RESERVE(5));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int EC2VP_PV(BOOL (*idle)(void),ec2_domain *DOM,octet *W,octet *H,octet *D,octet *V)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,DOM->words,0);
#else
    miracl *mr_mip=mirsys(DOM->words,0);
#endif
	big r,a,b,gx,gy,v,wx,wy,d,h;
    epoint *G,*WP,*P;
    BOOL compressed,valid;
    int bit,M,aa,bb,cc,err,res=0;
#ifndef MR_STATIC
    char *mem;
    char *mem1;
#else
    char mem[MR_BIG_RESERVE(10)];
    char mem1[MR_ECP_RESERVE(3)];
    memset(mem,0,MR_BIG_RESERVE(10));
    memset(mem1,0,MR_ECP_RESERVE(3));
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 10);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
    mem1=(char *)ecp_memalloc(_MIPP_ 3);
    if (mem1==NULL) res=MR_P1363_OUT_OF_MEMORY; 
#endif
    if (res==0)
    {
        r=mirvar_mem(_MIPP_ mem, 0);
        a=mirvar_mem(_MIPP_ mem, 1);
        b=mirvar_mem(_MIPP_ mem, 2);
        gx=mirvar_mem(_MIPP_ mem, 3);
        gy=mirvar_mem(_MIPP_ mem, 4);
        v=mirvar_mem(_MIPP_ mem, 5);
        wx=mirvar_mem(_MIPP_ mem, 6);
        wy=mirvar_mem(_MIPP_ mem, 7);
        d=mirvar_mem(_MIPP_ mem, 8);
        h=mirvar_mem(_MIPP_ mem, 9);

        M=DOM->M;
        aa=DOM->a; bb=DOM->b;  cc=DOM->c; 
        OS2FEP(_MIPP_ &DOM->R,r);
        OS2FEP(_MIPP_ &DOM->Gx,gx);
        OS2FEP(_MIPP_ &DOM->Gy,gy);
        OS2FEP(_MIPP_ &DOM->B,b);
        OS2FEP(_MIPP_ &DOM->A,a);
        OS2FEP(_MIPP_ H,h);
        OS2FEP(_MIPP_ D,d);
        if (compare(d,r)>=0) res=MR_P1363_INVALID;
    }
    if (res==0)
    {
        ecurve2_init(_MIPP_ M,aa,bb,cc,a,b,FALSE,MR_PROJECTIVE);
        G=epoint_init_mem(_MIPP_ mem1,0);   
        WP=epoint_init_mem(_MIPP_ mem1,1);
        P=epoint_init_mem(_MIPP_ mem1,2);   
        epoint2_set(_MIPP_ gx,gy,0,G);

        compressed=OS2ECP(_MIPP_ W,wx,wy,DOM->fsize,&bit);
        if (!compressed)
             valid=epoint2_set(_MIPP_ wx,wy,0,WP);
        else valid=epoint2_set(_MIPP_ wx,wx,bit,WP);
        if (!valid) res=MR_P1363_ERROR;
        else
        {
            ecurve2_mult2(_MIPP_ d,G,h,WP,P);
            if (P->marker==MR_EPOINT_INFINITY) res=MR_P1363_INVALID;
            else
            {
                epoint2_get(_MIPP_ P,v,v);
                FE2OSP(_MIPP_ v,DOM->fsize,V);
            }
        }
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,10);
    ecp_memkill(_MIPP_ mem1,3);
#else
    memset(mem,0,MR_BIG_RESERVE(10));
    memset(mem1,0,MR_ECP_RESERVE(3));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

/*** RSA/RW primitives - support functions ***/
/* destroy the Public Key structure */

P1363_API void IF_PUBLIC_KEY_KILL(if_public_key *PUB)
{
    OCTET_KILL(&PUB->N);
    PUB->words=PUB->fsize=0; PUB->e=0;
}

/* destroy the Private Key structure */

P1363_API void IF_PRIVATE_KEY_KILL(if_private_key *PRIV)
{
    OCTET_KILL(&PRIV->P);
    OCTET_KILL(&PRIV->Q);
    OCTET_KILL(&PRIV->DP);
    OCTET_KILL(&PRIV->DQ);
    OCTET_KILL(&PRIV->C);
    PRIV->words=0; 
}

/* generate an RSA/RW key pair. The size of the public key in bits
 * is passed as a parameter. If the exponent E is even, then an RW key pair
 * is generated, otherwise an RSA key pair */

P1363_API int IF_KEY_PAIR(BOOL (*idle)(void),csprng *RNG,int bits,int E,if_private_key *PRIV,if_public_key *PUB)
{ /* A16.11/A16.12 more or less */
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
#endif
    miracl *mr_mip;
    int hE,m,r,bytes,hbytes,words,err,res=0;
    big p,q,dp,dq,c,n,t,p1,q1;
    BOOL RW;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(9)];
    memset(mem,0,MR_BIG_RESERVE(9));
#endif
    words=MR_ROUNDUP(bits,MIRACL);
#ifdef MR_GENERIC_AND_STATIC
	mr_mip=mirsys(&instance,words,0);
#else
	mr_mip=mirsys(words,0);
#endif
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    mr_mip->NTRY=50;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 9);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        p=mirvar_mem(_MIPP_ mem, 0);
        q=mirvar_mem(_MIPP_ mem, 1);
        dp=mirvar_mem(_MIPP_ mem, 2);
        dq=mirvar_mem(_MIPP_ mem, 3);
        c=mirvar_mem(_MIPP_ mem, 4);
        n=mirvar_mem(_MIPP_ mem, 5);
        p1=mirvar_mem(_MIPP_ mem, 6);
        q1=mirvar_mem(_MIPP_ mem, 7);
        t=mirvar_mem(_MIPP_ mem, 8);
        RW=FALSE;
        if (E%2==0)
        {
            RW=TRUE;   /* Rabin-Williams */
            hE=E/2;
        }

        PRIV->words=words;
        PUB->words=words;
        PUB->e=E;
    
        bytes=MR_ROUNDUP(bits,8);
     
        PUB->fsize=bytes;

        OCTET_INIT(&PUB->N,bytes);
        hbytes=2+(bytes/2);
        OCTET_INIT(&PRIV->P,hbytes);
        OCTET_INIT(&PRIV->Q,hbytes);
        OCTET_INIT(&PRIV->DP,hbytes);
        OCTET_INIT(&PRIV->DQ,hbytes);
        OCTET_INIT(&PRIV->C,hbytes);
        gprime(_MIPP_ 10000);
        forever
        {
            if (mr_mip->ERNUM) break;
            m=(bits+1)/2;
            expb2(_MIPP_ m,t);
            do
            {
                if (mr_mip->ERNUM) break;
                strong_bigrand(_MIPP_ RNG,t,p);
            } while (logb2(_MIPP_ p)<m); 
            if (remain(_MIPP_ p,2)==0) incr(_MIPP_ p,1,p);
            if (RW) 
            { /* make p=3 mod 8 */
                r=remain(_MIPP_ p,8);
                incr(_MIPP_ p,(3-r)%8,p);
            }
            do
            {
                if (mr_mip->ERNUM) break;
                if (RW) incr(_MIPP_ p,6,p);
                incr(_MIPP_ p,1,p);
                if (RW)
                {
                    if (hE==1) r=1;
                    else r=remain(_MIPP_ p,hE);
                } 
                else r=remain(_MIPP_ p,E);
                incr(_MIPP_ p,1,p);
                if (r==0) continue;
            } while (!isprime(_MIPP_ p));

            expb2(_MIPP_ bits,t);
            divide(_MIPP_ t,p,t);
            expb2(_MIPP_ bits-1,c);
            divide(_MIPP_ c,p,c);
            incr(_MIPP_ c,1,c);
            do
            {
                if (mr_mip->ERNUM) break;
                strong_bigrand(_MIPP_ RNG,t,q);
            } while (compare(q,c)<0);
            if (remain(_MIPP_ q,2)==0) incr(_MIPP_ q,1,q);

            if (RW) 
            { /* make q=7 mod 8 */
                r=remain(_MIPP_ q,8);
                incr(_MIPP_ q,(7-r)%8,q);
            }
            do
            {
                if (mr_mip->ERNUM) break;
                if (RW) incr(_MIPP_ q,6,q);
                incr(_MIPP_ q,1,q);
                if (RW)
                {
                    if (hE==1) r=1;
                    else r=remain(_MIPP_ q,hE);
                } 
                else r=remain(_MIPP_ q,E);
                incr(_MIPP_ q,1,q);
                if (r==0) continue;
            } while (!isprime(_MIPP_ q));
            multiply(_MIPP_ p,q,n);
            if (logb2(_MIPP_ n)!=bits) continue;  /* very rare! */
            decr(_MIPP_ p,1,p1);
            decr(_MIPP_ q,1,q1);
            multiply(_MIPP_ p1,q1,c);
            egcd(_MIPP_ p1,q1,t);
            divide(_MIPP_ c,t,c);             /* c=LCM(p-1,q-1) */
            if (RW) subdiv(_MIPP_ c,2,c);
            convert(_MIPP_ E,t);
           
            if (xgcd(_MIPP_ t,c,dp,dp,dp)!=1) continue;
            break;
        }
        copy(dp,dq); 
        divide(_MIPP_ dp,p1,p1);
        divide(_MIPP_ dq,q1,q1);
        xgcd(_MIPP_ q,p,c,c,c);           /* c=1/q mod p */
      
        convert_big_octet(_MIPP_ n,&PUB->N);
        convert_big_octet(_MIPP_ p,&PRIV->P);
        convert_big_octet(_MIPP_ q,&PRIV->Q);
        convert_big_octet(_MIPP_ dp,&PRIV->DP);
        convert_big_octet(_MIPP_ dq,&PRIV->DQ);
        convert_big_octet(_MIPP_ c,&PRIV->C);
    }
#ifndef MR_STATIC       
    memkill(_MIPP_ mem,9);
#else
    memset(mem,0,MR_BIG_RESERVE(9));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    if (res<0) return res;
    else return bytes;
}

static void private_key_op(_MIPD_ big p,big q,big dp,big dq,big c,big i,big s)
{ /* internal:- basic RSA/RW decryption operation */
    big jp,jq;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(2)];
    memset(mem,0,MR_BIG_RESERVE(2));
#endif    
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 2);
#endif
    jp=mirvar_mem(_MIPP_ mem,0);
    jq=mirvar_mem(_MIPP_ mem,1);
    powmod(_MIPP_ i,dp,p,jp);
    powmod(_MIPP_ i,dq,q,jq);
    subtract(_MIPP_ jp,jq,jp);
    mad(_MIPP_ c,jp,jp,p,p,s);
    if (size(s)<0) add(_MIPP_ s,p,s);
    multiply(_MIPP_ s,q,jp);
    add(_MIPP_ jq,jp,s);
#ifndef MR_STATIC       
    memkill(_MIPP_ mem,2);
#else
    memset(mem,0,MR_BIG_RESERVE(2));
#endif
}

/*** P1363 RSA/RW primitives ***/
/* See P1363 documentation for more details */

P1363_API int IFEP_RSA(BOOL (*idle)(void),if_public_key *PUB,octet *F,octet *G)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,PUB->words,0);
#else
    miracl *mr_mip=mirsys(PUB->words,0);
#endif
	int e,err,res=0;
    big f,g,n;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(3)];
    memset(mem,0,MR_BIG_RESERVE(3));
#endif  
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 3);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        f=mirvar_mem(_MIPP_ mem, 0);
        g=mirvar_mem(_MIPP_ mem, 1);
        n=mirvar_mem(_MIPP_ mem, 2);

        OS2FEP(_MIPP_ &PUB->N,n);
        OS2FEP(_MIPP_ F,f);
        if (compare(f,n)>=0) res=MR_P1363_BAD_ASSUMPTION;
    }
    if (res==0)
    {    
        e=PUB->e;
        power(_MIPP_ f,e,n,g);
        convert_big_octet(_MIPP_ g,G);
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,3);
#else
    memset(mem,0,MR_BIG_RESERVE(3));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int IFDP_RSA(BOOL (*idle)(void),if_private_key *PRIV,octet *G,octet *F)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,PRIV->words,0);
#else
	miracl *mr_mip=mirsys(PRIV->words,0);
#endif
	int err,res=0;
    big f,g,p,q,dp,dq,c,n;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(8)];
    memset(mem,0,MR_BIG_RESERVE(8));
#endif 
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 8);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        f=mirvar_mem(_MIPP_ mem, 0);
        g=mirvar_mem(_MIPP_ mem, 1);
        p=mirvar_mem(_MIPP_ mem, 2);
        q=mirvar_mem(_MIPP_ mem, 3);
        dp=mirvar_mem(_MIPP_ mem, 4);
        dq=mirvar_mem(_MIPP_ mem, 5);
        c=mirvar_mem(_MIPP_ mem, 6);
        n=mirvar_mem(_MIPP_ mem, 7);

        OS2FEP(_MIPP_ &PRIV->P,p);
        OS2FEP(_MIPP_ &PRIV->Q,q);
        OS2FEP(_MIPP_ &PRIV->DP,dp);
        OS2FEP(_MIPP_ &PRIV->DQ,dq);
        OS2FEP(_MIPP_ &PRIV->C,c);
        OS2FEP(_MIPP_ G,g);
        multiply(_MIPP_ p,q,n);
        if (compare(g,n)>=0) res=MR_P1363_BAD_ASSUMPTION;
    }
    if (res==0)
    {    
        private_key_op(_MIPP_ p,q,dp,dq,c,g,f);
        convert_big_octet(_MIPP_ f,F);
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,8);
#else
    memset(mem,0,MR_BIG_RESERVE(8));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int IFSP_RSA1(BOOL (*idle)(void),if_private_key *PRIV,octet *F,octet *S)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,PRIV->words,0);
#else
    miracl *mr_mip=mirsys(PRIV->words,0);
#endif
	int err,res=0;
    big f,s,p,q,dp,dq,c,n;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(8)];
    memset(mem,0,MR_BIG_RESERVE(8));
#endif 
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 8);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        s=mirvar_mem(_MIPP_ mem, 0);
        f=mirvar_mem(_MIPP_ mem, 1);
        p=mirvar_mem(_MIPP_ mem, 2);
        q=mirvar_mem(_MIPP_ mem, 3);
        dp=mirvar_mem(_MIPP_ mem, 4);
        dq=mirvar_mem(_MIPP_ mem, 5);
        c=mirvar_mem(_MIPP_ mem, 6);
        n=mirvar_mem(_MIPP_ mem, 7);

        OS2FEP(_MIPP_ &PRIV->P,p);
        OS2FEP(_MIPP_ &PRIV->Q,q);
        OS2FEP(_MIPP_ &PRIV->DP,dp);
        OS2FEP(_MIPP_ &PRIV->DQ,dq);
        OS2FEP(_MIPP_ &PRIV->C,c);
        OS2FEP(_MIPP_ F,f);
        multiply(_MIPP_ p,q,n);
        if (compare(f,n)>=0) res=MR_P1363_BAD_ASSUMPTION;
    }
    if (res==0)
    {    
        private_key_op(_MIPP_ p,q,dp,dq,c,f,s);
        convert_big_octet(_MIPP_ s,S);
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,8);
#else
    memset(mem,0,MR_BIG_RESERVE(8));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int IFVP_RSA1(BOOL (*idle)(void),if_public_key *PUB,octet *S,octet *F)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,PUB->words,0);
#else
    miracl *mr_mip=mirsys(PUB->words,0);
#endif
	int e,err,res=0;
    big f,s,n;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(3)];
    memset(mem,0,MR_BIG_RESERVE(3));
#endif 
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 3);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        s=mirvar_mem(_MIPP_ mem, 0);
        f=mirvar_mem(_MIPP_ mem, 1);
        n=mirvar_mem(_MIPP_ mem, 2);

        OS2FEP(_MIPP_ &PUB->N,n);
        OS2FEP(_MIPP_ S,s);
        if (compare(s,n)>=0) res=MR_P1363_INVALID;
    }
    if (res==0)
    {        
        e=PUB->e;
        power(_MIPP_ s,e,n,f);
        convert_big_octet(_MIPP_ f,F);
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,3);
#else
    memset(mem,0,MR_BIG_RESERVE(3));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int IFSP_RSA2(BOOL (*idle)(void),if_private_key *PRIV,octet *F,octet *S)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,PRIV->words,0);
#else
    miracl *mr_mip=mirsys(PRIV->words,0);
#endif
	int err,res=0;
    big f,s,p,q,dp,dq,c,n,t;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(9)];
    memset(mem,0,MR_BIG_RESERVE(9));
#endif 
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 9);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        s=mirvar_mem(_MIPP_ mem, 0);
        f=mirvar_mem(_MIPP_ mem, 1);
        p=mirvar_mem(_MIPP_ mem, 2);
        q=mirvar_mem(_MIPP_ mem, 3);
        dp=mirvar_mem(_MIPP_ mem, 4);
        dq=mirvar_mem(_MIPP_ mem, 5);
        c=mirvar_mem(_MIPP_ mem, 6);
        n=mirvar_mem(_MIPP_ mem, 7);
        t=mirvar_mem(_MIPP_ mem, 8);

        OS2FEP(_MIPP_ &PRIV->P,p);
        OS2FEP(_MIPP_ &PRIV->Q,q);
        OS2FEP(_MIPP_ &PRIV->DP,dp);
        OS2FEP(_MIPP_ &PRIV->DQ,dq);
        OS2FEP(_MIPP_ &PRIV->C,c);
        OS2FEP(_MIPP_ F,f);
        multiply(_MIPP_ p,q,n);
        if (remain(_MIPP_ f,16)!=12) res=MR_P1363_BAD_ASSUMPTION;
        if (compare(f,n)>=0) res=MR_P1363_BAD_ASSUMPTION;
    }
    if (res==0)
    {
        private_key_op(_MIPP_ p,q,dp,dq,c,f,s);
        subtract(_MIPP_ n,s,t);
        if (compare(s,t)>0)
            convert_big_octet(_MIPP_ t,S);
        else
            convert_big_octet(_MIPP_ s,S);
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,9);
#else
    memset(mem,0,MR_BIG_RESERVE(9));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int IFVP_RSA2(BOOL (*idle)(void),if_public_key *PUB,octet *S,octet *F)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,PUB->words,0);
#else
    miracl *mr_mip=mirsys(PUB->words,0);
#endif
	int e,err,res=0;
    big f,s,n;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(3)];
    memset(mem,0,MR_BIG_RESERVE(3));
#endif 
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 3);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        s=mirvar_mem(_MIPP_ mem, 0);
        f=mirvar_mem(_MIPP_ mem, 1);
        n=mirvar_mem(_MIPP_ mem, 2);
   
        OS2FEP(_MIPP_ &PUB->N,n);
        OS2FEP(_MIPP_ S,s);
        decr(_MIPP_ n,1,f);
        subdiv(_MIPP_ f,2,f);
        if (compare(s,f)>0) res=MR_P1363_INVALID;
    }
    if (res==0)
    {        
        e=PUB->e;
        power(_MIPP_ s,e,n,f);
        if (remain(_MIPP_ f,16)==12) 
            convert_big_octet(_MIPP_ f,F);
        else
        {
            subtract(_MIPP_ n,f,f);
            if (remain(_MIPP_ f,16)==12) 
                convert_big_octet(_MIPP_ f,F);
            else
                res=MR_P1363_INVALID;
        }
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,3);
#else
    memset(mem,0,MR_BIG_RESERVE(3));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int IFSP_RW(BOOL (*idle)(void),if_private_key *PRIV,octet *F,octet *S)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
	miracl *mr_mip=mirsys(&instance,PRIV->words,0);
#else
    miracl *mr_mip=mirsys(PRIV->words,0);
#endif
	int err,res=0;
    big f,s,p,q,dp,dq,c,n,t;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(9)];
    memset(mem,0,MR_BIG_RESERVE(9));
#endif 
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 9);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        s=mirvar_mem(_MIPP_ mem, 0);
        f=mirvar_mem(_MIPP_ mem, 1);
        p=mirvar_mem(_MIPP_ mem, 2);
        q=mirvar_mem(_MIPP_ mem, 3);
        dp=mirvar_mem(_MIPP_ mem, 4);
        dq=mirvar_mem(_MIPP_ mem, 5);
        c=mirvar_mem(_MIPP_ mem, 6);
        n=mirvar_mem(_MIPP_ mem, 7);
        t=mirvar_mem(_MIPP_ mem, 8);

        OS2FEP(_MIPP_ &PRIV->P,p);
        OS2FEP(_MIPP_ &PRIV->Q,q);
        OS2FEP(_MIPP_ &PRIV->DP,dp);
        OS2FEP(_MIPP_ &PRIV->DQ,dq);
        OS2FEP(_MIPP_ &PRIV->C,c);
        OS2FEP(_MIPP_ F,f);
        multiply(_MIPP_ p,q,n);
        if (remain(_MIPP_ f,16)!=12) res=MR_P1363_BAD_ASSUMPTION;
        if (compare(f,n)>=0) res=MR_P1363_BAD_ASSUMPTION;
    }
    if (res==0)
    {
        if (jack(_MIPP_ f,n)!=1)
            subdiv(_MIPP_ f,2,f);
        private_key_op(_MIPP_ p,q,dp,dq,c,f,s);
        subtract(_MIPP_ n,s,t);
        if (compare(s,t)>0)
            convert_big_octet(_MIPP_ t,S);
        else
            convert_big_octet(_MIPP_ s,S);

    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,9);
#else
    memset(mem,0,MR_BIG_RESERVE(9));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

P1363_API int IFVP_RW(BOOL (*idle)(void),if_public_key *PUB,octet *S,octet *F)
{
#ifdef MR_GENERIC_AND_STATIC
	miracl instance;
    miracl *mr_mip=mirsys(&instance,PUB->words,0);
#else
    miracl *mr_mip=mirsys(PUB->words,0);
#endif
	int e,err,res=0;
    big f,s,n,t1,t2;
#ifndef MR_STATIC
    char *mem;
#else
    char mem[MR_BIG_RESERVE(5)];
    memset(mem,0,MR_BIG_RESERVE(5));
#endif 
    if (mr_mip==NULL) return MR_P1363_OUT_OF_MEMORY;
    mr_mip->ERCON=TRUE;
    set_user_function(_MIPP_ idle);
#ifndef MR_STATIC
    mem=(char *)memalloc(_MIPP_ 5);
    if (mem==NULL) res=MR_P1363_OUT_OF_MEMORY;
#endif
    if (res==0)
    {
        s=mirvar_mem(_MIPP_ mem, 0);
        f=mirvar_mem(_MIPP_ mem, 1);
        n=mirvar_mem(_MIPP_ mem, 2);
        t1=mirvar_mem(_MIPP_ mem, 3);
        t2=mirvar_mem(_MIPP_ mem, 4);
   
        OS2FEP(_MIPP_ &PUB->N,n);
        OS2FEP(_MIPP_ S,s);
        decr(_MIPP_ n,1,f);
        subdiv(_MIPP_ f,2,f);
        if (compare(s,f)>0) res=MR_P1363_INVALID;
    }
    if (res==0)
    {        
        e=PUB->e;
        power(_MIPP_ s,e,n,t1);
        subtract(_MIPP_ n,t1,t2);

        if (remain(_MIPP_ t1,16)==12) 
            convert_big_octet(_MIPP_ t1,F);
        else
        {
            if (remain(_MIPP_ t1,8)==6)
            {
                premult(_MIPP_ t1,2,f);
                convert_big_octet(_MIPP_ f,F);
            }
            else
            { 
                if (remain(_MIPP_ t2,16)==12)
                    convert_big_octet(_MIPP_ t2,F);
                else
                {
                    if (remain(_MIPP_ t2,8)==6)
                    {
                        premult(_MIPP_ t2,2,f);
                        convert_big_octet(_MIPP_ f,F);
                    }
                    else res=MR_P1363_INVALID;
                }  
            }
        }
    }
#ifndef MR_STATIC
    memkill(_MIPP_ mem,5);
#else
    memset(mem,0,MR_BIG_RESERVE(5));
#endif
    err=mr_mip->ERNUM;
    mirexit(_MIPPO_ );
    if (err==MR_ERR_OUT_OF_MEMORY) return MR_P1363_OUT_OF_MEMORY;
    if (err==MR_ERR_DIV_BY_ZERO) return MR_P1363_DIV_BY_ZERO;
    if (err!=0) return -(1000+err);
    return res;
}

