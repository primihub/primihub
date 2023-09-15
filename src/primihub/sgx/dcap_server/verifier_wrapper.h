#ifndef SGX_DCAP_SERVER_VERIFIER_H_
#define SGX_DCAP_SERVER_VERIFIER_H_
#include <string>
bool quote_verify(const std::string &quote, const std::string &nonce,
                  const std::string &private_key_path, std::string *signature);
bool gen_cert(const std::string &privatekey_path, const std::string &root_ca_path, const std::string &csr,
              std::string *cert);

#endif  // SGX_DCAP_SERVER_VERIFIER_H_
