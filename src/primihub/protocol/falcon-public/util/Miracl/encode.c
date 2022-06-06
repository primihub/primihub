/*
 *   Program to encode text using RSA public key.
 *
 *   *** For Demonstration use only *****
 *
 */

#include <stdio.h>
#include "miracl.h"
#include <stdlib.h>
#include <string.h>

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

miracl *mip;

int main()
{  /*  encode using public key  */
    big e,m,y,ke,mn,mx;
    FILE *ifile;
    FILE *ofile;
    static char line[500];
    static char buff[256];
    char ifname[13],ofname[13];
    BOOL fli,last;
    int i,ipt,klen;
    mip=mirsys(100,0);
    e=mirvar(0);
    m=mirvar(0);
    y=mirvar(0);
    ke=mirvar(0);
    mn=mirvar(0);
    mx=mirvar(0);
    if ((ifile=fopen("public.key","rt"))==NULL)
    {
        printf("Unable to open file public.key\n");
        return 0;
    }
    mip->IOBASE=16;
    cinnum(ke,ifile);
    fclose(ifile);
    nroot(ke,3,mn);
    multiply(mn,mn,m);
    multiply(mn,m,mx);
    subtract(mx,m,mx);
    klen=0;
    copy(mx,m);
    while (size(m)>0)
    { /* find key length in characters */
        klen++;
        subdiv(m,128,m);
    }
    klen--;
    printf("file to be encoded = ");
    gets(ifname);
    fli=FALSE;
    if (strlen(ifname)>0) fli=TRUE;
    if (fli)
    { /* set up input file */
        strcpy(ofname,ifname);
        strip(ofname);
        strcat(ofname,".rsa");
        if ((ifile=fopen(ifname,"rt"))==NULL)
        {
            printf("Unable to open file %s\n",ifname);
            return 0;
        }
        printf("encoding message\n");
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
        strcat(ofname,".rsa");
        printf("input message - finish with cntrl z\n");
    }
    ofile=fopen(ofname,"wt");
    ipt=0;
    last=FALSE;
    while (!last)
    { /* encode line by line */
        if (fgets(&line[ipt],132,ifile)==NULL) last=TRUE;
        if (line[ipt]==EOF) last=TRUE;
        ipt=strlen(line);
        if (ipt<klen && !last) continue;
        while (ipt>=klen)
        { /* chop up into klen-sized chunks and encode */
            for (i=0;i<klen;i++)
                buff[i]=line[i];
            buff[klen]='\0';
            for (i=klen;i<=ipt;i++)
                line[i-klen]=line[i];
            ipt-=klen;
            mip->IOBASE=128;
            cinstr(m,buff);
            power(m,3,ke,e);
            mip->IOBASE=16;
            cotnum(e,ofile);
        }
        if (last && ipt>0)
        { /* now deal with left overs */
            mip->IOBASE=128;
            cinstr(m,line);
            if (compare(m,mn)<0)
            { /* pad out with random number if necessary */
                bigrand(mn,y);
                multiply(mn,mn,e);
                subtract(e,y,e);
                multiply(mn,e,y);
                add(m,y,m);
            }
            power(m,3,ke,e);
            mip->IOBASE=16;
            cotnum(e,ofile);
        }
    }
    fclose(ofile);
    if (fli) fclose(ifile);
    return 0;
}   

