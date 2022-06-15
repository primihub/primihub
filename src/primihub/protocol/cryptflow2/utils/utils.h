/*
Original Work Copyright (c) 2018 Xiao Wang (wangxiao@gmail.com)
Modified Work Copyright (c) 2020 Microsoft Research

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

Enquiries about further applications and development opportunities are welcome.

Modified by Deevashwer Rathee
*/

#ifndef UTILS_H__
#define UTILS_H__
#include "src/primihub/protocol/cryptflow2/utils/block.h"
#include "src/primihub/protocol/cryptflow2/utils/prg.h"
#include <chrono>
#include <cstddef>
#include <gmp.h>
#include <sstream>
#include <string>
#define macro_xstr(a) macro_str(a)
#define macro_str(a) #a

using std::string;
using std::chrono::high_resolution_clock;
using std::chrono::time_point;

namespace primihub::sci
{
  template <typename T>
  void inline delete_array_null(T *ptr);

  inline void error(const char *s, int line = 0, const char *file = nullptr);

  template <class... Ts>
  void run_function(void *function, const Ts &...args);

  inline void parse_party_and_port(char **arg, int argc, int *party, int *port);

  std::string Party(int p);

  // Timing related
  inline time_point<high_resolution_clock> clock_start();
  inline double time_from(const time_point<high_resolution_clock> &s);

  // block128 conversions
  template <typename T = uint64_t>
  std::string m128i_to_string(const __m128i var);
  block128 bool_to128(const bool *data);

  template <typename T>
  inline void int_to_bool(bool *data, T input, int len);
  void int64_to_bool(bool *data, uint64_t input, int length);
  inline void uint8_to_bool(uint8_t *data, uint8_t input, int length);

  // Other conversions
  template <typename T>
  T bool_to_int(const bool *data, size_t len = 0);
  inline uint8_t bool_to_uint8(const uint8_t *data, size_t len = 0);
  std::string hex_to_binary(std::string hex);
  inline string change_base(string str, int old_base, int new_base);
  inline string dec_to_bin(const string &dec);
  inline string bin_to_dec(const string &bin2);
  inline const char *hex_char_to_bin(char c);

  inline int bitlen(int x);
  inline int bitlen_true(int x);

  inline int64_t neg_mod(int64_t val, int64_t mod);
  inline int8_t neg_mod(int8_t val, int8_t mod);

  // deprecate soon
  inline void parse_party_and_port(char **arg, int *party, int *port)
  {
    parse_party_and_port(arg, 2, party, port);
  }

#include "src/primihub/protocol/cryptflow2/utils/utils.hpp"
} // namespace primihub::sci
#endif // UTILS_H__
