/*
 *    MIRACL  C++ Header file ebrick.h
 *
 *    AUTHOR  : M. Scott
 *  
 *    PURPOSE : Definition of class EBrick  
 *              Brickell et al's method for fast exponentiation with 
 *              precomputation - elliptic curve version GF(p)
 *    NOTE    : Must be used in conjunction with big.cpp
 *                
 */

#ifndef EBRICK_H
#define EBRICK_H

#include "big.h"

class EBrick 
{ 
    BOOL created;
    ebrick B;
public:
    EBrick(Big x,Big y,Big a,Big b,Big n,int window,int nb) 
    {ebrick_init(&B,x.getbig(),y.getbig(),a.getbig(),b.getbig(),n.getbig(),window,nb);
     created=TRUE;}
    
    EBrick(ebrick *b) {B=*b; created=FALSE;}  /* set structure */

    ebrick *get(void) {return &B;} /* get address of structure */

    int mul(Big &e,Big &x,Big &y) {int d=mul_brick(&B,e.getbig(),x.getbig(),y.getbig()); return d;}       

    ~EBrick() {if (created) ebrick_end(&B);}
};

#endif

