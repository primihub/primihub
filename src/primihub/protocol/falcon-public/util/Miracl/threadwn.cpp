/*                    

  Example program to illustrate MIRACL multi-threading with MS Windows
  Make sure miracl library is built with MR_WINDOWS_MT defined in mirdef.h

  Microsoft C++ compiler

  cl /O2 /GX /c /MT threadwn.cpp
  cl /O2 /GX /c /MT big.cpp
  cl /O2 /GX /c /MT zzn.cpp
  link /NODEFAULTLIB:libc.lib threadwn.obj big.obj zzn.obj miracl.lib

  Runs from the command prompt

  First thread factors one number, while second thread factors a second
  Each thread is initialised differently
  One outputs factors in decimal, the other in Hex
  The work of the threads are inter-leaved.

*/

#include <iostream>
#include <iomanip>
#include <windows.h>
#include <process.h> 
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

unsigned int __stdcall first(LPVOID lpvx)
{ // factors in Hex!
    Miracl precision(50,0);
    brent(1,"1111111111111111111111111111111111",16);
    return 0;
}

unsigned int __stdcall second(LPVOID lpvx)
{ // factors in decimal!
    Miracl precision(500,10);
    brent(2,"100000000000000000000000000000000013",10);  // find factors
    return 0;
}

int main()
{
    HANDLE thread1,thread2;
    unsigned int ID1,ID2;

    mr_init_threading();   // initialize MIRACL for multi-threading

    thread1=(HANDLE)_beginthreadex(NULL,0,first,(LPVOID)NULL,0,&ID1);
    thread2=(HANDLE)_beginthreadex(NULL,0,second,(LPVOID)NULL,0,&ID2);

    WaitForSingleObject(thread1,INFINITE);   // wait for threads to finish
    WaitForSingleObject(thread2,INFINITE);

    CloseHandle(thread1);
    CloseHandle(thread2);

    mr_end_threading();
   
    return 0;
}

