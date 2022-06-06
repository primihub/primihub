//
// Simple Program to transform an elliptic curve  
// to the simplified Weierstrass form.
//
// See "Elliptic Curve Public Key Cryptosystems", Menezes, 
// Kluwer Academic Publishers, Chapter 3
//
// Requires: big.cpp 
//
// cl /O2 trans.cpp big.cpp miracl.lib
// g++ -O2 trans.cpp big.cpp miracl.a -o trans
//
//


#include <iostream>
#include "big.h"

using namespace std;

Miracl precision=100;

int main(int argc,char **argv)
{
    Big az,a1,a2,a3,a4,a6,aw;
    Big t,t1,t2,t3,t4,a,b;
    Big delta,j,g;
    argc--; argv++;

    if (argc!=7)
    {
        cout << "Wrong number of parameters provided" << endl;
        cout << "trans a b c d e f g" << endl;
        cout << "Transforms elliptic curve from the general form" << endl;
        cout << "ay^2+bxy+cy=dx^3+ex^2+fx+g" << endl;
        cout << "to the simplified Weierstrass form" << endl;
        cout << "Y^2=X^3+AX+B" << endl; 
        exit(0); 
    }
    az=argv[0];
    a1=argv[1];
    a2=argv[4];
    a3=argv[2];
    a4=argv[5];
    a6=argv[6];
    aw=argv[3];

    if (aw==0 || az==0)
    {
        cout << "This is not an elliptic curve" << endl;
        return 0;
    }

    cout << "Transforming elliptic curve" << endl;
    if (az==1) cout << "y^2";
    else       cout << az << ".y^2";
    if (a1!=0)
    {
        if (a1>0) cout << "+";
        if (a1==-1) cout << "-xy";
        if (a1==1) cout << "xy";
        if (a1!=-1 && a1!=1) cout << a1 << ".xy";
    }
    if (a3!=0)
    {
        if (a3>0) cout << "+";
        if (a3==-1) cout << "-y"; 
        if (a3==1) cout << "y"; 
        if (a3!=-1 && a3!=1) cout << a3 << ".y";
    }
    if (aw==1) cout << " = x^3";
    else cout << " = " << aw << ".x^3";
    
    if (a2!=0)
    {
        if (a2>0) cout << "+";
        if (a2==-1) cout << "-x^2";
        if (a2==1) cout << "x^2";
        if (a2!=-1 && a2!=1) cout << a2 << ".x^2";
    }
    if (a4!=0)
    {
        if (a4>0) cout << "+";
        if (a4==-1) cout << "-x";  
        if (a4==1) cout << "x";
        if (a4!=-1 && a4!=1) cout << a4 << ".x";
    } 
    if (a6!=0)
    {
        if (a6>0) cout << "+";
        cout << a6;
    } 
    cout << endl;

/* transform az*y^2 to y^2 by simple substitution.... */

    if (az!=1)
    {
        aw*=az; a2*=az; a4*=az; a6*=az;
    }

/* transform aw*x^3 to x^3 by simple substitution.... */

    if (aw!=1)
    {
        a3*=aw; a4*=aw; a6*=(aw*aw);
    }

    t=4*a2+a1*a1;
    t1=3*t;
    t3=18*36*(a1*a3+2*a4);
    t4=9*36*36*(4*a6+a3*a3);

    a=t3-3*t1*t1;
    b=t4-t1*t3+2*t1*t1*t1;

    if (a%16==0 && b%64==0)       { a/=16;   b/=64;}
    if (a%81==0 && b%729==0)      { a/=81;   b/=729;}
    if (a%625==0 && b%15625==0)   { a/=625;  b/=15625;}
    if (a%2401==0 && b%117649==0) { a/=2401; b/=117649;} 

    cout << ".. to the simplified Weierstrass form" << endl;
    cout << "Y^2 = X^3"; 
    if (a!=0)
    {
        if (a>0) cout << "+";
        if (a==-1) cout << "-X"; 
        if (a==1) cout << "X";
        if (a!=-1 && a!=1) cout << a << ".X";
    }
    if (b!=0)
    {
        if (b>0) cout << "+";
        cout << b;
    } 
    cout << endl;

    delta=-16*(4*a*a*a+27*b*b);

    cout << "discriminant= " << delta << endl;

    if (delta==0) cout << "curve is singular" << endl;
    else
    {
        j=-1728*(4*a)*(4*a)*(4*a);
        g=gcd(j,delta);
        j/=g;
        delta/=g;
        if (delta<0)
        {
            j=-j; delta=-delta;
        }
        if (delta>1) cout << "j-invariant = " << j << "/" << delta << endl;
        else         cout << "j-invariant = " << j << endl;
    }
}

