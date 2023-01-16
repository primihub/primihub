// Copyright [2021] <primihub.com>

#include "gtest/gtest.h"
#include <tuple>
#include <vector>
#include <memory>

#include "src/primihub/algorithm/logistic.h"
#include "src/primihub/service/dataset/localkv/storage_default.h"
#include "src/primihub/data_store/factory.h"
using namespace primihub;

static void RunLogistic(std::string node_id, rpc::Task &task,
                        std::shared_ptr<DatasetService> data_service,
                        std::unique_ptr<LinkContext> &link_context) {
  PartyConfig config(node_id, task);
  LogisticRegressionExecutor exec(config, data_service, link_context);
  EXPECT_EQ(exec.loadParams(task), 0);
  EXPECT_EQ(exec.initPartyComm(), 0);
  EXPECT_EQ(exec.loadDataset(), 0);
  EXPECT_EQ(exec.execute(), 0);
  EXPECT_EQ(exec.saveModel(), 0);
  exec.finishPartyComm();
}
using meta_type_t = std::tuple<std::string, std::string, std::string>;
void registerDataSet(const std::vector<meta_type_t>& meta_infos,
    std::shared_ptr<DatasetService> service) {
  for (auto& meta : meta_infos) {
    auto& dataset_id = std::get<0>(meta);
    auto& dataset_type = std::get<1>(meta);
    auto& dataset_path = std::get<2>(meta);
    auto access_info = service->createAccessInfo(dataset_type, dataset_path);
    auto driver = DataDirverFactory::getDriver(dataset_type, "test addr", std::move(access_info));
    service->registerDriver(dataset_id, driver);
  }
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

  // Construct task for party 0.
  rpc::Task task1;
  auto node_map = task1.mutable_node_map();
  (*node_map)["node_1"] = node_1;
  (*node_map)["node_2"] = node_2;
  (*node_map)["node_3"] = node_3;
  task1.set_task_id("mpc_lr");
  task1.set_job_id("lr_job");

  rpc::ParamValue pv_train_input;
  pv_train_input.set_var_type(rpc::VarType::STRING);
  // pv_train_input.set_value_string("data/train_party_0.csv");
  pv_train_input.set_value_string("train_party_0");

  // rpc::ParamValue pv_test_input;
  // pv_test_input.set_var_type(rpc::VarType::STRING);
  // pv_test_input.set_value_string("data/test_party_0.csv");

  rpc::ParamValue pv_batch_size;
  pv_batch_size.set_var_type(rpc::VarType::INT32);
  pv_batch_size.set_value_int32(128);

  rpc::ParamValue pv_num_iter;
  pv_num_iter.set_var_type(rpc::VarType::INT32);
  pv_num_iter.set_value_int32(100);

  auto param_map = task1.mutable_params()->mutable_param_map();
  (*param_map)["Data_File"] = pv_train_input;
  // (*param_map)["TestData"] = pv_test_input;
  (*param_map)["NumIters"] = pv_num_iter;
  (*param_map)["BatchSize"] = pv_batch_size;

  // Construct task for party 1.
  rpc::Task task2;
  node_map = task2.mutable_node_map();
  (*node_map)["node_1"] = node_1;
  (*node_map)["node_2"] = node_2;
  (*node_map)["node_3"] = node_3;
  task2.set_task_id("mpc_lr");
  task2.set_job_id("lr_job");

  // pv_train_input.set_value_string("data/train_party_1.csv");
  pv_train_input.set_value_string("train_party_1");
  // pv_test_input.set_value_string("data/test_party_1.csv");
  param_map = task2.mutable_params()->mutable_param_map();
  (*param_map)["Data_File"] = pv_train_input;
  // (*param_map)["TestData"] = pv_test_input;
  (*param_map)["NumIters"] = pv_num_iter;
  (*param_map)["BatchSize"] = pv_batch_size;

  // Construct task for party 2.
  rpc::Task task3;
  node_map = task3.mutable_node_map();
  (*node_map)["node_1"] = node_1;
  (*node_map)["node_2"] = node_2;
  (*node_map)["node_3"] = node_3;
  task3.set_task_id("mpc_lr");
  task3.set_job_id("lr_job");

  // pv_train_input.set_value_string("data/train_party_2.csv");
  pv_train_input.set_value_string("train_party_2");
  // pv_test_input.set_value_string("data/test_party_2.csv");
  param_map = task3.mutable_params()->mutable_param_map();
  (*param_map)["Data_File"] = pv_train_input;
  // (*param_map)["TestData"] = pv_test_input;
  (*param_map)["NumIters"] = pv_num_iter;
  (*param_map)["BatchSize"] = pv_batch_size;

  std::vector<std::string> bootstrap_ids;
  bootstrap_ids.emplace_back("/ip4/172.28.1.13/tcp/4001/ipfs/"
                             "QmP2C45o2vZfy1JXWFZDUEzrQCigMtd4r3nesvArV8dFKd");
  bootstrap_ids.emplace_back("/ip4/172.28.1.13/tcp/4001/ipfs/"
                             "QmdSyhb8eR9dDSR5jjnRoTDBwpBCSAjT7WueKJ9cQArYoA");

  pid_t pid = fork();
  if (pid != 0) {
    // Child process as party 0.
    auto stub = std::make_shared<p2p::NodeStub>(bootstrap_ids);
    stub->start("/ip4/127.0.0.1/tcp/65530");

    std::shared_ptr<service::DatasetMetaService> meta_service =
	    std::make_shared<service::DatasetMetaService>(
			    stub, std::make_shared<service::StorageBackendDefault>());

    std::shared_ptr<DatasetService> service = std::make_shared<DatasetService>(
        meta_service, "test addr");

    std::unique_ptr<LinkContext> link_context(nullptr);

    using meta_type_t = std::tuple<std::string, std::string, std::string>;
    std::vector<meta_type_t> meta_infos {
      {"train_party_0", "csv", "data/train_party_0.csv"},
    };
    registerDataSet(meta_infos, service);
    LOG(INFO) << "RunLogistic(node_1, task1, service);";

    RunLogistic("node_1", task1, service, link_context);
    return;
  }

  pid = fork();
  if (pid != 0) {
    // Child process as party 1.
    sleep(1);
    auto stub = std::make_shared<p2p::NodeStub>(bootstrap_ids);
    stub->start("/ip4/127.0.0.1/tcp/65531");

    std::shared_ptr<service::DatasetMetaService> meta_service =
	    std::make_shared<service::DatasetMetaService>(
			    stub, std::make_shared<service::StorageBackendDefault>());

    std::shared_ptr<DatasetService> service = std::make_shared<DatasetService>(
        meta_service, "test addr");

    std::unique_ptr<LinkContext> link_context(nullptr);

    using meta_type_t = std::tuple<std::string, std::string, std::string>;
    std::vector<meta_type_t> meta_infos {
      {"train_party_1", "csv", "data/train_party_1.csv"},
    };
    registerDataSet(meta_infos, service);
    LOG(INFO) << "RunLogistic(node_2, task2, service);";

    RunLogistic("node_2", task2, service, link_context);
    return;
  }

  // Parent process as party 2.
  sleep(3);
  auto stub = std::make_shared<p2p::NodeStub>(bootstrap_ids);
  stub->start("/ip4/127.0.0.1/tcp/65532");

  std::shared_ptr<service::DatasetMetaService> meta_service =
	  std::make_shared<service::DatasetMetaService>(
			  stub, std::make_shared<service::StorageBackendDefault>());

  std::shared_ptr<DatasetService> service = std::make_shared<DatasetService>(
      meta_service, "test addr");

  std::unique_ptr<LinkContext> link_context(nullptr);

  // register dataset
  // dataset_id, dataset_type, dataset_metainfo
  using meta_type_t = std::tuple<std::string, std::string, std::string>;
  std::vector<meta_type_t> meta_infos {
    {"train_party_2", "csv", "data/train_party_2.csv"},
  };
  registerDataSet(meta_infos, service);

  LOG(INFO) << "RunLogistic(node_3, task3, service);";
  RunLogistic("node_3", task3, service, link_context);

  return;
}
