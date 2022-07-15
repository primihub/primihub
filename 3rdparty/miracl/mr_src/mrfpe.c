/***************************************************************************
                                                                           *
Copyright 2013 CertiVox UK Ltd.                                           *
                                                                           *
This file is part of CertiVox MIRACL Crypto SDK.                           *
                                                                           *
The CertiVox MIRACL Crypto SDK provides developers with an                 *
extensive and efficient set of cryptographic functions.                    *
For further information about its features and functionalities please      *
refer to http://www.certivox.com                                           *
                                                                           *
* The CertiVox MIRACL Crypto SDK is free software: you can                 *
  redistribute it and/or modify it under the terms of the                  *
  GNU Affero General Public License as published by the                    *
  Free Software Foundation, either version 3 of the License,               *
  or (at your option) any later version.                                   *
                                                                           *
* The CertiVox MIRACL Crypto SDK is distributed in the hope                *
  that it will be useful, but WITHOUT ANY WARRANTY; without even the       *
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. *
  See the GNU Affero General Public License for more details.              *
                                                                           *
* You should have received a copy of the GNU Affero General Public         *
  License along with CertiVox MIRACL Crypto SDK.                           *
  If not, see <http://www.gnu.org/licenses/>.                              *
                                                                           *
You can be released from the requirements of the license by purchasing     *
a commercial license. Buying such a license is mandatory as soon as you    *
develop commercial activities involving the CertiVox MIRACL Crypto SDK     *
without disclosing the source code of your own applications, or shipping   *
the CertiVox MIRACL Crypto SDK with a closed source product.               *
                                                                           *
***************************************************************************/
/*
 * Implementation of BPS Format Preserving Encryption
 *
 * See "BPS: a Format Preserving Encryption Proposal" by E. Brier, T. Peyrin and J. Stern
 *
 * http://csrc.nist.gov/groups/ST/toolkit/BCM/documents/proposedmodes/bps/bps-spec.pdf
 *
 * Uses AES internally
 *
 * Author: M. Scott 2012/2015
 */

#include <stdlib.h> 
#include "../mr_include/miracl.h"

/* Define FPE_TEST to activate test main program and run test vectors */
/* Link with mraes.c */
/* gcc -O2 mrfpe.c mraes.c -o mrfpe.exe */
/* #define FPE_TEST */

#define UINT32 mr_unsign32 /* 32-bit unsigned type */
#define W 8 /* recommended number of rounds */
#define BLOCK_SIZE 16 /* 16 Byte Blocks - AES */
#define ENCRYPT 0
#define DECRYPT 1

static void unpack(UINT32 a,MR_BYTE *b)
{ /* unpack bytes from a word */
    b[0]=MR_TOBYTE(a);
    b[1]=MR_TOBYTE(a>>8);
    b[2]=MR_TOBYTE(a>>16);
    b[3]=MR_TOBYTE(a>>24);
}

/* Little Endian */

static int to_base_256(char *x,int len,int s,MR_BYTE *y)
{ /* x[] of length len to base s is converted to byte array y[] of length BLOCK_SIZE */
	int i,j,m;
	UINT32 c;

	for (i=0;i<BLOCK_SIZE;i++) y[i]=0;
	if (len==0) return 0;	

	m=1; y[0]=x[len-1];
	for (j=len-2;j>=0;j--)
	{ /* multiply by s */
		c=x[j];
		for (i=0;i<m;i++)
		{
			c+=(UINT32)y[i]*s;
			y[i]=c&0xff;
			c>>=8;
		}
		if (c>0) {m++; y[m-1]=c;}
	}
	
	return m;
}

/* Find max_b for chosen cipher and number base */

static int maxb(int s)
{
	MR_BYTE y[BLOCK_SIZE];
	int i,m,n,c;
	if (s==2) return 192;
	m=1; y[0]=1;
	for (n=0;;n++)
	{
		c=0;
		for (i=0;i<m;i++)
		{ /* multiply y by s */
			c+=(UINT32)y[i]*s;
			y[i]=c&0xff;
			c>>=8;
		}
		if (c>0) {m++; y[m-1]=c;}
		if (m==13) break;	/* greater than 2^96 for AES */
	}
	return 2*n;
}

static void from_base_256(int addsub,MR_BYTE *y,int len,int s,char *x)
{ /* y[] of length BLOCK_SIZE is added to or subtracted from base s array x[] of length len. */
	int i,m,n;
	UINT32 c,d;

	m=BLOCK_SIZE;
	n=0; c=0;
	forever
	{
		while (m>0 && y[m-1]==0) m--;
		d=0;
		for (i=m-1;i>=0;i--)
		{ /* divide y by s */
			d=(d<<8)+y[i];
			y[i]=d/s;
			d%=s;
		}
		if (addsub==ENCRYPT)
		{ /* ADD */
			d+=c+x[n]; c=0;
			if ((int)d>=s) 
				{c=1; x[n]=d-s;}
			else x[n]=d;
		}
		else
		{ /* SUB */
			d+=c; c=0;
			if ((UINT32)x[n]>=d) x[n]-=d;
			else
				{x[n]+=(s-d); c=1;}
		}
		n++;
		if (n>=len) break;
	}
}

/* AES instance must be initialised and passed */
/* Format Preserving Encryption/Decryption routine */
/* Array x of length len to base s is encrypted/decrypted in place */

static void BC(int crypt,char *x,int len,int s,aes *a,UINT32 TL,UINT32 TR)
{
	int i,j;
	char *left,*right;
	MR_BYTE buff[BLOCK_SIZE];
	int l,r;
	l=r=len/2;
	if (len%2==1) l++;

	left=&x[0]; right=&x[l];

	for (i=0;i<W;i++)
	{
		if (crypt==ENCRYPT) j=i;
		else j=W-i-1;
		if (j%2==0)
		{
			to_base_256(right,r,s,buff);
			unpack(TR^j,&buff[12]);
			aes_ecb_encrypt(a,buff);
			from_base_256(crypt,buff,l,s,left);
		}
		else
		{
			to_base_256(left,l,s,buff);
			unpack(TL^j,&buff[12]);
			aes_ecb_encrypt(a,buff);
			from_base_256(crypt,buff,r,s,right);
		}
	}
}

/* Algorithm 3 */

/* x is an array of length len of numbers to the base s */
/* a is an initialised AES instance  */
/* TL and TR are 32-bit tweak values */
/* x is replaced in place by encrypted values. The format of x[] is preserved */

void FPE_encrypt(int s,aes *a,UINT32 TL,UINT32 TR,char *x,int len)
{
	int i,j,c,rest,mb=maxb(s);
	if (len<=mb)
	{
		BC(ENCRYPT,x,len,s,a,TL,TR);
		return;
	}
	rest=len%mb;
	c=0; i=0;
	while (len-c>=mb)
	{
		if (i!=0) for (j=c;j<c+mb;j++) x[j]=(x[j]+x[j-mb])%s;
		BC(ENCRYPT,&x[c],mb,s,a,TL^(i<<16),TR^(i<<16));
		c+=mb; i++;
	}
	if (len!=c)
	{
		for (j=len-rest;j<len;j++)
			x[j]=(x[j]+x[j-mb])%s;
		BC(ENCRYPT,&x[len-mb],mb,s,a,TL^(i<<16),TR^(i<<16));
	}
}

/* Algorithm 4 */

void FPE_decrypt(int s,aes *a,UINT32 TL,UINT32 TR,char *x,int len)
{
	int i,j,c,rest,mb=maxb(s);
	int b;
	if (len<=mb)
	{
		BC(DECRYPT,x,len,s,a,TL,TR);
		return;
	}
	rest=len%mb;
	c=len-rest; i=c/mb;
	if (len!=c)
	{
		BC(DECRYPT,&x[len-mb],mb,s,a,TL^(i<<16),TR^(i<<16));
		for (j=len-rest;j<len;j++)
			{b=(x[j]-x[j-mb])%s; if (b<0) x[j]=b+s; else x[j]=b;}
	}
	while (c!=0)
	{
		c-=mb; i--;
		BC(DECRYPT,&x[c],mb,s,a,TL^(i<<16),TR^(i<<16));
		if (i!=0) for (j=c;j<c+mb;j++) {b=(x[j]-x[j-mb])%s; if (b<0) x[j]=b+s; else x[j]=b;}
	}
}

/* Test Program - runs NIST test vectors from http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/FF3samples.pdf */

#ifdef FPE_TEST

int main()
{
	int i,j,n,radix;
	aes a;
	char key[32];
	char x[256];    /* any length... */
	UINT32 TL,TR;

//# sample #1

	radix=10;
	TL=0xD8E7920A;
	TR=0xFA330A73; /* random tweaks */

/* Set AES key */
//EF 43 59 D8 D5 80 AA 4F 7F 03 6D 6F 04 FC 6A 94 

	key[15]=0xEF; key[14]=0x43; key[13]=0x59; key[12]=0xD8; key[11]=0xD5; key[10]=0x80; key[9]=0xAA; key[8]=0x4F;
	key[7]=0x7F; key[6]=0x03; key[5]=0x6D; key[4]=0x6F; key[3]=0x04; key[2]=0xFC; key[1]=0x6A; key[0]=0x94;

	printf("\n128 bit aes key= ");
	for (i=0;i<16;i++)
		printf("%02x",(unsigned char) key[i]);
	printf("\n");

    aes_init(&a,MR_ECB,16,key,NULL);

	n=18;
	x[0]=8; x[1]=9; x[2]=0; x[3]=1; x[4]=2; x[5]=1; x[6]=2; x[7]=3;
	x[8]=4; x[9]=5; x[10]=6; x[11]=7; x[12]=8; x[13]=9; x[14]=0; x[15]=0; x[16]=0; x[17]=0;

/* Encrypt/decrypt short string */

	printf("\nSample #1\n");
	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_encrypt(radix,&a,TL,TR,x,n);

	printf("Ciphertext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_decrypt(radix,&a,TL,TR,x,n);

	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");

//sample #2

	TL=0x9A768A92;
	TR=0xF60E12D8;

/* Encrypt/decrypt short string */

	printf("\nSample #2\n");
	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_encrypt(radix,&a,TL,TR,x,n);

	printf("Ciphertext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_decrypt(radix,&a,TL,TR,x,n);

	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");

//sample #3

	n=29;
	x[0]=8; x[1]=9; x[2]=0; x[3]=1; x[4]=2; x[5]=1; x[6]=2; x[7]=3;
	x[8]=4; x[9]=5; x[10]=6; x[11]=7; x[12]=8; x[13]=9; x[14]=0; x[15]=0; x[16]=0; x[17]=0;
	x[18]=0; x[19]=0; x[20]=7; x[21]=8; x[22]=9; x[23]=0; x[24]=0; x[25]=0; x[26]=0; x[27]=0; x[28]=0;

	TL=0xD8E7920A;
	TR=0xFA330A73; /* random tweaks */

/* Encrypt/decrypt short string */

	printf("\nSample #3\n");
	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_encrypt(radix,&a,TL,TR,x,n);

	printf("Ciphertext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_decrypt(radix,&a,TL,TR,x,n);

	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");

// sample #4
	TL=0x0;
	TR=0x0;

	printf("\nSample #4\n");
	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_encrypt(radix,&a,TL,TR,x,n);

	printf("Ciphertext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_decrypt(radix,&a,TL,TR,x,n);

	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n"); 

// sample #5

	radix=26;
	TL=0x9A768A92;
	TR=0xF60E12D8;

	n=19;
	x[0]=0; x[1]=1; x[2]=2; x[3]=3; x[4]=4; x[5]=5; x[6]=6; x[7]=7;
	x[8]=8; x[9]=9; x[10]=10; x[11]=11; x[12]=12; x[13]=13; x[14]=14; x[15]=15; x[16]=16; x[17]=17;
	x[18]=18; 

	printf("\nSample #5\n");
	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_encrypt(radix,&a,TL,TR,x,n);

	printf("Ciphertext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_decrypt(radix,&a,TL,TR,x,n);

	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n"); 

// sample #6

	key[23]=0xEF; key[22]=0x43; key[21]=0x59; key[20]=0xD8; key[19]=0xD5; key[18]=0x80; key[17]=0xAA; key[16]=0x4F;
	key[15]=0x7F; key[14]=0x03; key[13]=0x6D; key[12]=0x6F; key[11]=0x04; key[10]=0xFC; key[9]=0x6A;  key[8]=0x94;
	key[7]=0x2b;  key[6]=0x7E;  key[5]=0x15;  key[4]=0x16;  key[3]=0x28;  key[2]=0xAE;  key[1]=0xD2;  key[0]=0xA6;

	printf("\n192-bit aes key= ");
	for (i=0;i<24;i++)
		printf("%02x",(unsigned char) key[i]);
	printf("\n");

    aes_init(&a,MR_ECB,24,key,NULL);

	n=18;
	x[0]=8; x[1]=9; x[2]=0; x[3]=1; x[4]=2; x[5]=1; x[6]=2; x[7]=3;
	x[8]=4; x[9]=5; x[10]=6; x[11]=7; x[12]=8; x[13]=9; x[14]=0; x[15]=0; x[16]=0; x[17]=0;

	radix=10;
	TL=0xD8E7920A;
	TR=0xFA330A73; /* random tweaks */

	printf("\nSample #6\n");
	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_encrypt(radix,&a,TL,TR,x,n);

	printf("Ciphertext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_decrypt(radix,&a,TL,TR,x,n);

	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n"); 

// sample #7

	TL=0x9A768A92;
	TR=0xF60E12D8;

	printf("\nSample #7\n");
	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_encrypt(radix,&a,TL,TR,x,n);

	printf("Ciphertext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_decrypt(radix,&a,TL,TR,x,n);

	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n"); 

// sample #8

	n=29;
	x[0]=8; x[1]=9; x[2]=0; x[3]=1; x[4]=2; x[5]=1; x[6]=2; x[7]=3;
	x[8]=4; x[9]=5; x[10]=6; x[11]=7; x[12]=8; x[13]=9; x[14]=0; x[15]=0; x[16]=0; x[17]=0;
	x[18]=0; x[19]=0; x[20]=7; x[21]=8; x[22]=9; x[23]=0; x[24]=0; x[25]=0; x[26]=0; x[27]=0; x[28]=0;

	TL=0xD8E7920A;
	TR=0xFA330A73; /* random tweaks */

	printf("\nSample #8\n");
	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_encrypt(radix,&a,TL,TR,x,n);

	printf("Ciphertext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_decrypt(radix,&a,TL,TR,x,n);

	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n"); 

// sample #9

	TL=0x0;
	TR=0x0;

	printf("\nSample #9\n");
	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_encrypt(radix,&a,TL,TR,x,n);

	printf("Ciphertext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_decrypt(radix,&a,TL,TR,x,n);

	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n"); 

// sample #10

	radix=26;
	TL=0x9A768A92;
	TR=0xF60E12D8;

	n=19;
	x[0]=0; x[1]=1; x[2]=2; x[3]=3; x[4]=4; x[5]=5; x[6]=6; x[7]=7;
	x[8]=8; x[9]=9; x[10]=10; x[11]=11; x[12]=12; x[13]=13; x[14]=14; x[15]=15; x[16]=16; x[17]=17;
	x[18]=18; 

	printf("\nSample #10\n");
	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_encrypt(radix,&a,TL,TR,x,n);

	printf("Ciphertext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_decrypt(radix,&a,TL,TR,x,n);

	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n"); 

// sample #11

	key[31]=0xEF; key[30]=0x43; key[29]=0x59; key[28]=0xD8; key[27]=0xD5; key[26]=0x80; key[25]=0xAA; key[24]=0x4F;
	key[23]=0x7F; key[22]=0x03; key[21]=0x6D; key[20]=0x6F; key[19]=0x04; key[18]=0xFC; key[17]=0x6A; key[16]=0x94;
	key[15]=0x2b; key[14]=0x7E; key[13]=0x15; key[12]=0x16; key[11]=0x28; key[10]=0xAE; key[9]=0xD2;  key[8]=0xA6;
	key[7]=0xAB;  key[6]=0xF7;  key[5]=0x15;  key[4]=0x88;  key[3]=0x09;  key[2]=0xCF;  key[1]=0x4F;  key[0]=0x3C;

	printf("\n256-bit aes key= ");
	for (i=0;i<32;i++)
		printf("%02x",(unsigned char) key[i]);
	printf("\n");

    aes_init(&a,MR_ECB,32,key,NULL);

	n=18;
	x[0]=8; x[1]=9; x[2]=0; x[3]=1; x[4]=2; x[5]=1; x[6]=2; x[7]=3;
	x[8]=4; x[9]=5; x[10]=6; x[11]=7; x[12]=8; x[13]=9; x[14]=0; x[15]=0; x[16]=0; x[17]=0;

	radix=10;
	TL=0xD8E7920A;
	TR=0xFA330A73; /* random tweaks */

	printf("\nSample #11\n");
	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_encrypt(radix,&a,TL,TR,x,n);

	printf("Ciphertext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_decrypt(radix,&a,TL,TR,x,n);

	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n"); 

// sample #12

	TL=0x9A768A92;
	TR=0xF60E12D8;

	printf("\nSample #12\n");
	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_encrypt(radix,&a,TL,TR,x,n);

	printf("Ciphertext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_decrypt(radix,&a,TL,TR,x,n);

	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n"); 

// sample #13

	n=29;
	x[0]=8; x[1]=9; x[2]=0; x[3]=1; x[4]=2; x[5]=1; x[6]=2; x[7]=3;
	x[8]=4; x[9]=5; x[10]=6; x[11]=7; x[12]=8; x[13]=9; x[14]=0; x[15]=0; x[16]=0; x[17]=0;
	x[18]=0; x[19]=0; x[20]=7; x[21]=8; x[22]=9; x[23]=0; x[24]=0; x[25]=0; x[26]=0; x[27]=0; x[28]=0;

	TL=0xD8E7920A;
	TR=0xFA330A73; /* random tweaks */

	printf("\nSample #13\n");
	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_encrypt(radix,&a,TL,TR,x,n);

	printf("Ciphertext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_decrypt(radix,&a,TL,TR,x,n);

	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n"); 

// sample #14

	TL=0x0;
	TR=0x0; /* random tweaks */

	printf("\nSample #14\n");
	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_encrypt(radix,&a,TL,TR,x,n);

	printf("Ciphertext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_decrypt(radix,&a,TL,TR,x,n);

	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n"); 

// sample #15

	radix=26;
	TL=0x9A768A92;
	TR=0xF60E12D8;

	n=19;
	x[0]=0; x[1]=1; x[2]=2; x[3]=3; x[4]=4; x[5]=5; x[6]=6; x[7]=7;
	x[8]=8; x[9]=9; x[10]=10; x[11]=11; x[12]=12; x[13]=13; x[14]=14; x[15]=15; x[16]=16; x[17]=17;
	x[18]=18; 

	printf("\nSample #15\n");
	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_encrypt(radix,&a,TL,TR,x,n);

	printf("Ciphertext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");
	FPE_decrypt(radix,&a,TL,TR,x,n);

	printf("Plaintext=\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n"); 

// Timing Test

	radix=10;
	TL=0xD8E7920A;
	TR=0xFA330A73; /* random tweaks */

/* Set 128-bit AES key */

	key[15]=0xEF; key[14]=0x43; key[13]=0x59; key[12]=0xD8; key[11]=0xD5; key[10]=0x80; key[9]=0xAA; key[8]=0x4F;
	key[7]=0x7F; key[6]=0x03; key[5]=0x6D; key[4]=0x6F; key[3]=0x04; key[2]=0xFC; key[1]=0x6A; key[0]=0x94;

	printf("\n128-bit aes key= ");
	for (i=0;i<16;i++)
		printf("%02x",(unsigned char) key[i]);
	printf("\n");

    aes_init(&a,MR_ECB,16,key,NULL);

	n=18;
	x[0]=8; x[1]=9; x[2]=0; x[3]=1; x[4]=2; x[5]=1; x[6]=2; x[7]=3;
	x[8]=4; x[9]=5; x[10]=6; x[11]=7; x[12]=8; x[13]=9; x[14]=0; x[15]=0; x[16]=0; x[17]=0;

/* Encrypt/decrypt short string */

	printf("\nTiming test for 1 million encryption/decryptions\n");
	printf("Start..\n");

	for (j=0;j<1000000;j++)
	{
		FPE_encrypt(radix,&a,TL,TR,x,n);
		FPE_decrypt(radix,&a,TL,TR,x,n);
	}
	printf("Finished\n");
	for (i=0;i<n;i++) printf("%d ",x[i]);
	printf("\n");


	return 0;
}

#endif
