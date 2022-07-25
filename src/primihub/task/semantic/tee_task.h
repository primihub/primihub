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

/**
 * @brief TEE Executor role task
 *  1. compile AI server SGX enclave application
 *  2. run SGX enclave application
 *  3. Notice all DataProvider  start to provide data
 */
class TEEExecutorTask : public TaskBase {
    public:
        TEEExecutorTask(const TaskParam *task_param, 
                        std::shared_ptr<DatasetService> dataset_service);
        ~TEEExecutorTask() {}

        int compile();
        int execute();
};


/**
 * @brief TEE DataProvider role task
 * 
 */
class TEEDataProviderTask {

};

} // namespace primihub::task