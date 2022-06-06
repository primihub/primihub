/*
 *   Program to encipher text using Blum-Goldwasser Probabalistic
 *   Public Key method
 *   See "Modern Cryptology - a tutorial" by Gilles Brassard. 
 *   Published by Springer-Verlag, 1988
 *
 *   Define RSA to use cubing instead of squaring. This is no longer
 *   provably as difficult to break as factoring the modulus, its a bit 
 *   slower, but it is resistant to chosen ciphertext attack.
 *
 *
 *   Note: This implementation uses only the Least Significant Byte
 *   of the big random number x in encipherment/decipherment, as it has 
 *   proven to be completely secure. However it is conjectured that
 *   that up to half the bytes in x (the lower half) are also secure.
 *   They could be used to considerably speed up this method.
 *   
 */

#include <stdio.h>
#include "miracl.h"
#include <stdlib.h>
#include <string.h>

/*
#define RSA
*/

void strip(char *name)
{ /* strip off filename extension */
    int i;
    for (i=0;name[i]!='\0';i++)
    {
        if (name[i]!='.') continue;
        name[i]='\0';
        break;
    }
}

int main()
{  /*  encipher using public key  */
    big x,ke;
    FILE *ifile;
    FILE *ofile;
    char ifname[13],ofname[13];
    BOOL fli;
    int ch;
    long seed,ipt;
    miracl *mip=mirsys(100,0);
    x=mirvar(0);
    ke=mirvar(0);
    mip->IOBASE=16;
    if ((ifile=fopen("public.key","rt"))==NULL)
    {
        printf("Unable to open file public.key\n");
        return 0;
    }
    cinnum(ke,ifile);
    fclose(ifile);
    printf("Enter 9 digit random number seed  = ");
    scanf("%ld",&seed);
    getchar();
    irand(seed);
    bigrand(ke,x);
    printf("file to be enciphered = ");
    gets(ifname);
    fli=FALSE;
    if (strlen(ifname)>0) fli=TRUE;
    if (fli)
    { /* set up input file */
        strcpy(ofname,ifname);
        strip(ofname);
        strcat(ofname,".blg");
        if ((ifile=fopen(ifname,"rt"))==NULL)
        {
            printf("Unable to open file %s\n",ifname);
            return 0;
        }
        printf("enciphering message\n");
    }
    else
    { /* accept input from keyboard */
        ifile=stdin;
        do
        {
            printf("output filename = ");
            gets(ofname); 
        } while (strlen(ofname)==0);
        strip(ofname);    
        strcat(ofname,".blg");
        printf("input message - finish with cntrl z\n");
    }
    ofile=fopen(ofname,"wb");
    ipt=0;
    forever
    { /* encipher character by character */
#ifdef RSA
        power(x,3,ke,x);
#else
        mad(x,x,x,ke,ke,x);
#endif
        ch=fgetc(ifile);
        if (ch==EOF) break;
        ch^=x->w[0];              /* XOR with last byte of x */
        fputc(ch,ofile);
        ipt++;
    }
    fclose(ofile);
    if (fli) fclose(ifile);
    strip(ofname);
    strcat(ofname,".key");
    ofile=fopen(ofname,"wt");
    fprintf(ofile,"%ld\n",ipt);
    cotnum(x,ofile);
    fclose(ofile);
    return 0;
}   

