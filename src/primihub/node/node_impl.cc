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
#include <curl/curl.h>
#include <vector>
#include <fstream>
#include <algorithm>
#include "src/primihub/common/common.h"
#include "src/primihub/data_store/factory.h"
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/task/language/factory.h"
#include "src/primihub/task/semantic/parser.h"
#include "src/primihub/util/file_util.h"
#include "src/primihub/util/util.h"
#include "src/primihub/util/network/link_factory.h"
#include "src/primihub/util/log.h"
#include "src/primihub/util/proto_log_helper.h"
#include "uuid.h"                             // NOLINT
#include "src/primihub/util/hash.h"

namespace pb_util = primihub::proto::util;
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
  report_alive_info_fut_.get();
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
  if (node_cfg.use_tls()) {
    link_ctx_->initCertificate(server_config.getCertificateConfig());
  }
  CleanFinishedTaskThread();
  CleanTimeoutCachedTaskStatusThread();
  CleanFinishedSchedulerWorkerThread();
  ManageTaskOperatorThread();
  ProcessKillTaskThread();
  ReportAliveInfoThread();
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
          PH_LOG(WARNING, LogType::kScheduler) << "ProcessKilledTask exit";
          break;
        }
        SCopedTimer timer;
        while (!task_queue.empty()) {
          auto& task_info = task_queue.front();
          auto& task_worker_ptr = std::get<0>(task_info);
          if (task_worker_ptr != nullptr) {
            PH_LOG(ERROR, LogType::kScheduler)
                << "kill task for workerId: " << task_worker_ptr->workerId();
            task_worker_ptr->kill_task();
            // auto& fut = std:;get<1>(task_info);
          }
          task_queue.pop();
        }
        task_executor_container_t().swap(task_queue);
        auto time_cost = timer.timeElapse();
        PH_LOG(ERROR, LogType::kScheduler)
            << "ManageTaskThread operator : kill task time cost: " << time_cost;
      }
    });
}

std::string VMNodeImpl::GenerateUUID() {
  std::random_device rd;
  auto seed_data = std::array<int, std::mt19937::state_size> {};
  std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
  std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
  std::mt19937 generator(seq);
  uuids::uuid_random_generator gen{generator};
  const uuids::uuid id = gen();
  std::string device_uuid = uuids::to_string(id);
  return device_uuid;
}

auto VMNodeImpl::GetDeviceInfo() ->
    std::tuple<std::string, std::string> {
  auto& server_cfg = ServerConfig::getInstance();
  auto& node_info = server_cfg.getServiceConfig();
  std::string node_info_str = node_info.to_string();
  auto hash_func = Hash("md5");
  std::string file_name = hash_func.HashToString(node_info_str);
  const auto buffer = reinterpret_cast<uint8_t*>(file_name.data());
  size_t buffer_size = file_name.size();
  file_name = ".device_";
  file_name.append(buf_to_hex_string(buffer, buffer_size));
  std::string file_path = CompletePath("./config", file_name);
  std::string device_id;
  if (!FileExists(file_path)) {
    device_id = GenerateUUID();
    std::ofstream fout(file_path, std::ios::out);
    if (fout.is_open()) {
      fout << device_id << "\n";
      fout.close();
    }
  } else {
    std::ifstream fin(file_path, std::ios::in);
    if (fin.is_open()) {
      std::getline(fin, device_id);
    }
    if (device_id.empty()) {
      device_id = GenerateUUID();
      std::ofstream fout(file_path, std::ios::out);
      if (fout.is_open()) {
        fout << device_id << "\n";
        fout.close();
      }
    }
  }
  return std::make_tuple(device_id, node_info.id());
}

static size_t WriteCallback(char* d, size_t n, size_t l, void* p) {
  (void*)d;   // NOLINT
  (void*)p;   // NOLINT
  return n * l;
}

void VMNodeImpl::ReportAliveInfoThread() {
  report_alive_info_fut_ = std::async(
    std::launch::async,
    [&]() -> void {
      auto& ins = ServerConfig::getInstance();
      auto& cfg = ins.getNodeConfig();
      if (cfg.disable_report) {
        return;
      }
      curl_global_init(CURL_GLOBAL_ALL);
      CURL* curl = curl_easy_init();
      if (curl == nullptr) {
        return;
      }
      const auto& [device_id, node_id] = GetDeviceInfo();
      int report_period_min = 30;  // report status every 30 minutes
      std::string url = "https://node1.primihub.com/collect/operate/addNode";
      struct curl_slist* header = nullptr;
      header = curl_slist_append(
          header, "Content-Type: application/x-www-form-urlencoded");
      if (header == nullptr) {
        return;
      }

      auto& node_info = ins.getServiceConfig();
      std::string content;
      content
        .append("key=").append("5oC06czJLeF3kXdfg7D1q2z0G4wwYJ3l").append("&")
        .append("globalId=").append(device_id).append("&")
        .append("globalName=").append(node_id);
      VLOG(9) << "ReportAliveInfoThread: content: " << content;
      while (true) {
        if (stop_.load(std::memory_order::memory_order_relaxed)) {
          PH_LOG(WARNING, LogType::kScheduler) << "ReportAliveInfoThread exit";
          break;
        }
        bool success_flag{true};
        if (curl) {
          CURLcode res;
          curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
          curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
          curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
          curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
          curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
          curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 5000L);  //  5s
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, content.c_str());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
          res = curl_easy_perform(curl);
          if (res != CURLE_OK) {
            VLOG(9) << "ReportAliveInfoThread curl_easy_perform() failed, "
                    << curl_easy_strerror(res);
          }
          curl_easy_reset(curl);
        }
        std::this_thread::sleep_for(std::chrono::minutes(report_period_min));
      }
      curl_slist_free_all(header);
      curl_easy_cleanup(curl);
    });
}

void VMNodeImpl::ManageTaskOperatorThread() {
  manage_task_worker_fut_ = std::async(
    std::launch::async,
    [&]() {
      SET_THREAD_NAME("manageTaskOperator");
      PH_VLOG(0, LogType::kScheduler) << "start ManageTaskThread";
      while (true) {
        task_manage_t task_info;
        task_manage_queue_.wait_and_pop(task_info);
        if (stop_.load(std::memory_order::memory_order_relaxed)) {
          PH_LOG(WARNING, LogType::kScheduler) << "service begin to exit";
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
  rpc::TaskContext task_info;
  return CreateWorker(task_info);
}

std::shared_ptr<Worker> VMNodeImpl::CreateWorker(const std::string& worker_id) {
  rpc::TaskContext task_info;
  task_info.set_request_id(worker_id);
  return CreateWorker(task_info);
}

std::shared_ptr<Worker> VMNodeImpl::CreateWorker(
    const rpc::TaskContext& task_info) {
  std::string TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
  PH_LOG(INFO, LogType::kScheduler)
      << TASK_INFO_STR
      << "Start create worker " << this->node_id_;
  auto worker =
      std::make_shared<Worker>(this->node_id_, task_info, GetNodelet());
  PH_LOG(INFO, LogType::kScheduler)
      << TASK_INFO_STR
      << "Fininsh create worker " << this->node_id_;
  return worker;
}

retcode VMNodeImpl::DispatchTask(const rpc::PushTaskRequest& task_request,
                                 rpc::PushTaskReply* reply) {
  auto scheduler_func = [this](std::shared_ptr<Worker> worker,
      std::vector<Node> parties, const rpc::Task task_config,
      ThreadSafeQueue<std::string>* finished_scheduler_workers) -> void {
    SET_THREAD_NAME("DispatchTask");
    // get the final task status, if failed, kill current task
    auto ret = worker->waitUntilTaskFinish();
    auto& task_info = task_config.task_info();
    std::string TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
    if (ret != retcode::SUCCESS) {
      PH_LOG(ERROR, LogType::kScheduler)
          << TASK_INFO_STR
          << "task execute encountes error, "
          << "begin to kill task to release resource";
      std::vector<Node> recv_cmd_party;;
      std::vector<Node> all_party;
      GetAllParties(task_config, &all_party);
      std::set<std::string> duplicate_filter;
      for (const auto& party : all_party) {
        std::string node_info = party.to_string();
        if (duplicate_filter.find(node_info) != duplicate_filter.end()) {
          continue;
        }
        duplicate_filter.emplace(std::move(node_info));
        recv_cmd_party.push_back(party);
      }
      for (const auto& party : parties) {
        std::string node_info = party.to_string();
        if (duplicate_filter.find(node_info) != duplicate_filter.end()) {
          continue;
        }
        duplicate_filter.emplace(std::move(node_info));
        recv_cmd_party.push_back(party);
      }
      rpc::KillTaskRequest kill_request;
      auto task_info_ = kill_request.mutable_task_info();
      task_info_->CopyFrom(task_info);
      kill_request.set_executor(rpc::KillTaskRequest::SCHEDULER);
      for (const auto& patry : recv_cmd_party) {
        rpc::KillTaskResponse reply;
        auto channel = link_ctx_->getChannel(patry);
        channel->killTask(kill_request, &reply);
      }
    }
    finished_scheduler_workers->push(task_info.request_id());
  };
  const auto& task_config = task_request.task();
  const auto& task_info = task_config.task_info();
  std::string TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
  PH_LOG(INFO, LogType::kScheduler)
      << TASK_INFO_STR
      << "start to schedule task, task_type: "
      << static_cast<int>(task_config.type());
  std::string worker_id = this->GetWorkerId(task_info);
  std::shared_ptr<Worker> worker_ptr = CreateWorker(task_info);
  {
    std::unique_lock<std::shared_mutex> lck(task_scheduler_mtx_);
    task_scheduler_map_.insert(
        {worker_id, std::make_tuple(worker_ptr, std::future<void>())});
  }
  PH_VLOG(2, LogType::kScheduler)
      << TASK_INFO_STR
      << "insert worker id: " << worker_id
      << " into task_scheduler_map success";
  // absl::MutexLock lock(&parser_mutex_);
  // Construct language parser
  try {
    auto lan_parser_ = task::LanguageParserFactory::Create(task_request);
    if (lan_parser_ == nullptr) {
      std::string error_msg = "create LanguageParser failed";
      PH_LOG(ERROR, LogType::kScheduler) << TASK_INFO_STR << error_msg;
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
      PH_LOG(ERROR, LogType::kScheduler) << TASK_INFO_STR << error_msg;
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
      PH_VLOG(7, LogType::kScheduler)
          << TASK_INFO_STR << "update worker id: " << worker_id;
    }
    auto& server_cfg = ServerConfig::getInstance();
    auto& service_node_info = server_cfg.getServiceConfig();
    auto pb_task_server = reply->add_task_server();
    node2PbNode(service_node_info, pb_task_server);
    reply->set_party_count(task_server.size());
  } catch (std::exception& e) {
    std::string error_msg = "dispatch task to party failed, detail: ";
    auto len = strlen(e.what());
    error_msg.append(e.what(), len);
    PH_LOG(ERROR, LogType::kScheduler) << TASK_INFO_STR << error_msg;
    reply->set_ret_code(2);
    reply->set_msg_info(error_msg);
  }
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
    std::string TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
    // add proxy server info
    auto& server_cfg = ServerConfig::getInstance();
    auto& proxy_node = server_cfg.ProxyServerCfg();
    auto auxiliary_server = request.mutable_task()->mutable_auxiliary_server();
    {
      rpc::Node pb_proxy_node;
      node2PbNode(proxy_node, &pb_proxy_node);
      (*auxiliary_server)[PROXY_NODE] = std::move(pb_proxy_node);
    }

    PH_LOG(INFO, LogType::kScheduler)
        << TASK_INFO_STR << "begin to execute task";
    rpc::TaskStatus::StatusCode status = rpc::TaskStatus::RUNNING;
    std::string status_info = "task is running";
    this->NotifyTaskStatus(request, status, status_info);
    try {
      auto result_info = worker->execute(&request);
      if (result_info == retcode::SUCCESS) {
        status = rpc::TaskStatus::SUCCESS;
        status_info = "task finished";
      } else {
        status = rpc::TaskStatus::FAIL;
        status_info = "task execute encountes error";
      }
    } catch (std::exception& e) {
      status = rpc::TaskStatus::FAIL;
      size_t len = std::min<size_t>(strlen(e.what()), 1024);
      status_info = std::string(e.what(), len);
    }

    this->NotifyTaskStatus(request, status, status_info);
    if (request.manual_launch()) {
      PH_VLOG(0, LogType::kScheduler)
          << TASK_INFO_STR << "execute sub task finished";
      return;
    }
    PH_VLOG(5, LogType::kScheduler)
        << TASK_INFO_STR
        << "execute task end, begin to clean task";

    this->CacheLastTaskStatus(worker->worker_id(), status);
    // finished_worker_queue->push(worker->worker_id());
    task_manage_queue->push(
        std::make_tuple(worker->worker_id(),
            std::make_tuple(nullptr, std::future<void>()),
            OperateTaskType::kDel));
    auto time_cost = timer.timeElapse();
    PH_VLOG(5, LogType::kScheduler)
        << TASK_INFO_STR
        << "execute task end, clean task finished, "
        << "task total cost time(ms): " << time_cost;
  };
  const auto& task_config = task_request.task();
  const auto& task_info = task_config.task_info();
  auto TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
  PH_LOG(INFO, LogType::kScheduler)
      << TASK_INFO_STR
      << "start to create worker for task: ";
  CleanDuplicateTaskIdFilter();
  if (IsDuplicateTask(task_info)) {
    PH_LOG(ERROR, LogType::kScheduler)
        << TASK_INFO_STR
        << "task has already received, ignore ....";
    return retcode::FAIL;
  }
  std::string worker_id = GetWorkerId(task_info);
  std::shared_ptr<Worker> worker = CreateWorker(task_info);
  auto fut = std::async(std::launch::async,
                        executor_func,
                        worker,
                        task_request,
                        &this->task_manage_queue_);
  PH_LOG(INFO, LogType::kScheduler)
      << TASK_INFO_STR
      << "create execute worker thread future finished";
  auto ret = worker->waitForTaskReady();
  if (ret == retcode::FAIL) {
    rpc::TaskStatus::StatusCode status = rpc::TaskStatus::FAIL;
    std::string status_info = "Initialize task failed.";
    this->NotifyTaskStatus(task_request, status, status_info);
    return retcode::FAIL;
  }
  // save work for future use
  PH_VLOG(7, LogType::kScheduler)
      << TASK_INFO_STR
      << "end of worker->waitForTaskReady(), begin to save handler";
  auto task_worker = std::make_tuple(std::move(worker), std::move(fut));
  auto task_worker_info
    = std::make_tuple(worker_id, std::move(task_worker), OperateTaskType::kAdd);
  task_manage_queue_.push(std::move(task_worker_info));
  PH_LOG(INFO, LogType::kScheduler)
      << TASK_INFO_STR
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
  auto TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
  auto executor = request.executor();
  PH_VLOG(0, LogType::kScheduler)
      << TASK_INFO_STR
      << ", receive request for kill task from "
      << (executor == rpc::KillTaskRequest::CLIENT ?
                      ROLE_CLIENT :
                      ROLE_SCHEDULER);

  std::string worker_id = this->GetWorkerId(task_info);
  auto finished_task = this->IsFinishedTask(worker_id);
  if (std::get<0>(finished_task)) {
    std::string err_msg = "worker for ";
    err_msg.append(TASK_INFO_STR).append("has finished");
    PH_LOG(WARNING, LogType::kScheduler) << err_msg;
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
    PH_LOG(WARNING, LogType::kScheduler)
        << TASK_INFO_STR
        << "worker does not find for worker id: " << worker_id;
  }
  response->set_ret_code(rpc::SUCCESS);
  PH_VLOG(0, LogType::kScheduler)
      << TASK_INFO_STR << "end of VMNodeImpl::KillTask";
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
    const auto& task_info = request.task().task_info();
    auto TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
    PH_LOG(ERROR, LogType::kScheduler)
        << TASK_INFO_STR
        << "no config found for: " << SCHEDULER_NODE;
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode VMNodeImpl::FetchTaskStatus(const rpc::TaskContext& task_info,
                                    rpc::TaskStatusReply* response) {
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
    const auto& task_info = request.task().task_info();
    auto TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
    PH_LOG(ERROR, LogType::kScheduler)
        << TASK_INFO_STR << "get scheduler node cfg failed";
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
  auto TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
  auto worker_ptr = GetSchedulerWorker(task_info);
  if (worker_ptr == nullptr) {
    PH_LOG(ERROR, LogType::kScheduler)
        << TASK_INFO_STR
        << "no scheduler worker is available";
    return retcode::FAIL;
  }
  auto ret = worker_ptr->updateTaskStatus(task_status);
  if (ret != retcode::SUCCESS) {
    PH_LOG(WARNING, LogType::kScheduler)
        << TASK_INFO_STR
        << "no task status get from scheduler";
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
  auto TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
  PH_LOG(ERROR, LogType::kScheduler)
      << TASK_INFO_STR
      << "no worker is available for worker id: " << worker_id;
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
  PH_LOG(WARNING, LogType::kScheduler)
      << pb_util::TaskInfoToString(task_info)
      << "no scheduler worker is available for worker id: " << worker_id;
  return nullptr;
}

bool VMNodeImpl::IsTaskWorkerReady(const std::string& worker_id) {
  if (write_flag_.load(std::memory_order::memory_order_relaxed)) {
    PH_LOG(WARNING, LogType::kScheduler)
        << pb_util::TaskInfoToString(worker_id)
        << "lock task_executor_mtx is not available";
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
          PH_LOG(WARNING, LogType::kScheduler) << "cleanFinihsedTask exit";
          break;
        }
        SCopedTimer timer;
        task_executor_container_t().swap(tmp_task);
        auto time_cost = timer.timeElapse();
        PH_VLOG(5, LogType::kScheduler)
            << "ManageTaskThread operator: destroy time cost: " << time_cost;
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
          PH_LOG(WARNING, LogType::kScheduler) << "service begin to exit";
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
          PH_VLOG(2, LogType::kScheduler)
              << "number of timeout task status need to earse: "
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
            PH_LOG(ERROR, LogType::kScheduler) << "cleanSchedulerTask quit";
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
          PH_VLOG(3, LogType::kScheduler)
              << "cleanSchedulerTask size of need to clean task: "
              << timeouted_shceduler.size();
          std::lock_guard<std::shared_mutex> lck(this->task_executor_mtx_);
          for (const auto& scheduler_worker_id : timeouted_shceduler) {
            auto it = task_scheduler_map_.find(scheduler_worker_id);
            if (it != task_scheduler_map_.end()) {
              PH_VLOG(5, LogType::kScheduler)
                  << "scheduler worker id : " << scheduler_worker_id << " "
                  << "has timeouted, begin to erase";
              task_scheduler_map_.erase(scheduler_worker_id);
              PH_VLOG(5, LogType::kScheduler)
                  << "erase scheduler worker id : "
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
  auto TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
  if (std::get<0>(finished_task)) {
    std::string err_msg = "Task Worker for ";
    err_msg.append(TASK_INFO_STR)
           .append("has finished");
    PH_LOG(ERROR, LogType::kTask) << err_msg;
    return retcode::FAIL;
  }
  auto ret = WaitUntilWorkerReady(worker_id, wait_worker_ready_timeout_ms_);
  if (ret != retcode::SUCCESS) {
    PH_LOG(ERROR, LogType::kTask)
        << TASK_INFO_STR << "wait worker ready timeout";
    return retcode::FAIL;
  }
  auto worker_ptr = this->GetWorker(task_info);
  if (worker_ptr == nullptr) {
    std::string err_msg;
    err_msg.append(TASK_INFO_STR)
           .append("Task worker is not found");
    PH_LOG(ERROR, LogType::kTask) << err_msg;
    return retcode::FAIL;
  }
  auto& link_ctx = worker_ptr->getTask()->getTaskContext().getLinkContext();
  if (link_ctx == nullptr) {
    std::string err_msg;
    err_msg.append(TASK_INFO_STR)
           .append("LinkContext is empty");
    PH_LOG(ERROR, LogType::kTask) << err_msg;
    return retcode::FAIL;
  }
  size_t data_size = data_buffer.size();
  auto& recv_queue = link_ctx->GetRecvQueue(key);
  recv_queue.push(std::move(data_buffer));
  PH_VLOG(5, LogType::kTask)
      << TASK_INFO_STR
      << "end of VMNodeImpl::Send, data total received size:" << data_size;
  return retcode::SUCCESS;
}

retcode VMNodeImpl::ProcessSendData(const rpc::TaskContext& task_info,
                                    const std::string& key,
                                    std::string* data_buffer) {
  auto TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
  std::string worker_id = this->GetWorkerId(task_info);
  auto finished_task = this->IsFinishedTask(worker_id);
  if (std::get<0>(finished_task)) {
    std::string err_msg;
    err_msg.append(TASK_INFO_STR)
            .append("Task Worker has been finished");
    PH_LOG(ERROR, LogType::kTask) << err_msg;
    return retcode::FAIL;
  }
  auto ret = WaitUntilWorkerReady(worker_id, wait_worker_ready_timeout_ms_);
  if (ret != retcode::SUCCESS) {
    PH_LOG(ERROR, LogType::kTask)
        << TASK_INFO_STR << "wati worker ready timeout";
    return retcode::FAIL;
  }
  auto worker_ptr = this->GetWorker(task_info);
  if (worker_ptr == nullptr) {
    std::string err_msg;
    err_msg.append(TASK_INFO_STR)
           .append("Task worker is not found");
    PH_LOG(ERROR, LogType::kTask) << err_msg;
    return retcode::FAIL;
  }
  auto& link_ctx = worker_ptr->getTask()->getTaskContext().getLinkContext();
  if (link_ctx == nullptr) {
    std::string err_msg;
    err_msg.append(TASK_INFO_STR)
           .append("LinkContext is empty");
    PH_LOG(ERROR, LogType::kTask) << err_msg;
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
  auto TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
  auto finished_task = this->IsFinishedTask(worker_id);
  if (std::get<0>(finished_task)) {
    std::string err_msg;
    err_msg.append(TASK_INFO_STR)
            .append("Task Worker has been finished");
    PH_LOG(ERROR, LogType::kTask) << err_msg;
    return retcode::FAIL;
  }
  auto ret = WaitUntilWorkerReady(
      worker_id, this->wait_worker_ready_timeout_ms_);
  if (ret != retcode::SUCCESS) {
    PH_LOG(ERROR, LogType::kTask)
        << TASK_INFO_STR << ", wati worker ready is timeout";
    return retcode::FAIL;
  }
  auto worker_ptr = this->GetWorker(task_info);
  if (worker_ptr == nullptr) {
    std::string err_msg;
    err_msg.append(TASK_INFO_STR)
           .append("Task worker is not found");
    PH_LOG(ERROR, LogType::kTask) << err_msg;
    return retcode::FAIL;
  }
  auto& link_ctx = worker_ptr->getTask()->getTaskContext().getLinkContext();
  if (link_ctx == nullptr) {
    std::string err_msg;
    err_msg.append(TASK_INFO_STR).append("LinkContext is empty");
    PH_LOG(ERROR, LogType::kTask) << err_msg;
    return retcode::FAIL;
  }
  auto& recv_queue = link_ctx->GetRecvQueue(key);
  recv_queue.wait_and_pop(*data_buffer);
  auto& complete_queue = link_ctx->GetCompleteQueue(key);
  complete_queue.push(retcode::SUCCESS);
  return retcode::SUCCESS;
}

retcode VMNodeImpl::ProcessCompleteStatus(const rpc::TaskContext& task_info,
                                          const std::string& key,
                                          uint64_t expected_complete_num) {
//
  std::string worker_id = this->GetWorkerId(task_info);
  auto TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
  auto finished_task = this->IsFinishedTask(worker_id);
  if (std::get<0>(finished_task)) {
    std::string err_msg;
    err_msg.append(TASK_INFO_STR)
            .append("Task Worker has been finished");
    PH_LOG(ERROR, LogType::kTask) << err_msg;
    return retcode::FAIL;
  }
  auto ret = WaitUntilWorkerReady(
      worker_id, this->wait_worker_ready_timeout_ms_);
  if (ret != retcode::SUCCESS) {
    PH_LOG(ERROR, LogType::kTask)
        << TASK_INFO_STR << ", wati worker ready is timeout";
    return retcode::FAIL;
  }
  auto worker_ptr = this->GetWorker(task_info);
  if (worker_ptr == nullptr) {
    std::string err_msg;
    err_msg.append(TASK_INFO_STR)
           .append("Task worker is not found");
    PH_LOG(ERROR, LogType::kTask) << err_msg;
    return retcode::FAIL;
  }
  auto& link_ctx = worker_ptr->getTask()->getTaskContext().getLinkContext();
  if (link_ctx == nullptr) {
    std::string err_msg;
    err_msg.append(TASK_INFO_STR).append("LinkContext is empty");
    PH_LOG(ERROR, LogType::kTask) << err_msg;
    return retcode::FAIL;
  }
  auto& complete_queue = link_ctx->GetCompleteQueue(key);
  uint64_t complete_count{0};
  do {
    if (complete_count == expected_complete_num) {
      LOG(INFO) << "all package send complete, "
          << "exptected: " << expected_complete_num << " "
          << "actual: " << complete_count;
      break;
    }
    retcode ret_code;
    complete_queue.wait_and_pop(ret_code);
    complete_count++;
  } while (true);
  return retcode::SUCCESS;
}

retcode VMNodeImpl::WaitUntilWorkerReady(const std::string& worker_id,
                                         int timeout_ms) {
  SCopedTimer timer;
  auto TASK_INFO_STR = pb_util::TaskInfoToString(worker_id);
  do {
    // try to get lock
    if (IsTaskWorkerReady(worker_id)) {
      break;
    }
    PH_VLOG(5, LogType::kTask)
        << TASK_INFO_STR
        << "sleep and wait for worker ready ........";
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (timeout_ms == -1) {
      continue;
    }
    auto time_elapse_ms = timer.timeElapse();
    if (time_elapse_ms > timeout_ms) {
      PH_LOG(ERROR, LogType::kScheduler)
          << TASK_INFO_STR
          << "wait for worker ready is timeout, wait time(ms): " << timeout_ms;
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
  PH_VLOG(5, LogType::kScheduler)
      << "CleanDuplicateTaskIdFilter time cost(ms): " << time_cost << " "
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
    Node node;
    pbNode2Node(pb_node, &node);
    all_party->push_back(node);
  }
  // auxiliary server
  auto auxiliary_server = task_config.auxiliary_server();
  auto it = auxiliary_server.find(AUX_COMPUTE_NODE);
  if (it != auxiliary_server.end()) {
    auto& pb_node = it->second;
    Node node;
    pbNode2Node(pb_node, &node);
    all_party->push_back(node);
  }
  return retcode::SUCCESS;
}

retcode VMNodeImpl::ExecuteAddTaskOperation(task_manage_t&& task_detail_) {
  auto task_detail = std::move(task_detail_);
  auto& worker_id = std::get<0>(task_detail);
  auto TASK_INFO_STR = pb_util::TaskInfoToString(worker_id);
  auto& task = std::get<1>(task_detail);
  PH_VLOG(7, LogType::kScheduler)
      << TASK_INFO_STR
      << "VMNodeImpl::ManageTaskThread recv task operator ADD";
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
    PH_VLOG(7, LogType::kScheduler)
        << TASK_INFO_STR
        << "ManageTaskThread operator: add worker id: " << worker_id << " "
        << "time cost(ms): " << time_cost;
    write_flag_.store(false);
  } while (0);

  return retcode::SUCCESS;
}

retcode VMNodeImpl::ExecuteDelTaskOperation(task_manage_t&& task_detail_) {
  SCopedTimer timer;
  auto task_detail = std::move(task_detail_);
  write_flag_.store(true);
  std::lock_guard<std::shared_mutex> lck(task_executor_mtx_);
  auto lock_time = timer.timeElapse();
  auto& worker_id = std::get<0>(task_detail);
  auto& task_executor_info = std::get<1>(task_detail);
  auto worker_ptr = std::get<0>(task_executor_info);
  std::string TASK_INFO_STR;
  if (worker_ptr) {
    TASK_INFO_STR = pb_util::TaskInfoToString(worker_ptr->TaskInfo());
  } else {
    TASK_INFO_STR = pb_util::TaskInfoToString(worker_id);
  }
  PH_VLOG(7, LogType::kScheduler)
      << TASK_INFO_STR
      << "VMNodeImpl::ManageTaskThread recv task operator DEL";
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
    PH_VLOG(7, LogType::kScheduler)
        << TASK_INFO_STR
        << "ManageTaskThread operator: erase worker id: " << worker_id << " "
        << "get lock time: " << lock_time << " "
        << "swap queue cost: " << swap_cost << " "
        << "destroy queue cost: " << dctr_cost << " "
        << "total time cost(ms): " << time_cost;
  } else {
    PH_LOG(WARNING, LogType::kScheduler)
        << "worker_id: " << worker_id << " has already been erased";
  }
  write_flag_.store(false);
  return retcode::SUCCESS;
}

retcode VMNodeImpl::ExecuteKillTaskOperation(task_manage_t&& task_detail_) {
  SCopedTimer timer;
  auto task_detail = std::move(task_detail_);
  write_flag_.store(true);
  std::lock_guard<std::shared_mutex> lck(task_executor_mtx_);
  auto lock_time = timer.timeElapse();
  auto& worker_id = std::get<0>(task_detail);
  auto& task_executor_info = std::get<1>(task_detail);
  auto worker_ptr = std::get<0>(task_executor_info);
  std::string TASK_INFO_STR;
  if (worker_ptr) {
    TASK_INFO_STR = pb_util::TaskInfoToString(worker_ptr->TaskInfo());
  } else {
    TASK_INFO_STR = pb_util::TaskInfoToString(worker_id);
  }

  PH_VLOG(7, LogType::kScheduler)
      << TASK_INFO_STR
      << "VMNodeImpl::ManageTaskThread recv task operator KILL";
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
    PH_VLOG(7, LogType::kScheduler)
        << TASK_INFO_STR
        << "ManageTaskThread operator: kill worker id: " << worker_id << " "
        << "get lock time: " << lock_time << " "
        << "swap queue cost: " << swap_cost << " "
        << "destroy queue cost: " << dctr_cost << " "
        << "total time cost(ms): " << time_cost;
  }
  write_flag_.store(false);
  return retcode::SUCCESS;
}
}  // namespace primihub
