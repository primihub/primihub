/*
 *    MIRACL  C++ Header file brick.h
 *
 *    AUTHOR  : M. Scott
 *  
 *    PURPOSE : Definition of class Brick  
 *              Comb method for fast exponentiation with 
 *              precomputation
 *    NOTE    : Must be used in conjunction with big.cpp
 *                
 */

#ifndef BRICK_H
#define BRICK_H

#include "big.h"

class Brick 
{ 
    BOOL created;
    brick b;
public:
    Brick(Big g,Big n,int window,int nb) 
        {brick_init(&b,g.getbig(),n.getbig(),window,nb); created=TRUE;}

    Brick(brick *bb) { b=*bb; created=FALSE; }

    brick *get(void) {return &b;}

    Big pow(Big &e) {Big w; pow_brick(&b,e.getbig(),w.getbig()); return w;}       

    ~Brick() {if (created) brick_end(&b);}
};

#endif

