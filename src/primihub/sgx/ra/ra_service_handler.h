#ifndef SGX_RA_RA_SERVICE_HANDLER_H_
#define SGX_RA_RA_SERVICE_HANDLER_H_
#include <string>
#include <vector>
#include "sgx/ra/ra_util.h"

namespace sgx {
class RaTlsHandler {
 public:
  virtual void init(sgx::Ra *handler) = 0;
  virtual ~RaTlsHandler() {}
  virtual std::vector <std::string> handle_remote_attestation(const std::string &remote_cert, const std::string &nonce,
                                                              const std::string &nonce_sig) = 0;
  virtual bool handle_encryptedkey(const std::string &keyid, const std::string &enkey) = 0;
};

class RaTlsHandlerImpl : public RaTlsHandler {
 public:
  explicit RaTlsHandlerImpl(bool local = false)
      : local_(local) {}
  // bool init(const RAInfo &info);
  // v[0]: nonce_sig v[1]: cert
  std::vector <std::string> handle_remote_attestation(const std::string &remote_cert, const std::string &nonce,
                                                      const std::string &nonce_sig) override;
  bool handle_encryptedkey(const std::string &keyid, const std::string &enkey) override;
  void init(sgx::Ra *handler) { handler_ = handler; }
  sgx::Ra *get_handler() { return handler_; }

 private:
  bool local_ = false;
  sgx::Ra *handler_ = nullptr;

  bool verify_certificate(const std::string &certificate, const std::string &nonce = "",
                          const std::string &nonce_signature = "");

  bool verify_cert_remote(const std::string &certificate, const std::string &nonce = "",
                          const std::string &nonce_signature = "");
  bool verify_cert_local(const std::string &certificate, const std::string &nonce = "",
                         const std::string &nonce_signature = "");
  bool verify_signature(const std::string &msg, const std::string &cert, const std::string &signature);
  std::string generate_nonce();
  std::string get_cert();
  bool get_quote_from_cert(const std::string &cert, std::string *quote);
  inline std::string get_dcap_cert() {
    if (handler_) {
      return handler_->get_dcap_cert();
    } else {
      return "";
    }
  }
};
}  // namespace sgx

#endif  // SGX_RA_RA_SERVICE_HANDLER_H_
