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
#include "src/primihub/service/notify/model.h"
#include "src/primihub/util/util.h"


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
Status VMNodeImpl::Send(ServerContext* context,
        ServerReader<TaskRequest>* reader, TaskResponse* response) {
    VLOG(5) << "VMNodeImpl::Send: received: ";
    bool recv_meta_info{false};
    std::string job_id;
    std::string task_id;
    std::string role;
    std::vector<std::string> recv_data;
    std::shared_ptr<Worker> worker_ptr{nullptr};
    VLOG(5) << "VMNodeImpl::Send: received xxx: ";
    TaskRequest request;
    std::string received_data;
    // received_data.reserve(10*1024*1024);
    while (reader->Read(&request)) {
        if (!recv_meta_info) {
            job_id = request.job_id();
            task_id = request.task_id();
            role = request.role();
            if (role.empty()) {
                role = "default";
            }
            recv_meta_info = true;
            VLOG(5) << "job_id: " << job_id << " "
                    << "task_id: " << task_id << " "
                    << "role: " << role;
            std::string worker_id = this->getWorkerId(job_id, task_id);
            waitUntilWorkerReady(worker_id, context, this->wait_worker_ready_timeout_ms);
            worker_ptr = this->getWorker(job_id, task_id);
            if (worker_ptr == nullptr) {
                std::string err_msg = "worker for job id: ";
                err_msg.append(job_id).append(" task id: ").append(task_id)
                    .append(" not found");
                response->set_ret_code(rpc::retcode::FAIL);
                response->set_msg_info(std::move(err_msg));
                return Status::OK;
            }
        }
        received_data.append(request.data());
    }
    auto& recv_queue = worker_ptr->getTask()->getTaskContext().getRecvQueue(role);
    recv_queue.push(std::move(received_data));
    VLOG(5) << "end of VMNodeImpl::Send";
    response->set_ret_code(primihub::rpc::retcode::SUCCESS);
    return Status::OK;
}

Status VMNodeImpl::SendRecv(grpc::ServerContext* context,
        grpc::ServerReaderWriter<rpc::TaskResponse, rpc::TaskRequest>* stream) {
    VLOG(5) << "VMNodeImpl::SendRecv: received: ";
    bool is_initialized{false};
    std::string job_id;
    std::string task_id;
    std::string role;
    std::shared_ptr<Worker> worker_ptr{nullptr};
    TaskRequest request;
    rpc::TaskResponse response;
    std::string recv_data;
    int64_t timeout{-1};
    while (stream->Read(&request)) {
        if (!is_initialized) {
            job_id = request.job_id();
            task_id = request.task_id();
            role = request.role();
            if (role.empty()) {
                role = "default";
            }
            VLOG(5) << "job_id: " << job_id << " "
                    << "task_id: " << task_id << " "
                    << "role: " << role;
            std::string worker_id = this->getWorkerId(job_id, task_id);
            waitUntilWorkerReady(worker_id, context, this->wait_worker_ready_timeout_ms);
            worker_ptr = this->getWorker(job_id, task_id);
            if (worker_ptr == nullptr) {
                std::string err_msg = "worker for job id: ";
                err_msg.append(job_id).append(" task id: ").append(task_id)
                    .append(" not found");
                response.set_ret_code(rpc::retcode::FAIL);
                response.set_msg_info(std::move(err_msg));
                return Status::OK;
            }
            is_initialized = true;
        }
        recv_data.append(request.data());
    }
    VLOG(5) << "received data length: " << recv_data.size();
    auto& task_context = worker_ptr->getTask()->getTaskContext();
    auto& recv_queue = task_context.getRecvQueue(role);
    recv_queue.push(std::move(recv_data));
    VLOG(5) << "end of read data for server";
    // waiting for response data send to client
    auto& send_queue = task_context.getSendQueue(role);
    std::string send_data;
    send_queue.wait_and_pop(send_data);
    VLOG(5) << "send data length: " << send_data.length();
    std::vector<rpc::TaskResponse> response_data;
    buildTaskResponse(send_data, &response_data);
    for (const auto& res : response_data) {
        stream->Write(res);
    }
    auto& complete_queue = task_context.getCompleteQueue(role);
    // VLOG(5) << "get complete_queue success";
    complete_queue.push(retcode::SUCCESS);
    // VLOG(5) << "push success flag to complete queue success";
    return Status::OK;
}

Status VMNodeImpl::Recv(::grpc::ServerContext* context,
        const TaskRequest* request,
        grpc::ServerWriter< ::primihub::rpc::TaskResponse>* writer) {
    std::string job_id = request->job_id();
    std::string task_id = request->task_id();
    std::string role = request->role();
    std::string worker_id = this->getWorkerId(job_id, task_id);
    waitUntilWorkerReady(worker_id, context, this->wait_worker_ready_timeout_ms);
    auto worker_ptr = this->getWorker(job_id, task_id);
    if (worker_ptr == nullptr) {
        std::string err_msg  = "worker for job id: ";
        err_msg.append(job_id).append(" task id: ")
            .append(task_id).append(" not found");
        LOG(ERROR) << err_msg;
        TaskResponse response;
        response.set_ret_code(rpc::retcode::FAIL);
        response.set_msg_info(std::move(err_msg));
        writer->Write(response);
        return grpc::Status::OK;
    }
    auto& send_queue = worker_ptr->getTask()->getTaskContext().getSendQueue(role);
    std::string send_data;
    send_queue.wait_and_pop(send_data);
    auto& complete_queue = worker_ptr->getTask()->getTaskContext().getCompleteQueue(role);
    complete_queue.push(retcode::SUCCESS);
    std::vector<rpc::TaskResponse> response_data;
    buildTaskResponse(send_data, &response_data);
    for (const auto& res : response_data) {
        writer->Write(res);
    }
    return grpc::Status::OK;
}

void VMNodeImpl::buildTaskRequest(const std::string& job_id,
        const std::string& task_id, const std::string& role,
        const std::string& data, std::vector<rpc::TaskRequest>* requests) {
  size_t sended_size = 0;
  constexpr size_t max_package_size = 1 << 21;  // limit data size 4M
  size_t total_length = data.size();
  char* send_buf = const_cast<char*>(data.c_str());
  do {
    rpc::TaskRequest task_request;
    task_request.set_job_id(job_id);
    task_request.set_task_id(task_id);
    task_request.set_role(role);
    auto data_ptr = task_request.mutable_data();
    data_ptr->reserve(max_package_size);
    size_t data_len = std::min(max_package_size, total_length - sended_size);
    data_ptr->append(send_buf + sended_size, data_len);
    sended_size += data_len;
    requests->emplace_back(std::move(task_request));
    if (sended_size > total_length) {
      break;
    }
  } while(true);
}

void VMNodeImpl::buildTaskResponse(const std::string& data,
        std::vector<rpc::TaskResponse>* responses) {
  size_t sended_size = 0;
  constexpr size_t max_package_size = 1 << 21;  // limit data size 4M
  size_t total_length = data.size();
  char* send_buf = const_cast<char*>(data.c_str());
  do {
    rpc::TaskResponse task_response;
    task_response.set_ret_code(rpc::retcode::SUCCESS);
    auto data_ptr = task_response.mutable_data();
    data_ptr->reserve(max_package_size);
    size_t data_len = std::min(max_package_size, total_length - sended_size);
    data_ptr->append(send_buf + sended_size, data_len);
    sended_size += data_len;
    responses->emplace_back(std::move(task_response));
    if (sended_size < total_length) {
      continue;
    } else {
      break;
    }
  } while(true);
}

// for communication between different process
Status VMNodeImpl::ForwardSend(::grpc::ServerContext* context,
        ::grpc::ServerReader<::primihub::rpc::ForwardTaskRequest>* reader,
        ::primihub::rpc::TaskResponse* response) {
    // send data to destination node specified by request
    VLOG(5) << "enter VMNodeImpl::ForwardSend";
    primihub::rpc::ForwardTaskRequest forward_request;
    std::unique_ptr<VMNode::Stub> stub{nullptr};
    grpc::ClientContext client_context;
    primihub::rpc::TaskResponse task_response;
    std::unique_ptr<grpc::ClientWriter<primihub::rpc::TaskRequest>> writer{nullptr};
    bool is_initialized{false};
    bool write_success{false};
    std::string dest_address;
    while (reader->Read(&forward_request)) {
        if (!is_initialized) {
            const auto& dest_node = forward_request.dest_node();
            const auto& task_request = forward_request.task_request();
            dest_address = dest_node.ip() + ":" + std::to_string(dest_node.port());
            bool use_tls = false;
            stub = get_stub(dest_address, use_tls);
            writer = stub->Send(&client_context, &task_response);
            is_initialized = true;
            write_success = writer->Write(task_request);
            if (!write_success) {
                LOG(ERROR) << "forward data to " << dest_address << "failed";
                break;
            }
        } else {
            const auto& task_request = forward_request.task_request();
            write_success = writer->Write(task_request);
            if (!write_success) {
                LOG(ERROR) << "forward data to " << dest_address << "failed";
                break;
            }
        }
    }
    writer->WritesDone();
    Status status = writer->Finish();
    if (status.ok()) {
        auto ret_code = task_response.ret_code();
        if (ret_code) {
            LOG(ERROR) << "client Node send result data to server: " << dest_address
                    <<" return failed error code: " << ret_code;
        }
        response->set_ret_code(rpc::retcode::SUCCESS);
        response->set_msg_info(task_response.msg_info());
    } else {
        LOG(ERROR) << "client Node send result data to server "  << dest_address
                << " failed. error_code: "
                << status.error_code() << ": " << status.error_message();
        response->set_ret_code(rpc::retcode::FAIL);
        response->set_msg_info(status.error_message());
    }
    return Status::OK;
}

// for communication between different process
Status VMNodeImpl::ForwardRecv(::grpc::ServerContext* context,
        const ::primihub::rpc::TaskRequest* request,
        ::grpc::ServerWriter<::primihub::rpc::TaskRequest>* writer) {
    // waiting for peer node send data
    std::string job_id = request->job_id();
    std::string task_id = request->task_id();
    std::string role = request->role();
    VLOG(5) << "enter ForwardRecv: job_id: " << job_id << " "
        << "task id: " << task_id << " role: " << role;
    auto worker = this->getWorker(job_id, task_id);
    if (worker == nullptr) {
        return grpc::Status::OK;
    }
    // fetch data from task recv queue
    auto& recv_queue = worker->getTask()->getTaskContext().getRecvQueue(role);
    std::string forward_recv_data;
    recv_queue.wait_and_pop(forward_recv_data);
    std::vector<rpc::TaskRequest> forward_recv_datas;
    buildTaskRequest(job_id, task_id, role, forward_recv_data, &forward_recv_datas);
    for (const auto& data_ : forward_recv_datas) {
        writer->Write(data_);
    }
    return grpc::Status::OK;
}

std::unique_ptr<VMNode::Stub> VMNodeImpl::get_stub(const std::string& dest_address, bool use_tls) {
    grpc::ClientContext context;
    grpc::ChannelArguments channel_args;
    channel_args.SetMaxReceiveMessageSize(128*1024*1024);
    std::shared_ptr<grpc::Channel> channel =
        grpc::CreateCustomChannel(dest_address, grpc::InsecureChannelCredentials(), channel_args);
    std::unique_ptr<VMNode::Stub> stub = VMNode::NewStub(channel);
    return stub;
}

Status VMNodeImpl::SubmitTask(ServerContext *context,
                              const PushTaskRequest *pushTaskRequest,
                              PushTaskReply *pushTaskReply) {
    std::string str;
    google::protobuf::TextFormat::PrintToString(*pushTaskRequest, &str);
    LOG(INFO) << str << std::endl;

    std::string job_task = pushTaskRequest->task().job_id() + pushTaskRequest->task().task_id();
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

    } else if (pushTaskRequest->task().type() == primihub::rpc::TaskType::PIR_TASK ||
            pushTaskRequest->task().type() == primihub::rpc::TaskType::PSI_TASK) {
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
        VLOG(5) << "Construct protocol semantic parser finished";
        // Parse and dispathc pir task.
        if (pushTaskRequest->task().type() ==
            primihub::rpc::TaskType::PIR_TASK) {
            _psp.schedulePirTask(lan_parser_, this->nodelet->getNodeletAddr());
        } else {
            _psp.schedulePsiTask(lan_parser_);
        }
        auto _type = static_cast<int>(pushTaskRequest->task().type());
        VLOG(5) << "end schedule schedule task for type: " << _type;
    } else {
        auto executor_func = [this](
                std::shared_ptr<Worker> worker, PushTaskRequest request,
                primihub::ThreadSafeQueue<std::string>* finished_worker_queue) -> void {
            SCopedTimer timer;
            std::string submit_client_id = request.submit_client_id();
            std::string task_id = request.task().task_id();
            std::string job_id = request.task().job_id();
            LOG(INFO) << "begin to execute task";
            std::string status = "RUNNING";
            std::string status_info = "task is running";
            using EventBusNotifyDelegate = primihub::service::EventBusNotifyDelegate;
            auto& notify_proxy = EventBusNotifyDelegate::getInstance();
            notify_proxy.notifyStatus(job_id, task_id, submit_client_id, status, status_info);
            auto result_info = worker->execute(&request);

            if (result_info == retcode::SUCCESS) {
                status = "SUCCESS";
                status_info = "task finished";
            } else {
                status = "FAIL";
                status_info = "task execute encountes error";
            }
            notify_proxy.notifyStatus(job_id, task_id, submit_client_id, status, status_info);

            VLOG(5) << "execute task end, begin to clean task";
            finished_worker_queue->push(worker->worker_id());
            auto time_cost = timer.timeElapse();
            VLOG(5) << "execute task end, clean task finished, "
                    << "task total cost time(ms): " << time_cost;
        };
        std::string task_id = pushTaskRequest->task().task_id();
        std::string job_id = pushTaskRequest->task().job_id();
        LOG(INFO) << "start to create worker for task: "
                << "job_id : " << job_id  << " task_id: " << task_id;
        std::string worker_id = getWorkerId(job_id, task_id);
        // running_set.insert(job_task);
        std::shared_ptr<Worker> worker = CreateWorker(worker_id);
        auto fut = std::async(std::launch::async,
              executor_func,
              worker,
              *pushTaskRequest,
              &this->fininished_workers);
        LOG(INFO) << "create worker thread future for task: "
                << "job_id : " << job_id  << " task_id: " << task_id << " finished";
        // save work for future use
        {
            std::lock_guard<std::mutex> lck(task_executor_mtx);
            task_executor_map.insert(
                {worker_id, std::make_tuple(std::move(worker), std::move(fut))});
        }
        LOG(INFO) << "create worker thread for task: "
                << "job_id : " << job_id  << " task_id: " << task_id << " finished";
        // running_set.erase(job_task);
        auto& notify_server_info = this->nodelet->getNotifyServerConfig();
        auto notify_server = pushTaskReply->add_notify_server();
        notify_server->set_ip(notify_server_info.ip_);
        notify_server->set_port(notify_server_info.port_);
        notify_server->set_use_tls(notify_server_info.use_tls_);
        return Status::OK;
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

retcode VMNodeImpl::waitUntilWorkerReady(const std::string& worker_id,
        grpc::ServerContext* context, int timeout_ms) {
    if (context->IsCancelled()) {
        LOG(ERROR) << "context is cancelled by client";
        return retcode::FAIL;
    }
    auto _start = std::chrono::high_resolution_clock::now();
    do {
        // try to get lock
        {
          std::unique_lock<std::mutex> lck(task_executor_mtx, std::try_to_lock);
          if (lck.owns_lock()) {
            auto it = task_executor_map.find(worker_id);
            if (it != task_executor_map.end()) {
                break;
            }
          }
        }
        if (context->IsCancelled()) {
            LOG(ERROR) << "context is cancelled by client";
            return retcode::FAIL;
        }
        VLOG(5) << "sleep and wait for worker ready........";
        std::this_thread::sleep_for(std::chrono::seconds(1));
         if (timeout_ms == -1) {
            continue;
        }
        auto _now = std::chrono::high_resolution_clock::now();
        auto time_cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(_now - _start).count();
        if (time_cost_ms > timeout_ms) {
            LOG(ERROR) << "wait for worker ready timeout";
            return retcode::FAIL;
        }
    } while(true);
    return retcode::SUCCESS;
}

int VMNodeImpl::process_task_reseponse(bool is_psi_response,
        const ExecuteTaskResponse &response, std::vector<ExecuteTaskResponse> *splited_responses) {
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
    std::string task_id;
    std::string job_id;
    std::string submit_client_id;
    while (stream->Read(&recv_request)) {
        if (!first_visit_flag) {
            submit_client_id = recv_request.submit_client_id();
            task_request.set_submit_client_id(submit_client_id);
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
                job_id = psi_req.job_id();
                task_id = psi_req.task_id();
                job_task = job_id + task_id;
                auto psi_request = task_request.mutable_psi_request();
                psi_request->set_job_id(job_id);
                psi_request->set_task_id(task_id);
                psi_request->set_reveal_intersection(psi_req.reveal_intersection());
            } else if (req_type == ExecuteTaskRequest::AlgorithmRequestCase::kPirRequest) {
                is_pir_request = true;
                taskType = primihub::rpc::TaskType::NODE_PIR_TASK;
                const auto& pir_req = recv_request.pir_request();
                job_id = pir_req.job_id();
                task_id = pir_req.task_id();
                job_task = job_id + task_id;
                auto pir_request = task_request.mutable_pir_request();
                pir_request->set_galois_keys(pir_req.galois_keys());
                pir_request->set_relin_keys(pir_req.relin_keys());
                pir_request->set_job_id(job_id);
                pir_request->set_task_id(task_id);
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
        LOG(INFO) << "Start to create PSI/PIR server task";
        running_set.insert(job_task);
        std::string worker_id = job_task;
        std::shared_ptr<Worker> worker = CreateWorker(worker_id);
        // running_map.insert({job_task, worker});
        worker->execute(&task_request, &task_response);
        // running_map.erase(job_task);
        running_set.erase(job_task);
    }

    std::vector<ExecuteTaskResponse> splited_resp;
    process_task_reseponse(is_psi_request, task_response, &splited_resp);
    VLOG(5) << "ExecuteTaskResponse size: " << splited_resp.size();
    for (const auto& res : splited_resp) {
        stream->Write(res);
    }
    // the end of psi ecdh server
    LOG(INFO) << "begin to execute task";
    std::string status = "SUCCESS";
    std::string status_info = "task is finished";
    using EventBusNotifyDelegate = primihub::service::EventBusNotifyDelegate;
    EventBusNotifyDelegate::getInstance().notifyStatus(
            job_id, task_id, submit_client_id, status, status_info);
    return Status::OK;
}

std::shared_ptr<Worker> VMNodeImpl::CreateWorker(const std::string& worker_id) {
    LOG(INFO) << " 🤖️ Start create worker " << this->node_id << " worker id: " << worker_id;
    // absl::MutexLock lock(&worker_map_mutex_);
    auto worker = std::make_shared<Worker>(this->node_id, worker_id, this->nodelet);
    // workers_.emplace("simple_test_worker", worker);
    LOG(INFO) << " 🤖️ Fininsh create worker " << this->node_id << " worker id: " << worker_id;
    return worker;
}

std::shared_ptr<Worker> VMNodeImpl::CreateWorker() {
    std::string worker_id = "";
    LOG(INFO) << " 🤖️ Start create worker " << this->node_id;
    // absl::MutexLock lock(&worker_map_mutex_);
    auto worker = std::make_shared<Worker>(this->node_id, worker_id, this->nodelet);
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
