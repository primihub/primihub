// #include "../include/paillier.h"
// #include "../include/utils.h"
// #include "../include/crt_datapack.h"
// #include <chrono>
// #include <iostream>
// #include <random>
// #include <string>
// #include <cstring>
// #include <algorithm>

// int main() {
//   opt_public_key_t* pub;
//   opt_secret_key_t* prv;

//   double keygen_cost = 0.0;
//   auto keygen_st = std::chrono::high_resolution_clock::now();
//   opt_paillier_keygen(112, &pub, &prv);
//   auto keygen_ed = std::chrono::high_resolution_clock::now();
//   keygen_cost += 1.0 * std::chrono::duration_cast<std::chrono::microseconds>(keygen_ed - keygen_st).count();

//   std::cout << "==================KeyGen is finished==================" << std::endl;
//   std::cout << "KeyGen costs: " << keygen_cost / 1000.0 << " ms." << std::endl;

//   int round = 100;
//   mpz_t cipher_test;
//   mpz_t plain_test;
//   mpz_t decrypt_test;
//   double encrypt_cost = 0.0, decrypt_cost = 0.0;


//   mpz_inits(cipher_test, plain_test, decrypt_test, nullptr);
//   std::default_random_engine e;
//   std::uniform_int_distribution<long long> u(-922337203685477580, 922337203685477580);

//   CrtMod* crtmod;
//   init_crt(&crtmod, 20, 172);
//   char** nums;
//   char** test;
  
//   for (int i = 1; i <= round; ++i) {
//     size_t data_size = 10;
//     nums = (char**)malloc(sizeof(char*) * data_size);
//     for (size_t j = 0; j < data_size; ++j) {
//       nums[j] = (char*)malloc(sizeof(char) * 32);
//       long long cur = u(e);
//       sprintf(nums[j], "%lld", cur);
//       // std::cout << nums[j] << std::endl;
//     }
//     mpz_t pack;
//     mpz_init(pack);
//     data_packing_crt(pack, nums, data_size, crtmod);

//     auto e_st = std::chrono::high_resolution_clock::now();
//     opt_paillier_encrypt_crt_fb(cipher_test, pub, prv, pack);
//     auto e_ed = std::chrono::high_resolution_clock::now();
//     encrypt_cost += 1.0 * std::chrono::duration_cast<std::chrono::microseconds>(e_ed - e_st).count();

//     auto d_st = std::chrono::high_resolution_clock::now();
//     opt_paillier_decrypt_crt(decrypt_test, pub, prv, cipher_test);
//     auto d_ed = std::chrono::high_resolution_clock::now();
//     decrypt_cost += 1.0 * std::chrono::duration_cast<std::chrono::microseconds>(d_ed - d_st).count();

//     data_retrieve_crt(test, decrypt_test, crtmod, data_size, 32);

//     for (size_t j = 0; j < data_size; ++j) {
//       if (strcmp(test[j], nums[j])) {
//         std::cout << "error" << std::endl;
//       }
//     }

//     for (size_t j = 0; j < data_size; ++j) {
//       free(nums[j]);
//       free(test[j]);
//     }
//     free(nums);
//     free(test);

//     mpz_clear(pack);

//   }

//     std::cout << "=========================opt test=========================" << std::endl;

//   encrypt_cost = 1.0 / round * encrypt_cost;
//   decrypt_cost = 1.0 / round * decrypt_cost;

//   std::cout << "The total encryption cost is " << encrypt_cost / 1000.0 << " ms." << std::endl;
//   std::cout << "The total decryption cost is " << decrypt_cost / 1000.0 << " ms." << std::endl;

//   std::cout << "========================================================" << std::endl;

//   opt_paillier_freepubkey(pub);
//   opt_paillier_freeprvkey(prv);

//   mpz_clears(cipher_test, plain_test, decrypt_test, nullptr);
//   return 0;
// }