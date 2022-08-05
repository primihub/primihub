/*
 Copyright 2022 Primihub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#ifndef SRC_PRIMIHUB_SERVICE_NOTIFY_MODEL_H_
#define SRC_PRIMIHUB_SERVICE_NOTIFY_MODEL_H_

#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#include "src/primihub/common/eventbus/eventbus.hpp"
#include "src/primihub/protos/service.grpc.pb.h"

/* Primihub notification toplogy 

                                             {Inprocess}                            {Inprocess or Interprocess}                    {Interprocess}

   ---------    notifyStatus/notifyResult   ----------------   publish/subscribe    -----------------------------    Any channel  -----------------
   |Nodelet| -----------------------------> |NotifyDelegate | --------------------> |NotifyServer Implemention 1| ------------> |Register Client 1| 
   ---------                                ----------------  |                     -----------------------------                -----------------
                                                              | publish/subscribe    ----------------------------                ------------------
                                                              |--------------------> |NotifyServer Implemention 2| ------------> |Register Client 2|
                                                                                     ----------------------------                ------------------
*/
using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;
using primihub::rpc::ClientContext;
using primihub::rpc::TaskContext;
using primihub::rpc::NodeContext;
using primihub::rpc::NodeService;
using primihub::common::event_bus;

namespace primihub::service {


class NotifyDelegate {
    public:
        virtual void notifyStatus(const std::string &status) = 0;
        virtual void notifyResult(const std::string &result) = 0;
};

struct TaskStatusEvent {
    std::string msg;
};


class EventBusNotifyDelegate : public NotifyDelegate {
    public:
        explicit EventBusNotifyDelegate(const rpc::TaskContext &task_context);
        ~EventBusNotifyDelegate();
        void notifyStatus(const std::string &status);
        void notifyResult(const std::string &result_dataset_url);
        primihub::common::event_bus* getEventBusPtr() { return &event_bus_; }

    private:
        primihub::common::event_bus event_bus_;
};

class NotifyServer;

class NotifyServerSubscriber {
    public:
        explicit NotifyServerSubscriber(const std::shared_ptr<NotifyServer> &notify_server): notify_server_(notify_server) {}
        ~NotifyServerSubscriber() {}
        virtual void subscribe(const std::string &topic, const std::string &server_addr) = 0;
        virtual void unsubscribe(const std::string &topic, const std::string &server_addr) = 0;
    protected:
        std::shared_ptr<NotifyServer> notify_server_;
};


class NotifyServer {
    public:
        // 1. task context register
        virtual int regTaskContext(const rpc::TaskContext &task_context) = 0;
        // 2. subscribe task notify use task context id
        virtual int subscribeTaskNotify(const rpc::TaskContext &task_context) = 0;
        // 3. client context register
        virtual int regClientContext(const rpc::ClientContext &client_context) = 0;
        // 4. client notification implement
        virtual int notifyProcess() = 0;
        // Event handlers
        virtual void onTaskStatusEvent(const primihub::rpc::TaskStatus &e) = 0;
        virtual void onTaskResultEvent(const primihub::rpc::TaskResult &e) = 0;

    protected:
        // Running task context list
        std::map<std::string, TaskContext> task_context_map_;
        // client context list
        std::list<rpc::ClientContext> client_context_list_;
        // channel id
        std::string channel_id_;
};

class EventBusNotifyServerSubscriber : public NotifyServerSubscriber {
    public:
        explicit EventBusNotifyServerSubscriber(const std::shared_ptr<NotifyServer> &notify_server,
                                                primihub::common::event_bus *event_bus_ptr);
        ~EventBusNotifyServerSubscriber();
        void subscribe(const std::string &topic) {};
        void unsubscribe(const std::string &topic) {};
        
        
    private:
        primihub::common::event_bus *event_bus_ptr_; 
        primihub::common::handler_registration task_stauts_reg_, task_result_reg_;

};


////////////////////////////////GRPC Notification Server & client session impolemetion ///////////////////////////////////////////////////////

class GRPCClientSession;

class GRPCNotifyServer final : public NotifyServer  {
    public:
        GRPCNotifyServer() {};
        ~GRPCNotifyServer();

        int regTaskContext(const rpc::TaskContext &task_context);
        int subscribeTaskNotify(const rpc::TaskContext &task_context);
        int regClientContext(const rpc::ClientContext &client_context);
        int notifyProcess();
        
        // Event handlers
        void onTaskStatusEvent(const primihub::rpc::TaskStatus &e);
        void onTaskResultEvent(const primihub::rpc::TaskResult &e);

        // gRPC methods
        bool init(const std::string &server);
        void run();
        void stop(); 
        std::shared_ptr<GRPCClientSession> addSession();
        void removeSession(uint64_t sessionId);
        std::shared_ptr<GRPCClientSession> getSession(uint64_t sessionId);

    
    private:
        std::shared_ptr<NotifyServerSubscriber> notify_server_subscriber_;
        // TODO task context -> grpc client map
      
        // gRPC members
        std::atomic_bool running_{false};
        std::atomic_uint64_t session_id_allocator_{0};

        std::unique_ptr<::grpc::Server> server_{};
        NodeService::AsyncService node_async_service_{};

        // subscribe_greeting
        std::unique_ptr<::grpc::ServerCompletionQueue> completion_queue_call_{};
        std::unique_ptr<::grpc::ServerCompletionQueue> completion_queue_notification_{};
        // better choice for production environment: boost::shared_mutex or std::shared_mutex(since C++17)
        mutable std::shared_mutex mutex_sessions_{};
        std::unordered_map<uint64_t /*session id*/, std::shared_ptr<GreetingSession>> sessions_{};

        

};

#include <iostream>

#define GRPC_NOTIFY_EVENT_MASK 0x3u
#define GRPC_NOTIFY_EVENT_BIT_LENGTH 2u

enum GrpcNotifyEvent {
    GRPC_NOTIFY_EVENT_CONNECTED = 0,
    GRPC_NOTIFY_EVENT_READ_DONE = 1,
    GRPC_NOTIFY_EVENT_WRITE_DONE = 2,
    GRPC_NOTIFY_EVENT_FINISHED = 3
};

std::ostream& operator<<(std::ostream& os, GrpcNotifyEvent event);

enum class GrpcClientSessionStatus { 
    WAIT_CONNECT,
    READY_TO_WRITE,
    WAIT_WRITE_DONE, 
    FINISHED 
};

std::ostream& operator<<(std::ostream& os, GrpcClientSessionStatus sessionStatus);

class GRPCClientSession {
    public:
        explicit GRPCClientSession(uint64_t sessionId) : session_id_(sessionId) {}
        
        bool init();

        void process(GrpcEvent event);

        void reply();

        void finish();

    public:
        const uint64_t session_id_{0}; // client id?
        
        std::mutex mutex_{};
        GrpcClientSessionStatus status_{GrpcClientSessionStatus::WAIT_CONNECT};

        ::grpc::ServerContext server_context_{};

        ::grpc::ServerAsyncReaderWriter<primihub::rpc::NodeEventReply, primihub::rpc::ClientContext>
            subscribe_stream{&server_context_};

        primihub::rpc::ClientContext request_{};

        std::string name_{};
        std::deque<std::shared_ptr<primihub::rpc::NodeEventReply>> message_queue_{};
        Performance performance_{};
        uint64_t reply_times_{0};
};


} // namespace primihub::service

#endif // SRC_PRIMIHUB_SERVICE_NOTIFY_MODEL_H_
