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
  } catch (const runtime_error& error) {
    std::cerr << "Skip maxpool test because this platform "
	         "don't have avx2 support." << std::endl;
  }
}

TEST(cryptflow2_maxpool, maxpool_2pc_test) {
  // Node 1.
  rpc::Node node_1;
  node_1.set_node_id("node0");
  node_1.set_ip("127.0.0.1");

  rpc::VirtualMachine *vm = node_1.add_vm();
  vm->set_party_id(0);

  rpc::EndPoint *next = vm->mutable_next();

  next->set_ip("127.0.0.1");
  next->set_port(8000);
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
  next->set_port(8000);
  next->set_name("CRYPTFLOW2_client");
  next->set_link_type(rpc::LinkType::CLIENT);

  // Construct task for party 0.
  rpc::Task task1;
  {
    auto node_map = task1.mutable_node_map();
    (*node_map)["node_1"] = node_1;
    (*node_map)["node_2"] = node_2;
    task1.set_task_id("mpc_maxpool");
    task1.set_job_id("maxpool_job");

    rpc::ParamValue pv_train_data;
    pv_train_data.set_var_type(rpc::VarType::STRING);
    pv_train_data.set_value_string("data/train_party_0.csv");

    auto param_map = task1.mutable_params()->mutable_param_map();
    (*param_map)["TrainData"] = pv_train_data;
  }

  // Construct task for party 1.
  rpc::Task task2;
  {
    auto node_map = task2.mutable_node_map();
    (*node_map)["node_1"] = node_1;
    (*node_map)["node_2"] = node_2;
    task2.set_task_id("mpc_maxpool");
    task2.set_job_id("maxpool_job");

    rpc::ParamValue pv_train_data;
    pv_train_data.set_var_type(rpc::VarType::STRING);
    pv_train_data.set_value_string("data/train_party_1.csv");

    auto param_map = task2.mutable_params()->mutable_param_map();
    (*param_map)["TrainData"] = pv_train_data;
  }

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
    std::shared_ptr<DatasetService> service = std::make_shared<DatasetService>(
        stub, std::make_shared<service::StorageBackendDefault>());
    
    RunMaxpool("node_2", task2, service);
    return;
  }
  
  // Parent process.
  auto stub = std::make_shared<p2p::NodeStub>(bootstrap_ids);
  stub->start("/ip4/127.0.0.1/tcp/65534");
  std::shared_ptr<DatasetService> service = std::make_shared<DatasetService>(
      stub, std::make_shared<service::StorageBackendDefault>());
  
  RunMaxpool("node_1", task1, service);
  return;
}
