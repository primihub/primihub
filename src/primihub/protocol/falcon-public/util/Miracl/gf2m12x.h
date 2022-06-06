/*
 *    MIRACL C++ Headerfile GF2m12x.h
 *
 *    AUTHOR  : M. Scott
 *
 *    PURPOSE : Definition of class GF2m12x - Arithmetic over the extension 
 *              field GF(2^12m) - uses irreducible polynomial x^12+x^FA+x^FB+x^FC+1
 *
 *    NOTE    : The underlying field basis must be set by the modulo() routine
 *
 * WARNING: This class has been cobbled together for a specific use with
 * the MIRACL library. It is not complete, and may not work in other 
 * applications
 *
 */


#ifndef GF2m12x_H
#define GF2m12x_H

#include <iostream>
#include "gf2m.h"  

// set field and irreducible polynomial, undefine FB and FC for trinomials

#define FM 12
#define FA 5
//#define FB 5
//#define FC 1

class GF2m12x
{
    GF2m x[FM];
public:
    GF2m12x()                    {for (int i=0;i<FM;i++) x[i]=0; }
    GF2m12x(const GF2m12x & b)   {for (int i=0;i<FM;i++) x[i]=b.x[i]; }
    GF2m12x(int i)               {x[0]=i; }
    GF2m12x(const GF2m* a)       {for (int i=0;i<FM;i++) x[i]=a[i];}
    GF2m12x(const GF2m& a,const GF2m& b, const GF2m& c=0, const GF2m& d=0, const GF2m& e=0, const GF2m& f=0, 
    const GF2m& g=0, const GF2m& h=0, const GF2m& i=0, const GF2m& j=0, const GF2m& k=0, const GF2m& l=0)
    {x[0]=a; x[1]=b; x[2]=c; x[3]=d; x[4]=e; x[5]=f; x[6]=g; x[7]=h; x[8]=i; x[9]=j; x[10]=k; x[11]=l;} 

    GF2m12x(const Big& a) {x[0]=(GF2m)a;}

    void set(const GF2m* a)      {for (int i=0;i<FM;i++) x[i]=a[i];}
    void set(const GF2m& a)      {x[0]=a; for (int i=1;i<FM;i++) x[i]=0; }
    void set(const GF2m& a,const GF2m& b=0, const GF2m& c=0, const GF2m& d=0, const GF2m& e=0, const GF2m& f=0, 
    const GF2m& g=0, const GF2m& h=0, const GF2m& i=0, const GF2m& j=0, const GF2m& k=0, const GF2m& l=0)      
    {x[0]=a; x[1]=b; x[2]=c; x[3]=d; x[4]=e; x[5]=f; x[6]=g; x[7]=h; x[8]=i; x[9]=j; x[10]=k; x[11]=l;}

    void invert();

    void get(GF2m*);
    void get(GF2m&);

    void clear() {for (int i=0;i<FM;i++) x[i]=0; }
    int degree();

    BOOL iszero() const 
    {for (int i=0;i<FM;i++) if (!x[i].iszero()) return FALSE; return TRUE; } 

    BOOL isunity() const
    {if (!x[0].isone()) return FALSE; for (int i=1;i<FM;i++) if (!x[i].iszero()) return FALSE; return TRUE; } 

    GF2m12x& powq();

    GF2m12x& operator=(const GF2m12x& b)
        { for (int i=0;i<FM;i++) x[i]=b.x[i]; return *this; }
    GF2m12x& operator=(const GF2m& b)
        { x[0]=b; for (int i=1;i<FM;i++) x[i]=0; return *this; }
    GF2m12x& operator=(int b)
        { x[0]=b; for (int i=1;i<FM;i++) x[i]=0; return *this; }
    GF2m12x& operator+=(const GF2m12x& b) 
        { for (int i=0;i<FM;i++) x[i]+=b.x[i]; return *this; }
    GF2m12x& operator+=(const GF2m& b)
        {x[0]+=b; return *this; }
    GF2m12x& operator*=(const GF2m12x&);
    GF2m12x& operator*=(const GF2m&);
    GF2m12x& operator/=(const GF2m12x&);
    GF2m12x& operator/=(const GF2m&);

    friend GF2m12x operator+(const GF2m12x&,const GF2m12x&);
    friend GF2m12x operator+(const GF2m12x&,const GF2m&);
    friend GF2m12x operator+(const GF2m&,const GF2m12x&);
    
    friend GF2m12x operator*(const GF2m12x&,const GF2m12x&);
    friend GF2m12x operator*(const GF2m12x&,const GF2m&);
    friend GF2m12x operator*(const GF2m&,const GF2m12x&);
    friend GF2m12x operator/(const GF2m12x&,const GF2m12x&); 

    friend BOOL operator==(const GF2m12x& a,const GF2m12x& b)
    { for (int i=0;i<FM;i++) if (a.x[i]!=b.x[i]) return FALSE; return TRUE; }

    friend BOOL operator!=(const GF2m12x& a,const GF2m12x& b)
    { for (int i=0;i<FM;i++) if (a.x[i]==b.x[i]) return FALSE; return TRUE; }

    friend GF2m12x pow(const GF2m12x&,const Big&);
    
    friend GF2m12x randx12();
    
    friend ostream& operator<<(ostream&,const GF2m12x&);

    ~GF2m12x() {} ;
};

extern GF2m12x randx12();

#endif

