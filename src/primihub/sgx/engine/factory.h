#ifndef SGX_ENGINE_FACTORY_H_
#define SGX_ENGINE_FACTORY_H_

#include <string>
#include <memory>
#include "two_parties_psi.h"

class Opfactory {
 public:
  static std::shared_ptr <OpBase> create(const std::string &code, size_t parties, uint64_t gid) {
    if (code == "psi" && parties == 2) {
      return std::make_shared<twopc_psi>(parties, gid);
    } else { return nullptr; }
  }
};

#endif  // SGX_ENGINE_FACTORY_H_
