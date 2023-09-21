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

#include "src/primihub/util/network/mem_channel.h"
#include <string_view>
#include <atomic>
#include <functional>
#include <utility>
#include <cstring>

namespace primihub::network {
using retcode = ph_link::retcode;
retcode SimpleMemoryChannel::SendImpl(const std::string& send_buf) {
  auto send_sv = std::string_view(send_buf.data(), send_buf.size());
  return SendImpl(send_sv);
}

retcode SimpleMemoryChannel::SendImpl(const char* buff, size_t size) {
  auto send_sv = std::string_view(buff, size);
  return SendImpl(send_sv);
}

retcode SimpleMemoryChannel::SendImpl(std::string_view send_buff_sv) {
  auto& send_queue = (*storage_)[send_key_];
  send_queue.push(std::string(send_buff_sv.data(), send_buff_sv.size()));

  if (VLOG_IS_ON(8)) {
    std::string send_data;
    for (const auto& ch : send_buff_sv) {
      send_data.append(std::to_string(static_cast<int>(ch))).append(" ");
    }
    LOG(INFO) << "SimpleMemoryChannel::SendImpl "
              << "send_key: " << send_key_ << " "
              << "data size: " << send_buff_sv.size() << " "
              << "send data: [" << send_data << "]";
  }

  return retcode::SUCCESS;
}

retcode SimpleMemoryChannel::RecvImpl(std::string* recv_buf) {
  std::string data_buf;
  auto& recv_queue = (*storage_)[recv_key_];
  recv_queue.wait_and_pop(data_buf);
  *recv_buf = std::move(data_buf);
  if (VLOG_IS_ON(8)) {
    std::string recv_data;
    for (const auto& ch : *recv_buf) {
      recv_data.append(std::to_string(static_cast<int>(ch))).append(" ");
    }
    LOG(INFO) << "SimpleMemoryChannel::RecvImpl "
              << "recv_key: " << recv_key_
              << "data size: " << recv_buf->size() << " "
              << "recv data: " << recv_data;
  }
  return retcode::SUCCESS;
}

retcode SimpleMemoryChannel::RecvImpl(char* recv_buf, size_t recv_size) {
  std::string tmp_recv_buf;
  auto& recv_queue = (*storage_)[recv_key_];
  recv_queue.wait_and_pop(tmp_recv_buf);
  if (tmp_recv_buf.size() != recv_size) {
    LOG(ERROR) << "data length does not match: " << " "
              << "expected: " << recv_size << " "
              << "actually: " << tmp_recv_buf.size();
    return retcode::FAIL;
  }
  memcpy(recv_buf, tmp_recv_buf.data(), recv_size);

  if (VLOG_IS_ON(8)) {
    std::string recv_data;
    for (const auto& ch : tmp_recv_buf) {
      recv_data.append(std::to_string(static_cast<int>(ch))).append(" ");
    }
    LOG(ERROR) << "SimpleMemoryChannel::RecvImpl "
              << "recv_key: " << recv_key_ << " "
              << "data size: " << recv_size << " "
              << "recv data: [" << recv_data << "] ";
  }
  return retcode::SUCCESS;
}

void SimpleMemoryChannel::close() {
}

void SimpleMemoryChannel::cancel() {
  cancel_.store(true);
}
}  // namespace primihub::network
