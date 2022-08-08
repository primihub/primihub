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

#include "src/primihub/service/notify/service.h"

using primihub::service::NotifyService;
using primihub::service::NotifyServer;

NotifyService *notify_service_ptr;

void handler(int sig) {
    std::cout << "get signal: " << sig << std::endl;
    // GreetingServer::getInstance().stop();
    delete notify_service_ptr;
}


int main(int argc, const char **argv) {
    signal(SIGTERM, handler);
    signal(SIGINT, handler);

    notify_service_ptr = new NotifyService();
    notify_service_ptr->run();
    

    // TODO stdin read to simulate task stauts and result.
    

    return 0;
}


