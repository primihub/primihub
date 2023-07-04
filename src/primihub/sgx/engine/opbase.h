#ifndef SGX_ENGINE_OPBASE_H_
#define SGX_ENGINE_OPBASE_H_

#include <vector>
#include <string>

class OpBase {
 public:
  explicit OpBase(size_t parties, uint64_t gid) : parties_(parties), gid_(gid) {}
  virtual ~OpBase() = default;
  virtual int init(const std::vector <std::string> &extra_info) = 0;
  virtual int run(const std::vector <std::string> &files, size_t *result_size) = 0;

 protected:
  size_t parties_ = 0;
  uint64_t gid_ = 0;
};
#endif  // SGX_ENGINE_OPBASE_H_
