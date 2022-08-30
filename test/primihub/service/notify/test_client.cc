

#include <glog/logging.h>
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include <memory>
#include <string>
#include <thread>

#include <signal.h>
#include "src/primihub/protos/service.grpc.pb.h"

using grpc::CreateChannel;


class NotifyTestClient {
  public:
    NotifyTestClient(const NotifyTestClient &) = delete;
    NotifyTestClient(NotifyTestClient &&) = delete;
    NotifyTestClient &operator=(const NotifyTestClient &) = delete;
    NotifyTestClient &operator=(NotifyTestClient &&) = delete;

    // only for C++11, return static local var reference is thread safe
    static NotifyTestClient &getInstance() {
        static NotifyTestClient kSingleInstance;
        return kSingleInstance;
    }

    bool init(const std::string &client_id, const std::string &server) {
        client_id_ = client_id;

        stub_ = primihub::rpc::NodeService::NewStub(
            grpc::CreateChannel(server, grpc::InsecureChannelCredentials()));
          
        return true;
    }

    void run() {
        running_ = true;
        // when connect to server, send client_id to server 
        primihub::rpc::ClientContext request;
        request.set_client_id(client_id_);
        stream_ = stub_->SubscribeNodeEvent(&context_, request);
        std::thread thread_for_reply{[this] {
            LOG(INFO) << "thread_for_reply run";
            primihub::rpc::NodeEventReply reply;
            while (stream_->Read(&reply)) {
                LOG(INFO) << "receive reply: " << reply.ShortDebugString();
            }
            LOG(INFO) << "thread_for_reply stop";
        }};
        if (thread_for_reply.joinable()) {
            thread_for_reply.join();
        }
    }

    void stop() {
        if (!running_) {
            return;
        }
        running_ = false;
        LOG(INFO) << "context_->TryCancel() begin";
        context_.TryCancel();
        LOG(INFO) << "context_->TryCancel() end";
    }

  private:
    NotifyTestClient() = default;
    virtual ~NotifyTestClient() = default;

  public:
    std::atomic_bool running_{false};

    std::unique_ptr<primihub::rpc::NodeService::Stub> stub_;
    ::grpc::ClientContext context_{};
    std::unique_ptr<::grpc::ClientReaderInterface<primihub::rpc::NodeEventReply>> stream_{};
    std::string client_id_{};
};

void handler(int sig) {
    LOG(INFO) << "get signal: " << sig;
    NotifyTestClient::getInstance().stop();
}

int main(int argc, char **argv) {
    signal(SIGTERM, handler);
    signal(SIGINT, handler);

    try {
        if (!NotifyTestClient::getInstance().init( "1" /*client id*/, 
                                                   "localhost:6666" /*server address*/
                                                   )) {
            LOG(INFO) << "NotifyTestClient init failed";
            return 1;
        }
        NotifyTestClient::getInstance().run();
    } catch (std::exception &e) {
        LOG(INFO) << "exception: " << e.what();
        return 1;
    }

    return 0;
}
