
/* simplest possible secure rsa implementation */
/* Uses IEEE-1363 standard methods  */
/* See IEEE-1363 section 11.2 IFES, page 57 */
/* cl /O2 rsa.c p1363.c miracl.lib    */
 
#include <stdio.h>
#include "p1363.h"

int main()
{   
    int i,bytes,res;
    octet m,m1,c,e,raw;
    if_public_key pub;
    if_private_key priv;
    csprng RNG;  
                           /* some true randomness needed here */
    OCTET_INIT(&raw,100);  /* should be filled with 100 random bytes */

    CREATE_CSPRNG(&RNG,&raw);   /* initialise strong RNG */

    bytes=IF_KEY_PAIR(NULL,&RNG,1024,65537,&priv,&pub);
                                /* generate random public & private key */
    OCTET_INIT(&m,bytes);       /* allocate space for plaintext and ciphertext */
    OCTET_INIT(&m1,bytes);
    OCTET_INIT(&c,bytes);   OCTET_INIT(&e,bytes);

    OCTET_JOIN_STRING((char *)"Hello World\n",&m);

    EME1_ENCODE(&m,&RNG,1023,NULL,SHA256,&e); /* OAEP encode message m to e  */
                                              /* must be less than 1024 bits */

    IFEP_RSA(NULL,&pub,&e,&c);     /* encrypt encoded message */
    IFDP_RSA(NULL,&priv,&c,&m1);   /* ... and then decrypt it */

    EME1_DECODE(&m1,1023,NULL,SHA256,&m1);    /* decode it */

    OCTET_PRINT_STRING(&m1);       /* print out decrypted/decoded message */

    OCTET_KILL(&m); OCTET_KILL(&m1);   /* clean up afterwards */
    OCTET_KILL(&c); OCTET_KILL(&raw); OCTET_KILL(&e); 

    IF_PUBLIC_KEY_KILL(&pub);          /* kill the keys */
    IF_PRIVATE_KEY_KILL(&priv);
}

