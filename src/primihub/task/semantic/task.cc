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

#include "src/primihub/task/semantic/task.h"

namespace primihub::task {

    TaskBase::TaskBase(const TaskParam *task_param,
                       std::shared_ptr<DatasetService> dataset_service) {
        setTaskParam(task_param);
        dataset_service_ = dataset_service;
    }

    TaskParam* TaskBase::getTaskParam()  {
        return &task_param_;
    }

    void TaskBase::setTaskParam(const TaskParam *task_param) {
        task_param_.CopyFrom(*task_param);
    }
 
} // namespace primihub::task
