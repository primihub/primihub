#include <nlohmann/json.hpp>
#include "test/primihub/algorithm/statistics_util.h"

namespace primihub::test {
using namespace primihub;
std::string BuildTaskDetail(const std::string& function_type,
                    const std::vector<std::string>& party_datasets,
                    const std::vector<std::string>& checked_columns) {
  nlohmann::json js;
  js["type"] = function_type;
  nlohmann::json array = nlohmann::json::array();
  for (const auto& dataset : party_datasets) {
    nlohmann::json item {
      {"resourceId", dataset},
      {"checked", checked_columns}
    };
    array.push_back(item);
  }
  js["features"] = array;
  return js.dump();
}

std::string BuildColumnInfo(std::map<std::string,
                                std::map<std::string, std::string>>& datasets_info,
                            std::map<std::string, int>& column_convert_option) {
  nlohmann::json js;
  for (auto& [dataset_id, new_dataset_info] : datasets_info) {
    nlohmann::json item = {
      {"columns", column_convert_option},
      {"newDataSetId", new_dataset_info["newDataSetId"]},
      {"outputFilePath", new_dataset_info["outputFilePath"]}
    };
    js[dataset_id] = item;
  }
  return js.dump();
}

void registerDataSet(const std::vector<DatasetMetaInfo>& meta_infos,
    std::shared_ptr<primihub::service::DatasetService> service) {
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

void BuildTaskConfig(const std::string& role,
    const std::vector<rpc::Node>& node_list,
    std::map<std::string, std::string>& dataset_list,
    std::map<std::string, std::string>& param_info,
    rpc::Task* task_config) {
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
  task_info->set_task_id("mpc_statistics_avg");
  task_info->set_job_id("statistics_avg_job");
  task_info->set_request_id("statistics_avg_task");
  // datasets
  auto party_datasets = task.mutable_party_datasets();
  auto datasets = (*party_datasets)[role].mutable_data();
  for (const auto& [key, dataset_id] : dataset_list) {
    (*datasets)[key] = dataset_id;
  }
  // param
  auto param_map = task.mutable_params()->mutable_param_map();
  for (const auto& [key, value] : param_info) {
    rpc::ParamValue pv_item;
    pv_item.set_var_type(rpc::VarType::STRING);
    pv_item.set_value_string(value);
    (*param_map)[key] = pv_item;
  }
}

void BuildPartyNodeInfo(std::vector<rpc::Node>* node_list) {
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

  node_list->emplace_back(std::move(node_1));
  node_list->emplace_back(std::move(node_2));
  node_list->emplace_back(std::move(node_3));
}

}  //  namespace primihub::test
