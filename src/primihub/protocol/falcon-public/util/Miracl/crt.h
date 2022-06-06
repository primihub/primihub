/*
 *    MIRACL  C++ Header file crt.h
 *
 *    AUTHOR  : M. Scott
 *  
 *    PURPOSE : Definition of class Crt  (Chinese Remainder Thereom)
 *    NOTE    : Must be used in conjunction with big.cpp
 *              Can be used with either Big or utype moduli
 */

#ifndef CRT_H
#define CRT_H

#include "big.h"

#define MR_CRT_BIG   0
#define MR_CRT_SMALL 1

class Crt 
{ 
    big_chinese bc;
    small_chinese sc;
    int type;
public:
    Crt(int,Big *);
    Crt(int,mr_utype *);

    Big eval(Big *);       
    Big eval(mr_utype *);

    ~Crt() 
    {  /* destructor */
        if (type==MR_CRT_BIG) crt_end(&bc);
        if (type==MR_CRT_SMALL) scrt_end(&sc);
    }
};

#endif

