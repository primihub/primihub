/**
  \file 		optPaillier.h
  \author 	Jiang Zhengliang
  \copyright Copyright (C) 2022 Jiang Zhengliang
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
  const int radix = 10);

// void data_packing_mul(
//   mpz_t res,
//   const mpz_t cipher_pack,
//   const mpz_t constant_op);

void free_crt(
  CrtMod* crtmod);

#endif
