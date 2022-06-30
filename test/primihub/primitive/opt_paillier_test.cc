/**
  \file 		optPaillier.cc
  \author 	Jiang Zhengliang
  \copyright Copyright (C) 2022 Jiang Zhengliang
 */

#include "paillier.h"
#include "utils.h"
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <cstring>
#include <algorithm>

int main() {
  opt_public_key_t* pub;
  opt_secret_key_t* prv;

  double keygen_cost = 0.0;
  auto keygen_st = std::chrono::high_resolution_clock::now();
  opt_paillier_keygen(112, &pub, &prv);
  auto keygen_ed = std::chrono::high_resolution_clock::now();
  keygen_cost += 1.0 * std::chrono::duration_cast<std::chrono::microseconds>(keygen_ed - keygen_st).count();

  std::cout << "==================KeyGen is finished==================" << std::endl;
  std::cout << "KeyGen costs: " << keygen_cost / 1000.0 << " ms." << std::endl;

  int round = 1000;
  mpz_t plain_test1;
  mpz_t plain_test2;
  mpz_t cipher_test1;
  mpz_t cipher_test2;
  mpz_t decrypt_test;
  mpz_init(plain_test1);
  mpz_init(plain_test2);
  mpz_init(cipher_test1);
  mpz_init(cipher_test2);
  mpz_init(decrypt_test);
  double encrypt_cost = 0.0, encrypt_crt_cost = 0.0, decrypt_cost = 0.0, add_cost;
  std::default_random_engine e;
  std::uniform_int_distribution<long long> u(-1000, 1000);
  for (int i = 1; i <= round; ++i) {
    // std::cout << "==============================================================" << std::endl;
    std::string op1 = std::to_string(u(e));
    opt_paillier_set_plaintext(plain_test1, op1.c_str(), pub);

    auto start = std::chrono::high_resolution_clock::now();
    // opt_paillier_encrypt_crt(cipher_test1, pub, prv, plain_test1);
    opt_paillier_encrypt_crt_fb(cipher_test1, pub, prv, plain_test1);
    auto end = std::chrono::high_resolution_clock::now();
    encrypt_crt_cost += 1.0 * std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    // gmp_printf("Ciphertext = %Zd\n", cipher_test);

    std::string op2 = std::to_string(u(e));
    opt_paillier_set_plaintext(plain_test2, op2.c_str(), pub);

    start = std::chrono::high_resolution_clock::now();
    opt_paillier_encrypt(cipher_test2, pub, plain_test2);
    // opt_paillier_encrypt_crt_fb(cipher_test2, pub, prv, plain_test2);
    end = std::chrono::high_resolution_clock::now();
    encrypt_cost += 1.0 * std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    start = std::chrono::high_resolution_clock::now();
    opt_paillier_add(cipher_test1, cipher_test1, cipher_test2, pub);
    end = std::chrono::high_resolution_clock::now();
    add_cost += 1.0 * std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    mpz_add(plain_test1, plain_test1, plain_test2);
    mpz_mod(plain_test1, plain_test1, pub->n);

    start = std::chrono::high_resolution_clock::now();
    // opt_paillier_decrypt(decrypt_test, pub, prv, cipher_test);
    opt_paillier_decrypt_crt(decrypt_test, pub, prv, cipher_test1);
    end = std::chrono::high_resolution_clock::now();
    decrypt_cost += 1.0 * std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    // gmp_printf("Plaintext = %Zd\n", decrypt_test);

    if (0 != mpz_cmp(decrypt_test, plain_test1)) {
      std::cout << "Error" << std::endl;
    }

    char *str_test;
    opt_paillier_get_plaintext(str_test, decrypt_test, pub);
    char *str_plaintext;
    opt_paillier_get_plaintext(str_plaintext, plain_test1, pub);

    if (strcmp(str_test, str_plaintext)) {
      std::cout << "string error" << std::endl;
    }
  }
   mpz_clears(decrypt_test, cipher_test1, cipher_test2, plain_test1, plain_test2, nullptr);

  std::cout << "=========================opt test=========================" << std::endl;

  encrypt_cost = 1.0 / round * encrypt_cost;
  encrypt_crt_cost = 1.0 / round * encrypt_crt_cost;
  decrypt_cost = 1.0 / round * decrypt_cost;
  add_cost = 1.0 / round * add_cost;

  std::cout << "The avg encrypt_crt  cost is " << encrypt_crt_cost / 1000.0 << " ms." << std::endl;
  std::cout << "The avg encryption   cost is " << encrypt_cost / 1000.0 << " ms." << std::endl;
  std::cout << "The avg add_cost     cost is " << add_cost / 1000.0 << " ms." << std::endl;
  std::cout << "The avg decrypt_cost cost is " << decrypt_cost / 1000.0 << " ms." << std::endl;

  std::cout << "========================================================" << std::endl;
  
  opt_paillier_freepubkey(pub);
  opt_paillier_freeprvkey(prv);
  return 0;
}