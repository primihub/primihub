
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
 * Implementation of the Secure Hashing Algorithm SHA3 - Keccak
 * M. Scott 19/06/2013
 *
 * For use with byte-oriented messages only. 
 *
 * NOTE: This requires a 64-bit integer type to be defined
 */

#include "../mr_include/miracl.h"

#ifdef mr_unsign64

/* round constants */

static const mr_unsign64 RC[24]={
0x0000000000000001LL,0x0000000000008082LL,0x800000000000808ALL,0x8000000080008000LL,
0x000000000000808BLL,0x0000000080000001LL,0x8000000080008081LL,0x8000000000008009LL,
0x000000000000008ALL,0x0000000000000088LL,0x0000000080008009LL,0x000000008000000ALL,
0x000000008000808BLL,0x800000000000008BLL,0x8000000000008089LL,0x8000000000008003LL,
0x8000000000008002LL,0x8000000000000080LL,0x000000000000800ALL,0x800000008000000ALL,
0x8000000080008081LL,0x8000000000008080LL,0x0000000080000001LL,0x8000000080008008LL};

/* functions  */

#define SHA3_ROUNDS 24
#define rotl(x,n) (((x)<<n) | ((x)>>(64-n)))

/* permutation */

static void shs_transform(sha3 *sh)
{
	int i,j,k;
	mr_unsign64 C[5],D[5],B[5][5];
	
	for (k=0;k<SHA3_ROUNDS;k++)
	{
		C[0]=sh->S[0][0]^sh->S[0][1]^sh->S[0][2]^sh->S[0][3]^sh->S[0][4];
		C[1]=sh->S[1][0]^sh->S[1][1]^sh->S[1][2]^sh->S[1][3]^sh->S[1][4];
		C[2]=sh->S[2][0]^sh->S[2][1]^sh->S[2][2]^sh->S[2][3]^sh->S[2][4];
		C[3]=sh->S[3][0]^sh->S[3][1]^sh->S[3][2]^sh->S[3][3]^sh->S[3][4];
		C[4]=sh->S[4][0]^sh->S[4][1]^sh->S[4][2]^sh->S[4][3]^sh->S[4][4];

		D[0]=C[4]^rotl(C[1],1);
		D[1]=C[0]^rotl(C[2],1);
		D[2]=C[1]^rotl(C[3],1);
		D[3]=C[2]^rotl(C[4],1);
		D[4]=C[3]^rotl(C[0],1);

		for (i=0;i<5;i++)
			for (j=0;j<5;j++)
				sh->S[i][j]^=D[i];  /* let the compiler unroll it! */

		B[0][0]=sh->S[0][0];
		B[1][3]=rotl(sh->S[0][1],36);
		B[2][1]=rotl(sh->S[0][2],3);
		B[3][4]=rotl(sh->S[0][3],41);
		B[4][2]=rotl(sh->S[0][4],18);

		B[0][2]=rotl(sh->S[1][0],1);
		B[1][0]=rotl(sh->S[1][1],44);
		B[2][3]=rotl(sh->S[1][2],10);
		B[3][1]=rotl(sh->S[1][3],45);
		B[4][4]=rotl(sh->S[1][4],2);

		B[0][4]=rotl(sh->S[2][0],62);
		B[1][2]=rotl(sh->S[2][1],6);
		B[2][0]=rotl(sh->S[2][2],43);
		B[3][3]=rotl(sh->S[2][3],15);
		B[4][1]=rotl(sh->S[2][4],61);

		B[0][1]=rotl(sh->S[3][0],28);
		B[1][4]=rotl(sh->S[3][1],55);
		B[2][2]=rotl(sh->S[3][2],25);
		B[3][0]=rotl(sh->S[3][3],21);
		B[4][3]=rotl(sh->S[3][4],56);

		B[0][3]=rotl(sh->S[4][0],27);
		B[1][1]=rotl(sh->S[4][1],20);
		B[2][4]=rotl(sh->S[4][2],39);
		B[3][2]=rotl(sh->S[4][3],8);
		B[4][0]=rotl(sh->S[4][4],14);

		for (i=0;i<5;i++)
			for (j=0;j<5;j++)
				sh->S[i][j]=B[i][j]^(~B[(i+1)%5][j]&B[(i+2)%5][j]);

		sh->S[0][0]^=RC[k];
	}
}

/* Re-Initialize. olen is output length in bytes - 
   should be 28, 32, 48 or 64 (224, 256, 384, 512 bits resp.) */

void sha3_init(sha3 *sh,int olen)
{ 
    int i,j;
    for (i=0;i<5;i++) 
		for (j=0;j<5;j++)
			sh->S[i][j]=0;    /* 5x5x8 bytes = 200 bytes of state */
    sh->length=0;
	sh->len=olen;
	sh->rate=200-2*olen; /* number of bytes consumed in one gulp. Note that some bytes in the 
	                        state ("capacity") are not touched. Gulps are smaller for larger digests. 
							Important that olen<rate */
}

void sha3_process(sha3 *sh,int byte)
{
	int cnt=(int)(sh->length%sh->rate);
	int i,j,b=cnt%8;
	cnt/=8;
	i=cnt%5; j=cnt/5;  /* process by columns! */
	sh->S[i][j]^=((mr_unsign64)byte<<(8*b));
	sh->length++;
	if (sh->length%sh->rate==0) shs_transform(sh);
}

void sha3_hash(sha3 *sh,char *hash)
{ /* pad message and finish - supply digest */
	int i,j,k,m=0;
	mr_unsign64 el;
	int q=sh->rate-(sh->length%sh->rate);
	if (q==1) sha3_process(sh,0x86); 
	else
	{
		sha3_process(sh,0x06);
		while (sh->length%sh->rate!=sh->rate-1) sha3_process(sh,0x00);
		sha3_process(sh,0x80); /* this will force a final transform */
	}

/* extract by columns - assuming here digest size < rate */
	for (j=0;j<5;j++)
	    for (i=0;i<5;i++)
		{
			el=sh->S[i][j];
			for (k=0;k<8;k++)
			{
				hash[m++]=(el&0xff);
				if (m>=sh->len) return;
				el>>=8;
			}
		}
}

#endif

/* test program - see http://www.di-mgt.com.au/sha_testvectors.html

//char *s="abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";

//char *s="abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";

char *s="abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno";

int main()
{
	mr_unsign64 ii;
	int i,j,k,olen;
	char hash[64];
    sha3 sh;

	for (j=0;j<4;j++)
	{

		if (j==0) olen=28;
		if (j==1) olen=32;
		if (j==2) olen=48;
		if (j==3) olen=64;
		sha3_init(&sh,olen);

//		for (ii=0;ii<16777216;ii++)
//		{
//			for (k=0;k<strlen(s);k++)
//				sha3_process(&sh,s[k]);
//		}

	    sha3_process(&sh,'a');
	    sha3_process(&sh,'b');
	    sha3_process(&sh,'c');

		sha3_hash(&sh,hash);
		for (i=0;i<olen;i++) printf("%02x",(unsigned char)hash[i]);
		printf("\n");
	}
    return 0;
}

 */
