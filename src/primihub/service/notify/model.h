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

using primihub::rpc::ClientContext;
using primihub::rpc::TaskContext;
using primihub::rpc::NodeContext;
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
        virtual int RegTaskContext(const rpc::TaskContext &task_context) = 0;
        // 2. subscribe task notify use task context id
        virtual int SubscribeTaskNotify(const rpc::TaskContext &task_context) = 0;
        // 3. client context register
        virtual int RegClientContext(const rpc::ClientContext &client_context) = 0;
        // 4. client notification implement
        virtual int notifyProcess() = 0;
        // Event handlers
        virtual void on_task_status_event(const primihub::rpc::TaskStatus &e) = 0;
        virtual void on_task_result_event(const primihub::rpc::TaskResult &e) = 0;

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
        void subscribe(const std::string &topic);
        void unsubscribe(const std::string &topic);
        
    private:
        primihub::common::event_bus *event_bus_ptr_; 
        primihub::common::handler_registration task_stauts_reg_, task_result_reg_;

};


class GRPCNotifyServer : public NotifyServer {
    public:
        explicit GRPCNotifyServer(const std::string &channel_id);
        ~GRPCNotifyServer();
        int RegTaskContext(const rpc::TaskContext &task_context);
        int SubscribeTaskNotify(const rpc::TaskContext &task_context);
        int RegClientContext(const rpc::ClientContext &client_context);
        int notifyProcess();
    
    private:
        std::string channel_id_;
        std::shared_ptr<NotifyServerSubscriber> notify_server_subscriber_;
        // TODO task context -> grpc client map
};


} // namespace primihub::service

#endif // SRC_PRIMIHUB_SERVICE_NOTIFY_MODEL_H_
