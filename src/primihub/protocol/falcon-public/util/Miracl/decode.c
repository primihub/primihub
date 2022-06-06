/*
 *   Program to decode message using RSA private key.
 *
 *   **** For Demonstration use only ******
 *
 */

#include <stdio.h>
#include "miracl.h"
#include <stdlib.h>
#include <string.h>

#define NP 2         /* two primes - could be used with more */

miracl *mip;

void strip(char *name)
{ /* strip extension off filename */
    int i;
    for (i=0;name[i]!='\0';i++)
    {
        if (name[i]!='.') continue;
        name[i]='\0';
        break;
    }
}

int main()
{  /*  decode using private key  */
    int i;
    big e,ep[NP],m,ke,kd,p[NP],kp[NP],mn,mx;
    FILE *ifile;
    FILE *ofile;
    char ifname[13],ofname[13];
    BOOL flo;
    big_chinese ch;
    mip=mirsys(100,0);
    for (i=0;i<NP;i++)
    {
        p[i]=mirvar(0);
        ep[i]=mirvar(0);
        kp[i]=mirvar(0);
    }
    e=mirvar(0);
    m=mirvar(0);
    kd=mirvar(0);
    ke=mirvar(0);
    mn=mirvar(0);
    mx=mirvar(0);
    mip->IOBASE=16;
    if ((ifile=fopen("private.key","rt"))==NULL)
    {
        printf("Unable to open file private.key\n");
        return 0;
    }
    for (i=0;i<NP;i++)
    {
        cinnum(p[i],ifile);
    }
    fclose(ifile);
 /* generate public and private keys */
    convert(1,ke);
    for (i=0;i<NP;i++)
    {
        multiply(ke,p[i],ke);
    }
    for (i=0;i<NP;i++)
    { /* kp[i]=(2*(p[i]-1)+1)/3 = 1/3 mod p[i]-1 */
        decr(p[i],1,kd);
        premult(kd,2,kd);
        incr(kd,1,kd);
        subdiv(kd,3,kp[i]);
    }

    crt_init(&ch,NP,p);
    nroot(ke,3,mn);
    multiply(mn,mn,m);
    multiply(mn,m,mx);
    subtract(mx,m,mx);
    do
    { /* get input file */
        printf("file to be decoded = ");
        gets(ifname);
    } while (strlen(ifname)==0);
    strip(ifname);
    strcat(ifname,".rsa");
    printf("output filename = ");
    gets(ofname);
    flo=FALSE;
    if (strlen(ofname)>0) 
    { /* set up output file */
        flo=TRUE;
        ofile=fopen(ofname,"wt");
    }
    printf("decoding message\n");
    if ((ifile=fopen(ifname,"rt"))==NULL)
    {
        printf("Unable to open file %s\n",ifname);
        return 0;
    }
    forever
    { /* decode line by line */
        mip->IOBASE=16;
        cinnum(m,ifile);
        if (size(m)==0) break;
        for (i=0;i<NP;i++)
            powmod(m,kp[i],p[i],ep[i]);
        crt(&ch,ep,e);    /* Chinese remainder thereom */
        if (compare(e,mx)>=0) divide(e,mn,mn);
        mip->IOBASE=128;
        if (flo) cotnum(e,ofile);
        cotnum(e,stdout);
    }
    crt_end(&ch);
    fclose(ifile);
    if (flo) fclose(ofile);
    printf("message ends\n");
    return 0;
}

