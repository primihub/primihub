/**
  \file 		optPaillier.h
  \author 	Jiang Zhengliang
  \copyright Copyright (C) 2022 Qinghai University
 */

#ifndef __CRT_DATA_PACK__
#define __CRT_DATA_PACK__

#include <gmp.h>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include "powmod.h"
#include "utils.h"

using ll = long long;

struct CrtMod {
  // mpz_t muls;
  // mpz_t* crt_coef;
  mpz_t* crt_half_mod;
  mpz_t* crt_mod;
  size_t crt_size;
  mp_bitcnt_t mod_size;
};

void init_crt(
  CrtMod** crtmod,
  const size_t crt_size,
  const mp_bitcnt_t mod_size);

void data_packing_crt(
  mpz_t res,
  char** seq,
  const size_t seq_size,
  const CrtMod* crtmod,
  const int radix = 10);

void data_retrieve_crt(
  char**& seq,
  const mpz_t pack,
  const CrtMod* crtmod,
  const size_t data_size,
  const size_t str_size,
  const int radix = 10);

void free_crt(
  CrtMod* crtmod);

#endif
