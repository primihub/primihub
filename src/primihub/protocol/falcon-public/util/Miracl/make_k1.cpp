#include <iostream>
#include <fstream>
#include "ecn.h"
#include "zzn.h"

using namespace std;

Miracl precision(36,0); 

int main()
{
    ofstream params("xk1.ecs");
    miracl *mip=&precision;
    Big q,p,c,lambda;
 
    lambda=pow((Big)2,80)+pow((Big)2,16);
    q=lambda*lambda+lambda+1;
/*
    forever
    {
        q+=1;
        if (!prime(q)) continue;
        if (jacobi((Big)-3,q)!=1) continue;
        break;
    }
*/
    c=pow((Big)2,351);
    if (c%2==1) c+=1;
   
    for (; ;c+=2)
    {
        p=3*q*q*c*c+1;
        if (p%8==1) continue;
        if (prime(p)) break;
    }

    cout << "bits(p)= " << bits(p) << endl;
    cout << "p%8= " << p%8 << endl;
    cout << "p%3= " << p%3 << endl;
    cout << "jacobi(-3,q)= " << jacobi((Big)-3,q) << endl;

    cout << "cm " << p << " -D3" << endl;

    mip->IOBASE=16;
    cout << "p= " << p << endl;
    cout << "c= " << c << endl;
    cout << "q= " << q << endl;

    params << "1024" << endl;
    params << p << endl;
    params << "0" << endl;
    params << "1" << endl;

    return 0;
}
