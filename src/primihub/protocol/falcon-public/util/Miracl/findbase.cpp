//
// cl /O2 /GX findbase.cpp gf2m.cpp big.cpp miracl.lib
//
// program to find "best" irreducible polynomial for GF(2^m)
// See http://eprint.iacr.org/2007/192
// cl /O2 /GX findbase.cpp gf2m.cpp big.cpp miracl.lib
// To generate the code for the reduction in a form for inclusion in 
// mrgf2m.c, function reduce2(.) use the irp.cpp utility
//

#include <iostream>
#include "gf2m.h"

using namespace std;

Miracl precision=100;

/* Max word size to be supported with irreducible polynomial */

#define MAX_WORD_SIZE MIRACL

/* define bit length of machine word - this can be changed here to 16 or whatever */

#define WORDLENGTH MAX_WORD_SIZE

/* insert costs of each operation in clock cycles */

#define XOR 1

/* cost of shifts (left and right) per shift size */


/* MSP430 Costs  MIRACL = 16 */

//int ls[16]={0,1,2,3,4,5,6,5,2,3,4,5,6,5,4,3};
//int rs[16]={0,2,3,4,5,6,7,5,2,3,4,5,6,5,4,3};


/* Atmeg128 Costs  MIRACL = 8 */

//int ls[8]={0,1,2,3,2,3,4,3};
//int rs[8]={0,1,2,3,2,3,4,3};


/* Pentium Costs MIRACL = 32 */

//int ls[32]={0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
//int rs[32]={0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};


/* x86-64 Costs MIRACL=64 */

int ls[64]={0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
int rs[64]={0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};


/* ARM Costs MIRACL = 32 */

//int ls[32]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
//int rs[32]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};


int priceit(int *s,int *d)
{ /* given the shifts and the cost per shift - put a price on it */
    int i,j,k,price,best;
    if (s[0]==0) return 0;
    
    price=d[s[1]];
    for (i=2;i<=s[0];i++)
    {
        best=s[i]-s[i-1];  /* a number of one-bit shifts is always an option */
        if (d[s[i]]< best) best=d[s[i]];
        for (j=1;j<=i-1;j++)
        { /* see is there a cheaper way */
            k=s[i]-s[j];
            if (d[k]<best) best=d[k];
        }
        price+=best;
    }
    return price;
}

void insert(int sh,int *sharray)
{ /* insert into sorted array */
    int i,j;
    if (sharray[0]==0)
    { /* first element */
        sharray[1]=sh;
        sharray[0]=1;
        return;
    }
    if (sh>=sharray[sharray[0]])
    { /* shift greatest so far - put it on the end */
        sharray[0]++;
        sharray[sharray[0]]=sh;
        return;
    }
/* insert */
    i=1;
    while (sh>sharray[i]) i++;
    
    for (j=sharray[0];j>=i;j--)
        sharray[j+1]=sharray[j];
    
    sharray[0]++;
    sharray[i]=sh;
    return;
}

int cost(int W,int M,int A,int B,int C,int *shifts)
{
    int i,j,len,rsh[4],lsh[4],ii;
    int xors,lshifts[5],rshifts[5];
    
    lshifts[0]=rshifts[0]=0;

    rsh[0]=M%W;
    lsh[0]=W-rsh[0];

    rsh[1]=(M-A)%W;
    lsh[1]=W-rsh[1];

    len=2;
    if (B)
    {
        len=4;
        rsh[2]=(M-B)%W;
        lsh[2]=W-rsh[2];
        rsh[3]=(M-C)%W;
        lsh[3]=W-rsh[3];
    }

    xors=0;
    for (i=0;i<len;i++)
    {
        if (rsh[i]==0 || rsh[i]==W) xors++;
        else
        {
            xors+=2;
            insert(rsh[i],rshifts);
            insert(lsh[i],lshifts);
        }
    }

    *shifts=priceit(lshifts,ls)+priceit(rshifts,rs);

    return xors;
}

BOOL irreducible(int m,int a,int b,int c)
{
    GF2m w4,w5,modulo;
    Big modulus;
    
    get_mip()->M=m;
    get_mip()->AA=a;
    get_mip()->BB=b;
    get_mip()->CC=c;

    if (b==0) modulus=pow((Big)2,m)+pow((Big)2,a)+1;
    else      modulus=pow((Big)2,m)+pow((Big)2,a)+pow((Big)2,b)+pow((Big)2,c)+1;
    copy(modulus.getbig(),get_mip()->modulus);
    copy(modulus.getbig(),getbig(modulo));

    w4=2;
    for (int i=1;i<=m/2;i++)
    {
        w4*=w4;
        w5=w4+2;    
        if (gcd(w5,modulo)!=1) return FALSE;
    }

    return TRUE;
}

int main(int argc,char **argv)
{
    int i,M,A,B,C,L,xors,shifts,price,bestprice;
    long cheapest,wcost;
    int CA,CB,CC;

    argc--; argv++;

    if (argc!=1 && argc!=2)
    {
        cout << "Bad Parameters" << endl;
        cout << "findbase <M>" << endl;
        cout << "finds irreducible polynomials for field F(2^M)" << endl;
        cout << "findbase <M> <L>" << endl;
        cout << "finds irreducible polynomials with cost less than or equal to L" << endl; 
        exit(0);
    }
 
    get_mip()->IOBASE=16;
    M=atoi(argv[0]);
    if (argc==2) L=atoi(argv[1]);
    else L=0;

    cheapest=1000000L;

    cout << "Looking for suitable trinomial basis x^M+x^A+1" << endl;

    for (A=M-MAX_WORD_SIZE;A>=1;A--)
    {
        if (A%2==0) continue;       // odds only
        if (irreducible(M,A,0,0))
        {
            xors=cost(WORDLENGTH,M,A,0,0,&shifts);
            price=xors+shifts;
            wcost=1000*price-A;
            if (wcost<cheapest)
            {
                cheapest=wcost;
                bestprice=price;
                CA=A;
                CB=CC=0;
            }
            if (L==0 || price<=L)
            {
                cout << "A= " << A;
                cout << " ";
                if ((M-A)%WORDLENGTH==0) cout << "*";
                if (((A+1)/2)%WORDLENGTH==0) cout << "+";
                cout << " cost= " << xors << "+" << shifts << " = " << price << endl;
            }
        }
    }

    cout << "Looking for suitable pentanomial basis x^M+x^A+x^B+x^C+1" << endl;

    for (A=M-MAX_WORD_SIZE;A>=1;A--)
    {
        if (A%2==0) continue;   // odds only
        for (B=A-1;B>=1;B--)
        {
            if (B%2==0) continue;   // odds only
            for (C=B-1;C>=1;C--)
            {
                if (C%2==0) continue;   // odds only
                if (irreducible(M,A,B,C))
                {     
                    xors=cost(WORDLENGTH,M,A,B,C,&shifts);
                    price=xors+shifts;
                    wcost=1000*price-C;
                    if (wcost<cheapest) 
                    {
                        cheapest=wcost;
                        bestprice=price;
                        CA=A;
                        CB=B;
                        CC=C;
                    }
                    if (L==0 || price<=L)
                    {
                        cout << "A= " << A << " B= " << B << " C= " << C;  
                        cout << " ";
                        if ((M-A)%WORDLENGTH==0) cout << "*";
                        if ((M-B)%WORDLENGTH==0) cout << "*";
                        if ((M-C)%WORDLENGTH==0) cout << "*";
                        if (((A+1)/2)%WORDLENGTH==0) cout << "+";
                        if (((B+1)/2)%WORDLENGTH==0) cout << "+";
                        if (((C+1)/2)%WORDLENGTH==0) cout << "+";
                        cout << " cost= " << xors << "+" << shifts << "=" << price << endl;
                    }
                }
            }
        }
    }
    if (cheapest==1000000L) return 0;
    
    cout << "cheapest= " << bestprice << endl;
    cout << "A= " << CA;
    if (CB)
        cout << " B= " << CB << " C= " << CC;
    cout << endl;

    return 0;
}
