
#ifndef SGX_RA_RA_ENCLAVE_WRAPPER_H_
#define SGX_RA_RA_ENCLAVE_WRAPPER_H_
#include <string>
bool init_(uint64_t *gid);
bool get_sharekey(uint64_t gid, const std::string &keyid, const std::string &certificate, std::string *enc_key);
bool set_sharekey(uint64_t gid, const std::string &privkey_path, const std::string &enc_key, const std::string &keyid);
bool get_signature(uint64_t gid, const std::string &sign_key_path, const std::string &msg, std::string *signature);

bool encrypt_data_wrapper(uint64_t gid, const std::string &keyid, const char *plain_text, size_t plain_size,
                          char *iv, size_t *iv_len,
                          char *mac, size_t *mac_len,
                          char *cipher_data, size_t cipher_size);

bool decrypt_data_wrapper(uint64_t gid, const std::string &keyid, const char *cipher_data, size_t cipher_size,
                          const char *iv, size_t iv_len, const char *mac, size_t mac_len,
                          char *plain_text, size_t plain_size);

bool decrypt_data_inside_wrapper(uint64_t gid, const std::string &keyid, const std::string &filename,
                                 const char *cipher_data, size_t cipher_size,
                                 const char *iv, size_t iv_len, const char *mac, size_t mac_len);

bool encrypt_data_inside_wrapper(uint64_t gid, const std::string &keyid, const std::string &filename,
                                 size_t offset, size_t plain_size, char *iv, size_t *iv_len,
                                 char *mac, size_t *mac_len, char *cipher_data, size_t cipher_size);
#endif  // SGX_RA_RA_ENCLAVE_WRAPPER_H_
