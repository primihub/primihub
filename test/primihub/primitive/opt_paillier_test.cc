/**
  \file 		optPaillier.cc
  \author 	Jiang Zhengliang
  \copyright Copyright (C) 2022 Jiang Zhengliang
 */

#include "src/primihub/primitive/opt_paillier/include/paillier.h"
#include "src/primihub/primitive/opt_paillier/include/utils.h"
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <cstring>
#include <algorithm>

opt_public_key_t* pub;
opt_secret_key_t* prv;

double keygen_cost = 0.0;
double encrypt_cost = 0.0, encrypt_crt_cost = 0.0, decrypt_cost = 0.0;
int encrypt_times = 0, encrypt_crt_times = 0, decrypt_times = 0;
double add_cost = 0.0, mul_cost = 0.0;
int add_times = 0, mul_times = 0;

std::default_random_engine e;
std::uniform_int_distribution<long long> u((-922337203685477580, 922337203685477580));

void key_gen() {

  auto keygen_st = std::chrono::high_resolution_clock::now();
  opt_paillier_keygen(112, &pub, &prv);
  auto keygen_ed = std::chrono::high_resolution_clock::now();

  keygen_cost += 1.0 * std::chrono::duration_cast<std::chrono::microseconds>(keygen_ed - keygen_st).count();
  std::cout << "==================KeyGen is finished==================" << std::endl;
  std::cout << "KeyGen costs: " << keygen_cost / 1000.0 << " ms." << std::endl;
}

void test_encrypt_crt(mpz_t cipher_test, mpz_t plain_test) {
    mpz_init(cipher_test);
    auto start = std::chrono::high_resolution_clock::now();
    opt_paillier_encrypt_crt_fb(cipher_test, pub, prv, plain_test);
    auto end = std::chrono::high_resolution_clock::now();
    encrypt_crt_cost += 1.0 * std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    encrypt_crt_times++;
    // gmp_printf("Ciphertext = %Zd\n", cipher_test);
}

void test_encrypt(mpz_t cipher_test, mpz_t plain_test) {
    mpz_init(cipher_test);

    auto start = std::chrono::high_resolution_clock::now();
    opt_paillier_encrypt_crt(cipher_test, pub, prv, plain_test);
    auto end = std::chrono::high_resolution_clock::now();

    encrypt_cost += 1.0 * std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    encrypt_times++;
}

void test_add(mpz_t add_cipher_test, mpz_t cipher_test1, mpz_t cipher_test2) {
    mpz_init(add_cipher_test);

    auto start = std::chrono::high_resolution_clock::now();
    opt_paillier_add(add_cipher_test, cipher_test1, cipher_test2, pub);
    auto end = std::chrono::high_resolution_clock::now();
    add_cost += 1.0 * std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    add_times++;
}

void test_decrypt(mpz_t decrypt_test, mpz_t cipher_test) {
    mpz_init(decrypt_test);

    auto start = std::chrono::high_resolution_clock::now();
    // opt_paillier_decrypt(decrypt_test, pub, prv, cipher_test);
    opt_paillier_decrypt_crt(decrypt_test, pub, prv, cipher_test);
    auto end = std::chrono::high_resolution_clock::now();

    decrypt_cost += 1.0 * std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    decrypt_times++;
}

bool check_add_res(mpz_t plain_test1, mpz_t plain_test2, mpz_t decrypt_add_test) {
    mpz_t add_plain_test;
    mpz_init(add_plain_test);

    mpz_add(add_plain_test, plain_test1, plain_test2);
    mpz_mod(add_plain_test, add_plain_test, pub->n);
    if (0 != mpz_cmp(decrypt_add_test, add_plain_test)) {
      std::cout << "Error" << std::endl;
      return false;
    }

    mpz_clear(add_plain_test);
    return true;
}

void test_cons_mul(mpz_t mul_cipher_test, mpz_t cipher_test1, mpz_t plain_test2) {
    mpz_init(mul_cipher_test);

    auto start = std::chrono::high_resolution_clock::now();
    opt_paillier_constant_mul(mul_cipher_test, cipher_test1, plain_test2, pub);
    auto end = std::chrono::high_resolution_clock::now();
    mul_cost += 1.0 * std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    mul_times++;
}

bool check_cons_mul_res(mpz_t decrypt_mul_test, std::string op1, std::string op2) {
    char *str_test, *str_plaintext;

    opt_paillier_get_plaintext(str_test, decrypt_mul_test, pub);

    mpz_t plain_op1, plain_op2;
    mpz_inits(plain_op1, plain_op2, nullptr);
    mpz_set_str(plain_op1, op1.c_str(), 10);
    mpz_set_str(plain_op2, op2.c_str(), 10);
    mpz_mul(plain_op1, plain_op1, plain_op2);
    mpz_mod(plain_op1, plain_op1, pub->n);
    opt_paillier_get_plaintext(str_plaintext, plain_op1, pub);

    if (strcmp(str_test, str_plaintext)) {
      std::cout << op1 << " " << op2 << std::endl;
      std::cout << str_test << " " << str_plaintext << std::endl;
      std::cout << "string error" << std::endl;
      return false;
    }

    mpz_clears(plain_op1, plain_op2, nullptr);
    return true;
}

void convert_op2zq(mpz_t plain_test, std::string op) {
    mpz_init(plain_test);
    opt_paillier_set_plaintext(plain_test, op.c_str(), pub);
}

int main() {
  key_gen();

  int round = 1000;
  for (int i = 1; i <= round; ++i) {
    std::string op1, op2;
    op1 = std::to_string(u(e));
    op2 = std::to_string(u(e));

    // convert op1 op2 to z^q
    mpz_t plain_test1, plain_test2;
    convert_op2zq(plain_test1, op1);
    convert_op2zq(plain_test2, op2);

    // encrypt op1 and op2
    mpz_t cipher_test1;
    test_encrypt_crt(cipher_test1, plain_test1);
    mpz_t cipher_test2;
    test_encrypt(cipher_test2, plain_test2);
    // add cipher1 and cipher2
    mpz_t add_cipher_test;
    test_add(add_cipher_test, cipher_test1, cipher_test2);
    // decry the add result
    mpz_t decrypt_add_test;
    test_decrypt(decrypt_add_test, add_cipher_test);
    // check add result
    check_add_res(plain_test1, plain_test2, decrypt_add_test);

    // mul cipher1 with cons op2
    mpz_t mul_cipher_test;
    test_cons_mul(mul_cipher_test, cipher_test1, plain_test2);
    // decrypt mul res
    mpz_t decrypt_mul_test;
    test_decrypt(decrypt_mul_test, mul_cipher_test);
    // check mul result
    check_cons_mul_res(decrypt_mul_test, op1, op2);

    mpz_clears(plain_test1, plain_test2, cipher_test1, cipher_test2, add_cipher_test, decrypt_add_test, mul_cipher_test, decrypt_mul_test, nullptr);
  }

  std::cout << "=========================opt test=========================" << std::endl;

  encrypt_cost = 1.0 / encrypt_times * encrypt_cost;
  encrypt_crt_cost = 1.0 / encrypt_crt_times * encrypt_crt_cost;
  decrypt_cost = 1.0 / decrypt_times * decrypt_cost;
  add_cost = 1.0 / add_times * add_cost;
  mul_cost = 1.0 / mul_times * mul_cost;

  std::cout << "The avg encrypt_crt  cost is " << encrypt_crt_cost / 1000.0 << " ms." << std::endl;
  std::cout << "The avg encryption   cost is " << encrypt_cost / 1000.0 << " ms." << std::endl;
  std::cout << "The avg add_cost     cost is " << add_cost / 1000.0 << " ms." << std::endl;
  std::cout << "The avg mul_cost     cost is " << mul_cost / 1000.0 << " ms." << std::endl;
  std::cout << "The avg decrypt_cost cost is " << decrypt_cost / 1000.0 << " ms." << std::endl;

  std::cout << "========================================================" << std::endl;

  opt_paillier_freepubkey(pub);
  opt_paillier_freeprvkey(prv);
  return 0;
}
