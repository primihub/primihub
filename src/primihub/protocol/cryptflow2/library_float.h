/*
Authors: Anwesh Bhattacharya
Copyright:
Copyright (c) 2021 Microsoft Research
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef LIBRARY_FLOAT_H__
#define LIBRARY_FLOAT_H__

#include "defines_float.h"
#include "FloatingPoint/floating-point.h"
#include "FloatingPoint/fp-math.h"

using namespace std;
using namespace sci;

// Packs
extern IOPack *__iopack ;
extern OTPack *__otpack ;

// Operations
extern BoolOp *__bool_op ;		// bool
extern FixOp *__fix_op ;			// int
extern FPOp *__fp_op ;			// float
extern FPMath *__fp_math ;		// float math operations

// Floating point descriptors
extern int __m_bits ;				// mantissa bits
extern int __e_bits ;				// exponent bits

// Handy globals ;
extern int BATCH ;

// Output operations
extern BoolArray __bool_pub ;				// bool
extern FixArray __fix_pub ;				// int
extern FPArray __fp_pub ;					// float

/********************* Public Variables *********************/

// Initialization
void __init(int __argc, char **__argv) ;

float __get_comm() ;

/********************* Primitive Vectors *********************/

template<typename T>
vector<T> make_vector(size_t size) {
	return std::vector<T>(size) ;
}

template <typename T, typename... Args>
auto make_vector(size_t first, Args... sizes) {
	auto inner = make_vector<T>(sizes...) ;
	return vector<decltype(inner)>(first, inner) ;
}

/********************* Boolean Multidimensional Arrays *********************/

BoolArray __public_bool_to_boolean(uint8_t b, int party) ;

vector<BoolArray> make_vector_bool(int party, size_t last) ;

template <typename... Args>
auto make_vector_bool(int party, size_t first, Args... sizes) {
	auto _inner = make_vector_bool(party, sizes...) ;
	vector<decltype(_inner)> _ret ;
	_ret.push_back(_inner) ;
	for (size_t i = 1 ; i < first ; i++) {
		_ret.push_back(make_vector_bool(party, sizes...)) ;
	}
	return _ret ;
}

BoolArray __rand_bool(int party) ;

vector<BoolArray> make_vector_bool_rand(int party, size_t last) ;

template <typename... Args>
auto make_vector_bool_rand(int party, size_t first, Args... sizes) {
	auto _inner = make_vector_bool_rand(party, sizes...) ;
	vector<decltype(_inner)> _ret ;
	_ret.push_back(_inner) ;
	for (size_t i = 1 ; i < first ; i++) {
		_ret.push_back(make_vector_bool_rand(party, sizes...)) ;
	}
	return _ret ;
}

/********************* Integer Multidimensional Arrays *********************/

FixArray __public_int_to_arithmetic(uint64_t i, bool sign, int len, int party) ;

vector<FixArray> make_vector_int(int party, bool sign, int len, size_t last) ;

template <typename... Args>
auto make_vector_int(int party, bool sign, int len, size_t first, Args... sizes) {
	auto _inner = make_vector_int(party, sign, len, sizes...) ;
	vector<decltype(_inner)> _ret ;
	_ret.push_back(_inner) ;
	for (size_t i = 1 ; i < first ; i++) {
		_ret.push_back(make_vector_int(party, sign, len, sizes...)) ;
	}
	return _ret ;
}

/********************* Floating Multidimensional Arrays *********************/

FPArray __public_float_to_arithmetic(float f, int party) ;

vector<FPArray> make_vector_float(int party, size_t last) ;

template <typename... Args>
auto make_vector_float(int party, size_t first, Args... sizes) {
	auto _inner = make_vector_float(party, sizes...) ;
	vector<decltype(_inner)> _ret ;
	_ret.push_back(_inner) ;
	for (size_t i = 1 ; i < first ; i++) {
		_ret.push_back(make_vector_float(party, sizes...)) ;
	}
	return _ret ;
}


FPArray __rand_float(int party) ;

vector<FPArray> make_vector_float_rand(int party, size_t last) ;

template <typename... Args>
auto make_vector_float_rand(int party, size_t first, Args... sizes) {
	auto _inner = make_vector_float_rand(party, sizes...) ;
	vector<decltype(_inner)> _ret ;
	_ret.push_back(_inner) ;
	for (size_t i = 1 ; i < first ; i++) {
		_ret.push_back(make_vector_float_rand(party, sizes...)) ;
	}
	return _ret ;
}


void ElemWiseSub(int32_t s1, vector<FPArray>& arr1, vector<FPArray>& arr2, vector<FPArray>& outArr) ; 

void ElemWiseMul(int32_t s1, vector<FPArray>& arr1, vector<FPArray>& arr2, vector<FPArray>& outArr) ;

void getOutDer(int32_t s1, int32_t s2, vector<vector<FPArray>>& P, vector<vector<FPArray>>& Phat, vector<vector<FPArray>>& der) ;

void MatMul(int32_t m, int32_t n, int32_t p, 
	vector<vector<FPArray>> &A, 
	vector<vector<FPArray>> &B, 
	vector<vector<FPArray>> &C) ;

// use flip as true if 
void IfElse(
	int32_t s1, 
	vector<FPArray> &inArr,
	vector<BoolArray> &condArr, 
	vector<FPArray> &outArr, bool flip) ;

void ElemWiseAdd(int32_t s1, vector<FPArray>& arr1, vector<FPArray>& arr2, vector<FPArray>& outArr) ;

void GemmAdd(int32_t s1, int32_t s2, 
	vector<vector<FPArray>> &inArr, 
	vector<FPArray> &bias, 
	vector<vector<FPArray>> &outArr) ;

// hotArr is positive if input is negative
void Relu(
	int32_t s1, 
	vector<FPArray> &inArr, 
	vector<FPArray> &outArr,
	vector<BoolArray> &hotArr) ; 

void getBiasDer(int32_t m, int32_t s2, vector<vector<FPArray>>& batchSoftDer, vector<FPArray> &biasDer) ;

void updateWeights(int32_t sz, float lr, vector<FPArray>& inArr, vector<FPArray>& derArr) ; 

void Softmax2(
	int32_t s1, 
	int32_t s2, 
	vector<vector<FPArray>> &inArr, 
	vector<vector<FPArray>> &outArr) ; 

void Ln(int32_t s1, vector<FPArray>& inArr, vector<FPArray>& outArr) ; 

void dotProduct2(int32_t s1, int32_t s2, vector<vector<FPArray>>& arr1, vector<vector<FPArray>>& arr2, vector<FPArray>& outArr) ;

void getLoss(int32_t s, vector<FPArray>& arr, vector<FPArray>& outArr) ; 

void computeMSELoss(int32_t m, int32_t s, vector<vector<FPArray>>& target, vector<vector<FPArray>>& fwdOut, vector<FPArray>& loss) ;

#endif