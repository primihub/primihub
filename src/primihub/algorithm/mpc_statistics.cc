// "Copyright [2023] <Primihub>"
#include "src/primihub/algorithm/mpc_statistics.h"
#include <arrow/api.h>
#include <arrow/csv/api.h>
#include <arrow/csv/writer.h>
#include <arrow/io/api.h>
#include <arrow/result.h>
#include <arrow/status.h>
#include <arrow/table.h>
#include <rapidjson/document.h>

#include <algorithm>
#include <utility>

#include "src/primihub/common/common.h"
#include "src/primihub/util/file_util.h"
#include "src/primihub/util/network/message_interface.h"

using namespace rapidjson;
// using primihub::columnDtypeToString;

namespace primihub {
MPCStatisticsExecutor::MPCStatisticsExecutor(
    PartyConfig &config, std::shared_ptr<DatasetService> dataset_service)
    : AlgorithmBase(dataset_service) {
  party_config_.Init(config);
  // party_id_ = party_config_.SelfPartyId();
  this->set_party_name(party_config_.SelfPartyName());
  this->set_party_id(party_config_.SelfPartyId());
  // node_id_ = config.node_id;
  // job_id_ = config.job_id;
  // task_id_ = config.task_id;
  // // Save all party's node config.
  // const auto &node_map = config.node_map;
  // for (auto iter = node_map.begin(); iter != node_map.end(); iter++) {
  //   const rpc::Node &node = iter->second;
  //   uint16_t party_id = static_cast<uint16_t>(node.vm(0).party_id());
  //   node_map_[party_id] =
  //       primihub::Node(node.node_id(), node.ip(), node.port(), node.use_tls());
  //   node_map_[party_id].id_ = node.node_id();

  //   LOG(INFO) << "Import node: party id " << party_id << ", node "
  //             << node_map_[party_id].to_string() << ".";
  // }

  // // Find config of local node then fill local node config.
  // auto iter = node_map.find(config.node_id);
  // if (iter == node_map.end()) {
  //   std::stringstream ss;
  //   ss << "Can't find node config with node id " << config.node_id << ".";
  //   throw std::runtime_error(ss.str());
  // }

  // party_id_ = iter->second.vm(0).party_id();
  // local_node_.ip_ = iter->second.ip();
  // local_node_.port_ = iter->second.port();
  // local_node_.use_tls_ = iter->second.use_tls();
  // local_node_.id_ = iter->second.node_id();

  // // Dump all party of the task.
  // LOG(INFO) << "[Local party] party id " << party_id_
  //           << ", node: " << local_node_.to_string() << ".";

  // uint16_t next_party_id = (party_id_ + 1) % 3;
  // LOG(INFO) << "[Remote party] party id " << next_party_id
  //           << ", node: " << node_map_[next_party_id].to_string() << ".";

  // next_party_id = (party_id_ + 2) % 3;
  // LOG(INFO) << "[Remote party] party id " << next_party_id
  //           << ", node: " << node_map_[next_party_id].to_string() << ".";
}
#ifndef MPC_SOCKET_CHANNEL
retcode MPCStatisticsExecutor::_parseColumnName(const std::string &json_str) {
  do_nothing_ = false;
  Document json_doc;
  if (json_doc.Parse(json_str.c_str()).HasParseError()) {
    LOG(ERROR) << "Parse json string failed, json:\n" << json_str << ".";
    return retcode::FAIL;
  }

  if (!json_doc.HasMember("features")) {
    LOG(ERROR)
        << "Json object should have 'features' member but not found now.";
    return retcode::FAIL;
  }

  const Value &features_content = json_doc["features"];
  if (!features_content.IsArray()) {
    LOG(ERROR) << "Value of 'features' in json must be a array.";
    return retcode::FAIL;
  }

  // Value of key 'features' contain dataset name and target columns.
  bool found = false;
  for (int i = 0; i < features_content.Size(); i++) {
    const Value &item = features_content[i];
    if (!item.HasMember("resourceId")) {
      LOG(ERROR) << "Can't find 'resourceId' in the value of 'features'in "
                    "json string.";
      return retcode::FAIL;
    }

    // Value of 'resourceId' is dataset name.
    const Value &resource_id = item["resourceId"];
    if (!resource_id.IsString()) {
      LOG(ERROR) << "Type of 'resourceId' should be string.";
      return retcode::FAIL;
    }

    if (ds_name_ != resource_id.GetString())
      continue;

    // Value of 'checked' is array that include target column.
    const Value &checked = item["checked"];
    if (!checked.IsArray()) {
      LOG(ERROR)
          << "Value of 'checked' in the value of 'features' should be a array.";
      return retcode::FAIL;
    }

    LOG(INFO) << "Count of column processed is " << checked.Size() << ".";

    if (checked.Size() == 0) {
      LOG(WARNING) << "No column in the value of key 'checked'.";
      do_nothing_ = true;
      return retcode::FAIL;
    }

    for (int i = 0; i < checked.Size(); i++) {
      const Value &tmp = checked[i];
      if (!tmp.IsString()) {
        LOG(ERROR) << "The " << i + 1
                   << "th value of array in the value of 'checked' should be a "
                      "string.";
        return retcode::FAIL;
      }

      target_columns_.emplace_back(tmp.GetString());
    }

    found = true;
    break;
  }

  if (found == false) {
    LOG(ERROR) << "Can't find content for dataset " << ds_name_
               << " in the json string.";
    return retcode::FAIL;
  }

  if (!json_doc.HasMember("type")) {
    LOG(ERROR) << "Json object should have 'type' member but not found now.";
    return retcode::FAIL;
  }

  const Value &type_object = json_doc["type"];
  if (type_object.GetString() == std::string("1")) {
    type_ = MPCStatisticsType::AVG;
  } else if (type_object.GetString() == std::string("2")) {
    type_ = MPCStatisticsType::SUM;
  } else if (type_object.GetString() == std::string("3")) {
    type_ = MPCStatisticsType::MAX;
  } else if (type_object.GetString() == std::string("4")) {
    type_ = MPCStatisticsType::MIN;
  } else {
    LOG(ERROR) << "Unknown statistics type " << type_object.GetString() << ".";
    return retcode::FAIL;
  }

  return retcode::SUCCESS;
}

retcode MPCStatisticsExecutor::_parseColumnDtype(const std::string &json_str) {
  Document json_doc;
  if (json_doc.Parse(json_str.c_str()).HasParseError()) {
    LOG(ERROR) << "Parse json string failed, json:\n" << json_str << ".";
    return retcode::FAIL;
  }

  if (!json_doc.HasMember(ds_name_.c_str())) {
    LOG(ERROR) << "Can't find key '" << ds_name_ << "' in json string.";
    return retcode::FAIL;
  }

  const Value &map_obj = json_doc[ds_name_.c_str()];
  if (!map_obj.HasMember("columns")) {
    LOG(ERROR) << "Can't find key 'columns' in the value of '" << ds_name_
               << "' in the json string.";
    return retcode::FAIL;
  }

  if (!map_obj.HasMember("newDataSetId")) {
    LOG(ERROR) << "Can't find key 'newDataSetId' in the value of '" << ds_name_
               << "' in the json string.";
    return retcode::FAIL;
  }

  if (!map_obj.HasMember("outputFilePath")) {
    LOG(ERROR) << "Can't find key 'outputFilePath' in the value of '"
               << ds_name_ << "' in the json string.";
    return retcode::FAIL;
  }

  // Get dtype of target columns.
  const Value &dtype_obj = map_obj["columns"];
  if (!dtype_obj.IsObject()) {
    LOG(ERROR) << "Value of 'columns' in the value of '" << ds_name_
               << "' should be object.";
    return retcode::FAIL;
  }

  for (Value::ConstMemberIterator iter = dtype_obj.MemberBegin();
       iter != dtype_obj.MemberEnd(); iter++) {
    std::string col_name = iter->name.GetString();
    ColumnDtype dtype;
    columnDtypeFromInteger(iter->value.GetInt(), dtype);
    col_type_.insert(std::make_pair(col_name, dtype));

    LOG(INFO) << "Type of column " << iter->name.GetString() << " is "
              << columnDtypeToString(dtype) << ".";
  }

  // Get dataset id of new dataset.
  const Value &new_id_obj = map_obj["newDataSetId"];
  if (!new_id_obj.IsString()) {
    LOG(ERROR) << "Value of 'newDataSetId' in the value of '" << ds_name_
               << "' should be string.";
    return retcode::FAIL;
  }

  new_ds_id_ = new_id_obj.GetString();

  // Get save path of new dataset.
  const Value &new_path = map_obj["outputFilePath"];
  if (!new_path.IsString()) {
    LOG(ERROR) << "Value of 'outputFilePath' in the value of '" << ds_name_
               << "' should be string.";
    return retcode::FAIL;
  }

  output_path_ = new_path.GetString();

  return retcode::SUCCESS;
}


int MPCStatisticsExecutor::loadParams(primihub::rpc::Task &task) {
  LOG(INFO) << "party_name: " << this->party_name_;
  auto party_datasets = task.party_datasets();
  auto it = party_datasets.find(this->party_name());
  if (it == party_datasets.end()) {
    LOG(ERROR) << "no data set found for party name: " << this->party_name();
    return -1;
  }
  const auto& dataset = it->second.data();
  auto iter = dataset.find("Data_File");
  if (iter == dataset.end()) {
    LOG(ERROR) << "no dataset found for dataset name Data_File";
    return -1;
  }
  // File path.
  ds_name_ = iter->second;

  auto param_map = task.params().param_map();
  std::string task_detail = param_map["TaskDetail"].value_string();
  std::string col_info = param_map["ColumnInfo"].value_string();

  bool use_mpc_div = false;
  if (param_map["UseMPC_Div"].value_string() != "") {
    LOG(INFO) << "Use MPC Div instead of plaintext div.";
    use_mpc_div = true;
  }

  if (_parseColumnName(task_detail) != retcode::SUCCESS) {
    if (do_nothing_ == true) {
      LOG(WARNING) << "Do nothing due to no column selected.";
      return 0;
    } else {
      LOG(ERROR) << "Obtain column to be processed from json string failed.";
      return -1;
    }
  }

  if (_parseColumnDtype(col_info) != retcode::SUCCESS) {
    LOG(ERROR) << "Obtain column dtype and other from json string failed.";
    return -1;
  }

  std::sort(target_columns_.begin(), target_columns_.end());

  // Ensure the dtype of all columns processed later can be found.
  bool miss_flag = false;
  for (const auto &col_name : target_columns_) {
    auto iter = col_type_.find(col_name);
    if (iter == col_type_.end()) {
      LOG(ERROR) << "Can't find dtype of column " << col_name << ".";
      miss_flag = true;
      break;
    }

    if ((iter->second != ColumnDtype::LONG) &&
        (iter->second != ColumnDtype::INTEGER) &&
        (iter->second != ColumnDtype::DOUBLE)) {
      LOG(ERROR) << "Dtype of column " << iter->first << " is "
                 << columnDtypeToString(iter->second)
                 << ", don't support this dtype now.";
      miss_flag = true;
      break;
    }
  }

  if (miss_flag)
    return -1;

  switch (type_) {
  case MPCStatisticsType::AVG:
    executor_ = std::make_unique<MPCSumOrAvg>(type_);
    break;
  case MPCStatisticsType::SUM:
    executor_ = std::make_unique<MPCSumOrAvg>(type_);
    break;
  case MPCStatisticsType::MAX:
    executor_ = std::make_unique<MPCMinOrMax>(type_);
    break;
  case MPCStatisticsType::MIN:
    executor_ = std::make_unique<MPCMinOrMax>(type_);
    break;
  default:
    LOG(ERROR) << "No executor for "
               << MPCStatisticsOperator::statisticsTypeToString(type_) << ".";
    return -1;
  }

  {
    std::stringstream ss;
    uint16_t col_count = target_columns_.size();
    ss << "Run " << MPCStatisticsOperator::statisticsTypeToString(type_)
       << " on columns";
    for (uint16_t i = 0; i < col_count - 1; i++)
      ss << " " << target_columns_[i] << ",";
    ss << " " << target_columns_[col_count - 1] << ".";
    LOG(INFO) << ss.str();
  }

  return 0;
}

int MPCStatisticsExecutor::execute() {
  if (do_nothing_) {
    LOG(WARNING) << "Skip execute due to nothing to do.";
    return 0;
  }

  auto ret = executor_->run(input_value_, target_columns_, col_type_);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "Run MPC statistics executor failed.";
    return -1;
  }

  executor_->getResult(result_);
  return 0;
}

retcode MPCStatisticsExecutor::InitEngine() {
  executor_->setupChannel(this->party_id(), this->CommPkgPtr());
  return retcode::SUCCESS;
}

int MPCStatisticsExecutor::loadDataset() {
  if (do_nothing_) {
    LOG(WARNING) << "Skip load dataset due to nothing to do.";
    return 0;
  }

  auto driver = dataset_service_->getDriver(ds_name_);
  auto cursor = std::move(driver->read());
  input_value_ = cursor->read();
  if (input_value_.get() == nullptr) {
    LOG(ERROR) << "Load data from dataset failed.";
    return -1;
  }

  return 0;
}

int MPCStatisticsExecutor::saveModel() {
  if (do_nothing_) {
    LOG(WARNING) << "Skip save model due to nothing to do.";
    return 0;
  }

  std::vector<std::shared_ptr<arrow::Field>> table_fields;
  for (const auto &col : target_columns_)
    table_fields.emplace_back(arrow::field(col, arrow::float64()));
  auto schema = std::make_shared<arrow::Schema>(table_fields);

  std::vector<std::shared_ptr<arrow::Array>> column_values;
  arrow::MemoryPool *pool = arrow::default_memory_pool();
  for (int i = 0; i < result_.rows(); i++) {
    arrow::DoubleBuilder builder(pool);
    builder.Append(result_(i, 0));
    std::shared_ptr<arrow::Array> array;
    builder.Finish(&array);
    column_values.emplace_back(array);
  }

  std::shared_ptr<arrow::Table> table =
      arrow::Table::Make(schema, column_values);

  ValidateDir(output_path_);

  std::shared_ptr<DataDriver> driver =
      DataDirverFactory::getDriver("CSV", dataset_service_->getNodeletAddr());
  driver->initCursor(output_path_);
  auto dataset = std::make_shared<primihub::Dataset>(table, driver);
  auto cursor = driver->initCursor(output_path_);
  int ret = cursor->write(dataset);
  if (ret != 0) {
    LOG(ERROR) << "Save statistics result to file " << output_path_
               << " failed.";
    return -1;
  }

  LOG(INFO) << "MPC Statistics result saves to " << output_path_ << ".";

  service::DatasetMeta meta(dataset, new_ds_id_,
                            service::DatasetVisbility::PUBLIC, output_path_);
  dataset_service_->regDataset(meta);

  return 0;
}

#else
int MPCStatisticsExecutor::loadParams(primihub::rpc::Task &task) {return 0;}
int MPCStatisticsExecutor::loadDataset() {return 0;}
int MPCStatisticsExecutor::initPartyComm() {return 0;}
int MPCStatisticsExecutor::execute() {return 0;}
int MPCStatisticsExecutor::finishPartyComm() {return 0;}
int MPCStatisticsExecutor::saveModel() {return 0;}
retcode MPCStatisticsExecutor::_parseColumnName(const std::string &json_str) {
  return retcode::SUCCESS;
}
retcode MPCStatisticsExecutor::_parseColumnDtype(const std::string &json_str) {
  return retcode::SUCCESS;
}
#endif  // endif MPC_SOCKET_CHANNEL
}; // namespace primihub
