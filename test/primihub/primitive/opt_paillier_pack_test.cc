#include "src/primihub/primitive/opt_paillier/include/paillier.h"
#include "src/primihub/primitive/opt_paillier/include/utils.h"
#include "src/primihub/primitive/opt_paillier/include/crt_datapack.h"
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

  int round = 100;
  mpz_t cipher_test;
  mpz_t cipher_test2;
  mpz_t plain_test;
  mpz_t decrypt_test;

  mpz_inits(cipher_test, cipher_test2, plain_test, decrypt_test, nullptr);
  std::default_random_engine e;
  std::uniform_int_distribution<long long> u(-922337203685477580, 922337203685477580);

  int max_dimension = 28;// dimension <= 28

  CrtMod* crtmod;
  init_crt(&crtmod, max_dimension, 70);
  char** nums1;
  char** nums2;
  char** test;
  char** adds;

  for (int d = 0; d <= max_dimension; ++d) {

    double encrypt_cost = 0.0, decrypt_cost = 0.0;

    for (int i = 1; i <= round; ++i) {
      size_t data_size = d;
      nums1 = (char**)malloc(sizeof(char*) * data_size);
      nums2 = (char**)malloc(sizeof(char*) * data_size);
      adds = (char**)malloc(sizeof(char*) * data_size);
      for (size_t j = 0; j < data_size; ++j) {
        nums1[j] = (char*)malloc(sizeof(char) * 32);
        long long cur1 = u(e);
        sprintf(nums1[j], "%lld", cur1);
        // std::cout << nums1[j] << std::endl;

        nums2[j] = (char*)malloc(sizeof(char) * 32);
        long long cur2 = u(e);
        sprintf(nums2[j], "%lld", cur2);

        adds[j] = (char*)malloc(sizeof(char) * 32);
        long long cur = cur1 + cur2;
        sprintf(adds[j], "%lld", cur);
      }
      mpz_t pack;
      mpz_init(pack);
      data_packing_crt(pack, nums1, data_size, crtmod);

      auto e_st = std::chrono::high_resolution_clock::now();
      opt_paillier_encrypt_crt_fb(cipher_test, pub, prv, pack);
      auto e_ed = std::chrono::high_resolution_clock::now();
      encrypt_cost += 1.0 * std::chrono::duration_cast<std::chrono::microseconds>(e_ed - e_st).count();

      auto d_st = std::chrono::high_resolution_clock::now();
      opt_paillier_decrypt_crt(decrypt_test, pub, prv, cipher_test);
      auto d_ed = std::chrono::high_resolution_clock::now();
      decrypt_cost += 1.0 * std::chrono::duration_cast<std::chrono::microseconds>(d_ed - d_st).count();

      data_retrieve_crt(test, decrypt_test, crtmod, data_size);

      for (size_t j = 0; j < data_size; ++j) {
        if (strcmp(test[j], nums1[j])) {
          std::cout << "error" << std::endl;
        }
      }

      for (size_t j = 0; j < data_size; ++j) {
        free(test[j]);
      }
      free(test);

      mpz_t pack2;
      mpz_init(pack2);
      data_packing_crt(pack2, nums2, data_size, crtmod);
      opt_paillier_encrypt(cipher_test2, pub, pack2);

      opt_paillier_add(cipher_test, cipher_test, cipher_test2, pub);

      opt_paillier_decrypt_crt(decrypt_test, pub, prv, cipher_test);

      data_retrieve_crt(test, decrypt_test, crtmod, data_size);

      for (size_t j = 0; j < data_size; ++j) {
        if (strcmp(test[j], adds[j])) {
          std::cout << "error" << std::endl;
          break;
        }
      }

      for (size_t j = 0; j < data_size; ++j) {
        free(nums1[j]);
        free(nums2[j]);
        free(adds[j]);
        free(test[j]);
      }
      free(nums1);
      free(nums2);
      free(adds);
      free(test);

      mpz_clear(pack);
      mpz_clear(pack2);
    }

    std::cout << "=========================opt test=========================" << std::endl;

    encrypt_cost = 1.0 / round * encrypt_cost;
    decrypt_cost = 1.0 / round * decrypt_cost;

    std::cout << "The total encryption cost is " << encrypt_cost / 1000.0 << " ms." << std::endl;
    std::cout << "The total decryption cost is " << decrypt_cost / 1000.0 << " ms." << std::endl;

    std::cout << "========================================================" << std::endl;

  }


  opt_paillier_freepubkey(pub);
  opt_paillier_freeprvkey(prv);

  mpz_clears(cipher_test, cipher_test2, plain_test, decrypt_test, nullptr);
  return 0;
}