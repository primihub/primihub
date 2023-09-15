#ifndef SGX_ENGINE_TWO_PARTIES_PSI_H_
#define SGX_ENGINE_TWO_PARTIES_PSI_H_
#include <string>
#include <vector>
#include "opbase.h"

class twopc_psi : public OpBase {
 public:
  explicit twopc_psi(size_t parties, uint64_t gid): OpBase(parties, gid) {}
  ~twopc_psi();
  int init(const std::vector<std::string> &extra_info) override;
  int run(const std::vector<std::string>& files, size_t *result_size) override;

 private:
  size_t data_size_ = 0;
};
#endif  // SGX_ENGINE_TWO_PARTIES_PSI_H_
