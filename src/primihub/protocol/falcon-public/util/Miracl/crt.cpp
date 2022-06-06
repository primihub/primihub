/*
 *   MIRACL Chinese Remainder Thereom routines (for use with crt.h) 
 */

#include "crt.h"

Crt::Crt(int r,Big *moduli)
{ /* constructor */
    big *b=(big *)mr_alloc(r,sizeof(big));
    for (int i=0;i<r;i++) b[i]=moduli[i].getbig();
    type=MR_CRT_BIG;
    crt_init(&bc,r,b);
    mr_free(b);
}

Crt::Crt(int r,mr_utype *moduli)
{ /* constructor */
    type=MR_CRT_SMALL; 
    scrt_init(&sc,r,moduli);
}

Big Crt::eval(Big *u)
{           
    Big x;
    big *b=(big *)mr_alloc(bc.NP,sizeof(big));
    for (int i=0;i<bc.NP;i++) b[i]=u[i].getbig();
    crt(&bc,b,x.getbig());
    mr_free(b); 
    return x;
}

Big Crt::eval(mr_utype *u)
{
    Big x;
    scrt(&sc,u,x.getbig());
    return x;
}

