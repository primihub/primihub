
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
 * Implementation of the Secure Hashing Standard (SHS)
 * specified for use with the NIST Digital Signature Standard (DSS)
 *
 * Generates a 160 bit message digest. It should be impossible to come
 * come up with two messages that hash to the same value ("collision free").
 *
 * For use with byte-oriented messages only. Could/Should be speeded
 * up by unwinding loops in shs_transform(), and assembly patches.
 */

#include "miracl.h"
                /* for definition of mr_unsign32 & prototypes */
#define FIX

/* Include this #define in order to implement the
   rather mysterious 'fix' to SHS

   With this definition in, SHA-1 is implemented
   Without this definition, SHA-0 is implemented
*/


#define H0 0x67452301L
#define H1 0xefcdab89L
#define H2 0x98badcfeL
#define H3 0x10325476L
#define H4 0xc3d2e1f0L

#define K0 0x5a827999L
#define K1 0x6ed9eba1L
#define K2 0x8f1bbcdcL
#define K3 0xca62c1d6L

#define PAD  0x80
#define ZERO 0

/* functions */

#define S(n,x) (((x)<<n) | ((x)>>(32-n)))

#define F0(x,y,z) (z^(x&(y^z)))
#define F1(x,y,z) (x^y^z)
#define F2(x,y,z) ((x&y) | (z&(x|y))) 
#define F3(x,y,z) (x^y^z)

static void shs_transform(sha *sh)
{ /* basic transformation step */
    mr_unsign32 a,b,c,d,e,temp;
    int t;
#ifdef FIX
    for (t=16;t<80;t++) sh->w[t]=S(1,sh->w[t-3]^sh->w[t-8]^sh->w[t-14]^sh->w[t-16]);
#else
    for (t=16;t<80;t++) sh->w[t]=sh->w[t-3]^sh->w[t-8]^sh->w[t-14]^sh->w[t-16];
#endif
    a=sh->h[0]; b=sh->h[1]; c=sh->h[2]; d=sh->h[3]; e=sh->h[4];
    for (t=0;t<20;t++)
    { /* 20 times - mush it up */
        temp=K0+F0(b,c,d)+S(5,a)+e+sh->w[t];
        e=d; d=c;
        c=S(30,b);
        b=a; a=temp;
    }
    for (t=20;t<40;t++)
    { /* 20 more times - mush it up */
        temp=K1+F1(b,c,d)+S(5,a)+e+sh->w[t];
        e=d; d=c;
        c=S(30,b);
        b=a; a=temp;
    }
    for (t=40;t<60;t++)
    { /* 20 more times - mush it up */
        temp=K2+F2(b,c,d)+S(5,a)+e+sh->w[t];
        e=d; d=c;
        c=S(30,b);
        b=a; a=temp;
    }
    for (t=60;t<80;t++)
    { /* 20 more times - mush it up */
        temp=K3+F3(b,c,d)+S(5,a)+e+sh->w[t];
        e=d; d=c;
        c=S(30,b);
        b=a; a=temp;
    }
    sh->h[0]+=a; sh->h[1]+=b; sh->h[2]+=c;
    sh->h[3]+=d; sh->h[4]+=e;
} 

void shs_init(sha *sh)
{ /* re-initialise */
    int i;
    for (i=0;i<80;i++) sh->w[i]=0L;
    sh->length[0]=sh->length[1]=0L;
    sh->h[0]=H0;
    sh->h[1]=H1;
    sh->h[2]=H2;
    sh->h[3]=H3;
    sh->h[4]=H4;
}

void shs_process(sha *sh,int byte)
{ /* process the next message byte */
    int cnt;
    
    cnt=(int)((sh->length[0]/32)%16);
    
    sh->w[cnt]<<=8;
    sh->w[cnt]|=(mr_unsign32)(byte&0xFF);

    sh->length[0]+=8;
    if (sh->length[0]==0L) { sh->length[1]++; sh->length[0]=0L; }
    if ((sh->length[0]%512)==0) shs_transform(sh);
}

void shs_hash(sha *sh,char hash[20])
{ /* pad message and finish - supply digest */
    int i;
    mr_unsign32 len0,len1;
    len0=sh->length[0];
    len1=sh->length[1];
    shs_process(sh,PAD);
    while ((sh->length[0]%512)!=448) shs_process(sh,ZERO);
    sh->w[14]=len1;
    sh->w[15]=len0;    
    shs_transform(sh);
    for (i=0;i<20;i++)
    { /* convert to bytes */
        hash[i]=(char)((sh->h[i/4]>>(8*(3-i%4))) & 0xffL);
    }
    shs_init(sh);
}

/* test program: should produce digest  

   84983e44 1c3bd26e baae4aa1 f95129e5 e54670f1

#include <stdio.h>
#include "miracl.h"

char test[]="abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";

int main()
{
    char hash[20];
    int i;
    sha sh;
    shs_init(&sh);
    for (i=0;test[i]!=0;i++) shs_process(&sh,test[i]);
    shs_hash(&sh,hash);    
    for (i=0;i<20;i++) printf("%02x",(unsigned char)hash[i]);
    printf("\n");
    return 0;
}
 
*/

