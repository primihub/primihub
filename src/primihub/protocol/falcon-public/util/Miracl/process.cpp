//
// Program to pre-process the .raw file of modular polynomials
// to produce a .pol file, with coefficients reduced mod p
//
// .pol file format
// <modulus>,<prime>,<1st coef.>,<1st power of X>,<1st power of Y>,<2nd coeff>..
// Each polynomial ends with zero powers of X and Y
//
//

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>
#include "big.h"

using namespace std;

Miracl precision=1000;

// Code to parse formula in command line
// This code isn't mine, but its public domain
// Shamefully I forget the source
//
// NOTE: It may be necessary on some platforms to change the operators * and #
//

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

int main(int argc, char **argv)
{
    ofstream ofile;
    ifstream ifile;
    int ip,lp,nx,ny,max;
    Big p,c;
    BOOL gotP,gotI,gotO,dir;
    int Base;
    miracl *mip=&precision;
    set_io_buffer_size(2048);
    argv++; argc--;
    if (argc<1)
    {
        cout << "incorrect usage" << endl;
        cout << "Program pre-processes Modular Polynomials with respect" << endl;
        cout << "to a given prime modulus" << endl;
        cout << "process <prime modulus P> <-i input> <-o output>" << endl;
        cout << "OR" << endl;
        cout << "process <formula for P>   <-i input> <-o output> " << endl;
#if defined(unix)
        cout << "e.g. process -f 2^192-2^64-1 -i mueller.raw -o p192.pol" << endl;
#else
        cout << "e.g. process -f 2#192-2#64-1 -i mueller.raw -o p192.pol" << endl;
#endif
        cout << "processes the file mueller.raw to p192.pol for the given modulus" << endl;
        cout << "To input P in Hex, precede with -h" << endl;
        cout << "To search downwards for a prime, use flag -d" << endl;
        cout << "Use flag -m <max> to limit polynomials processed to those" << endl;
        cout << "associated with primes less than or equal to <max>" << endl;
        cout << "\nFreeware from Certivox, Dublin, Ireland" << endl;
        cout << "Full C++ source code and MIRACL multiprecision library available" << endl;
        cout << "email mscott@indigo.ie" << endl;
        return 0;
    }

    ip=0;
    gprime(1000);
    gotP=gotI=gotO=dir=FALSE;
    p=0;
    max=0;
    Base=10;
    while (ip<argc)
    { 
        if (!gotP && strcmp(argv[ip],"-f")==0)
        {
            ip++;
            if (!gotP && ip<argc)
            {

                ss=argv[ip++];
                tt=0;
                eval();
                p=tt;
                gotP=TRUE;
                continue;
            }
            else
            {
                cout << "Error in command line" << endl;
                return 0;
            }
        }
        if (!gotO && strcmp(argv[ip],"-o")==0)
        {
            ip++;
            if (ip<argc)
            {
                gotO=TRUE;
                ofile.open(argv[ip++]);
                continue;
            }
            else
            {
                cout << "Error in command line" << endl;
                return 0;
            }
        }
        if (!gotI && strcmp(argv[ip],"-i")==0)
        {
            ip++;
            if (ip<argc)
            {
                gotI=TRUE;

                ifile.open(argv[ip],ios::in);

                if (!ifile)
                {
                    cout << "Input file " << argv[ip] << " could not be opened" << endl;
                    return 0;
                }
                ip++;
                continue;
            }
            else
            {
                cout << "Error in command line" << endl;
                return 0;
            }
        }
        if (strcmp(argv[ip],"-d")==0)
        {
            ip++;
            dir=TRUE;
            continue;
        }
        if (strcmp(argv[ip],"-m")==0)
        {
            ip++;
            if (ip<argc)
            {
                max=atoi(argv[ip++]);
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
        if (!gotP)
        {
            mip->IOBASE=Base;
            p=argv[ip++];
            mip->IOBASE=10;
            gotP=TRUE;
            continue;
        }
        cout << "Error in command line" << endl;
        return 0;
    }
    if (!gotP || !gotI || !gotO)
    {
        cout << "Error in command line" << endl;
        return 0;
    }

    if (!prime(p))
    {
        int incr=0;
        cout << "That number is not prime!" << endl;
        if (dir)
        {
            cout << "Looking for next lower prime" << endl;
            p-=1; incr++;
            while (!prime(p)) { p-=1;  incr++; }
            cout << "Prime P = P-" << incr << endl;
        }
        else
        {
            cout << "Looking for next higher prime" << endl;
            p+=1; incr++;
            while (!prime(p)) { p+=1;  incr++; }
            cout << "Prime P = P+" << incr << endl;
        }
        cout << "Prime P = " << p << endl;
    }
    cout << "P mod 24 = " << p%24 << endl;
    cout << "P is " << bits(p) << " bits long" << endl;

    cout << "Prime     " << flush;   
    ofile << p << endl;
    forever
    {
        ifile >> lp;
        if (ifile.eof()) break;
        if (max>0 && lp>max) break;

        cout << "\b\b\b\b";
        cout << setw(4) << lp << flush;
        ofile << lp << endl;
        forever
        {
            ifile >> c >> nx >> ny;
  
   // reduce coefficients mod p
            c=c%p;

            ofile << c << endl;
            ofile << nx << endl;
            ofile << ny << endl;

            if (nx==0 && ny==0) break;
        }
    }
    cout << endl;
    return 0;
}

