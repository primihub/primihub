/*                    

  Example program to illustrate MIRACL multi-threading with openMP
  Make sure miracl library is built with MR_OPENMP_MT defined in mirdef.h
  This allows multi-threading in C++. 

  For multithreading in C use MR_GENERIC_MT with openMP (but do not define MR_OPENMP_MT)

  Microsoft C++ compiler, VC2005 (for openMP support)

  Make sure to compile miracl modules with /openmp flag

  cl /O2 /GX /openmp threadmp.cpp big.cpp zzn.cpp miracl.lib
  
  Runs from the command prompt

  First thread factors one number, while second thread factors a second
  Each thread is initialised differently
  One outputs factors in decimal, the other in Hex
  The work of the threads are inter-leaved.

  NOTE: You really need a multi-core processor to appreciate this....

*/

#include <iostream>
#include <iomanip>
#include <omp.h> 
#include "big.h"
#include "zzn.h"

using namespace std;

// Brents factoring algorithm

#define mr_min(a,b) ((a) < (b)? (a) : (b))

int brent(int id,char * fred,int base)
{  /*  factoring program using Brents method */
    long k,r,i,m,iter;
    Big n,z;

    ZZn x,y,q,ys;
    n=fred;

    cout << "thread "<< id << " factoring " << n << endl;
    get_mip()->IOBASE=base;
    m=10L;
    r=1L;
    iter=0L;
    z=0;
    do
    {
        modulo(n);    // ZZn arithmetic done mod n
        y=z;          // convert back to ZZn (n has changed!)

        cout << "thread " << id << " iteration " << iter << endl;
        q=1;
        do
        {
            x=y;
            for (i=1L;i<=r;i++) y=(y*y+3);
            k=0;
            do
            {
                iter++;
                if (iter%10==0) cout << "thread " << id << " iteration " << iter << endl;
                ys=y;
                for (i=1L;i<=mr_min(m,r-k);i++)
                {   
                    y=(y*y+3);
                    q=((y-x)*q);
                }
                z=gcd(q,n);
                k+=m;
            } while (k<r && z==1);
            r*=2;
        } while (z==1);
        if (z==n) do 
        { 
            ys=(ys*ys+3);
            z=gcd(ys-x,n);
        } while (z==1);
        if (!prime(z))
             cout << "thread " << id << " composite factor ";
        else cout << "thread " << id << " prime factor     ";
        cout << z << endl;
        if (z==n) return 0;
        n/=z;
        z=y;          
    } while (!prime(n));      
    cout << "thread " << id << " prime factor     ";
    cout << n << endl;
    return 0;
}

int main()
{
    #pragma omp parallel sections
    {
        #pragma omp section 
        {
            Miracl precision(50,0);
            brent(1,"11111111111112111111111111111111",16);
        }
        #pragma omp section
        {
            Miracl precision(500,10);
            brent(2,"100000000000000000000000000000000013",10);
        }    
    }
  
    return 0;
}

