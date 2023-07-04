#ifndef SGX_UTIL_SGX_UTIL_H_
#define SGX_UTIL_SGX_UTIL_H_
#include <glog/logging.h>
#include <string>

int init_enclave(const std::string &enclave_path, uint64_t *);
void exit_enclave(uint64_t gid);
#endif  // SGX_UTIL_SGX_UTIL_H_
