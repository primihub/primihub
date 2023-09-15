//
// Created by root on 5/27/23.
//

#ifndef SGX_CERT_CERT_WRAPPER_H_
#define SGX_CERT_CERT_WRAPPER_H_
#include <string>
bool get_csr_privkey(const std::string &privkey_path, const std::string &cn, std::string *csr_buf, std::string *nonce);

#endif  //  SGX_CERT_CERT_WRAPPER_H_
