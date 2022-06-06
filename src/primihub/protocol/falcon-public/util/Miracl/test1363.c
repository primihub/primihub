
/* test driver and function exerciser for P1363 Functions */
/* NOTE: its important to initialize OCTETs to the correct size */
/* see comments in p1363.c */

/* define this next to use Windows DLL */
/* #define MR_P1363_DLL  */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "p1363.h"

int main(int argc,char **argv)
{   
    int i,ip,mlen,precompute;
    BOOL compress,dhaes,result;
    octet h,s,p,f,g,c,d,u,v,w,m,m1,tag,tag1;
    octet s0,s1,w0,w1,u0,u1,v0,v1,k1,k2,z,vz;
    octet z1,z2,f1,f2,f3,k;
    octet p1,p2,L2,C;
    octet raw;
    unsigned long ran;
    dl_domain dom;
    ecp_domain epdom;
    ec2_domain e2dom;
    if_public_key pub;
    if_private_key priv;
    csprng RNG;                  /* Crypto Strong RNG */
 
    int res,bytes,bits;

    argc--; argv++;
    compress=FALSE;
    precompute=0;

    ip=0;
    while (ip<argc)
    {
        if (strcmp(argv[ip],"-c")==0)
        {
            compress=TRUE;
            ip++; 
            continue;
        }

        if (strcmp(argv[ip],"-p")==0)
        {
            precompute=8;
            ip++; 
            continue;
        }
 
        printf("Command line error\n");
        exit(0);
    }

    time((time_t *)&ran);
                               /* fake random seed source */
    OCTET_INIT(&raw,100);
    raw.len=100;
    raw.val[0]=ran;
    raw.val[1]=ran>>8;
    raw.val[2]=ran>>16;
    raw.val[3]=ran>>24;
    for (i=4;i<100;i++) raw.val[i]=i+1;

    CREATE_CSPRNG(&RNG,&raw);   /* initialise strong RNG */

    OCTET_KILL(&raw);

/*
    OCTET_INIT(&k,100);
    OCTET_JOIN_BYTE(0x0b,20,&k);
    OCTET_INIT(&m,100);
    OCTET_JOIN_STRING("Hi There",&m);
    OCTET_INIT(&d,100);

    if (!MAC1(&m,NULL,&k,20,SHA1,&d)) printf("failed\n");

    OCTET_OUTPUT(&k);
    OCTET_OUTPUT(&m);
    OCTET_OUTPUT(&d);

    OCTET_KILL(&k);
    OCTET_KILL(&m);
    OCTET_KILL(&d); 
*/

    bytes=DL_DOMAIN_INIT(&dom,"common.dss",NULL,precompute);

    res=DL_DOMAIN_VALIDATE(NULL,&dom);
    if (res!=0)
    {
        printf("Domain parameters are invalid\n");
        return 0;
    }
    OCTET_INIT(&s0,bytes);
    OCTET_INIT(&s1,bytes);
    OCTET_INIT(&w0,bytes);
    OCTET_INIT(&w1,bytes);

    DL_KEY_PAIR_GENERATE(NULL,&dom,&RNG,&s0,&w0);
    DL_KEY_PAIR_GENERATE(NULL,&dom,&RNG,&s1,&w1);

    OCTET_INIT(&z,bytes);
    OCTET_INIT(&z1,bytes);
    OCTET_INIT(&z2,bytes);
    DLSVDP_DH(NULL,&dom,&s0,&w1,&z);
    DLSVDP_DH(NULL,&dom,&s1,&w0,&z1);

    if (OCTET_COMPARE(&z,&z1))
        printf("DLSVDP-DH - OK\n");
    else
    {
        printf("*** DLSVDP-DH Failed\n");
        return 0;
    }

    res=DLSVDP_DHC(NULL,&dom,&s1,&w0,TRUE,&z2);


    if (OCTET_COMPARE(&z,&z2))
        printf("DLSVDP-DHC Compatibility Mode - OK\n");
    else
    {
        printf("*** DLSVDP-DHC Compatibility Mode Failed\n");
        return 0;
    }

    DLSVDP_DHC(NULL,&dom,&s0,&w1,FALSE,&z);
    DLSVDP_DHC(NULL,&dom,&s1,&w0,FALSE,&z1);

    if (OCTET_COMPARE(&z,&z1))
        printf("DLSVDP-DHC - OK\n");
    else
    {
        printf("*** DLSVDP-DHC Failed\n");
        return 0;
    }

    OCTET_INIT(&u0,bytes);
    OCTET_INIT(&u1,bytes);
    OCTET_INIT(&v0,bytes);
    OCTET_INIT(&v1,bytes);

    DL_KEY_PAIR_GENERATE(NULL,&dom,&RNG,&u0,&v0);
    DL_KEY_PAIR_GENERATE(NULL,&dom,&RNG,&u1,&v1);

    DLSVDP_MQV(NULL,&dom,&s0,&u0,&v0,&w1,&v1,&z);
    DLSVDP_MQV(NULL,&dom,&s1,&u1,&v1,&w0,&v0,&z1);

    if (OCTET_COMPARE(&z,&z1))
        printf("DLSVDP-MQV - OK\n");
    else
    {
        printf("*** DLSVDP-MQV Failed\n");
        return 0;
    }

    DLSVDP_MQVC(NULL,&dom,&s0,&u0,&v0,&w1,&v1,TRUE,&z2);

    if (OCTET_COMPARE(&z,&z2))
        printf("DLSVDP-MQVC Compatibility Mode - OK\n");
    else
    {
        printf("*** DLSVDP-MQVC Compatibility Mode Failed\n");
        return 0;
    }

    DLSVDP_MQVC(NULL,&dom,&s0,&u0,&v0,&w1,&v1,FALSE,&z);
    DLSVDP_MQVC(NULL,&dom,&s0,&u0,&v0,&w1,&v1,FALSE,&z1);

    if (OCTET_COMPARE(&z,&z1))
        printf("DLSVDP-MQVC - OK\n");
    else
    {
        printf("*** DLSVDP-MQVC Failed\n");
        return 0;
    }

    OCTET_INIT(&s,bytes);
    OCTET_INIT(&p,bytes);

    DL_KEY_PAIR_GENERATE(NULL,&dom,&RNG,&s,&p);
    res=DL_PUBLIC_KEY_VALIDATE(NULL,&dom,TRUE,&p);
    if (res!=0)
    {
        printf("DL Public Key is invalid!\n");
        return 0;
    }

    OCTET_INIT(&f,bytes+1);
    OCTET_INIT(&c,bytes);
    OCTET_INIT(&d,bytes);
    OCTET_INIT(&f1,bytes);
    OCTET_INIT(&f2,bytes);

    f.len=16;                   /* fake a message */
    for (i=0;i<16;i++) f.val[i]=i+1;          
                                /* make sure its less than group order */
                                /* because we will use the same message */
                                /* to test signature with message recovery */
    OCTET_INIT(&k,16);
    k.len=16; for (i=0;i<16;i++) k.val[i]=1;    /* fake an AES key */

    res=AES_CBC_IV0_ENCRYPT(&k,&f,NULL,&f1,NULL);
    res=AES_CBC_IV0_DECRYPT(&k,&f1,NULL,&f2,NULL);

    if (OCTET_COMPARE(&f,&f2))
        printf("AES Encrypt/Decrypt OK\n");
    else
    {
        printf("*** AES Encrypt/Decrypt Failed\n");
        return 0;
    }

    OCTET_KILL(&k);

/* Test OAEP/EME1 encoding and decoding */

    OCTET_INIT(&f3,128);
    res=EME1_ENCODE(&f,&RNG,1023,NULL,SHA256,&f3);
    res=EME1_DECODE(&f3,1023,NULL,SHA256,&f2);
    if (OCTET_COMPARE(&f,&f2))
        printf("OAEP Encoding/Decoding - OK\n");
    else
    {
        printf("*** OAEP Encoding/Decoding Failed\n");
        return 0;
    }

    OCTET_KILL(&f2);
    OCTET_KILL(&f3);


    res=DLSP_DSA(NULL,&dom,&RNG,&s,&f,&c,&d); /* sign it   */
    res=DLVP_DSA(NULL,&dom,&p,&c,&d,&f);       /* verify it */
    if (res==0)
        printf("DL DSA Signature - OK\n");
    else
    {
        printf("*** DL DSA Signature Failed\n");
        return 0;
    }
    
    res=DLSP_NR(NULL,&dom,&RNG,&s,&f,&c,&d); /* sign it */
    res=DLVP_NR(NULL,&dom,&p,&c,&d,&f1);

    if (OCTET_COMPARE(&f,&f1))
        printf("DL  NR Signature - OK\n");
    else
    {
        printf("*** DL  NR Signature Failed\n");
        return 0;
    }
    
    OCTET_INIT(&u,bytes);
    OCTET_INIT(&v,bytes);

    res=DLPSP_NR2PV(NULL,&dom,&RNG,&u,&v);
    res=DLSP_NR2(NULL,&dom,&s,&u,&v,&f,&c,&d);
    res=DLVP_NR2(NULL,&dom,&p,&c,&d,&f1,&v1);

    if (OCTET_COMPARE(&f,&f1) && OCTET_COMPARE(&v,&v1))
        printf("DL NR2 Signature - OK\n");
    else
    {
        printf("*** DL NR2 Signature Failed\n");
        return 0;
    }

    res=DLPSP_NR2PV(NULL,&dom,&RNG,&u,&v);
    res=DLSP_PV(NULL,&dom,&s,&u,&f,&d);
    res=DLVP_PV(NULL,&dom,&p,&f,&d,&v1);

    if (OCTET_COMPARE(&v,&v1))
        printf("DL  PV Signature - OK\n");
    else
    {
        printf("*** DL  PV Signature Failed\n");
        return 0;
    }

    OCTET_KILL(&u);  OCTET_KILL(&v);
    OCTET_KILL(&s0); OCTET_KILL(&s1);
    OCTET_KILL(&w0); OCTET_KILL(&w1);
    OCTET_KILL(&z);  OCTET_KILL(&z1); OCTET_KILL(&z2);
    OCTET_KILL(&u0); OCTET_KILL(&u1);
    OCTET_KILL(&v0); OCTET_KILL(&v1);
    OCTET_KILL(&s);  OCTET_KILL(&p);
    OCTET_KILL(&f);  OCTET_KILL(&f1);
    OCTET_KILL(&c);  OCTET_KILL(&d);

    DL_DOMAIN_KILL(&dom);

/* Now test Elliptic Curves mod p */

    bytes=ECP_DOMAIN_INIT(&epdom,"common.ecs",NULL,precompute);
    res=ECP_DOMAIN_VALIDATE(NULL,&epdom);
    if (res!=0)
    {
        printf("Domain parameters are invalid %d\n",res);
        return 0;
    }
    OCTET_INIT(&s0,bytes);
    OCTET_INIT(&s1,bytes);
    OCTET_INIT(&w0,2*bytes+1);   
    OCTET_INIT(&w1,2*bytes+1); /* NOTE: these are EC points (x,y) */
                               /* hence the size of 2*bytes+1. However */
                               /* bytes+1 is OK if compression is used */

    ECP_KEY_PAIR_GENERATE(NULL,&epdom,&RNG,&s0,compress,&w0);
    res=ECP_PUBLIC_KEY_VALIDATE(NULL,&epdom,TRUE,&w0);
    if (res!=0)
    {
        printf("ECP Public Key is invalid!\n");
        return 0;
    }

    ECP_KEY_PAIR_GENERATE(NULL,&epdom,&RNG,&s1,compress,&w1);
    res=ECP_PUBLIC_KEY_VALIDATE(NULL,&epdom,TRUE,&w1);
    if (res!=0)
    {
        printf("ECP Public Key is invalid!\n");
        return 0;
    }

    OCTET_INIT(&z,bytes);
    OCTET_INIT(&z1,bytes);
    OCTET_INIT(&z2,bytes);

    ECPSVDP_DH(NULL,&epdom,&s0,&w1,&z);
    ECPSVDP_DH(NULL,&epdom,&s1,&w0,&z1);

    if (OCTET_COMPARE(&z,&z1))
        printf("ECPSVDP-DH - OK\n");
    else
    {
        printf("*** ECPSVDP-DH Failed\n");
        return 0;
    }

    ECPSVDP_DHC(NULL,&epdom,&s1,&w0,TRUE,&z2);

    if (OCTET_COMPARE(&z,&z2))
        printf("ECPSVDP-DHC Compatibility Mode - OK\n");
    else
    {
        printf("*** ECPSVDP-DHC Compatibility Mode Failed\n");
        return 0;
    }

    ECPSVDP_DHC(NULL,&epdom,&s0,&w1,FALSE,&z);
    ECPSVDP_DHC(NULL,&epdom,&s1,&w0,FALSE,&z1);

    if (OCTET_COMPARE(&z,&z1))
        printf("ECPSVDP-DHC - OK\n");
    else
    {
        printf("*** ECPSVDP-DHC Failed\n");
        return 0;
    }

    OCTET_INIT(&u0,bytes);
    OCTET_INIT(&u1,bytes);
    OCTET_INIT(&v0,2*bytes+1);
    OCTET_INIT(&v1,2*bytes+1);

    ECP_KEY_PAIR_GENERATE(NULL,&epdom,&RNG,&u0,compress,&v0);
    ECP_KEY_PAIR_GENERATE(NULL,&epdom,&RNG,&u1,compress,&v1);

    ECPSVDP_MQV(NULL,&epdom,&s0,&u0,&v0,&w1,&v1,&z);
    ECPSVDP_MQV(NULL,&epdom,&s1,&u1,&v1,&w0,&v0,&z1);

    if (OCTET_COMPARE(&z,&z1))
        printf("ECPSVDP-MQV - OK\n");
    else
    {
        printf("*** ECPSVDP-MQV Failed\n");
        return 0;
    }

    ECPSVDP_MQVC(NULL,&epdom,&s0,&u0,&v0,&w1,&v1,TRUE,&z2);
       
    if (OCTET_COMPARE(&z,&z2))
        printf("ECPSVDP_MQVC Compatibility Mode - OK\n");
    else
    {
        printf("*** ECPSVDP_MQVC Compatibility Mode Failed\n");
        return 0;
    }

    ECPSVDP_MQVC(NULL,&epdom,&s0,&u0,&v0,&w1,&v1,FALSE,&z);
    ECPSVDP_MQVC(NULL,&epdom,&s1,&u1,&v1,&w0,&v0,FALSE,&z1);

    if (OCTET_COMPARE(&z,&z1))
        printf("ECPSVDP-MQVC - OK\n");
    else
    {
        printf("*** ECPSVDP-MQVC Failed\n");
        return 0;
    }

    OCTET_INIT(&s,bytes);
    OCTET_INIT(&w,2*bytes+1);

    ECP_KEY_PAIR_GENERATE(NULL,&epdom,&RNG,&s,compress,&w);
    res=ECP_PUBLIC_KEY_VALIDATE(NULL,&epdom,TRUE,&w);

    if (res!=0)
    {
        printf("ECP Public Key is invalid!\n");
        return 0;
    }

    OCTET_INIT(&f,bytes);
    OCTET_INIT(&f1,bytes);
    OCTET_INIT(&c,bytes);
    OCTET_INIT(&d,bytes);
 
    f.len=16;                   /* fake a message */
    for (i=0;i<16;i++) f.val[i]=i+1;          
                                /* make sure its less than group order */

    res=ECPSP_DSA(NULL,&epdom,&RNG,&s,&f,&c,&d);
    res=ECPVP_DSA(NULL,&epdom,&w,&c,&d,&f);

    if (res==0)
        printf("ECP DSA  Signature - OK\n");
    else
    {
        printf("*** ECP DSA Signature Failed\n");
        return 0;
    } 

    res=ECPSP_NR(NULL,&epdom,&RNG,&s,&f,&c,&d);
    res=ECPVP_NR(NULL,&epdom,&w,&c,&d,&f1);

    if (OCTET_COMPARE(&f,&f1))
        printf("ECP  NR  Signature - OK\n");
    else
    {
        printf("*** ECP  NR Signature Failed\n");
        return 0;
    } 

    OCTET_INIT(&u,bytes);
    OCTET_INIT(&v,bytes);

    res=ECPPSP_NR2PV(NULL,&epdom,&RNG,&u,&v);
    res=ECPSP_NR2(NULL,&epdom,&s,&u,&v,&f,&c,&d);
    res=ECPVP_NR2(NULL,&epdom,&w,&c,&d,&f1,&v1);

    if (OCTET_COMPARE(&f,&f1) && OCTET_COMPARE(&v,&v1))
        printf("ECP NR2  Signature - OK\n");
    else
    {
        printf("*** ECP  NR2  Signature Failed\n");
        return 0;
    }

    res=ECPPSP_NR2PV(NULL,&epdom,&RNG,&u,&v);
    res=ECPSP_PV(NULL,&epdom,&s,&u,&f,&d);
    res=ECPVP_PV(NULL,&epdom,&w,&f,&d,&v1);

    if (OCTET_COMPARE(&v,&v1))
        printf("ECP  PV  Signature - OK\n");
    else
    {
        printf("*** ECP  PV  Signature Failed\n");
        return 0;
    }

    OCTET_KILL(&u); OCTET_KILL(&v);
    OCTET_KILL(&s0); OCTET_KILL(&s1);
    OCTET_KILL(&w0); OCTET_KILL(&w1);
    OCTET_KILL(&z); 
    OCTET_KILL(&z1); OCTET_KILL(&z2);
    OCTET_KILL(&u0); OCTET_KILL(&u1);
    OCTET_KILL(&v0); OCTET_KILL(&v1);
    OCTET_KILL(&s); 
    OCTET_KILL(&w); 
    OCTET_KILL(&f); OCTET_KILL(&f1);
    OCTET_KILL(&c); OCTET_KILL(&d);

    ECP_DOMAIN_KILL(&epdom);

/* Now test Elliptic Curves mod 2^m */

    bytes=EC2_DOMAIN_INIT(&e2dom,"common2.ecs",NULL,precompute);
    res=EC2_DOMAIN_VALIDATE(NULL,&e2dom);
    if (res!=0)
    {
        printf("Domain parameters are invalid\n",res);
        return 0;
    }

    OCTET_INIT(&s0,bytes);
    OCTET_INIT(&s1,bytes);
    OCTET_INIT(&w0,2*bytes+1);
    OCTET_INIT(&w1,2*bytes+1);

    EC2_KEY_PAIR_GENERATE(NULL,&e2dom,&RNG,&s0,compress,&w0);
    EC2_KEY_PAIR_GENERATE(NULL,&e2dom,&RNG,&s1,compress,&w1);

    OCTET_INIT(&z,bytes);
    OCTET_INIT(&z1,bytes);
    OCTET_INIT(&z2,bytes);

    EC2SVDP_DH(NULL,&e2dom,&s0,&w1,&z);
    EC2SVDP_DH(NULL,&e2dom,&s1,&w0,&z1);

    if (OCTET_COMPARE(&z,&z1))
        printf("EC2SVDP-DH - OK\n");
    else
    {
        printf("*** EC2SVDP-DH Failed\n");
        return 0;
    }

    EC2SVDP_DHC(NULL,&e2dom,&s1,&w0,TRUE,&z2);

    if (OCTET_COMPARE(&z,&z2))
        printf("EC2SVDP-DHC Compatibility Mode - OK\n");
    else
    {
        printf("*** EC2SVDP-DHC Compatibility Mode Failed\n");
        return 0;
    }

    EC2SVDP_DHC(NULL,&e2dom,&s0,&w1,FALSE,&z);
    EC2SVDP_DHC(NULL,&e2dom,&s1,&w0,FALSE,&z1);

    if (OCTET_COMPARE(&z,&z1))
        printf("EC2SVDP-DHC - OK\n");
    else
    {
        printf("*** EC2SVDP-DHC Failed\n");
        return 0;
    }

    OCTET_INIT(&u0,bytes);
    OCTET_INIT(&u1,bytes);
    OCTET_INIT(&v0,2*bytes+1);
    OCTET_INIT(&v1,2*bytes+1);

    EC2_KEY_PAIR_GENERATE(NULL,&e2dom,&RNG,&u0,compress,&v0);
    EC2_KEY_PAIR_GENERATE(NULL,&e2dom,&RNG,&u1,compress,&v1);

    EC2SVDP_MQV(NULL,&e2dom,&s0,&u0,&v0,&w1,&v1,&z);
    EC2SVDP_MQV(NULL,&e2dom,&s1,&u1,&v1,&w0,&v0,&z1);

    if (OCTET_COMPARE(&z,&z1))
        printf("EC2SVDP-MQV - OK\n");
    else
    {
        printf("*** EC2SVDP-MQV Failed\n");
        return 0;
    }

    EC2SVDP_MQVC(NULL,&e2dom,&s0,&u0,&v0,&w1,&v1,TRUE,&z2);
       
    if (OCTET_COMPARE(&z,&z2))
        printf("EC2SVDP_MQVC Compatibility Mode - OK\n");
    else
    {
        printf("*** EC2SVDP_MQVC Compatibility Mode Failed\n");
        return 0;
    }

    EC2SVDP_MQVC(NULL,&e2dom,&s0,&u0,&v0,&w1,&v1,FALSE,&z);
    EC2SVDP_MQVC(NULL,&e2dom,&s1,&u1,&v1,&w0,&v0,FALSE,&z1);

    if (OCTET_COMPARE(&z,&z1))
        printf("EC2SVDP-MQVC - OK\n");
    else
    {
        printf("*** EC2SVDP-MQVC Failed\n");
        return 0;
    }

    OCTET_INIT(&s,bytes);
    OCTET_INIT(&w,2*bytes+1);

    EC2_KEY_PAIR_GENERATE(NULL,&e2dom,&RNG,&s,compress,&w);
    res=EC2_PUBLIC_KEY_VALIDATE(NULL,&e2dom,TRUE,&w);

    if (res!=0)
    {
        printf("EC2 Public Key is invalid!\n");
        return 0;
    }

    OCTET_INIT(&f,bytes);
    OCTET_INIT(&f1,bytes);
    OCTET_INIT(&c,bytes);
    OCTET_INIT(&d,bytes);

    f.len=16;                   /* fake a message */
    for (i=0;i<16;i++) f.val[i]=i+1;          
                                /* make sure its less than group order */  

    res=EC2SP_DSA(NULL,&e2dom,&RNG,&s,&f,&c,&d);
    res=EC2VP_DSA(NULL,&e2dom,&w,&c,&d,&f);

    if (res==0)
        printf("EC2 DSA  Signature - OK\n");
    else
    {
        printf("*** EC2 DSA Signature Failed\n");
        return 0;
    } 

    res=EC2SP_NR(NULL,&e2dom,&RNG,&s,&f,&c,&d);
    res=EC2VP_NR(NULL,&e2dom,&w,&c,&d,&f1);

    if (OCTET_COMPARE(&f,&f1))
        printf("EC2  NR  Signature - OK\n");
    else
    {
        printf("*** EC2  NR Signature Failed\n");
        return 0;
    } 
    OCTET_INIT(&u,bytes);
    OCTET_INIT(&v,bytes);

    res=EC2PSP_NR2PV(NULL,&e2dom,&RNG,&u,&v);
    res=EC2SP_NR2(NULL,&e2dom,&s,&u,&v,&f,&c,&d);
    res=EC2VP_NR2(NULL,&e2dom,&w,&c,&d,&f1,&v1);

    if (OCTET_COMPARE(&f,&f1) && OCTET_COMPARE(&v,&v1))
        printf("EC2 NR2  Signature - OK\n");
    else
    {
        printf("*** EC2 NR2  Signature Failed\n");
        return 0;
    }

    res=EC2PSP_NR2PV(NULL,&e2dom,&RNG,&u,&v);
    res=EC2SP_PV(NULL,&e2dom,&s,&u,&f,&d);
    res=EC2VP_PV(NULL,&e2dom,&w,&f,&d,&v1);

    if (OCTET_COMPARE(&v,&v1))
        printf("EC2  PV  Signature - OK\n");
    else
    {
        printf("*** EC2  PV  Signature Failed\n");
        return 0;
    }

    OCTET_KILL(&u); OCTET_KILL(&v);
    OCTET_KILL(&s0); OCTET_KILL(&s1);
    OCTET_KILL(&w0); OCTET_KILL(&w1);
    OCTET_KILL(&z); 
    OCTET_KILL(&z1); OCTET_KILL(&z2);
    OCTET_KILL(&u0); OCTET_KILL(&u1);
    OCTET_KILL(&v0); OCTET_KILL(&v1);
    OCTET_KILL(&s); 
    OCTET_KILL(&w); 
    OCTET_KILL(&f); OCTET_KILL(&f1);
    OCTET_KILL(&c); OCTET_KILL(&d);

    EC2_DOMAIN_KILL(&e2dom);

    printf("Generating 1024-bit RSA Key Pair....\n");
    bytes=IF_KEY_PAIR(NULL,&RNG,1024,65537,&priv,&pub);

    OCTET_INIT(&f,bytes);
    OCTET_INIT(&f1,bytes);
    OCTET_INIT(&g,bytes);

    f.len=20;
    for (i=0;i<20;i++) f.val[i]=i+1;    /* fake a message */
    IFEP_RSA(NULL,&pub,&f,&g);
    IFDP_RSA(NULL,&priv,&g,&f1);

    if (OCTET_COMPARE(&f,&f1))
        printf("RSA Encryption - OK\n");
    else
    {
        printf("RSA Encryption Failed\n");
        return 0;
    } 

    OCTET_INIT(&s,bytes);
    OCTET_CLEAR(&f1);

    IFSP_RSA1(NULL,&priv,&f,&s);
    IFVP_RSA1(NULL,&pub,&s,&f1);

    if (OCTET_COMPARE(&f,&f1))
        printf("RSA1 Signature - OK\n");
    else
    {
        printf("RSA1 Signature Failed\n");
        return 0;
    } 

    f.len=20;
    for (i=0;i<20;i++) f.val[i]=i+1;    /* fake a message */
    f.val[19]=12;                       /* =12 mod 16     */
    OCTET_CLEAR(&f1);

    IFSP_RSA2(NULL,&priv,&f,&s);
    IFVP_RSA2(NULL,&pub,&s,&f1);

    if (OCTET_COMPARE(&f,&f1))
        printf("RSA2 Signature - OK\n");
    else
    {
        printf("RSA2 Signature Failed\n");
        return 0;
    } 

    IF_PUBLIC_KEY_KILL(&pub);
    IF_PRIVATE_KEY_KILL(&priv);

    printf("Generating 1024-bit RW Key Pair....\n");
    IF_KEY_PAIR(NULL,&RNG,1024,2,&priv,&pub);

    OCTET_CLEAR(&f1);
    IFSP_RW(NULL,&priv,&f,&s);
    IFVP_RW(NULL,&pub,&s,&f1);

    if (OCTET_COMPARE(&f,&f1))
        printf("RW Signature - OK\n");
    else
    {
        printf("RW Signature Failed\n");
        return 0;
    } 

    printf("\nP1363 IFSSA Signature\n");
    OCTET_CLEAR(&g);
    g.len=20;
    for (i=0;i<20;i++) g.val[i]=i+1;
    printf("Message is \n");
    OCTET_OUTPUT(&g);
    mlen=1023; 
    if (!EMSA4_ENCODE(TRUE,SHA256,10,&RNG,mlen,&g,NULL,&f))
    {
        printf("Failed to EMSA encode message\n");
        return 0;  
    }

    printf("Message Representative is\n");
    OCTET_OUTPUT(&f);
    IFSP_RW(NULL,&priv,&f,&s);
    printf("Signature is\n");
    OCTET_OUTPUT(&s);
    printf("Verification\n");
    IFVP_RW(NULL,&pub,&s,&f1);
    printf("Message Representative is\n");
    OCTET_OUTPUT(&f1);
    result=EMSA4_DECODE(TRUE,SHA256,10,mlen,&f1,&g,NULL);
    if (result && OCTET_COMPARE(&f,&f1))
        printf("IFSSA Signature - OK\n");
    else
    {
        printf("IFSSA Signature Failed\n");
        return 0;
    } 
    
    printf("\nP1363 IFSSR Signature with recovery\n");
    OCTET_CLEAR(&g);
    g.len=20;
    for (i=0;i<20;i++) g.val[i]=i+1;
    printf("Message is \n");
    OCTET_OUTPUT(&g);
    mlen=1023; 
    EMSR3_ENCODE(FALSE,SHA256,10,&RNG,mlen,&g,NULL,NULL,&f);
    printf("Message Representative is\n");
    OCTET_OUTPUT(&f);
    IFSP_RW(NULL,&priv,&f,&s);
    OCTET_CLEAR(&g);
    printf("Signature is\n");
    OCTET_OUTPUT(&s);
    printf("Verification\n");

    result=EMSR3_DECODE(FALSE,SHA256,10,1023,&f,NULL,NULL,&g);
    printf("Recovered message= \n");
    OCTET_OUTPUT(&g);
    if (result) 
        printf("IFSSR Signature with recovery - OK\n");
    else
    {
        printf("IFSSR Signature with recovery Failed\n");
        return 0;
    } 

    IF_PUBLIC_KEY_KILL(&pub);
    IF_PRIVATE_KEY_KILL(&priv);

    OCTET_KILL(&f); OCTET_KILL(&f1);
    OCTET_KILL(&g); OCTET_KILL(&s);

    printf("\nP1363 DLSSR Signature with message recovery\n");


    bytes=DL_DOMAIN_INIT(&dom,"common.dss",NULL,precompute);
    res=DL_DOMAIN_VALIDATE(NULL,&dom);
    if (res!=0)
    {
        printf("Domain parameters are invalid\n");
        return 0;
    }

    OCTET_INIT(&s,bytes);
    OCTET_INIT(&w,bytes);
    OCTET_INIT(&u,bytes);
    OCTET_INIT(&v,bytes);
    OCTET_INIT(&v1,bytes);
    OCTET_INIT(&c,bytes);
    OCTET_INIT(&d,bytes);
    OCTET_INIT(&f,bytes);
    OCTET_INIT(&f1,bytes);

    OCTET_INIT(&m1,8);

    DL_KEY_PAIR_GENERATE(NULL,&dom,&RNG,&s,&w);

    res=DLPSP_NR2PV(NULL,&dom,&RNG,&u,&v);
    bits=dom.rbits-1;

    m1.len=8;
    for (i=0;i<8;i++) m1.val[i]=i+1;

    printf("recoverable message= ");
    OCTET_OUTPUT(&m1);
    
    result=EMSR1_ENCODE(FALSE,10,20,&m1,NULL,NULL,bits,SHA1,&v,&f);
    res=DLSP_NR2(NULL,&dom,&s,&u,&v,&f,&c,&d);
    OCTET_CLEAR(&m1);
    
    printf("Signature= \n");
    OCTET_OUTPUT(&c);
    OCTET_OUTPUT(&d);
    res=DLVP_NR2(NULL,&dom,&w,&c,&d,&f1,&v1);

    result=EMSR1_DECODE(FALSE,10,20,&f,NULL,NULL,bits,SHA1,&v1,8,&m1);
    printf("recovered message=   ");
    OCTET_OUTPUT(&m1);
    if (result)
        printf("DLSSR Signature with message recovery - OK\n");
    else
    {
        printf("*** DLSSR Signature with message recovery Failed\n");
        return 0;
    } 

    OCTET_KILL(&s);
    OCTET_KILL(&w);
    OCTET_KILL(&u);
    OCTET_KILL(&v);
    OCTET_KILL(&v1);
    OCTET_KILL(&c);
    OCTET_KILL(&d);
    OCTET_KILL(&f);
    OCTET_KILL(&f1);
    OCTET_KILL(&m1);

    DL_DOMAIN_KILL(&dom);

    printf("\nP1363 DLSSR-PV Signature with message recovery\n");


    bytes=DL_DOMAIN_INIT(&dom,"common.dss",NULL,precompute);
    res=DL_DOMAIN_VALIDATE(NULL,&dom);
    if (res!=0)
    {
        printf("Domain parameters are invalid\n");
        return 0;
    }

    OCTET_INIT(&s,bytes);
    OCTET_INIT(&w,bytes);
    OCTET_INIT(&u,bytes);
    OCTET_INIT(&v,bytes);
    OCTET_INIT(&m,bytes);
    OCTET_INIT(&c,bytes);
    OCTET_INIT(&h,bytes);
    OCTET_INIT(&d,bytes);

    DL_KEY_PAIR_GENERATE(NULL,&dom,&RNG,&s,&w);
    res=DLPSP_NR2PV(NULL,&dom,&RNG,&u,&v);

    m.len=20;
    for (i=0;i<20;i++) m.val[i]=i+1;
    printf("recoverable message= ");
    OCTET_OUTPUT(&m);

    EMSR2_ENCODE(100,TRUE,SHA1,NULL,&m,&v,&c);
    HASH(SHA1,&c,&h);
    res=DLSP_PV(NULL,&dom,&s,&u,&h,&d);
    OCTET_CLEAR(&m);
    OCTET_CLEAR(&h);
    OCTET_CLEAR(&v);
    
    printf("Signature= \n");
    OCTET_OUTPUT(&c);
    OCTET_OUTPUT(&d);

    HASH(SHA1,&c,&h);
    res=DLVP_PV(NULL,&dom,&w,&h,&d,&v);
    result=EMSR2_DECODE(100,TRUE,SHA1,NULL,&c,&v,&m);
    printf("recovered message=   ");
    OCTET_OUTPUT(&m);
    if (result)
        printf("DLSSR-PV Signature with message recovery - OK\n");
    else
    {
        printf("*** DLSSR-PV Signature with message recovery Failed\n");
        return 0;
    } 

    OCTET_KILL(&s);
    OCTET_KILL(&w);
    OCTET_KILL(&u);
    OCTET_KILL(&v);
    OCTET_KILL(&m);
    OCTET_KILL(&c);
    OCTET_KILL(&h);
    OCTET_KILL(&d);
    DL_DOMAIN_KILL(&dom);

    printf("\nP1363 ECIES Encryption/Decryption - DHAES mode\n");

    dhaes=TRUE;   /* Use DHAES mode */


    bytes=ECP_DOMAIN_INIT(&epdom,"common.ecs",NULL,precompute);

    OCTET_INIT(&m,20); OCTET_INIT(&c,32); /* round up to block size */
    OCTET_INIT(&k,32); OCTET_INIT(&s,bytes);
    OCTET_INIT(&u,bytes); OCTET_INIT(&v,2*bytes+1);
    OCTET_INIT(&w,2*bytes+1);
    OCTET_INIT(&m1,20);
    OCTET_INIT(&k1,16); OCTET_INIT(&k2,16);
    OCTET_INIT(&tag,12); OCTET_INIT(&tag1,12);
    OCTET_INIT(&z,bytes);  OCTET_INIT(&vz,3*bytes+2);
    OCTET_INIT(&p1,30); OCTET_INIT(&p2,30);
    OCTET_INIT(&L2,8); OCTET_INIT(&C,300);

    OCTET_JOIN_STRING("Key Derivation Parameters",&p1);
    OCTET_JOIN_STRING("Encoding Parameters",&p2);

    ECP_KEY_PAIR_GENERATE(NULL,&epdom,&RNG,&u,compress,&v);  /* one time key pair */
    ECP_KEY_PAIR_GENERATE(NULL,&epdom,&RNG,&s,compress,&w);  /* recipients key pair */

    res=ECPSVDP_DH(NULL,&epdom,&u,&w,&z);     
    
    if (dhaes)
    {
        OCTET_COPY(&v,&vz);
        OCTET_JOIN_OCTET(&z,&vz);
    }
    else OCTET_COPY(&z,&vz);

    res=KDF2(&vz,&p1,32,SHA1,&k);
    printf("Key is \n");
    OCTET_OUTPUT(&k);

    k1.len=k2.len=16;
    for (i=0;i<16;i++) {k1.val[i]=k.val[i]; k2.val[i]=k.val[16+i];} 

    printf("Encryption\n");

    m.len=20;
    for (i=0;i<20;i++) m.val[i]=i+1;    /* fake a message */
    printf("Message is \n");
    OCTET_OUTPUT(&m);

    res=AES_CBC_IV0_ENCRYPT(&k1,&m,NULL,&c,NULL);

    printf("Ciphertext is \n");
    OCTET_OUTPUT(&c);

    if (dhaes) OCTET_JOIN_LONG((long)p2.len,8,&L2);

    OCTET_COPY(&c,&C);
    OCTET_JOIN_OCTET(&p2,&C);
    OCTET_JOIN_OCTET(&L2,&C);

    res=MAC1(&C,NULL,&k2,12,SHA256,&tag);

    printf("HMAC tag is \n");
    OCTET_OUTPUT(&tag);

/* Note that "two passes" are required, one to encrypt, one
   to calculate the MAC.  */

/* Overall ciphertext is (v,c,tag) */

    OCTET_CLEAR(&z); OCTET_CLEAR(&k); 
    OCTET_CLEAR(&k1); OCTET_CLEAR(&k2); 
    OCTET_CLEAR(&vz); OCTET_CLEAR(&C);
    OCTET_CLEAR(&L2);

    printf("Decryption\n");

    res=ECPSVDP_DH(NULL,&epdom,&s,&v,&z);  

    if (dhaes)
    {
        OCTET_COPY(&v,&vz);
        OCTET_JOIN_OCTET(&z,&vz);
    }

    else OCTET_COPY(&z,&vz);

    res=KDF2(&vz,&p1,32,SHA1,&k);
    k1.len=k2.len=16;
    for (i=0;i<16;i++) {k1.val[i]=k.val[i]; k2.val[i]=k.val[16+i];} 

    res=AES_CBC_IV0_DECRYPT(&k1,&c,NULL,&m1,NULL);

    printf("Message is \n");
    OCTET_OUTPUT(&m1);

    if (dhaes) OCTET_JOIN_LONG((long)p2.len,8,&L2);

    OCTET_COPY(&c,&C);
    OCTET_JOIN_OCTET(&p2,&C);
    OCTET_JOIN_OCTET(&L2,&C);

    res=MAC1(&C,NULL,&k2,12,SHA256,&tag1);

    printf("HMAC tag is \n");
    OCTET_OUTPUT(&tag1);

    if (OCTET_COMPARE(&m,&m1) && OCTET_COMPARE(&tag,&tag1))
        printf("ECIES Encryption/Decryption - OK\n");
    else
    {
        printf("ECIES Encryption/Decryption Failed\n");
        return 0;
    } 
       
    OCTET_KILL(&tag); OCTET_KILL(&tag1);
    OCTET_KILL(&k1); OCTET_KILL(&k2);
    OCTET_KILL(&m);  OCTET_KILL(&c);
    OCTET_KILL(&k); OCTET_KILL(&s);
    OCTET_KILL(&u); OCTET_KILL(&v); 
    OCTET_KILL(&m1); OCTET_KILL(&w);  
    OCTET_KILL(&p1); OCTET_KILL(&p2);
    OCTET_KILL(&L2); OCTET_KILL(&C);

    ECP_DOMAIN_KILL(&epdom);
    OCTET_KILL(&z); OCTET_KILL(&vz);

    KILL_CSPRNG(&RNG);
    return 0;
}

