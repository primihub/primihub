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
#include "src/primihub/util/util.h"
#include "src/primihub/util/file_util.h"
#include "src/primihub/common/config/config.h"
#include "src/primihub/util/network/link_factory.h"
#include "src/primihub/common/common.h"
#include "src/primihub/util/proto_log_helper.h"
#include "src/primihub/cli/common.h"
namespace pb_util = primihub::proto::util;

namespace primihub {
retcode SDKClient::CheckTaskStauts(const rpc::PushTaskReply& task_reply_info) {
  const auto& task_info = task_reply_info.task_info();
  size_t party_count = task_reply_info.party_count();
  LOG(INFO) << "party count: " << party_count;
  std::map<std::string, std::string> task_status;
  // fecth task status
  size_t fetch_count = 0;
  do {
    rpc::TaskContext request;
    request.CopyFrom(task_info);
    rpc::TaskStatusReply status_reply;
    auto ret = channel_->fetchTaskStatus(request, &status_reply);
    // parse task status
    if (ret != retcode::SUCCESS) {
      LOG(ERROR) << "fetch task status from server failed";
      return retcode::FAIL;
    }
    bool is_finished{false};
    for (const auto& status_info : status_reply.task_status()) {
      auto party = status_info.party();
      auto status_code = status_info.status();
      auto message = status_info.message();
      VLOG(5) << "task_status party: " << party << " "
          << "status: " << static_cast<int>(status_code) << " "
          << "message: " << message;
      if (status_code !=primihub::rpc::TaskStatus::RUNNING) {
        VLOG(0) << pb_util::TaskStatusToString(status_info);
      }
      if (status_info.status() == primihub::rpc::TaskStatus::SUCCESS ||
          status_info.status() == primihub::rpc::TaskStatus::FAIL) {
        if (party == AUX_COMPUTE_NODE) {
        } else {
          task_status[party] = message;
        }
        if (status_info.status() == primihub::rpc::TaskStatus::FAIL) {
          is_finished = true;
        }
      }
      if (task_status.size() == party_count) {
        LOG(INFO) << "all node has finished";
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
    // // for test active kill task
    // {
    //   std::this_thread::sleep_for(std::chrono::seconds(5));
    //   LOG(INFO) << "begin to kill task by client";
    //   rpc::KillTaskRequest request;
    //   auto task_info = request.mutable_task_info();
    //   task_info->set_task_id(task_id);
    //   task_info->set_job_id(job_id);
    //   task_info->set_request_id(request_id);
    //   request.set_executor(rpc::KillTaskRequest::CLIENT);
    //   rpc::KillTaskResponse status_reply;
    //   auto ret = channel_->killTask(request, &status_reply);
    //   break;
    // }

    if (fetch_count < 1000) {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(fetch_count*100));
    } else {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  } while (true);
  return retcode::SUCCESS;
}

retcode SDKClient::RegisterDataset(const rpc::NewDatasetRequest& req,
                                   rpc::NewDatasetResponse* reply) {
  auto ret = channel_->NewDataset(req, reply);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "NewDataset encountes error";
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode SDKClient::SubmitTask(const rpc::PushTaskRequest& task_reqeust,
                              rpc::PushTaskReply* reply) {
  // PushTaskRequest pushTaskRequest;
  PushTaskReply pushTaskReply;
  // BuildTaskRequest(&pushTaskRequest);
  const auto& task_info = task_reqeust.task().task_info();
  LOG(INFO) << "SubmitTask: " << pb_util::TaskInfoToString(task_info);

  // submit task
  auto ret = channel_->submitTask(task_reqeust, &pushTaskReply);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "submitTask encountes error";
    return retcode::FAIL;
  }
  size_t party_count = pushTaskReply.party_count();
  if (party_count < 1) {
    LOG(ERROR) << "party count from reply is: " << party_count << "\n "
               << "message info: " << pushTaskReply.msg_info();
    return retcode::FAIL;
  }
  reply->CopyFrom(pushTaskReply);
  return retcode::SUCCESS;
  // return this->CheckTaskStauts(task_info, party_count);
}

retcode SDKClient::DownloadData(const rpc::TaskContext& task_info,
                                const std::vector<std::string>& file_list,
                                std::vector<std::string>* recv_data) {
  rpc::DownloadRequest request;
  const auto& request_id = task_info.request_id();
  request.set_request_id(request_id);
  for (const auto& file : file_list) {
    request.add_file_list(file);
  }
  return channel_->DownloadData(request, recv_data);
}

retcode SDKClient::SaveData(const std::string& file_name,
                            const std::vector<std::string>& recv_data) {
  auto ret = ValidateDir(file_name);
  if (ret) {
    LOG(ERROR) << "check file path failed: " << file_name;
    return retcode::FAIL;
  }

  std::ofstream fout(file_name, std::ios::binary);
  if (fout) {
    for (const auto& content : recv_data) {
      LOG(INFO) << "content size: " << content.length();
      fout.write(content.c_str(), content.length());
    }
    fout.close();
  } else {
    LOG(ERROR) << "open file: " << file_name << " failed";
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}
}  // namespace primihub
