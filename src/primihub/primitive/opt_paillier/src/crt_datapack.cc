#include "../include/crt_datapack.h"

void init_crt(
  CrtMod** crtmod,
  const size_t crt_size,
  const mp_bitcnt_t mod_size) {
    (*crtmod) = (CrtMod*)malloc(sizeof(CrtMod));
    (*crtmod)->crt_size = crt_size;
    (*crtmod)->mod_size = mod_size;
    (*crtmod)->crt_mod = (mpz_t*)malloc(sizeof(mpz_t) * crt_size);
    // (*crtmod)->crt_coef = (mpz_t*)malloc(sizeof(mpz_t) * crt_size);
    (*crtmod)->crt_half_mod = (mpz_t*)malloc(sizeof(mpz_t) * crt_size);
    // mpz_init((*crtmod)->muls);
    // mpz_set_ui((*crtmod)->muls, 1);
    mpz_t cur;
    mpz_init(cur);
    aby_prng(cur, mod_size);
    for (size_t i = 0; i < crt_size; ++i) {
      mpz_init((*crtmod)->crt_mod[i]);
      mpz_init((*crtmod)->crt_half_mod[i]);
      mpz_nextprime((*crtmod)->crt_mod[i], cur);
      mpz_div_ui((*crtmod)->crt_half_mod[i], (*crtmod)->crt_mod[i], 2);
      // mpz_mul((*crtmod)->muls, (*crtmod)->muls, (*crtmod)->crt_mod[i]);
      mpz_set(cur, (*crtmod)->crt_mod[i]);
    }
    // for (size_t i = 0; i < crt_size; ++i) {
    //   mpz_divexact(temp, (*crtmod)->muls, (*crtmod)->crt_mod[i]);
    //   mpz_invert((*crtmod)->crt_coef[i], temp, (*crtmod)->crt_mod[i]);
    //   mpz_mul((*crtmod)->crt_coef[i], (*crtmod)->crt_coef[i], temp);
    //   mpz_mod((*crtmod)->crt_coef[i], (*crtmod)->crt_coef[i], (*crtmod)->muls);
    // }
    mpz_clear(cur);
  }

void data_packing_crt(
  mpz_t res,
  char** seq,
  const size_t seq_size,
  const CrtMod* crtmod,
  const int radix) {
    if (seq_size > crtmod->crt_size) {
      throw "size of packing is more than crt's";
    }
    mpz_set_ui(res, 0);
    mpz_t cur, muls, coef;
    mpz_inits(cur, muls, coef, nullptr);
    mpz_set_ui(muls, 1);
    for (size_t i = 0; i < seq_size; ++i) {
      mpz_mul(muls, muls, crtmod->crt_mod[i]);
    }
    for (size_t i = 0; i < seq_size; ++i) {
      mpz_set_str(cur, seq[i], radix);
      if (mpz_cmp_ui(cur, 0) < 0) {
        mpz_add(cur, cur, crtmod->crt_mod[i]);
        mpz_mod(cur, cur, crtmod->crt_mod[i]);
      }
      mpz_divexact(coef, muls, crtmod->crt_mod[i]);
      mpz_mul(cur, cur, coef);
      mpz_invert(coef, coef, crtmod->crt_mod[i]);
      mpz_mul(cur, cur, coef);
      mpz_add(res, res, cur);
    }
    mpz_mod(res, res, muls);
    mpz_clears(cur, muls, coef, nullptr);
  }

void data_retrieve_crt(
  char**& seq,
  const mpz_t pack,
  const CrtMod* crtmod,
  const size_t data_size,
  const size_t str_size,
  const int radix) {
    if (data_size > crtmod->crt_size) {
      throw "size of packing is more than super_vector's";
    }
    seq = (char**)malloc(sizeof(char*) * data_size);
    mpz_t cur;
    mpz_init(cur);
    for (size_t i = 0; i < data_size; ++i) {
      seq[i] = nullptr;
      mpz_mod(cur, pack, crtmod->crt_mod[i]);
      if (mpz_cmp(cur, crtmod->crt_half_mod[i]) >= 0) {
        mpz_sub(cur, cur, crtmod->crt_mod[i]);
      }
      seq[i] = mpz_get_str(seq[i], radix, cur);
    }
    mpz_clear(cur);
  }

void free_crt(
  CrtMod* crtmod) {
    for (size_t i = 0; i < crtmod->crt_size; ++i) {
      mpz_clear(crtmod->crt_mod[i]);
      // mpz_clear(crtmod->crt_coef[i]);
      mpz_clear(crtmod->crt_half_mod[i]);
    }
    // mpz_clear(crtmod->muls);
    free(crtmod->crt_mod);
    crtmod->crt_mod = nullptr;
    // free(crtmod->crt_coef);
    // crtmod->crt_coef = nullptr;
    free(crtmod->crt_half_mod);
    crtmod->crt_half_mod = nullptr;
    free(crtmod);
    crtmod = nullptr;
  }
