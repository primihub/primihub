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
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <iostream>
#include <string>
#include "src/primihub/task_engine/task_executor.h"
#include "src/primihub/common/config/server_config.h"

DEFINE_string(node_id, "node0", "unique node_id");
DEFINE_int32(task_engine_type, 0, "task engine type, 0: python, 1: other");
DEFINE_string(config_file, "./config/node1.yaml", "server config file");
DEFINE_string(request, "", "task request, serialized by rpc::Task");
DEFINE_string(request_id, "", "task request, serialized by rpc::Task");
DEFINE_string(log_path, "", "log path");

int main(int argc, char **argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  std::string node_id = FLAGS_node_id;
  std::string config_file = FLAGS_config_file;
  std::string task_request_str = FLAGS_request;
  std::string request_id = FLAGS_request_id;
  std::string log_path = FLAGS_log_path;

  google::InitGoogleLogging(request_id.c_str());
  FLAGS_colorlogtostderr = false;
  FLAGS_alsologtostderr = false;
  if (!log_path.empty()) {
    FLAGS_logtostderr = false;
    FLAGS_log_dir = log_path.c_str();
  }

  if (task_request_str.empty()) {
    LOG(ERROR) << "task request empty is not allowed";
    return -1;
  }
  VLOG(0) << "";
  VLOG(0) << "start task process main: " << request_id;
  auto& server_cfg = primihub::ServerConfig::getInstance();
  auto ret = server_cfg.initServerConfig(config_file);
  if (ret != primihub::retcode::SUCCESS) {
    LOG(ERROR) << "init Server config failed";
    return -1;
  }
  auto& service_cfg = server_cfg.getServiceConfig();
  auto task_engine = std::make_unique<primihub::task_engine::TaskEngine>();
  ret = task_engine->Init(service_cfg.id(), config_file, task_request_str);
  if (ret != primihub::retcode::SUCCESS) {
    LOG(ERROR) << "init py executor failed";
    return -1;
  }
  ret = task_engine->Execute();
  if (ret != primihub::retcode::SUCCESS) {
    LOG(ERROR) << "task executor encoutes error when executing task";
    return -1;
  }
  return 0;
}