/*
 * secCompMultiParty.h
 *
 *  Created on: Mar 31, 2015
 *      Author: froike (Roi Inbar) 
 * 	Modified: Aner Ben-Efraim
 * 
 */



#ifndef SECCOMPMULTIPARTY_H_
#define SECCOMPMULTIPARTY_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef ENABLE_SSE
#include <emmintrin.h>
#else
#include "util/block.h"
#endif

#include <iomanip>  //cout
#include <iostream>
#include "util/aes.h"
#include "util/TedKrovetzAesNiWrapperC.h"
#include  "src/primihub/protocol/falcon-public/globals.h"

#ifndef ENABLE_SSE
#include "util/block.h"
#endif
namespace primihub{
    namespace falcon
{
void XORvectors(__m128i *vec1, __m128i *vec2, __m128i *out, int length);

//creates a cryptographic(pseudo)random 128bit number
__m128i LoadSeedNew();

bool LoadBool();

void initializeRandomness(char* key, int numOfParties);

int getrCounter();
}// namespace primihub{
}
#endif /* SECCOMPMULTIPARTY_H_ */
