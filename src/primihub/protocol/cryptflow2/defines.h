/*
Authors: Deevashwer Rathee
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

#ifndef DEFINES_H___
#define DEFINES_H___

#define LOG_LAYERWISE
// #define VERIFY_LAYERWISE
#define USE_EIGEN
#define WRITE_LOG
// #define APPROXIMATE_SIGMOID
// #define APPROXIMATE_TANH
#define THREADING_MIN_CHUNK_SIZE 128

#define DIV_RESCALING
#ifndef DIV_RESCALING
#define TRUNCATION_RESCALING
#endif

#include <fstream>
#include <stdint.h>
#include <string>

#define RESET "\033[0m"
#define RED "\033[31m"   /* Red */
#define GREEN "\033[32m" /* Green */

const int SERVER = 1;
const int CLIENT = 2;

extern int party;
extern std::string address;
extern int port;
extern int num_threads;

#endif
