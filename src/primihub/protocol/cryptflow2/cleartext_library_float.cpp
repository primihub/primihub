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

#include "cleartext_library_float.h"
#include "math.h"

using namespace std ;

float intToFloat(int32_t m) {
	return (float)m ;
}

void Softmax1(int32_t s1,  vector<float>& inArr, vector<float>& outArr) {
	float sum = 0.0 ;
	float max = inArr[0] ;

	for (int i = 1 ; i < s1 ; i++)
		if (max > inArr[i])
			max = inArr[i] ;

	for (int i = 0 ; i < s1 ; i++) {
		outArr[i] = exp(inArr[i]-max) ;
		sum += outArr[i] ;
	}	

	for (int i = 0 ; i < s1 ; i++) {
		outArr[i] /= sum ;
	}
}

void Softmax2(int32_t s1, int32_t s2, vector<vector<float>>& inArr, vector<vector<float>>& outArr) {
	for (int i = 0 ; i < s1 ; i++)
		Softmax1(s2, inArr[i], outArr[i]) ;
}

void Ln(int32_t s1, vector<float>& inArr, vector<float>& outArr) {
	for (int i = 0 ; i < s1 ; i++) {
		outArr[i] = log(inArr[i]) ;
	}
}

void Tanh(int32_t s1, vector<float>& inArr, vector<float>& outArr) {
	for (int i = 0 ; i < s1 ; i++) {
		outArr[i] = inArr[i] + 1 ;
	}
}

void getOutDer(int32_t s1, int32_t s2, vector<vector<float>>& batchSoft, vector<vector<float>>& lab, vector<vector<float>>& der) {
for (uint32_t i1 = 0; i1 < s1; i1++){
for (uint32_t i2 = 0; i2 < s2; i2++){
der[i1][i2] = ((batchSoft[i1][i2] - lab[i1][i2]) / intToFloat(s1)) ;
}
}
}

void MatMul(int32_t s1, int32_t s2, int32_t s3, vector<vector<float>>& mat1, vector<vector<float>>& mat2, vector<vector<float>>& mat3) {
for (uint32_t i1 = 0; i1 < s1; i1++){
for (uint32_t i3 = 0; i3 < s3; i3++){
mat3[i1][i3] = 0. ;

for (uint32_t i2 = 0; i2 < s2; i2++){
mat3[i1][i3] = (mat3[i1][i3] + (mat1[i1][i2] * mat2[i2][i3])) ;

}
}
}
}

void GemmAdd(int32_t s1, int32_t s2, vector<vector<float>>& prod, vector<float>& bias, vector<vector<float>>& out) {
for (uint32_t i1 = 0; i1 < s1; i1++){
for (uint32_t i2 = 0; i2 < s2; i2++){
out[i1][i2] = (prod[i1][i2] + bias[i2]) ;

}
}
}

void dotProduct2(int32_t s1, int32_t s2, vector<vector<float>>& arr1, vector<vector<float>>& arr2, vector<float>& outArr) {
for (uint32_t i = 0; i < s1; i++){
outArr[i] = 0. ;

for (uint32_t j = 0; j < s2; j++){
outArr[i] = (outArr[i] + (arr1[i][j] * arr2[i][j])) ;

}
}
}

void getLoss(int32_t m, vector<float>& lossTerms, vector<float>& loss) {
loss[0] = 0. ;

for (uint32_t i = 0; i < m; i++){
loss[0] = (loss[0] + lossTerms[i]) ;

}
loss[0] = ((0. - loss[0]) / intToFloat(m)) ;

}

void computeMSELoss(int32_t m, int32_t s, vector<vector<float>>& target, vector<vector<float>>& fwdOut, vector<float>& loss) {
loss[0] = 0. ;

float term ;

for (uint32_t i = 0; i < m; i++){
term = (fwdOut[i][0] - target[i][0]) ;

loss[0] = (loss[0] + (term * term)) ;

}
loss[0] = (loss[0] / (2. * intToFloat(m))) ;

}

void getBiasDer(int32_t s1, int32_t s2, vector<vector<float>>& der, vector<float>& biasDer) {
for (uint32_t i2 = 0; i2 < s2; i2++){
biasDer[i2] = der[0][i2] ;

for (uint32_t i1 = 1; i1 < s1; i1++){
biasDer[i2] = (biasDer[i2] + der[i1][i2]) ;

}
}
}


void updateWeights(int32_t s, float lr, vector<float>& bias, vector<float>& der) {
for (uint32_t i = 0; i < s; i++){
bias[i] = (bias[i] - (lr * der[i])) ;
}
}

void IfElse(int32_t s1, vector<float>& dat, vector<bool>& hot, vector<float>& out, bool flip) {
	for (uint32_t i1 = 0; i1 < s1; i1++) {
		out[i1] = hot[i1] ? dat[i1] : 0. ;
	}
}

void Relu(int32_t s1, vector<float>& inArr, vector<float>& outArr, vector<bool>& hotArr) {
	for (uint32_t i1 = 0; i1 < s1; i1++){
		hotArr[i1] = (inArr[i1] > 0.) ;
		outArr[i1] = hotArr[i1] ? inArr[i1] : 0. ;	
	}
}