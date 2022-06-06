/* 

  Example program to illustrate MIRACL multi-threading with Unix
  tested with RedHat Linux 6.0

  GCC compiler
 
  1. Make sure MR_UNIX_MT is defined in mirdef.h
  2. Compile all MIRACL modules with -D_REENTRANT flag
  3. g++ -I. -D_REENTRANT -O2 threadux.cpp big.o zzn.o miracl.a -lpthread
     Don't forget the -lpthread flag to the linker
  

  Runs from the command prompt

  First thread factors one number, while second thread factors a second
  Each thread is initialised differently
  One outputs factors in decimal, the other in Hex
  The work of the threads are inter-leaved.

*/

#include <iostream>
#include <iomanip>
#include <pthread.h>
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

void  *first(void *lpvx)
{ // factors in Hex!
    int i;
    Miracl precision(50,0);
    brent(1,"1111111111111111111111111111111111",16);
    return NULL;
}

void *second(void *lpvx)
{ // factors in decimal!
    int i;
    Miracl precision(500,10);
    brent(2,"100000000000000000000000000000000013",10);  // find factors
    return NULL;
}

int main()
{
    pthread_t thread1,thread2;
    pthread_attr_t attributes;
    void *status;
    
    mr_init_threading();   // initialize MIRACL for multi-threading
    pthread_attr_init(&attributes); 

    pthread_create(&thread1,&attributes,first,(void *)NULL);
    pthread_create(&thread2,&attributes,second,(void *)NULL);

    pthread_join(thread1,&status);
    pthread_join(thread2,&status);
    
    pthread_attr_destroy(&attributes);
    pthread_cancel(thread1);
    pthread_cancel(thread2);
    pthread_detach(thread1);
    pthread_detach(thread2);

    mr_end_threading();
   
    return 0;
}

