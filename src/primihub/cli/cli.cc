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
#include "src/primihub/common/config/config.h"
#include "src/primihub/util/network/link_factory.h"
#include "src/primihub/common/common.h"
#include "uuid.h"

using primihub::rpc::ParamValue;
using primihub::rpc::string_array;
using primihub::rpc::TaskType;

ABSL_FLAG(std::string, server, "127.0.0.1:50050", "server address");

ABSL_FLAG(std::string, task_type, "ACTOR_TASK",
          "task type, [ACTOR_TASK, PSI_TASK, PIR_TASK, TEE_TASK]");

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

ABSL_FLAG(std::string, task_config_file, "example/psi_ecdh_task_conf.json", "path of task config file");

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
int getFileContents(const std::string& fpath, std::string* contents) {
    std::ifstream f_in_stream(fpath);
    std::string contents_((std::istreambuf_iterator<char>(f_in_stream)),
                        std::istreambuf_iterator<char>());
    *contents = std::move(contents_);
    return 0;
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
int get_task_execute_status(const primihub::rpc::Node& notify_server,
        const PushTaskRequest& request_info) {
    LOG(INFO) << "get_task_execute_status for "
        << notify_server.ip() << " port: " << notify_server.port()
        <<  " use_tls: " << notify_server.use_tls();
    std::string node_info = notify_server.ip() + ":" + std::to_string(notify_server.port());
    auto channel = grpc::CreateChannel(node_info, grpc::InsecureChannelCredentials());
    auto stub = primihub::rpc::NodeService::NewStub(channel);
    using ClientContext = primihub::rpc::ClientContext;
    using NodeEventReply = primihub::rpc::NodeEventReply;
    bool has_error{false};
    do {
        bool is_finished{false};
        grpc::ClientContext context;
        ClientContext request;
        request.set_client_id(request_info.submit_client_id());
        request.set_client_ip("127.0.0.1");
        request.set_client_port(12345);
        std::unique_ptr<grpc::ClientReader<NodeEventReply>> reader(
            stub->SubscribeNodeEvent(&context, request));
        NodeEventReply reply;
        std::string server_node = notify_server.ip();
        while (reader->Read(&reply)) {
            const auto& task_status = reply.task_status();
            std::string status = task_status.status();
            LOG(INFO) << "get reply from " << server_node << " with status: "
                    << status;
            if (status == std::string("SUCCESS") || status == std::string("FAIL")) {
                is_finished = true;
                if (status == std::string("FAIL")) {
                    has_error = true;
                }
                break;
            }
        }
        if (is_finished) {
            break;
        }
    } while (true);
    if (has_error) {
        return -1;
    }

    return 0;
}

retcode buildRequestWithFlag(PushTaskRequest* request) {
    auto task_ptr = request->mutable_task();
    std::string task_type_str = absl::GetFlag(FLAGS_task_type);
    TaskType task_type;
    auto ret = getTaskType(task_type_str, &task_type);
    if (ret != retcode::SUCCESS) {
        return retcode::FAIL;
    }
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
        task_ptr->set_code(buffer.str());
    } else {
        // read code from command line
        task_ptr->set_code(task_code);
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
      for (const auto& item : obj) {
        // TODO (fix in future)
        auto val = item.get<std::string>();
        pv->set_value_string(std::move(val));
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
    }
}

retcode buildRequestWithTaskConfigFile(const std::string& file_path, PushTaskRequest* request) {
    try {
      std::string task_config_content;
      primihub::client::getFileContents(file_path, &task_config_content);
      auto js = nlohmann::json::parse(task_config_content);
      auto task_ptr = request->mutable_task();
      std::string task_type_str = js["task_type"].get<std::string>();
      TaskType task_type;
      auto ret = getTaskType(task_type_str, &task_type);
      if (ret != retcode::SUCCESS) {
          return retcode::FAIL;
      }
      task_ptr->set_type(task_type);
      // Setup task params
      if (js.contains("task_name")) {
          std::string task_name = js["task_name"].get<std::string>();
          task_ptr->set_name(task_name);
      } else {
          task_ptr->set_name("demoTask");
      }
      // task language
      auto task_lang = js["task_lang"].get<std::string>();
      if (task_lang == "proto") {
          task_ptr->set_language(Language::PROTO);
      } else if (task_lang == "python") {
          task_ptr->set_language(Language::PYTHON);
      } else {
          std::cerr << "task language not supported" << std::endl;
          return retcode::FAIL;
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
      std::string task_code = js["task_code"].get<std::string>();
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
          task_ptr->set_code(buffer.str());
      } else {
          // read code from command line
          task_ptr->set_code(std::move(task_code));
      }
      // dataset
      // Setup input datasets
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
    } catch(std::exception& e) {
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
        buildRequestWithTaskConfigFile(task_config_file, &pushTaskRequest);
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
    std::string task_id = uuids::to_string(id);
    std::string job_id = absl::GetFlag(FLAGS_job_id);
    pushTaskRequest.mutable_task()->set_job_id(job_id);
    pushTaskRequest.mutable_task()->set_task_id(task_id);
    pushTaskRequest.set_sequence_number(11);
    pushTaskRequest.set_client_processed_up_to(22);
    pushTaskRequest.set_submit_client_id(job_id + "_" + task_id);
    LOG(INFO) << " SubmitTask...";

    auto ret = channel_->submitTask(pushTaskRequest, &pushTaskReply);
    if (ret != retcode::SUCCESS) {
        return -1;
    }
    std::vector<std::future<int>> wait_result_futs;
    std::vector<bool> result_fetched;
    for (const auto& server_info : pushTaskReply.notify_server()) {
        wait_result_futs.push_back(
            std::async(
                std::launch::async,
                get_task_execute_status,
                std::ref(server_info),
                std::ref(pushTaskRequest)
            ));
        result_fetched.push_back(false);
    }

    bool has_error{false};
    do {
        for (int i = 0; i < wait_result_futs.size(); i++) {
            if (result_fetched[i]) {
                continue;
            } else {
                auto st = wait_result_futs[i].wait_for(std::chrono::seconds(1));
                if (st == std::future_status::ready) {
                    auto result = wait_result_futs[i].get();
                    result_fetched[i] = true;
                    if (result != 0) {
                        has_error = true;
                        break;
                    }
                }
            }
        }
        bool is_all_fininshed{true};
        for (auto fetched : result_fetched) {
            if (!fetched) {
                is_all_fininshed = false;
            }
        }
        if (is_all_fininshed) {
            break;
        }
        if (has_error) {
            break;
        }
    } while (true);

    if (has_error) {
        LOG(INFO) << "begin to send kill task";
        for (const auto& server_info: pushTaskReply.task_server()) {
            VLOG(0) << "node: " << server_info.ip() << " port: " << server_info.port();
            Node remote_node(server_info.ip(), server_info.port(), server_info.use_tls());
            auto channel = link_ctx_ref_->getChannel(remote_node);
            rpc::KillTaskRequest request;
            request.set_job_id(job_id);
            request.set_task_id(task_id);
            request.set_executor(rpc::KillTaskRequest::SCHEDULER);
            rpc::KillTaskResponse reponse;
            channel->killTask(request, &reponse);
        }
        LOG(INFO) << "end of send kill task to all node";
        do {
            for (int i = 0; i < wait_result_futs.size(); i++) {
                if (result_fetched[i]) {
                    continue;
                } else {
                    auto st = wait_result_futs[i].wait_for(std::chrono::seconds(1));
                    if (st == std::future_status::ready) {
                        auto result = wait_result_futs[i].get();
                        result_fetched[i] = true;
                        if (result != 0) {
                            has_error = true;
                            break;
                        }
                    }
                }
            }
            bool all_fetched = true;
            for (size_t i = 0; i < result_fetched.size(); i++) {
                if (!result_fetched[i]) {
                    all_fetched = false;
                }
            }
            if (all_fetched) {
                break;
            }
        } while (true);
    }
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
    auto link_ctx = primihub::network::LinkFactory::createLinkContext(primihub::network::LinkMode::GRPC);
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
