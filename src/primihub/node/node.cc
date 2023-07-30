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
#include "src/primihub/util/util.h"
#include "src/primihub/util/network/link_factory.h"

using grpc::Server;
using primihub::rpc::Params;
using primihub::rpc::ParamValue;
using primihub::rpc::PsiType;
using primihub::rpc::TaskType;
using primihub::task::LanguageParserFactory;
using primihub::task::ProtocolSemanticParser;

namespace primihub {

VMNodeImpl::VMNodeImpl(const std::string& node_id_,
                        const std::string& config_file_path_,
                        std::shared_ptr<service::DatasetService> service) :
    node_id(node_id_), config_file_path(config_file_path_),
    dataset_service_(std::move(service)) {
  Init();
}

retcode VMNodeImpl::Init() {
  running_set.clear();
  task_executor_map.clear();
  nodelet = std::make_shared<Nodelet>(config_file_path, dataset_service_);
  auto link_mode{primihub::network::LinkMode::GRPC};
  link_ctx_ = primihub::network::LinkFactory::createLinkContext(link_mode);
  CleanFinishedTaskThread();
  CleanTimeoutCachedTaskStatusThread();
  CleanFinishedSchedulerWorkerThread();
  return retcode::SUCCESS;
}

void VMNodeImpl::CleanFinishedTaskThread() {
  // clean finished task
  finished_worker_fut = std::async(
    std::launch::async,
    [&]() {
      SET_THREAD_NAME("cleanFinihsedTask");
      while(true) {
        std::string finished_worker_id;
        fininished_workers.wait_and_pop(finished_worker_id);
        if (stop_.load(std::memory_order::memory_order_relaxed)) {
          LOG(WARNING) << "cleanFinihsedTask begin to exit";
          break;
        }
        {
          std::unique_lock<std::mutex> lck(task_executor_mtx, std::try_to_lock);
          if (!lck.owns_lock()) {
            LOG(WARNING) << "try to lock task executor map failed, ignore....";
            continue;
          }
          auto it = task_executor_map.find(finished_worker_id);
          if (it != task_executor_map.end()) {
            VLOG(5) << "worker id : " << finished_worker_id << " "
                    << "has finished, begin to erase";
            task_executor_map.erase(finished_worker_id);
            VLOG(5) << "erase worker id : " << finished_worker_id << " success";
          }
        }
      }
    });
}

void VMNodeImpl::CleanTimeoutCachedTaskStatusThread() {
  // clean cached timeout stask status
  clean_cached_task_status_fut_ = std::async(
    std::launch::async,
    [&]() {
      SET_THREAD_NAME("cleanCachedTaskStatus");
      while(true) {
        if (stop_.load(std::memory_order::memory_order_relaxed)) {
          LOG(WARNING) << "service begin to exit";
          break;
        }
        std::set<std::string> timeout_worker_id;
        {
          time_t now_ = ::time(nullptr);
          std::shared_lock<std::shared_mutex> lck(this->finished_task_status_mtx_);
          for (const auto& task_status : finished_task_status_) {
            auto& timestamp = std::get<2>(task_status.second);
            if (now_ - timestamp > this->cached_task_status_timeout_) {
              timeout_worker_id.emplace(task_status.first);
            }
          }
        }
        if (!timeout_worker_id.empty()) {
          VLOG(2) << "number of timeout task status need to earse: " << timeout_worker_id.size();
          std::unique_lock<std::shared_mutex> lck(this->finished_task_status_mtx_);
          for (const auto& worker_id : timeout_worker_id) {
            finished_task_status_.erase(worker_id);
          }
        }
        std::this_thread::sleep_for(
            std::chrono::seconds(this->cached_task_status_timeout_/2));
      }
    }
  );
}
void VMNodeImpl::CleanFinishedSchedulerWorkerThread() {
  // clean finished scheduler worker
  finished_scheduler_worker_fut = std::async(
    std::launch::async,
    [&]() {
      SET_THREAD_NAME("cleanSchedulerTask");
      // time_t now_ = ::time(nullptr);
      std::map<std::string, time_t> finished_scheduler;
      while(true) {
        std::set<std::string> timeouted_shceduler;
        do {
          std::string finished_scheduler_worker_id;
          fininished_scheduler_workers_.wait_and_pop(finished_scheduler_worker_id);
          if (stop_.load(std::memory_order::memory_order_relaxed)) {
            LOG(ERROR) << "cleanSchedulerTask begin to quit";
            return;
          }
          time_t now_ = ::time(nullptr);
          finished_scheduler[finished_scheduler_worker_id] = now_;
          for (auto it = finished_scheduler.begin(); it != finished_scheduler.end();) {
            if (now_ - it->second > this->scheduler_worker_timeout_s) {
              timeouted_shceduler.insert(it->first);
              it = finished_scheduler.erase(it);
            } else {
              ++it;
            }
          }
          if (timeouted_shceduler.size() > 100) {
            break;
          }
        } while (true);

        SCopedTimer timer;
        {
          VLOG(3) << "cleanSchedulerTask size of need to clean task: "
              << timeouted_shceduler.size();
          std::lock_guard<std::mutex> lck(this->task_executor_mtx);
          for (const auto& scheduler_worker_id : timeouted_shceduler) {
            auto it = task_scheduler_map_.find(scheduler_worker_id);
            if (it != task_scheduler_map_.end()) {
              VLOG(5) << "scheduler worker id : " << scheduler_worker_id << " "
                      << "has timeouted, begin to erase";
              task_scheduler_map_.erase(scheduler_worker_id);
              VLOG(5) << "erase scheduler worker id : "
                  << scheduler_worker_id << " success";
            }
          }
        }
        LOG(INFO) << "clean timeouted scheduler task cost(ms): " << timer.timeElapse();
      }
    });
}

VMNodeImpl::~VMNodeImpl() {
    stop_.store(true);
    fininished_workers.shutdown();
    fininished_scheduler_workers_.shutdown();
    finished_worker_fut.get();
    clean_cached_task_status_fut_.get();
    finished_scheduler_worker_fut.get();
    this->nodelet.reset();
}

Status VMNodeImpl::Send(ServerContext* context,
        ServerReader<TaskRequest>* reader, TaskResponse* response) {
    VLOG(5) << "VMNodeImpl::Send: begin to receive data...";
    bool recv_meta_info{false};
    std::string job_id;
    std::string task_id;
    std::string request_id;
    std::string role;
    std::vector<std::string> recv_data;
    std::shared_ptr<Worker> worker_ptr{nullptr};
    TaskRequest request;
    std::string received_data;
    // received_data.reserve(10*1024*1024);
    while (reader->Read(&request)) {
        if (!recv_meta_info) {
            job_id = request.task_info().job_id();
            task_id = request.task_info().task_id();
            request_id = request.task_info().request_id();
            role = request.role();
            if (role.empty()) {
                role = "default";
            }
            size_t data_len = request.data_len();
            received_data.reserve(data_len);
            recv_meta_info = true;
            VLOG(5) << "job_id: " << job_id << " "
                    << "task_id: " << task_id << " "
                    << "request_id: " << request_id << " "
                    << "role: " << role;
            std::string worker_id = this->getWorkerId(job_id, task_id, request_id);
            auto finished_task = this->isFinishedTask(worker_id);
            if (std::get<0>(finished_task)) {
                std::string err_msg = "worker for job id: ";
                err_msg.append(job_id).append(" task id: ").append(task_id)
                    .append(" has finished");
                response->set_ret_code(rpc::retcode::FAIL);
                response->set_msg_info(std::move(err_msg));
                return Status::OK;
            }
            waitUntilWorkerReady(worker_id, context, this->wait_worker_ready_timeout_ms);
            worker_ptr = this->getWorker(job_id, task_id, request_id);
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
    auto& link_ctx = worker_ptr->getTask()->getTaskContext().getLinkContext();
    if (link_ctx == nullptr) {
      std::string err_msg = "LinkContext is empty for job id: ";
      err_msg.append(job_id).append(" task id: ").append(task_id)
          .append(" request id: ").append(request_id);
      response->set_ret_code(rpc::retcode::FAIL);
      response->set_msg_info(std::move(err_msg));
      return Status::OK;
    }
    size_t data_size = received_data.size();
    auto& recv_queue = link_ctx->GetRecvQueue(role);
    recv_queue.push(std::move(received_data));
    VLOG(5) << "end of VMNodeImpl::Send, data total received size:" << data_size;
    response->set_ret_code(primihub::rpc::retcode::SUCCESS);
    return Status::OK;
}

Status VMNodeImpl::SendRecv(grpc::ServerContext* context,
        grpc::ServerReaderWriter<rpc::TaskResponse, rpc::TaskRequest>* stream) {
    VLOG(5) << "VMNodeImpl::SendRecv: received: ";
    bool is_initialized{false};
    std::string job_id;
    std::string task_id;
    std::string request_id;
    std::string role;
    std::shared_ptr<Worker> worker_ptr{nullptr};
    TaskRequest request;
    rpc::TaskResponse response;
    std::string recv_data;
    int64_t timeout{-1};
    while (stream->Read(&request)) {
        if (!is_initialized) {
            job_id = request.task_info().job_id();
            task_id = request.task_info().task_id();
            request_id = request.task_info().request_id();
            role = request.role();
            if (role.empty()) {
                role = "default";
            }
            size_t data_len = request.data_len();
            recv_data.reserve(data_len);
            VLOG(5) << "job_id: " << job_id << " "
                    << "task_id: " << task_id << " "
                    << "role: " << role;
            std::string worker_id = this->getWorkerId(job_id, task_id, request_id);
            auto finished_task = this->isFinishedTask(worker_id);
            if (std::get<0>(finished_task)) {
                std::string err_msg = "worker for job id: ";
                err_msg.append(job_id).append(" task id: ").append(task_id)
                    .append(" has finished");
                LOG(WARNING) << err_msg;
                response.set_ret_code(rpc::retcode::FAIL);
                response.set_msg_info(std::move(err_msg));
                return Status::OK;
            }
            waitUntilWorkerReady(worker_id, context, this->wait_worker_ready_timeout_ms);
            worker_ptr = this->getWorker(job_id, task_id, request_id);
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
    auto& link_ctx = worker_ptr->getTask()->getTaskContext().getLinkContext();
    if (link_ctx == nullptr) {
      std::string err_msg = "LinkContext is empty for job id: ";
      err_msg.append(job_id).append(" task id: ").append(task_id)
          .append(" request id: ").append(request_id);
      response.set_ret_code(rpc::retcode::FAIL);
      response.set_msg_info(std::move(err_msg));
      return Status::OK;
    }
    auto& recv_queue = link_ctx->GetRecvQueue(role);
    recv_queue.push(std::move(recv_data));
    VLOG(5) << "end of read data for server";
    // waiting for response data send to client
    auto& send_queue = link_ctx->GetSendQueue(role);
    std::string send_data;
    send_queue.wait_and_pop(send_data);
    VLOG(5) << "send data length: " << send_data.length();
    std::vector<rpc::TaskResponse> response_data;
    buildTaskResponse(send_data, &response_data);
    for (const auto& res : response_data) {
        stream->Write(res);
    }
    auto& complete_queue = link_ctx->GetCompleteQueue(role);
    // VLOG(5) << "get complete_queue success";
    complete_queue.push(retcode::SUCCESS);
    // VLOG(5) << "push success flag to complete queue success";
    return Status::OK;
}

Status VMNodeImpl::Recv(::grpc::ServerContext* context,
        const TaskRequest* request,
        grpc::ServerWriter< ::primihub::rpc::TaskResponse>* writer) {
    std::string job_id = request->task_info().job_id();
    std::string task_id = request->task_info().task_id();
    std::string request_id = request->task_info().request_id();
    std::string role = request->role();
    std::string worker_id = this->getWorkerId(job_id, task_id, request_id);
    auto finished_task = this->isFinishedTask(worker_id);
    if (std::get<0>(finished_task)) {
        TaskResponse response;
        std::string err_msg = "worker for job id: ";
        err_msg.append(job_id).append(" task id: ").append(task_id)
            .append(" has finished");
        response.set_ret_code(rpc::retcode::FAIL);
        response.set_msg_info(std::move(err_msg));
        writer->Write(response);
        return grpc::Status::OK;
    }
    waitUntilWorkerReady(worker_id, context, this->wait_worker_ready_timeout_ms);
    auto worker_ptr = this->getWorker(job_id, task_id, request_id);
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
    auto& link_ctx = worker_ptr->getTask()->getTaskContext().getLinkContext();
    if (link_ctx == nullptr) {
        std::string err_msg  = "LinkContext is empty for job id: ";
        err_msg.append(job_id).append(" task id: ").append(task_id);
        LOG(ERROR) << err_msg;
        TaskResponse response;
        response.set_ret_code(rpc::retcode::FAIL);
        response.set_msg_info(std::move(err_msg));
        writer->Write(response);
        return grpc::Status::OK;
    }
    auto& send_queue = link_ctx->GetSendQueue(role);
    std::string send_data;
    send_queue.wait_and_pop(send_data);
    auto& complete_queue = link_ctx->GetCompleteQueue(role);
    complete_queue.push(retcode::SUCCESS);
    std::vector<rpc::TaskResponse> response_data;
    buildTaskResponse(send_data, &response_data);
    for (const auto& res : response_data) {
        writer->Write(res);
    }
    return grpc::Status::OK;
}

void VMNodeImpl::buildTaskRequest(const std::string& job_id,
        const std::string& task_id, const std::string& request_id, const std::string& role,
        const std::string& data, std::vector<rpc::TaskRequest>* requests) {
  size_t sended_size = 0;
  constexpr size_t max_package_size = 1 << 21;  // limit data size 4M
  size_t total_length = data.size();
  char* send_buf = const_cast<char*>(data.c_str());
  do {
    rpc::TaskRequest task_request;
    auto task_info = task_request.mutable_task_info();
    task_info->set_job_id(job_id);
    task_info->set_task_id(task_id);
    task_info->set_request_id(request_id);
    task_request.set_role(role);
    task_request.set_data_len(total_length);
    auto data_ptr = task_request.mutable_data();
    data_ptr->reserve(max_package_size);
    size_t data_len = std::min(max_package_size, total_length - sended_size);
    data_ptr->append(send_buf + sended_size, data_len);
    sended_size += data_len;
    requests->emplace_back(std::move(task_request));
    if (sended_size < total_length) {
      continue;
    } else {
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
    task_response.set_data_len(total_length);
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
    std::string job_id = request->task_info().job_id();
    std::string task_id = request->task_info().task_id();
    std::string request_id = request->task_info().request_id();
    std::string role = request->role();
    VLOG(5) << "enter ForwardRecv: job_id: " << job_id << " "
        << "task id: " << task_id << " role: " << role;
    std::string worker_id = this->getWorkerId(job_id, task_id, request_id);
    auto finished_task = this->isFinishedTask(worker_id);
    if (std::get<0>(finished_task)) {
        std::string err_msg = "worker for job id: ";
        err_msg.append(job_id).append(" task id: ").append(task_id)
            .append(" request id: ").append(request_id)
            .append(" has finished");
        LOG(WARNING) << err_msg;
        return Status::OK;
    }
    waitUntilWorkerReady(worker_id, context, this->wait_worker_ready_timeout_ms);
    auto worker = this->getWorker(job_id, task_id, request_id);
    if (worker == nullptr) {
        return grpc::Status::OK;
    }
    // fetch data from task recv queue
    auto& link_ctx = worker->getTask()->getTaskContext().getLinkContext();
    if (link_ctx == nullptr) {
      std::string err_msg = "LinkContext is empty for job id: ";
      err_msg.append(job_id).append(" task id: ").append(task_id)
          .append(" request id: ").append(request_id);
      LOG(WARNING) << err_msg;
      return Status::OK;
    }
    auto& recv_queue = link_ctx->GetRecvQueue(role);
    std::string forward_recv_data;
    recv_queue.wait_and_pop(forward_recv_data);
    std::vector<rpc::TaskRequest> forward_recv_datas;
    buildTaskRequest(job_id, task_id, request_id, role, forward_recv_data, &forward_recv_datas);
    for (const auto& data_ : forward_recv_datas) {
        writer->Write(data_);
    }
    return grpc::Status::OK;
}

Status VMNodeImpl::KillTask(::grpc::ServerContext* context,
                            const ::primihub::rpc::KillTaskRequest* request,
                            ::primihub::rpc::KillTaskResponse* response) {
    std::string job_id = request->task_info().job_id();
    std::string task_id = request->task_info().task_id();
    std::string request_id = request->task_info().request_id();
    auto executor = request->executor();
    VLOG(0) << "receive request for kill task for: "
        << "job_id: " << job_id << " "
        << "task id: " << task_id << " "
        << "request id: " << request_id << " "
        << "from " << (executor == rpc::KillTaskRequest::CLIENT ? "CLIENT" : "SCHEDULER");
    std::string worker_id = this->getWorkerId(job_id, task_id, request_id);
    auto finished_task = this->isFinishedTask(worker_id);
    if (std::get<0>(finished_task)) {
        std::string err_msg = "worker for job id: ";
        err_msg.append(job_id).append(" task id: ").append(task_id)
            .append(" request id: ").append(request_id)
            .append(" has finished");
        LOG(WARNING) << err_msg;
        response->set_ret_code(rpc::SUCCESS);
        response->set_msg_info(std::move(err_msg));
        return Status::OK;
    }
    std::shared_ptr<Worker> worker{nullptr};
    if (executor == rpc::KillTaskRequest::CLIENT) {
        worker = this->getSchedulerWorker(job_id, task_id, request_id);
        if (worker != nullptr) {
          rpc::TaskStatus task_status;
          auto task_info = task_status.mutable_task_info();
          task_info->CopyFrom(request->task_info());
          task_status.set_party("SCHEDULER");
          task_status.set_message("kill task by client actively");
          task_status.set_status(rpc::TaskStatus::FAIL);
          worker->updateTaskStatus(task_status);
        }
    } else {
        worker = this->getWorker(job_id, task_id, request_id);
        if (worker != nullptr) {
            worker->kill_task();
        }
    }
    if (worker == nullptr) {
        LOG(WARNING) << "worker does not find for worker id: " << worker_id;
    }
    response->set_ret_code(rpc::SUCCESS);
    LOG(ERROR) << "end of VMNodeImpl::KillTask";
    return grpc::Status::OK;
}

Status VMNodeImpl::FetchTaskStatus(::grpc::ServerContext* context,
                                  const ::primihub::rpc::TaskContext* request,
                                  ::primihub::rpc::TaskStatusReply* response) {
    const auto& request_id = request->request_id();
    const auto& task_id = request->task_id();
    const auto& job_id = request->job_id();
    auto worker_ptr = getSchedulerWorker(task_id, job_id, request_id);
    if (worker_ptr == nullptr) {
        auto task_status = response->add_task_status();
        task_status->set_status(rpc::TaskStatus::NONEXIST);
        task_status->set_message("No shecudler found for task");
        return grpc::Status::OK;
    }
    // fetch all exist task status which has been collected from all party
    // and return to client in one request
    do {
        auto res = response->add_task_status();
        auto ret = worker_ptr->fetchTaskStatus(res);
        if (ret != retcode::SUCCESS) {
            // LOG(WARNING) << "no task status get from scheduler";
            break;
        }
    } while (true);
    return grpc::Status::OK;
}

Status VMNodeImpl::UpdateTaskStatus(::grpc::ServerContext* context,
                                    const ::primihub::rpc::TaskStatus* request,
                                    ::primihub::rpc::Empty* response) {
    updateTaskStatusInternal(*request);
    return grpc::Status::OK;
}

retcode VMNodeImpl::updateTaskStatusInternal(const rpc::TaskStatus& task_status) {
    std::string request_id = task_status.task_info().request_id();
    std::string task_id = task_status.task_info().task_id();
    std::string job_id = task_status.task_info().job_id();
    auto worker_ptr = getSchedulerWorker(task_id, job_id, request_id);
    if (worker_ptr == nullptr) {
        LOG(ERROR) << "no scheduler worker found for request_id: " << request_id;
        return retcode::FAIL;
    }
    auto ret = worker_ptr->updateTaskStatus(task_status);
    if (ret != retcode::SUCCESS) {
        LOG(WARNING) << "no task status get from scheduler";
    }
    return retcode::SUCCESS;
}

retcode VMNodeImpl::notifyTaskStatus(const Node& scheduler_node,
        const std::string& task_id, const std::string& job_id,
        const std::string& request_id, const std::string& party_name,
        const rpc::TaskStatus::StatusCode& status, const std::string& message) {
    rpc::TaskStatus task_status;
    auto task_info = task_status.mutable_task_info();
    task_info->set_task_id(task_id);
    task_info->set_job_id(job_id);
    task_info->set_request_id(request_id);
    task_status.set_party(party_name);
    task_status.set_status(status);
    task_status.set_message(message);
    rpc::Empty no_reply;
    auto channel = link_ctx_->getChannel(scheduler_node);
    return channel->updateTaskStatus(task_status, &no_reply);
}

retcode VMNodeImpl::notifyTaskStatus(const PushTaskRequest& request,
        const rpc::TaskStatus::StatusCode& status, const std::string& message) {
    Node scheduler_node;
    getSchedulerNodeCfg(request, &scheduler_node);
    rpc::TaskStatus task_status;
    auto& task_config = request.task();
    auto task_info = task_status.mutable_task_info();
    task_info->set_task_id(task_config.task_info().task_id());
    task_info->set_job_id(task_config.task_info().job_id());
    task_info->set_request_id(task_config.task_info().request_id());
    task_status.set_party(task_config.party_name());
    task_status.set_status(status);
    task_status.set_message(message);
    rpc::Empty no_reply;
    auto channel = link_ctx_->getChannel(scheduler_node);
    return channel->updateTaskStatus(task_status, &no_reply);
}

retcode VMNodeImpl::getSchedulerNodeCfg(const PushTaskRequest& request, Node* scheduler_node) {
    const auto& party_access_info = request.task().party_access_info();
    auto it = party_access_info.find(SCHEDULER_NODE);
    if (it != party_access_info.end()) {
        auto& pb_schedule_node = it->second;
        pbNode2Node(pb_schedule_node, scheduler_node);
    } else {
        LOG(ERROR) << "no config found for: " << SCHEDULER_NODE;
        return retcode::FAIL;
    }
    return retcode::SUCCESS;
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
    const PushTaskRequest *pushTaskRequest, PushTaskReply *pushTaskReply) {
  if (VLOG_IS_ON(5)) {
    std::string str;
    google::protobuf::TextFormat::PrintToString(*pushTaskRequest, &str);
    LOG(INFO) << str;
  }
  DispatchTask(*pushTaskRequest, pushTaskReply);

  auto task_info = pushTaskReply->mutable_task_info();
  task_info->CopyFrom(pushTaskRequest->task().task_info());

  return Status::OK;
}

retcode VMNodeImpl::DispatchTask(const PushTaskRequest& task_request, PushTaskReply* reply) {
  auto scheduler_func = [this](std::shared_ptr<Worker> worker,
      std::vector<Node> parties, const rpc::TaskContext task_info,
      primihub::ThreadSafeQueue<std::string>* finished_scheduler_workers) -> void {
    SET_THREAD_NAME("task_executor");
    // get the finall task status, if failed, kill current task
    auto ret = worker->waitUntilTaskFinish();
    if (ret != retcode::SUCCESS) {
      LOG(ERROR) << "task execute encountes error, begin to kill task to release resource";
      rpc::KillTaskRequest kill_request;
      auto task_info_ = kill_request.mutable_task_info();
      task_info_->CopyFrom(task_info);
      kill_request.set_executor(rpc::KillTaskRequest::SCHEDULER);
      for (const auto& patry : parties) {
        LOG(ERROR) << "party info: " << patry.to_string();
        rpc::KillTaskResponse reply;
        auto channel = link_ctx_->getChannel(patry);
        channel->killTask(kill_request, &reply);
      }
    }
    finished_scheduler_workers->push(task_info.request_id());
  };
  const auto& task_config = task_request.task();
  const auto& task_info = task_config.task_info();
  std::string task_id = task_info.task_id();
  std::string job_id = task_info.job_id();
  std::string request_id = task_info.request_id();
  std::string job_task = job_id + task_id;
  LOG(INFO) << "start to schedule task, task_type: " << static_cast<int>(task_config.type());
  std::string worker_id = getWorkerId(job_id, task_id, request_id);
  std::shared_ptr<Worker> worker_ptr = CreateWorker(worker_id);
  {
    std::unique_lock<std::shared_mutex> lck(task_scheduler_mtx_);
    task_scheduler_map_.insert(
        {worker_id, std::make_tuple(worker_ptr, std::future<void>())});
    VLOG(2) << "insert worker id: " << worker_id << " into task_scheduler_map_";
  }
  // absl::MutexLock lock(&parser_mutex_);
  // Construct language parser
  auto lan_parser_ = LanguageParserFactory::Create(task_request);
  if (lan_parser_ == nullptr) {
    std::string error_msg = "create LanguageParser failed";
    LOG(ERROR) << error_msg;
    reply->set_ret_code(1);
    reply->set_msg_info(error_msg);
    return retcode::FAIL;
  }
  lan_parser_->parseTask();
  lan_parser_->parseDatasets();
  // Construct protocol semantic parser
  auto _psp = ProtocolSemanticParser(
      this->node_id, this->singleton, this->nodelet->getDataService());
  // Parse and dispatch task.
  auto ret = _psp.parseTaskSyntaxTree(lan_parser_);
  if (ret != retcode::SUCCESS) {
    std::string error_msg = "dispatch task to party failed";
    LOG(ERROR) << error_msg;
    reply->set_ret_code(1);
    reply->set_msg_info(error_msg);
    return retcode::FAIL;
  }
  std::vector<Node> task_server = _psp.taskServer();
  worker_ptr->setPartyCount(task_server.size());
  // save work for future use
  {
    rpc::TaskContext task_info;
    task_info.set_task_id(task_id);
    task_info.set_job_id(job_id);
    task_info.set_request_id(request_id);
    auto fut = std::async(std::launch::async,
                          scheduler_func,
                          worker_ptr,
                          task_server,
                          task_info,
                          &this->fininished_scheduler_workers_);
    std::shared_lock<std::shared_mutex> lck(task_scheduler_mtx_);
    auto& worker_info = task_scheduler_map_[worker_id];
    auto& fut_info = std::get<1>(worker_info);
    fut_info = std::move(fut);
    VLOG(7) << "update worker id: " << worker_id;
  }
  auto& server_cfg = ServerConfig::getInstance();
  auto& service_node_info = server_cfg.getServiceConfig();
  auto pb_task_server = reply->add_task_server();
  node2PbNode(service_node_info, pb_task_server);
  reply->set_party_count(task_server.size());
  return retcode::SUCCESS;
}

retcode VMNodeImpl::ExecuteTask(const PushTaskRequest& task_request, PushTaskReply* reply) {
  auto executor_func = [this](
      std::shared_ptr<Worker> worker, PushTaskRequest request,
      primihub::ThreadSafeQueue<std::string>* finished_worker_queue) -> void {
    SET_THREAD_NAME("task_executor");
    SCopedTimer timer;
    std::string submit_client_id = request.submit_client_id();
    auto& task_info = request.task().task_info();
    std::string task_id = task_info.task_id();
    std::string job_id = task_info.job_id();
    std::string request_id = task_info.request_id();
    LOG(INFO) << "begin to execute task";
    rpc::TaskStatus::StatusCode status = rpc::TaskStatus::RUNNING;
    std::string status_info = "task is running";
    notifyTaskStatus(request, status, status_info);

    auto result_info = worker->execute(&request);

    if (result_info == retcode::SUCCESS) {
        status = status = rpc::TaskStatus::SUCCESS;
        status_info = "task finished";
    } else {
        status = rpc::TaskStatus::FAIL;
        status_info = "task execute encountes error";
    }
    notifyTaskStatus(request, status, status_info);
    VLOG(5) << "execute task end, begin to clean task";
    this->cacheLastTaskStatus(worker->worker_id(), submit_client_id, status);
    finished_worker_queue->push(worker->worker_id());
    auto time_cost = timer.timeElapse();
    VLOG(5) << "execute task end, clean task finished, "
            << "task total cost time(ms): " << time_cost;
  };
  const auto& task_config = task_request.task();
  const auto& task_info = task_config.task_info();
  std::string task_id = task_info.task_id();
  std::string job_id = task_info.job_id();
  std::string request_id = task_info.request_id();
  LOG(INFO) << "start to create worker for task: "
          << "job_id : " << job_id  << " task_id: " << task_id
          << " request id: " << request_id;
  std::string worker_id = getWorkerId(job_id, task_id, request_id);
  std::shared_ptr<Worker> worker = CreateWorker(worker_id);
  auto fut = std::async(std::launch::async,
                        executor_func,
                        worker,
                        task_request,
                        &this->fininished_workers);
  LOG(INFO) << "create execute worker thread future for task: "
          << "job_id : " << job_id  << " task_id: " << task_id
          << " request id: " << request_id << " finished";
  auto ret = worker->waitForTaskReady();
  if (ret == retcode::FAIL) {
    rpc::TaskStatus::StatusCode status = rpc::TaskStatus::FAIL;
    std::string status_info = "Initialize task failed.";
    std::string submit_client_id = task_request.submit_client_id();
    notifyTaskStatus(task_request, status, status_info);
    return retcode::FAIL;
  }
  // save work for future use
  {
    std::lock_guard<std::mutex> lck(task_executor_mtx);
    task_executor_map.insert(
        {worker_id, std::make_tuple(std::move(worker), std::move(fut))});
  }
  LOG(INFO) << "create worker thread for task: "
          << "job_id : " << job_id  << " task_id: " << task_id << " "
          << "request id: " << request_id << " finished";
  // service node info
  auto& server_cfg = ServerConfig::getInstance();
  auto& service_node_info = server_cfg.getServiceConfig();
  auto task_server = reply->add_task_server();
  node2PbNode(service_node_info, task_server);
  return retcode::SUCCESS;
}

std::shared_ptr<Worker> VMNodeImpl::getWorker(const std::string& job_id,
                                      const std::string& task_id,
                                      const std::string& request_id) {
    // std::string worker_id = getWorkerId(job_id, task_id);
    std::string worker_id = request_id;
    std::lock_guard<std::mutex> lck(task_executor_mtx);
    auto it = task_executor_map.find(worker_id);
    if (it != task_executor_map.end()) {
      return std::get<0>(it->second);
    }
    LOG(ERROR) << "no worker found for worker id: " << worker_id;
    return nullptr;
}

std::shared_ptr<Worker> VMNodeImpl::getSchedulerWorker(const std::string& task_id,
                                              const std::string& job_id,
                                              const std::string& request_id) {
    std::string worker_id = request_id;
    std::shared_lock<std::shared_mutex> lck(task_scheduler_mtx_);
    auto it = task_scheduler_map_.find(worker_id);
    if (it != task_scheduler_map_.end()) {
        return std::get<0>(it->second);
    }
    LOG(WARNING) << "no scheduler worker found for worker id: " << worker_id;
    return nullptr;
}

void VMNodeImpl::cacheLastTaskStatus(const std::string& worker_id,
        const std::string& submit_client_id, const rpc::TaskStatus::StatusCode& status) {
    time_t now_ = std::time(nullptr);
    std::unique_lock<std::shared_mutex> lck(this->finished_task_status_mtx_);
    finished_task_status_[worker_id] = std::make_tuple(submit_client_id, status, now_);
}

std::tuple<bool, std::string, std::string>
VMNodeImpl::isFinishedTask(const std::string& worker_id) {
    std::shared_lock<std::shared_mutex> lck(finished_task_status_mtx_);
    auto it = finished_task_status_.find(worker_id);
    if (it != finished_task_status_.end()) {
        // LOG(ERROR) << "found worker id: " << worker_id;
        return std::make_tuple(true, std::get<0>(it->second), std::get<1>(it->second));
    } else {
        return std::make_tuple(false, "", "UNKNOWN");
    }
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
        VLOG(5) << "sleep and wait for worker ready, "
                << "worker id : " << worker_id << " ........";
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

Status VMNodeImpl::ExecuteTask(ServerContext* context,
    const rpc::PushTaskRequest* request, rpc::PushTaskReply* response) {
  VLOG(5) << "enter VMNodeImpl::ExecuteTask";
  auto ret = ExecuteTask(*request, response);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "ExecuteTask encountes error";
  }
  return Status::OK;
}

std::shared_ptr<Worker> VMNodeImpl::CreateWorker(const std::string& worker_id) {
    LOG(INFO) << " 🤖️ Start create worker " << this->node_id << " worker id: " << worker_id;
    // absl::MutexLock lock(&worker_map_mutex_);
    auto worker = std::make_shared<Worker>(this->node_id, worker_id, getNodelet());
    // workers_.emplace("simple_test_worker", worker);
    LOG(INFO) << " 🤖️ Fininsh create worker " << this->node_id << " worker id: " << worker_id;
    return worker;
}

std::shared_ptr<Worker> VMNodeImpl::CreateWorker() {
    std::string worker_id = "";
    LOG(INFO) << " 🤖️ Start create worker " << this->node_id;
    // absl::MutexLock lock(&worker_map_mutex_);
    auto worker = std::make_shared<Worker>(this->node_id, worker_id, getNodelet());
    // workers_.emplace("simple_test_worker", worker);
    LOG(INFO) << " 🤖️ Fininsh create worker " << this->node_id;
    return worker;
}

}  // namespace primihub
