// Copyright [2021] <primihub.com>

#include "gtest/gtest.h"
#include <tuple>
#include <vector>
#include <memory>

#include "src/primihub/common/common.h"
#include "src/primihub/algorithm/logistic.h"
#include "src/primihub/data_store/factory.h"
#include "src/primihub/service/dataset/meta_service/factory.h"
using namespace primihub;

static void RunLogistic(std::string node_id, rpc::Task &task,
                        std::shared_ptr<DatasetService> data_service) {
  PartyConfig config(node_id, task);
  LogisticRegressionExecutor exec(config, data_service);
  EXPECT_EQ(exec.loadParams(task), 0);
  EXPECT_EQ(exec.initPartyComm(), 0);
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
  // datsets
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
}

TEST(logistic, logistic_3pc_test) {
  uint32_t base_port = 8000;
  rpc::Node node_1;
  node_1.set_node_id("node_1");
  node_1.set_ip("127.0.0.1");

  rpc::VirtualMachine *vm = node_1.add_vm();
  vm->set_party_id(0);

  rpc::EndPoint *next = vm->mutable_next();
  rpc::EndPoint *prev = vm->mutable_prev();
  next->set_ip("127.0.0.1");
  next->set_port(base_port);
  prev->set_ip("127.0.0.1");
  prev->set_port(base_port+100);

  rpc::Node node_2;
  node_2.set_node_id("node_2");
  node_2.set_ip("127.0.0.1");

  vm = node_2.add_vm();
  vm->set_party_id(1);

  next = vm->mutable_next();
  prev = vm->mutable_prev();
  next->set_ip("127.0.0.1");
  next->set_port(base_port+200);
  prev->set_ip("127.0.0.1");
  prev->set_port(base_port);

  rpc::Node node_3;
  node_3.set_node_id("node_3");
  node_3.set_ip("127.0.0.1");

  vm = node_3.add_vm();
  vm->set_party_id(2);

  next = vm->mutable_next();
  prev = vm->mutable_prev();
  next->set_ip("127.0.0.1");
  next->set_port(base_port+100);
  prev->set_ip("127.0.0.1");
  prev->set_port(base_port+200);

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

  std::vector<std::string> bootstrap_ids;
  bootstrap_ids.emplace_back("/ip4/172.28.1.13/tcp/4001/ipfs/"
                             "QmP2C45o2vZfy1JXWFZDUEzrQCigMtd4r3nesvArV8dFKd");
  bootstrap_ids.emplace_back("/ip4/172.28.1.13/tcp/4001/ipfs/"
                             "QmdSyhb8eR9dDSR5jjnRoTDBwpBCSAjT7WueKJ9cQArYoA");

  pid_t pid = fork();
  if (pid != 0) {
    // Child process as party 0.
    primihub::Node node;
    auto meta_service =
        primihub::service::MetaServiceFactory::Create(
            primihub::service::MetaServiceMode::MODE_MEMORY, node);
    auto service = std::make_shared<DatasetService>(std::move(meta_service));
    std::vector<DatasetMetaInfo> meta_infos {
      {"train_party_0", "csv", "data/train_party_0.csv"},
    };
    registerDataSet(meta_infos, service);
    LOG(INFO) << "RunLogistic(node_1, task1, service);";
    RunLogistic("node_1", task1, service);
    return;
  }

  pid = fork();
  if (pid != 0) {
    // Child process as party 1.
    sleep(1);
    primihub::Node node;
    auto meta_service =
        primihub::service::MetaServiceFactory::Create(
            primihub::service::MetaServiceMode::MODE_MEMORY, node);
    auto service = std::make_shared<DatasetService>(std::move(meta_service));
    std::vector<DatasetMetaInfo> meta_infos {
      {"train_party_1", "csv", "data/train_party_1.csv"},
    };
    registerDataSet(meta_infos, service);
    LOG(INFO) << "RunLogistic(node_2, task2, service);";
    RunLogistic("node_2", task2, service);
    return;
  }

  // Parent process as party 2.
  sleep(3);
  primihub::Node node;
  auto meta_service =
        primihub::service::MetaServiceFactory::Create(
            primihub::service::MetaServiceMode::MODE_MEMORY, node);
  auto service = std::make_shared<DatasetService>(std::move(meta_service));
  // register dataset
  std::vector<DatasetMetaInfo> meta_infos {
    {"train_party_2", "csv", "data/train_party_2.csv"},
  };
  registerDataSet(meta_infos, service);
  LOG(INFO) << "RunLogistic(node_3, task3, service);";
  RunLogistic("node_3", task3, service);
  return;
}
