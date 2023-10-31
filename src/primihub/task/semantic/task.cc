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

#include "src/primihub/task/semantic/task.h"

namespace primihub::task {

TaskBase::TaskBase(const TaskParam* task_config,
                    std::shared_ptr<DatasetService> dataset_service) {
    setTaskParam(*task_config);
    dataset_service_ = dataset_service;
}

TaskParam* TaskBase::getTaskParam()  {
    return &task_param_;
}

void TaskBase::setTaskParam(const TaskParam& task_param) {
  task_param_.CopyFrom(task_param);
  const auto& task_info = task_param.task_info();
  this->party_name_ = task_param.party_name();
  setTaskInfo("", task_info.job_id(), task_info.task_id(),
                task_info.request_id(), task_info.sub_task_id());
}

retcode TaskBase::ExtractProxyNode(const rpc::Task& task_config,
                                   Node* proxy_node) {
  const auto& auxiliary_server = task_config.auxiliary_server();
  auto it = auxiliary_server.find(PROXY_NODE);
  if (it == auxiliary_server.end()) {
    LOG(ERROR) << "no proxy node found";
    return retcode::FAIL;
  }
  const auto& pb_proxy_node = it->second;
  pbNode2Node(pb_proxy_node, proxy_node);
  return retcode::SUCCESS;
}

retcode TaskBase::send(const std::string& key,
                        const Node& dest_node,
                        const std::string& send_buff) {
  auto& link_ctx = this->getTaskContext().getLinkContext();
  CHECK_NULLPOINTER_WITH_ERROR_MSG(link_ctx, "LinkContext is empty");
  auto channel = link_ctx->getChannel(dest_node);
  return channel->send(key, send_buff);
}

retcode TaskBase::send(const std::string& key,
                        const Node& dest_node,
                        std::string_view send_buff) {
  auto& link_ctx = this->getTaskContext().getLinkContext();
  CHECK_NULLPOINTER_WITH_ERROR_MSG(link_ctx, "LinkContext is empty");
  auto channel = link_ctx->getChannel(dest_node);
  return channel->send(key, send_buff);
}

retcode TaskBase::recv(const std::string& key, std::string* recv_buff) {
  auto& link_ctx = this->getTaskContext().getLinkContext();
  CHECK_NULLPOINTER_WITH_ERROR_MSG(link_ctx, "LinkContext is empty");
  auto& recv_queue = link_ctx->GetRecvQueue(key);
  std::string recv_data;
  recv_queue.wait_and_pop(recv_data);
  *recv_buff = std::move(recv_data);
  return retcode::SUCCESS;
}

retcode TaskBase::recv(const std::string& key, char* recv_buff, size_t length) {
    std::string tmp_data;
    auto ret = recv(key, &tmp_data);
    if (ret != retcode::SUCCESS) {
        return retcode::FAIL;
    }
    if (tmp_data.length() != length) {
        LOG(ERROR) << "recv data length does not match, expected: " << length
                << " actually recv data length: " << tmp_data.length();
        return retcode::FAIL;
    }
    memcpy(recv_buff, tmp_data.c_str(), length);
    return retcode::SUCCESS;
}

retcode TaskBase::sendRecv(const std::string& key, const Node& dest_node,
        const std::string& send_buff, std::string* recv_buff) {
    std::string_view send_buff_sv{send_buff.data(), send_buff.length()};
    return sendRecv(key, dest_node, send_buff_sv, recv_buff);
}

retcode TaskBase::sendRecv(const std::string& key, const Node& dest_node,
        std::string_view send_buff, std::string* recv_buff) {
    std::string recv_data_str;
    auto& link_ctx = this->getTaskContext().getLinkContext();
    CHECK_NULLPOINTER_WITH_ERROR_MSG(link_ctx, "LinkContext is empty");
    auto channel = link_ctx->getChannel(dest_node);
    auto ret = channel->sendRecv(key, send_buff, &recv_data_str);
    if (ret != retcode::SUCCESS) {
        return retcode::FAIL;
    }
    *recv_buff = std::move(recv_data_str);
    return retcode::SUCCESS;
}

retcode TaskBase::pushDataToSendQueue(const std::string& key, std::string&& send_data) {
    if (send_data.empty()) {
        LOG(ERROR) << "data can not be empty";
        return retcode::FAIL;
    }
    auto& link_ctx = this->getTaskContext().getLinkContext();
    CHECK_NULLPOINTER_WITH_ERROR_MSG(link_ctx, "LinkContext is empty");
    auto& send_queue = link_ctx->GetSendQueue(key);
    send_queue.push(std::move(send_data));
    auto& complete_queue = link_ctx->GetCompleteQueue(key);
    retcode complete_flag;
    complete_queue.wait_and_pop(complete_flag);
    return retcode::SUCCESS;
}
} // namespace primihub::task
