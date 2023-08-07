// "Copyright [2023] <PrimiHub>"
#include "src/primihub/util/hash.h"
#include <memory>
#include <openssl/ossl_typ.h>
#include <openssl/sha.h>
#include <glog/logging.h>
namespace primihub {
std::string Hash::HashToString(const std::string& msg) {
  unsigned int dgstlen = EVP_MD_meth_get_result_size(md_);
  if (0 == dgstlen) {
    LOG(ERROR) << "unexpected size from EVP_MD_meth_get_result_size";
    return std::string();
  }
  std::string hash_res;
  auto dgst = std::make_unique<unsigned char[]>(dgstlen);
  memset(dgst.get(), 0x0, dgstlen);
  int ret = EVP_Digest(reinterpret_cast<const unsigned char *>(msg.c_str()),
                       msg.size(), dgst.get(), &dgstlen, md_, nullptr);
  if (!ret) {
    LOG(ERROR) << "EVP_Digest return err" << ret;
    return std::string();
  }
  hash_res = std::string(reinterpret_cast<char*>(dgst.get()), dgstlen);
  return hash_res;
}
}  // namespace primihub
