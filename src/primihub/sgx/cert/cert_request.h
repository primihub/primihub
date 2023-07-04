
#ifndef SGX_CERT_CERT_REQUEST_H_
#define SGX_CERT_CERT_REQUEST_H_
#include <string>
bool gen_csr(const std::string &enclave_key_path, const std::string &cn);
#endif  //  SGX_CERT_CERT_REQUEST_H_
