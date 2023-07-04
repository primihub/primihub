#ifndef SGX_RA_RA_UTIL_H_
#define SGX_RA_RA_UTIL_H_

#include <string>
#include <memory>
#include <atomic>
#include "sgx/common/common_def.h"
#include "sgx/cert/util.h"

namespace sgx {
class Ra {
 public:
  explicit Ra(std::string base_url = "",
              const std::string &base_cert_path = default_sgx_data_dir,
              sgx::CertAuth *CertAuth = nullptr);
  ~Ra();
  bool init(const RAInfo &info, const std::string &selfid);
  bool request_certficate(const std::string &cn);
  bool remote_attestation_between_node(const std::string &request_id, const std::string &selfid,
                                       const sgx::Node &remote_node);
  bool exchange_key_between_node(const std::string &request_id, const std::string &self_id,
                                 const sgx::Node &remote_node);

  bool import_sharekey(const std::string &enc_key, const std::string &keyid);
  bool export_sharekey(const std::string &keyid, const std::string &certificate, std::string *enc_key);
  std::string sign(const std::string &msg);

  inline const std::string get_certificate_path() { return enclave_cert_path_; }

  inline std::string get_keyid(const std::string request_id, const std::string &remote_ip) {
    return request_id + remote_ip;
  }
  inline sgx::CertAuth *get_auth() { return auth_; }
  inline std::string get_dcap_cert() { return dcap_cert_; }
  inline std::string get_ca_cert_path() { return ca_cert_path_; }
  inline std::string get_base_url() { return base_url_; }
  inline uint64_t get_gid() { return gid_; }

 private:
  bool verify_signature(const std::string &msg, const std::string &cert, const std::string &signature);
  bool verify_attestor_cert(const std::string &attestor_cert,
                            const std::string &nonce,
                            const std::string &nonce_signature);
  std::string generate_nonce();
  inline std::string get_remote_cert() { return remote_cert_; }
  inline void set_remote_cert(const std::string &cert) { remote_cert_ = cert; }
  bool handle_remote_attestation(const std::string &request_id, const sgx::Node &remote_node,
                                 const std::string &attestor_cert, const std::string &nonce,
                                 const std::string &nonce_sig);
  bool handle_send_sharekey(const std::string &request_id, const std::string &node_id,
                            const sgx::Node &remote_node,
                            const std::string &encrypted_key);
  bool init_dcap_cert();
  std::string base_url_;
  std::string base_path_;
  std::string dcap_cert_;
  std::string dcap_cert_path_;
  std::string ca_cert_path_;
  bool sgx_enabled_;
  uint64_t gid_;
  std::string enclave_key_path_;
  std::string enclave_cert_path_;
  std::atomic<bool> encalve_exist_{false};

  int max_slice_size_ = 0;
  sgx::CertAuth *auth_ = nullptr;
  std::string remote_cert_;
};
}  // namespace sgx

#endif  // SGX_RA_RA_UTIL_H_
