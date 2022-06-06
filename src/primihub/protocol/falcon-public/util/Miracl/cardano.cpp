/* Solving cubic x^3+AX+B using Cardano's formula */
/* cl /O2 /GX cardona.cpp zzn2.cpp zzn.cpp big.cpp miracl.lib */

#include <iostream>
#include <ctime>
#include "zzn2.h"

using namespace std;

Miracl precision(50,0);

//
// Shanks method modified to find cube roots
//

ZZn2 shanks(ZZn2 n)
{
    int i,s;
    Big q,p=get_modulus();
    ZZn2 t,W,R,V;
    BOOL inv;

    if (pow(n,(p*p-1)/3)!=1)
    {
 //      cout << "Not a cubic residue" << endl;
       return (ZZn2)0;
    }

    W=randn2();
    while (pow(W,(p*p-1)/3)==1) W=randn2();
 
    s=0;
    q=p*p-1;
    while (q%3==0)
    {
        q/=3;
        s++;
    }
 
    if ((q+1)%3==0)
    {
        R=pow(n,(q+1)/3);
        inv=FALSE;
    }
    else
    {
        R=pow(n,(q-1)/3);
        inv=TRUE;
    }

    V=pow(W,q);

    forever
    {
        if (!inv) t=(R*R*R)/n;
        else      t=(R*R*R)*n;

        for (i=0;;i++ )
        {
            if (t==1) break;
            t=t*t*t;
        }
        if (i==0)
        {
            if (!inv) return R;
            else      return (ZZn2)1/R;
        }
        R=R*pow(V,pow((Big)3,s-i-1));
    }

}

int main(int argc, char *argv[])
{
    int i,j,lt,gt;
    Big p;
    ZZn x,A,B,D;
    ZZn2 r,r3,r1,r2,CD,cu;
    time_t seed;

    time(&seed);
    irand((long)seed);

//
// Generate a random prime, (not 1 mod 8)
//

    cout << "Generate a random prime and a random cubic, and try to solve it!" << endl;
    cout << "Solutions might be Complex" << endl << endl;


    p=rand(80,2);
    while (p%8==1 || !prime(p)) p+=1;

    cout << "p= " << p << endl;
    cout << "p%24= " << p%24 << endl;
    modulo(p);

// Find a cube root of unity

    do
    {
        cu=pow((ZZn2)randn2(),(p*p-1)/3);
    } while(cu==1);
//    cout << "cube root of unity= " << cu << endl;

    A=(ZZn)rand(p);
    B=(ZZn)rand(p);

// Generate random parameters

    cout << "Finding a root of x^3+AX+B mod p, where" << endl;
    cout << "A= " << A << endl;
    cout << "B= " << B << endl;

// Cardona's formula

    D=(B*B)/4 + (A*A*A)/27;

    CD=sqrt((ZZn2)D);  // Solution may be "complex"

    r1=(ZZn2)-B/2+CD; r2=(ZZn2)-B/2-CD;

    r1=shanks(r1);   // cube roots
    r2=shanks(r2);

    if (r1==0 || r2==0)
    {
        cout << "No roots exist" << endl;
        return 0;
    }

// search for "right" r2

    if (r1*r2!=-A/3)
        r2*=cu;

    if (r1*r2!=-A/3)
        r2*=cu;   
 
    r=r1+r2;

    cout << "root 1= " << r << endl;
    if (r*r*r+A*r+B!=0) cout << "Check failed" << endl;

// try next value for r1

    r1*=cu;

    if (r1*r2!=-A/3)
        r2*=cu;

    if (r1*r2!=-A/3)
        r2*=cu;

 
    r=r1+r2;

    cout << "root 2= " << r << endl;
    if (r*r*r+A*r+B!=0) cout << "Check failed" << endl;

    r1*=cu;

    if (r1*r2!=-A/3)
        r2*=cu;

    if (r1*r2!=-A/3)
        r2*=cu;

    r=r1+r2;

    cout << "root 3= " << r << endl;
    if (r*r*r+A*r+B!=0) cout << "Check failed" << endl;

    return 0;

}
