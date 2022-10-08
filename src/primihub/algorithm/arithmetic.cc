#include "src/primihub/algorithm/arithmetic.h"
#include "src/primihub/data_store/csv/csv_driver.h"
#include "src/primihub/data_store/factory.h"
#include <arrow/api.h>
#include <arrow/array.h>
#include <arrow/result.h>
using arrow::Array;
using arrow::DoubleArray;
using arrow::Int64Array;
using arrow::Table;
namespace primihub {
void spiltStr(string str, const string &split, std::vector<string> &strlist) {
  strlist.clear();
  if (str == "")
    return;
  string strs = str + split;
  size_t pos = strs.find(split);
  int steps = split.size();

  while (pos != strs.npos) {
    string temp = strs.substr(0, pos);
    strlist.push_back(temp);
    strs = strs.substr(pos + steps, strs.size());
    pos = strs.find(split);
  }
}

ArithmeticExecutor::ArithmeticExecutor(
    PartyConfig &config, std::shared_ptr<DatasetService> dataset_service)
    : AlgorithmBase(dataset_service) {
  this->algorithm_name_ = "arithmetic";

  mpc_exec_ = new MPCExpressExecutor();
  party_id_ = 0;
  next_ip_ = "127.0.0.1";
  prev_ip_ = "127.0.0.1";
  next_port_ = 10010;
  prev_port_ = 10020;
  if (config.node_id == "node1") {
    party_id_ = 1;
    next_port_ = 10030;
    prev_port_ = 10010;
  } else if (config.node_id == "node2") {
    party_id_ = 2;
    next_port_ = 10020;
    prev_port_ = 10030;
  }
}

int ArithmeticExecutor::loadParams(primihub::rpc::Task &task) {
  auto param_map = task.params().param_map();
  try {
    data_file_path_ = param_map["Data_File"].value_string();
    // col_and_owner
    std::string col_and_owner = param_map["Col_And_Owner"].value_string();
    std::vector<string> tmp1, tmp2, tmp3;
    spiltStr(col_and_owner, ";", tmp1);
    for (auto itr = tmp1.begin(); itr != tmp1.end(); itr++) {
      int pos = itr->find('-');
      std::string col = itr->substr(0, pos);
      int owner = std::atoi((itr->substr(pos + 1, itr->size())).c_str());
      col_and_owner_.insert(make_pair(col, owner));
      LOG(INFO) << col << ":" << owner;
    }
    // LOG(INFO) << col_and_owner;

    std::string col_and_dtype = param_map["Col_And_Dtype"].value_string();
    spiltStr(col_and_dtype, ";", tmp2);
    for (auto itr = tmp2.begin(); itr != tmp2.end(); itr++) {
      int pos = itr->find('-');
      std::string col = itr->substr(0, pos);
      int dtype = std::atoi((itr->substr(pos + 1, itr->size())).c_str());
      col_and_dtype_.insert(make_pair(col, dtype));
      LOG(INFO) << col << ":" << dtype;
    }
    // LOG(INFO) << col_and_dtype;

    expr_ = param_map["Expr"].value_string();
    LOG(INFO) << expr_;
    std::string parties = param_map["Parties"].value_string();
    spiltStr(parties, ";", tmp3);
    for (auto itr = tmp3.begin(); itr != tmp3.end(); itr++) {
      uint32_t party = std::atoi((*itr).c_str());
      parties_.push_back(party);
      LOG(INFO) << party;
    }
    LOG(INFO) << parties;

    res_name_ = param_map["ResFileName"].value_string();
  } catch (std::exception &e) {
    LOG(ERROR) << "Failed to load params: " << e.what();
    return -1;
  }

  return 0;
}

int ArithmeticExecutor::loadDataset() {
  int ret = _LoadDatasetFromCSV(data_file_path_);
  // file reading error or file empty
  if (ret <= 0) {
    LOG(ERROR) << "Load dataset for train failed.";
    return -1;
  }
  mpc_exec_->initColumnConfig(party_id_);
  for (auto &pair : col_and_owner_)
    mpc_exec_->importColumnOwner(pair.first, pair.second);
  for (auto &pair : col_and_dtype_)
    mpc_exec_->importColumnDtype(pair.first, pair.second);
  mpc_exec_->importExpress(expr_);
  mpc_exec_->resolveRunMode();
  mpc_exec_->InitFeedDict();
  if (col_and_val_double.size() != 0) {
    for (auto &pair : col_and_val_double)
      mpc_exec_->importColumnValues(pair.first, pair.second);
  } else {
    for (auto &pair : col_and_val_int)
      mpc_exec_->importColumnValues(pair.first, pair.second);
  }
  return 0;
}

int ArithmeticExecutor::initPartyComm(void) {
  mpc_exec_->initMPCRuntime(party_id_, next_ip_, prev_ip_, next_port_,
                            prev_port_);
  return 0;
}

int ArithmeticExecutor::execute() {
  try {
    mpc_exec_->runMPCEvaluate();
    if (mpc_exec_->isFP64RunMode()) {
      mpc_exec_->revealMPCResult(parties_, final_val_double_);
      for (auto itr = final_val_double_.begin(); itr != final_val_double_.end();
           itr++)
        LOG(INFO) << *itr;
    } else {
      mpc_exec_->revealMPCResult(parties_, final_val_int64_);
      for (auto itr = final_val_int64_.begin(); itr != final_val_int64_.end();
           itr++)
        LOG(INFO) << *itr;
    }
  } catch (const std::exception &e) {
    std::string msg = "In party 0, ";
    msg = msg + e.what();
    throw std::runtime_error(msg);
  }
  return 0;
}

int ArithmeticExecutor::finishPartyComm(void) {
  delete mpc_exec_;
  return 0;
}

int ArithmeticExecutor::saveModel(void) {
  arrow::MemoryPool *pool = arrow::default_memory_pool();
  arrow::DoubleBuilder builder(pool);
  if (final_val_double_.size() != 0)
    for (int i = 0; i < final_val_double_.size(); i++)
      builder.Append(final_val_double_[i]);
  else
    for (int i = 0; i < final_val_int64_.size(); i++)
      builder.Append(final_val_int64_[i]);

  std::shared_ptr<arrow::Array> array;
  builder.Finish(&array);

  std::vector<std::shared_ptr<arrow::Field>> schema_vector_double = {
      arrow::field(expr_, arrow::float64())};
  std::vector<std::shared_ptr<arrow::Field>> schema_vector_int64 = {
      arrow::field(expr_, arrow::int64())};
  std::shared_ptr<arrow::Table> table;
  if (final_val_double_.size() != 0)
    table = arrow::Table::Make(
        std::make_shared<arrow::Schema>(schema_vector_double), {array});
  else
    table = arrow::Table::Make(
        std::make_shared<arrow::Schema>(schema_vector_int64), {array});
  std::shared_ptr<DataDriver> driver =
      DataDirverFactory::getDriver("CSV", dataset_service_->getNodeletAddr());
  std::shared_ptr<CSVDriver> csv_driver =
      std::dynamic_pointer_cast<CSVDriver>(driver);

  std::string filepath = "data/" + res_name_ + ".csv";
  int ret = 0;
  if (col_and_val_double.size() != 0)
    ret = csv_driver->write(table, filepath);
  else
    ret = csv_driver->write(table, filepath);
  if (ret != 0) {
    LOG(ERROR) << "Save LR model to file " << filepath << " failed.";
    return -1;
  }
  LOG(INFO) << "Save model to " << filepath << ".";

  return 0;
}

int ArithmeticExecutor::_LoadDatasetFromCSV(std::string &filename) {
  std::string nodeaddr("test address"); // TODO
  std::shared_ptr<DataDriver> driver =
      DataDirverFactory::getDriver("CSV", nodeaddr);
  std::shared_ptr<Cursor> &cursor = driver->read(filename);
  std::shared_ptr<Dataset> ds = cursor->read();
  std::shared_ptr<Table> table = std::get<std::shared_ptr<Table>>(ds->data);

  // Label column.
  std::vector<std::string> col_names = table->ColumnNames();
  // for (auto itr = col_names.begin(); itr != col_names.end(); itr++) {
  //   LOG(INFO) << *itr;
  // }
  bool errors = false;
  int num_col = table->num_columns();

  // 'array' include values in a column of csv file.
  auto array = std::static_pointer_cast<DoubleArray>(
      table->column(num_col - 1)->chunk(0));
  int64_t array_len = array->length();
  LOG(INFO) << "Label column '" << col_names[num_col - 1] << "' has "
            << array_len << " values.";

  // Force the same value count in every column.
  for (int i = 0; i < num_col; i++) {
    if (col_and_dtype_[col_names[i]] == 0) {
      auto array =
          std::static_pointer_cast<Int64Array>(table->column(i)->chunk(0));
      std::vector<int64_t> tmp_data;
      for (int64_t j = 0; j < array->length(); j++) {
        tmp_data.push_back(array->Value(j));
        LOG(INFO) << array->Value(j);
      }
      if (array->length() != array_len) {
        LOG(ERROR) << "Column " << col_names[i] << " has " << array->length()
                   << " value, but other column has " << array_len << " value.";
        errors = true;
        break;
      }
      col_and_val_int.insert(
          pair<string, std::vector<int64_t>>(col_names[i], tmp_data));
      for (auto itr = col_and_val_int.begin(); itr != col_and_val_int.end();
           itr++) {
        LOG(INFO) << itr->first;
        auto tmp_vec = itr->second;
        for (auto iter = tmp_vec.begin(); iter != tmp_vec.end(); iter++)
          LOG(INFO) << *iter;
      }
    } else {
      auto array =
          std::static_pointer_cast<DoubleArray>(table->column(i)->chunk(0));
      std::vector<double> tmp_data;
      for (int64_t j = 0; j < array->length(); j++) {
        tmp_data.push_back(array->Value(j));
        LOG(INFO) << array->Value(j);
      }
      if (array->length() != array_len) {
        LOG(ERROR) << "Column " << col_names[i] << " has " << array->length()
                   << " value, but other column has " << array_len << " value.";
        errors = true;
        break;
      }
      col_and_val_double.insert(
          pair<string, std::vector<double>>(col_names[i], tmp_data));
      for (auto itr = col_and_val_double.begin();
           itr != col_and_val_double.end(); itr++) {
        LOG(INFO) << itr->first;
        auto tmp_vec = itr->second;
        for (auto iter = tmp_vec.begin(); iter != tmp_vec.end(); iter++)
          LOG(INFO) << *iter;
      }
    }
  }
  if (errors)
    return -1;

  return array->length();
}

} // namespace primihub
