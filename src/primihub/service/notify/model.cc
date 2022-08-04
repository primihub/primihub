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

#include "src/primihub/service/notify/model.h"

namespace primihub::service {

//////////////////////////////EventBusNotifyDelegate/////////////////////////////////////////////////
EventBusNotifyDelegate::EventBusNotifyDelegate(const rpc::TaskContext &task_context) {
    
}

void EventBusNotifyDelegate::notifyStatus(const std::string &status) {

}

void EventBusNotifyDelegate::notifyResult(const std::string &result_dataset_url) {

}

/////////////////////////////////////EventBusNotifyServerSubscriber//////////////////////////////////////////

EventBusNotifyServerSubscriber::EventBusNotifyServerSubscriber(const std::shared_ptr<NotifyServer> &notify_server,
                                                            primihub::common::event_bus *event_bus_ptr)
: NotifyServerSubscriber(notify_server), event_bus_ptr_(event_bus_ptr) {
    
} 

EventBusNotifyServerSubscriber::~EventBusNotifyServerSubscriber() {
    event_bus_ptr_->remove_handler(task_stauts_reg_);
    event_bus_ptr_->remove_handler(task_reuslt_reg_);
}


void EventBusNotifyServerSubscriber::subscribe() {
    task_stauts_reg_ = std::move(event_bus_ptr_->register_handler<primihub::rpc::TaskStatus>(
        notify_server_, GRPCNotifyServer::on_task_status_event
    ));
    task_reuslt_reg_ = std::move(event_bus_ptr_->register_handler<primihub::rpc::TaskResult>(
        notify_server_, NotifyServer::on_task_result_event
    ));
}

//////////////////////////////////////////GRPCNotifyServer////////////////////////////////////////////////////

GRPCNotifyServer::GRPCNotifyServer(const std::shared_ptr<NotifyServerSubscriber> &notify_server_subscriber) {
    notify_server_subscriber_ = notify_server_subscriber;
}

GRPCNotifyServer::~GRPCNotifyServer() {

}





} // namespace primihub::service
