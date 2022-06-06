
/***************************************************************************
                                                                           *
Copyright 2013 CertiVox IOM Ltd.                                           *
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
 * Implementation of the AES-GCM Encryption/Authentication
 *
 * Some restrictions.. 
 * 1. Only for use with AES
 * 2. Returned tag is always 128-bits. Truncate at your own risk.
 * 3. The order of function calls must follow some rules
 *
 * Typical sequence of calls..
 * 1. call gcm_init
 * 2. call gcm_add_header any number of times, as long as length of header is multiple of 16 bytes (block size)
 * 3. call gcm_add_header one last time with any length of header
 * 4. call gcm_add_cipher any number of times, as long as length of cipher/plaintext is multiple of 16 bytes
 * 5. call gcm_add_cipher one last time with any length of cipher/plaintext
 * 6. call gcm_finish to extract the tag.
 *
 * See http://www.mindspring.com/~dmcgrew/gcm-nist-6.pdf
 */

#include <stdlib.h> 
#include <string.h>
#include "miracl.h"

#define NB 4
#define MR_WORD mr_unsign32

static MR_WORD pack(const MR_BYTE *b)
{ /* pack bytes into a 32-bit Word */
    return ((MR_WORD)b[0]<<24)|((MR_WORD)b[1]<<16)|((MR_WORD)b[2]<<8)|(MR_WORD)b[3];
}

static void unpack(MR_WORD a,MR_BYTE *b)
{ /* unpack bytes from a word */
    b[3]=MR_TOBYTE(a);
    b[2]=MR_TOBYTE(a>>8);
    b[1]=MR_TOBYTE(a>>16);
    b[0]=MR_TOBYTE(a>>24);
}

static void precompute(gcm *g,MR_BYTE *H)
{ /* precompute small 2k bytes gf2m table of x^n.H */
	int i,j;
	MR_WORD *last,*next,b;

	for (i=j=0;i<NB;i++,j+=4) g->table[0][i]=pack((MR_BYTE *)&H[j]);

	for (i=1;i<128;i++)
	{
		next=g->table[i]; last=g->table[i-1]; b=0;
		for (j=0;j<NB;j++) {next[j]=b|(last[j])>>1; b=last[j]<<31;}
		if (b) next[0]^=0xE1000000; /* irreducible polynomial */
	}
}

static void gf2mul(gcm *g)
{ /* gf2m mul - Z=H*X mod 2^128 */
	int i,j,m,k;
	MR_WORD P[4];
	MR_BYTE b;

	P[0]=P[1]=P[2]=P[3]=0;
	j=8; m=0;
	for (i=0;i<128;i++)
	{
		b=(g->stateX[m]>>(--j))&1;
		if (b) for (k=0;k<NB;k++) P[k]^=g->table[i][k];
		if (j==0)
		{
			j=8; m++;
			if (m==16) break;
		}
	}
	for (i=j=0;i<NB;i++,j+=4) unpack(P[i],(MR_BYTE *)&g->stateX[j]);
}

static void gcm_wrap(gcm *g)
{ /* Finish off GHASH */
	int i,j;
	MR_WORD F[4];
	MR_BYTE L[16];

/* convert lengths from bytes to bits */
	F[0]=(g->lenA[0]<<3)|(g->lenA[1]&0xE0000000)>>29;
	F[1]=g->lenA[1]<<3;
	F[2]=(g->lenC[0]<<3)|(g->lenC[1]&0xE0000000)>>29;
	F[3]=g->lenC[1]<<3;
	for (i=j=0;i<NB;i++,j+=4) unpack(F[i],(MR_BYTE *)&L[j]);

	for (i=0;i<16;i++) g->stateX[i]^=L[i];
	gf2mul(g);
}

void gcm_init(gcm* g,int nk,char *key,int niv,char *iv)
{ /* iv size niv is usually 12 bytes (96 bits). AES key size nk can be 16,24 or 32 bytes */
	int i;
	MR_BYTE H[16];
	for (i=0;i<16;i++) {H[i]=0; g->stateX[i]=0;}

	aes_init(&(g->a),MR_ECB,nk,key,iv);
	aes_ecb_encrypt(&(g->a),H);     /* E(K,0) */
	precompute(g,H);
	
	g->lenA[0]=g->lenC[0]=g->lenA[1]=g->lenC[1]=0;
	if (niv==12)
	{
		for (i=0;i<12;i++) g->a.f[i]=iv[i];
		unpack((MR_WORD)1,(MR_BYTE *)&(g->a.f[12]));  /* initialise IV */
		for (i=0;i<16;i++) g->Y_0[i]=g->a.f[i];
	}
	else
	{
		g->status=GCM_ACCEPTING_CIPHER;
		gcm_add_cipher(g,0,iv,niv,NULL); /* GHASH(H,0,IV) */
		gcm_wrap(g);
		for (i=0;i<16;i++) {g->a.f[i]=g->stateX[i];g->Y_0[i]=g->a.f[i];g->stateX[i]=0;}
		g->lenA[0]=g->lenC[0]=g->lenA[1]=g->lenC[1]=0;
	}
	g->status=GCM_ACCEPTING_HEADER;
}

BOOL gcm_add_header(gcm* g,char *header,int len)
{ /* Add some header. Won't be encrypted, but will be authenticated. len is length of header */
	int i,j=0;
	if (g->status!=GCM_ACCEPTING_HEADER) return FALSE;

	while (j<len)
	{
		for (i=0;i<16 && j<len;i++)
		{
			g->stateX[i]^=header[j++];
			g->lenA[1]++; if (g->lenA[1]==0) g->lenA[0]++;
		}
		gf2mul(g);
	}
	if (len%16!=0) g->status=GCM_ACCEPTING_CIPHER;
	return TRUE;
}

BOOL gcm_add_cipher(gcm *g,int mode,char *plain,int len,char *cipher)
{ /* Add plaintext to extract ciphertext, or visa versa, depending on mode. len is length of plaintext/ciphertext. Note this file combines GHASH() functionality with encryption/decryption */
	int i,j=0;
	MR_WORD counter;
	MR_BYTE B[16];
	if (g->status==GCM_ACCEPTING_HEADER) g->status=GCM_ACCEPTING_CIPHER;
	if (g->status!=GCM_ACCEPTING_CIPHER) return FALSE;

	while (j<len)
	{
		if (cipher!=NULL)
		{
			counter=pack((MR_BYTE *)&(g->a.f[12]));
			counter++;
			unpack(counter,(MR_BYTE *)&(g->a.f[12]));  /* increment counter */
			for (i=0;i<16;i++) B[i]=g->a.f[i];
			aes_ecb_encrypt(&(g->a),B);        /* encrypt it  */
		}
		for (i=0;i<16 && j<len;i++)
		{
			if (cipher==NULL)
				g->stateX[i]^=plain[j++];
			else
			{
				if (mode==GCM_ENCRYPTING) cipher[j]=plain[j]^B[i];
				if (mode==GCM_DECRYPTING) plain[j]=cipher[j]^B[i];
				g->stateX[i]^=cipher[j++];
			}
			g->lenC[1]++; if (g->lenC[1]==0) g->lenC[0]++;
		}
		gf2mul(g);
	}
	if (len%16!=0) g->status=GCM_NOT_ACCEPTING_MORE;
	return TRUE;
}

void gcm_finish(gcm *g,char *tag)
{ /* Finish off GHASH and extract tag (MAC) */
	int i;

	gcm_wrap(g);

/* extract tag */
	if (tag!=NULL)
	{
		aes_ecb_encrypt(&(g->a),g->Y_0);        /* E(K,Y0) */
		for (i=0;i<16;i++) g->Y_0[i]^=g->stateX[i];
		for (i=0;i<16;i++) {tag[i]=g->Y_0[i];g->Y_0[i]=g->stateX[i]=0;}
	}
	g->status=GCM_FINISHED;
	aes_end(&(g->a));
}

/*
// Compile with
// cl /O2 mrgcm.c mraes.c

static void hex2bytes(char *hex,char *bin)
{
	int i;
	char v;
	int len=strlen(hex);
	for (i = 0; i < len/2; i++) {
        char c = hex[2*i];
        if (c >= '0' && c <= '9') {
            v = c - '0';
        } else if (c >= 'A' && c <= 'F') {
            v = c - 'A' + 10;
        } else if (c >= 'a' && c <= 'f') {
            v = c - 'a' + 10;
        } else {
            v = 0;
        }
        v <<= 4;
        c = hex[2*i + 1];
        if (c >= '0' && c <= '9') {
            v += c - '0';
        } else if (c >= 'A' && c <= 'F') {
            v += c - 'A' + 10;
        } else if (c >= 'a' && c <= 'f') {
            v += c - 'a' + 10;
        } else {
            v = 0;
        }
        bin[i] = v;
    }
}

int main()
{
	int i;

	char* KT="feffe9928665731c6d6a8f9467308308";
	char* MT="d9313225f88406e5a55909c5aff5269a86a7a9531534f7da2e4c303d8a318a721c3c0c95956809532fcf0e2449a6b525b16aedf5aa0de657ba637b39";
	char* HT="feedfacedeadbeeffeedfacedeadbeefabaddad2";
//	char* NT="cafebabefacedbaddecaf888";
// Tag should be 5bc94fbc3221a5db94fae95ae7121a47
	char* NT="9313225df88406e555909c5aff5269aa6a7a9538534f7da1e4c303d2a318a728c3c0c95156809539fcf0e2429a6b525416aedbf5a0de6a57a637b39b";
// Tag should be 619cc5aefffe0bfa462af43c1699d050


	int len=strlen(MT)/2;
	int lenH=strlen(HT)/2;
	int lenK=strlen(KT)/2;
	int lenIV=strlen(NT)/2;

	char T[16];   // Tag
	char K[32];   // AES Key
	char H[64];   // Header - to be included in Authentication, but not encrypted
	char N[100];   // IV - Initialisation vector
	char M[100];  // Plaintext to be encrypted/authenticated
	char C[100];  // Ciphertext
	char P[100];  // Recovered Plaintext 

	gcm g;

    hex2bytes(MT, M);
    hex2bytes(HT, H);
    hex2bytes(NT, N);
	hex2bytes(KT, K);

 	printf("Plaintext=\n");
	for (i=0;i<len;i++) printf("%02x",(unsigned char)M[i]);
	printf("\n");

	gcm_init(&g,lenK,K,lenIV,N);
	gcm_add_header(&g,H,lenH);
	gcm_add_cipher(&g,GCM_ENCRYPTING,M,len,C);
	gcm_finish(&g,T);

	printf("Ciphertext=\n");
	for (i=0;i<len;i++) printf("%02x",(unsigned char)C[i]);
	printf("\n");
        
	printf("Tag=\n");
	for (i=0;i<16;i++) printf("%02x",(unsigned char)T[i]);
	printf("\n");

	gcm_init(&g,lenK,K,lenIV,N);
	gcm_add_header(&g,H,lenH);
	gcm_add_cipher(&g,GCM_DECRYPTING,P,len,C);
	gcm_finish(&g,T);

 	printf("Plaintext=\n");
	for (i=0;i<len;i++) printf("%02x",(unsigned char)P[i]);
	printf("\n");

	printf("Tag=\n");
	for (i=0;i<16;i++) printf("%02x",(unsigned char)T[i]);
	printf("\n");
}
*/
