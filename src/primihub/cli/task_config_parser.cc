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
#include "src/primihub/cli/task_config_parser.h"
#include <fstream>  // std::ifstream

#include "src/primihub/util/file_util.h"
#include "src/primihub/cli/common.h"
#include "src/primihub/util/proto_log_helper.h"

namespace primihub {
namespace pb_util = primihub::proto::util;
retcode ParseTaskConfigFile(const std::string& file_path,
                            TaskFlow* task_flow,
                            DownloadFileListType* download_file_cfg) {
  std::string task_config_content;
  auto retcode = ReadFileContents(file_path, &task_config_content);
  if (retcode != retcode::SUCCESS) {
    LOG(ERROR) << "read task config content failed";
    return retcode::FAIL;
  }
  try {
    auto js_task_cfg = nlohmann::json::parse(task_config_content);
    retcode = BuildRequestWithTaskConfig(js_task_cfg, task_flow);
    if (retcode != retcode::SUCCESS) {
      LOG(ERROR) << "build task request failed";
      return retcode::FAIL;
    }
    retcode = ParseDownFileConfig(js_task_cfg, download_file_cfg);
    if (retcode != retcode::SUCCESS) {
      LOG(ERROR) << "ParseDownFileConfig failed";
      return retcode::FAIL;
    }
  } catch (std::exception& e) {
    LOG(ERROR) << e.what();
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

void fillParamByArray(const std::string& value_type,
        const nlohmann::json& obj, rpc::ParamValue* pv) {
  pv->set_is_array(true);
  if (value_type == "STRING") {
    auto array_ptr = pv->mutable_value_string_array();
    for (const auto& item : obj) {
      auto val = item.get<std::string>();
      array_ptr->add_value_string_array(std::move(val));
    }
  } else if (value_type == "INT32") {
    auto int32_ptr = pv->mutable_value_int32_array();
    for (const auto& item : obj) {
      auto val = item.get<int32_t>();
      int32_ptr->add_value_int32_array(std::move(val));
    }
  } else if (value_type == "INT64") {
    auto int64_ptr = pv->mutable_value_int64_array();
    for (const auto& item : obj) {
      auto val = item.get<int64_t>();
      int64_ptr->add_value_int64_array(std::move(val));
    }
  } else if (value_type == "BOOL") {
    auto bool_ptr = pv->mutable_value_bool_array();
    for (const auto& item : obj) {
      auto val = item.get<bool>();
      bool_ptr->add_value_bool_array(std::move(val));
    }
  } else if (value_type == "FLOAT") {
    auto float_ptr = pv->mutable_value_float_array();
    for (const auto& item : obj) {
      auto val = item.get<float>();
      float_ptr->add_value_float_array(std::move(val));
    }
  } else if (value_type == "DOUBLE") {
    auto double_ptr = pv->mutable_value_double_array();
    for (const auto& item : obj) {
      auto val = item.get<double>();
      double_ptr->add_value_double_array(std::move(val));
    }
  } else if (value_type == "OBJECT") {
    auto array_ptr = pv->mutable_value_string_array();
    for (const auto& item : obj) {
      array_ptr->add_value_string_array(item.dump());
    }
  }
}

void fillParamByScalar(const std::string& value_type,
                       const nlohmann::json& obj, rpc::ParamValue* pv) {
  pv->set_is_array(false);
  if (value_type == "STRING") {
    std::string val = obj.get<std::string>();
    pv->set_value_string(val);
  } else if (value_type == "INT32") {
    auto val = obj.get<int32_t>();
    pv->set_value_int32(val);
  } else if (value_type == "INT64") {
    auto val = obj.get<int64_t>();
    pv->set_value_int64(val);
  } else if (value_type == "BOOL") {
    auto val = obj.get<bool>();
    pv->set_value_bool(val);
  } else if (value_type == "FLOAT") {
    auto val = obj.get<float>();
    pv->set_value_float(val);
  } else if (value_type == "DOUBLE") {
    auto val = obj.get<double>();
    pv->set_value_double(val);
  } else if (value_type == "OBJECT") {
    pv->set_value_string(obj.dump());
  }
}

retcode BuildFederatedRequest(const nlohmann::json& js_task_config,
                              rpc::Task* task_ptr) {
  if (!js_task_config.contains("component_params")) {
    return retcode::SUCCESS;
  }
  std::set<std::string> all_parties;
  // get all party from component_params.roles
  // component_params
  auto& component_params = js_task_config["component_params"];
  for (const auto& [role, party_members] : component_params["roles"].items()) {
    for (const auto& party_name : party_members) {
      all_parties.emplace(party_name);
    }
  }
  const auto& party_access_info = task_ptr->party_access_info();
  if (party_access_info.size() > 0 &&
      party_access_info.size() == all_parties.size()) {
  } else {
    // parse party info
    if (js_task_config.contains("party_info")) {
      auto access_info_ptr = task_ptr->mutable_party_access_info();
      for (auto& [party_name, host] : js_task_config["party_info"].items()) {
        if (all_parties.find(party_name) == all_parties.end()) {
          LOG(WARNING) << party_name
                      << " is not one of party in this task, ignore....";
          continue;
        }
        auto& access_info = (*access_info_ptr)[party_name];
        access_info.set_ip(host["ip"].get<std::string>());
        access_info.set_port(host["port"].get<uint32_t>());
        access_info.set_use_tls(host["use_tls"].get<bool>());
        all_parties.emplace(party_name);
      }
    }
  }

  std::string component_params_str = component_params.dump();
  auto param_map_ptr = task_ptr->mutable_params()->mutable_param_map();
  auto& pv_config = (*param_map_ptr)["component_params"];
  pv_config.set_is_array(false);
  pv_config.set_var_type(rpc::VarType::STRING);
  pv_config.set_value_string(component_params_str);
  // dataset for parties
  auto party_dataset_ptr = task_ptr->mutable_party_datasets();
  auto& role_params = component_params["role_params"];
  for (const auto& party_name : all_parties) {
    if (!role_params.contains(party_name)) {
      continue;
    }
    auto& role_param = role_params[party_name];
    if (!role_param.contains("data_set")) {
      continue;
    }
    auto dataset_ptr = (*party_dataset_ptr)[party_name].mutable_data();
    std::string data_key = role_param["data_set"].get<std::string>();
    if (!data_key.empty()) {
      (*dataset_ptr)["data_set"] = data_key;
    }
  }
  return retcode::SUCCESS;
}

retcode BuildRequestWithTaskConfig(const nlohmann::json& js,
                                   PushTaskRequest* request) {
  try {
    auto task_ptr = request->mutable_task();
    TaskType task_type;
    if (js.contains("task_type")) {
      std::string task_type_str = js["task_type"].get<std::string>();
      auto ret = ProtoTypeMgr::GetTaskType(task_type_str, &task_type);
      if (ret != retcode::SUCCESS) {
        LOG(ERROR) << "GetTaskType failed";
        return retcode::FAIL;
      }
      task_ptr->set_type(task_type);
    }

    // Setup task params
    if (js.contains("task_name")) {
      std::string task_name = js["task_name"].get<std::string>();
      task_ptr->set_name(task_name);
    } else {
      task_ptr->set_name("demoTask");
    }
    // task language
    std::string task_lang;
    if (js.contains("task_lang")) {
      task_lang = js["task_lang"].get<std::string>();
      if (task_lang == "proto") {
        task_ptr->set_language(rpc::Language::PROTO);
      } else if (task_lang == "python") {
        task_ptr->set_language(rpc::Language::PYTHON);
      } else {
        LOG(ERROR) << "task language not supported";
        return retcode::FAIL;
      }
    }

    // param
    if (js.contains("params")) {
      auto map = task_ptr->mutable_params()->mutable_param_map();
      for (auto& [key, value_info]: js["params"].items()) {
        auto value_type = value_info["type"].get<std::string>();
        rpc::ParamValue pv;
        if (value_info["value"].is_array()) {
          fillParamByArray(value_type, value_info["value"], &pv);
        } else {
          fillParamByScalar(value_type, value_info["value"], &pv);
        }
        (*map)[key] = std::move(pv);
      }
    }

    // code
    if (js.contains("task_code")) {
      std::string code_file_path =
          js["task_code"]["code_file_path"].get<std::string>();
      auto code_ptr = task_ptr->mutable_code();
      if (!code_file_path.empty()) {
        // read file
        std::ifstream ifs(code_file_path);
        if (!ifs.is_open()) {
          std::cerr << "open file failed: " << code_file_path << std::endl;
          return retcode::FAIL;
        }
        std::stringstream buffer;
        buffer << ifs.rdbuf();
        std::cout << buffer.str() << std::endl;
        (*code_ptr) = buffer.str();
      } else {
        // read code from command line
        std::string task_code = js["task_code"]["code"].get<std::string>();
        (*code_ptr) = std::move(task_code);
      }
    }

    // party_datasets
    auto party_datasets = task_ptr->mutable_party_datasets();
    if (js.contains("party_datasets")) {
      for (auto& [party_name, dataset_list]: js["party_datasets"].items()) {
        auto& datasets = (*party_datasets)[party_name];
        auto dataset_info = datasets.mutable_data();
        for (auto& [dataset_index, dataset_id] : dataset_list.items()) {
          auto& dataset_value = (*dataset_info)[dataset_index];
          dataset_value = dataset_id;
        }
      }
    }
    // party access info
    auto party_access_info = task_ptr->mutable_party_access_info();
    if (js.contains("party_info")) {
      for (auto& [key, value]: js["party_info"].items()) {
        if (value.is_string()) {
          continue;
        }
        auto& party_node = (*party_access_info)[key];
        std::string ip = value["ip"].get<std::string>();
        int32_t port = value["port"].get<int32_t>();
        bool use_tls = value["use_tls"].get<bool>();
        party_node.set_ip(ip);
        party_node.set_port(port);
        party_node.set_use_tls(use_tls);
      }
    }
    BuildFederatedRequest(js, task_ptr);
    // generate task info
    auto task_info = task_ptr->mutable_task_info();
    std::string task_id = "100";
    std::string job_id = "200";
    ProtoTypeMgr::GenerateTaskInfo(task_id, job_id, task_info);
    std::string str;
    str = pb_util::TaskRequestToString(*request);
    LOG(INFO) << "Task Request: " << str;
  } catch(std::exception& e) {
    LOG(ERROR) << "catch exception: " << e.what();
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode ParseDownFileConfig(const nlohmann::json& js_cfg,
                            DownloadFileListType* download_file_cfg) {
  std::string kDonwload = "download";
  if (!js_cfg.contains(kDonwload)) {
    LOG(INFO) << "no download cfg, ignore...";
    return retcode::SUCCESS;
  }
  if (js_cfg[kDonwload].is_array()) {
    for (const auto& item : js_cfg[kDonwload]) {
      DownloadFileInfo file_info;
      file_info.remote_file_path = item["remote_file_path"].get<std::string>();
      file_info.save_as = item["save_as"].get<std::string>();
      download_file_cfg->push_back(std::move(file_info));
    }
  }
  return retcode::SUCCESS;
}

retcode DownloadData(primihub::SDKClient& client,
                     const rpc::TaskContext& task_info,
                     const DownloadFileListType& download_files_cfg) {
  if (download_files_cfg.empty()) {
    return retcode::SUCCESS;
  }
  for (const auto& file_cfg : download_files_cfg) {
    // download data from server
    const auto& remote_path = file_cfg.remote_file_path;
    std::vector<std::string> file_list;
    file_list.push_back(remote_path);
    std::vector<std::string> downloaded_data;
    LOG(INFO) << "Begin to Download file from remote path: " << remote_path;
    auto ret = client.DownloadData(task_info, file_list, &downloaded_data);
    if (ret != primihub::retcode::SUCCESS) {
      LOG(ERROR) << "Download Data Failed";
      return retcode::FAIL;
    }
    auto& file_name = file_cfg.save_as;
    LOG(INFO) << "save data to " << file_name;
    ret = client.SaveData(file_name, downloaded_data);
    if (ret != primihub::retcode::SUCCESS) {
      LOG(WARNING) << "SaveData to " << file_name << " Failed";
      return retcode::FAIL;
    }
  }
  LOG(INFO) << "download data success";
  return retcode::SUCCESS;
}

void RunTask(const std::string& task_config_file, SDKClient& client) {
  rpc::PushTaskRequest task_request;
  rpc::PushTaskReply reply;
  primihub::DownloadFileListType download_files_cfg;
  if (task_config_file.empty()) {
    LOG(ERROR) << "task config file is empty";
    return;
  }
  auto ret = ParseTaskConfigFile(task_config_file,
                                 &task_request, &download_files_cfg);
  if (ret != primihub::retcode::SUCCESS) {
    LOG(ERROR) << "ParseTaskConfigFile failed";
    return;
  }

  auto _start = std::chrono::high_resolution_clock::now();
  ret = client.SubmitTask(task_request, &reply);
  if (ret != primihub::retcode::SUCCESS) {
    LOG(ERROR) << "SubmitTask failed";
    return;
  }
  ret = client.CheckTaskStauts(reply);
  auto _end = std::chrono::high_resolution_clock::now();
  auto time_cost =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          _end - _start).count();
  LOG(INFO) << "SubmitTask time cost(ms): " << time_cost;
  if (ret != primihub::retcode::SUCCESS) {
    LOG(ERROR) << "submit task encountes error";
    return;
  }

  // download result
  auto task_info = task_request.task().task_info();
  DownloadData(client, task_info, download_files_cfg);
}

}  // namespace primihub
