/*
 *    MIRACL  C++ Header file ebrick2.h
 *
 *    AUTHOR  : M. Scott
 *  
 *    PURPOSE : Definition of class EBrick2  
 *              Brickell et al's method for fast exponentiation with 
 *              precomputation - elliptic curve version GF(2^m)
 *    NOTE    : Must be used in conjunction with big.cpp
 */

#ifndef EBRICK2_H
#define EBRICK2_H

#include "big.h"

class EBrick2 
{ 
    BOOL created;
    ebrick2 B;
public:
    EBrick2(Big x,Big y,Big a2,Big a6,int m,int a,int b,int c,int window,int nb) 
    {ebrick2_init(&B,x.getbig(),y.getbig(),a2.getbig(),a6.getbig(),m,a,b,c,window,nb);
     created=TRUE;}
    
    EBrick2(ebrick2 *b) {B=*b; created=FALSE;}  /* set structure */

    ebrick2 *get(void) {return &B;} /* get address of structure */

    int mul(Big &e,Big &x,Big &y) {int d=mul2_brick(&B,e.getbig(),x.getbig(),y.getbig()); return d;}       

    ~EBrick2() {if (created) ebrick2_end(&B);}
};

#endif

