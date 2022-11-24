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

#ifndef SRC_PRIMIHUB_SERVICE_NOTIFY_SERVICE_H_
#define SRC_PRIMIHUB_SERVICE_NOTIFY_SERVICE_H_

#include <string>
#include "src/primihub/service/notify/model.h"

namespace primihub::service {

class NotifyService {
  public:
    NotifyService(const std::string& addr);
    ~NotifyService();

    // TODO create notify server list by config file.

    void run();
    void notifyStatus(const std::string& job_id, const std::string& task_id,
        const std::string& submit_client_id, const std::string& status, const std::string& message);
    void notifyResult(const std::string& job_id, const std::string& task_id,
        const std::string& submit_client_id, const std::string& result_dataset_url);

    // when client subscribe by node, notify client.
    void onSubscribeClientEvent(const std::string& client_id, const uint64_t session_id);

    // void onNewSessionEvent(const std::string session_id) = 0;
  private:
    void init(const std::string& addr);
};

} // namespace primihub::service

#endif // SRC_PRIMIHUB_SERVICE_NOTIFY_SERVICE_H_
