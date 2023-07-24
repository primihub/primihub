/**
  \file 		paillier.h
  \author 	Jiang Zhengliang
  \copyright Copyright (C) 2022 Jiang Zhengliang
 */

#ifndef __OPT_PAILLIER__
#define __OPT_PAILLIER__

#include <gmp.h>
#include <unordered_map>
#include <algorithm>
#include <vector>
#include "powmod.h"

using ui = unsigned int;


/**
 * @brief secret_parameter__MapTo__nbits_lbits
 * 
 */
extern 
std::unordered_map<ui, std::vector<ui>> mapTo_nbits_lbits;


/**
 * @brief Miller-Rabin parameter prob
 * 
 * identify as a prime with an asymptotic probability of less than 4^(-prob)
 * 
 */
extern 
ui prob;


/**
 * @brief TYPES
 * 
 * struct opt_public_key
 * 
 * struct opt_secret_key
 * 
 */
struct opt_public_key_t {
  ui nbits;
  ui lbits;
  mpz_t n;// n=P*Q
  mpz_t half_n;// plaintext domain
  /* for precomputing */
  mpz_t n_squared;// n=n^2
  mpz_t h_s;// h_s=(-y^2b)^n mod n^2

  /* fixed-base parameters */
  fb_instance fb_mod_P_sqaured;
  fb_instance fb_mod_Q_sqaured;
};

struct opt_secret_key_t {
  mpz_t p;// Len(p) = lbits/2, prime
  mpz_t q;// Len(q) = Len(p), prime
  mpz_t p_;// Len(p_) = nbits/2-lbits/2-1, odd number
  mpz_t q_;// L(q_) = L(p_), odd number
  mpz_t alpha;// alpha = p*q
  mpz_t beta;// beta = p_*q_
  mpz_t P;// P = 2*p*p_+1
  mpz_t Q;// Q = 2*q*q_+1
  /* for precomputing of encryption and decryption */
  mpz_t P_squared;
  mpz_t Q_squared;
  mpz_t double_alpha;
  mpz_t double_beta;
  mpz_t double_alpha_inverse;
  /* for precomputing of CRT */
  // P^2 * ((P^2)^-1 mod Q^2)
  mpz_t P_squared_mul_P_squared_inverse;
  // P * (P^-1 mod Q)
  mpz_t P_mul_P_inverse;
  /* for precomputing of advance */
  mpz_t double_p;
  mpz_t double_q;
  mpz_t Q_mul_double_p_inverse;
  mpz_t P_mul_double_q_inverse;
};

/**
 * @brief Assistant functions that CRT, Fixed-base
 * 
 */

void opt_paillier_mod_n_squared_crt(
  mpz_t res,
  const mpz_t base,
  const mpz_t exp,
  const opt_public_key_t* pub,
  const opt_secret_key_t* prv);

void opt_paillier_mod_n_squared_crt_fb(
  mpz_t res,
  const mpz_t exp,
  const opt_public_key_t* pub,
  const opt_secret_key_t* prv);

/**
 * @brief Optimizations of Paillier cryptosystem functions
 * 
 */

void opt_paillier_keygen(
  ui k_sec,
  opt_public_key_t** pub,
  opt_secret_key_t** prv);

void opt_paillier_set_plaintext(
  mpz_t mpz_plaintext,
  const char* plaintext,
  const opt_public_key_t* pub,
  int radix = 10);

void opt_paillier_get_plaintext(
  char* &plaintext,
  const mpz_t mpz_plaintext,
  const opt_public_key_t* pub,
  int radix = 10);

bool validate_message(
  mpz_t msg,
  opt_public_key_t* pub);

void opt_paillier_encrypt(
  mpz_t res,
  const opt_public_key_t* pub,
  const mpz_t plaintext);

void opt_paillier_encrypt_crt(
  mpz_t res,
  const opt_public_key_t* pub,
  const opt_secret_key_t* prv,
  const mpz_t plaintext);

void opt_paillier_encrypt_crt_fb(
  mpz_t res,
  const opt_public_key_t* pub,
  const opt_secret_key_t* prv,
  const mpz_t plaintext);

void opt_paillier_decrypt(
  mpz_t res,
  const opt_public_key_t* pub,
  const opt_secret_key_t* prv,
  const mpz_t ciphertext);

void opt_paillier_decrypt_crt(
  mpz_t res,
  const opt_public_key_t* pub,
  const opt_secret_key_t* prv,
  const mpz_t ciphertext);

void opt_paillier_add(
  mpz_t res,
  const mpz_t op1,
  const mpz_t op2,
  const opt_public_key_t* pub);

void opt_paillier_constant_mul(
  mpz_t res,
  const mpz_t op1,
  const mpz_t op2,
  const opt_public_key_t* pub);

/**
 * @brief memory manager
 * 
 */
void opt_paillier_freepubkey(
  opt_public_key_t* pub);

void opt_paillier_freeprvkey(
  opt_secret_key_t* prv);

#endif