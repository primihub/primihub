#include "gtest/gtest.h"

#include "src/primihub/algorithm/cryptflow2_maxpool.h"
#include "src/primihub/service/dataset/localkv/storage_default.h"

using namespace primihub;
using namespace primihub::cryptflow2;

static void RunMaxpool(std::string node_id, rpc::Task &task,
                       std::shared_ptr<DatasetService> data_service) {
  PartyConfig config(node_id, task);

  try {
    MaxPoolExecutor exec(config, data_service);
    EXPECT_EQ(exec.loadParams(task), 0);
    EXPECT_EQ(exec.initPartyComm(), 0);
    EXPECT_EQ(exec.loadDataset(), 0);
    EXPECT_EQ(exec.execute(), 0);
    EXPECT_EQ(exec.saveModel(), 0);
    exec.finishPartyComm();
  } catch (const runtime_error &error) {
    std::cerr << "Skip maxpool test because this platform "
                 "don't have avx2 support."
              << std::endl;
  }
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

void BuildTaskConfig(const std::string& role, const std::vector<rpc::Node>& node_list,
    std::vector<std::string>& dataset_list, rpc::Task* task_config) {
//
  auto& task = *task_config;
  task.set_role(role);
  // party access info
  auto party_access_info = task.mutable_party_access_info();
  auto& party0 = (*party_access_info)["PARTY0"];
  auto node = party0.add_node();
  node->CopyFrom(node_list[0]);
  auto& party1 = (*party_access_info)["PARTY1"];
  node = party1.add_node();
  node->CopyFrom(node_list[1]);
  // task info
  auto task_info = task.mutable_task_info();
  task_info->set_task_id("mpc_maxpool");
  task_info->set_job_id("maxpool_job");
  task_info->set_request_id("maxpool_task");
  // datsets
  auto party_datasets = task.mutable_party_datasets();
  auto& datasets = (*party_datasets)[role];
  for (const auto& dataset : dataset_list) {
    datasets.add_item(dataset);
  }
}

TEST(cryptflow2_maxpool, maxpool_2pc_test) {
  uint32_t base_port = 8000;
  // Node 1.
  rpc::Node node_1;
  node_1.set_node_id("node0");
  node_1.set_ip("127.0.0.1");

  rpc::VirtualMachine *vm = node_1.add_vm();
  vm->set_party_id(0);

  rpc::EndPoint *next = vm->mutable_next();

  next->set_ip("127.0.0.1");
  next->set_port(base_port);
  next->set_link_type(rpc::LinkType::SERVER);
  next->set_name("CRYPTFLOW2_Server");

  // Node 2.
  rpc::Node node_2;
  node_2.set_node_id("node1");
  node_2.set_ip("127.0.0.1");

  vm = node_2.add_vm();
  vm->set_party_id(1);

  next = vm->mutable_next();
  next->set_ip("127.0.0.1");
  next->set_port(base_port);
  next->set_name("CRYPTFLOW2_client");
  next->set_link_type(rpc::LinkType::CLIENT);

  std::vector<rpc::Node> node_list;
  node_list.emplace_back(std::move(node_1));
  node_list.emplace_back(std::move(node_2));
  // Construct task for party 0.
  rpc::Task task1;
  std::vector<std::string> party0_datasets{"train_party_0", "test_party_0"};
  BuildTaskConfig("PARTY0", node_list, party0_datasets, &task1);

  // Construct task for party 1.
  rpc::Task task2;
  std::vector<std::string> party1_datasets{"train_party_1", "test_party_0"};
  BuildTaskConfig("PARTY1", node_list, party1_datasets, &task2);

  std::vector<std::string> bootstrap_ids;
  bootstrap_ids.emplace_back("/ip4/172.28.1.13/tcp/4001/ipfs/"
                             "QmP2C45o2vZfy1JXWFZDUEzrQCigMtd4r3nesvArV8dFKd");
  bootstrap_ids.emplace_back("/ip4/172.28.1.13/tcp/4001/ipfs/"
                             "QmdSyhb8eR9dDSR5jjnRoTDBwpBCSAjT7WueKJ9cQArYoA");

  pid_t pid = fork();
  if (pid != 0) {
    // Child process.
    sleep(1);
    auto stub = std::make_shared<p2p::NodeStub>(bootstrap_ids);
    stub->start("/ip4/127.0.0.1/tcp/65533");

    std::shared_ptr<service::DatasetMetaService> meta_service =
	    std::make_shared<service::DatasetMetaService>(
			    stub, std::make_shared<service::StorageBackendDefault>());

    std::shared_ptr<DatasetService> service = std::make_shared<DatasetService>(
        meta_service, "test addr");
    using meta_type_t = std::tuple<std::string, std::string, std::string>;
    std::vector<meta_type_t> meta_infos {
      {"train_party_1", "csv", "data/train_party_0.csv"},
    };
    registerDataSet(meta_infos, service);
    RunMaxpool("node_2", task2, service);
    return;
  }

  // Parent process.
  auto stub = std::make_shared<p2p::NodeStub>(bootstrap_ids);
  stub->start("/ip4/127.0.0.1/tcp/65534");

  std::shared_ptr<service::DatasetMetaService> meta_service =
	  std::make_shared<service::DatasetMetaService>(
			  stub, std::make_shared<service::StorageBackendDefault>());

  std::shared_ptr<DatasetService> service = std::make_shared<DatasetService>(
        meta_service, "test addr");
  using meta_type_t = std::tuple<std::string, std::string, std::string>;
  std::vector<meta_type_t> meta_infos {
    {"train_party_0", "csv", "data/train_party_0.csv"},
  };
  registerDataSet(meta_infos, service);
  RunMaxpool("node_1", task1, service);
  return;
}
