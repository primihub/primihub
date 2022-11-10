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

#include <fstream>
#include <iostream>
#include <memory>
#include <signal.h>
#include <sstream>
#include <unistd.h>

#include <arrow/flight/server.h>
#include <arrow/util/logging.h>

#include "src/primihub/data_store/factory.h"
#include "src/primihub/node/ds.h"
#include "src/primihub/node/node.h"
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/service/dataset/util.hpp"
#include "src/primihub/task/language/factory.h"
#include "src/primihub/task/semantic/parser.h"
#include "src/primihub/util/file_util.h"
#include "src/primihub/util/log_wrapper.h"

using grpc::Server;
using primihub::rpc::Params;
using primihub::rpc::ParamValue;
using primihub::rpc::PsiType;
using primihub::rpc::TaskType;
using primihub::task::LanguageParserFactory;
using primihub::task::ProtocolSemanticParser;

ABSL_FLAG(std::string, node_id, "node0", "unique node_id");
ABSL_FLAG(std::string, config, "./config/node.yaml", "config file");
ABSL_FLAG(bool, singleton, false, "singleton mode"); // TODO: remove this flag
ABSL_FLAG(int, service_port, 50050, "node service port");

namespace primihub {
#define PLATFORM typeToName(PlatFormType::WORKER_NODE)
#undef V_VLOG
#define V_VLOG(level, PLATFORM, JOB_ID, TASK_ID) \
    VLOG_WRAPPER(level, PLATFORM, JOB_ID, TASK_ID)
#undef LOG_INFO
#define LOG_INFO(PLATFORM, JOB_ID, TASK_ID) \
    LOG_INFO_WRAPPER(PLATFORM, JOB_ID, TASK_ID)
#undef LOG_WRANING
#define LOG_WRANING(PLATFORM, JOB_ID, TASK_ID) \
    LOG_WRANING_WRAPPER(PLATFORM, JOB_ID, TASK_ID)
#undef LOG_ERROR
#define LOG_ERROR(PLATFORM, JOB_ID, TASK_ID) \
    LOG_ERROR_WRAPPER(PLATFORM, JOB_ID, TASK_ID)

Status VMNodeImpl::Send(ServerContext* context,
        ServerReader<TaskRequest>* reader, TaskResponse* response) {
    VLOG(5) << "VMNodeImpl::Send: received: ";

    bool recv_meta_info{false};
    std::string job_id;
    std::string task_id;
    int storage_type{-1};
    std::string storage_info;
    std::vector<std::string> recv_data;
    VLOG(5) << "VMNodeImpl::Send: received xxx: ";
    TaskRequest request;
    while (reader->Read(&request)) {
        if (!recv_meta_info) {
            job_id = request.job_id();
            task_id = request.task_id();
            storage_type = request.storage_type();
            storage_info = request.storage_info();
            recv_meta_info = true;
            V_VLOG(5, PLATFORM, job_id, task_id) << "storage_type: " << storage_type << " "
                    << "storage_info: " << storage_info;
        }
        auto item_size = request.data().size();
        V_VLOG(5, PLATFORM, job_id, task_id) << "item_size: " << item_size;
        for (size_t i = 0; i < item_size; i++) {
            recv_data.push_back(request.data(i));
        }
    }

    if (storage_type == primihub::rpc::TaskRequest::FILE) {
        std::string data_path = storage_info;
        save_data_to_file(job_id, task_id, data_path, std::move(recv_data));
    }
    V_VLOG(5, PLATFORM, job_id, task_id) << "end of read data from client";
    response->set_ret_code(primihub::rpc::retcode::SUCCESS);
    return Status::OK;
}

int VMNodeImpl::save_data_to_file(const std::string& job_id, const std::string& task_id,
        const std::string& data_path, std::vector<std::string>&& save_data) {
    if (ValidateDir(data_path)) {
        LOG_ERROR(PLATFORM, job_id, task_id) << "file path is not exist, please check";
        return -1;
    }
    auto data = std::move(save_data);
    if (data.empty()) {
        return 0;
    }
    std::ofstream in_fs(data_path, std::ios::out);
    for (const auto& data_item : data) {
        in_fs << data_item << "\n";
    }
    in_fs.close();
    return 0;
}

Status VMNodeImpl::SubmitTask(ServerContext *context,
                              const PushTaskRequest *pushTaskRequest,
                              PushTaskReply *pushTaskReply) {
//
    const auto& job_id = pushTaskRequest->task().job_id();
    const auto& task_id = pushTaskRequest->task().task_id();
    std::string str;
    google::protobuf::TextFormat::PrintToString(*pushTaskRequest, &str);
    LOG_INFO(PLATFORM, job_id, task_id) << str << std::endl;
    std::string job_task = job_id + task_id;
    pushTaskReply->set_job_id(job_id);

    // if (running_set.find(job_task) != running_set.end()) {
    //     pushTaskReply->set_ret_code(1);
    //     return Status::OK;
    // }

    // actor
    auto task_type = pushTaskRequest->task().type();
    if (task_type == primihub::rpc::TaskType::ACTOR_TASK ||
        task_type == primihub::rpc::TaskType::TEE_TASK) {
        LOG_INFO(PLATFORM, job_id, task_id) << "start to schedule task";
        // absl::MutexLock lock(&parser_mutex_);
        // Construct language parser
        std::shared_ptr<LanguageParser> lan_parser_ =
            LanguageParserFactory::Create(*pushTaskRequest);
        if (lan_parser_ == nullptr) {
            pushTaskReply->set_ret_code(1);
            return Status::OK;
        }
        lan_parser_->parseTask();
        lan_parser_->parseDatasets();

        // Construct protocol semantic parser
        auto _psp = ProtocolSemanticParser(this->node_id, this->singleton,
                                           this->nodelet->getDataService());
        // Parse and dispatch task.
        _psp.parseTaskSyntaxTree(lan_parser_);
    } else if (task_type == primihub::rpc::TaskType::PIR_TASK ||
            task_type == primihub::rpc::TaskType::PSI_TASK) {
        LOG_INFO(PLATFORM, job_id, task_id) << "start to schedule task";
        absl::MutexLock lock(&parser_mutex_);
        std::shared_ptr<LanguageParser> lan_parser_ =
            LanguageParserFactory::Create(*pushTaskRequest);
        if (lan_parser_ == nullptr) {
            pushTaskReply->set_ret_code(1);
            return Status::OK;
        }
        lan_parser_->parseDatasets();

        // Construct protocol semantic parser
        auto _psp = ProtocolSemanticParser(this->node_id, this->singleton,
                                           this->nodelet->getDataService());
        VLOG(5) << "Construct protocol semantic parser finished";
        // Parse and dispathc pir task.
        if (pushTaskRequest->task().type() ==
            primihub::rpc::TaskType::PIR_TASK) {
            _psp.schedulePirTask(lan_parser_, this->nodelet->getNodeletAddr());
        } else {
            _psp.schedulePsiTask(lan_parser_);
        }
        auto _type = static_cast<int>(pushTaskRequest->task().type());
        V_VLOG(5, PLATFORM, job_id, task_id) << "end schedule schedule task for type: " << _type;
    } else {
        LOG_INFO(PLATFORM, job_id, task_id) << "start to create worker for task";
        running_set.insert(job_task);
        std::shared_ptr<Worker> worker = CreateWorker();
        worker->execute(pushTaskRequest);
        running_set.erase(job_task);
    }
    return Status::OK;
}

/***********************************************
 *
 * method runs on the node as psi or pir server
 *
 * *********************************************/
int VMNodeImpl::process_psi_response(const ExecuteTaskResponse& response,
          std::vector<ExecuteTaskResponse>* splited_responses) {
//
    splited_responses->clear();
    size_t limited_size = 1 << 21; // 4M
    const auto& psi_res = response.psi_response();
    const auto& server_setup = psi_res.server_setup();
    size_t encrypted_elements_num = psi_res.encrypted_elements().size();
    const auto& encrypted_elements = psi_res.encrypted_elements();
    size_t sended_size = 0;
    size_t sended_index = 0;
    do {
        ExecuteTaskResponse sub_resp;
        auto sub_psi_res = sub_resp.mutable_psi_response();
        sub_psi_res->set_ret_code(0);
        size_t pack_size = 0;
        auto sub_server_setup = sub_psi_res->mutable_server_setup();
        sub_server_setup->CopyFrom(server_setup);
        for (size_t i = sended_index; i < encrypted_elements_num; i++) {
            const auto& data_item = encrypted_elements[i];
            size_t item_len = data_item.size();
            if (pack_size + item_len > limited_size) {
                break;
            }
            sub_psi_res->add_encrypted_elements(data_item);
            pack_size += item_len;
            sended_index++;
        }
        splited_responses->push_back(std::move(sub_resp));
        VLOG(5) << "pack_size+pack_size: " << pack_size;
        if (sended_index >= encrypted_elements_num) {
            break;
        }
    } while(true);
    return 0;
}

int VMNodeImpl::process_pir_response(const ExecuteTaskResponse& response,
        std::vector<ExecuteTaskResponse>* splited_responses) {
//
    splited_responses->clear();
    size_t limited_size = 1 << 21; // 4M
    const auto& pir_res = response.pir_response();
    size_t reply_num = pir_res.reply().size();
    const auto& reply =  pir_res.reply();
    size_t sended_size = 0;
    size_t sended_index = 0;
    do {
        ExecuteTaskResponse sub_resp;
        auto sub_pir_res = sub_resp.mutable_pir_response();
        sub_pir_res->set_ret_code(0);
        size_t pack_size = 0;
        for (size_t i = sended_index; i < reply_num; i++) {
            // get query len
            size_t query_size = 0;
            for (const auto& ct : reply[i].ct()) {
                query_size += ct.size();
            }
            if (pack_size + query_size > limited_size) {
                break;
            }
            auto _reply = sub_pir_res->add_reply();
            for (const auto& ct : reply[i].ct()) {
                _reply->add_ct(ct);
            }
            sended_index++;
        }
        splited_responses->push_back(std::move(sub_resp));
        if (sended_index >= reply_num) {
            break;
        }
    } while(true);
    return 0;
}

int VMNodeImpl::process_task_reseponse(bool is_psi_response,
        const ExecuteTaskResponse& response, std::vector<ExecuteTaskResponse>* splited_responses) {
    if (is_psi_response) {
        process_psi_response(response, splited_responses);
    } else  {  // pir response
        process_pir_response(response, splited_responses);
    }
    return 0;
}

Status VMNodeImpl::ExecuteTask(ServerContext* context,
        grpc::ServerReaderWriter<ExecuteTaskResponse, ExecuteTaskRequest>* stream) {
// recv data from client
    VLOG(5) << "VMNodeImpl::ExecuteTask";
    ExecuteTaskRequest task_request;
    ExecuteTaskResponse task_response;
    std::string job_task = "";
    primihub::rpc::TaskType taskType;
    ExecuteTaskRequest recv_request;
    bool first_visit_flag{false};
    bool is_psi_request{false};
    bool is_pir_request{false};
    while (stream->Read(&recv_request)) {
        if (!first_visit_flag) {
            auto ptr_params = task_request.mutable_params()->mutable_param_map();
            const auto& _params = recv_request.params().param_map();
            for (auto it = _params.begin(); it != _params.end(); it++) {
                const auto& key = it->first;
                const auto& value = it->second;
                ParamValue pv;
                pv.CopyFrom(value);
                (*ptr_params)[key] = pv;
            }
            auto req_type = recv_request.algorithm_request_case();
            if (req_type == ExecuteTaskRequest::AlgorithmRequestCase::kPsiRequest) {
                is_psi_request = true;
                taskType = primihub::rpc::TaskType::NODE_PSI_TASK;
                const auto& psi_req = recv_request.psi_request();
                job_task = psi_req.job_id() + psi_req.task_id();
                auto psi_request = task_request.mutable_psi_request();
                psi_request->set_job_id(psi_req.job_id());
                psi_request->set_task_id(psi_req.task_id());
                psi_request->set_reveal_intersection(psi_req.reveal_intersection());
            } else if (req_type == ExecuteTaskRequest::AlgorithmRequestCase::kPirRequest) {
                is_pir_request = true;
                taskType = primihub::rpc::TaskType::NODE_PIR_TASK;
                const auto& pir_req = recv_request.pir_request();
                job_task = pir_req.job_id() + pir_req.task_id();
                auto pir_request = task_request.mutable_pir_request();
                pir_request->set_galois_keys(pir_req.galois_keys());
                pir_request->set_relin_keys(pir_req.relin_keys());
                pir_request->set_job_id(pir_req.job_id());
                pir_request->set_task_id(pir_req.task_id());
            }
            first_visit_flag = true;
        }
        if (is_psi_request) {
            const auto& recved_psi_req = recv_request.psi_request();
            auto psi_request = task_request.mutable_psi_request();
            for (const auto& encrypted_element : recved_psi_req.encrypted_elements()) {
                psi_request->add_encrypted_elements(encrypted_element);
            }
        } else if (is_pir_request) {
            const auto& recved_pir_req = recv_request.pir_request();
            auto pir_request = task_request.mutable_pir_request();
            for (const auto& query : recved_pir_req.query()) {
                auto pir_query = pir_request->add_query();
                for (const auto& ct : query.ct()) {
                    pir_query->add_ct(ct);
                }
            }
        }
    }
    if (running_set.find(job_task) != running_set.end()) {
        if (is_psi_request) {
            task_response.mutable_psi_response()->set_ret_code(1);
        } else if (is_pir_request) {
            task_response.mutable_pir_response()->set_ret_code(1);
        }
        stream->Write(task_response);
        return Status::OK;
    }
    VLOG(5) << "VMNodeImpl::ExecuteTask recv data finished";
    // process receive data
    if (taskType == primihub::rpc::TaskType::NODE_PSI_TASK ||
        taskType == primihub::rpc::TaskType::NODE_PIR_TASK) {
        const auto& task_id = taskRequest->psi_request().task_id();
        const auto& job_id = taskRequest->psi_request().job_id();
        LOG_INFO(PLATFORM, job_id, task_id) << "Start to create PSI/PIR server task";
        running_set.insert(job_task);
        std::shared_ptr<Worker> worker = CreateWorker();
        running_map.insert({job_task, worker});
        worker->execute(&task_request, &task_response);
        running_map.erase(job_task);
        running_set.erase(job_task);
    }

    std::vector<ExecuteTaskResponse> splited_resp;
    process_task_reseponse(is_psi_request, task_response, &splited_resp);
    VLOG(5) << "ExecuteTaskResponse size: " << splited_resp.size();
    for (const auto& res : splited_resp) {
        stream->Write(res);
    }
    return Status::OK;
}

std::shared_ptr<Worker> VMNodeImpl::CreateWorker() {
    auto worker = std::make_shared<Worker>(this->node_id, this->nodelet);
    LOG(INFO) << " 🤖️ Start create worker " << this->node_id;
    // absl::MutexLock lock(&worker_map_mutex_);

    // workers_.emplace("simple_test_worker", worker);
    LOG(INFO) << " 🤖️ Fininsh create worker " << this->node_id;
    return worker;
}

/**
 * @brief
 *  Start Apache arrow flight server with NodeService and DatasetService.
 */
void RunServer(primihub::VMNodeImpl *node_service,
        primihub::DataServiceImpl *dataset_service, int service_port) {

    // Initialize server
    arrow::flight::Location location;

    // Listen to all interfaces
    ARROW_CHECK_OK(arrow::flight::Location::ForGrpcTcp("0.0.0.0", service_port,
                                                       &location));
    arrow::flight::FlightServerOptions options(location);
    auto server = std::unique_ptr<arrow::flight::FlightServerBase>(
        new primihub::service::FlightIntegrationServer(
            node_service->getNodelet()->getDataService()));

    // Use builder_hook to register grpc service
    options.builder_hook = [&](void *raw_builder) {
        auto *builder = reinterpret_cast<grpc::ServerBuilder *>(raw_builder);
        builder->RegisterService(node_service);
        builder->RegisterService(dataset_service);

        // set the max message size to 128M
        builder->SetMaxReceiveMessageSize(128 * 1024 * 1024);
    };

    // Start the server
    ARROW_CHECK_OK(server->Init(options));
    // Exit with a clean error code (0) on SIGTERM
    ARROW_CHECK_OK(server->SetShutdownOnSignals({SIGTERM}));

    LOG(INFO) << " 💻 Node listening on port: " << service_port;

    ARROW_CHECK_OK(server->Serve());
}

} // namespace primihub

primihub::VMNodeImpl *node_service;
primihub::DataServiceImpl *data_service;

int main(int argc, char **argv) {

    // std::atomic<bool> quit(false);    // signal flag for server to quit
    // Register SIGINT signal and signal handler

    signal(SIGINT, [](int sig) {
        LOG(INFO) << " 👋 Node received SIGINT signal, shutting down...";
        delete node_service;
        delete data_service;
        exit(0);
    });

    py::scoped_interpreter python;
    py::gil_scoped_release release;

    google::InitGoogleLogging(argv[0]);
    FLAGS_colorlogtostderr = true;
    FLAGS_alsologtostderr = true;
    FLAGS_log_dir = "./log";
    FLAGS_max_log_size = 10;
    FLAGS_stop_logging_if_full_disk = true;

    absl::ParseCommandLine(argc, argv);
    const std::string node_id = absl::GetFlag(FLAGS_node_id);
    bool singleton = absl::GetFlag(FLAGS_singleton);

    int service_port = absl::GetFlag(FLAGS_service_port);
    std::string config_file = absl::GetFlag(FLAGS_config);

    std::string node_ip = "0.0.0.0";
    node_service = new primihub::VMNodeImpl(node_id, node_ip, service_port,
                                            singleton, config_file);
    data_service = new primihub::DataServiceImpl(
        node_service->getNodelet()->getDataService(),
        node_service->getNodelet()->getNodeletAddr());
    primihub::RunServer(node_service, data_service, service_port);

    return EXIT_SUCCESS;
}
