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
#include <sstream>
#include <unistd.h>


#include "src/primihub/node/node.h"
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

    if (running_set.find(job_task) != running_set.end()) {
        pushTaskReply->set_ret_code(1);
        return Status::OK;
    }
    
    // actor
    if (pushTaskRequest->task().type() == primihub::rpc::TaskType::ACTOR_TASK) {
        LOG(INFO) << "start to schedule task";
        absl::MutexLock lock(&parser_mutex_);
        // Construct language parser
        std::shared_ptr<LanguageParser> lan_parser_ = LanguageParserFactory::Create(*pushTaskRequest);
        if (lan_parser_ == nullptr) {
            pushTaskReply->set_ret_code(1);
            return Status::OK;
        }
        lan_parser_->parseTask();
        lan_parser_->parseDatasets();
        
        // Construct protocol semantic parser
        auto _psp = ProtocolSemanticParser(this->node_id,
                                            this->singleton, 
                                            this->nodelet->getDataService());
        // Parse and dispatch task.
        _psp.parseTaskSyntaxTree(lan_parser_);

    } else if (pushTaskRequest->task().type() ==
               primihub::rpc::TaskType::PSI_TASK) {
        LOG(INFO) << "start to schedule psi task";
    } else {
        LOG(INFO) << "start to create worker for task";
        running_set.insert(job_task);
        std::shared_ptr<Worker> worker = CreateWorker();
        worker->execute(pushTaskRequest);
        running_set.erase(job_task);
    }
    return Status::OK;
}

/*****************************************
 *
 * method runs on the node as psi server
 *
 * **************************************/
Status VMNodeImpl::SubmitPsiTask(ServerContext *context,
                                 const ExecuteTaskRequest *taskRequest,
                                 ExecuteTaskResponse *taskResponse) {

    return Status::OK;
}

Status VMNodeImpl::SubmitPirTask(ServerContext *context,
                                 const ExecuteTaskRequest *taskRequest,
                                 ExecuteTaskResponse *taskResponse) {}

Status VMNodeImpl::ExecuteTask(ServerContext *context,
                               const ExecuteTaskRequest *taskRequest,
                               ExecuteTaskResponse *taskResponse) {
    if (taskRequest->algorithm_request_case() ==
        ExecuteTaskRequest::AlgorithmRequestCase::kPsiRequest) {
        return SubmitPsiTask(context, taskRequest, taskResponse);
    } else if (taskRequest->algorithm_request_case() ==
               ExecuteTaskRequest::AlgorithmRequestCase::kPirRequest) {
        return SubmitPirTask(context, taskRequest, taskResponse);
    }
}

std::shared_ptr<Worker> VMNodeImpl::CreateWorker() {
    auto worker = std::make_shared<Worker>(this->node_id, this->nodelet);
    LOG(INFO) << " 🤖️ Start create worker " << this->node_id;
    absl::MutexLock lock(&worker_map_mutex_);
    
    workers_.emplace("simple_test_worker", worker);
    LOG(INFO) << " 🤖️ Fininsh create worker " << this->node_id;
    return worker;
}

void RunServer(primihub::VMNodeImpl *service, int service_port) {
    ServerBuilder builder;
    builder.AddListeningPort(absl::StrCat("0.0.0.0:", service_port),
                             grpc::InsecureServerCredentials());
    builder.RegisterService(service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    LOG(INFO) << " 💻 Node listening on port: " << service_port;

    server->Wait();
}

} // namespace primihub

int main(int argc, char **argv) {

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

    // TODO(chenhongbo) peer_list need to remove. get Peer list from dataset
    // service.

    int service_port = absl::GetFlag(FLAGS_service_port);
    std::string config_file = absl::GetFlag(FLAGS_config);

    std::string node_ip = "0.0.0.0";
    primihub::VMNodeImpl service(node_id, node_ip, service_port, singleton,
                                 config_file);


    primihub::RunServer(&service, service_port);

    return 0;
}
