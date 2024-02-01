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
#include "src/primihub/cli/dataset_config_parser.h"
#include <glog/logging.h>
#include <nlohmann/json.hpp>
#include "src/primihub/util/file_util.h"
#include "src/primihub/cli/common.h"
#include "src/primihub/util/proto_log_helper.h"

namespace primihub {
namespace pb_util = primihub::proto::util;

retcode ParseDatasetMetaInfo(const std::string& config_file,
                             std::vector<rpc::NewDatasetRequest>* requests) {
  std::string task_config_content;
  try {
    auto ret = ReadFileContents(config_file, &task_config_content);
    if (ret != retcode::SUCCESS) {
      LOG(ERROR) << "read task config content failed";
      return retcode::FAIL;
    }
    auto meta_info_list = nlohmann::json::parse(task_config_content);
    size_t request_count = meta_info_list.size();
    requests->reserve(request_count);
    for (const auto& meta_info : meta_info_list) {
      rpc::NewDatasetRequest request;
      auto meta_info_ptr = request.mutable_meta_info();
      auto id = meta_info["id"].get<std::string>();
      auto type = meta_info["type"].get<std::string>();
      meta_info_ptr->set_id(id);
      meta_info_ptr->set_driver(type);
      meta_info_ptr->set_visibility(rpc::MetaInfo::PUBLIC);
      std::string access_info;
      if (meta_info["access_info"].is_string()) {
        access_info = meta_info["access_info"].get<std::string>();
      } else {
        access_info = meta_info["access_info"].dump();
      }
      meta_info_ptr->set_access_info(access_info);
      // meta_list->emplace_back(std::make_tuple(id, type, access_info));
      // dataset schema
      if (meta_info.contains("schema")) {
        for (const auto& [field_name, type_name] : meta_info["schema"].items()) {
          VLOG(3) << "key: " << field_name << " value: " << type_name;
          rpc::DataTypeInfo::PlainDataType type;
          auto ret = ProtoTypeMgr::GetPlainDataType(type_name, &type);
          if (ret != retcode::SUCCESS) {
            LOG(ERROR) << "no type found for " << type_name;
            return retcode::FAIL;
          }
          auto data_type_ptr = meta_info_ptr->add_data_type();
          data_type_ptr->set_name(field_name);
          data_type_ptr->set_type(type);
        }
      } else {
        LOG(ERROR) << "No dataset schema is configured";
      }
      VLOG(3) << pb_util::TypeToString(request);
      requests->push_back(std::move(request));
    }
  } catch (std::exception& e) {
    LOG(ERROR) << "parse dataset meta info failed: " << e.what();
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode ParseTaskConfigFile(const std::string& file_path,
                            std::vector<rpc::NewDatasetRequest>* requests) {
  auto ret{retcode::SUCCESS};
  try {
    ret = ParseDatasetMetaInfo(file_path, requests);
    if (ret != retcode::SUCCESS) {
      LOG(ERROR) << "build register dataset requests failed";
      return retcode::FAIL;
    }
  } catch (std::exception& e) {
    LOG(ERROR) << e.what();
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

void RegisterDataset(const std::string& task_config_file, SDKClient& client) {
  if (task_config_file.empty()) {
    LOG(ERROR) << "task config file is empty";
    return;
  }
  std::vector<rpc::NewDatasetRequest> requests;

  auto ret = ParseTaskConfigFile(task_config_file, &requests);
  if (ret != primihub::retcode::SUCCESS) {
    LOG(ERROR) << "ParseTaskConfigFile failed";
    return;
  }
  for (auto& req : requests) {
    req.set_op_type(rpc::NewDatasetRequest::REGISTER);
    rpc::NewDatasetResponse reply;
    client.RegisterDataset(req, &reply);
  }
}
}  // namespace primihub