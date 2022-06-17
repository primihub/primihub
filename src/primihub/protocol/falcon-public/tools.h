
#ifndef TOOLS_H
#define TOOLS_H
#pragma once

#include "AESObject.h"
#include "connect.h"
#include "globals.h"
#include "secCompMultiParty.h"
#include "util/Config.h"
#include "util/TedKrovetzAesNiWrapperC.h"
#include "util/main_gf_funcs.h"

#ifdef ENABL_SSE
#include <emmintrin.h>
#include <smmintrin.h>
#include <wmmintrin.h>
#else
#include "util/block.h"
#endif

#include <iostream>
#include <math.h>
#include <openssl/sha.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <time.h>
#include <vector>
namespace primihub
{
  namespace falcon
  {
    extern int partyNum;

    extern AESObject *aes_next;
    extern AESObject *aes_indep;

    extern smallType additionModPrime[PRIME_NUMBER][PRIME_NUMBER];
    extern smallType subtractModPrime[PRIME_NUMBER][PRIME_NUMBER];
    extern smallType multiplicationModPrime[PRIME_NUMBER][PRIME_NUMBER];

#if MULTIPLICATION_TYPE == 0
#define MUL(x, y) gfmul(x, y)
#define MULT(x, y, ans) gfmul(x, y, &ans)

#ifdef OPTIMIZED_MULTIPLICATION
#define MULTHZ(x, y, ans) gfmulHalfZeros(x, y, &ans) // optimized multiplication when half of y is zeros
#define MULHZ(x, y) gfmulHalfZeros(x, y)             // optimized multiplication when half of y is zeros
#else
#define MULTHZ(x, y, ans) gfmul(x, y, &ans)
#define MULHZ(x, y) gfmul(x, y)
#endif

#ifdef ENABLE_SSE
#define SET_ONE _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1)
#else
#define SET_ONE __m128i()
#endif

#else
#define MUL(x, y) gfmul3(x, y)
#define MULT(x, y, ans) gfmul3(x, y, &ans)
#ifdef OPTIMIZED_MULTIPLICATION
#define MULTHZ(x, y, ans) gfmul3HalfZeros(x, y, &ans) // optimized multiplication when half of y is zeros
#define MULHZ(x, y) gfmul3HalfZeros(x, y)             // optimized multiplication when half of y is zeros
#else
#define MULTHZ(x, y, ans) gfmul3(x, y, &ans)
#define MULHZ(x, y) gfmul3(x, y)
#endif

#ifdef ENABLE_SSE
#define SET_ONE _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1)
#else
#define SET_ONE __m128i()
#endif

#define SET_ONE
#endif

//
// field zero
#ifdef ENABLE_SSE
#define SET_ZERO _mm_setzero_si128()
// the partynumber(+1) embedded in the field
#define SETX(j) _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, j + 1) // j+1
// Test if 2 __m128i variables are equal
#define EQ(x, y) _mm_test_all_ones(_mm_cmpeq_epi8(x, y))
// Add 2 field elements in GF(2^128)
#define ADD(x, y) _mm_xor_si128(x, y)
#else
#define SET_ZERO __m128i()
#define SETX(i) TODO("Implement it.")
#define EQ(x, y) TODO("Implement it.");
#define ADD(x, y) TODO("Implement it.")
#endif

// Subtraction and addition are equivalent in characteristic 2 fields
#define SUB ADD
// Evaluate x^n in GF(2^128)
#define POW(x, n) fastgfpow(x, n)
// Evaluate x^2 in GF(2^128)
#define SQ(x) square(x)
// Evaluate x^(-1) in GF(2^128)
#define INV(x) inverse(x)
// Evaluate P(x), where p is a given polynomial, in GF(2^128)
#define EVAL(x, poly, ans) fastPolynomEval(x, poly, &ans) // polynomEval(SETX(x),y,z)
// Reconstruct the secret from the shares
#define RECON(shares, deg, secret) reconstruct(shares, deg, &secret)
// returns a (pseudo)random __m128i number using AES-NI
#define RAND LoadSeedNew
// returns a (pseudo)random bit using AES-NI
#define RAND_BIT LoadBool

// the encryption scheme
#define PSEUDO_RANDOM_FUNCTION(seed1, seed2, index, numberOfBlocks, result) pseudoRandomFunctionwPipelining(seed1, seed2, index, numberOfBlocks, result);

    // The degree of the secret-sharings before multiplications
    extern int degSmall;
    // The degree of the secret-sharing after multiplications (i.e., the degree of the secret-sharings of the PRFs)
    extern int degBig;
    // The type of honest majority we assume
    extern int majType;

    // bases for interpolation
    extern __m128i *baseReduc;
    extern __m128i *baseRecon;
    // saved powers for evaluating polynomials
    extern __m128i *powers;

    // one in the field
    extern const __m128i ONE;
    // zero in the field
    extern const __m128i ZERO;

    extern int testCounter;

    typedef struct polynomial
    {
      int deg;
      __m128i *coefficients;
    } Polynomial;

    void gfmul(__m128i a, __m128i b, __m128i *res);

    // This function works correctly only if all the upper half of b is zeros
    void gfmulHalfZeros(__m128i a, __m128i b, __m128i *res);

    // multiplies a and b
    __m128i gfmul(__m128i a, __m128i b);

    // This function works correctly only if all the upper half of b is zeros
    __m128i gfmulHalfZeros(__m128i a, __m128i b);

    __m128i gfpow(__m128i x, int deg);

    __m128i fastgfpow(__m128i x, int deg);

    __m128i square(__m128i x);

    __m128i inverse(__m128i x);

    string _sha256hash_(char *input, int length);

    string sha256hash(char *input, int length);

    void printError(string error);

    string __m128i_toHex(__m128i var);

    string __m128i_toString(__m128i var);

    __m128i stringTo__m128i(string str);

    unsigned int charValue(char c);

    string convertBooltoChars(bool *input, int length);

    string toHex(string s);

    string convertCharsToString(char *input, int size);

    void print(__m128i *arr, int size);

    void print128_num(__m128i var);

    void log_print(string str);
    void error(string str);
    string which_network(string network);

    void print_myType(myType var, string message, string type);
    void print_linear(myType var, string type);
    void print_vector(RSSVectorMyType &var, string type, string pre_text, int print_nos);
    void print_vector(RSSVectorSmallType &var, string type, string pre_text, int print_nos);
    void matrixMultRSS(const RSSVectorMyType &a, const RSSVectorMyType &b, vector<myType> &temp3,
                       size_t rows, size_t common_dim, size_t columns,
                       size_t transpose_a, size_t transpose_b);

    myType dividePlain(myType a, int b);
    void dividePlain(vector<myType> &vec, int divisor);

    size_t nextParty(size_t party);
    size_t prevParty(size_t party);

    inline smallType getMSB(myType a)
    {
      return ((smallType)((a >> (BIT_SIZE - 1)) & 1));
    }

    inline RSSSmallType addModPrime(RSSSmallType a, RSSSmallType b)
    {
      RSSSmallType ret;
      ret.first = additionModPrime[a.first][b.first];
      ret.second = additionModPrime[a.second][b.second];
      return ret;
    }

    inline smallType subModPrime(smallType a, smallType b)
    {
      return subtractModPrime[a][b];
    }

    inline RSSSmallType subConstModPrime(RSSSmallType a, const smallType r)
    {
      RSSSmallType ret = a;
      switch (partyNum)
      {
      case PARTY_A:
        ret.first = subtractModPrime[a.first][r];
        break;
      case PARTY_C:
        ret.second = subtractModPrime[a.second][r];
        break;
      }
      return ret;
    }

    inline RSSSmallType XORPublicModPrime(RSSSmallType a, bool r)
    {
      RSSSmallType ret;
      if (r == 0)
        ret = a;
      else
      {
        switch (partyNum)
        {
        case PARTY_A:
          ret.first = subtractModPrime[1][a.first];
          ret.second = subtractModPrime[0][a.second];
          break;
        case PARTY_B:
          ret.first = subtractModPrime[0][a.first];
          ret.second = subtractModPrime[0][a.second];
          break;
        case PARTY_C:
          ret.first = subtractModPrime[0][a.first];
          ret.second = subtractModPrime[1][a.second];
          break;
        }
      }
      return ret;
    }

    inline smallType wrapAround(myType a, myType b)
    {
      return (a > MINUS_ONE - b);
    }

    inline smallType wrap3(myType a, myType b, myType c)
    {
      myType temp = a + b;
      if (wrapAround(a, b))
        return 1 - wrapAround(temp, c);
      else
        return wrapAround(temp, c);
    }

    void wrapAround(const vector<myType> &a, const vector<myType> &b,
                    vector<smallType> &c, size_t size);
    void wrap3(const RSSVectorMyType &a, const vector<myType> &b,
               vector<smallType> &c, size_t size);

    void multiplyByScalar(const RSSVectorMyType &a, size_t scalar, RSSVectorMyType &b);
    // void transposeVector(const RSSVectorMyType &a, RSSVectorMyType &b, size_t rows, size_t columns);
    void zeroPad(const RSSVectorMyType &a, RSSVectorMyType &b,
                 size_t iw, size_t ih, size_t f, size_t Din, size_t B);
    // void convToMult(const RSSVectorMyType &vec1, RSSVectorMyType &vec2,
    // 				size_t iw, size_t ih, size_t f, size_t Din, size_t S, size_t B);

    /*********************** TEMPLATE FUNCTIONS ***********************/

    template <typename T, typename U>
    std::pair<T, U> operator+(const std::pair<T, U> &l, const std::pair<T, U> &r)
    {
      return {l.first + r.first, l.second + r.second};
    }

    template <typename T, typename U>
    std::pair<T, U> operator-(const std::pair<T, U> &l, const std::pair<T, U> &r)
    {
      return {l.first - r.first, l.second - r.second};
    }

    template <typename T, typename U>
    std::pair<T, U> operator^(const std::pair<T, U> &l, const std::pair<T, U> &r)
    {
      return {l.first ^ r.first, l.second ^ r.second};
    }

    template <typename T>
    T operator*(const std::pair<T, T> &l, const std::pair<T, T> &r)
    {
      return {l.first * r.first + l.second * r.first + l.first * r.second};
    }

    template <typename T, typename U>
    std::pair<U, U> operator*(const T a, const std::pair<U, U> &r)
    {
      return {a * r.first, a * r.second};
    }

    template <typename T>
    std::pair<T, T> operator<<(const std::pair<T, T> &l, const int shift)
    {
      return {l.first << shift, l.second << shift};
    }

    template <typename T>
    void addVectors(const vector<T> &a, const vector<T> &b, vector<T> &c, size_t size)
    {
      for (size_t i = 0; i < size; ++i)
        c[i] = a[i] + b[i];
    }

    template <typename T>
    void subtractVectors(const vector<T> &a, const vector<T> &b, vector<T> &c, size_t size)
    {
      for (size_t i = 0; i < size; ++i)
        c[i] = a[i] - b[i];
    }

    template <typename T>
    void copyVectors(const vector<T> &a, vector<T> &b, size_t size)
    {
      for (size_t i = 0; i < size; ++i)
        b[i] = a[i];
    }

#include <chrono>
#include <utility>

    typedef std::chrono::high_resolution_clock::time_point TimeVar;

#define duration(a) std::chrono::duration_cast<std::chrono::milliseconds>(a).count()
#define timeNow() std::chrono::high_resolution_clock::now()

    template <typename F, typename... Args>
    double funcTime(F func, Args &&...args)
    {
      TimeVar t1 = timeNow();
      func(std::forward<Args>(args)...);
      return duration(timeNow() - t1);
    }
  }
  }
#endif
