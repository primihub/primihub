/**
  \file 		paillier.cc
  \author 	Jiang Zhengliang
  \copyright Copyright (C) 2022 Jiang Zhengliang
 */

#include "../include/paillier.h"
#include "../include/utils.h"
#include "../include/powmod.h"
#include <cstdlib>
#include <chrono>
#include <iostream>

std::unordered_map<ui, std::vector<ui>>
mapTo_nbits_lbits = {
  {112, {2048, 448}},
  {128, {3072, 512}},
  {192, {7680, 768}}
};

ui
prob = 30;

void opt_paillier_mod_n_squared_crt(
  mpz_t res,
  const mpz_t base,
  const mpz_t exp,
  const opt_public_key_t* pub,
  const opt_secret_key_t* prv) {
    /* temp variable */
    mpz_t temp_base, cp, cq;
    mpz_inits(cq, cp, temp_base, nullptr);

    /* smaller exponentiations of base mod P^2, Q^2 */
    mpz_mod(temp_base, base, prv->P_squared);
    mpz_powm(cp, temp_base, exp, prv->P_squared);

    mpz_mod(temp_base, base, prv->Q_squared);
    mpz_powm(cq, temp_base, exp, prv->Q_squared);

    /* CRT to calculate base^exp mod n^2 */
    mpz_sub(cq, cq, cp);
    mpz_addmul(cp, cq, prv->P_squared_mul_P_squared_inverse);
    mpz_mod(res, cp, pub->n_squared);

    mpz_clears(cq, cp, temp_base, nullptr);
  }

void opt_paillier_mod_n_squared_crt_fb(
  mpz_t res,
  const mpz_t exp,
  const opt_public_key_t* pub,
  const opt_secret_key_t* prv) {
    /* temp variable */
    mpz_t cp, cq;
    mpz_inits(cq, cp, nullptr);

    /* smaller exponentiations of base mod P^2, Q^2 */
    fbpowmod_extend(pub->fb_mod_P_sqaured, cp, exp);

    fbpowmod_extend(pub->fb_mod_Q_sqaured, cq, exp);

    /* CRT to calculate base^exp mod n^2 */
    mpz_sub(cq, cq, cp);
    mpz_addmul(cp, cq, prv->P_squared_mul_P_squared_inverse);
    mpz_mod(res, cp, pub->n_squared);

    mpz_clears(cq, cp, nullptr);
  }

void opt_paillier_keygen(
  ui k_sec,
  opt_public_key_t** pub,
  opt_secret_key_t** prv) {
    /* temp variable */
    mpz_t temp;
    mpz_t y;
    mpz_inits(y, temp, nullptr);

    /* allocate the new key structures */
    *pub = (opt_public_key_t*)malloc(sizeof(opt_public_key_t));
    *prv = (opt_secret_key_t*)malloc(sizeof(opt_secret_key_t));
    
    /* initialize some secret parameters */
    (*pub)->nbits = mapTo_nbits_lbits[k_sec][0];
    (*pub)->lbits = mapTo_nbits_lbits[k_sec][1];

    /* initialize some integers */
    mpz_init((*pub)->h_s);
    mpz_init((*pub)->n);
    mpz_init((*pub)->n_squared);
    mpz_init((*pub)->half_n);

    mpz_init((*prv)->alpha);
    mpz_init((*prv)->beta);
    mpz_init((*prv)->p);
    mpz_init((*prv)->p_);
    mpz_init((*prv)->q);
    mpz_init((*prv)->q_);
    mpz_init((*prv)->P);
    mpz_init((*prv)->Q);
    mpz_init((*prv)->P_squared);
    mpz_init((*prv)->Q_squared);
    mpz_init((*prv)->double_alpha);
    mpz_init((*prv)->double_beta);
    mpz_init((*prv)->double_alpha_inverse);
    mpz_init((*prv)->P_mul_P_inverse);
    mpz_init((*prv)->P_squared_mul_P_squared_inverse);
    mpz_init((*prv)->double_p);
    mpz_init((*prv)->double_q);
    mpz_init((*prv)->Q_mul_double_p_inverse);
    mpz_init((*prv)->P_mul_double_q_inverse);

    ui l_dividetwo = (*pub)->lbits / 2;
    ui n_dividetwo = (*pub)->nbits / 2;

    do {
      aby_prng((*prv)->p_, n_dividetwo - l_dividetwo - 1);
      mpz_setbit((*prv)->p_, n_dividetwo - l_dividetwo - 1);
      mpz_nextprime((*prv)->p_, (*prv)->p_);

      aby_prng((*prv)->q_, n_dividetwo - l_dividetwo - 1);
      mpz_setbit((*prv)->q_, n_dividetwo - l_dividetwo - 1);
      mpz_nextprime((*prv)->q_, (*prv)->q_);
      
    } while (!mpz_cmp((*prv)->p_, (*prv)->q_));

    do {
      aby_prng((*prv)->p, l_dividetwo);
      mpz_setbit((*prv)->p, l_dividetwo);
      mpz_nextprime((*prv)->p, (*prv)->p);

      mpz_mul(temp, (*prv)->p, (*prv)->p_);
      mpz_mul_ui(temp, temp, 2);
      mpz_add_ui((*prv)->P, temp, 1);

      if (!mpz_probab_prime_p((*prv)->P, prob)) {
        continue;
      }
      break;

    } while (true);

    do {
      aby_prng((*prv)->q, l_dividetwo);
      mpz_setbit((*prv)->q, l_dividetwo);
      mpz_nextprime((*prv)->q, (*prv)->q);

      if (!mpz_cmp((*prv)->p, (*prv)->q)) {
        continue;
      }

      mpz_mul(temp, (*prv)->q, (*prv)->q_);
      mpz_mul_ui(temp, temp, 2);
      mpz_add_ui((*prv)->Q, temp, 1);

      if (!mpz_probab_prime_p((*prv)->Q, prob)) {
        continue;
      }
      break;

    } while(true);

    // alpha = p*q, beta = p_ * q_
    mpz_mul((*prv)->alpha, (*prv)->p, (*prv)->q);
    mpz_mul((*prv)->beta, (*prv)->p_, (*prv)->q_);

    // double_alpha = 2*alpha, double_beta = 2*beta
    mpz_mul_ui((*prv)->double_alpha, (*prv)->alpha, 2);
    mpz_mul_ui((*prv)->double_beta, (*prv)->beta, 2);

    // P_squared = P^2, Q_squared = Q^2
    mpz_mul((*prv)->P_squared, (*prv)->P, (*prv)->P);
    mpz_mul((*prv)->Q_squared, (*prv)->Q, (*prv)->Q);

    // double_p = p*2, double_q = q*2
    mpz_mul_ui((*prv)->double_p, (*prv)->p, 2);
    mpz_mul_ui((*prv)->double_q, (*prv)->q, 2);

    // n = P*Q, n_sqaured = n^2
    mpz_mul((*pub)->n, (*prv)->P, (*prv)->Q);
    mpz_mul((*pub)->n_squared, (*pub)->n, (*pub)->n);

    // half_n = n / 2, is plaintext domain
    mpz_div_ui((*pub)->half_n, (*pub)->n, 2);

    /* pick random y in Z_n^* */
    do {
      aby_prng(y, mpz_sizeinbase((*pub)->n, 2) + 128);
      mpz_mod(y, y, (*pub)->n);
      mpz_gcd(temp, y, (*pub)->n);
    } while (mpz_cmp_ui(temp, 1));

    // hs = (-y^(double_beta))^n mod n_squared
    mpz_powm((*pub)->h_s, y, (*prv)->double_beta, (*pub)->n);
    mpz_neg((*pub)->h_s, (*pub)->h_s);
    mpz_powm((*pub)->h_s, (*pub)->h_s, (*pub)->n, (*pub)->n_squared);

    // P_mul_P_inverse = P*(P^-1 mod Q)
    mpz_invert((*prv)->P_mul_P_inverse, (*prv)->P, (*prv)->Q);
    mpz_mul((*prv)->P_mul_P_inverse, (*prv)->P_mul_P_inverse, (*prv)->P);

    // P_squared_mul_P_squared_inverse = P^2 * (P^2)^-1 mod Q^2
    mpz_invert((*prv)->P_squared_mul_P_squared_inverse, 
              (*prv)->P_squared, (*prv)->Q_squared);
    mpz_mul((*prv)->P_squared_mul_P_squared_inverse, 
            (*prv)->P_squared_mul_P_squared_inverse, 
            (*prv)->P_squared);

    // double_alpha^-1 mod n
    mpz_invert((*prv)->double_alpha_inverse, (*prv)->double_alpha, (*pub)->n);

    // Q_mul_double_p_inverse = (Q * double_p)^-1 mod P)
    mpz_mul((*prv)->Q_mul_double_p_inverse, (*prv)->Q, (*prv)->double_p);
    mpz_invert((*prv)->Q_mul_double_p_inverse,
              (*prv)->Q_mul_double_p_inverse,
              (*prv)->P);

    // P_mul_double_q_inverse = (P * double_q)^-1 mod Q)
    mpz_mul((*prv)->P_mul_double_q_inverse, (*prv)->P, (*prv)->double_q);
    mpz_invert((*prv)->P_mul_double_q_inverse, 
              (*prv)->P_mul_double_q_inverse, 
              (*prv)->Q);

    mpz_clears(temp, y, nullptr);

    /* init fixed-base */
    mpz_t base_P_sqaured, base_Q_sqaured;
    mpz_inits(base_P_sqaured, base_Q_sqaured, nullptr);
    mpz_mod(base_P_sqaured, (*pub)->h_s, (*prv)->P_squared);
    mpz_mod(base_Q_sqaured, (*pub)->h_s, (*prv)->Q_squared);
    fbpowmod_init_extend((*pub)->fb_mod_P_sqaured,
      base_P_sqaured, (*prv)->P_squared, (*pub)->lbits + 1, 4);
    fbpowmod_init_extend((*pub)->fb_mod_Q_sqaured,
      base_Q_sqaured, (*prv)->Q_squared, (*pub)->lbits + 1, 4);
    mpz_clears(base_P_sqaured, base_Q_sqaured, nullptr);
  }

void opt_paillier_set_plaintext(
  mpz_t mpz_plaintext,
  const char* plaintext,
  const opt_public_key_t* pub,
  int radix) {
    mpz_set_str(mpz_plaintext, plaintext, radix);
    if (mpz_cmp_ui(mpz_plaintext, 0) < 0) {
      mpz_add(mpz_plaintext, mpz_plaintext, pub->n);
      mpz_mod(mpz_plaintext, mpz_plaintext, pub->n);
    }
  }

void opt_paillier_get_plaintext(
  char* &plaintext,
  const mpz_t mpz_plaintext,
  const opt_public_key_t* pub,
  int radix) {
    mpz_t temp;
    mpz_init(temp);
    if (mpz_cmp(mpz_plaintext, pub->half_n) >= 0) {
      mpz_sub(temp, mpz_plaintext, pub->n);
      plaintext = mpz_get_str(nullptr, radix, temp);
    }
    else {
      plaintext = mpz_get_str(nullptr, radix, mpz_plaintext);
    }
    mpz_clear(temp);
  }

bool validate_message(
  mpz_t msg,
  opt_public_key_t* pub) {
    if (mpz_cmp_ui(msg, 0) < 0) {
      return false;
    }
    if (mpz_cmp(msg, pub->n) >= 0) {
      return false;
    }
    return true;
  }

void opt_paillier_encrypt(
  mpz_t res,
  const opt_public_key_t* pub,
  const mpz_t plaintext) {
    /* temp variable */
    mpz_t r;
    mpz_init(r);
    aby_prng(r, pub->lbits);
    // res = m*n
    mpz_mul(res, plaintext, pub->n);
    // res = (1+m*n)
    mpz_add_ui(res, res, 1);
    // r = hs^r mod n^2
    mpz_powm(r, pub->h_s, r, pub->n_squared);
    // res = (1+m*n)*hs^r mod n^2
    mpz_mul(res, res, r);
    mpz_mod(res, res, pub->n_squared);
    mpz_clear(r);
  }

void opt_paillier_encrypt_crt(
  mpz_t res,
  const opt_public_key_t* pub,
  const opt_secret_key_t* prv,
  const mpz_t plaintext) {
    /* temp variable */
    mpz_t r;
    mpz_init(r);
    aby_prng(r, pub->lbits);
    mpz_mul(res, plaintext, pub->n);
    mpz_add_ui(res, res, 1);
    // r=hs^r mod n^2 => hs = hs mod p^2,q^2, r = r mod phi(n^2)
    opt_paillier_mod_n_squared_crt(
      r, pub->h_s, r, pub, prv);
    mpz_mul(res, res, r);
    mpz_mod(res, res, pub->n_squared);
    mpz_clear(r);
  }

void opt_paillier_encrypt_crt_fb(
  mpz_t res,
  const opt_public_key_t* pub,
  const opt_secret_key_t* prv,
  const mpz_t plaintext) {
    /* temp variable */
    mpz_t r;
    mpz_init(r);
    aby_prng(r, pub->lbits);
    mpz_mul(res, plaintext, pub->n);
    mpz_add_ui(res, res, 1);

    opt_paillier_mod_n_squared_crt_fb(
      r, r, pub, prv);

    mpz_mul(res, res, r);
    mpz_mod(res, res, pub->n_squared);

    mpz_clear(r);
  }

void opt_paillier_decrypt(
  mpz_t res,
  const opt_public_key_t* pub,
  const opt_secret_key_t* prv,
  const mpz_t ciphertext) {
    mpz_powm(res, ciphertext, prv->double_alpha, pub->n_squared);
    mpz_sub_ui(res, res, 1);
    mpz_divexact(res, res, pub->n);
    mpz_mul(res, res, prv->double_alpha_inverse);
    mpz_mod(res, res, pub->n);

  }

void opt_paillier_decrypt_crt(
  mpz_t res,
  const opt_public_key_t* pub,
  const opt_secret_key_t* prv,
  const mpz_t ciphertext) {
    /* temp variable and CRT variable */
    mpz_t temp_base, cp, cq;
    mpz_inits(cq, cp, temp_base, nullptr);

    // temp = c mod P^2
    mpz_mod(temp_base, ciphertext, prv->P_squared);
    // cp = temp_p^(2p) mod P^2
    mpz_powm(cp, temp_base, prv->double_p, prv->P_squared);
    // L(cp,P)
    mpz_sub_ui(cp, cp, 1);
    mpz_divexact(cp, cp, prv->P);
    // cp = cp * inv1 mod P
    mpz_mul(cp, cp, prv->Q_mul_double_p_inverse);
    mpz_mod(cp, cp, prv->P);

    // temp = c mod Q^2
    mpz_mod(temp_base, ciphertext, prv->Q_squared);
    // cq = temp_q^(2q) mod Q^2
    mpz_powm(cq, temp_base, prv->double_q, prv->Q_squared);
    // L(cq, Q)
    mpz_sub_ui(cq, cq, 1);
    mpz_divexact(cq, cq, prv->Q);
    // cq = cq * inv2 mod Q
    mpz_mul(cq, cq, prv->P_mul_double_q_inverse);
    mpz_mod(cq, cq, prv->Q);

    // cq = cq - cp
    mpz_sub(cq, cq, cp);
    // cp = cp + cq * (P * (P^-1 mod Q)) mod n
    mpz_addmul(cp, cq, prv->P_mul_P_inverse);
    mpz_mod(res, cp, pub->n);

    mpz_clears(cq, cp, temp_base, nullptr);
  }

void opt_paillier_add(
  mpz_t res,
  const mpz_t op1,
  const mpz_t op2,
  const opt_public_key_t* pub) {
    mpz_mul(res, op1, op2);
    mpz_mod(res, res, pub->n_squared);
  }

void opt_paillier_constant_mul(
  mpz_t res,
  const mpz_t op1,
  const mpz_t op2,
  const opt_public_key_t* pub) {
    mpz_powm(res, op1, op2, pub->n_squared);
  }

void opt_paillier_freepubkey(
  opt_public_key_t* pub) {
    mpz_clear(pub->h_s);
    mpz_clear(pub->n);
    mpz_clear(pub->n_squared);
    mpz_clear(pub->half_n);
    fbpowmod_end_extend(pub->fb_mod_P_sqaured);
    fbpowmod_end_extend(pub->fb_mod_Q_sqaured);
    free(pub);
    pub = nullptr;
  }

void opt_paillier_freeprvkey(
  opt_secret_key_t* prv) {
    mpz_clear(prv->alpha);
    mpz_clear(prv->double_alpha_inverse);
    mpz_clear(prv->beta);
    mpz_clear(prv->double_alpha);
    mpz_clear(prv->double_beta);
    mpz_clear(prv->P);
    mpz_clear(prv->p);
    mpz_clear(prv->p_);
    mpz_clear(prv->P_mul_P_inverse);
    mpz_clear(prv->P_squared);
    mpz_clear(prv->P_squared_mul_P_squared_inverse);
    mpz_clear(prv->Q);
    mpz_clear(prv->q);
    mpz_clear(prv->q_);
    mpz_clear(prv->Q_squared);
    mpz_clear(prv->double_p);
    mpz_clear(prv->double_q);
    mpz_clear(prv->Q_mul_double_p_inverse);
    mpz_clear(prv->P_mul_double_q_inverse);
    free(prv);
    prv = nullptr;
  }
