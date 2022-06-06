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

#ifndef LIBRARY_CLEARTEXT_FLOAT_H__
#define LIBRARY_CLEARTEXT_FLOAT_H__

#include <vector>
#include <math.h>
#include <cstdlib>
#include <iostream>
#include <fstream>

using namespace std ;

template<typename T>
vector<T> make_vector(size_t size) {
return std::vector<T>(size) ;
}

template <typename T, typename... Args>
auto make_vector(size_t first, Args... sizes)
{
auto inner = make_vector<T>(sizes...) ;
return vector<decltype(inner)>(first, inner) ;
}

float intToFloat(int32_t m);
void Softmax2(int32_t s1, int32_t s2, vector<vector<float>>& inArr, vector<vector<float>>& outArr);
void Ln(int32_t s1, vector<float>& inArr, vector<float>& outArr);
void getOutDer(int32_t s1, int32_t s2, vector<vector<float>>& batchSoft, vector<vector<float>>& lab, vector<vector<float>>& der);
void MatMul(int32_t s1, int32_t s2, int32_t s3, vector<vector<float>>& mat1, vector<vector<float>>& mat2, vector<vector<float>>& mat3);
void GemmAdd(int32_t s1, int32_t s2, vector<vector<float>>& prod, vector<float>& bias, vector<vector<float>>& out);
void dotProduct2(int32_t s1, int32_t s2, vector<vector<float>>& arr1, vector<vector<float>>& arr2, vector<float>& outArr);
void Relu(int32_t s1, vector<float>& inArr, vector<float>& outArr, vector<bool>& hotArr);
void getBiasDer(int32_t s1, int32_t s2, vector<vector<float>>& der, vector<float>& biasDer);
void IfElse(int32_t s1, vector<float>& dat, vector<bool>& hot, vector<float>& out, bool flip);
void updateWeights(int32_t s, float lr, vector<float>& bias, vector<float>& der);
void getLoss(int32_t m, vector<float>& lossTerms, vector<float>& loss);
void computeMSELoss(int32_t m, int32_t s, vector<vector<float>>& target, vector<vector<float>>& fwdOut, vector<float>& loss);

void Tanh(int32_t s1, vector<float>& inArr, vector<float>& outArr) ;


#endif