#include <grpcpp/grpcpp.h>
#include <grpc/grpc.h>
#include "sgx/ra/ra_service_handler.h"
#include "sgx/ra/tee.grpc.pb.h"

#ifndef SGX_RA_SERVICE_H_
#define SGX_RA_SERVICE_H_

namespace sgx {

class RaTlsService final : public rpc::RaTls::Service {
  grpc::Status RequestRemoteAttestation(grpc::ServerContext *context,
                                        const sgx::rpc::RemoteAttestationRequest *request,
                                        sgx::rpc::RemoteAttestationResponse *response) override;
  grpc::Status SendEncryptKey(grpc::ServerContext *context, const sgx::rpc::EncryptKey *key,
                              sgx::rpc::CommonResponse *response) override;

 public:
  inline void set_ra_handler(RaTlsHandler *handler) {
    this->ra_handler = handler;
  }

  inline RaTlsHandler *get_ra_handler() { return this->ra_handler; }
  inline void set_channel(Ra *ch) {
    this->Ra_channel_ = ch;
  }
  inline Ra *get_channel() { return this->Ra_channel_; }

 private:
  RaTlsHandler *ra_handler;
  std::mutex mtx;
  bool use_tls_ = false;
  Ra *Ra_channel_ = nullptr;
  std::mutex ra_mutex;
};

}  // namespace sgx
#endif  // SGX_RA_SERVICE_H_
