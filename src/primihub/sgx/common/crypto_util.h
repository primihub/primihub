#ifndef SGX_COMMON_CRYPTO_UTIL_H_
#define SGX_COMMON_CRYPTO_UTIL_H_
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/asn1.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <string>

class EnDecrypt {
 public:
  explicit EnDecrypt(SYM_ALG_TYPE s_type, const std::string &private_key_str,
                     ASYM_ALG_TYPE a_type = ASYM_ALG_TYPE::RSA_3072, HASH_ALG_TYPE h_type = HASH_ALG_TYPE::SHA_256);
  explicit EnDecrypt(const std::string &private_key_str, ASYM_ALG_TYPE a_type = ASYM_ALG_TYPE::RSA_3072,
                     HASH_ALG_TYPE h_type = HASH_ALG_TYPE::SHA_256);
  explicit EnDecrypt(SYM_ALG_TYPE s_type);
  explicit EnDecrypt(HASH_ALG_TYPE h_type = HASH_ALG_TYPE::SHA_256);
  ~EnDecrypt();
  bool encrypt(const unsigned char *key, const size_t key_len, const unsigned char *plaintext, const size_t pt_len,
               const unsigned char *aad, const size_t aad_len, const unsigned char *iv, const size_t iv_len,
               unsigned char *encrypted, size_t *enc_len, unsigned char *mac, size_t *mac_len);
  bool decrypt(const unsigned char *key, const size_t key_len, const unsigned char *encrypted, const size_t enc_len,
               const unsigned char *aad, const size_t aad_len, const unsigned char *iv, const size_t iv_len,
               const unsigned char *mac, const size_t mac_len, unsigned char *plaintext, size_t *pt_len);

  bool gen_msg_digest(const std::string &msg, std::string *digest);
  bool verify(const std::string &msg, const std::string &cert, const unsigned char *signature, size_t sig_len);
  bool sign(const std::string &msg, unsigned char *signature, size_t *sig_len);
  bool get_random(unsigned char *rand, size_t len);
  bool get_quote_from_certstring(const std::string &cert, std::string*quote);

 private:
  const EVP_MD *md_ = nullptr;
  const EVP_CIPHER *cipher_ = nullptr;
  EVP_PKEY *pkey_ = nullptr;
  EVP_PKEY_CTX *pkctx_ = nullptr;
  bool get_prikey_from_keystring(const std::string &prikey, const std::string &password);
  bool get_pubkey_from_certstring(const std::string &cert);
};

const std::string ssl_base64encode(const std::string &buffer);
const std::string ssl_base64decode(const std::string &buffer);

int get_sgx_file_content(const std::string &filename, std::string *content);
bool gen_keypair(char *pubkey_hash, std::string* privkey_buf, char *pubkey_buf, size_t *pubkey_len);
bool gen_csr(const char* pubkey, size_t pubkey_len, const std::string &privatekey, const char* quote,
             size_t quote_len, const char*szCommon, size_t cn_len, char *csr_buf);
#endif  // SGX_COMMON_CRYPTO_UTIL_H_
