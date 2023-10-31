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

#include "src/primihub/cli/reg_cli.h"
#include <fstream>  // std::ifstream
#include <string>
#include <chrono>
#include <random>
#include <vector>
#include <future>
#include <thread>
#include "uuid.h"
#include <nlohmann/json.hpp>
#include "src/primihub/common/common.h"

ABSL_FLAG(std::string, server, "127.0.0.1:50050", "server address");
ABSL_FLAG(std::string, config_file, "", "register dataset config file");
namespace primihub {
namespace client {
int getFileContents(const std::string& fpath, std::string* contents) {
    std::ifstream f_in_stream(fpath);
    std::string contents_((std::istreambuf_iterator<char>(f_in_stream)),
                        std::istreambuf_iterator<char>());
    *contents = std::move(contents_);
    return 0;
}
}  // namespace client
int RegClient::RegDataSet(const std::string& dataset_id,
                          const std::string& data_type,
                          const std::string& access_info) {
  NewDatasetRequest request;
  NewDatasetResponse response;
  grpc::ClientContext context;
  LOG(INFO) << "dataset_id: " << dataset_id;
  request.set_op_type(NewDatasetRequest::REGISTER);
  auto meta_info = request.mutable_meta_info();
  meta_info->set_id(dataset_id);
  meta_info->set_driver(data_type);
  meta_info->set_access_info(access_info);
  meta_info->set_visibility(rpc::MetaInfo::PUBLIC);

  LOG(INFO) << "Register Dataset for dataset_id: " << dataset_id << " "
          << "data_type: " <<  data_type << " "
          << "access_info: " << access_info;

  auto status = stub_->NewDataset(&context, request, &response);
  if (status.ok()) {
    LOG(INFO) << "NewDataset rpc succeeded.";
    if (response.ret_code() == rpc::GetDatasetResponse::SUCCESS) {
      LOG(INFO) << "dataset_url: " << response.dataset_url() << " success";
    } else {
      LOG(ERROR) << "dataset_url: " << response.dataset_url() << " failed, "
          << "error info: " << response.ret_msg();
    }
  } else {
    LOG(ERROR) << "ERROR: " << status.error_message() << "NewDataset rpc failed.";
    return -1;
  }
  return 0;
}

retcode ParseDatasetMetaInfo(const std::string& config_file,
    std::vector<std::tuple<std::string, std::string, std::string>>* meta_list) {
  meta_list->clear();
  std::string task_config_content;
  try {
    primihub::client::getFileContents(config_file, &task_config_content);
    if (task_config_content.empty()) {
      LOG(ERROR) << "config file is empty";
      return retcode::FAIL;
    }
    auto meta_info_list = nlohmann::json::parse(task_config_content);
    for (const auto& meta_info : meta_info_list) {
      auto id = meta_info["id"].get<std::string>();
      auto type = meta_info["type"].get<std::string>();
      std::string access_info;
      if (meta_info["access_info"].is_string()) {
        access_info = meta_info["access_info"].get<std::string>();
      } else {
        access_info = meta_info["access_info"].dump();
      }
      meta_list->emplace_back(std::make_tuple(id, type, access_info));
    }
  } catch (std::exception& e) {
    LOG(ERROR) << "parse dataset meta info failed: " << e.what();
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

void Usage() {
  std::string usage_info;
  usage_info.append("Usage:\n");
  usage_info.append("reg_cli --server=ip:port --config_file=dataset_meta_config_file\n");
  usage_info.append("--server:  [opttion default: 127.0.0.1:50050]\n");
  usage_info.append("--config_file: path of dataset meta config\n");
  usage_info.append("config file is json format, example is as follows ")
            .append("for different source dataset:\n");
  usage_info.append(R"""(
[
  {
    "id": "psi_client_data",
    "type:": "csv",
    "access_info": "data/client_e.csv",
  },
  {
    "id": "psi_client_data_db",
    "type:": "sqlite",
    "access_info": {
      "tableName": "psi_client_data",
      "db_path": "data/client_e.db3"
    },
  }
  {
    "id": "psi_client_data_mysql",
    "type": "mysql",
    "access_info": {
      "password": "primihub@123",
      "database": "privacy_test1",
      "port": 30306,
      "dbName": "privacy_test1",
      "host": "192.168.99.13",
      "type": "mysql",
      "username": "primihub",
      "tableName": "sys_user"
    }
  }
]
)""");
  LOG(INFO) << usage_info;
}

}  // namespace primihub

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  // Set Color logging on
  FLAGS_colorlogtostderr = true;
  // Set stderr logging on
  FLAGS_alsologtostderr = true;
  // Set stop logging if disk full on
  FLAGS_stop_logging_if_full_disk = true;

  absl::ParseCommandLine(argc, argv);

  std::vector<std::string> peers;
  peers.push_back(absl::GetFlag(FLAGS_server));
  std::string config_file = absl::GetFlag(FLAGS_config_file);
  if (config_file.empty()) {
    LOG(ERROR) << "no config file is specified";
    primihub::Usage();
    return -1;
  }
  std::vector<std::tuple<std::string, std::string, std::string>> meta_info;
  auto ret = primihub::ParseDatasetMetaInfo(config_file, &meta_info);
  if (ret != primihub::retcode::SUCCESS) {
    LOG(ERROR) << "ParseDatasetMetaInfo failed";
    return -1;
  }
  // std::vector<std::tuple<std::string, std::string, std::string>> meta_info = {
  //     {"psi_client_data", "csv", "data/client_e.csv"},
  //     {"psi_client_data_db", "sqlite",
  //       R"""({
  //         "tableName": "psi_client_data",
  //         "db_path": "data/client_e.db3"})"""},
  //     {"psi_client_data_mysql", "mysql",
  //         R"""({
  //             "password": "primihub@123",
  //             "database": "privacy_test1",
  //             "port": 30306,
  //             "dbName": "privacy_test1",
  //             "host": "192.168.99.13",
  //             "type": "mysql",
  //             "username": "primihub",
  //             "tableName": "sys_user"}
  //             )"""},
  // };

  for (auto peer : peers) {
    LOG(INFO) << "CLI Register Dataset to: " << peer;
    auto channel = grpc::CreateChannel(peer, grpc::InsecureChannelCredentials());
    primihub::RegClient client(channel);
    auto _start = std::chrono::high_resolution_clock::now();
    for (const auto meta : meta_info) {
      auto& fid = std::get<0>(meta);
      auto& data_type = std::get<1>(meta);
      auto& access_info = std::get<2>(meta);
      auto ret = client.RegDataSet(fid, data_type, access_info);
      if (ret) {
        LOG(ERROR) << "RegDataSet: " << fid << " data type: " <<  data_type << " "
                << "meta_info: [" << access_info << "] failed";
      }
    }
    auto _end = std::chrono::high_resolution_clock::now();
    auto time_cost = std::chrono::duration_cast<std::chrono::milliseconds>(_end - _start).count();
    LOG(INFO) << "Register dataset time cost(ms): " << time_cost;
    break;
  }

  return 0;
}
