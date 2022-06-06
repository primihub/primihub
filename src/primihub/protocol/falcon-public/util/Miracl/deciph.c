/*
 *   Program to decipher message using Blum-Goldwasser probabalistic
 *   Public key method
 *
 *   Define RSA to use cubing instead of squaring. This is no longer
 *   provably as difficult to break as factoring the modulus, its a bit 
 *   slower, but it is resistant to chosen ciphertext attack.
 */

#include <stdio.h>
#include "miracl.h"
#include <stdlib.h>
#include <string.h>

/*
#define RSA
*/

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
{  /*  decipher using private key  */
    big x,y,ke,p,q,n,a,b,alpha,beta,t;
    FILE *ifile;
    FILE *ofile;
    int ch;
    long i,ipt;
    char ifname[13],ofname[13];
    BOOL flo;
    miracl *mip=mirsys(100,0);
    x=mirvar(0);
    ke=mirvar(0);
    p=mirvar(0);
    q=mirvar(0);
    n=mirvar(0);
    y=mirvar(0);
    alpha=mirvar(0);
    beta=mirvar(0);
    a=mirvar(0);
    b=mirvar(0);
    t=mirvar(0);
    mip->IOBASE=16;
    if ((ifile=fopen("private.key","rt"))==NULL)
    {
        printf("Unable to open file private.key\n");
        return 0;
    }
    cinnum(p,ifile);
    cinnum(q,ifile);
    fclose(ifile);
    multiply(p,q,ke);
    do
    { /* get input file */
        printf("file to be deciphered = ");
        gets(ifname);
    } while (strlen(ifname)==0);
    strip(ifname);
    strcat(ifname,".key");
    if ((ifile=fopen(ifname,"rt"))==NULL)
    {
        printf("Unable to open file %s\n",ifname);
        return 0;
    }
    fscanf(ifile,"%ld\n",&ipt);
    cinnum(x,ifile);
    fclose(ifile);
    strip(ifname);
    strcat(ifname,".blg");
    printf("output filename = ");
    gets(ofname);
    flo=FALSE;
    if (strlen(ofname)>0) 
    { /* set up output file */
        flo=TRUE;
        ofile=fopen(ofname,"wt");
    }
    else ofile=stdout;
    printf("deciphering message\n");

    xgcd(p,q,a,b,t);
    lgconv(ipt,n);            /* first recover "one-time pad" */

#ifdef RSA
    decr(p,1,alpha);
    premult(alpha,2,alpha);
    incr(alpha,1,alpha);
    subdiv(alpha,3,alpha);
#else
    incr(p,1,alpha);
    subdiv(alpha,4,alpha);
#endif
    decr(p,1,y);
    powmod(alpha,n,y,alpha);


#ifdef RSA
    decr(q,1,beta);
    premult(beta,2,beta);
    incr(beta,1,beta);
    subdiv(beta,3,beta);
#else
    incr(q,1,beta);
    subdiv(beta,4,beta);
#endif
    decr(q,1,y);
    powmod(beta,n,y,beta);

    copy(x,y);
    divide(x,p,p);
    divide(y,q,q);
    powmod(x,alpha,p,x);    /* using chinese remainder thereom */
    powmod(y,beta,q,y);
    mad(x,q,q,ke,ke,t);
    mad(t,b,b,ke,ke,t);
    mad(y,p,p,ke,ke,x);
    mad(x,a,a,ke,ke,x);
    add(x,t,x);
    divide(x,ke,ke);
    if (size(x)<0) add(x,ke,x);

    if ((ifile=fopen(ifname,"rb"))==NULL)
    {
        printf("Unable to open file %s\n",ifname);
        return 0;
    }  
    for (i=0;i<ipt;i++)
    { /* decipher character by character */
        ch=fgetc(ifile);
        ch^=x->w[0];               /* XOR with last byte of x */
        fputc((char)ch,ofile);
#ifdef RSA
        power(x,3,ke,x);
#else
        mad(x,x,x,ke,ke,x);
#endif
    }
    fclose(ifile);
    if (flo) fclose(ofile);
    printf("\nmessage ends\n");
    return 0;
}

