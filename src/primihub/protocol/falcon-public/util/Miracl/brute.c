/*
 *   Program to factor numbers using brute force.
 */

#include <stdio.h>
#include <string.h>
#include "miracl.h"
#define LIMIT 15000

int main()
{ /* find factors by brute force division */
    int n,p;

#ifdef MR_NOFULLWIDTH
    miracl *mip=mirsys(50,MAXBASE);
#else  
    miracl *mip=mirsys(50,0);
#endif

    char stack_mem[mr_big_reserve(2,50)];  // 2 bigs, 4 bytes per big...
    big x,y;
       
    memset(stack_mem,0,mr_big_reserve(2,50));
    x=mirvar_mem(stack_mem,0);
    y=mirvar_mem(stack_mem,1);

/* This is an example of allocating bigs x and y from the stack rather than 
   from the heap. The "50" should be the same as that used in the prior call 
   to mirsys(.), and MUST be a constant. There is never a need to explicitly 
   delete bigs allocated from the stack, via a call to mirkill(). Therefore
   do not use for sensitive data as these bigs are not erased, unless you
   do it yourself via a call to zero(.) 

   Allocation from the stack is not particularly recommended, but could be 
   faster if bigs are not too big, and are required as temporary variables
   in an often called subroutine. Allocating from the stack also avoids the 
   possibility of memory leaks. 

   NOTE: A big allocated from the stack has a random initial value

*/
    gprime(LIMIT);
    printf("input number to be factored\n");
    cinnum(x,stdin);
    if (isprime(x))
    {
        printf("this number is prime!\n");
        return 0;
    }
    n=0;
    p=mip->PRIMES[0];
    printf("factors are \n");
    forever
    { /* try division by each prime in turn */
        if (subdiv(x,p,y)==0)
        { /* factor found */
            copy(y,x);
            printf("prime factor     ");
            printf("%d\n",p);
            if (size(x)==1) return 0;
            continue;
        }
        if (size(y)<=p) 
        { /* must be prime */
            printf("prime factor     ");
            cotnum(x,stdout);
            return 0;
        }
        p=mip->PRIMES[++n];
        if (p==0) break;
    }
    if (isprime(x)) printf("prime factor     ");
    else            printf("composite factor ");
    cotnum(x,stdout);
    return 0;
}

