
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
 * Implementation of the Secure Hashing Algorithm (SHA-384 and SHA-512)
 *
 * Generates a a 384 or 512 bit message digest. It should be impossible to come
 * come up with two messages that hash to the same value ("collision free").
 *
 * For use with byte-oriented messages only. Could/Should be speeded
 * up by unwinding loops in shs_transform(), and assembly patches.
 *
 * NOTE: This requires a 64-bit integer type to be defined
 */

#include "miracl.h"

#ifdef mr_unsign64


#ifdef __GNUC__ 
#define H0 0x6a09e667f3bcc908LL
#define H1 0xbb67ae8584caa73bLL
#define H2 0x3c6ef372fe94f82bLL
#define H3 0xa54ff53a5f1d36f1LL
#define H4 0x510e527fade682d1LL
#define H5 0x9b05688c2b3e6c1fLL
#define H6 0x1f83d9abfb41bd6bLL
#define H7 0x5be0cd19137e2179LL

#define H8 0xcbbb9d5dc1059ed8LL
#define H9 0x629a292a367cd507LL
#define HA 0x9159015a3070dd17LL
#define HB 0x152fecd8f70e5939LL
#define HC 0x67332667ffc00b31LL
#define HD 0x8eb44a8768581511LL
#define HE 0xdb0c2e0d64f98fa7LL
#define HF 0x47b5481dbefa4fa4LL
#else
#define H0 0x6a09e667f3bcc908
#define H1 0xbb67ae8584caa73b
#define H2 0x3c6ef372fe94f82b
#define H3 0xa54ff53a5f1d36f1
#define H4 0x510e527fade682d1
#define H5 0x9b05688c2b3e6c1f
#define H6 0x1f83d9abfb41bd6b
#define H7 0x5be0cd19137e2179

#define H8 0xcbbb9d5dc1059ed8
#define H9 0x629a292a367cd507
#define HA 0x9159015a3070dd17
#define HB 0x152fecd8f70e5939
#define HC 0x67332667ffc00b31
#define HD 0x8eb44a8768581511
#define HE 0xdb0c2e0d64f98fa7
#define HF 0x47b5481dbefa4fa4
#endif
/* */

#ifdef __GNUC__ 
static const mr_unsign64 K[80]=
{0x428a2f98d728ae22LL,0x7137449123ef65cdLL,0xb5c0fbcfec4d3b2fLL,0xe9b5dba58189dbbcLL,
0x3956c25bf348b538LL,0x59f111f1b605d019LL,0x923f82a4af194f9bLL,0xab1c5ed5da6d8118LL,
0xd807aa98a3030242LL,0x12835b0145706fbeLL,0x243185be4ee4b28cLL,0x550c7dc3d5ffb4e2LL,
0x72be5d74f27b896fLL,0x80deb1fe3b1696b1LL,0x9bdc06a725c71235LL,0xc19bf174cf692694LL,
0xe49b69c19ef14ad2LL,0xefbe4786384f25e3LL,0x0fc19dc68b8cd5b5LL,0x240ca1cc77ac9c65LL,
0x2de92c6f592b0275LL,0x4a7484aa6ea6e483LL,0x5cb0a9dcbd41fbd4LL,0x76f988da831153b5LL,
0x983e5152ee66dfabLL,0xa831c66d2db43210LL,0xb00327c898fb213fLL,0xbf597fc7beef0ee4LL,
0xc6e00bf33da88fc2LL,0xd5a79147930aa725LL,0x06ca6351e003826fLL,0x142929670a0e6e70LL,
0x27b70a8546d22ffcLL,0x2e1b21385c26c926LL,0x4d2c6dfc5ac42aedLL,0x53380d139d95b3dfLL,
0x650a73548baf63deLL,0x766a0abb3c77b2a8LL,0x81c2c92e47edaee6LL,0x92722c851482353bLL,
0xa2bfe8a14cf10364LL,0xa81a664bbc423001LL,0xc24b8b70d0f89791LL,0xc76c51a30654be30LL,
0xd192e819d6ef5218LL,0xd69906245565a910LL,0xf40e35855771202aLL,0x106aa07032bbd1b8LL,
0x19a4c116b8d2d0c8LL,0x1e376c085141ab53LL,0x2748774cdf8eeb99LL,0x34b0bcb5e19b48a8LL,
0x391c0cb3c5c95a63LL,0x4ed8aa4ae3418acbLL,0x5b9cca4f7763e373LL,0x682e6ff3d6b2b8a3LL,
0x748f82ee5defb2fcLL,0x78a5636f43172f60LL,0x84c87814a1f0ab72LL,0x8cc702081a6439ecLL,
0x90befffa23631e28LL,0xa4506cebde82bde9LL,0xbef9a3f7b2c67915LL,0xc67178f2e372532bLL,
0xca273eceea26619cLL,0xd186b8c721c0c207LL,0xeada7dd6cde0eb1eLL,0xf57d4f7fee6ed178LL,
0x06f067aa72176fbaLL,0x0a637dc5a2c898a6LL,0x113f9804bef90daeLL,0x1b710b35131c471bLL,
0x28db77f523047d84LL,0x32caab7b40c72493LL,0x3c9ebe0a15c9bebcLL,0x431d67c49c100d4cLL,
0x4cc5d4becb3e42b6LL,0x597f299cfc657e2aLL,0x5fcb6fab3ad6faecLL,0x6c44198c4a475817LL};
#else
static const mr_unsign64 K[80]=
{0x428a2f98d728ae22,0x7137449123ef65cd,0xb5c0fbcfec4d3b2f,0xe9b5dba58189dbbc,
0x3956c25bf348b538,0x59f111f1b605d019,0x923f82a4af194f9b,0xab1c5ed5da6d8118,
0xd807aa98a3030242,0x12835b0145706fbe,0x243185be4ee4b28c,0x550c7dc3d5ffb4e2,
0x72be5d74f27b896f,0x80deb1fe3b1696b1,0x9bdc06a725c71235,0xc19bf174cf692694,
0xe49b69c19ef14ad2,0xefbe4786384f25e3,0x0fc19dc68b8cd5b5,0x240ca1cc77ac9c65,
0x2de92c6f592b0275,0x4a7484aa6ea6e483,0x5cb0a9dcbd41fbd4,0x76f988da831153b5,
0x983e5152ee66dfab,0xa831c66d2db43210,0xb00327c898fb213f,0xbf597fc7beef0ee4,
0xc6e00bf33da88fc2,0xd5a79147930aa725,0x06ca6351e003826f,0x142929670a0e6e70,
0x27b70a8546d22ffc,0x2e1b21385c26c926,0x4d2c6dfc5ac42aed,0x53380d139d95b3df,
0x650a73548baf63de,0x766a0abb3c77b2a8,0x81c2c92e47edaee6,0x92722c851482353b,
0xa2bfe8a14cf10364,0xa81a664bbc423001,0xc24b8b70d0f89791,0xc76c51a30654be30,
0xd192e819d6ef5218,0xd69906245565a910,0xf40e35855771202a,0x106aa07032bbd1b8,
0x19a4c116b8d2d0c8,0x1e376c085141ab53,0x2748774cdf8eeb99,0x34b0bcb5e19b48a8,
0x391c0cb3c5c95a63,0x4ed8aa4ae3418acb,0x5b9cca4f7763e373,0x682e6ff3d6b2b8a3,
0x748f82ee5defb2fc,0x78a5636f43172f60,0x84c87814a1f0ab72,0x8cc702081a6439ec,
0x90befffa23631e28,0xa4506cebde82bde9,0xbef9a3f7b2c67915,0xc67178f2e372532b,
0xca273eceea26619c,0xd186b8c721c0c207,0xeada7dd6cde0eb1e,0xf57d4f7fee6ed178,
0x06f067aa72176fba,0x0a637dc5a2c898a6,0x113f9804bef90dae,0x1b710b35131c471b,
0x28db77f523047d84,0x32caab7b40c72493,0x3c9ebe0a15c9bebc,0x431d67c49c100d4c,
0x4cc5d4becb3e42b6,0x597f299cfc657e2a,0x5fcb6fab3ad6faec,0x6c44198c4a475817};
#endif

#define PAD  0x80
#define ZERO 0

/* functions */

#define S(n,x) (((x)>>n) | ((x)<<(64-n)))
#define R(n,x) ((x)>>n)

#define Ch(x,y,z)  ((x&y)^(~(x)&z))
#define Maj(x,y,z) ((x&y)^(x&z)^(y&z))
#define Sig0(x)    (S(28,x)^S(34,x)^S(39,x))
#define Sig1(x)    (S(14,x)^S(18,x)^S(41,x))
#define theta0(x)  (S(1,x)^S(8,x)^R(7,x))
#define theta1(x)  (S(19,x)^S(61,x)^R(6,x))

static void shs_transform(sha512 *sh)
{ /* basic transformation step */
    mr_unsign64 a,b,c,d,e,f,g,h,t1,t2;
    int j;
    for (j=16;j<80;j++) 
        sh->w[j]=theta1(sh->w[j-2])+sh->w[j-7]+theta0(sh->w[j-15])+sh->w[j-16];

    a=sh->h[0]; b=sh->h[1]; c=sh->h[2]; d=sh->h[3]; 
    e=sh->h[4]; f=sh->h[5]; g=sh->h[6]; h=sh->h[7];

    for (j=0;j<80;j++)
    { /* 80 times - mush it up */
        t1=h+Sig1(e)+Ch(e,f,g)+K[j]+sh->w[j];
        t2=Sig0(a)+Maj(a,b,c);
        h=g; g=f; f=e;
        e=d+t1;
        d=c;
        c=b;
        b=a;
        a=t1+t2;        
    }
    sh->h[0]+=a; sh->h[1]+=b; sh->h[2]+=c; sh->h[3]+=d; 
    sh->h[4]+=e; sh->h[5]+=f; sh->h[6]+=g; sh->h[7]+=h; 
} 

void shs512_init(sha512 *sh)
{ /* re-initialise */
    int i;
    for (i=0;i<80;i++) sh->w[i]=0;
    sh->length[0]=sh->length[1]=0;
    sh->h[0]=H0;
    sh->h[1]=H1;
    sh->h[2]=H2;
    sh->h[3]=H3;
    sh->h[4]=H4;
    sh->h[5]=H5;
    sh->h[6]=H6;
    sh->h[7]=H7;
}

void shs384_init(sha384 *sh)
{ /* re-initialise */
    int i;
    for (i=0;i<80;i++) sh->w[i]=0;
    sh->length[0]=sh->length[1]=0;
    sh->h[0]=H8;
    sh->h[1]=H9;
    sh->h[2]=HA;
    sh->h[3]=HB;
    sh->h[4]=HC;
    sh->h[5]=HD;
    sh->h[6]=HE;
    sh->h[7]=HF;
}


void shs512_process(sha512 *sh,int byte)
{ /* process the next message byte */
    int cnt;
    
    cnt=(int)((sh->length[0]/64)%16);
    
    sh->w[cnt]<<=8;
    sh->w[cnt]|=(mr_unsign64)(byte&0xFF);

    sh->length[0]+=8;
    if (sh->length[0]==0L) { sh->length[1]++; sh->length[0]=0L; }
    if ((sh->length[0]%1024)==0) shs_transform(sh);
}


void shs384_process(sha384 *sh,int byte)
{ /* process the next message byte */
    int cnt;
    
    cnt=(int)((sh->length[0]/48)%16);
    
    sh->w[cnt]<<=8;
    sh->w[cnt]|=(mr_unsign64)(byte&0xFF);

    sh->length[0]+=8;
    if (sh->length[0]==0L) { sh->length[1]++; sh->length[0]=0L; }
    if ((sh->length[0]%1024)==0) shs_transform(sh);
}


void shs512_hash(sha512 *sh,char hash[64])
{ /* pad message and finish - supply digest */
    int i;
    mr_unsign64 len0,len1;
    len0=sh->length[0];
    len1=sh->length[1];
    shs512_process(sh,PAD);
    while ((sh->length[0]%1024)!=896) shs512_process(sh,ZERO);
    sh->w[14]=len1;
    sh->w[15]=len0;    
    shs_transform(sh);
    for (i=0;i<64;i++)
    { /* convert to bytes */
        hash[i]=(char)((sh->h[i/8]>>(8*(7-i%8))) & 0xffL);
    }
    shs512_init(sh);
}

void shs384_hash(sha384 *sh,char hash[48])
{ /* pad message and finish - supply digest */
    int i;
    mr_unsign64 len0,len1;
    len0=sh->length[0];
    len1=sh->length[1];
    shs384_process(sh,PAD);
    while ((sh->length[0]%1024)!=896) shs384_process(sh,ZERO);
    sh->w[14]=len1;
    sh->w[15]=len0;    
    shs_transform(sh);
    for (i=0;i<48;i++)
    { /* convert to bytes */
        hash[i]=(char)((sh->h[i/8]>>(8*(7-i%8))) & 0xffL);
    }
    shs384_init(sh);
}


#endif

/* test program: should produce digests

512 bit  

8e959b75dae313da 8cf4f72814fc143f 8f7779c6eb9f7fa1 7299aeadb6889018 
501d289e4900f7e4 331b99dec4b5433a c7d329eeb6dd2654 5e96e55b874be909


384 bit

09330c33f71147e8 3d192fc782cd1b47 53111b173b3b05d2 2fa08086e3b0f712 
fcc7c71a557e2db9 66c3e9fa91746039


#include <stdio.h>
#include "miracl.h"

char test[]="abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";

int main()
{
    char hash[64];
    int i;
    sha512 sh;
    shs512_init(&sh);
    for (i=0;test[i]!=0;i++) shs512_process(&sh,test[i]);
    shs512_hash(&sh,hash);    
    for (i=0;i<64;i++) printf("%02x",(unsigned char)hash[i]);
    printf("\n");

    shs384_init(&sh);
    for (i=0;test[i]!=0;i++) shs384_process(&sh,test[i]);
    shs384_hash(&sh,hash);    
    for (i=0;i<48;i++) printf("%02x",(unsigned char)hash[i]);
    printf("\n");

    return 0;
}

*/

