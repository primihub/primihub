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
#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_FL_TASK_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_FL_TASK_H_

#include <string>
#include <vector>
#include <pybind11/embed.h>
#include "src/primihub/task/semantic/task.h"
#include "src/primihub/task/language/py_parser.h"

namespace py = pybind11;
namespace primihub::task {
    /* *
     * @brief Federated Learning task, only support python code.
     *  TODO Run python use pybind11.
     */
    class FLTask: public TaskBase {
        public:
            FLTask(const std::string &node_id,
                     const TaskParam *task_param,
                     std::shared_ptr<DatasetService> dataset_service);

            ~FLTask();
            
            int execute() override;
        
        private:
            std::string py_code_;
            NodeContext node_context_;
            py::object set_task_context_output_file_, set_task_context_dataset_map_, set_node_context_,  ph_exec_m, ph_context_m, set_task_context_func_params_;
            std::string next_peer_address_;
            std::map<std::string, std::string> dataset_meta_map_;
            std::string output_file_path_;

    };

} // namespace primihub::task

#endif // SRC_PRIMIHUB_TASK_SEMANTIC_FL_TASK_H_
