// "Copyright [2023] <PrimiHub>"
#ifndef SRC_PRIMIHUB_UTIL_HASH_H_
#define SRC_PRIMIHUB_UTIL_HASH_H_
#include <openssl/evp.h>
#include <string>
namespace primihub {
class Hash {
 public:
  explicit Hash(const std::string& alg = "sha256");
  bool Init() { return md_ != nullptr;}
  std::string HashToString(const std::string &msg);

 private:
  int alg_{NID_sha256};
  const EVP_MD* md_{nullptr};
};
}  // namespace primihub
#endif  // SRC_PRIMIHUB_UTIL_HASH_H_
