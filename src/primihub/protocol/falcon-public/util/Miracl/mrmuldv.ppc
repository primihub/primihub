
/* GCC inline assembly version for Linux */

#include "miracl.h"

mr_small muldiv(a,b,c,m,rp)
mr_small a,b,c,m;
mr_small *rp;
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

mr_small muldvm(a,c,m,rp)
mr_small a,c,m;
mr_small *rp;
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

void muldvd2(mr_small a,mr_small b,mr_small *c,mr_small *rp)
{
    __asm__ __volatile__ (
    "mulld  %%r16,%0,%1\n"
    "mulhdu %%r17,%0,%1\n"
    "ld     %%r18,0(%2)\n" 
    "addc   %%r16,%%r18,%%r16\n"
    "addze  %%r17,%%r17\n"
    "ld     %%r19,0(%3)\n" 
    "addc   %%r16,%%r19,%%r16\n"
    "addze  %%r17,%%r17\n"
    "std    %%r16,0(%3)\n"
    "std    %%r17,0(%2)\n"
    : 
    : "r"(a),"r"(b),"r"(c),"r"(rp)
    : "r16","r17","r18","r19","memory"
    );
    
}

mr_small muldvd(mr_small a,mr_small b,mr_small c,mr_small *rp)
{
    mr_small q;
    __asm__ __volatile__ (
    "mulld  %%r16,%1,%2\n"
    "mulhdu %%r17,%1,%2\n"
    "addc   %%r16,%3,%%r16\n"
    "addze  %%r17,%%r17\n" 
    "std    %%r16,0(%4)\n" 
    "or     %0,%%r17,%%r17\n"
    : "=r"(q)
    : "r"(a),"r"(b),"r"(c),"r"(rp)
    : "r16","r17","memory"
    );
    return q;
}

/*
mr_small test(mr_small a,mr_small b,mr_small c,mr_small *rp)
{
    mr_small q;
    q=a*b+c+*rp;
    *rp=q;
    return q;
}

int main()
{
    mr_small a,b,c,q,r;


    printf("sizeof(mr_small)= %d\n",sizeof(mr_small));
    a=(mr_small)-1;
    b=(mr_small)-1;
    c=0;
    q=0;
    r=0;
    q=muldvd(a,b,c,&r);   
    muldvd2(a,b,&c,&r);  
    printf("c= %lx r= %lx\n",c,r);
}

*/
