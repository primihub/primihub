// Copyright [2021] <primihub.com>

#include "gtest/gtest.h"
#include <tuple>
#include <vector>
#include <memory>

#include "src/primihub/common/common.h"
#include "src/primihub/algorithm/logistic.h"
#include "src/primihub/data_store/factory.h"
#include "src/primihub/service/dataset/meta_service/factory.h"
#include "network/channel_interface.h"
#include "src/primihub/util/network/mem_channel.h"
#include "src/primihub/common/party_config.h"
#include "src/primihub/algorithm/base.h"
#include "src/primihub/util/network/mem_channel.h"
using namespace primihub;
namespace ph_link = primihub::link;
using StorageType = primihub::network::StorageType;
static void RunLogistic(std::string node_id, rpc::Task& task,
                        std::shared_ptr<DatasetService> data_service,
                        const std::vector<ph_link::Channel>& channels) {
  PartyConfig config(node_id, task);
  LogisticRegressionExecutor exec(config, data_service);
  EXPECT_EQ(exec.loadParams(task), 0);
  EXPECT_EQ(exec.initPartyComm(channels), 0);
  EXPECT_EQ(exec.InitEngine(), retcode::SUCCESS);
  EXPECT_EQ(exec.loadDataset(), 0);
  EXPECT_EQ(exec.execute(), 0);
  EXPECT_EQ(exec.saveModel(), 0);
  exec.finishPartyComm();
}

void registerDataSet(const std::vector<DatasetMetaInfo>& meta_infos,
    std::shared_ptr<DatasetService> service) {
  for (auto& meta : meta_infos) {
    auto& dataset_id = meta.id;
    auto& dataset_type = meta.driver_type;
    auto access_info = service->createAccessInfo(dataset_type, meta);
    std::string access_meta = access_info->toString();
    auto driver = DataDirverFactory::getDriver(dataset_type, "test addr", std::move(access_info));
    service->registerDriver(dataset_id, driver);
    service::DatasetMeta meta_;
    service->newDataset(driver, dataset_id, access_meta, &meta_);
  }
}

void BuildTaskConfig(const std::string& role, const std::vector<rpc::Node>& node_list,
    std::map<std::string, std::string>& dataset_list, rpc::Task* task_config) {
//
  auto& task = *task_config;
  task.set_party_name(role);
  // party access info
  auto party_access_info = task.mutable_party_access_info();
  auto& party0 = (*party_access_info)["PARTY0"];
  party0.CopyFrom(node_list[0]);
  auto& party1 = (*party_access_info)["PARTY1"];
  party1.CopyFrom(node_list[1]);
  auto& party2 = (*party_access_info)["PARTY2"];
  party2.CopyFrom(node_list[2]);
  // task info
  auto task_info = task.mutable_task_info();
  task_info->set_task_id("mpc_lr");
  task_info->set_job_id("lr_job");
  task_info->set_request_id("lr_task");
  // datasets
  auto party_datasets = task.mutable_party_datasets();
  auto datasets = (*party_datasets)[role].mutable_data();
  for (const auto& [key, dataset_id] : dataset_list) {
    (*datasets)[key] = dataset_id;
  }
  auto auxiliary_server = task.mutable_auxiliary_server();
  rpc::Node fake_proxy_node;
  fake_proxy_node.set_ip("127.0.0.1");
  fake_proxy_node.set_port(50050);
  fake_proxy_node.set_use_tls(false);
  (*auxiliary_server)[PROXY_NODE] = std::move(fake_proxy_node);
  // param
  rpc::ParamValue pv_batch_size;
  pv_batch_size.set_var_type(rpc::VarType::INT32);
  pv_batch_size.set_value_int32(128);

  rpc::ParamValue pv_num_iter;
  pv_num_iter.set_var_type(rpc::VarType::INT32);
  pv_num_iter.set_value_int32(100);

  auto param_map = task.mutable_params()->mutable_param_map();
  (*param_map)["NumIters"] = pv_num_iter;
  (*param_map)["BatchSize"] = pv_batch_size;
  rpc::ParamValue pv_columns_exclude;
  std::vector<std::string> columns_exclude{"ID"};
  pv_columns_exclude.set_var_type(rpc::VarType::STRING);
  pv_columns_exclude.set_is_array(true);
  auto array_ptr = pv_columns_exclude.mutable_value_string_array();
  for (const auto& item : columns_exclude) {
    array_ptr->add_value_string_array(item);
  }
  (*param_map)["ColumnsExclude"] = std::move(pv_columns_exclude);
}
void RunTest(rpc::Task& task_config,
             std::vector<DatasetMetaInfo>& meta_info,
             std::shared_ptr<StorageType> storage) {
  std::string party_name = task_config.party_name();
  primihub::Node node;
  auto meta_service =
      primihub::service::MetaServiceFactory::Create(
          primihub::service::MetaServiceMode::MODE_MEMORY, node);
  auto service = std::make_shared<DatasetService>(std::move(meta_service));
  // create channel
  primihub::PartyConfig party_config("default", task_config);
  primihub::ABY3PartyConfig aby3_party_config(party_config);;
  const auto& task_info = task_config.task_info();
  std::string job_id = task_info.job_id();
  std::string task_id = task_info.task_id();
  std::string request_id = task_info.request_id();
  uint16_t party_id = aby3_party_config.SelfPartyId();
  uint16_t prev_party_id = aby3_party_config.PrevPartyId();
  uint16_t next_party_id = aby3_party_config.NextPartyId();
  // construct channel for communication to next party
  std::string party_name_next = aby3_party_config.NextPartyName();
  auto party_node_next = aby3_party_config.NextPartyInfo();

  // construct channel for communication to prev party
  std::string party_name_prev = aby3_party_config.PrevPartyName();
  auto party_node_prev = aby3_party_config.PrevPartyInfo();


  LOG(INFO) << "local_id_local_id_: " << party_id;
  LOG(INFO) << "next_party: " << party_name_next
            << " detail: " << party_node_next.to_string();
  LOG(INFO) << "prev_party: " << party_name_prev
            << " detail: " << party_node_prev.to_string();
  LOG(INFO) << "job_id: " << job_id << " task_id: "
            << task_id << " request id: " << request_id;
  auto channel_impl_prev =
      std::make_shared<network::SimpleMemoryChannel>(
          job_id, task_id, request_id,
          party_name, party_name_prev, storage);

  auto channel_impl_next =
      std::make_shared<network::SimpleMemoryChannel>(
          job_id, task_id, request_id,
          party_name, party_name_next, storage);

  ph_link::Channel chl_prev(channel_impl_prev);
  ph_link::Channel chl_next(channel_impl_next);

  std::vector<ph_link::Channel> channels;
  channels.push_back(chl_prev);
  channels.push_back(chl_next);
  registerDataSet(meta_info, service);
  LOG(INFO) << "RunLogistic(" << party_name << ", task_config, service);";
  RunLogistic(party_name, task_config, service, channels);
}

TEST(logistic, logistic_3pc_test) {
  rpc::Node node_1;
  node_1.set_node_id("node_1");
  node_1.set_ip("127.0.0.1");
  node_1.set_party_id(0);

  rpc::Node node_2;
  node_2.set_node_id("node_2");
  node_2.set_ip("127.0.0.1");
  node_2.set_party_id(1);

  rpc::Node node_3;
  node_3.set_node_id("node_3");
  node_3.set_ip("127.0.0.1");
  node_3.set_party_id(2);

  std::vector<rpc::Node> node_list;
  node_list.emplace_back(std::move(node_1));
  node_list.emplace_back(std::move(node_2));
  node_list.emplace_back(std::move(node_3));
  // Construct task for party 0.
  rpc::Task task1;
  std::map<std::string, std::string> party0_datasets{
    {"Data_File", "train_party_0"},
    {"TestData", "test_party_0"}};
  BuildTaskConfig("PARTY0", node_list, party0_datasets, &task1);

  // Construct task for party 1.
  rpc::Task task2;
  std::map<std::string, std::string> party1_datasets{
    {"Data_File", "train_party_1"},
    {"TestData", "test_party_1"}};
  BuildTaskConfig("PARTY1", node_list, party1_datasets, &task2);

  // Construct task for party 2.
  rpc::Task task3;
  std::map<std::string, std::string> party2_datasets{
    {"Data_File", "train_party_2"},
    {"TestData", "test_party_2"}};
  BuildTaskConfig("PARTY2", node_list, party2_datasets, &task3);

  // create channel
  std::vector<DatasetMetaInfo> party0_meta_infos {
    {"train_party_0", "csv", "data/train_party_0.csv"},
  };
  std::vector<DatasetMetaInfo> party1_meta_infos {
    {"train_party_1", "csv", "data/train_party_1.csv"},
  };
  std::vector<DatasetMetaInfo> party2_meta_infos {
    {"train_party_2", "csv", "data/train_party_2.csv"},
  };
  auto g_storage = std::make_shared<primihub::network::StorageType>();
  auto party_0_fut = std::async(
    std::launch::async,
    RunTest,
    std::ref(task1),
    std::ref(party0_meta_infos),
    g_storage);
  auto party_1_fut = std::async(
    std::launch::async,
    RunTest,
    std::ref(task2),
    std::ref(party1_meta_infos),
    g_storage);
  auto party_2_fut = std::async(
    std::launch::async,
    RunTest,
    std::ref(task3),
    std::ref(party2_meta_infos),
    g_storage);
  party_0_fut.get();
  party_1_fut.get();
  party_2_fut.get();
  return;
}
