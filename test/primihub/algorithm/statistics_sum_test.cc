// Copyright [2021] <primihub.com>

#include "gtest/gtest.h"
#include <tuple>
#include <vector>
#include <memory>
#include "src/primihub/algorithm/mpc_statistics.h"
#include "test/primihub/algorithm/statistics_util.h"

using namespace primihub;
using namespace primihub::test;

static void RunStatisticsSumTest(std::string node_id, rpc::Task &task,
                        std::shared_ptr<DatasetService> data_service) {
  PartyConfig config(node_id, task);
  MPCStatisticsExecutor exec(config, data_service);
  EXPECT_EQ(exec.loadParams(task), 0);
  EXPECT_EQ(exec.initPartyComm(), 0);
  EXPECT_EQ(exec.loadDataset(), 0);
  EXPECT_EQ(exec.execute(), 0);
  EXPECT_EQ(exec.saveModel(), 0);
  exec.finishPartyComm();
}

TEST(statistics_sum, statistics_sum_test) {
  std::vector<rpc::Node> node_list;
  BuildPartyNodeInfo(&node_list);
  // common param for all party
  std::vector<std::string> party_datasets{
    "avg_test_data_0",
    "avg_test_data_1",
    "avg_test_data_2"
  };
  std::vector<std::string> checked_columns{"x_1", "x_2"};
  std::string fucntion_name = "2";   //  "1为均值, 2为求和, 3为最大值, 4为最小值"
  std::string task_detail = BuildTaskDetail(fucntion_name, party_datasets, checked_columns);
  LOG(INFO) << task_detail;
  std::map<std::string, int> convert_option;
  for (const auto& colum_name : checked_columns) {
    convert_option[colum_name] = 2;
  }
  std::map<std::string, std::map<std::string, std::string>> dataset_info;
  int party_id = 0;
  for (const auto& dataset_id : party_datasets) {
    std::string out_file = "data/result/";
    out_file.append("mpc_sum_party_").append(std::to_string(party_id)).append(".csv");
    std::string new_dataset_id = "new_" + dataset_id;
    dataset_info[dataset_id] = {
      {"outputFilePath", out_file},
      {"newDataSetId", new_dataset_id}
    };
    party_id++;
  }
  std::string column_info = BuildColumnInfo(dataset_info, convert_option);
  LOG(INFO) << column_info;
  std::map<std::string, std::string> params_info = {
    {"ColumnInfo", column_info},
    {"TaskDetail", task_detail}
  };
  // Construct task for party 0.
  rpc::Task task1;
  std::map<std::string, std::string> party0_datasets{
    {"Data_File", party_datasets[0]}};
  BuildTaskConfig("PARTY0", node_list, party0_datasets, params_info, &task1);

  // Construct task for party 1.
  rpc::Task task2;
  std::map<std::string, std::string> party1_datasets{
    {"Data_File", party_datasets[1]}};
  BuildTaskConfig("PARTY1", node_list, party1_datasets, params_info, &task2);

  // Construct task for party 2.
  rpc::Task task3;
  std::map<std::string, std::string> party2_datasets{
    {"Data_File", party_datasets[2]}};
  BuildTaskConfig("PARTY2", node_list, party2_datasets, params_info, &task3);


  pid_t pid = fork();
  if (pid != 0) {
    // Child process as party 0.
    primihub::Node node;
    auto meta_service =
        primihub::service::MetaServiceFactory::Create(
            primihub::service::MetaServiceMode::MODE_MEMORY, node);
    auto service = std::make_shared<DatasetService>(std::move(meta_service));
    std::vector<DatasetMetaInfo> meta_infos {
      {party_datasets[0], "csv", "data/mpc_test.csv"},
    };
    registerDataSet(meta_infos, service);
    LOG(INFO) << "RunStatisticsSumTest(node_1, task1, service);";
    RunStatisticsSumTest("node_1", task1, service);
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
      {party_datasets[1], "csv", "data/mpc_test.csv"},
    };
    registerDataSet(meta_infos, service);
    LOG(INFO) << "RunStatisticsSumTest(node_2, task2, service);";
    RunStatisticsSumTest("node_2", task2, service);
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
    {party_datasets[2], "csv", "data/mpc_test.csv"},
  };
  registerDataSet(meta_infos, service);
  LOG(INFO) << "RunStatisticsSumTest(node_3, task3, service);";
  RunStatisticsSumTest("node_3", task3, service);
  return;
}
