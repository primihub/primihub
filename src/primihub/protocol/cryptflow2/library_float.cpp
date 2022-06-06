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

#include "globals_float.h"
#include "library_float.h"

using namespace std ;
using namespace sci ;

#define WHICHPARTY tid&1?3-__party:__party

// Packs
IOPack *__iopack = nullptr ;		
OTPack *__otpack = nullptr ;

// Addressing stuff
string __address = "127.0.0.1" ;
int __port = 32000 ;

// Operations
BoolOp *__bool_op = nullptr ;		// bool
FixOp *__fix_op = nullptr ;			// int
FPOp *__fp_op = nullptr ;			// float
FPMath *__fp_math = nullptr ;		// float math operations

// Floating point descriptors
int __m_bits = 23 ;					// mantissa bits
int __e_bits = 8 ;					// exponent bits

// Other stuff from defines_float.h
int __nt = MAX_THREADS ;
int __party = 0 ;

// Handy globals for experiments
int BATCH = 1 ;

// Output operations
BoolArray __bool_pub ;				// bool
FixArray __fix_pub ;				// int
FPArray __fp_pub ;					// float

// Initialization
void __init(int __argc, char **__argv) {
	cout.precision(15) ;
	ArgMapping __amap ;

	__amap.arg("r", __party, "Role of party: ALICE/SERVER = 1; BOB/CLIENT = 2") ;
	__amap.arg("nt", __nt, "Number of threads") ;
	__amap.arg("mbits", __m_bits, "mantissa bits") ;
	__amap.arg("ebits", __e_bits, "exponent bits") ;
	__amap.arg("port", __port, "port") ;
	__amap.arg("add", __address, "address") ;
	__amap.arg("batch", BATCH, "batch size for experiments") ;
	__amap.parse(__argc, __argv);

    for (int i = 0; i < __nt ; i++) {
    	iopackArr[i] = new IOPack(__party, __port+i, __address) ;
    	if (i & 1) {
    		otpackArr[i] = new OTPack(iopackArr[i], 3-__party) ;
    	}
    	else {
    		otpackArr[i] = new OTPack(iopackArr[i], __party) ;
    	}
    }

    for (int i = 0 ; i < __nt ; i++) {
    	int pt ;
    	if (i & 1)
    		pt = 3 - __party ;
    	else
    		pt = __party ;

    	boolopArr[i] = new BoolOp(pt, iopackArr[i], otpackArr[i]) ;
    	fixopArr[i] = new FixOp(pt, iopackArr[i], otpackArr[i]) ;
    	fpopArr[i] = new FPOp(pt, iopackArr[i], otpackArr[i]) ;
    	fpmathArr[i] = new FPMath(pt, iopackArr[i], otpackArr[i]) ;
    }

    __iopack = iopackArr[0] ;
    __otpack = otpackArr[0] ;

    __bool_op = boolopArr[0] ;
    __fix_op = fixopArr[0] ;
    __fp_op = fpopArr[0] ;    
    __fp_math = fpmathArr[0] ;
}

float __get_comm() {
	float c = 0.0 ;
	for (int i = 0 ; i < __nt ; i++)
		c += (float)iopackArr[i]->get_comm() ;

	return c ;
}

BoolArray __public_bool_to_boolean(uint8_t b, int party=ALICE) {
	uint8_t *_dummy = new uint8_t[1] ;
	_dummy[0] = b ;
	BoolArray _ret = __bool_op->input(party, 1, _dummy) ;
	delete[] _dummy ;
	return _ret ;
}

FixArray __public_int_to_arithmetic(uint64_t i, bool sign, int len, int party=ALICE) {
	uint64_t *_dummy = new uint64_t[1] ;
	_dummy[0] = i ;
	FixArray _ret = __fix_op->input(party, 1, _dummy, sign, len, 0) ;
	delete[] _dummy ;
	return _ret ;
}

FPArray __public_float_to_arithmetic(float f, int party=ALICE) {
	float *_dummy = new float[1] ;
	_dummy[0] = f ;
	FPArray _ret = __fp_op->input<float>(party, 1, _dummy, __m_bits, __e_bits) ;
	delete[] _dummy ;
	return _ret ;
}

FPArray __rand_float(int party) {
	float *_dummy = new float[1] ;
	_dummy[0] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) ;
	FPArray _ret = __fp_op->input<float>(party, 1, _dummy, __m_bits, __e_bits) ;
	delete[] _dummy ;
	return _ret ;
}

BoolArray __rand_bool(int party) {
	bool b ;
	if (static_cast <float> (rand()) / static_cast <float> (RAND_MAX) > 0.5)
		b = true ;
	else
		b = false ;
	return __public_bool_to_boolean(b, party) ;
}

vector<BoolArray> make_vector_bool(int party, size_t last) {
	vector<BoolArray> _ret ;
	for (size_t i = 0 ; i < last ; i++) {
		_ret.push_back(__public_bool_to_boolean(false, party)) ;
	}
	return _ret ;
}


vector<FixArray> make_vector_int(int party, bool sign, int len, size_t last) {
	vector<FixArray> _ret ;
	for (size_t i = 0 ; i < last ; i++) {
		_ret.push_back(__public_int_to_arithmetic(0, sign, len, party)) ;
	}
	return _ret ;
}

vector<FPArray> make_vector_float(int party, size_t last) {
	vector<FPArray> _ret ;
	for (size_t i = 0 ; i < last ; i++) {
		_ret.push_back(__public_float_to_arithmetic(0.0, party)) ;
	}
	return _ret ;
}

vector<FPArray> make_vector_float_rand(int party, size_t last) {
	vector<FPArray> _ret ;
	for (size_t i = 0 ; i < last ; i++) {
		_ret.push_back(__rand_float(party)) ;
	}
	return _ret ;
}

vector<BoolArray> make_vector_bool_rand(int party, size_t last) {
	vector<BoolArray> _ret ;
	for (size_t i = 0 ; i < last ; i++) {
		_ret.push_back(__rand_bool(party)) ;
	}
	return _ret ;
}

vector<int> get_chunks(int items, int slots) {
	int allocated, chunk, remaining ;

	chunk = items/slots ;
	vector<int> ret(slots, chunk) ;

	allocated = chunk*slots ;
	remaining = items - allocated ;
	for (int i = 0 ; i < remaining ; i++)
		ret[i]++ ;

	return ret ;
}

tuple<BoolArray,BoolArray,FixArray,FixArray> get_components(int tid, const FPArray &x) {
  BoolArray x_s = boolopArr[tid]->input(x.party, x.size, x.s);
  BoolArray x_z = boolopArr[tid]->input(x.party, x.size, x.z);
  FixArray x_m = fixopArr[tid]->input(x.party, x.size, x.m, false, x.m_bits + 1, x.m_bits);
  FixArray x_e = fixopArr[tid]->input(x.party, x.size, x.e, true, x.e_bits + 2, 0);
  return make_tuple(x_s, x_z, x_m, x_e);
}

void ElemWiseSub_thread(
	int32_t tid, int32_t sz, int m_bits, int e_bits,
	uint8_t *arr1_s, uint8_t *arr1_z, uint64_t *arr1_m, uint64_t *arr1_e,
	uint8_t *arr2_s, uint8_t *arr2_z, uint64_t *arr2_m, uint64_t *arr2_e,
	uint8_t *out_s, uint8_t *out_z, uint64_t *out_m, uint64_t *out_e
	) {
	FPArray arr1_flat = fpopArr[tid]->input(WHICHPARTY, sz, arr1_s, arr1_z, arr1_m, arr1_e, m_bits, e_bits) ;
	FPArray arr2_flat = fpopArr[tid]->input(WHICHPARTY, sz, arr2_s, arr2_z, arr2_m, arr2_e, m_bits, e_bits) ;
	FPArray out = fpopArr[tid]->sub(arr1_flat, arr2_flat) ;

	memcpy(out_s, out.s, sz*sizeof(uint8_t)) ;
	memcpy(out_z, out.z, sz*sizeof(uint8_t)) ;
	memcpy(out_m, out.m, sz*sizeof(uint64_t)) ;
	memcpy(out_e, out.e, sz*sizeof(uint64_t)) ;
}

void ElemWiseSub(int32_t s1, vector<FPArray>& arr1, vector<FPArray>& arr2, vector<FPArray>& outArr) {
	int m_bits, e_bits ;
	m_bits = arr1[0].m_bits ;
	e_bits = arr1[0].e_bits ;

	uint8_t *arr1_s = new uint8_t[s1] ;
	uint8_t *arr1_z = new uint8_t[s1] ;
	uint64_t *arr1_m = new uint64_t[s1] ;
	uint64_t *arr1_e = new uint64_t[s1] ;
	for (int i = 0 ; i < s1 ; i++) {
		arr1_s[i] = arr1[i].s[0] ;
		arr1_z[i] = arr1[i].z[0] ;
		arr1_m[i] = arr1[i].m[0] ;
		arr1_e[i] = arr1[i].e[0] ;
	}

	uint8_t *arr2_s = new uint8_t[s1] ;
	uint8_t *arr2_z = new uint8_t[s1] ;
	uint64_t *arr2_m = new uint64_t[s1] ;
	uint64_t *arr2_e = new uint64_t[s1] ;
	for (int i = 0 ; i < s1 ; i++) {
		arr2_s[i] = arr2[i].s[0] ;
		arr2_z[i] = arr2[i].z[0] ;
		arr2_m[i] = arr2[i].m[0] ;
		arr2_e[i] = arr2[i].e[0] ;
	}

	uint8_t *out_s = new uint8_t[s1] ;
	uint8_t *out_z = new uint8_t[s1] ;
	uint64_t *out_m = new uint64_t[s1] ;
	uint64_t *out_e = new uint64_t[s1] ;

	vector<int> chunks = get_chunks(s1, __nt) ;
	thread threads[MAX_THREADS] ;
	int offset = 0 ;
	for (int i = 0 ; i < __nt ; i++) {
		if (chunks[i] > 0) {
			threads[i] = thread(ElemWiseSub_thread,
				i, chunks[i], m_bits, e_bits,
				arr1_s+offset, arr1_z+offset, arr1_m+offset, arr1_e+offset,
				arr2_s+offset, arr2_z+offset, arr2_m+offset, arr2_e+offset,
				out_s+offset, out_z+offset, out_m+offset, out_e+offset
			) ;
			offset += chunks[i] ;
		}
	}

	for (int i = 0 ; i < __nt ; i++) {
		if (chunks[i] > 0)
			threads[i].join() ;
	}
	
	for (int i = 0 ; i < s1 ; i++) {
		outArr[i].m_bits = m_bits ;
		outArr[i].e_bits = e_bits ;

		outArr[i].s[0] = out_s[i] ;
		outArr[i].z[0] = out_z[i] ;
		outArr[i].m[0] = out_m[i] ;
		outArr[i].e[0] = out_e[i] ;
	}

	delete[] arr1_s ; delete[] arr2_s ; delete[] out_s ;
	delete[] arr1_z ; delete[] arr2_z ; delete[] out_z ;
	delete[] arr1_m ; delete[] arr2_m ; delete[] out_m ;
	delete[] arr1_e ; delete[] arr2_e ; delete[] out_e ;
}

void ElemWiseMul_thread(
	int32_t tid, int32_t sz, int m_bits, int e_bits,
	uint8_t *arr1_s, uint8_t *arr1_z, uint64_t *arr1_m, uint64_t *arr1_e,
	uint8_t *arr2_s, uint8_t *arr2_z, uint64_t *arr2_m, uint64_t *arr2_e,
	uint8_t *out_s, uint8_t *out_z, uint64_t *out_m, uint64_t *out_e
	) {
	FPArray arr1_flat = fpopArr[tid]->input(WHICHPARTY, sz, arr1_s, arr1_z, arr1_m, arr1_e, m_bits, e_bits) ;
	FPArray arr2_flat = fpopArr[tid]->input(WHICHPARTY, sz, arr2_s, arr2_z, arr2_m, arr2_e, m_bits, e_bits) ;
	FPArray out = fpopArr[tid]->mul(arr1_flat, arr2_flat) ;

	memcpy(out_s, out.s, sz*sizeof(uint8_t)) ;
	memcpy(out_z, out.z, sz*sizeof(uint8_t)) ;
	memcpy(out_m, out.m, sz*sizeof(uint64_t)) ;
	memcpy(out_e, out.e, sz*sizeof(uint64_t)) ;
}

void ElemWiseMul(int32_t s1, vector<FPArray>& arr1, vector<FPArray>& arr2, vector<FPArray>& outArr) {
	int m_bits, e_bits ;
	m_bits = arr1[0].m_bits ;
	e_bits = arr1[0].e_bits ;

	uint8_t *arr1_s = new uint8_t[s1] ;
	uint8_t *arr1_z = new uint8_t[s1] ;
	uint64_t *arr1_m = new uint64_t[s1] ;
	uint64_t *arr1_e = new uint64_t[s1] ;
	for (int i = 0 ; i < s1 ; i++) {
		arr1_s[i] = arr1[i].s[0] ;
		arr1_z[i] = arr1[i].z[0] ;
		arr1_m[i] = arr1[i].m[0] ;
		arr1_e[i] = arr1[i].e[0] ;
	}

	uint8_t *arr2_s = new uint8_t[s1] ;
	uint8_t *arr2_z = new uint8_t[s1] ;
	uint64_t *arr2_m = new uint64_t[s1] ;
	uint64_t *arr2_e = new uint64_t[s1] ;
	for (int i = 0 ; i < s1 ; i++) {
		arr2_s[i] = arr2[i].s[0] ;
		arr2_z[i] = arr2[i].z[0] ;
		arr2_m[i] = arr2[i].m[0] ;
		arr2_e[i] = arr2[i].e[0] ;
	}

	uint8_t *out_s = new uint8_t[s1] ;
	uint8_t *out_z = new uint8_t[s1] ;
	uint64_t *out_m = new uint64_t[s1] ;
	uint64_t *out_e = new uint64_t[s1] ;

	vector<int> chunks = get_chunks(s1, __nt) ;
	thread threads[MAX_THREADS] ;
	int offset = 0 ;
	for (int i = 0 ; i < __nt ; i++) {
		if (chunks[i] > 0 ) {
			threads[i] = thread(ElemWiseMul_thread,
				i, chunks[i], m_bits, e_bits,
				arr1_s+offset, arr1_z+offset, arr1_m+offset, arr1_e+offset,
				arr2_s+offset, arr2_z+offset, arr2_m+offset, arr2_e+offset,
				out_s+offset, out_z+offset, out_m+offset, out_e+offset
			) ;
			offset += chunks[i] ;
		}
	}

	for (int i = 0 ; i < __nt ; i++) {
		if (chunks[i] > 0)
			threads[i].join() ;
	}
	
	for (int i = 0 ; i < s1 ; i++) {
		outArr[i].m_bits = m_bits ;
		outArr[i].e_bits = e_bits ;

		outArr[i].s[0] = out_s[i] ;
		outArr[i].z[0] = out_z[i] ;
		outArr[i].m[0] = out_m[i] ;
		outArr[i].e[0] = out_e[i] ;
	}

	delete[] arr1_s ; delete[] arr2_s ; delete[] out_s ;
	delete[] arr1_z ; delete[] arr2_z ; delete[] out_z ;
	delete[] arr1_m ; delete[] arr2_m ; delete[] out_m ;
	delete[] arr1_e ; delete[] arr2_e ; delete[] out_e ;
}

void getOutDer(int32_t s1, int32_t s2, vector<vector<FPArray>>& P, vector<vector<FPArray>>& Phat, vector<vector<FPArray>>& der) {
	int sz = s1*s2 ;
	int m_bits, e_bits ;
	m_bits = P[0][0].m_bits ;
	e_bits = P[0][0].e_bits ;

	vector<FPArray> flat1, flat2 ;
	vector<FPArray> flat3 = make_vector_float(ALICE, sz) ;

	for (int i = 0 ; i < s1 ; i++)
		for (int j = 0 ; j < s2 ; j++)
			flat1.push_back(P[i][j]) ;
	
	for (int i = 0 ; i < s1 ; i++)
		for (int j = 0 ; j < s2 ; j++)
			flat2.push_back(Phat[i][j]) ;

	ElemWiseSub(sz, flat1, flat2, flat3) ;

	vector<FPArray> divver = make_vector_float(ALICE, sz) ;
	for (int i = 0 ; i < sz ; i++)
		divver[i] = __fp_op->input<float>(PUBLIC, sz, (float)(1.0/s1), m_bits, e_bits) ;
	ElemWiseMul(sz, flat3, divver, flat3) ;

	for (int i = 0, k = 0 ; i < s1 ; i++) 
		for (int j = 0 ; j < s2 ; j++, k++)
			der[i][j] = flat3[k] ;
}

void MatMul_thread(
	int tid, int m_chunk, int n, int p, int m_bits, int e_bits, FPMatrix B,
	uint8_t *A_s, uint8_t *A_z, uint64_t *A_m, uint64_t *A_e,
	uint8_t *res_s, uint8_t *res_z, uint64_t *res_m, uint64_t *res_e
	) {

	FPMatrix A_chunk = fpopArr[tid]->input(WHICHPARTY, m_chunk, n, A_s, A_z, A_m, A_e, m_bits, e_bits) ;
	FPMatrix res = fpopArr[tid]->matrix_multiplication(A_chunk, B) ;
	
	memcpy(res_s, res.s, m_chunk*p*sizeof(uint8_t)) ;
	memcpy(res_z, res.z, m_chunk*p*sizeof(uint8_t)) ;
	memcpy(res_m, res.m, m_chunk*p*sizeof(uint64_t)) ;
	memcpy(res_e, res.e, m_chunk*p*sizeof(uint64_t)) ;
}

void MatMul(int32_t m, int32_t n, int32_t p, 
	vector<vector<FPArray>> &A, 
	vector<vector<FPArray>> &B, 
	vector<vector<FPArray>> &C) {

	if (m <= __nt && p > __nt) {
		vector<vector<FPArray>> BT = make_vector_float(ALICE, p, n) ;
		vector<vector<FPArray>> AT = make_vector_float(ALICE, n, m) ;
		vector<vector<FPArray>> CT = make_vector_float(ALICE, p, m) ;

		for (int i = 0 ; i < n ; i++) {
			for (int j = 0 ; j < m ; j++) {
				AT[i][j] = A[j][i] ;
			}
		}

		for (int i = 0 ; i < p ; i++)
			for (int j = 0 ; j < n ; j++)
				BT[i][j] = B[j][i] ;

		MatMul(p, n, m, BT, AT, CT) ;

		for (int i = 0 ; i < m ; i++)
			for (int j = 0 ; j < p ; j++)
				C[i][j] = CT[j][i] ;

		return ;
	} 

	int m_bits = A[0][0].m_bits ;
	int e_bits = A[0][0].e_bits ;

	uint8_t *A_s = new uint8_t[m*n] ;
	uint8_t *A_z = new uint8_t[m*n] ;
	uint64_t *A_m = new uint64_t[m*n] ;
	uint64_t *A_e = new uint64_t[m*n] ;
	for (int i = 0, k=0 ; i < m ; i++) {
		for (int j = 0 ; j < n ; j++, k++) {
			A_s[k] = A[i][j].s[0] ;
			A_z[k] = A[i][j].z[0] ;
			A_m[k] = A[i][j].m[0] ;
			A_e[k] = A[i][j].e[0] ;
		}
	}

	uint8_t *B_s = new uint8_t[n*p] ;
	uint8_t *B_z = new uint8_t[n*p] ;
	uint64_t *B_m = new uint64_t[n*p] ;
	uint64_t *B_e = new uint64_t[n*p] ;
	for (int i = 0, k = 0 ; i < n ; i++) {
		for (int j = 0 ; j < p ; j++, k++) {
			B_s[k] = B[i][j].s[0] ;
			B_z[k] = B[i][j].z[0] ;
			B_m[k] = B[i][j].m[0] ;
			B_e[k] = B[i][j].e[0] ;
		}
	}
	FPMatrix mat2 = __fp_op->input(__party, n, p, B_s, B_z, B_m, B_e, m_bits, e_bits) ;

	uint8_t *res_s = new uint8_t[m*p] ;
	uint8_t *res_z = new uint8_t[m*p] ;
	uint64_t *res_m = new uint64_t[m*p] ;
	uint64_t *res_e = new uint64_t[m*p] ;

	vector<int> chunks = get_chunks(m, __nt) ;
	thread threads[MAX_THREADS] ;
	int m_offset, A_offset, res_offset ;
	m_offset = A_offset = res_offset = 0 ;
	for (int i = 0 ; i < __nt ; i++) {
		if (chunks[i] > 0) {
			threads[i] = thread(MatMul_thread,
				i, chunks[i], n, p, m_bits, e_bits, mat2,
				A_s+A_offset, A_z+A_offset, A_m+A_offset, A_e+A_offset,
				res_s+res_offset, res_z+res_offset, res_m+res_offset, res_e+res_offset
			) ;

			m_offset += chunks[i] ;
			A_offset += chunks[i]*n ;
			res_offset += chunks[i]*p ;
		}
	}

	for (int i = 0 ; i < __nt ; i++) {
		if (chunks[i] > 0)
			threads[i].join() ;
	}

	for (int i = 0, k = 0 ; i < m ; i++) {
		for (int j = 0 ; j < p ; j++, k++) {
			C[i][j].m_bits = m_bits ;
			C[i][j].e_bits = e_bits ;

			C[i][j].s[0] = res_s[k] ;
			C[i][j].z[0] = res_z[k] ;
			C[i][j].m[0] = res_m[k] ;
			C[i][j].e[0] = res_e[k] ;
		}
	}

	delete[] A_s ; delete[] B_s ; delete[] res_s ;
	delete[] A_z ; delete[] B_z ; delete[] res_z ;
	delete[] A_m ; delete[] B_m ; delete[] res_m ;
	delete[] A_e ; delete[] B_e ; delete[] res_e ; 
}

void IfElse_thread(
	int tid, int sz, int m_bits, int e_bits,
	uint8_t *in_s, uint8_t *in_z, uint64_t *in_m, uint64_t *in_e, uint8_t *in_hot,
	uint8_t *out_s, uint8_t *out_z, uint64_t *out_m, uint64_t *out_e, bool flip=false
	) {

	FPArray in_flat = fpopArr[tid]->input(WHICHPARTY, sz, in_s, in_z, in_m, in_e, m_bits, e_bits) ;
	BoolArray cond_flat = boolopArr[tid]->input(WHICHPARTY, sz, in_hot) ;
	FPArray zero_flat = fpopArr[tid]->input<float>(ALICE, sz, (float)0.0, m_bits, e_bits) ;

	FPArray out_flat ;
	if (flip)
		out_flat = fpopArr[tid]->if_else(cond_flat, zero_flat, in_flat) ;
	else
		out_flat = fpopArr[tid]->if_else(cond_flat, in_flat, zero_flat) ;

	memcpy(out_s, out_flat.s, sz*sizeof(uint8_t)) ;
	memcpy(out_z, out_flat.z, sz*sizeof(uint8_t)) ;
	memcpy(out_m, out_flat.m, sz*sizeof(uint64_t)) ;
	memcpy(out_e, out_flat.e, sz*sizeof(uint64_t)) ;
}

void IfElse(
	int32_t s1, 
	vector<FPArray> &inArr,
	vector<BoolArray> &condArr, 
	vector<FPArray> &outArr, bool flip=false) {
	int m_bits, e_bits ;
	m_bits = inArr[0].m_bits ;
	e_bits = inArr[0].e_bits ;

	uint8_t *in_s = new uint8_t[s1] ;
	uint8_t *in_z = new uint8_t[s1] ;
	uint64_t *in_m = new uint64_t[s1] ;
	uint64_t *in_e = new uint64_t[s1] ;
	uint8_t *in_hot = new uint8_t[s1] ;
	for (int i = 0 ; i < s1 ; i++) {
		in_s[i] = inArr[i].s[0] ;
		in_z[i] = inArr[i].z[0] ;
		in_m[i] = inArr[i].m[0] ;
		in_e[i] = inArr[i].e[0] ;

		in_hot[i] = condArr[i].data[0] ;
	}

	uint8_t *out_s = new uint8_t[s1] ;
	uint8_t *out_z = new uint8_t[s1] ;
	uint64_t *out_m = new uint64_t[s1] ;
	uint64_t *out_e = new uint64_t[s1] ;

	vector<int> chunks = get_chunks(s1, __nt) ;
	thread threads[MAX_THREADS] ;
	int offset = 0 ;
	for (int i = 0 ; i < __nt ; i++) {
		if (chunks[i] > 0) {
			threads[i] = thread(IfElse_thread,
				i, chunks[i], m_bits, e_bits,
				in_s + offset, in_z + offset, in_m + offset, in_e + offset, in_hot+offset,
				out_s + offset, out_z + offset, out_m + offset, out_e + offset, flip
			) ;
			offset += chunks[i] ;
		}
	}

	for (int i = 0 ; i < __nt ; i++)
		if (chunks[i] > 0)
			threads[i].join() ;

	for (int i = 0 ; i < s1 ; i++) {
		outArr[i].m_bits = m_bits ;
		outArr[i].e_bits = e_bits ;

		outArr[i].s[0] = out_s[i] ;
		outArr[i].z[0] = out_z[i] ;
		outArr[i].m[0] = out_m[i] ;
		outArr[i].e[0] = out_e[i] ;
	}

	delete[] in_s ; delete[] out_s ;
	delete[] in_z ; delete[] out_z ;
	delete[] in_m ; delete[] out_m ;
	delete[] in_e ; delete[] out_e ;
	delete[] in_hot ;
}

void ElemWiseAdd_thread(
	int32_t tid, int32_t sz, int m_bits, int e_bits,
	uint8_t *arr1_s, uint8_t *arr1_z, uint64_t *arr1_m, uint64_t *arr1_e,
	uint8_t *arr2_s, uint8_t *arr2_z, uint64_t *arr2_m, uint64_t *arr2_e,
	uint8_t *out_s, uint8_t *out_z, uint64_t *out_m, uint64_t *out_e
	) {
	FPArray arr1_flat = fpopArr[tid]->input(WHICHPARTY, sz, arr1_s, arr1_z, arr1_m, arr1_e, m_bits, e_bits) ;
	FPArray arr2_flat = fpopArr[tid]->input(WHICHPARTY, sz, arr2_s, arr2_z, arr2_m, arr2_e, m_bits, e_bits) ;
	FPArray out = fpopArr[tid]->add(arr1_flat, arr2_flat) ;

	memcpy(out_s, out.s, sz*sizeof(uint8_t)) ;
	memcpy(out_z, out.z, sz*sizeof(uint8_t)) ;
	memcpy(out_m, out.m, sz*sizeof(uint64_t)) ;
	memcpy(out_e, out.e, sz*sizeof(uint64_t)) ;
}

void ElemWiseAdd(int32_t s1, vector<FPArray>& arr1, vector<FPArray>& arr2, vector<FPArray>& outArr) {
	int m_bits, e_bits ;
	m_bits = arr1[0].m_bits ;
	e_bits = arr1[0].e_bits ;

	uint8_t *arr1_s = new uint8_t[s1] ;
	uint8_t *arr1_z = new uint8_t[s1] ;
	uint64_t *arr1_m = new uint64_t[s1] ;
	uint64_t *arr1_e = new uint64_t[s1] ;
	for (int i = 0 ; i < s1 ; i++) {
		arr1_s[i] = arr1[i].s[0] ;
		arr1_z[i] = arr1[i].z[0] ;
		arr1_m[i] = arr1[i].m[0] ;
		arr1_e[i] = arr1[i].e[0] ;
	}

	uint8_t *arr2_s = new uint8_t[s1] ;
	uint8_t *arr2_z = new uint8_t[s1] ;
	uint64_t *arr2_m = new uint64_t[s1] ;
	uint64_t *arr2_e = new uint64_t[s1] ;
	for (int i = 0 ; i < s1 ; i++) {
		arr2_s[i] = arr2[i].s[0] ;
		arr2_z[i] = arr2[i].z[0] ;
		arr2_m[i] = arr2[i].m[0] ;
		arr2_e[i] = arr2[i].e[0] ;
	}

	uint8_t *out_s = new uint8_t[s1] ;
	uint8_t *out_z = new uint8_t[s1] ;
	uint64_t *out_m = new uint64_t[s1] ;
	uint64_t *out_e = new uint64_t[s1] ;


	vector<int> chunks = get_chunks(s1, __nt) ;
	thread threads[MAX_THREADS] ;
	int offset = 0 ;
	for (int i = 0 ; i < __nt ; i++) {
		if (chunks[i] > 0) {
			threads[i] = thread(ElemWiseAdd_thread,
				i, chunks[i], m_bits, e_bits,
				arr1_s+offset, arr1_z+offset, arr1_m+offset, arr1_e+offset,
				arr2_s+offset, arr2_z+offset, arr2_m+offset, arr2_e+offset,
				out_s+offset, out_z+offset, out_m+offset, out_e+offset
			) ;
			offset += chunks[i] ;
		}
	}

	for (int i = 0 ; i < __nt ; i++) {
		if (chunks[i] > 0)
			threads[i].join() ;
	}
	
	for (int i = 0 ; i < s1 ; i++) {
		outArr[i].m_bits = m_bits ;
		outArr[i].e_bits = e_bits ;

		outArr[i].s[0] = out_s[i] ;
		outArr[i].z[0] = out_z[i] ;
		outArr[i].m[0] = out_m[i] ;
		outArr[i].e[0] = out_e[i] ;
	}

	delete[] arr1_s ; delete[] arr2_s ; delete[] out_s ;
	delete[] arr1_z ; delete[] arr2_z ; delete[] out_z ;
	delete[] arr1_m ; delete[] arr2_m ; delete[] out_m ;
	delete[] arr1_e ; delete[] arr2_e ; delete[] out_e ;
}

void GemmAdd(int32_t s1, int32_t s2, 
	vector<vector<FPArray>> &inArr, 
	vector<FPArray> &bias, 
	vector<vector<FPArray>> &outArr) {

	int m_bits, e_bits ;
	int sz ;

	m_bits = inArr[0][0].m_bits ;
	e_bits = inArr[0][0].e_bits ;
	sz = s1*s2 ;

	vector<FPArray> arr1 = make_vector_float(ALICE, sz) ;
	vector<FPArray> arr2 = make_vector_float(ALICE, sz) ;
	vector<FPArray> out = make_vector_float(ALICE, sz) ;

	for (int i1=0, k=0 ; i1 < s1 ; i1++) {
		for (int i2 = 0 ; i2 < s2 ; i2++, k++) {
			arr1[k] = inArr[i1][i2] ;
			arr2[k] = bias[i2] ;
		}
	}

	ElemWiseAdd(sz, arr1, arr2, out) ;

	for (int i1 = 0, k = 0 ; i1 < s1 ; i1++) {
		for (int i2 = 0 ; i2 < s2 ; i2++, k++) {
			outArr[i1][i2] = out[k] ;
		}
	}
}

void Relu_thread(
	int tid, int sz, int m_bits, int e_bits,
	uint8_t *in_s, uint8_t *in_z, uint64_t *in_m, uint64_t *in_e,   
	uint8_t *out_s, uint8_t *out_z, uint64_t *out_m, uint64_t *out_e, uint8_t *hot
	) {

	FPArray in_flat = fpopArr[tid]->input(WHICHPARTY, sz, in_s, in_z, in_m, in_e, m_bits, e_bits) ;

	BoolArray sgn, zero ;
	FixArray _2, _3 ;
	std::tie(sgn, zero, _2, _3) = get_components(tid, in_flat) ;

	FPArray zero_flat = fpopArr[tid]->input<float>(ALICE, sz, (float)0.0, m_bits, e_bits) ;
	FPArray out_flat = fpopArr[tid]->if_else(sgn, zero_flat, in_flat) ;

	memcpy(out_s, out_flat.s, sz*sizeof(uint8_t)) ;
	memcpy(out_z, out_flat.z, sz*sizeof(uint8_t)) ;
	memcpy(out_m, out_flat.m, sz*sizeof(uint64_t)) ;
	memcpy(out_e, out_flat.e, sz*sizeof(uint64_t)) ;
	memcpy(hot, sgn.data, sz*sizeof(uint8_t))  ;
}

void Relu(
	int32_t s1, 
	vector<FPArray> &inArr, 
	vector<FPArray> &outArr,
	vector<BoolArray> &hotArr) {
	int m_bits, e_bits ;
	m_bits = inArr[0].m_bits ;
	e_bits = inArr[0].e_bits ;

	uint8_t *in_s = new uint8_t[s1] ;
	uint8_t *in_z = new uint8_t[s1] ;
	uint64_t *in_m = new uint64_t[s1] ;
	uint64_t *in_e = new uint64_t[s1] ;
	for (int i = 0 ; i < s1 ; i++) {
		in_s[i] = inArr[i].s[0] ;
		in_z[i] = inArr[i].z[0] ;
		in_m[i] = inArr[i].m[0] ;
		in_e[i] = inArr[i].e[0] ;
	}

	uint8_t *out_s = new uint8_t[s1] ;
	uint8_t *out_z = new uint8_t[s1] ;
	uint64_t *out_m = new uint64_t[s1] ;
	uint64_t *out_e = new uint64_t[s1] ;
	uint8_t *hot = new uint8_t[s1] ;

	vector<int> chunks = get_chunks(s1, __nt) ;
	thread threads[MAX_THREADS] ;
	int offset = 0 ;
	for (int i = 0 ; i < __nt ; i++) {
		if (chunks[i] > 0) {
			threads[i] = thread(Relu_thread,
				i, chunks[i], m_bits, e_bits,
				in_s + offset, in_z + offset, in_m + offset, in_e + offset,
				out_s + offset, out_z + offset, out_m + offset, out_e + offset, hot + offset
			) ;
			offset += chunks[i] ;
		}
	}

	for (int i = 0 ; i < __nt ; i++)
		if (chunks[i] > 0)
			threads[i].join() ;

	for (int i = 0 ; i < s1 ; i++) {
		outArr[i].m_bits = m_bits ;
		outArr[i].e_bits = e_bits ;

		outArr[i].s[0] = out_s[i] ;
		outArr[i].z[0] = out_z[i] ;
		outArr[i].m[0] = out_m[i] ;
		outArr[i].e[0] = out_e[i] ;
		hotArr[i].data[0] = hot[i] ;
	}

	delete[] in_s ; delete[] out_s ;
	delete[] in_z ; delete[] out_z ;
	delete[] in_m ; delete[] out_m ;
	delete[] in_e ; delete[] out_e ;
	delete[] hot ;
}

void vectorSum_thread(
	int tid, int chunk, int colsize, int m_bits, int e_bits,
	uint8_t **Row_s, uint8_t **Row_z, uint64_t **Row_m, uint64_t **Row_e,
	uint8_t *row_s, uint8_t *row_z, uint64_t *row_m, uint64_t *row_e
	) {
	vector<FPArray> sums ;
	for (int i = 0 ; i < chunk ; i++) {
		sums.push_back(
			fpopArr[tid]->input(
				WHICHPARTY, colsize, Row_s[i], Row_z[i], Row_m[i], Row_e[i], m_bits, e_bits
			)
		) ;
	}

	FPArray vsum = fpopArr[tid]->treesum(sums) ;

	memcpy(row_s, vsum.s, chunk*sizeof(uint8_t)) ;
	memcpy(row_z, vsum.z, chunk*sizeof(uint8_t)) ;
	memcpy(row_m, vsum.m, chunk*sizeof(uint64_t)) ;
	memcpy(row_e, vsum.e, chunk*sizeof(uint64_t)) ;
}

void getBiasDer(int32_t m, int32_t s2, vector<vector<FPArray>>& batchSoftDer, vector<FPArray> &biasDer) {
	int m_bits, e_bits ;
	m_bits = batchSoftDer[0][0].m_bits ;
	e_bits = batchSoftDer[0][0].e_bits ;

	uint8_t **Row_s = new uint8_t*[s2] ;
	uint8_t **Row_z = new uint8_t*[s2] ;
	uint64_t **Row_m = new uint64_t*[s2] ;
	uint64_t **Row_e = new uint64_t*[s2] ;

	uint8_t *row_s = new uint8_t[s2] ;
	uint8_t *row_z = new uint8_t[s2] ;
	uint64_t *row_m = new uint64_t[s2] ;
	uint64_t *row_e = new uint64_t[s2] ;

	for (int i = 0 ; i < s2 ; i++) {
		Row_s[i] = new uint8_t[m] ;
		Row_z[i] = new uint8_t[m] ;
		Row_m[i] = new uint64_t[m] ;
		Row_e[i] = new uint64_t[m] ;

		for (int j = 0 ; j < m ; j++) {
			Row_s[i][j] = batchSoftDer[j][i].s[0] ;
			Row_z[i][j] = batchSoftDer[j][i].z[0] ;
			Row_m[i][j] = batchSoftDer[j][i].m[0] ;
			Row_e[i][j] = batchSoftDer[j][i].e[0] ;
		}
	}


	vector<int> chunks = get_chunks(s2, __nt) ;
	thread threads[MAX_THREADS] ;
	int offset = 0 ;
	for (int i = 0 ; i < __nt ; i++) {
		if (chunks[i] > 0) {
			threads[i] = thread(vectorSum_thread,
				i, chunks[i], m, m_bits, e_bits,
				Row_s+offset, Row_z+offset, Row_m+offset, Row_e+offset,
				row_s+offset, row_z+offset, row_m+offset, row_e+offset
			) ;
			offset += chunks[i] ;
		}
	}	

	for (int i = 0 ; i < __nt ; i++)
		if (chunks[i] > 0)
			threads[i].join() ;

	for (int i = 0 ; i < s2 ; i++) {
		biasDer[i].m_bits = m_bits ;
		biasDer[i].e_bits = e_bits ;

		biasDer[i].s[0] = row_s[i] ;
		biasDer[i].z[0] = row_z[i] ;
		biasDer[i].m[0] = row_m[i] ;
		biasDer[i].e[0] = row_e[i] ;
	}

	for (int i = 0 ; i < s2 ; i++) {
		delete[] Row_s[i] ;
		delete[] Row_z[i] ;
		delete[] Row_m[i] ;
		delete[] Row_e[i] ;
	}

	delete[] Row_s ; delete[] row_s ;
	delete[] Row_z ; delete[] row_z ;
	delete[] Row_m ; delete[] row_m ;
	delete[] Row_e ; delete[] row_e ;
}

void updateWeights_thread(
	int tid, int chunk, float lr, int m_bits, int e_bits,
	uint8_t *in_s, uint8_t *in_z, uint64_t *in_m, uint64_t *in_e,
	uint8_t *der_s, uint8_t *der_z, uint64_t *der_m, uint64_t *der_e
	) {
	FPArray weight = fpopArr[tid]->input(WHICHPARTY, chunk, in_s, in_z, in_m, in_e, m_bits, e_bits) ;
	FPArray der = fpopArr[tid]->input(WHICHPARTY, chunk, der_s, der_z, der_m, der_e, m_bits, e_bits) ;
	FPArray muller = fpopArr[tid]->input<float>(PUBLIC, chunk, lr, m_bits, e_bits) ;

	der = fpopArr[tid]->mul(der, muller) ;
	weight = fpopArr[tid]->sub(weight, der) ;

	memcpy(in_s, weight.s, chunk*sizeof(uint8_t)) ;
	memcpy(in_z, weight.z, chunk*sizeof(uint8_t)) ;
	memcpy(in_m, weight.m, chunk*sizeof(uint64_t)) ;
	memcpy(in_e, weight.e, chunk*sizeof(uint64_t)) ;
}

void updateWeights(int32_t sz, float lr, vector<FPArray>& inArr, vector<FPArray>& derArr) {
	int m_bits, e_bits ;
	m_bits = inArr[0].m_bits ;
	e_bits = inArr[0].e_bits ;

	uint8_t *in_s = new uint8_t[sz] ;
	uint8_t *in_z = new uint8_t[sz] ;
	uint64_t *in_m = new uint64_t[sz] ;
	uint64_t *in_e = new uint64_t[sz] ;

	uint8_t *der_s = new uint8_t[sz] ;
	uint8_t *der_z = new uint8_t[sz] ;
	uint64_t *der_m = new uint64_t[sz] ;
	uint64_t *der_e = new uint64_t[sz] ;

	for (int i = 0 ; i < sz ; i++) {
		in_s[i] = inArr[i].s[0] ; der_s[i] = derArr[i].s[0] ;
		in_z[i] = inArr[i].z[0] ; der_z[i] = derArr[i].z[0] ;
		in_m[i] = inArr[i].m[0] ; der_m[i] = derArr[i].m[0] ;
		in_e[i] = inArr[i].e[0] ; der_e[i] = derArr[i].e[0] ;
	}

	vector<int> chunks = get_chunks(sz, __nt) ;
	thread threads[MAX_THREADS] ;
	int offset = 0 ;
	for (int i = 0 ; i < __nt ; i++) {
		if (chunks[i] > 0) {
			threads[i] = thread(updateWeights_thread,
				i, chunks[i], lr, m_bits, e_bits,
				in_s+offset, in_z+offset, in_m+offset, in_e+offset,
				der_s+offset, der_z+offset, der_m+offset, der_e+offset
			) ;
			offset += chunks[i] ;
		}
	}

	for (int i = 0 ; i < __nt ; i++)
		if (chunks[i] > 0)
			threads[i].join() ;

	for (int i = 0 ; i < sz ; i++) {
		inArr[i].m_bits = m_bits ;
		inArr[i].e_bits = e_bits ;

		inArr[i].s[0] = in_s[i] ;
		inArr[i].z[0] = in_z[i] ;
		inArr[i].m[0] = in_m[i] ;
		inArr[i].e[0] = in_e[i] ;
	}

	delete[] in_s ; delete[] der_s ;
	delete[] in_z ; delete[] der_z ;
	delete[] in_m ; delete[] der_m ;
	delete[] in_e ; delete[] der_e ;
}

void Softmax2_thread(
	int tid, int mchunk, int n, int m_bits, int e_bits,
	uint8_t **in_s, uint8_t **in_z, uint64_t **in_m, uint64_t **in_e,
	uint8_t **out_s, uint8_t **out_z, uint64_t **out_m, uint64_t **out_e
	) {
	vector<FPArray> softin, softout ;

	for (int i = 0 ; i < mchunk ; i++)
		softin.push_back(fpopArr[tid]->input(WHICHPARTY, n, in_s[i], in_z[i], in_m[i], in_e[i], m_bits, e_bits)) ;

	softout = fpmathArr[tid]->softmax(softin) ;	
	for (int i = 0 ; i < mchunk ; i++) {
		memcpy(out_s[i], softout[i].s, n*sizeof(uint8_t)) ;
		memcpy(out_z[i], softout[i].z, n*sizeof(uint8_t)) ;
		memcpy(out_m[i], softout[i].m, n*sizeof(uint64_t)) ;
		memcpy(out_e[i], softout[i].e, n*sizeof(uint64_t)) ;
	}
}

void Softmax2(
	int32_t s1, 
	int32_t s2, 
	vector<vector<FPArray>> &inArr, 
	vector<vector<FPArray>> &outArr) {
	int m_bits = inArr[0][0].m_bits ;
	int e_bits = inArr[0][0].e_bits ;

	uint8_t **row_s = new uint8_t*[s1] ;
	uint8_t **row_z = new uint8_t*[s1] ;
	uint64_t **row_m = new uint64_t*[s1] ;
	uint64_t **row_e = new uint64_t*[s1] ;

	uint8_t **out_s = new uint8_t*[s1] ;
	uint8_t **out_z = new uint8_t*[s1] ;
	uint64_t **out_m = new uint64_t*[s1] ;
	uint64_t **out_e = new uint64_t*[s1] ;

	for (int i = 0 ; i < s1 ; i++) {
		row_s[i] = new uint8_t[s2] ;
		row_z[i] = new uint8_t[s2] ;
		row_m[i] = new uint64_t[s2] ;
		row_e[i] = new uint64_t[s2] ;

		out_s[i] = new uint8_t[s2] ;
		out_z[i] = new uint8_t[s2] ;
		out_m[i] = new uint64_t[s2] ;
		out_e[i] = new uint64_t[s2] ;

		for (int j = 0 ; j < s2 ; j++) {
			row_s[i][j] = inArr[i][j].s[0] ;
			row_z[i][j] = inArr[i][j].z[0] ;
			row_m[i][j] = inArr[i][j].m[0] ;
			row_e[i][j] = inArr[i][j].e[0] ;
		}
	}

	vector<int> chunks = get_chunks(s1, __nt) ;
	thread threads[MAX_THREADS] ;
	int offset = 0 ;
	for (int i = 0 ; i < __nt ; i++) {
		if (chunks[i] > 0) {
			threads[i] = thread(Softmax2_thread,
				i, chunks[i], s2, m_bits, e_bits,
				row_s+offset, row_z+offset, row_m+offset, row_e+offset,
				out_s+offset, out_z+offset, out_m+offset, out_e+offset
			) ;
			offset += chunks[i] ;
		}
	}

	for (int i = 0 ; i < __nt ; i++)
		if (chunks[i] > 0)
			threads[i].join() ;


	for (int i = 0 ; i < s1 ; i++) {
		for (int j = 0 ; j < s2 ; j++) {
			outArr[i][j].s[0] = out_s[i][j] ;
			outArr[i][j].z[0] = out_z[i][j] ;
			outArr[i][j].m[0] = out_m[i][j] ;
			outArr[i][j].e[0] = out_e[i][j] ;
		}
	}

	for (int i = 0 ; i < s1 ; i++) {
		delete[] row_s[i] ; delete[] out_s[i] ;
		delete[] row_z[i] ; delete[] out_z[i] ;
		delete[] row_m[i] ; delete[] out_m[i] ;
		delete[] row_e[i] ; delete[] out_e[i] ;
	}

	delete[] row_s ; delete[] out_s ;
	delete[] row_z ; delete[] out_z ;
	delete[] row_m ; delete[] out_m ;
	delete[] row_e ; delete[] out_e ;
}

void Ln_thread(
	int tid, int sz, int m_bits, int e_bits,
	uint8_t *in_s, uint8_t *in_z, uint64_t *in_m, uint64_t *in_e,   
	uint8_t *out_s, uint8_t *out_z, uint64_t *out_m, uint64_t *out_e
	) {

	FPArray in_flat = fpopArr[tid]->input(WHICHPARTY, sz, in_s, in_z, in_m, in_e, m_bits, e_bits) ;
	FPArray out_flat = fpmathArr[tid]->ln(in_flat) ;

	memcpy(out_s, out_flat.s, sz*sizeof(uint8_t)) ;
	memcpy(out_z, out_flat.z, sz*sizeof(uint8_t)) ;
	memcpy(out_m, out_flat.m, sz*sizeof(uint64_t)) ;
	memcpy(out_e, out_flat.e, sz*sizeof(uint64_t)) ;
}

void Ln(
	int32_t s1, 
	vector<FPArray> &inArr, 
	vector<FPArray> &outArr) {
	int m_bits, e_bits ;
	m_bits = inArr[0].m_bits ;
	e_bits = inArr[0].e_bits ;

	uint8_t *in_s = new uint8_t[s1] ;
	uint8_t *in_z = new uint8_t[s1] ;
	uint64_t *in_m = new uint64_t[s1] ;
	uint64_t *in_e = new uint64_t[s1] ;
	for (int i = 0 ; i < s1 ; i++) {
		in_s[i] = inArr[i].s[0] ;
		in_z[i] = inArr[i].z[0] ;
		in_m[i] = inArr[i].m[0] ;
		in_e[i] = inArr[i].e[0] ;
	}

	uint8_t *out_s = new uint8_t[s1] ;
	uint8_t *out_z = new uint8_t[s1] ;
	uint64_t *out_m = new uint64_t[s1] ;
	uint64_t *out_e = new uint64_t[s1] ;
	uint8_t *hot = new uint8_t[s1] ;

	vector<int> chunks = get_chunks(s1, __nt) ;
	thread threads[MAX_THREADS] ;
	int offset = 0 ;
	for (int i = 0 ; i < __nt ; i++) {
		if (chunks[i] > 0) {
			threads[i] = thread(Ln_thread,
				i, chunks[i], m_bits, e_bits,
				in_s + offset, in_z + offset, in_m + offset, in_e + offset,
				out_s + offset, out_z + offset, out_m + offset, out_e + offset
			) ;
			offset += chunks[i] ;
		}
	}

	for (int i = 0 ; i < __nt ; i++)
		if (chunks[i] > 0)
			threads[i].join() ;

	for (int i = 0 ; i < s1 ; i++) {
		outArr[i].m_bits = m_bits ;
		outArr[i].e_bits = e_bits ;

		outArr[i].s[0] = out_s[i] ;
		outArr[i].z[0] = out_z[i] ;
		outArr[i].m[0] = out_m[i] ;
		outArr[i].e[0] = out_e[i] ;
	}

	delete[] in_s ; delete[] out_s ;
	delete[] in_z ; delete[] out_z ;
	delete[] in_m ; delete[] out_m ;
	delete[] in_e ; delete[] out_e ;
}

void dotProduct2(int32_t s1, int32_t s2, vector<vector<FPArray>>& arr1, vector<vector<FPArray>>& arr2, vector<FPArray>& outArr) {
	int m_bits, e_bits ;
	m_bits = arr1[0][0].m_bits ;
	e_bits = arr1[0][0].e_bits ;

	vector<FPArray> dot1, dot2 ;

	uint8_t *arr1_s = new uint8_t[s2] ;
	uint8_t *arr1_z = new uint8_t[s2] ;
	uint64_t *arr1_m = new uint64_t[s2] ;
	uint64_t *arr1_e = new uint64_t[s2] ;

	uint8_t *arr2_s = new uint8_t[s2] ;
	uint8_t *arr2_z = new uint8_t[s2] ;
	uint64_t *arr2_m = new uint64_t[s2] ;
	uint64_t *arr2_e = new uint64_t[s2] ;

	for (int i = 0 ; i < s1 ; i++) {
		for (int j = 0 ; j < s2 ; j++) {
			arr1_s[j] = arr1[i][j].s[0] ; arr2_s[j] = arr2[i][j].s[0] ;
			arr1_z[j] = arr1[i][j].z[0] ; arr2_z[j] = arr2[i][j].z[0] ;
			arr1_m[j] = arr1[i][j].m[0] ; arr2_m[j] = arr2[i][j].m[0] ;
			arr1_e[j] = arr1[i][j].e[0] ; arr2_e[j] = arr2[i][j].e[0] ;
		}

		dot1.push_back(__fp_op->input(__party, s2, arr1_s, arr1_z, arr1_m, arr1_e, m_bits, e_bits)) ;
		dot2.push_back(__fp_op->input(__party, s2, arr2_s, arr2_z, arr2_m, arr2_e, m_bits, e_bits)) ;
	}

	FPArray res = __fp_op->treesum(__fp_op->mul(dot1, dot2)) ;
	for (int i = 0 ; i < s1 ; i++) {
		outArr[i].s[0] = res.s[i] ;
		outArr[i].z[0] = res.z[i] ;
		outArr[i].m[0] = res.m[i] ;
		outArr[i].e[0] = res.e[i] ;
	}

	delete[] arr1_s ; delete[] arr2_s ;
	delete[] arr1_z ; delete[] arr2_z ;
	delete[] arr1_m ; delete[] arr2_m ;
	delete[] arr1_e ; delete[] arr2_e ;
}

void getLoss(int32_t s, vector<FPArray>& arr, vector<FPArray>& outArr) {
	int m_bits, e_bits ;
	m_bits = arr[0].m_bits ;
	e_bits = arr[0].e_bits ;

	uint8_t *in_s = new uint8_t[s] ;
	uint8_t *in_z = new uint8_t[s] ;
	uint64_t *in_m = new uint64_t[s] ;
	uint64_t *in_e = new uint64_t[s] ;

	for (int i = 0 ; i < s ; i++) {
		in_s[i] = arr[i].s[0] ;
		in_z[i] = arr[i].z[0] ;
		in_m[i] = arr[i].m[0] ;
		in_e[i] = arr[i].e[0] ;
	}

	vector<FPArray> sum ;
	sum.push_back(__fp_op->input(__party, s, in_s, in_z, in_m, in_e, m_bits, e_bits)) ;
	
	FPArray res = __fp_op->treesum(sum) ;
	FPArray div = __fp_op->input<float>(ALICE, 1, (float)-1.0/s, m_bits, e_bits) ;
	res = __fp_op->mul(res, div) ;

	outArr[0].s[0] = res.s[0] ;
	outArr[0].z[0] = res.z[0] ;
	outArr[0].m[0] = res.m[0] ;
	outArr[0].e[0] = res.e[0] ;

	delete[] in_s ;
	delete[] in_z ;
	delete[] in_m ;
	delete[] in_e ;
}

void computeMSELoss(int32_t m, int32_t s, vector<vector<FPArray>>& target, vector<vector<FPArray>>& fwdOut, vector<FPArray>& loss) {
	vector<FPArray> target_flat = make_vector_float(ALICE, m) ;
	vector<FPArray> out_flat = make_vector_float(ALICE, m) ;

	for (int i = 0 ; i < m ; i++) {
		target_flat[i] = target[i][0] ;
		out_flat[i] = fwdOut[i][0] ;
	}

	vector<FPArray> subbed = make_vector_float(ALICE, m) ;
	vector<FPArray> loss_terms = make_vector_float(ALICE, m) ;

	ElemWiseSub(m, out_flat, target_flat, subbed) ;
	ElemWiseMul(m, subbed, subbed, loss_terms) ;
	getLoss(m, loss_terms, loss) ;
}