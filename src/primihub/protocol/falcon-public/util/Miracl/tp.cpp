//
// Agrawal, Kayal & Saxena Prime Prover (Conjecture 4)
//
// cl /O2 /GX tp.cpp polymod.cpp poly.cpp zzn.cpp big.cpp miracl.lib
//
// Note neat way of assigning polynomials via the dummy type Variable
//
// No known pseudoprimes - so a useful test to confirm primality.. and its very quick
//

#include <iostream>
#include "big.h"
#include "poly.h"
#include "polymod.h"

using namespace std;

Miracl precision=100;


// Code to parse formula in command line
// This code isn't mine, but its public domain
// Shamefully I forget the source
//
// NOTE: It may be necessary on some platforms to change the operators * and #

#if defined(unix)
#define TIMES '.'
#define RAISE '^'
#else
#define TIMES '*'
#define RAISE '#'
#endif

Big tt;
static char *ss;

void eval_power (Big& oldn,Big& n,char op)
{
        if (op) n=pow(oldn,toint(n));    // power(oldn,size(n),n,n);
}

void eval_product (Big& oldn,Big& n,char op)
{
        switch (op)
        {
        case TIMES:
                n*=oldn; 
                break;
        case '/':
                n=oldn/n;
                break;
        case '%':
                n=oldn%n;
        }
}

void eval_sum (Big& oldn,Big& n,char op)
{
        switch (op)
        {
        case '+':
                n+=oldn;
                break;
        case '-':
                n=oldn-n;
        }
}

void eval (void)
{
        Big oldn[3];
        Big n;
        int i;
        char oldop[3];
        char op;
        char minus;
        for (i=0;i<3;i++)
        {
            oldop[i]=0;
        }
LOOP:
        while (*ss==' ')
        ss++;
        if (*ss=='-')    /* Unary minus */
        {
        ss++;
        minus=1;
        }
        else
        minus=0;
        while (*ss==' ')
        ss++;
        if (*ss=='(' || *ss=='[' || *ss=='{')    /* Number is subexpression */
        {
        ss++;
        eval ();
        n=tt;
        }
        else            /* Number is decimal value */
        {
        for (i=0;ss[i]>='0' && ss[i]<='9';i++)
                ;
        if (!i)         /* No digits found */
        {
                cout <<  "Error - invalid number" << endl;
                exit (20);
        }
        op=ss[i];
        ss[i]=0;
        n=atoi(ss);
        ss+=i;
        *ss=op;
        }
        if (minus) n=-n;
        do
        op=*ss++;
        while (op==' ');
        if (op==0 || op==')' || op==']' || op=='}')
        {
        eval_power (oldn[2],n,oldop[2]);
        eval_product (oldn[1],n,oldop[1]);
        eval_sum (oldn[0],n,oldop[0]);
        tt=n;
        return;
        }
        else
        {
        if (op==RAISE)
        {
                eval_power (oldn[2],n,oldop[2]);
                oldn[2]=n;
                oldop[2]=RAISE;
        }
        else
        {
                if (op==TIMES || op=='/' || op=='%')
                {
                eval_power (oldn[2],n,oldop[2]);
                oldop[2]=0;
                eval_product (oldn[1],n,oldop[1]);
                oldn[1]=n;
                oldop[1]=op;
                }
                else
                {
                if (op=='+' || op=='-')
                {
                        eval_power (oldn[2],n,oldop[2]);
                        oldop[2]=0;
                        eval_product (oldn[1],n,oldop[1]);
                        oldop[1]=0;
                        eval_sum (oldn[0],n,oldop[0]);
                        oldn[0]=n;
                        oldop[0]=op;
                }
                else    /* Error - invalid operator */
                {
                        cout <<  "Error - invalid operator" << endl;
                        exit (20);
                }
                }
        }
        }
        goto LOOP;
}

int main(int argc,char **argv)
{
    Big n;
    int i,ip,r,Base;
    BOOL gotN;
    miracl*mip=&precision;

    argc--; argv++;
    if (argc<1)
    {
        cout << "Incorrect Usage" << endl;
        cout << "Program tests number for primality" << endl;
        cout << "using AKS algorithm, conjecture 4" << endl;
        cout << "tp <number N>" << endl;
        cout << "OR" << endl;
        cout << "tp -f <formula for N>" << endl;
#if defined(unix)
        cout << "e.g. tp -f 2^192-2^64-1" << endl;
#else
        cout << "e.g. tp -f 2#192-2#64-1" << endl;
#endif
        cout << "To input N in Hex, precede with -h" << endl;
        return 0;
    }

    ip=0;
    gotN=FALSE;
    gprime(10000);

// Interpret command line
    Base=10;
    while (ip<argc)
    {
        if (strcmp(argv[ip],"-f")==0)
        {
            ip++;
            if (!gotN && ip<argc)
            {

                ss=argv[ip++];
                tt=0;
                eval();
                n=tt;
                gotN=TRUE;
                continue;
            }
            else
            {
                cout << "Error in command line" << endl;
                return 0;
            }
        }

        if (strcmp(argv[ip],"-h")==0)
        {
            ip++;
            Base=16;
            continue;
        }
     
        if (!gotN) 
        {
            mip->IOBASE=Base;
            n=argv[ip++];
            mip->IOBASE=10;
            gotN=TRUE;
            continue;
        }
 
        cout << "Error in command line" << endl;
        return 0;
    }    

    if (!gotN)
    {
        cout << "Error in command line" << endl;
        return 0;
    }
    
    if (n==2)
    {
        cout << "PRIME" << endl;
        return 0;
    }
    if (small_factors(n))
    {
        cout << "COMPOSITE - has small factors" << endl;
        return 0;
    }

    if (perfect_power(n))
    {
        cout << "COMPOSITE - is a perfect power" << endl;
        return 0;
    }


    for (i=0;;i++)
    {
        r=mip->PRIMES[i];
        if ((n*n-1)%r!=0) break;
    }

    modulo(n);

    Variable x;
    Poly M=pow(x,r)-1;  // M=x^r-1

    setmod(M);

    PolyMod lhs,rhs;

    lhs=x-1;            // left-hand side
    lhs=pow(lhs,n);     // (x-1)^n mod M

    rhs=x;              // right-hand side
    rhs=pow(rhs,n)-1;   // x^n-1   mod M

    if (lhs==rhs) cout << "PRIME" << endl;
    else          cout << "COMPOSITE" << endl;

    return 0;
}

