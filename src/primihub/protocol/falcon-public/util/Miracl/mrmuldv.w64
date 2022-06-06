/* Win64 C version of mrmuldv.c */

#include "miracl.h"

mr_small muldiv(mr_small a,mr_small b,mr_small c,mr_small m,mr_small *rp)
{
    int i;
    mr_small d,q=0,r=0;
    d=m-a;
    for (i=MIRACL/4;i>0;i--)
    { /* do it bit by bit */
        r<<=1;
        if ((mr_utype)c<0) r++;
        c<<=1;
        q<<=1;
        if ((mr_utype)b<0)
        {
            if (r>=m) { r-=d; q++; }
            else        r+=a;
        }
        if (r>=m) { r-=m; q++; }
        b<<=1;
        r<<=1;
        if ((mr_utype)c<0) r++;
        c<<=1;
        q<<=1;
        if ((mr_utype)b<0)
        {
            if (r>=m) { r-=d; q++; }
            else        r+=a;
        }
        if (r>=m) { r-=m; q++; }
        b<<=1;
        r<<=1;
        if ((mr_utype)c<0) r++;
        c<<=1;
        q<<=1;
        if ((mr_utype)b<0)
        {
            if (r>=m) { r-=d; q++; }
            else        r+=a;
        }
        if (r>=m) { r-=m; q++; }
        b<<=1;
        r<<=1;
        if ((mr_utype)c<0) r++;
        c<<=1;
        q<<=1;
        if ((mr_utype)b<0)
        {
            if (r>=m) { r-=d; q++; }
            else        r+=a;
        }
        if (r>=m) { r-=m; q++; }
        b<<=1;
    }
    *rp=r;
    return q;
}

mr_small muldvm(mr_small a,mr_small c,mr_small m,mr_small *rp)
{ /* modified Blakely-Sloan */
    register int i,carry;
    register mr_small q=0,r=0;
    r=a;
    for (i=MIRACL/4;i>0;i--)
    { /* do it bit by bit */
        carry=0;
        if ((mr_utype)r<0) carry=1;
        r<<=1;
        if ((mr_utype)c<0) r++;
        c<<=1;
        q<<=1;
        if (carry || r>=m) { r-=m; q++; }
        carry=0;
        if ((mr_utype)r<0) carry=1;
        r<<=1;
        if ((mr_utype)c<0) r++;
        c<<=1;
        q<<=1;
        if (carry || r>=m) { r-=m; q++; }
        carry=0;
        if ((mr_utype)r<0) carry=1;
        r<<=1;
        if ((mr_utype)c<0) r++;
        c<<=1;
        q<<=1;
        if (carry || r>=m) { r-=m; q++; }
        carry=0;
        if ((mr_utype)r<0) carry=1;
        r<<=1;
        if ((mr_utype)c<0) r++;
        c<<=1;
        q<<=1;
        if (carry || r>=m) { r-=m; q++; }
    }
    *rp=r;
    return q;
}

#ifndef MR_NOFULLWIDTH

#ifdef MR_NO_INTRINSICS

/* define mr_hltype as that C type that is half the size in bits of the 
   underlying type (mr_utype in mirdef.h). Perhaps short if mr_utype is long? 
   Possible int if mr_utype is 64-bit long long ?? */

#define mr_hltype int

mr_small muldvd(mr_small a,mr_small b,mr_small c,mr_small *rp)
{ /* multiply by parts */
    mr_small middle,middle2;
    mr_small q,r;
    unsigned mr_hltype am,al,bm,bl;
    int hshift=(MIRACL>>1);
    am=(unsigned mr_hltype)(a>>hshift);
    al=(unsigned mr_hltype)a;
    bm=(unsigned mr_hltype)(b>>hshift);
    bl=(unsigned mr_hltype)b;
/* form partial products */
    r= (mr_small)al*bl;  
    q= (mr_small)am*bm;
    middle=(mr_small)al*bm;
    middle2=(mr_small)bl*am;
    middle+=middle2;                        /* combine them - carefully */
    if (middle<middle2) q+=((mr_small)1<<hshift);
    r+=(middle << hshift);
    if ((r>>hshift)<(unsigned mr_hltype)middle) q++;
    q+=(middle>>hshift);
    r+=c;
    if (r<c) q++;
    *rp=r;
    return q;
}   

void muldvd2(mr_small a,mr_small b,mr_small *c,mr_small *rp)
{ /* multiply by parts */
    mr_small middle,middle2;
    mr_small q,r;
    unsigned mr_hltype am,al,bm,bl;
    int hshift=(MIRACL>>1);
    am=(unsigned mr_hltype)(a>>hshift);
    al=(unsigned mr_hltype)a;
    bm=(unsigned mr_hltype)(b>>hshift);
    bl=(unsigned mr_hltype)b;
/* form partial products */
    r= (mr_small)al*bl;  
    q= (mr_small)am*bm;
    middle=(mr_small)al*bm;
    middle2=(mr_small)bl*am;
    middle+=middle2;                        /* combine them - carefully */
    if (middle<middle2) q+=((mr_small)1<<hshift);
    r+=(middle << hshift);
    if ((r>>hshift)<(unsigned mr_hltype)middle) q++;
    q+=(middle>>hshift);
    r+=*c;
    if (r<*c) q++;
    r+=*rp;
    if (r<*rp) q++;
    *rp=r;
    *c=q;
}   


#endif

/* These are now in-lined - see miracl.h */

/*
mr_small muldvd(mr_small a,mr_small b,mr_small c,mr_small *rp)
{
    mr_small q,r;
    r=_umul128(a,b,&q);
    r+=c;
    q+=(r<c);
    *rp=r;
    return q;
}

void muldvd2(mr_small a,mr_small b,mr_small *c,mr_small *rp)
{
    mr_small q,r;
    r=_umul128(a,b,&q);
    r+=*c;
    q+=(r<*c);
    r+=*rp;
    q+=(r<*rp);
    *rp=r;
    *c=q;
}
*/
#endif

