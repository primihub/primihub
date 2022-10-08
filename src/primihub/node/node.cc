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

Status VMNodeImpl::SubmitTask(ServerContext *context,
                              const PushTaskRequest *pushTaskRequest,
                              PushTaskReply *pushTaskReply) {

    std::string str;
    google::protobuf::TextFormat::PrintToString(*pushTaskRequest, &str);
    LOG(INFO) << str << std::endl;

    std::string job_task =
        pushTaskRequest->task().job_id() + pushTaskRequest->task().task_id();
    pushTaskReply->set_job_id(pushTaskRequest->task().job_id());

    // if (running_set.find(job_task) != running_set.end()) {
    //     pushTaskReply->set_ret_code(1);
    //     return Status::OK;
    // }

    // actor
    if (pushTaskRequest->task().type() == primihub::rpc::TaskType::ACTOR_TASK ||
        pushTaskRequest->task().type() == primihub::rpc::TaskType::TEE_TASK) {
        LOG(INFO) << "start to schedule task";
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

    } else if (pushTaskRequest->task().type() ==
                   primihub::rpc::TaskType::PIR_TASK ||
               pushTaskRequest->task().type() ==
                   primihub::rpc::TaskType::PSI_TASK) {
        LOG(INFO) << "start to schedule schedule task";
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

    } else {
        LOG(INFO) << "start to create worker for task";
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
        LOG(INFO) << "Start to create PSI/PIR server task";
        running_set.insert(job_task);
        std::shared_ptr<Worker> worker = CreateWorker();
        worker->execute(taskRequest, taskResponse);
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
