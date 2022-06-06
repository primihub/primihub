#pragma once
#include <stdio.h> 
#include <chrono>
#include <iostream>
#include <wmmintrin.h>
#include <emmintrin.h>
#include <smmintrin.h>
#include <stdint.h>
#include <stdio.h>


void REDUCE_printable(__m128i high, __m128i low, __m128i *RES);
void print_m128i_with_string_le(char* string, __m128i data);

void gfmul3(__m128i A, __m128i B, __m128i * RES);

void gfmul3HalfZeros(__m128i A, __m128i B, __m128i * RES);

__m128i gfmul3(__m128i A, __m128i B);

__m128i gfmul3HalfZeros(__m128i A, __m128i B);
//Dot product
void gfDotProductPiped(__m128i* Vec1, __m128i* Vec2, int length, __m128i* ans);
//Should be used if all elements in vec2 are "small", i.e., upper half is zeros
void gfDotProductPipedHZ(__m128i* vec1, __m128i* vec2, int length, __m128i* ans);

//Add pointwise mutliplications RES1=RES1+(A*B),...,RES3=RES3+G*H
void Add_Pointwise_4_Multiplication(__m128i *A, __m128i *B, __m128i *C,
	__m128i *D, __m128i *E, __m128i *F, __m128i *G, __m128i *H,
	__m128i *RES0, __m128i *RES1, __m128i *RES2, __m128i *RES3);
void Pointwise_vec_Multiplication(__m128i* vec1, __m128i* vec2,	int length, __m128i* resVec);


__m128i gfmulNew(__m128i A, __m128i B);
