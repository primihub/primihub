// Copyright [2021] <primihub.com>

#include "gtest/gtest.h"

#include "src/primihub/algorithm/falcon_lenet.h"
#include "src/primihub/service/dataset/meta_service/factory.h"

using namespace primihub;

static void RunFalconlenet(std::string node_id, rpc::Task &task,
                           std::shared_ptr<DatasetService> data_service) {
  PartyConfig config(node_id, task);

  falcon::FalconLenetExecutor exec(config, data_service);
  EXPECT_EQ(exec.loadParams(task), 0);
  EXPECT_EQ(exec.initPartyComm(), 0);
  EXPECT_EQ(exec.loadDataset(), 0);
  EXPECT_EQ(exec.execute(), 0);
  EXPECT_EQ(exec.saveModel(), 0);
  exec.finishPartyComm();
}

void BuildTaskConfig(const std::string& role, const std::vector<rpc::Node>& node_list,
    std::vector<std::string>& dataset_list, rpc::Task* task_config) {
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
  task_info->set_task_id("mpc_lenet");
  task_info->set_job_id("lenet_job");
  task_info->set_request_id("lenet_task");

  // param
  rpc::ParamValue pv_batch_size;
  pv_batch_size.set_var_type(rpc::VarType::INT32);
  pv_batch_size.set_value_int32(128);

  rpc::ParamValue pv_num_iter;
  pv_num_iter.set_var_type(rpc::VarType::INT32);
  pv_num_iter.set_value_int32(1);
  rpc::ParamValue pv_train_data_filepath_self_,pv_train_data_filepath_next_;
  rpc::ParamValue pv_train_label_filepath_self_, pv_train_label_filepath_next_;

  pv_train_data_filepath_self_.set_var_type(rpc::VarType::STRING);
  pv_train_data_filepath_next_.set_var_type(rpc::VarType::STRING);
  pv_train_label_filepath_self_.set_var_type(rpc::VarType::STRING);
  pv_train_label_filepath_next_.set_var_type(rpc::VarType::STRING);
  // self train, self label, remote train, remote label
  pv_train_data_filepath_self_.set_value_string(dataset_list[0]);
  pv_train_label_filepath_self_.set_value_string(dataset_list[1]);
  pv_train_data_filepath_next_.set_value_string(dataset_list[2]);
  pv_train_label_filepath_next_.set_value_string(dataset_list[3]);

  auto param_map = task.mutable_params()->mutable_param_map();
  (*param_map)["Train_Data_Self"] = pv_train_data_filepath_self_;
  (*param_map)["Train_Lable_Self"] = pv_train_label_filepath_self_;
  (*param_map)["Train_Data_Next"] = pv_train_data_filepath_next_;
  (*param_map)["Train_Lable_Next"] = pv_train_label_filepath_next_;
  (*param_map)["NumIters"] = pv_num_iter;
  (*param_map)["BatchSize"] = pv_batch_size;
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

  std::vector<rpc::Node> node_list;
  node_list.emplace_back(std::move(node_1));
  node_list.emplace_back(std::move(node_2));
  node_list.emplace_back(std::move(node_3));
  // Construct task for party 0.
  rpc::Task task1;
  std::vector<std::string> party1_data {
    "data/falcon/dataset/MNIST/train_data_A",
    "data/falcon/dataset/MNIST/train_labels_A",
    "data/falcon/dataset/MNIST/train_data_B",
    "data/falcon/dataset/MNIST/train_labels_B",
  };
  BuildTaskConfig("PARTY0", node_list, party1_data, &task1);

  // Construct task for party 1.
  rpc::Task task2;
  std::vector<std::string> party2_data {
    "data/falcon/dataset/MNIST/train_data_B",
    "data/falcon/dataset/MNIST/train_labels_B",
    "data/falcon/dataset/MNIST/train_data_C",
    "data/falcon/dataset/MNIST/train_labels_C",
  };
  BuildTaskConfig("PARTY1", node_list, party2_data, &task2);

  // Construct task for party 2.
  rpc::Task task3;
  std::vector<std::string> party3_data {
    "data/falcon/dataset/MNIST/train_data_C",
    "data/falcon/dataset/MNIST/train_labels_C",
    "data/falcon/dataset/MNIST/train_data_A",
    "data/falcon/dataset/MNIST/train_labels_A",
  };
  BuildTaskConfig("PARTY2", node_list, party3_data, &task3);

  pid_t pid = fork();
  if (pid != 0) {
    primihub::Node node;
    auto meta_service =
        primihub::service::MetaServiceFactory::Create(
            primihub::service::MetaServiceMode::MODE_MEMORY, node);
    auto service = std::make_shared<service::DatasetService>(std::move(meta_service));
    google::InitGoogleLogging("LENET-Party0");
    RunFalconlenet("node_1", task1, service);
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
    auto service = std::make_shared<service::DatasetService>(std::move(meta_service));

    google::InitGoogleLogging("LENET-party1");
    RunFalconlenet("node_2", task2, service);
    return;
  }

  // Parent process as party 2.
  sleep(3);
  primihub::Node node;
  auto meta_service =
        primihub::service::MetaServiceFactory::Create(
            primihub::service::MetaServiceMode::MODE_MEMORY, node);
  auto service = std::make_shared<service::DatasetService>(std::move(meta_service));


  google::InitGoogleLogging("LENET-party2");
  RunFalconlenet("node_3", task3, service);
  return;
}
