// #include "../include/paillier.h"
// #include "../include/utils.h"
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

//   int round = 1000;
//   mpz_t plain_test1;
//   mpz_t plain_test2;
//   mpz_t cipher_test1;
//   mpz_t cipher_test2;
//   mpz_t decrypt_test;
//   mpz_init(plain_test1);
//   mpz_init(plain_test2);
//   mpz_init(cipher_test1);
//   mpz_init(cipher_test2);
//   mpz_init(decrypt_test);
//   // double encrypt_cost = 0.0, decrypt_cost = 0.0;
//   std::default_random_engine e;
//   std::uniform_int_distribution<long long> u(-1000, 1000);
//   for (int i = 1; i <= round; ++i) {
//     // std::cout << "==============================================================" << std::endl;
//     std::string op1 = std::to_string(u(e));
//     opt_paillier_set_plaintext(plain_test1, op1.c_str(), pub);

//     // auto e_st = std::chrono::high_resolution_clock::now();
//     opt_paillier_encrypt_crt_fb(cipher_test1, pub, prv, plain_test1);
//     // auto e_ed = std::chrono::high_resolution_clock::now();
//     // encrypt_cost += 1.0 * std::chrono::duration_cast<std::chrono::microseconds>(e_ed - e_st).count();
//     // gmp_printf("Ciphertext = %Zd\n", cipher_test);

//     std::string op2 = std::to_string(u(e));
//     opt_paillier_set_plaintext(plain_test2, op2.c_str(), pub);
//     opt_paillier_encrypt_crt_fb(cipher_test2, pub, prv, plain_test2);

//     // opt_paillier_constant_mul(cipher_test, cipher_test, plain_test2, pub);
//     opt_paillier_add(cipher_test1, cipher_test1, cipher_test2, pub);

//     mpz_add(plain_test1, plain_test1, plain_test2);
//     mpz_mod(plain_test1, plain_test1, pub->n);

//     // auto d_st = std::chrono::high_resolution_clock::now();
//     // opt_paillier_decrypt(decrypt_test, pub, prv, cipher_test);
//     opt_paillier_decrypt_crt(decrypt_test, pub, prv, cipher_test1);
//     // auto d_ed = std::chrono::high_resolution_clock::now();
//     // decrypt_cost += 1.0 * std::chrono::duration_cast<std::chrono::microseconds>(d_ed - d_st).count();
//     // gmp_printf("Plaintext = %Zd\n", decrypt_test);

//     // char str_test[256] = {'\0'};
//     // opt_paillier_get_plaintext(str_test, decrypt_test, pub);

//     if (0 != mpz_cmp(decrypt_test, plain_test1)) {
//       std::cout << "Error" << std::endl;
//     }

//     // if (strcmp(str_test, str_plaintext.c_str())) {
//     //   std::cout << "string error" << std::endl;
//     // }
//   }
//   mpz_clears(decrypt_test, cipher_test1, cipher_test2, plain_test1, plain_test2, nullptr);

//   std::cout << "=========================opt test=========================" << std::endl;

//   // encrypt_cost = 1.0 / round * encrypt_cost;
//   // decrypt_cost = 1.0 / round * decrypt_cost;

//   // std::cout << "The total encryption cost is " << encrypt_cost / 1000.0 << " ms." << std::endl;
//   // std::cout << "The total decryption cost is " << decrypt_cost / 1000.0 << " ms." << std::endl;

//   std::cout << "========================================================" << std::endl;

//   opt_paillier_freepubkey(pub);
//   opt_paillier_freeprvkey(prv);
//   return 0;
// }