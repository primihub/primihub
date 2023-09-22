/*
 * Copyright (c) 2023 by PrimiHub
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

#include "src/primihub/node/node_impl.h"
#include <vector>

#include "src/primihub/common/common.h"
#include "src/primihub/data_store/factory.h"
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/task/language/factory.h"
#include "src/primihub/task/semantic/parser.h"
#include "src/primihub/util/file_util.h"
#include "src/primihub/util/util.h"
#include "src/primihub/util/network/link_factory.h"

namespace primihub {
VMNodeImpl::VMNodeImpl(const std::string& config_file,
                       std::shared_ptr<service::DatasetService> service) :
                       config_file_path_(config_file),
                       dataset_service_(std::move(service)) {
  Init();
}

VMNodeImpl::~VMNodeImpl() {
  stop_.store(true);
  fininished_workers_.shutdown();
  fininished_scheduler_workers_.shutdown();
  finished_worker_fut_.get();
  clean_cached_task_status_fut_.get();
  finished_scheduler_worker_fut_.get();
  manage_task_worker_fut_.get();
  this->nodelet_.reset();
}

retcode VMNodeImpl::Init() {
  auto& server_config = primihub::ServerConfig::getInstance();
  auto& node_cfg = server_config.getServiceConfig();
  this->node_id_ = node_cfg.id();
  task_executor_map_.clear();
  nodelet_ = std::make_shared<Nodelet>(config_file_path_, dataset_service_);
  auto link_mode{network::LinkMode::GRPC};
  link_ctx_ = network::LinkFactory::createLinkContext(link_mode);
  CleanFinishedTaskThread();
  CleanTimeoutCachedTaskStatusThread();
  CleanFinishedSchedulerWorkerThread();
  ManageTaskOperatorThread();
  ProcessKillTaskThread();
  return retcode::SUCCESS;
}
void VMNodeImpl::ProcessKillTaskThread() {
  kill_task_queue_fut_ = std::async(
    std::launch::async,
    [&]() {
      SET_THREAD_NAME("ProcessKilledTask");
      while (true) {
        task_executor_container_t task_queue;
        kill_task_queue_.wait_and_pop(task_queue);
        if (stop_.load(std::memory_order::memory_order_relaxed)) {
          LOG(WARNING) << "ProcessKilledTask exit";
          break;
        }
        SCopedTimer timer;
        while (!task_queue.empty()) {
          auto& task_info = task_queue.front();
          auto& task_worker_ptr = std::get<0>(task_info);
          if (task_worker_ptr != nullptr) {
            LOG(ERROR) << "kill task for workerId: "
                       << task_worker_ptr->workerId();
            task_worker_ptr->kill_task();
            // auto& fut = std:;get<1>(task_info);
          }
          task_queue.pop();
        }
        task_executor_container_t().swap(task_queue);
        auto time_cost = timer.timeElapse();
        LOG(ERROR) << "ManageTaskThread operator : kill task time cost: " << time_cost;
      }
    });
}
void VMNodeImpl::ManageTaskOperatorThread() {
  manage_task_worker_fut_ = std::async(
    std::launch::async,
    [&]() {
      SET_THREAD_NAME("manageTaskOperator");
      VLOG(0) << "start ManageTaskThread";
      while (true) {
        task_manage_t task_info;
        task_manage_queue_.wait_and_pop(task_info);
        if (stop_.load(std::memory_order::memory_order_relaxed)) {
          LOG(WARNING) << "service begin to exit";
          break;
        }
        auto type = std::get<2>(task_info);
        switch (type) {
          case OperateTaskType::kAdd: {
            ExecuteAddTaskOperation(std::move(task_info));
            break;
          }
          case OperateTaskType::kDel: {
            ExecuteDelTaskOperation(std::move(task_info));
            break;
          }
          case OperateTaskType::kKill: {
            ExecuteKillTaskOperation(std::move(task_info));
            break;
          }
          default:
            break;
        }
      }
    });
}

std::shared_ptr<Worker> VMNodeImpl::CreateWorker() {
  std::string worker_id = "";
  return CreateWorker(worker_id);
}

std::shared_ptr<Worker> VMNodeImpl::CreateWorker(const std::string& worker_id) {
  LOG(INFO) << "Start create worker "
            << this->node_id_ << " worker id: " << worker_id;
  // absl::MutexLock lock(&worker_map_mutex_);
  auto worker =
      std::make_shared<Worker>(this->node_id_, worker_id, GetNodelet());
  // workers_.emplace("simple_test_worker", worker);
  LOG(INFO) << "Fininsh create worker "
            << this->node_id_ << " worker id: " << worker_id;
  return worker;
}

retcode VMNodeImpl::DispatchTask(const rpc::PushTaskRequest& task_request,
                                 rpc::PushTaskReply* reply) {
  auto scheduler_func = [this](std::shared_ptr<Worker> worker,
      std::vector<Node> parties, const rpc::Task task_config,
      ThreadSafeQueue<std::string>* finished_scheduler_workers) -> void {
    SET_THREAD_NAME("DispatchTask");
    // get the finall task status, if failed, kill current task
    auto ret = worker->waitUntilTaskFinish();
    auto& task_info = task_config.task_info();
    if (ret != retcode::SUCCESS) {
      LOG(ERROR) << TaskInfoToString(task_info) << ", "
                 << "task execute encountes error, "
                 << "begin to kill task to release resource";
      std::vector<Node> all_party;
      GetAllParties(task_config, &all_party);
      rpc::KillTaskRequest kill_request;
      auto task_info_ = kill_request.mutable_task_info();
      task_info_->CopyFrom(task_info);
      kill_request.set_executor(rpc::KillTaskRequest::SCHEDULER);
      for (const auto& patry : all_party) {
        rpc::KillTaskResponse reply;
        auto channel = link_ctx_->getChannel(patry);
        channel->killTask(kill_request, &reply);
      }
    }
    finished_scheduler_workers->push(task_info.request_id());
  };
  const auto& task_config = task_request.task();
  const auto& task_info = task_config.task_info();
  LOG(INFO) << "start to schedule task, task_type: "
            << static_cast<int>(task_config.type());
  std::string worker_id = this->GetWorkerId(task_info);
  std::shared_ptr<Worker> worker_ptr = CreateWorker(worker_id);
  {
    std::unique_lock<std::shared_mutex> lck(task_scheduler_mtx_);
    task_scheduler_map_.insert(
        {worker_id, std::make_tuple(worker_ptr, std::future<void>())});
  }
   VLOG(2) << TaskInfoToString(task_info) << ", "
           << "insert worker id: " << worker_id
           << " into task_scheduler_map success";
  // absl::MutexLock lock(&parser_mutex_);
  // Construct language parser
  auto lan_parser_ = task::LanguageParserFactory::Create(task_request);
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
  auto _psp = task::ProtocolSemanticParser(
      this->node_id_, this->singleton_, GetNodelet()->getDataService());
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
    auto task_config = lan_parser_->getPushTaskRequest().task();
    auto fut = std::async(std::launch::async,
                          scheduler_func,
                          worker_ptr,
                          task_server,
                          task_config,
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

retcode VMNodeImpl::ExecuteTask(const rpc::PushTaskRequest& task_request,
                                rpc::PushTaskReply* reply) {
  auto executor_func = [this](
      std::shared_ptr<Worker> worker, PushTaskRequest request,
      ThreadSafeQueue<task_manage_t>* task_manage_queue) -> void {
    SET_THREAD_NAME("ExecuteTask");
    SCopedTimer timer;
    auto& task_info = request.task().task_info();
    // add proxy server info
    auto& server_cfg = ServerConfig::getInstance();
    auto& proxy_node = server_cfg.ProxyServerCfg();
    auto auxiliary_server = request.mutable_task()->mutable_auxiliary_server();
    {
      rpc::Node pb_proxy_node;
      node2PbNode(proxy_node, &pb_proxy_node);
      (*auxiliary_server)[PROXY_NODE] = std::move(pb_proxy_node);
    }

    LOG(INFO) << TaskInfoToString(task_info) << ", begin to execute task";
    rpc::TaskStatus::StatusCode status = rpc::TaskStatus::RUNNING;
    std::string status_info = "task is running";
    this->NotifyTaskStatus(request, status, status_info);
    auto result_info = worker->execute(&request);
    if (result_info == retcode::SUCCESS) {
      status = rpc::TaskStatus::SUCCESS;
      status_info = "task finished";
    } else {
      status = rpc::TaskStatus::FAIL;
      status_info = "task execute encountes error";
    }
    this->NotifyTaskStatus(request, status, status_info);
    if (request.manual_launch()) {
      VLOG(0) << TaskInfoToString(task_info) << ", execute sub task finished";
      return;
    }
    VLOG(5) << TaskInfoToString(task_info)
            << ", execute task end, begin to clean task";
    this->CacheLastTaskStatus(worker->worker_id(), status);
    // finished_worker_queue->push(worker->worker_id());
    task_manage_queue->push(
        std::make_tuple(worker->worker_id(),
            std::make_tuple(nullptr, std::future<void>()),
            OperateTaskType::kDel));
    auto time_cost = timer.timeElapse();
    VLOG(5) << "execute task end, clean task finished, "
            << "task total cost time(ms): " << time_cost;
  };
  const auto& task_config = task_request.task();
  const auto& task_info = task_config.task_info();
  LOG(INFO) << TaskInfoToString(task_info)
            << ", start to create worker for task: ";
  CleanDuplicateTaskIdFilter();
  if (IsDuplicateTask(task_info)) {
    LOG(ERROR) << TaskInfoToString(task_info)
               << ", task has alread received, ignore ....";
    return retcode::FAIL;
  }
  std::string worker_id = GetWorkerId(task_info);
  std::shared_ptr<Worker> worker = CreateWorker(worker_id);
  auto fut = std::async(std::launch::async,
                        executor_func,
                        worker,
                        task_request,
                        &this->task_manage_queue_);
  LOG(INFO) << TaskInfoToString(task_info)
            << ", create execute worker thread future finished";
  auto ret = worker->waitForTaskReady();
  if (ret == retcode::FAIL) {
    rpc::TaskStatus::StatusCode status = rpc::TaskStatus::FAIL;
    std::string status_info = "Initialize task failed.";
    this->NotifyTaskStatus(task_request, status, status_info);
    return retcode::FAIL;
  }
  // save work for future use
  VLOG(7) << "end of worker->waitForTaskReady(), begin to save handler";
  auto task_worker = std::make_tuple(std::move(worker), std::move(fut));
  auto task_worker_info
    = std::make_tuple(worker_id, std::move(task_worker), OperateTaskType::kAdd);
  task_manage_queue_.push(std::move(task_worker_info));
  LOG(INFO) << TaskInfoToString(task_info)
            << "create worker thread finished";
  // service node info
  auto& server_cfg = ServerConfig::getInstance();
  auto& service_node_info = server_cfg.getServiceConfig();
  auto task_server = reply->add_task_server();
  node2PbNode(service_node_info, task_server);
  return retcode::SUCCESS;
}

retcode VMNodeImpl::StopTask(const rpc::TaskContext& task_info) {
  std::string worker_id = GetWorkerId(task_info);
  this->task_manage_queue_.push(
    std::make_tuple(worker_id,
                    std::make_tuple(nullptr, std::future<void>()),
                    OperateTaskType::kDel));
  return retcode::SUCCESS;
}

retcode VMNodeImpl::KillTask(const rpc::KillTaskRequest& request,
                             rpc::KillTaskResponse* response) {
  const auto& task_info = request.task_info();
  auto executor = request.executor();
  VLOG(0) << TaskInfoToString(task_info)
          << ", receive request for kill task from "
          << (executor == rpc::KillTaskRequest::CLIENT ?
                          ROLE_CLIENT :
                          ROLE_SCHEDULER);

  std::string worker_id = this->GetWorkerId(task_info);
  auto finished_task = this->IsFinishedTask(worker_id);
  if (std::get<0>(finished_task)) {
    std::string err_msg = "worker for ";
    err_msg.append(TaskInfoToString(task_info)).append(" has finished");
    LOG(WARNING) << err_msg;
    response->set_ret_code(rpc::SUCCESS);
    response->set_msg_info(std::move(err_msg));
    return retcode::SUCCESS;
  }
  std::shared_ptr<Worker> worker{nullptr};
  if (executor == rpc::KillTaskRequest::CLIENT) {
    worker = this->GetSchedulerWorker(task_info);
    if (worker != nullptr) {
      rpc::TaskStatus task_status;
      auto task_info_ptr = task_status.mutable_task_info();
      task_info_ptr->CopyFrom(task_info);
      task_status.set_party(ROLE_SCHEDULER);
      task_status.set_message("kill task by client actively");
      task_status.set_status(rpc::TaskStatus::FAIL);
      worker->updateTaskStatus(task_status);
    }
  } else {
    std::string worker_id = this->GetWorkerId(task_info);
    task_manage_queue_.push(
        std::make_tuple(worker_id,
            std::make_tuple(nullptr, std::future<void>()),
            OperateTaskType::kKill));
  }
  if (worker == nullptr) {
    LOG(WARNING) << TaskInfoToString(task_info)
                 << ", worker does not find for worker id: " << worker_id;
  }
  response->set_ret_code(rpc::SUCCESS);
  VLOG(0) << "end of VMNodeImpl::KillTask";
  return retcode::SUCCESS;
}

retcode VMNodeImpl::GetSchedulerNode(const PushTaskRequest& request,
                                        Node* scheduler_node) {
  const auto& auxiliary_server = request.task().auxiliary_server();
  auto it = auxiliary_server.find(SCHEDULER_NODE);
  if (it != auxiliary_server.end()) {
    auto& pb_schedule_node = it->second;
    pbNode2Node(pb_schedule_node, scheduler_node);
  } else {
    LOG(ERROR) << "no config found for: " << SCHEDULER_NODE;
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode VMNodeImpl::FetchTaskStatus(const rpc::TaskContext& task_info,
                                    rpc::TaskStatusReply* response) {
  const auto& request_id = task_info.request_id();
  const auto& task_id = task_info.task_id();
  const auto& job_id = task_info.job_id();
  auto worker_ptr = GetSchedulerWorker(task_info);
  if (worker_ptr == nullptr) {
    auto task_status = response->add_task_status();
    task_status->set_status(rpc::TaskStatus::NONEXIST);
    task_status->set_message("No shecudler found for task");
    return retcode::SUCCESS;
  }
  // fetch all exist task status which has been collected from all party
  // and put into current request
  do {
    auto status = response->add_task_status();
    auto ret = worker_ptr->fetchTaskStatus(status);
    if (ret != retcode::SUCCESS) {
        // LOG(WARNING) << "no task status get from scheduler";
        break;
    }
  } while (true);
  return retcode::SUCCESS;
}

retcode VMNodeImpl::NotifyTaskStatus(const PushTaskRequest& request,
                                     const rpc::TaskStatus::StatusCode status,
                                     const std::string& message) {
  Node scheduler_node;
  auto ret = GetSchedulerNode(request, &scheduler_node);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "get scheduler node cfg failed";
    return retcode::FAIL;
  }
  rpc::TaskStatus task_status;
  auto& task_config = request.task();
  auto task_info_ptr = task_status.mutable_task_info();
  task_info_ptr->CopyFrom(task_config.task_info());
  task_status.set_party(task_config.party_name());
  task_status.set_status(status);
  task_status.set_message(message);
  rpc::Empty no_reply;
  auto channel = link_ctx_->getChannel(scheduler_node);
  return channel->updateTaskStatus(task_status, &no_reply);
}

retcode VMNodeImpl::NotifyTaskStatus(const Node& scheduler_node,
                                     const rpc::TaskContext& task_info,
                                     const std::string& party_name,
                                     const rpc::TaskStatus::StatusCode status,
                                     const std::string& message) {
  rpc::TaskStatus task_status;
  auto task_info_ptr = task_status.mutable_task_info();
  task_info_ptr->CopyFrom(task_info);
  task_status.set_party(party_name);
  task_status.set_status(status);
  task_status.set_message(message);
  rpc::Empty no_reply;
  auto channel = link_ctx_->getChannel(scheduler_node);
  return channel->updateTaskStatus(task_status, &no_reply);
}

retcode VMNodeImpl::UpdateTaskStatus(
    const rpc::TaskStatus& task_status) {
  const auto& task_info = task_status.task_info();
  std::string request_id = task_info.request_id();
  std::string task_id = task_info.task_id();
  std::string job_id = task_info.job_id();
  auto worker_ptr = GetSchedulerWorker(task_info);
  if (worker_ptr == nullptr) {
    LOG(ERROR) << "no scheduler worker found for ["
               << TaskInfoToString(task_info) << "]";
    return retcode::FAIL;
  }
  auto ret = worker_ptr->updateTaskStatus(task_status);
  if (ret != retcode::SUCCESS) {
    LOG(WARNING) << "no task status get from scheduler";
  }
  return retcode::SUCCESS;
}

std::shared_ptr<Worker> VMNodeImpl::GetWorker(
    const rpc::TaskContext& task_info) {
  std::string worker_id = this->GetWorkerId(task_info);
  std::shared_lock<std::shared_mutex> lck(task_executor_mtx_);
  auto it = task_executor_map_.find(worker_id);
  if (it != task_executor_map_.end()) {
    return std::get<0>(it->second.front());
  }
  LOG(ERROR) << "no worker found for worker id: " << worker_id;
  return nullptr;
}

std::shared_ptr<Worker> VMNodeImpl::GetSchedulerWorker(
    const rpc::TaskContext& task_info) {
  std::string worker_id = this->GetWorkerId(task_info);
  std::shared_lock<std::shared_mutex> lck(task_scheduler_mtx_);
  auto it = task_scheduler_map_.find(worker_id);
  if (it != task_scheduler_map_.end()) {
    return std::get<0>(it->second);
  }
  LOG(WARNING) << "no scheduler worker found for ["
               << TaskInfoToString(task_info) << "]";
  return nullptr;
}

bool VMNodeImpl::IsTaskWorkerReady(const std::string& worker_id) {
  if (write_flag_.load(std::memory_order::memory_order_relaxed)) {
    LOG(WARNING) << "lock task_executor_mtx_ is not available";
    return false;
  }
  std::shared_lock<std::shared_mutex> lck(task_executor_mtx_);
  auto it = task_executor_map_.find(worker_id);
  if (it != task_executor_map_.end()) {
    return true;
  }
  return false;
}

void VMNodeImpl::CacheLastTaskStatus(const std::string& worker_id,
    const rpc::TaskStatus::StatusCode status) {
  time_t now_ = std::time(nullptr);
  std::unique_lock<std::shared_mutex> lck(this->finished_task_status_mtx_);
  finished_task_status_[worker_id] = std::make_tuple(status, now_);
}

auto VMNodeImpl::IsFinishedTask(const std::string& worker_id) ->
    std::tuple<bool, rpc::TaskStatus::StatusCode> {
  std::shared_lock<std::shared_mutex> lck(finished_task_status_mtx_);
  auto it = finished_task_status_.find(worker_id);
  if (it != finished_task_status_.end()) {
    // LOG(ERROR) << "found worker id: " << worker_id;
    return std::make_tuple(true, std::get<0>(it->second));
  } else {
    return std::make_tuple(false, rpc::TaskStatus::FINISHED);
  }
}

void VMNodeImpl::CleanFinishedTaskThread() {
  // clean finished task
  finished_worker_fut_ = std::async(
    std::launch::async,
    [&]() {
      SET_THREAD_NAME("cleanFinihsedTask");
      while (true) {
        task_executor_container_t tmp_task;
        finished_task_queue_.wait_and_pop(tmp_task);
        if (stop_.load(std::memory_order::memory_order_relaxed)) {
          LOG(WARNING) << "cleanFinihsedTask exit";
          break;
        }
        SCopedTimer timer;
        task_executor_container_t().swap(tmp_task);
        auto time_cost = timer.timeElapse();
        VLOG(5) << "ManageTaskThread operator : desctory time cost: " << time_cost;
      }
    });
}

void VMNodeImpl::CleanTimeoutCachedTaskStatusThread() {
  // clean cached timeout stask status
  clean_cached_task_status_fut_ = std::async(
    std::launch::async,
    [&]() {
      SET_THREAD_NAME("cleanCachedTaskStatus");
      using shared_lock_t = std::shared_lock<std::shared_mutex>;
      while (true) {
        if (stop_.load(std::memory_order::memory_order_relaxed)) {
          LOG(WARNING) << "service begin to exit";
          break;
        }
        std::set<std::string> timeout_worker_id;
        {
          time_t now_ = ::time(nullptr);
          shared_lock_t lck(this->finished_task_status_mtx_);
          for (const auto& task_status : finished_task_status_) {
            auto& timestamp = std::get<1>(task_status.second);
            if (now_ - timestamp > this->cached_task_status_timeout_) {
              timeout_worker_id.emplace(task_status.first);
            }
          }
        }
        if (!timeout_worker_id.empty()) {
          VLOG(2) << "number of timeout task status need to earse: "
                  << timeout_worker_id.size();
          shared_lock_t lck(this->finished_task_status_mtx_);
          for (const auto& worker_id : timeout_worker_id) {
            finished_task_status_.erase(worker_id);
          }
        }
        std::this_thread::sleep_for(
            std::chrono::seconds(this->cached_task_status_timeout_/2));
      }
    });
}

void VMNodeImpl::CleanFinishedSchedulerWorkerThread() {
  // clean finished scheduler worker
  finished_scheduler_worker_fut_ = std::async(
    std::launch::async,
    [&]() {
      SET_THREAD_NAME("cleanSchedulerTask");
      // time_t now_ = ::time(nullptr);
      std::map<std::string, time_t> finished_scheduler;
      while (true) {
        std::set<std::string> timeouted_shceduler;
        do {
          std::string worker_id;
          fininished_scheduler_workers_.wait_and_pop(worker_id);
          if (stop_.load(std::memory_order::memory_order_relaxed)) {
            LOG(ERROR) << "cleanSchedulerTask quit";
            return;
          }
          time_t now_ = ::time(nullptr);
          finished_scheduler[worker_id] = now_;
          for (auto it = finished_scheduler.begin();
              it != finished_scheduler.end();) {
            if (now_ - it->second > this->scheduler_worker_timeout_s_) {
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
          std::lock_guard<std::shared_mutex> lck(this->task_executor_mtx_);
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
        LOG(INFO) << "clean timeouted scheduler task cost(ms): "
                  << timer.timeElapse();
      }
    });
}

// data communication related
retcode VMNodeImpl::ProcessReceivedData(const rpc::TaskContext& task_info,
                                        const std::string& key,
                                        std::string&& data_buffer) {
  std::string worker_id = this->GetWorkerId(task_info);
  auto finished_task = this->IsFinishedTask(worker_id);
  if (std::get<0>(finished_task)) {
    std::string err_msg = "Task Worker for ";
    err_msg.append("job id: ").append(task_info.job_id())
            .append(" task id: ").append(task_info.task_id())
            .append(" has finished");
    LOG(ERROR) << err_msg;
    return retcode::FAIL;
  }
  auto ret = WaitUntilWorkerReady(worker_id, wait_worker_ready_timeout_ms_);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "wati worker ready timeout";
    return retcode::FAIL;
  }
  auto worker_ptr = this->GetWorker(task_info);
  if (worker_ptr == nullptr) {
    std::string err_msg = "Task worker for ";
    err_msg.append("job id: ").append(task_info.job_id())
           .append(" task id: ").append(task_info.task_id())
           .append(" not found");
    LOG(ERROR) << err_msg;
    return retcode::FAIL;
  }
  auto& link_ctx = worker_ptr->getTask()->getTaskContext().getLinkContext();
  if (link_ctx == nullptr) {
    std::string err_msg = "LinkContext is empty for ";
    err_msg.append("job id: ").append(task_info.job_id()).append(" ")
           .append("task id: ").append(task_info.task_id()).append(" ")
           .append("request id: ").append(task_info.request_id());
    LOG(ERROR) << err_msg;
    return retcode::FAIL;
  }
  size_t data_size = data_buffer.size();
  auto& recv_queue = link_ctx->GetRecvQueue(key);
  recv_queue.push(std::move(data_buffer));
  VLOG(5) << "end of VMNodeImpl::Send, data total received size:" << data_size;
  return retcode::SUCCESS;
}

retcode VMNodeImpl::ProcessSendData(const rpc::TaskContext& task_info,
                                    const std::string& key,
                                    std::string* data_buffer) {
  std::string worker_id = this->GetWorkerId(task_info);
  auto finished_task = this->IsFinishedTask(worker_id);
  if (std::get<0>(finished_task)) {
    std::string err_msg = "Task Worker for ";
    err_msg.append("job id: ").append(task_info.job_id())
            .append(" task id: ").append(task_info.task_id())
            .append(" has finished");
    LOG(ERROR) << err_msg;
    return retcode::FAIL;
  }
  auto ret = WaitUntilWorkerReady(worker_id, wait_worker_ready_timeout_ms_);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "wati worker ready timeout";
    return retcode::FAIL;
  }
  auto worker_ptr = this->GetWorker(task_info);
  if (worker_ptr == nullptr) {
    std::string err_msg = "Task worker for ";
    err_msg.append("job id: ").append(task_info.job_id())
           .append(" task id: ").append(task_info.task_id())
           .append(" request id: ").append(task_info.request_id())
           .append(" not found");
    LOG(ERROR) << err_msg;
    return retcode::FAIL;
  }
  auto& link_ctx = worker_ptr->getTask()->getTaskContext().getLinkContext();
  if (link_ctx == nullptr) {
    std::string err_msg = "LinkContext is empty for ";
    err_msg.append("job id: ").append(task_info.job_id()).append(" ")
           .append("task id: ").append(task_info.task_id()).append(" ")
           .append("request id: ").append(task_info.request_id());
    LOG(ERROR) << err_msg;
    return retcode::FAIL;
  }
  auto& send_queue = link_ctx->GetSendQueue(key);
  send_queue.wait_and_pop(*data_buffer);
  // make sure the send thread get the send data success
  auto& complete_queue = link_ctx->GetCompleteQueue(key);
  complete_queue.push(retcode::SUCCESS);
  return retcode::SUCCESS;
}

retcode VMNodeImpl::ProcessForwardData(const rpc::TaskContext& task_info,
                                       const std::string& key,
                                       std::string* data_buffer) {
  std::string worker_id = this->GetWorkerId(task_info);
  auto finished_task = this->IsFinishedTask(worker_id);
  if (std::get<0>(finished_task)) {
    std::string err_msg = "Task Worker for ";
    err_msg.append("job id: ").append(task_info.job_id())
            .append(" task id: ").append(task_info.task_id())
            .append(" request id: ").append(task_info.request_id())
            .append(" has finished");
    LOG(ERROR) << err_msg;
    return retcode::FAIL;
  }
  auto ret = WaitUntilWorkerReady(worker_id,
                                  this->wait_worker_ready_timeout_ms_);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "wati worker ready timeout";
    return retcode::FAIL;
  }
  auto worker_ptr = this->GetWorker(task_info);
  if (worker_ptr == nullptr) {
    std::string err_msg = "Task worker for ";
    err_msg.append("job id: ").append(task_info.job_id())
           .append(" task id: ").append(task_info.task_id())
           .append(" request id: ").append(task_info.request_id())
           .append(" not found");
    LOG(ERROR) << err_msg;
    return retcode::FAIL;
  }
  auto& link_ctx = worker_ptr->getTask()->getTaskContext().getLinkContext();
  if (link_ctx == nullptr) {
    std::string err_msg = "LinkContext is empty for ";
     err_msg.append("job id: ").append(task_info.job_id())
           .append(" task id: ").append(task_info.task_id())
           .append(" request id: ").append(task_info.request_id());
    LOG(ERROR) << err_msg;
    return retcode::FAIL;
  }
  auto& recv_queue = link_ctx->GetRecvQueue(key);
  recv_queue.wait_and_pop(*data_buffer);
  return retcode::SUCCESS;
}

retcode VMNodeImpl::WaitUntilWorkerReady(const std::string& worker_id,
                                         int timeout_ms) {
  SCopedTimer timer;
  do {
    // try to get lock
    if (IsTaskWorkerReady(worker_id)) {
      break;
    }
    VLOG(5) << "sleep and wait for worker ready, "
            << "worker id : " << worker_id << " ........";
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (timeout_ms == -1) {
      continue;
    }
    auto time_elapse_ms = timer.timeElapse();
    if (time_elapse_ms > timeout_ms) {
      LOG(ERROR) << "wait for worker ready timeout(ms): " << timeout_ms;
      return retcode::FAIL;
    }
  } while (true);

  return retcode::SUCCESS;
}

void VMNodeImpl::CleanDuplicateTaskIdFilter() {
  SCopedTimer timer;
  std::map<std::string, int8_t> timeouted_task_id;
  std::lock_guard<std::mutex> lck(duplicate_task_filter_mtx_);
  time_t now = time(nullptr);
  for (const auto& [uid, c_time] : duplicate_task_id_filter_) {
    if (now - c_time > task_id_timeout_s_) {
      timeouted_task_id[uid] = 1;
    }
  }

  for (const auto& [uid, _] : timeouted_task_id) {
    duplicate_task_id_filter_.erase(uid);
  }
  size_t timeout_task_count = timeouted_task_id.size();
  size_t filter_size = duplicate_task_id_filter_.size();
  auto time_cost = timer.timeElapse();
  VLOG(5) << "CleanDuplicateTaskIdFilter time cost(ms): " << time_cost << " "
          << "timeout task count: " << timeout_task_count << " "
          << "task remain count: " << filter_size;
}

bool VMNodeImpl::IsDuplicateTask(const rpc::TaskContext& task_info) {
  auto task_uid = task_info.request_id() + "_" + task_info.sub_task_id();
  std::lock_guard<std::mutex> lck(duplicate_task_filter_mtx_);
  auto it = duplicate_task_id_filter_.find(task_uid);
  if (it != duplicate_task_id_filter_.end()) {
    return true;
  } else {
    duplicate_task_id_filter_.insert({task_uid, time(nullptr)});
    return false;
  }
}

retcode VMNodeImpl::GetAllParties(const rpc::Task& task_config,
                                  std::vector<Node>* all_party) {
  const auto& party_access_info = task_config.party_access_info();
  for (const auto& [party_name, pb_node] : party_access_info) {
    LOG(ERROR) << "GetAllParties party_name: " << party_name;
    Node node;
    pbNode2Node(pb_node, &node);
    all_party->push_back(node);
  }
  // auxiliary server
  auto auxiliary_server = task_config.auxiliary_server();
  auto it = auxiliary_server.find(AUX_COMPUTE_NODE);
  if (it != auxiliary_server.end()) {
    LOG(ERROR) << "GetAllParties party_name: " << AUX_COMPUTE_NODE;
    auto& pb_node = it->second;
    Node node;
    pbNode2Node(pb_node, &node);
    all_party->push_back(node);
  }
  return retcode::SUCCESS;
}

retcode VMNodeImpl::ExecuteAddTaskOperation(task_manage_t&& task_detail_) {
  VLOG(7) << "VMNodeImpl::ManageTaskThread recv task operator ADD";
  auto task_detail = std::move(task_detail_);
  auto& worker_id = std::get<0>(task_detail);
  auto& task = std::get<1>(task_detail);
  do {
    std::shared_lock<std::shared_mutex> lck(task_executor_mtx_);
    auto it = task_executor_map_.find(worker_id);
    if (it != task_executor_map_.end()) {
      auto& task_queue = task_executor_map_[worker_id];
      task_queue.emplace(std::move(task));
      return retcode::SUCCESS;
    }
  } while (0);

  do {
    write_flag_.store(true);
    SCopedTimer timer;
    std::lock_guard<std::shared_mutex> lck(task_executor_mtx_);
    auto& task_queue = task_executor_map_[worker_id];
    task_queue.emplace(std::move(task));
    double time_cost = timer.timeElapse();
    VLOG(7) << "ManageTaskThread operator: add worker id: " << worker_id << " "
            << "time cost(ms): " << time_cost;
    write_flag_.store(false);
  } while (0);

  return retcode::SUCCESS;
}

retcode VMNodeImpl::ExecuteDelTaskOperation(task_manage_t&& task_detail_) {
  VLOG(7) << "VMNodeImpl::ManageTaskThread recv task operator DEL";
  SCopedTimer timer;
  auto task_detail = std::move(task_detail_);
  write_flag_.store(true);
  std::lock_guard<std::shared_mutex> lck(task_executor_mtx_);
  auto lock_time = timer.timeElapse();
  auto& worker_id = std::get<0>(task_detail);
  auto it = task_executor_map_.find(worker_id);
  if (it != task_executor_map_.end()) {
    double swap_cost = 0;
    double dctr_cost = 0;
    SCopedTimer sub_timer;
    {
      task_executor_container_t tmp;
      auto& queue = task_executor_map_[worker_id];
      queue.swap(tmp);
      task_executor_map_.erase(worker_id);
      finished_task_queue_.push(std::move(tmp));  // push to real clean thread
      swap_cost = sub_timer.timeElapse();
    }
    auto end = sub_timer.timeElapse();
    dctr_cost = end - swap_cost;
    double time_cost = timer.timeElapse();
    VLOG(7) << "ManageTaskThread operator: erase worker id: "
            << worker_id << " "
            << "get lock time: " << lock_time << " "
            << "swap queue cost: " << swap_cost << " "
            << "destroy queue cost: " << dctr_cost << " "
            << "total time cost(ms): " << time_cost;
  }
  write_flag_.store(false);
  return retcode::SUCCESS;
}

retcode VMNodeImpl::ExecuteKillTaskOperation(task_manage_t&& task_detail_) {
  VLOG(7) << "VMNodeImpl::ManageTaskThread recv task operator KILL";
  SCopedTimer timer;
  auto task_detail = std::move(task_detail_);
  write_flag_.store(true);
  std::lock_guard<std::shared_mutex> lck(task_executor_mtx_);
  auto lock_time = timer.timeElapse();
  auto& worker_id = std::get<0>(task_detail);
  auto it = task_executor_map_.find(worker_id);
  if (it != task_executor_map_.end()) {
    double swap_cost = 0;
    double dctr_cost = 0;
    SCopedTimer sub_timer;
    {
      task_executor_container_t tmp;
      auto& queue = task_executor_map_[worker_id];
      queue.swap(tmp);
      task_executor_map_.erase(worker_id);
      kill_task_queue_.push(std::move(tmp));
      swap_cost = sub_timer.timeElapse();
    }
    auto end = sub_timer.timeElapse();
    dctr_cost = end - swap_cost;
    double time_cost = timer.timeElapse();
    VLOG(7) << "ManageTaskThread operator: kill worker id: "
            << worker_id << " "
            << "get lock time: " << lock_time << " "
            << "swap queue cost: " << swap_cost << " "
            << "destroy queue cost: " << dctr_cost << " "
            << "total time cost(ms): " << time_cost;
  }
  write_flag_.store(false);
  return retcode::SUCCESS;
}
}  // namespace primihub
