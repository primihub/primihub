/*
* secCompMultiParty.c
*
*  Created on: Mar 31, 2015
*      Author: froike (Roi Inbar) 
*      Modified: Aner Ben-Efraim
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "secCompMultiParty.h"
#include <emmintrin.h>
#include <smmintrin.h>
#include <stdint.h>

__m128i pseudoRandomString[RANDOM_COMPUTE];
__m128i tempSecComp[RANDOM_COMPUTE];

AES_KEY_TED aes_key;

unsigned long rCounter;

using namespace std;

//XORs 2 vectors
void XORvectors(__m128i *vec1, __m128i *vec2, __m128i *out, int length)
{
	for (int p = 0; p < length; p++)
	{
		out[p] = _mm_xor_si128(vec1[p], vec2[p]);
	}
}

//creates a cryptographic(pseudo)random 128bit number
__m128i LoadSeedNew()
{
	rCounter++;
	if (rCounter % RANDOM_COMPUTE == 0)//generate more random seeds
	{
		for (int i = 0; i < RANDOM_COMPUTE; i++)
			tempSecComp[i] = _mm_set1_epi32(rCounter+i);//not exactly counter mode - (rcounter+i,rcouter+i,rcounter+i,rcounter+i)
		AES_ecb_encrypt_chunk_in_out(tempSecComp, pseudoRandomString, RANDOM_COMPUTE, &aes_key);
	}
	return pseudoRandomString[rCounter%RANDOM_COMPUTE];
}

bool LoadBool()
{
	__m128i mask = _mm_setr_epi32(0,0,0,1);
	__m128i t = _mm_and_si128(LoadSeedNew(), mask);
	bool ans = !_mm_testz_si128(t,t);
	return ans;
}

void initializeRandomness(char* key, int numOfParties)
{
#ifdef FIXED_KEY_AES
	AES_set_encrypt_key((unsigned char*)FIXED_KEY_AES, 128, &fixed_aes_key);
#endif
	AES_init(numOfParties);
	AES_set_encrypt_key((unsigned char*)key, 256, &aes_key);
	rCounter = -1;
}

__m128i AES(__m128i input)
{
	return AES_ecb_encrypt_for_1(input, &fixed_aes_key);
}

int getrCounter()
{
	return rCounter;
}
