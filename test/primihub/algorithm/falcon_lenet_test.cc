// Copyright [2021] <primihub.com>

#include "gtest/gtest.h"

#include "src/primihub/algorithm/falcon_lenet.h"
#include "src/primihub/service/dataset/localkv/storage_default.h"

using namespace primihub;

static void RunFalconlenet(std::string node_id, rpc::Task &task,
                           std::shared_ptr<DatasetService> data_service)
{
  PartyConfig config(node_id, task);

  falcon::FalconLenetExecutor exec(config, data_service);
  EXPECT_EQ(exec.loadParams(task), 0);
  EXPECT_EQ(exec.initPartyComm(), 0);
  EXPECT_EQ(exec.loadDataset(), 0);
  EXPECT_EQ(exec.execute(), 0);
  EXPECT_EQ(exec.saveModel(), 0);
  exec.finishPartyComm();
}

TEST(falcon, falcon_lenet_test)
{
  rpc::Node node_1;
  node_1.set_node_id("node_1");
  node_1.set_ip("127.0.0.1");

  rpc::VirtualMachine *vm0, *vm1;
  vm0 = node_1.add_vm();
  vm0->set_party_id(0);
  vm1 = node_1.add_vm();
  vm1->set_party_id(0);
  rpc::EndPoint *ep_next = vm0->mutable_next();
  ep_next->set_ip("127.0.0.1");
  ep_next->set_port(32003);
  ep_next->set_link_type(rpc::LinkType::SERVER);
  ep_next->set_name("Falcon_server_0");

  rpc::EndPoint *ep_prev = vm0->mutable_prev();
  ep_prev->set_ip("127.0.0.1");
  ep_prev->set_port(32006);
  ep_prev->set_link_type(rpc::LinkType::CLIENT);
  ep_prev->set_name("Falcon_server_1");

  ep_next = vm1->mutable_next();
  ep_next->set_ip("127.0.0.1");
  ep_next->set_port(32001);
  ep_next->set_link_type(rpc::LinkType::CLIENT);
  ep_next->set_name("Falcon_client_0");

  ep_prev = vm1->mutable_prev();
  ep_prev->set_ip("127.0.0.1");
  ep_prev->set_port(32002);
  ep_prev->set_link_type(rpc::LinkType::CLIENT);
  ep_prev->set_name("Falcon_client_1");

  rpc::Node node_2;
  node_2.set_node_id("node_2");
  node_2.set_ip("127.0.0.1");
  vm0 = node_2.add_vm();
  vm0->set_party_id(1);
  vm1 = node_2.add_vm();
  vm1->set_party_id(1);
  ep_next = vm0->mutable_next();
  ep_next->set_ip("127.0.0.1");
  ep_next->set_port(32001);
  ep_next->set_link_type(rpc::LinkType::SERVER);
  ep_next->set_name("Falcon_server_2");

  ep_prev = vm0->mutable_prev();
  ep_prev->set_ip("127.0.0.1");
  ep_prev->set_port(32007);
  ep_prev->set_link_type(rpc::LinkType::SERVER);
  ep_prev->set_name("Falcon_server_3");

  ep_next = vm1->mutable_next();
  ep_next->set_ip("127.0.0.1");
  ep_next->set_port(32003);
  ep_next->set_link_type(rpc::LinkType::CLIENT);
  ep_next->set_name("Falcon_client_2");

  ep_prev = vm1->mutable_prev();
  ep_prev->set_ip("127.0.0.1");
  ep_prev->set_port(32005);
  ep_prev->set_link_type(rpc::LinkType::CLIENT);
  ep_prev->set_name("Falcon_client_3");

  rpc::Node node_3;
  node_3.set_node_id("node_3");
  node_3.set_ip("127.0.0.1");
  vm0 = node_3.add_vm();
  vm0->set_party_id(2);
  vm1 = node_3.add_vm();
  vm1->set_party_id(2);
  ep_next = vm0->mutable_next();
  ep_next->set_ip("127.0.0.1");
  ep_next->set_port(32002);
  ep_next->set_link_type(rpc::LinkType::SERVER);
  ep_next->set_name("Falcon_server_4");

  ep_prev = vm0->mutable_prev();
  ep_prev->set_ip("127.0.0.1");
  ep_prev->set_port(32005);
  ep_prev->set_link_type(rpc::LinkType::SERVER);
  ep_prev->set_name("Falcon_server_5");

  ep_next = vm1->mutable_next();
  ep_next->set_ip("127.0.0.1");
  ep_next->set_port(32006);
  ep_next->set_link_type(rpc::LinkType::CLIENT);
  ep_next->set_name("Falcon_client_4");

  ep_prev = vm1->mutable_prev();
  ep_prev->set_ip("127.0.0.1");
  ep_prev->set_port(32007);
  ep_prev->set_link_type(rpc::LinkType::CLIENT);
  ep_prev->set_name("Falcon_client_4");

  // Construct task for party 0.
  rpc::Task task1;
  auto node_map = task1.mutable_node_map();
  (*node_map)["node_1"] = node_1;
  (*node_map)["node_2"] = node_2;
  (*node_map)["node_3"] = node_3;
  task1.set_task_id("mpc_lenet"); //
  task1.set_job_id("lenet_job");  //

  rpc::ParamValue pv_train_data_filepath_self_,pv_train_data_filepath_next_;
  rpc::ParamValue pv_train_label_filepath_self_, pv_train_label_filepath_next_;

  pv_train_data_filepath_self_.set_var_type(rpc::VarType::STRING);
  pv_train_data_filepath_next_.set_var_type(rpc::VarType::STRING);
  pv_train_label_filepath_self_.set_var_type(rpc::VarType::STRING);
  pv_train_label_filepath_next_.set_var_type(rpc::VarType::STRING);

  pv_train_data_filepath_self_.set_value_string("data/falcon/dataset/MNIST/train_data_A");
  pv_train_label_filepath_self_.set_value_string("data/falcon/dataset/MNIST/train_labels_A");
  pv_train_data_filepath_next_.set_value_string("data/falcon/dataset/MNIST/train_data_B");
  pv_train_label_filepath_next_.set_value_string("data/falcon/dataset/MNIST/train_labels_B");

  rpc::ParamValue pv_batch_size;
  pv_batch_size.set_var_type(rpc::VarType::INT32);
  pv_batch_size.set_value_int32(128);

  rpc::ParamValue pv_num_iter;
  pv_num_iter.set_var_type(rpc::VarType::INT32);
  pv_num_iter.set_value_int32(1);

  auto param_map = task1.mutable_params()->mutable_param_map();
  (*param_map)["Train_Data_Self"] = pv_train_data_filepath_self_;
  (*param_map)["Train_Lable_Self"] = pv_train_label_filepath_self_;
  (*param_map)["Train_Data_Next"] = pv_train_data_filepath_next_;
  (*param_map)["Train_Lable_Next"] = pv_train_label_filepath_next_;
  (*param_map)["NumIters"] = pv_num_iter;
  (*param_map)["BatchSize"] = pv_batch_size;

  // Construct task for party 1.
  rpc::Task task2;
  node_map = task2.mutable_node_map();
  (*node_map)["node_1"] = node_1;
  (*node_map)["node_2"] = node_2;
  (*node_map)["node_3"] = node_3;
  task2.set_task_id("mpc_lenet");
  task2.set_job_id("lenet_job");

  pv_train_data_filepath_self_.set_value_string("data/falcon/dataset/MNIST/train_data_B" );
  pv_train_label_filepath_self_.set_value_string("data/falcon/dataset/MNIST/train_labels_B");
  pv_train_data_filepath_next_.set_value_string("data/falcon/dataset/MNIST/train_data_C");
  pv_train_label_filepath_next_.set_value_string("data/falcon/dataset/MNIST/train_labels_C");

  param_map = task2.mutable_params()->mutable_param_map();
  (*param_map)["Train_Data_Self"] = pv_train_data_filepath_self_;
  (*param_map)["Train_Lable_Self"] = pv_train_label_filepath_self_;
  (*param_map)["Train_Data_Next"] = pv_train_data_filepath_next_;
  (*param_map)["Train_Lable_Next"] = pv_train_label_filepath_next_;
  (*param_map)["NumIters"] = pv_num_iter;
  (*param_map)["BatchSize"] = pv_batch_size;

  // Construct task for party 2.
  rpc::Task task3;
  node_map = task3.mutable_node_map();
  (*node_map)["node_1"] = node_1;
  (*node_map)["node_2"] = node_2;
  (*node_map)["node_3"] = node_3;
  task3.set_task_id("mpc_lenet");
  task3.set_job_id("lenet_job");

  pv_train_data_filepath_self_.set_value_string("data/falcon/dataset/MNIST/train_data_C" );
  pv_train_label_filepath_self_.set_value_string("data/falcon/dataset/MNIST/train_labels_C" );
  pv_train_data_filepath_next_.set_value_string("data/falcon/dataset/MNIST/train_data_A");
  pv_train_label_filepath_next_.set_value_string("data/falcon/dataset/MNIST/train_labels_A");

  param_map = task3.mutable_params()->mutable_param_map();
  (*param_map)["Train_Data_Self"] = pv_train_data_filepath_self_;
  (*param_map)["Train_Lable_Self"] = pv_train_label_filepath_self_;
  (*param_map)["Train_Data_Next"] = pv_train_data_filepath_next_;
  (*param_map)["Train_Lable_Next"] = pv_train_label_filepath_next_;
  (*param_map)["NumIters"] = pv_num_iter;
  (*param_map)["BatchSize"] = pv_batch_size;

  std::vector<std::string> bootstrap_ids;
  bootstrap_ids.emplace_back("/ip4/172.28.1.13/tcp/4001/ipfs/"
                             "QmP2C45o2vZfy1JXWFZDUEzrQCigMtd4r3nesvArV8dFKd");
  bootstrap_ids.emplace_back("/ip4/172.28.1.13/tcp/4001/ipfs/"
                             "QmdSyhb8eR9dDSR5jjnRoTDBwpBCSAjT7WueKJ9cQArYoA");

  pid_t pid = fork();
  if (pid != 0)
  {
    // Child process as party 0.
    auto stub = std::make_shared<p2p::NodeStub>(bootstrap_ids);
    stub->start("/ip4/127.0.0.1/tcp/11050");
    std::shared_ptr<DatasetService> service = std::make_shared<DatasetService>(
        stub, std::make_shared<service::StorageBackendDefault>());

    google::InitGoogleLogging("LENET-Party0");
    RunFalconlenet("node_1", task1, service);
    return;
  }

  pid = fork();
  if (pid != 0)
  {
    // Child process as party 1.
    sleep(1);
    auto stub = std::make_shared<p2p::NodeStub>(bootstrap_ids);
    stub->start("/ip4/127.0.0.1/tcp/11060");
    std::shared_ptr<DatasetService> service = std::make_shared<DatasetService>(
        stub, std::make_shared<service::StorageBackendDefault>());

    google::InitGoogleLogging("LENET-party1");
    RunFalconlenet("node_2", task2, service);
    return;
  }

  // Parent process as party 2.
  sleep(3);
  auto stub = std::make_shared<p2p::NodeStub>(bootstrap_ids);
  stub->start("/ip4/127.0.0.1/tcp/11070");
  std::shared_ptr<DatasetService> service = std::make_shared<DatasetService>(
      stub, std::make_shared<service::StorageBackendDefault>());

  google::InitGoogleLogging("LENET-party2");
  RunFalconlenet("node_3", task3, service);
  return;
}
