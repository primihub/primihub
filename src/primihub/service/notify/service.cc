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
#include <memory>
#include <glog/logging.h>

#include "src/primihub/service/notify/service.h"       

using primihub::service::GRPCNotifyServer;

namespace primihub::service {
        
NotifyService::NotifyService() {
    init();
}

NotifyService::~NotifyService() {

}

void NotifyService::init() {
    // Composite GRPCNotifyServer & EventBusNotifyServerSubscriber
    try {   
        auto subscriber = std::make_shared<EventBusNotifyServerSubscriber>(GRPCNotifyServer::getInstance(),
                                                                        EventBusNotifyDelegate::getInstance().getEventBusPtr());
        auto subscriber_cast = std::dynamic_pointer_cast<NotifyServerSubscriber>(subscriber);                                                                        
        
        // FIXME test parameters
        GRPCNotifyServer::getInstance().init("0.0.0.0:6666", subscriber_cast);
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to init notify server: " << e.what();
    }
    
}

void NotifyService::run() {
    try {
        GRPCNotifyServer::getInstance().run();
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to run notify server: " << e.what();
    }
}

void NotifyService::notifyStatus(const std::string task_id, const std::string &status) {
    EventBusNotifyDelegate::getInstance().notifyStatus(task_id, status);
}

void NotifyService::notifyResult(const std::string task_id, const std::string &result_dataset_url) {
    EventBusNotifyDelegate::getInstance().notifyResult(task_id, result_dataset_url);
}

void NotifyService::onSubscribeTaskEvent(const std::string task_id, const uint64_t &session_id) {
    GRPCNotifyServer::getInstance().addTaskSession(task_id, 
                                                   GRPCNotifyServer::getInstance().getSession(session_id));
}

}   // namespace primihub::service
