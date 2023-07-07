#ifndef SGX_ENGINE_SGX_ENGINE_H_
#define SGX_ENGINE_SGX_ENGINE_H_

#include <string>
#include <vector>
#include <atomic>
#include <memory>
#include "sgx/util/common_define.h"
#include "sgx/ra/ra_util.h"

namespace sgx {

enum engine_role_t {
  TEE_DATA_SERVER = 1,
  TEE_EXECUTOR = 2,
};

class TeeEngine {
 public:
  explicit TeeEngine(engine_role_t role, const int max_slice_size, const std::string &cn = "sgx")
      : role_(role), encrypt_slice_size_(max_slice_size), node_id_(cn) {}
  ~TeeEngine();
  bool init(sgx::CertAuth *CertAuth, const std::string &cert_path, const std::string &ra_ip);
  const sgx::RAInfo get_ra_info();
  bool build_ratls_channel(const std::string &request_id, const sgx::Node &remote_node);
  bool decrypt_data_with_sharekey(const std::string &keyid, const char *cipher_data, size_t cipher_size,
                                  char *plain_text, size_t *plain_size);
  bool encrypt_data_with_sharekey(const std::string &keyid, const char *plain_text, size_t plain_size,
                                  char *cipher_data, size_t cipher_size);
  size_t get_encrypted_size(size_t plain_size);
  bool get_plain_size(const char *cipher_data, size_t cipher_size, size_t *plain_size);
  inline sgx::Ra *get_channel() { return ra_channel_.get(); }
  inline std::string get_key(const std::string &request_id, const std::string &node_id) {
    return ra_channel_->get_keyid(request_id, node_id);
  }
  bool decrypt_data_with_sharekey_inside(const std::string &keyid, const char *cipher_data, size_t cipher_size,
                                         const std::string &filename);
  bool encrypt_data_with_sharekey_inside(const std::string &keyid, const std::string &filename,
                                         size_t plain_size, char *cipher_data, size_t cipher_size);
  bool do_compute(const std::vector <std::string> &files, const std::string &code,
                  const std::vector <std::string> &extra_info, size_t *result_size);

 private:
  engine_role_t role_;
  int64_t encrypt_slice_size_ = 0;
  std::string node_id_;
  uint64_t gid_ = 0;
  std::string sign_key_path_;
  std::string sign_cert_path_;
  inline int64_t get_max_slice_size() { return encrypt_slice_size_; }
  std::atomic<bool> encalve_exist = false;
  std::shared_ptr <sgx::Ra> ra_channel_;
};
}  // namespace sgx

#endif  // SGX_ENGINE_SGX_ENGINE_H_
