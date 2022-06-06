
/* GCC inline assembly version for Linux */

#include "miracl.h"


mr_small muldiv(mr_small a,mr_small b,mr_small c,mr_small m,mr_small *rp)
{
    mr_small q;
    __asm__ __volatile__ (
    "movl %1,%%eax\n"
    "mull %2\n"
    "addl %3,%%eax\n"
    "adcl $0,%%edx\n"
    "divl %4\n"
    "movl %5,%%ebx\n"
    "movl %%edx,(%%ebx)\n"
    "movl %%eax,%0\n"
    : "=m"(q)
    : "m"(a),"m"(b),"m"(c),"m"(m),"m"(rp)
    : "eax","ebx","memory"
    );
    return q;
}

mr_small muldvm(mr_small a,mr_small c,mr_small m,mr_small *rp)
{
    mr_small q;
    __asm__ __volatile__ (
    "movl %1,%%edx\n"
    "movl %2,%%eax\n"
    "divl %3\n"
    "movl %4,%%ebx\n"
    "movl %%edx,(%%ebx)\n"
    "movl %%eax,%0\n"
    : "=m"(q)
    : "m"(a),"m"(c),"m"(m),"m"(rp)
    : "eax","ebx","memory"
    );        
    return q;
}

mr_small muldvd(mr_small a,mr_small b,mr_small c,mr_small *rp)
{
    mr_small q;
    __asm__ __volatile__ (
    "movl %1,%%eax\n"
    "mull %2\n"
    "addl %3,%%eax\n"
    "adcl $0,%%edx\n"
    "movl %4,%%ebx\n"
    "movl %%eax,(%%ebx)\n"
    "movl %%edx,%0\n"
    : "=m"(q)
    : "m"(a),"m"(b),"m"(c),"m"(rp)
    : "eax","ebx","memory"
    );
    return q;
}

void muldvd2(mr_small a,mr_small b,mr_small *c,mr_small *rp)
{
    __asm__ __volatile__ (
    "movl %0,%%eax\n"
    "mull %1\n"
    "movl %2,%%ebx\n"
    "addl (%%ebx),%%eax\n"
    "adcl $0,%%edx\n"
    "movl %3,%%esi\n"
    "addl (%%esi),%%eax\n"
    "adcl $0,%%edx\n"
    "movl %%eax,(%%esi)\n"
    "movl %%edx,(%%ebx)\n"
    : 
    : "m"(a),"m"(b),"m"(c),"m"(rp)
    : "eax","ebx","esi","memory"
    );
    
}

