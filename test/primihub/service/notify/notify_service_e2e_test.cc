/*
 Copyright 2022 PrimiHub

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
#include <unistd.h>
#include <cstdio>
#include <thread>
#include <string>

#include "src/primihub/service/notify/service.h"
#include "src/primihub/service/notify/model.h"
#include <signal.h>

using primihub::service::NotifyService;
using primihub::service::NotifyServer;
using primihub::service::GRPCNotifyServer;
using primihub::service::EventBusNotifyDelegate;

NotifyService *notify_service_ptr;

void handler(int sig) {
    std::cout << "get signal: " << sig << std::endl;
    GRPCNotifyServer::getInstance().stop();
    delete notify_service_ptr;
}

// TODO read to simulate task stauts and result.

void stdin_func() {
    std::string line;
    std::cout << "waiting input: ";
    while (std::getline(std::cin, line)) {
        if (line == "quit") {
            std::cout << "quit" << std::endl;
            break;
        } else if (line == "s") {
            // simulate send task status
            std::cout << "simulate send task status" << std::endl;
             // FIXME test code
            GRPCNotifyServer::getInstance().addSession();

            EventBusNotifyDelegate::getInstance().notifyStatus(
                  "1", "1","client_id", "SUCCESS", "task test status");

        } else if (line == "r") {
            // simulate send task status
            std::cout << "simulate send task result" << std::endl;
            EventBusNotifyDelegate::getInstance().notifyResult(
                "1", "1","client_id", "task test result");
        } else {
            std::cout << "input: " << line << std::endl;
        }
    }
}

void server_func() {
    notify_service_ptr = new NotifyService("127.0.0.1:7667");
    notify_service_ptr->run();
}

int main(int argc, const char **argv) {
    signal(SIGTERM, handler);
    signal(SIGINT, handler);
    // run stdin in thread
    std::thread stdin_thread(stdin_func);
    // run server in thread
    std::thread server_thread(server_func);
    stdin_thread.join();
    server_thread.join();

    return 0;
}


