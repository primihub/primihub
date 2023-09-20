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

#include "src/primihub/cli/cli.h"
#include <fstream>  // std::ifstream
#include <string>
#include <chrono>
#include <random>
#include <vector>
#include <future>
#include <thread>
#include <nlohmann/json.hpp>
#include "src/primihub/util/util.h"
#include "src/primihub/util/file_util.h"
#include "src/primihub/common/config/config.h"
#include "src/primihub/util/network/link_factory.h"
#include "src/primihub/common/common.h"
#include "uuid.h"
// #include <google/protobuf/text_format.h>

using primihub::rpc::ParamValue;
using primihub::rpc::string_array;
using primihub::rpc::TaskType;

ABSL_FLAG(std::string, server, "127.0.0.1:50050", "server address");

ABSL_FLAG(int, task_type, 0,
          "task type, [0-ACTOR_TASK, 3-PSI_TASK, 2-PIR_TASK, 6-TEE_TASK]");

ABSL_FLAG(std::vector<std::string>,
          params,
          std::vector<std::string>(
              {"BatchSize:INT32:0:128", "NumIters:INT32:0:100",
               "Data_File:STRING:0:train_party_0;train_party_1;train_party_2",
               "TestData:STRING:0:test_party_0;test_party_1;test_party_2",
               "modelFileName:STRING:0:./model",
               "hostLookupTable:STRING:0:./hostlookuptable.csv",
               "guestLookupTable:STRING:0:./guestlookuptable.csv",
               "predictFileName:STRING:0:./prediction.csv",
               "indicatorFileName:STRING:0:./indicator.csv"}),
          "task params, format is <name, type, is array, value>");

ABSL_FLAG(std::vector<std::string>,
          input_datasets,
          std::vector<std::string>({"Data_File", "TestData"}),
          "input datasets name list");

ABSL_FLAG(std::string, job_id, "100", "job id");
ABSL_FLAG(std::string, task_id, "200", "task id");
ABSL_FLAG(std::string, task_lang, "proto", "task language, proto or python");
ABSL_FLAG(std::string, task_code, "logistic_regression", "task code");
ABSL_FLAG(bool, use_tls, false, "true/false");
ABSL_FLAG(std::vector<std::string>, cert_config,
            std::vector<std::string>({
                "data/cert/ca.crt",
                "data/cert/client.key",
                "data/cert/client.crt"}),
            "cert config");

ABSL_FLAG(std::string, task_config_file, "", "path of task config file");

std::map<std::string, TaskType> task_type_map {
  {"ACTOR_TASK", TaskType::ACTOR_TASK},
  {"PIR_TASK", TaskType::PIR_TASK},
  {"PSI_TASK", TaskType::PSI_TASK},
  {"TEE_TASK", TaskType::TEE_TASK},
};

primihub::retcode getTaskType(const std::string& task_type_str, TaskType* task_type) {
  auto it = task_type_map.find(task_type_str);
  if (it != task_type_map.end()) {
    *task_type = it->second;
    return primihub::retcode::SUCCESS;
  } else {
    LOG(ERROR) << "unknown task type: " << task_type_str;
    return primihub::retcode::FAIL;
  }
}
namespace primihub {
namespace client {
using retcode = primihub::retcode;
retcode getFileContents(const std::string& fpath, std::string* contents) {
  bool exist_flag = primihub::FileExists(fpath);
  if (!exist_flag) {
    LOG(ERROR) << "task config file is not exist, path: " << fpath;
    return retcode::FAIL;
  }
  std::ifstream f_in_stream(fpath);
  std::string contents_((std::istreambuf_iterator<char>(f_in_stream)),
                      std::istreambuf_iterator<char>());
  if (contents_.empty()) {
    LOG(ERROR) << "task config is empty, please check, path: " << fpath;
    return retcode::FAIL;
  }
  *contents = std::move(contents_);
  return retcode::SUCCESS;
}
}

void fill_param(const std::vector<std::string>& params,
                google::protobuf::Map<std::string, ParamValue>* param_map) {
    for (std::string param : params) {
        std::vector<std::string> v;
        std::stringstream ss(param);
        std::string substr;
        for (int i = 0; i < 3; i++) {
            getline(ss, substr, ':');
            v.push_back(substr);
        }
        getline(ss, substr);
        v.push_back(substr);

        ParamValue pv;
        bool is_array{false};
        if (v[2] == std::string("1")) {
            pv.set_is_array(true);
            is_array = true;
        } else {
            pv.set_is_array(false);
        }

        if (v[1] == "INT32") {
            pv.set_var_type(VarType::INT32);
            if (is_array) {
                auto array_value = pv.mutable_value_int32_array();
                std::vector<std::string> result;
                str_split(v[3], &result, ';');
                for (const auto& r : result) {
                    array_value->add_value_int32_array(std::stoi(r));
                }
            } else {
                pv.set_value_int32(std::stoi(v[3]));
            }

        } else if (v[1] == "STRING") {
            pv.set_var_type(VarType::STRING);
            pv.set_value_string(v[3]);
        } else if (v[1] == "BYTE") {
            // TODO
        }

        (*param_map)[v[0]] = pv;
    }
}

retcode buildRequestWithFlag(PushTaskRequest* request) {
    auto task_ptr = request->mutable_task();
    int task_type_flag = absl::GetFlag(FLAGS_task_type);
    auto task_type = static_cast<TaskType>(task_type_flag);
    task_ptr->set_type(task_type);

    // Setup task params
    task_ptr->set_name("demoTask");

    auto task_lang = absl::GetFlag(FLAGS_task_lang);
    if (task_lang == "proto") {
        task_ptr->set_language(Language::PROTO);
    } else if (task_lang == "python") {
        task_ptr->set_language(Language::PYTHON);
    } else {
        std::cerr << "task language not supported" << std::endl;
        return retcode::FAIL;
    }

    // google::protobuf::Map<std::string, ParamValue>* map =
    auto map = task_ptr->mutable_params()->mutable_param_map();
    const std::vector<std::string> params = absl::GetFlag(FLAGS_params);
    fill_param(params, map);
    // if given task code file, read it and set task code
    auto task_code = absl::GetFlag(FLAGS_task_code);
    auto code_ptr = task_ptr->mutable_code();
    if (task_code.find('/') != std::string::npos ||
        task_code.find('\\') != std::string::npos) {
        // read file
        std::ifstream ifs(task_code);
        if (!ifs.is_open()) {
            std::cerr << "open file failed: " << task_code << std::endl;
            return retcode::FAIL;
        }
        std::stringstream buffer;
        buffer << ifs.rdbuf();
        std::cout << buffer.str() << std::endl;
        *code_ptr = buffer.str();
    } else {
        // read code from command line
        *code_ptr = task_code;
    }

    // Setup input datasets
    if (task_lang == "proto" && task_type != TaskType::TEE_TASK) {
        auto input_datasets = absl::GetFlag(FLAGS_input_datasets);
        for (int i = 0; i < input_datasets.size(); i++) {
            task_ptr->add_input_datasets(input_datasets[i]);
        }
    }

    // TEE task
    if (task_type == TaskType::TEE_TASK) {
        task_ptr->add_input_datasets("datasets");
    }
    return retcode::SUCCESS;
}

void fillParamByArray(const std::string& value_type,
        const nlohmann::json& obj, ParamValue* pv) {
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
        const nlohmann::json& obj, ParamValue* pv) {
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

retcode BuildFederatedRequest(const nlohmann::json& js_task_config, rpc::Task* task_ptr) {
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
  // parse party info
  if (js_task_config.contains("party_info")) {
    auto access_info_ptr = task_ptr->mutable_party_access_info();
    for (auto& [party_name, host] : js_task_config["party_info"].items()) {
      if (all_parties.find(party_name) == all_parties.end()) {
        LOG(WARNING) << party_name << " is not one of party in this task, ignore....";
        continue;
      }
      auto& access_info = (*access_info_ptr)[party_name];
      access_info.set_ip(host["ip"].get<std::string>());
      access_info.set_port(host["port"].get<uint32_t>());
      access_info.set_use_tls(host["use_tls"].get<bool>());
      all_parties.emplace(party_name);
    }
  }

  std::string component_params_str = component_params.dump();
  auto param_map_ptr = task_ptr->mutable_params()->mutable_param_map();
  // for (const auto& party_name : all_parties) {
  auto& pv_config = (*param_map_ptr)["component_params"];
  pv_config.set_is_array(false);
  pv_config.set_var_type(rpc::VarType::STRING);
  pv_config.set_value_string(component_params_str);
  // }
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

retcode buildRequestWithTaskConfigFile(const std::string& file_path,
                                       PushTaskRequest* request) {
  try {
    std::string task_config_content;
    auto retcode = primihub::client::getFileContents(file_path,
                                                     &task_config_content);
    if (retcode != retcode::SUCCESS) {
      LOG(ERROR) << "read task config failed";
      return retcode::FAIL;
    }
    auto js = nlohmann::json::parse(task_config_content);
    auto task_ptr = request->mutable_task();
    TaskType task_type;
    if (js.contains("task_type")) {
      std::string task_type_str = js["task_type"].get<std::string>();
      auto ret = getTaskType(task_type_str, &task_type);
      if (ret != retcode::SUCCESS) {
        LOG(ERROR) << "getTaskType failed";
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
        task_ptr->set_language(Language::PROTO);
      } else if (task_lang == "python") {
        task_ptr->set_language(Language::PYTHON);
      } else {
        LOG(ERROR) << "task language not supported" << std::endl;
        return retcode::FAIL;
      }
    }

    // param
    auto map = task_ptr->mutable_params()->mutable_param_map();
    for (auto& [key, value_info]: js["params"].items()) {
        auto value_type = value_info["type"].get<std::string>();
        ParamValue pv;
        if (value_info["value"].is_array()) {
            fillParamByArray(value_type, value_info["value"], &pv);
        } else {
            fillParamByScalar(value_type, value_info["value"], &pv);
        }
        (*map)[key] = std::move(pv);
    }
    // code
    if (js.contains("task_code")) {
      std::string code_file_path = js["task_code"]["code_file_path"].get<std::string>();
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
    if (js.contains("party_access_info")) {
      for (auto& [key, value]: js["party_access_info"].items()) {
        auto& party_node = (*party_access_info)[key];
        std::string ip = value["ip"].get<std::string>();
        int32_t port = value["port"].get<int32_t>();
        bool use_tls = value["use_tls"].get<bool>();
        party_node.set_ip(ip);
        party_node.set_port(port);
        party_node.set_use_tls(use_tls);
      }
    }
    // dataset
    if (task_lang == "proto" && task_type != TaskType::TEE_TASK) {
      auto input_datasets = absl::GetFlag(FLAGS_input_datasets);
      for (auto& item : js["input_datasets"]) {
        std::string dataset = item.get<std::string>();
        task_ptr->add_input_datasets(std::move(dataset));
      }
    }
    // TEE task
    if (task_type == TaskType::TEE_TASK) {
      task_ptr->add_input_datasets("datasets");
    }
    BuildFederatedRequest(js, task_ptr);
    // std::string str;
    // google::protobuf::TextFormat::PrintToString(*request, &str);
    // LOG(INFO) << "BuildedRequest: " << str;
  } catch(std::exception& e) {
    LOG(ERROR) << "catch exception: " << e.what();
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

int SDKClient::SubmitTask() {
    PushTaskRequest pushTaskRequest;
    PushTaskReply pushTaskReply;
    grpc::ClientContext context;

    std::string task_config_file = absl::GetFlag(FLAGS_task_config_file);
    if (task_config_file.empty()) {
        buildRequestWithFlag(&pushTaskRequest);
    } else {
      auto ret = buildRequestWithTaskConfigFile(task_config_file,
                                                &pushTaskRequest);
      if (ret != retcode::SUCCESS) {
        LOG(ERROR) << "buildRequestWithTaskConfigFile failed";
        return -1;
      }
    }
    // pushTaskRequest.set_submit_client_id("client_id_test");
    pushTaskRequest.set_intended_worker_id("1");

    // TODO Generate job id and task id
    std::random_device rd;
    auto seed_data = std::array<int, std::mt19937::state_size> {};
    std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
    std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
    std::mt19937 generator(seq);
    uuids::uuid_random_generator gen{generator};
    const uuids::uuid id = gen();
    std::string task_id = absl::GetFlag(FLAGS_task_id);
    std::string job_id = absl::GetFlag(FLAGS_job_id);
    std::string request_id = uuids::to_string(id);
    auto task_info = pushTaskRequest.mutable_task()->mutable_task_info();
    task_info->set_job_id(job_id);
    task_info->set_task_id(task_id);
    task_info->set_request_id(request_id);
    pushTaskRequest.set_sequence_number(11);
    pushTaskRequest.set_client_processed_up_to(22);
    pushTaskRequest.set_submit_client_id(job_id + "_" + task_id);
    LOG(INFO) << " SubmitTask...";

    auto ret = channel_->submitTask(pushTaskRequest, &pushTaskReply);
    if (ret != retcode::SUCCESS) {
      LOG(ERROR) << "submitTask encountes error";
      return -1;
    }
    size_t party_count = pushTaskReply.party_count();
    if (party_count < 1) {
      LOG(ERROR) << "party count from reply is: " << party_count;
      return -1;
    }
    VLOG(0) << "party count: " << party_count;
    std::map<std::string, std::string> task_status;
    // fecth task status
    size_t fetch_count = 0;
    do {
        rpc::TaskContext request;
        request.set_task_id(task_id);
        request.set_job_id(job_id);
        request.set_request_id(request_id);
        rpc::TaskStatusReply status_reply;
        auto ret = channel_->fetchTaskStatus(request, &status_reply);
        // parse task status
        if (ret != retcode::SUCCESS) {
            LOG(ERROR) << "fetch task status from server failed";
            return -1;
        }
        bool is_finished{false};
        for (const auto& status_info : status_reply.task_status()) {
            auto party = status_info.party();
            auto status_code = status_info.status();
            auto message = status_info.message();
            VLOG(5) << "task_status party: " << party << " "
                << "status: " << static_cast<int>(status_code) << " "
                << "message: " << message;
            if (status_info.status() == primihub::rpc::TaskStatus::SUCCESS ||
              status_info.status() == primihub::rpc::TaskStatus::FAIL) {
                task_status[party] = message;
            }
            if (task_status.size() == party_count) {
                VLOG(0) << "all node has finished";
                for (const auto& [party_name, msg_info] : task_status) {
                  LOG(INFO) << "party name: " << party_name
                            << " msg: " << msg_info;
                }
                is_finished = true;
                break;
            }
        }
        if (is_finished) {
            break;
        }
        // for test active kill task
        // {
        //   std::this_thread::sleep_for(std::chrono::seconds(10));
        //   rpc::KillTaskRequest request;
        //   auto task_info = request.mutable_task_info();
        //   task_info->set_task_id(task_id);
        //   task_info->set_job_id(job_id);
        //   task_info->set_request_id(request_id);
        //   request.set_executor(rpc::KillTaskRequest::CLIENT);
        //   rpc::KillTaskResponse status_reply;
        //   auto ret = channel_->killTask(request, &status_reply);
        // }

        if (fetch_count < 1000) {
            std::this_thread::sleep_for(std::chrono::milliseconds(fetch_count*100));
        } else {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } while (true);
    return 0;
}

Node getNode(const std::string& server_info, bool use_tls) {
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

}  // namespace primihub

int main(int argc, char** argv) {
    google::InitGoogleLogging(argv[0]);
    // Set Color logging on
    FLAGS_colorlogtostderr = true;
    // Set stderr logging on
    FLAGS_alsologtostderr = true;
    // Set log output directory
    FLAGS_log_dir = "./log/";
    // Set log max size (MB)
    FLAGS_max_log_size = 10;
    // Set stop logging if disk full on
    FLAGS_stop_logging_if_full_disk = true;

    absl::ParseCommandLine(argc, argv);

    std::vector<std::string> peers;
    peers.push_back(absl::GetFlag(FLAGS_server));
    auto cert_config_path = absl::GetFlag(FLAGS_cert_config);
    auto use_tls = absl::GetFlag(FLAGS_use_tls);
    LOG(INFO) << "use tls: " << use_tls;
    auto link_mode = primihub::network::LinkMode::GRPC;
    auto link_ctx = primihub::network::LinkFactory::createLinkContext(link_mode);
    if (use_tls) {
        auto& ca_path = cert_config_path[0];
        auto& key_path = cert_config_path[1];
        auto& cert_path = cert_config_path[2];
        primihub::common::CertificateConfig cert_cfg(ca_path, key_path, cert_path);
        link_ctx->initCertificate(cert_cfg);
    }
    for (auto peer : peers) {
        LOG(INFO) << "SDK SubmitTask to: " << peer;
        // auto channel = primihub::buildChannel(peer, use_tls, cert_config);
        auto peer_node = primihub::getNode(peer, use_tls);
        auto channel = link_ctx->getChannel(peer_node);
        if (channel == nullptr) {
            LOG(ERROR) << "link_ctx->getChannel(peer_node); failed";
            return -1;
        }
        primihub::SDKClient client(channel, link_ctx.get());
        auto _start = std::chrono::high_resolution_clock::now();
        auto ret = client.SubmitTask();
        auto _end = std::chrono::high_resolution_clock::now();
        auto time_cost = std::chrono::duration_cast<std::chrono::milliseconds>(_end - _start).count();
        LOG(INFO) << "SubmitTask time cost(ms): " << time_cost;
        if (!ret) {
            break;
        }
    }
    return 0;
}
