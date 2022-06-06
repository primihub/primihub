/*
 *   Program to factor numbers using brute force.
 *
 *   Requires: big.cpp
 */

#include <iostream>
#include "big.h"

using namespace std;

#define LIMIT 10000

#ifndef MR_NOFULLWIDTH
Miracl precision=50;
#else
Miracl precision(50,MAXBASE);
#endif

int main()
{
    int n,p;
    Big x;
    miracl *mr=&precision;
    gprime(LIMIT); /* generate all primes < LIMIT */
    cout << "input number to be factored\n";
    cin >> x;
    n=0;
    p=mr->PRIMES[0];
    cout << "factors are ";
    forever
    { /* try division by each prime */
        if (x%p==0)
        { /* factor found */
            x/=p;
            cout << "\nprime factor     " << p << flush;
            if (x==1) return 0;
            continue;
        }
        if ((x/p)<=p)
        { /* must be prime */
            cout << "\nprime factor     " << x << "\n"; 
            return 0;
        }
        p=mr->PRIMES[++n];
        if (p==0) break;
    }
    if (prime(x)) cout << "\nprime factor     " << x << "\n";
    else          cout << "\ncomposite factor " << x << "\n";
    return 0;
}

