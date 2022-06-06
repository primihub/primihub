/********************************************************************/
/* Copyright(c) 2015.                                               */
/* Shay Gueron (1) (2)                                              */
/* (1) University of Haifa, Israel                                  */
/* (2) Intel, Israel                                                */
/*                                                                  */
/* This code is part of the SCAPI project. It was taken from code   */
/* written by Shay Gueron.                                          */
/*                                                                  */
/********************************************************************/
/*NOTE: several changes have been made to the original code by Shay Gueron*/

#include "main_gf_funcs.h"

using namespace std;

void THREE_GFMUL_naive(__m128i &A, __m128i &B, __m128i &C, __m128i &D, __m128i &E, __m128i &F, __m128i *HIGH, __m128i *LOW)
{
	__m128i tmp1_1, tmp1_2, tmp1_3, tmp1_4;
	__m128i tmp2_1, tmp2_2, tmp2_3, tmp2_4;
	__m128i tmp3_1, tmp3_2, tmp3_3, tmp3_4;
	__m128i acc1, acc2, acc3, acc4;
	
	tmp1_1 = _mm_clmulepi64_si128(A, B, 0x00); 
    tmp1_3 = _mm_clmulepi64_si128(A, B, 0x10);
    tmp1_2 = _mm_clmulepi64_si128(A, B, 0x01);
    tmp1_4 = _mm_clmulepi64_si128(A, B, 0x11);
	
	tmp2_1 = _mm_clmulepi64_si128(C, D, 0x00); 
    tmp2_3 = _mm_clmulepi64_si128(C, D, 0x10);
    tmp2_2 = _mm_clmulepi64_si128(C, D, 0x01);
    tmp2_4 = _mm_clmulepi64_si128(C, D, 0x11);
	
	tmp3_1 = _mm_clmulepi64_si128(E, F, 0x00); 
    tmp3_3 = _mm_clmulepi64_si128(E, F, 0x10);
    tmp3_2 = _mm_clmulepi64_si128(E, F, 0x01);
    tmp3_4 = _mm_clmulepi64_si128(E, F, 0x11);
		
	acc1 = _mm_xor_si128(tmp1_1, tmp2_1);
	acc2 = _mm_xor_si128(tmp1_2, tmp2_2);
	acc3 = _mm_xor_si128(tmp1_3, tmp2_3);
	acc4 = _mm_xor_si128(tmp1_4, tmp2_4);
	
	acc1 = _mm_xor_si128(acc1, tmp3_1);
	acc2 = _mm_xor_si128(acc2, tmp3_2);
	acc3 = _mm_xor_si128(acc3, tmp3_3);
	acc4 = _mm_xor_si128(acc4, tmp3_4);
	
	acc2 = _mm_xor_si128(acc2, acc3);
	acc3 = _mm_slli_si128(acc2, 8);
    acc2 = _mm_srli_si128(acc2, 8);
	
	*LOW = _mm_xor_si128(acc1, acc3);
	*HIGH = _mm_xor_si128(acc2, acc4);
}

void THREE_GFMUL_accumulated(__m128i &A, __m128i &B, __m128i &C, __m128i &D, __m128i &E, __m128i &F, __m128i *HIGH, __m128i *LOW)
{
	__m128i tmp1, tmp2, tmp3, tmp4;
	__m128i acc1, acc2, acc3, acc4;
	
	acc1 = _mm_clmulepi64_si128(A, B, 0x00); 
    acc2 = _mm_clmulepi64_si128(A, B, 0x10);
    acc3 = _mm_clmulepi64_si128(A, B, 0x01);
    acc4 = _mm_clmulepi64_si128(A, B, 0x11);
	
	tmp1 = _mm_clmulepi64_si128(C, D, 0x00); 
    tmp2 = _mm_clmulepi64_si128(C, D, 0x10);
    tmp3 = _mm_clmulepi64_si128(C, D, 0x01);
    tmp4 = _mm_clmulepi64_si128(C, D, 0x11);
	
	acc1 = _mm_xor_si128(acc1, tmp1);
	acc2 = _mm_xor_si128(acc2, tmp2);
	acc3 = _mm_xor_si128(acc3, tmp3);
	acc4 = _mm_xor_si128(acc4, tmp4);
	
	tmp1 = _mm_clmulepi64_si128(E, F, 0x00); 
    tmp2 = _mm_clmulepi64_si128(E, F, 0x10);
    tmp3 = _mm_clmulepi64_si128(E, F, 0x01);
    tmp4 = _mm_clmulepi64_si128(E, F, 0x11);
		
	acc1 = _mm_xor_si128(acc1, tmp1);
	acc2 = _mm_xor_si128(acc2, tmp2);
	acc3 = _mm_xor_si128(acc3, tmp3);
	acc4 = _mm_xor_si128(acc4, tmp4);
		
	acc2 = _mm_xor_si128(acc2, acc3);
	acc3 = _mm_slli_si128(acc2, 8);
    acc2 = _mm_srli_si128(acc2, 8);
	
	*LOW = _mm_xor_si128(acc1, acc3);
	*HIGH = _mm_xor_si128(acc2, acc4);
}

void gfmul3(__m128i A, __m128i B, __m128i *RES) {

	__m128i POLY = _mm_setr_epi32(0x87, 0, 0, 0);
	__m128i tmp1, tmp2, tmp3, tmp4;
	__m128i acc1, acc2, acc3, acc4;

	acc1 = _mm_clmulepi64_si128(A, B, 0x00);
	acc2 = _mm_clmulepi64_si128(A, B, 0x10);
	acc3 = _mm_clmulepi64_si128(A, B, 0x01);
	acc4 = _mm_clmulepi64_si128(A, B, 0x11);

	acc2 = _mm_xor_si128(acc2, acc3);
	acc3 = _mm_slli_si128(acc2, 8);
	acc2 = _mm_srli_si128(acc2, 8);

	acc1 = _mm_xor_si128(acc1, acc3);
	acc4 = _mm_xor_si128(acc2, acc4);

	//phase 1
	tmp3 = _mm_clmulepi64_si128(acc4, POLY, 0x01);

	tmp1 = _mm_slli_si128(tmp3, 8);
	tmp4 = _mm_srli_si128(tmp3, 8);

	tmp2 = _mm_xor_si128(acc4, tmp4);
	tmp3 = _mm_xor_si128(acc1, tmp1);

	//phase 2
	acc2 = _mm_clmulepi64_si128(tmp2, POLY, 0x00);
	*RES = _mm_xor_si128(tmp3, acc2);
}

void gfmul3HalfZeros(__m128i A, __m128i B, __m128i *RES) {

	__m128i POLY = _mm_setr_epi32(0x87, 0, 0, 0);
	__m128i tmp1, tmp2, tmp3, tmp4;
	__m128i acc1, acc2, acc3, acc4;

	acc1 = _mm_clmulepi64_si128(A, B, 0x00);
	acc2 = _mm_setr_epi32(0, 0, 0, 0);//_mm_clmulepi64_si128(A, B, 0x10);
	acc3 = _mm_clmulepi64_si128(A, B, 0x01);
	acc4 = _mm_setr_epi32(0, 0, 0, 0);//_mm_clmulepi64_si128(A, B, 0x11);

	acc2 = _mm_xor_si128(acc2, acc3);
	acc3 = _mm_slli_si128(acc2, 8);
	acc2 = _mm_srli_si128(acc2, 8);

	acc1 = _mm_xor_si128(acc1, acc3);
	acc4 = _mm_xor_si128(acc2, acc4);

	//phase 1
	tmp3 = _mm_clmulepi64_si128(acc4, POLY, 0x01);

	tmp1 = _mm_slli_si128(tmp3, 8);
	tmp4 = _mm_srli_si128(tmp3, 8);

	tmp2 = _mm_xor_si128(acc4, tmp4);
	tmp3 = _mm_xor_si128(acc1, tmp1);

	//phase 2
	acc2 = _mm_clmulepi64_si128(tmp2, POLY, 0x00);
	*RES = _mm_xor_si128(tmp3, acc2);
}

__m128i gfmul3(__m128i A, __m128i B)
{
	__m128i ans;
	gfmul3(A, B, &ans);
	return ans;
}

__m128i gfmul3HalfZeros(__m128i A, __m128i B)
{
	__m128i ans;
	gfmul3HalfZeros(A, B, &ans);
	return ans;
}

void THREE_GFMUL_accumulated_REDUCED(__m128i &A, __m128i &B, __m128i &C, __m128i &D, __m128i &E, __m128i &F, __m128i *RES){
	
	__m128i POLY = _mm_setr_epi32(0x87, 0, 0, 0);
	__m128i tmp1, tmp2, tmp3, tmp4;
	__m128i acc1, acc2, acc3, acc4;
	
	acc1 = _mm_clmulepi64_si128(A, B, 0x00); 
    acc2 = _mm_clmulepi64_si128(A, B, 0x10);
    acc3 = _mm_clmulepi64_si128(A, B, 0x01);
    acc4 = _mm_clmulepi64_si128(A, B, 0x11);
	
	tmp1 = _mm_clmulepi64_si128(C, D, 0x00); 
    tmp2 = _mm_clmulepi64_si128(C, D, 0x10);
    tmp3 = _mm_clmulepi64_si128(C, D, 0x01);
    tmp4 = _mm_clmulepi64_si128(C, D, 0x11);
	
	acc1 = _mm_xor_si128(acc1, tmp1);
	acc2 = _mm_xor_si128(acc2, tmp2);
	acc3 = _mm_xor_si128(acc3, tmp3);
	acc4 = _mm_xor_si128(acc4, tmp4);
	
	tmp1 = _mm_clmulepi64_si128(E, F, 0x00); 
    tmp2 = _mm_clmulepi64_si128(E, F, 0x10);
    tmp3 = _mm_clmulepi64_si128(E, F, 0x01);
    tmp4 = _mm_clmulepi64_si128(E, F, 0x11);
		
	acc1 = _mm_xor_si128(acc1, tmp1);
	acc2 = _mm_xor_si128(acc2, tmp2);
	acc3 = _mm_xor_si128(acc3, tmp3);
	acc4 = _mm_xor_si128(acc4, tmp4);
		
	acc2 = _mm_xor_si128(acc2, acc3);
	acc3 = _mm_slli_si128(acc2, 8);
    acc2 = _mm_srli_si128(acc2, 8);
	
	acc1 = _mm_xor_si128(acc1, acc3);
	acc4 = _mm_xor_si128(acc2, acc4);
	
	//phase 1
	tmp3 = _mm_clmulepi64_si128(acc4, POLY, 0x01);			
	
	tmp1  = _mm_slli_si128(tmp3, 8);					
	tmp4 = _mm_srli_si128(tmp3, 8);						
	
	tmp2 = _mm_xor_si128(acc4, tmp4);						
	tmp3 = _mm_xor_si128(acc1, tmp1);						

	//phase 2
	acc2 = _mm_clmulepi64_si128(tmp2, POLY, 0x00);			
	*RES = _mm_xor_si128(tmp3, acc2);		
}		

void REDUCE(__m128i high, __m128i low, __m128i *RES){
	
	__m128i POLY = _mm_setr_epi32(0x87, 0, 0, 0);			
	__m128i xmm4, xmm3, xmm2;
	__m128i left, right;
		
	//phase 1
	xmm3 = _mm_clmulepi64_si128(high, POLY, 0x01);			
	
	left  = _mm_slli_si128(xmm3, 8);					
	right = _mm_srli_si128(xmm3, 8);						
	
	xmm2 = _mm_xor_si128(high, right);						
	xmm3 = _mm_xor_si128(low, left);						

	//phase 2
	xmm4 = _mm_clmulepi64_si128(xmm2, POLY, 0x00);			
	*RES = _mm_xor_si128(xmm3, xmm4);						
}


//Dot product
void gfDotProductPiped(__m128i* vec1, __m128i* vec2, int length, __m128i* ans)
{
	__m128i POLY = _mm_setr_epi32(0x87, 0, 0, 0);
	__m128i tmp1, tmp2, tmp3, tmp4;
	__m128i acc1, acc2, acc3, acc4;

	acc1 = _mm_clmulepi64_si128(vec1[0], vec2[0], 0x00);
	acc2 = _mm_clmulepi64_si128(vec1[0], vec2[0], 0x10);
	acc3 = _mm_clmulepi64_si128(vec1[0], vec2[0], 0x01);
	acc4 = _mm_clmulepi64_si128(vec1[0], vec2[0], 0x11);

	for (int i = 1; i < length; i++)
	{
		tmp1 = _mm_clmulepi64_si128(vec1[i], vec2[i], 0x00);
		tmp2 = _mm_clmulepi64_si128(vec1[i], vec2[i], 0x10);
		tmp3 = _mm_clmulepi64_si128(vec1[i], vec2[i], 0x01);
		tmp4 = _mm_clmulepi64_si128(vec1[i], vec2[i], 0x11);

		acc1 = _mm_xor_si128(acc1, tmp1);
		acc2 = _mm_xor_si128(acc2, tmp2);
		acc3 = _mm_xor_si128(acc3, tmp3);
		acc4 = _mm_xor_si128(acc4, tmp4);
	}

	acc2 = _mm_xor_si128(acc2, acc3);
	acc3 = _mm_slli_si128(acc2, 8);
	acc2 = _mm_srli_si128(acc2, 8);

	acc1 = _mm_xor_si128(acc1, acc3);
	acc4 = _mm_xor_si128(acc2, acc4);

	//phase 1
	tmp3 = _mm_clmulepi64_si128(acc4, POLY, 0x01);

	tmp1 = _mm_slli_si128(tmp3, 8);
	tmp4 = _mm_srli_si128(tmp3, 8);

	tmp2 = _mm_xor_si128(acc4, tmp4);
	tmp3 = _mm_xor_si128(acc1, tmp1);

	//phase 2
	acc2 = _mm_clmulepi64_si128(tmp2, POLY, 0x00);
	ans[0]= _mm_xor_si128(tmp3, acc2);
}

void gfDotProductPipedHZ(__m128i* vec1, __m128i* vec2, int length, __m128i* ans)
{
	__m128i POLY = _mm_setr_epi32(0x87, 0, 0, 0);
	__m128i tmp1, tmp2, tmp3, tmp4;
	__m128i acc1, acc2, acc3, acc4;

	acc1 = _mm_clmulepi64_si128(vec1[0], vec2[0], 0x00);
	acc2 = _mm_setr_epi32(0, 0, 0, 0);
	acc3 = _mm_clmulepi64_si128(vec1[0], vec2[0], 0x01);
	acc4 = _mm_setr_epi32(0, 0, 0, 0);

	
	for (int i = 1; i < length; i++)
	{
		tmp1 = _mm_clmulepi64_si128(vec1[i], vec2[i], 0x00);
		
		tmp3 = _mm_clmulepi64_si128(vec1[i], vec2[i], 0x01);
		
		
		acc1 = _mm_xor_si128(acc1, tmp1);
		
		acc3 = _mm_xor_si128(acc3, tmp3);
		
	}

	acc2 = _mm_xor_si128(acc2, acc3);
	acc3 = _mm_slli_si128(acc2, 8);
	acc2 = _mm_srli_si128(acc2, 8);

	acc1 = _mm_xor_si128(acc1, acc3);
	acc4 = _mm_xor_si128(acc2, acc4);

	//phase 1
	tmp3 = _mm_clmulepi64_si128(acc4, POLY, 0x01);
	
	tmp1 = _mm_slli_si128(tmp3, 8);
	tmp4 = _mm_srli_si128(tmp3, 8);

	tmp2 = _mm_xor_si128(acc4, tmp4);
	tmp3 = _mm_xor_si128(acc1, tmp1);

	//phase 2
	acc2 = _mm_clmulepi64_si128(tmp2, POLY, 0x00);
	ans[0] = _mm_xor_si128(tmp3, acc2);
}

__m128i gfmulNew(__m128i A, __m128i B)
{
	__m128i tmp = { 0 };
	__m128i res;
	THREE_GFMUL_accumulated_REDUCED(A, B, tmp, tmp, tmp, tmp, &res);
	return res;
}



void Add_Pointwise_4_Multiplication(__m128i *A, __m128i *B, __m128i *C,
	__m128i *D, __m128i *E, __m128i *F, __m128i *G, __m128i *H,
	__m128i *RES0, __m128i *RES1, __m128i *RES2, __m128i *RES3) {

	__m128i POLY = _mm_setr_epi32(0x87, 0, 0, 0);
	__m128i tmp1, tmp2, tmp3, tmp4;
	__m128i acc1, acc2, acc3, acc4;

	acc1 = _mm_clmulepi64_si128(*A, *B, 0x00);
	acc2 = _mm_clmulepi64_si128(*A, *B, 0x10);
	acc3 = _mm_clmulepi64_si128(*A, *B, 0x01);
	acc4 = _mm_clmulepi64_si128(*A, *B, 0x11);

	acc2 = _mm_xor_si128(acc2, acc3);
	acc3 = _mm_slli_si128(acc2, 8);
	acc2 = _mm_srli_si128(acc2, 8);

	acc1 = _mm_xor_si128(acc1, acc3);
	acc4 = _mm_xor_si128(acc2, acc4);


	tmp3 = _mm_clmulepi64_si128(acc4, POLY, 0x01);

	tmp1 = _mm_slli_si128(tmp3, 8);
	tmp4 = _mm_srli_si128(tmp3, 8);

	tmp2 = _mm_xor_si128(acc4, tmp4);
	tmp3 = _mm_xor_si128(acc1, tmp1);
	*RES0 = _mm_xor_si128(tmp3, *RES0);
	acc2 = _mm_clmulepi64_si128(tmp2, POLY, 0x00);
	*RES0 = _mm_xor_si128(*RES0, acc2);


	acc1 = _mm_clmulepi64_si128(*C, *D, 0x00);
	acc2 = _mm_clmulepi64_si128(*C, *D, 0x10);
	acc3 = _mm_clmulepi64_si128(*C, *D, 0x01);
	acc4 = _mm_clmulepi64_si128(*C, *D, 0x11);

	acc2 = _mm_xor_si128(acc2, acc3);
	acc3 = _mm_slli_si128(acc2, 8);
	acc2 = _mm_srli_si128(acc2, 8);

	acc1 = _mm_xor_si128(acc1, acc3);
	acc4 = _mm_xor_si128(acc2, acc4);


	tmp3 = _mm_clmulepi64_si128(acc4, POLY, 0x01);

	tmp1 = _mm_slli_si128(tmp3, 8);
	tmp4 = _mm_srli_si128(tmp3, 8);

	tmp2 = _mm_xor_si128(acc4, tmp4);
	tmp3 = _mm_xor_si128(acc1, tmp1);
	*RES1 = _mm_xor_si128(tmp3, *RES1);
	acc2 = _mm_clmulepi64_si128(tmp2, POLY, 0x00);
	*RES1 = _mm_xor_si128(*RES1, acc2);


	acc1 = _mm_clmulepi64_si128(*E, *F, 0x00);
	acc2 = _mm_clmulepi64_si128(*E, *F, 0x10);
	acc3 = _mm_clmulepi64_si128(*E, *F, 0x01);
	acc4 = _mm_clmulepi64_si128(*E, *F, 0x11);

	acc2 = _mm_xor_si128(acc2, acc3);
	acc3 = _mm_slli_si128(acc2, 8);
	acc2 = _mm_srli_si128(acc2, 8);

	acc1 = _mm_xor_si128(acc1, acc3);
	acc4 = _mm_xor_si128(acc2, acc4);


	tmp3 = _mm_clmulepi64_si128(acc4, POLY, 0x01);

	tmp1 = _mm_slli_si128(tmp3, 8);
	tmp4 = _mm_srli_si128(tmp3, 8);

	tmp2 = _mm_xor_si128(acc4, tmp4);
	tmp3 = _mm_xor_si128(acc1, tmp1);
	*RES2 = _mm_xor_si128(tmp3, *RES2);
	acc2 = _mm_clmulepi64_si128(tmp2, POLY, 0x00);
	*RES2 = _mm_xor_si128(*RES2, acc2);

	acc1 = _mm_clmulepi64_si128(*G, *H, 0x00);
	acc2 = _mm_clmulepi64_si128(*G, *H, 0x10);
	acc3 = _mm_clmulepi64_si128(*G, *H, 0x01);
	acc4 = _mm_clmulepi64_si128(*G, *H, 0x11);

	acc2 = _mm_xor_si128(acc2, acc3);
	acc3 = _mm_slli_si128(acc2, 8);
	acc2 = _mm_srli_si128(acc2, 8);

	acc1 = _mm_xor_si128(acc1, acc3);
	acc4 = _mm_xor_si128(acc2, acc4);


	tmp3 = _mm_clmulepi64_si128(acc4, POLY, 0x01);

	tmp1 = _mm_slli_si128(tmp3, 8);
	tmp4 = _mm_srli_si128(tmp3, 8);

	tmp2 = _mm_xor_si128(acc4, tmp4);
	tmp3 = _mm_xor_si128(acc1, tmp1);
	*RES3 = _mm_xor_si128(tmp3, *RES3);
	acc2 = _mm_clmulepi64_si128(tmp2, POLY, 0x00);
	*RES3 = _mm_xor_si128(*RES3, acc2);
}

void Pointwise_vec_Multiplication(__m128i* vec1, __m128i* vec2,
	int length, __m128i* resVec) {

	__m128i POLY = _mm_setr_epi32(0x87, 0, 0, 0);
	__m128i tmp1, tmp2, tmp3, tmp4;
	__m128i acc1, acc2, acc3, acc4;
	for (int i = 0; i < length; i++)
	{
		acc1 = _mm_clmulepi64_si128(vec1[0], vec2[0], 0x00);
		acc2 = _mm_clmulepi64_si128(vec1[0], vec2[0], 0x10);
		acc3 = _mm_clmulepi64_si128(vec1[0], vec2[0], 0x01);
		acc4 = _mm_clmulepi64_si128(vec1[0], vec2[0], 0x11);

		acc2 = _mm_xor_si128(acc2, acc3);
		acc3 = _mm_slli_si128(acc2, 8);
		acc2 = _mm_srli_si128(acc2, 8);

		acc1 = _mm_xor_si128(acc1, acc3);
		acc4 = _mm_xor_si128(acc2, acc4);


		tmp3 = _mm_clmulepi64_si128(acc4, POLY, 0x01);

		tmp1 = _mm_slli_si128(tmp3, 8);
		tmp4 = _mm_srli_si128(tmp3, 8);

		tmp2 = _mm_xor_si128(acc4, tmp4);
		tmp3 = _mm_xor_si128(acc1, tmp1);

		acc2 = _mm_clmulepi64_si128(tmp2, POLY, 0x00);
		resVec[i] = _mm_xor_si128(tmp3, acc2);
	}
}

//PRINT Variants
void REDUCE_printable(__m128i high, __m128i low, __m128i *RES){
	
	__m128i POLY = _mm_setr_epi32(0x87, 0, 0, 0);			print_m128i_with_string_le("POLY                 =  ", POLY);
	__m128i xmm5, xmm4, xmm3, xmm2;
	__m128i left, right;
		
	//phase 1
	xmm3 = _mm_clmulepi64_si128(high, POLY, 0x01);			print_m128i_with_string_le("T = high*poly        =  ", xmm3);
	
	left  = _mm_slli_si128(xmm3, 8);						print_m128i_with_string_le("left  = T<<64        =  ", left);
	right = _mm_srli_si128(xmm3, 8);						print_m128i_with_string_le("right = T>>64        =  ", right);
	
	xmm2 = _mm_xor_si128(high, right);						print_m128i_with_string_le("A = high_xor_right   =  ", xmm2);
	xmm3 = _mm_xor_si128(low, left);						print_m128i_with_string_le("B = low_xor_left     =  ", xmm3);

	//phase 2
	xmm4 = _mm_clmulepi64_si128(xmm2, POLY, 0x00);			print_m128i_with_string_le("CLMUL = A*poly       =  ", xmm4);
	xmm5 = _mm_xor_si128(xmm3, xmm4);						print_m128i_with_string_le("REDUCE = CLMUL_xor_B =  ", xmm5);
	*RES = xmm5;				
}	

void print_m128i_with_string_le(char* string,__m128i data)
{
    unsigned char *pointer = (unsigned char*)&data;
    int i;
    printf("%-20s[",string);
    for (i=0; i<16; i++)
        printf("%02x",pointer[15-i]);
    printf("]\n");
}











