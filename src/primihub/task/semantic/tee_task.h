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

#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_TEE_TASK_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_TEE_TASK_H_

// #include <pybind11/embed.h>
#include "src/primihub/task/semantic/task.h"

// namespace py = pybind11;
namespace primihub::task {
// /**
//  * @brief TEE Executor role task
//  *  1. compile AI server SGX enclave application
//  *  2. run SGX enclave application
//  *  3. Notice all DataProvider  start to provide data
//  */
// class TEEExecutorTask : public TaskBase {
//     public:
//         TEEExecutorTask(const TaskParam *task_param,
//                         std::shared_ptr<DatasetService> dataset_service);
//         ~TEEExecutorTask() {}

//         int compile();
//         int execute();
// };

/**
 * @brief TEE DataProvider role task
 *
 */
class TEEDataProviderTask: public TaskBase {
    public:
        TEEDataProviderTask(
            const std::string& node_id,
            const TaskParam *task_param,
            std::shared_ptr<DatasetService> dataset_service);
        ~TEEDataProviderTask();
        int execute();
    private:
        py::object flight_client_;
        std::string dataset_, server_addr_;
};

} // namespace primihub::task

#endif  // SRC_PRIMIHUB_TASK_SEMANTIC_TEE_TASK_H_
