#ifndef SGX_ENGINE_ENGINE_WRAPPER_H_
#define SGX_ENGINE_ENGINE_WRAPPER_H_
#include <string>
#include <vector>
bool init_engine_enclave(uint64_t *gid);
bool _2pc_psi(uint64_t gid, const std::vector <std::string> &files, size_t signle_data_size, size_t *result_size);
#endif  // SGX_ENGINE_ENGINE_WRAPPER_H_
