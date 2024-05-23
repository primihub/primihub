/*
* Copyright (c) 2024 by PrimiHub
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
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "src/primihub/cli/task_config_parser.h"
#include "src/primihub/cli/dataset_config_parser.h"
#include "src/primihub/util/network/link_factory.h"
#include "src/primihub/common/common.h"
#include "src/primihub/cli/common.h"

ABSL_FLAG(std::string, server, "127.0.0.1:50050", "server address");
ABSL_FLAG(std::string, type, "", "run task or register dataset(reg)");
ABSL_FLAG(std::string, job_id, "100", "job id");
ABSL_FLAG(std::string, task_id, "200", "task id");
ABSL_FLAG(std::string, task_config_file, "", "path of task config file");
ABSL_FLAG(bool, use_tls, false, "true/false");
ABSL_FLAG(std::vector<std::string>, cert_config,
          std::vector<std::string>({
              "data/cert/ca.crt",
              "data/cert/test.key",
              "data/cert/test.crt"}),
          "cert config");

namespace primihub {
retcode Run() {
  std::string server = absl::GetFlag(FLAGS_server);
  auto cert_config_path = absl::GetFlag(FLAGS_cert_config);
  auto use_tls = absl::GetFlag(FLAGS_use_tls);
  LOG(INFO) << "use tls: " << use_tls;
  auto link_mode = primihub::network::LinkMode::GRPC;
  auto link_ctx = primihub::network::LinkFactory::createLinkContext(link_mode);
  if (use_tls) {
    auto& ca_path = cert_config_path[0];
    auto& key_path = cert_config_path[1];
    auto& cert_path = cert_config_path[2];
    primihub::common::CertificateConfig cert_cfg(ca_path, key_path, cert_path);
    link_ctx->initCertificate(cert_cfg);
  }
  // auto channel = primihub::buildChannel(peer, use_tls, cert_config);
  auto peer_node = Common::GetNode(server, use_tls);
  auto channel = link_ctx->getChannel(peer_node);
  if (channel == nullptr) {
    LOG(ERROR) << "link_ctx->getChannel; failed, "
        << "server info: " << peer_node.to_string();
    return retcode::FAIL;
  }
  SDKClient client(channel, link_ctx.get());
  std::string task_config_file = absl::GetFlag(FLAGS_task_config_file);
  std::string execute_type = absl::GetFlag(FLAGS_type);
  if (execute_type == "reg") {
    LOG(INFO) << "SDK Run Register Dataset Task to: " << server;
    RegisterDataset(task_config_file, client);
  } else {
    LOG(INFO) << "SDK Run SubmitTask to: " << server;
    RunTask(task_config_file, client);
  }
  return retcode::SUCCESS;
}
}  // namespace primihub

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  // Set Color logging on
  FLAGS_colorlogtostderr = true;
  // Set stderr logging on
  FLAGS_alsologtostderr = true;
  // Set log output directory
  // FLAGS_log_dir = "./log/";
  // Set log max size (MB)
  FLAGS_max_log_size = 10;
  // Set stop logging if disk full on
  FLAGS_stop_logging_if_full_disk = true;
  absl::ParseCommandLine(argc, argv);
  primihub::Run();

  return 0;
}