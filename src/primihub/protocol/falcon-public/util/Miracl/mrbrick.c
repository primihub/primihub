
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
 *   Module to implement Comb method for fast
 *   computation of g^x mod n, for fixed g and n, using precomputation. 
 *   This idea can be used to substantially speed up certain phases 
 *   of the Digital Signature Standard (DSS) for example.
 *
 *   See "Handbook of Applied Cryptography", CRC Press, 2001
 */

#include <stdlib.h> 
#include "miracl.h"

#ifndef MR_STATIC

BOOL brick_init(_MIPD_ brick *b,big g,big n,int window,int nb)
{ /* Uses Montgomery arithmetic internally            *
   * g  is the fixed base for exponentiation          *
   * n  is the fixed modulus                          *
   * nb is the maximum number of bits in the exponent */

    int i,j,k,t,bp,len,bptr,is;
    big *table;

#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif
    if (nb<2 || window<1 || window>nb || mr_mip->ERNUM) return FALSE;
    t=MR_ROUNDUP(nb,window);
    if (t<2) return FALSE;

    MR_IN(109)

#ifndef MR_ALWAYS_BINARY
    if (mr_mip->base != mr_mip->base2)
    {
        mr_berror(_MIPP_ MR_ERR_NOT_SUPPORTED);
        MR_OUT
        return FALSE;
    }
#endif

    b->window=window;
    b->max=nb;
    table=(big *)mr_alloc(_MIPP_ (1<<window),sizeof(big));
    if (table==NULL)
    {
        mr_berror(_MIPP_ MR_ERR_OUT_OF_MEMORY);   
        MR_OUT
        return FALSE;
    }

    b->n=mirvar(_MIPP_ 0);
    copy(n,b->n);
    prepare_monty(_MIPP_ n);
    nres(_MIPP_ g,mr_mip->w1);
    convert(_MIPP_ 1,mr_mip->w2);
    nres(_MIPP_ mr_mip->w2,mr_mip->w2);

    table[0]=mirvar(_MIPP_ 0);
    copy(mr_mip->w2,table[0]);
    table[1]=mirvar(_MIPP_ 0);
    copy(mr_mip->w1,table[1]);
    for (j=0;j<t;j++)
        nres_modmult(_MIPP_ mr_mip->w1,mr_mip->w1,mr_mip->w1);

    k=1;
    for (i=2;i<(1<<window);i++)
    {
        table[i]=mirvar(_MIPP_ 0);
        if (i==(1<<k))
        {
            k++;
            copy(mr_mip->w1,table[i]);
            
            for (j=0;j<t;j++)
                 nres_modmult(_MIPP_ mr_mip->w1,mr_mip->w1,mr_mip->w1);
            continue;
        }
        bp=1;
        copy(mr_mip->w2,table[i]);
        for (j=0;j<k;j++)
        {
            if (i&bp)
			{
				is=1<<j;
                nres_modmult(_MIPP_ table[is],table[i],table[i]);
			}
            bp<<=1;
        }
    }

/* create the table */

    len=n->len;
    bptr=0;
    b->table=(mr_small *)mr_alloc(_MIPP_ len*(1<<window),sizeof(mr_small));

    for (i=0;i<(1<<window);i++)
    {
        for (j=0;j<len;j++)
        {
            b->table[bptr++]=table[i]->w[j];
        }
        mirkill(table[i]);
    }

    mr_free(table);
    
    MR_OUT
    return TRUE;
}

void brick_end(brick *b)
{
    mirkill(b->n);
    mr_free(b->table);  
}

#else

/* use precomputated table in ROM - see ebrick2.c for example of how to create such a table, and ecdh.c 
   for an example of use */

void brick_init(brick *b,const mr_small *table,big n,int window,int nb)
{
    b->table=table;
    b->n=n;
    b->window=window;
    b->max=nb;
}

#endif

void pow_brick(_MIPD_ brick *b,big e,big w)
{
    int i,j,t,len,promptr,maxsize;

#ifdef MR_OS_THREADS
    miracl *mr_mip=get_mip();
#endif

    if (size(e)<0) mr_berror(_MIPP_ MR_ERR_NEG_POWER);
    t=MR_ROUNDUP(b->max,b->window);
    
    MR_IN(110)

#ifndef MR_ALWAYS_BINARY
    if (mr_mip->base != mr_mip->base2)
    {
        mr_berror(_MIPP_ MR_ERR_NOT_SUPPORTED);
        MR_OUT
        return;
    }
#endif

    if (logb2(_MIPP_ e) > b->max)
    {
        mr_berror(_MIPP_ MR_ERR_EXP_TOO_BIG);
        MR_OUT
        return;
    }

    prepare_monty(_MIPP_ b->n);
    j=recode(_MIPP_ e,t,b->window,t-1);

    len=b->n->len;
    maxsize=(1<<b->window)*len;

    promptr=j*len;
    init_big_from_rom(mr_mip->w1,len,b->table,maxsize,&promptr);

    for (i=t-2;i>=0;i--)
    {
        j=recode(_MIPP_ e,t,b->window,i);
        nres_modmult(_MIPP_ mr_mip->w1,mr_mip->w1,mr_mip->w1);
        if (j>0)
        {
            promptr=j*len;
            init_big_from_rom(mr_mip->w2,len,b->table,maxsize,&promptr);
            nres_modmult(_MIPP_ mr_mip->w1,mr_mip->w2,mr_mip->w1);
        }
    }
    redc(_MIPP_ mr_mip->w1,w);
    MR_OUT
}
