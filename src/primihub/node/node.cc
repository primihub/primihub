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
Status VMNodeImpl::ExecuteTask(ServerContext *context,
                               const ExecuteTaskRequest *taskRequest,
                               ExecuteTaskResponse *taskResponse) {
    VLOG(5) << "VMNodeImpl::ExecuteTask";
    std::string job_task = "";
    primihub::rpc::TaskType taskType;
    if (taskRequest->algorithm_request_case() ==
        ExecuteTaskRequest::AlgorithmRequestCase::kPsiRequest) {
        taskType = primihub::rpc::TaskType::NODE_PSI_TASK;
        job_task = taskRequest->psi_request().job_id() +
                   taskRequest->psi_request().task_id();
        if (running_set.find(job_task) != running_set.end()) {
            taskResponse->mutable_psi_response()->set_ret_code(1);
            return Status::OK;
        }
    } else if (taskRequest->algorithm_request_case() ==
               ExecuteTaskRequest::AlgorithmRequestCase::kPirRequest) {
        taskType = primihub::rpc::TaskType::NODE_PIR_TASK;
        job_task = taskRequest->pir_request().job_id() +
                   taskRequest->pir_request().task_id();
        if (running_set.find(job_task) != running_set.end()) {
            taskResponse->mutable_pir_response()->set_ret_code(1);
            return Status::OK;
        }
    }

    if (taskType == primihub::rpc::TaskType::NODE_PSI_TASK ||
        taskType == primihub::rpc::TaskType::NODE_PIR_TASK) {
        const auto& task_id = taskRequest->psi_request().task_id();
        const auto& job_id = taskRequest->psi_request().job_id();
        LOG_INFO(PLATFORM, job_id, task_id) << "Start to create PSI/PIR server task";
        running_set.insert(job_task);
        std::shared_ptr<Worker> worker = CreateWorker();
        running_map.insert({job_task, worker});
        worker->execute(taskRequest, taskResponse);
        running_map.erase(job_task);
        running_set.erase(job_task);

        return Status::OK;
    } else {
        return Status::OK;
    }
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
 * read certificate file from config file and create SslServerCredentials
 * input: configure file
 * output std::shared_ptr<grpc::ServerCredentials>
*/
std::shared_ptr<grpc::ServerCredentials>
createSslServerCredentials(const std::string& config_file_path) {
    std::string key;
    std::string cert;
    std::string root;
    YAML::Node config = YAML::LoadFile(config_file_path);
    std::string root_ca_file_path = config["certificate"]["root_ca"].as<std::string>();
    std::string cert_file_path = config["certificate"]["cert"].as<std::string>();
    std::string key_file_path = config["certificate"]["key"].as<std::string>();
    key = getFileContents(key_file_path);
    cert = getFileContents(cert_file_path);
    root = getFileContents(root_ca_file_path);
    grpc::SslServerCredentialsOptions::PemKeyCertPair keycert{key, cert};
    grpc::SslServerCredentialsOptions ssl_opts(GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY);
    ssl_opts.pem_root_certs = root;
    ssl_opts.pem_key_cert_pairs.push_back(std::move(keycert));
    std::shared_ptr<grpc::ServerCredentials> creds = grpc::SslServerCredentials(ssl_opts);
    return creds;
}
/**
 * @brief
 *  Start Apache arrow flight server with NodeService and DatasetService.
 */
void RunServer(primihub::VMNodeImpl *node_service,
        primihub::DataServiceImpl *dataset_service, int service_port, bool use_tls) {
    //
    grpc::ServerBuilder builder;
    std::string server_address{"0.0.0.0:" + std::to_string(service_port)};
    if (use_tls) {
        std::string config_file = absl::GetFlag(FLAGS_config);
        auto creds = createSslServerCredentials(config_file);
        builder.AddListeningPort(server_address, creds);
    } else {
        // Listen on the given address without any authentication mechanism.
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    }
    // registe service
    builder.RegisterService(node_service);
    builder.RegisterService(dataset_service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    VLOG(0) << "Server listening on " << server_address;
    //等待服务
    server->Wait();


    // // Initialize server
    // arrow::flight::Location location;

    // // Listen to all interfaces
    // ARROW_CHECK_OK(arrow::flight::Location::ForGrpcTcp("0.0.0.0", service_port, &location));
    // arrow::flight::FlightServerOptions options(location);
    // // service tls
    // options.verify_client = true;
    // auto& tls_certificates = options.tls_certificates;
    // auto& root_certificates = options.root_certificates;
    // std::string cert_file_path{"data/cert/server.crt"};
    // std::string key_file_path{"data/cert/server.key"};
    // std::string root_file_path{"data/cert/ca.crt"};
    // std::string key = getFileContents(key_file_path);
    // std::string cert = getFileContents(cert_file_path);
    // std::string root_ca = getFileContents(root_file_path);
    // root_certificates = std::move(root_ca);
    // arrow::flight::CertKeyPair cert_pair;
    // cert_pair.pem_cert = std::move(cert);
    // cert_pair.pem_key = std::move(key);
    // tls_certificates.push_back(std::move(cert_pair));

    // auto server = std::unique_ptr<arrow::flight::FlightServerBase>(
    //     new primihub::service::FlightIntegrationServer(
    //         node_service->getNodelet()->getDataService()));

    // // Use builder_hook to register grpc service
    // options.builder_hook = [&](void *raw_builder) {
    //     auto *builder = reinterpret_cast<grpc::ServerBuilder *>(raw_builder);
    //     builder->RegisterService(node_service);
    //     builder->RegisterService(dataset_service);

    //     // set the max message size to 128M
    //     builder->SetMaxReceiveMessageSize(128 * 1024 * 1024);
    // };

    // // Start the server
    // ARROW_CHECK_OK(server->Init(options));
    // // Exit with a clean error code (0) on SIGTERM
    // ARROW_CHECK_OK(server->SetShutdownOnSignals({SIGTERM}));

    // LOG(INFO) << " 💻 Node listening on port: " << service_port;

    // ARROW_CHECK_OK(server->Serve());
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

    YAML::Node config = YAML::LoadFile(config_file);
    bool use_tls = config["use_tls"].as<int32_t>();
    std::string node_ip = "0.0.0.0";
    node_service = new primihub::VMNodeImpl(node_id, node_ip, service_port,
                                            singleton, config_file, use_tls);
    data_service = new primihub::DataServiceImpl(
        node_service->getNodelet()->getDataService(),
        node_service->getNodelet()->getNodeletAddr());
    primihub::RunServer(node_service, data_service, service_port, use_tls);

    return EXIT_SUCCESS;
}
