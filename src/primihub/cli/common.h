/*
* Copyright (c) 2024 by PrimiHub
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      https://www.apache.org/licenses/
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#ifndef SRC_PRIMIHUB_CLI_COMMON_H_
#define SRC_PRIMIHUB_CLI_COMMON_H_
#include <glog/logging.h>
#include "src/primihub/protos/service.pb.h"
#include "src/primihub/protos/worker.pb.h"
#include "src/primihub/protos/common.pb.h"
#include "src/primihub/common/common.h"
#include "uuid.h"
namespace primihub {
struct Common {
 public:
  static Node GetNode(const std::string& server_info, bool use_tls) {
    std::stringstream ss(server_info);
    std::vector<std::string> result;
    while (ss.good()) {
      std::string substr;
      getline(ss, substr, ':');
      result.push_back(substr);
    }
    Node server_node(result[0], std::stoi(result[1]), use_tls);
    return server_node;
  }
};

class ProtoTypeMgr {
 public:
  static retcode GenerateTaskInfo(const std::string& task_id,
                                  const std::string& job_id,
                                  rpc::TaskContext* task_info) {
    // Generate job id and task id
    std::random_device rd;
    auto seed_data = std::array<int, std::mt19937::state_size> {};
    std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
    std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
    std::mt19937 generator(seq);
    uuids::uuid_random_generator gen{generator};
    const uuids::uuid id = gen();
    std::string request_id = uuids::to_string(id);
    task_info->set_job_id(job_id);
    task_info->set_task_id(task_id);
    task_info->set_request_id(request_id);
    return retcode::SUCCESS;
  }
  static retcode GetTaskType(const std::string& task_type_str,
                      rpc::TaskType* task_type) {
    auto it = task_type_map.find(task_type_str);
    if (it != task_type_map.end()) {
      *task_type = it->second;
      return retcode::SUCCESS;
    } else {
      LOG(ERROR) << "unknown task type: " << task_type_str;
      return retcode::FAIL;
    }
  }

  static retcode GetPlainDataType(const std::string& name,
                                  rpc::DataTypeInfo::PlainDataType* type) {
    auto it = plain_data_type.find(name);
    if (it != plain_data_type.end()) {
      *type = it->second;
      return retcode::SUCCESS;
    } else {
      LOG(ERROR) << "unsuported data type name: " << name;
      return retcode::FAIL;
    }
  }

 protected:
  static std::string TaskTypeName(rpc::TaskType type) {
    return rpc::TaskType_Name(type);
  }

  static std::string PlainDataTypeName(rpc::DataTypeInfo::PlainDataType type) {
    return rpc::DataTypeInfo::PlainDataType_Name(type);
  }

 private:
  inline static std::map<std::string, rpc::TaskType> task_type_map {
    {TaskTypeName(rpc::TaskType::ACTOR_TASK), rpc::TaskType::ACTOR_TASK},
    {TaskTypeName(rpc::TaskType::PIR_TASK), rpc::TaskType::PIR_TASK},
    {TaskTypeName(rpc::TaskType::PSI_TASK), rpc::TaskType::PSI_TASK},
  };

  using DataTypeContainer =
      std::map<std::string, rpc::DataTypeInfo::PlainDataType>;
  inline static DataTypeContainer plain_data_type {
    {PlainDataTypeName(rpc::DataTypeInfo::INT32), rpc::DataTypeInfo::INT32},
    {PlainDataTypeName(rpc::DataTypeInfo::INT64), rpc::DataTypeInfo::INT64},
    {PlainDataTypeName(rpc::DataTypeInfo::STRING), rpc::DataTypeInfo::STRING},
    {PlainDataTypeName(rpc::DataTypeInfo::DOUBLE), rpc::DataTypeInfo::DOUBLE},
    {PlainDataTypeName(rpc::DataTypeInfo::FLOAT), rpc::DataTypeInfo::FLOAT},
    {PlainDataTypeName(rpc::DataTypeInfo::BOOL), rpc::DataTypeInfo::BOOL},
  };
};
}  // namespace primihub
#endif  // SRC_PRIMIHUB_CLI_COMMON_H_
