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
#include <glog/logging.h>
#include <Python.h>
#include <iostream>
#include <string>
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/memory/memory.h"
#include "pybind11/stl.h"
#include "pybind11/embed.h"
#include "src/primihub/task_engine/task_executor.h"
#include "src/primihub/node/server_config.h"

ABSL_FLAG(std::string, node_id, "node0", "unique node_id");
ABSL_FLAG(int, task_engine_type, 0, "task engine type, 0: python, 1: other");
ABSL_FLAG(std::string, config_file, "", "server config file");
ABSL_FLAG(std::string, request, "", "task request, serialized by rpc::Task");
ABSL_FLAG(std::string, request_id, "", "task request, serialized by rpc::Task");
ABSL_FLAG(std::string, log_path, "", "log path");
namespace py = pybind11;
int main(int argc, char **argv) {
  py::scoped_interpreter python;
  py::gil_scoped_release release;
  absl::ParseCommandLine(argc, argv);
  std::string node_id = absl::GetFlag(FLAGS_node_id);
  std::string config_file = absl::GetFlag(FLAGS_config_file);
  std::string task_request_str = absl::GetFlag(FLAGS_request);
  std::string request_id = absl::GetFlag(FLAGS_request_id);
  std::string log_path = absl::GetFlag(FLAGS_log_path);
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
  server_cfg.initServerConfig(config_file);
  auto task_engine = std::make_unique<primihub::task_engine::TaskEngine>();
  auto ret = task_engine->Init(node_id, config_file, task_request_str);
  if (ret != primihub::retcode::SUCCESS) {
    LOG(ERROR) << "init py executor failed";
    return -1;
  }
  ret = task_engine->Execute();
  if (ret != primihub::retcode::SUCCESS) {
    LOG(ERROR) << "py executor encoutes error when executing task";
    return -1;
  }
  return 0;
}