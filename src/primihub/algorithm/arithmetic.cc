#include "src/primihub/algorithm/arithmetic.h"

#include <arrow/api.h>
#include <arrow/array.h>
#include <arrow/result.h>

#include "src/primihub/data_store/csv/csv_driver.h"
#include "src/primihub/data_store/factory.h"
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

template <Decimal Dbit>
ArithmeticExecutor<Dbit>::ArithmeticExecutor(
    PartyConfig &config, std::shared_ptr<DatasetService> dataset_service)
    : AlgorithmBase(dataset_service) {
  this->algorithm_name_ = "arithmetic";

  std::map<std::string, Node> &node_map = config.node_map;
  // LOG(INFO) << node_map.size();
  std::map<uint16_t, rpc::Node> party_id_node_map;
  for (auto iter = node_map.begin(); iter != node_map.end(); iter++) {
    rpc::Node &node = iter->second;
    uint16_t party_id = static_cast<uint16_t>(node.vm(0).party_id());
    party_id_node_map[party_id] = node;
  }

  auto iter = node_map.find(config.node_id); // node_id
  if (iter == node_map.end()) {
    stringstream ss;
    ss << "Can't find " << config.node_id << " in node_map.";
    throw std::runtime_error(ss.str());
  }

  party_id_ = iter->second.vm(0).party_id();
  LOG(INFO) << "Note party id of this node is " << party_id_ << ".";

  if (party_id_ == 0) {
    rpc::Node &node = party_id_node_map[0];

    next_ip_ = node.ip();
    next_port_ = node.vm(0).next().port();

    prev_ip_ = node.ip();
    prev_port_ = node.vm(0).prev().port();

  } else if (party_id_ == 1) {
    rpc::Node &node = party_id_node_map[1];

    // A local server addr.
    uint16_t port = node.vm(0).next().port();
    // next_addr_ = std::make_pair(node.ip(), port);

    // // A remote server addr.
    // prev_addr_ =
    //     std::make_pair(node.vm(0).prev().ip(), node.vm(0).prev().port());

    next_ip_ = node.ip();
    next_port_ = port;

    prev_ip_ = node.vm(0).prev().ip();
    prev_port_ = node.vm(0).prev().port();
  } else {
    rpc::Node &node = party_id_node_map[2];

    // Two remote server addr.
    // next_addr_ =
    //     std::make_pair(node.vm(0).next().ip(), node.vm(0).next().port());
    // prev_addr_ =
    //     std::make_pair(node.vm(0).prev().ip(), node.vm(0).prev().port());

    next_ip_ = node.vm(0).next().ip();
    next_port_ = node.vm(0).next().port();

    prev_ip_ = node.vm(0).prev().ip();
    prev_port_ = node.vm(0).prev().port();
  }
}

template <Decimal Dbit>
int ArithmeticExecutor<Dbit>::loadParams(primihub::rpc::Task &task) {
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
      // LOG(INFO) << col << ":" << owner;
    }
    // LOG(INFO) << col_and_owner;

    std::string col_and_dtype = param_map["Col_And_Dtype"].value_string();
    spiltStr(col_and_dtype, ";", tmp2);
    for (auto itr = tmp2.begin(); itr != tmp2.end(); itr++) {
      int pos = itr->find('-');
      std::string col = itr->substr(0, pos);
      int dtype = std::atoi((itr->substr(pos + 1, itr->size())).c_str());
      col_and_dtype_.insert(make_pair(col, dtype));
      // LOG(INFO) << col << ":" << dtype;
    }
    // LOG(INFO) << col_and_dtype;

    expr_ = param_map["Expr"].value_string();
    is_cmp = false;
    if (expr_.substr(0, 3) == "CMP")
      is_cmp = true;
    if (is_cmp) {
      std::string next_name;
      std::string prev_name;
      if (party_id_ == 0) {
        next_name = "01";
        prev_name = "02";
      } else if (party_id_ == 1) {
        next_name = "12";
        prev_name = "01";
      } else if (party_id_ == 2) {
        next_name = "02";
        prev_name = "12";
      }
      mpc_op_exec_ = new MPCOperator(party_id_, next_name, prev_name);
    } else {
      mpc_exec_ = new MPCExpressExecutor<Dbit>();
    }
    // LOG(INFO) << expr_;
    std::string parties = param_map["Parties"].value_string();
    spiltStr(parties, ";", tmp3);
    for (auto itr = tmp3.begin(); itr != tmp3.end(); itr++) {
      uint32_t party = std::atoi((*itr).c_str());
      parties_.push_back(party);
      // LOG(INFO) << party;
    }
    // LOG(INFO) << parties;

    res_name_ = param_map["ResFileName"].value_string();
  } catch (std::exception &e) {
    LOG(ERROR) << "Failed to load params: " << e.what();
    return -1;
  }

  return 0;
}

template <Decimal Dbit> int ArithmeticExecutor<Dbit>::loadDataset() {
  int ret = _LoadDatasetFromCSV(data_file_path_);
  // file reading error or file empty
  if (ret <= 0) {
    LOG(ERROR) << "Load dataset for train failed.";
    return -1;
  }

  if (is_cmp) {
    return 0;
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

template <Decimal Dbit> int ArithmeticExecutor<Dbit>::initPartyComm(void) {
  if (is_cmp) {
    mpc_op_exec_->setup(next_ip_, prev_ip_, next_port_, prev_port_);
    return 0;
  }

  mpc_exec_->initMPCRuntime(party_id_, next_ip_, prev_ip_, next_port_,
                            prev_port_);
  return 0;
}

template <Decimal Dbit> int ArithmeticExecutor<Dbit>::execute() {
  if (is_cmp) {
    try {
      sbMatrix sh_res;
      f64Matrix<Dbit> m;
      if (col_and_owner_[expr_.substr(4, 1)] == party_id_) {
        m.resize(1, col_and_val_double[expr_.substr(4, 1)].size());
        for (size_t i = 0; i < col_and_val_double[expr_.substr(4, 1)].size();
             i++)
          m(i) = col_and_val_double[expr_.substr(4, 1)][i];
        mpc_op_exec_->MPC_Compare(m, sh_res);
      } else if (col_and_owner_[expr_.substr(6, 1)] == party_id_) {
        m.resize(1, col_and_val_double[expr_.substr(6, 1)].size());
        for (size_t i = 0; i < col_and_val_double[expr_.substr(6, 1)].size();
             i++)
          m(i) = col_and_val_double[expr_.substr(6, 1)][i];
        mpc_op_exec_->MPC_Compare(m, sh_res);
      } else
        mpc_op_exec_->MPC_Compare(sh_res);
      // reveal
      for (auto party : parties_) {
        if (party_id_ == party) {
          i64Matrix tmp = mpc_op_exec_->reveal(sh_res);
          for (size_t i = 0; i < tmp.rows(); i++)
            cmp_res_.emplace_back(static_cast<bool>(tmp(i, 0)));
        } else {
          mpc_op_exec_->reveal(sh_res, party);
        }
      }
    } catch (std::exception &e) {
      LOG(ERROR) << "In party " << party_id_ << ":\n" << e.what() << ".";
    }
    return 0;
  }
  try {
    mpc_exec_->runMPCEvaluate();
    if (mpc_exec_->isFP64RunMode()) {
      mpc_exec_->revealMPCResult(parties_, final_val_double_);
      // for (auto itr = final_val_double_.begin(); itr !=
      // final_val_double_.end();
      //      itr++)
      //   LOG(INFO) << *itr;
    } else {
      mpc_exec_->revealMPCResult(parties_, final_val_int64_);
      // for (auto itr = final_val_int64_.begin(); itr !=
      // final_val_int64_.end();
      //      itr++)
      //   LOG(INFO) << *itr;
    }
  } catch (const std::exception &e) {
    std::string msg = "In party 0, ";
    msg = msg + e.what();
    throw std::runtime_error(msg);
  }
  return 0;
}

template <Decimal Dbit> int ArithmeticExecutor<Dbit>::finishPartyComm(void) {
  if (is_cmp) {
    mpc_op_exec_->fini();
    delete mpc_op_exec_;
    return 0;
  }
  delete mpc_exec_;
  return 0;
}

template <Decimal Dbit> int ArithmeticExecutor<Dbit>::saveModel(void) {
  bool is_reveal = false;
  for (auto party : parties_) {
    if (party == party_id_) {
      is_reveal = true;
    }
  }
  if (!is_reveal) {
    return 0;
  }
  arrow::MemoryPool *pool = arrow::default_memory_pool();
  arrow::DoubleBuilder builder(pool);
  if (final_val_double_.size() != 0)
    for (int i = 0; i < final_val_double_.size(); i++)
      builder.Append(final_val_double_[i]);
  else if (final_val_int64_.size() != 0)
    for (int i = 0; i < final_val_int64_.size(); i++)
      builder.Append(final_val_int64_[i]);
  else
    for (int i = 0; i < cmp_res_.size(); i++)
      builder.Append(cmp_res_[i]);
  std::shared_ptr<arrow::Array> array;
  builder.Finish(&array);

  std::vector<std::shared_ptr<arrow::Field>> schema_vector_double = {
      arrow::field(expr_, arrow::float64())};
  std::vector<std::shared_ptr<arrow::Field>> schema_vector_int64 = {
      arrow::field(expr_, arrow::int64())};
  std::vector<std::shared_ptr<arrow::Field>> schema_vector_bool = {
      arrow::field(expr_, arrow::boolean())};

  std::shared_ptr<arrow::Table> table;
  if (final_val_double_.size() != 0)
    table = arrow::Table::Make(
        std::make_shared<arrow::Schema>(schema_vector_double), {array});
  else if (final_val_int64_.size() != 0)
    table = arrow::Table::Make(
        std::make_shared<arrow::Schema>(schema_vector_int64), {array});
  else
    table = arrow::Table::Make(
        std::make_shared<arrow::Schema>(schema_vector_bool), {array});
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
    LOG(ERROR) << "Save res to file " << filepath << " failed.";
    return -1;
  }
  LOG(INFO) << "Save res to " << filepath << ".";

  return 0;
}

template <Decimal Dbit>
int ArithmeticExecutor<Dbit>::_LoadDatasetFromCSV(std::string &filename) {
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
  int chunk_num = table->column(num_col - 1)->chunks().size();
  int64_t array_len = 0;
  for (int k = 0; k < chunk_num; k++) {
    auto array = std::static_pointer_cast<DoubleArray>(
        table->column(num_col - 1)->chunk(k));
    array_len += array->length();
  }
  
  LOG(INFO) << "Label column '" << col_names[num_col - 1] << "' has "
            << array_len << " values.";
  // Force the same value count in every column.

  for (int i = 0; i < num_col; i++) {
    int chunk_num = table->column(i)->chunks().size();
    if (col_and_dtype_[col_names[i]] == 0) {
      if (table->schema()->GetFieldByName(col_names[i])->type()->id() != 9) {
        LOG(ERROR) << "Local data type is inconsistent with the demand data "
                      "type!Demand data type is int,but local data type is "
                      "double!Please input consistent data type!";
        return -1;
      }
      std::vector<int64_t> tmp_data;
      int64_t tmp_len = 0;
      for (int k = 0; k < chunk_num; k++) {
        auto array =
            std::static_pointer_cast<Int64Array>(table->column(i)->chunk(k));
        tmp_len += array->length();
        for (int64_t j = 0; j < array->length(); j++) {
          tmp_data.push_back(array->Value(j));
          // LOG(INFO) << array->Value(j);
        }
      }
      if (tmp_len != array_len) {
        LOG(ERROR) << "Column " << col_names[i] << " has " << tmp_len
                   << " value, but other column has " << array_len << " value.";
        errors = true;
        break;
      }
      col_and_val_int.insert(
          pair<string, std::vector<int64_t>>(col_names[i], tmp_data));
      // for (auto itr = col_and_val_int.begin(); itr != col_and_val_int.end();
      //      itr++) {
      //   LOG(INFO) << itr->first;
      //   auto tmp_vec = itr->second;
      //   for (auto iter = tmp_vec.begin(); iter != tmp_vec.end(); iter++)
      //     LOG(INFO) << *iter;
      // }
    } else {
      std::vector<double> tmp_data;
      int64_t tmp_len = 0;
      if (table->schema()->GetFieldByName(col_names[i])->type()->id() == 9) {
        for (int k = 0; k < chunk_num; k++) {
          auto array =
              std::static_pointer_cast<Int64Array>(table->column(i)->chunk(k));
          tmp_len += array->length();
          for (int64_t j = 0; j < array->length(); j++) {
            tmp_data.push_back(array->Value(j));
            // LOG(INFO) << array->Value(j);
          }
        }
        if (tmp_len != array_len) {
          LOG(ERROR) << "Column " << col_names[i] << " has " << tmp_len
                     << " value, but other column has " << array_len
                     << " value.";
          errors = true;
          break;
        }
      } else {
        for (int k = 0; k < chunk_num; k++) {
          auto array =
              std::static_pointer_cast<DoubleArray>(table->column(i)->chunk(k));
          tmp_len += array->length();
          for (int64_t j = 0; j < array->length(); j++) {
            tmp_data.push_back(array->Value(j));
            // LOG(INFO) << array->Value(j);
          }
        }
        if (tmp_len != array_len) {
          LOG(ERROR) << "Column " << col_names[i] << " has " << tmp_len
                     << " value, but other column has " << array_len
                     << " value.";
          errors = true;
          break;
        }
      }
      col_and_val_double.insert(
          pair<string, std::vector<double>>(col_names[i], tmp_data));
      // for (auto itr = col_and_val_double.begin();
      //      itr != col_and_val_double.end(); itr++) {
      //   LOG(INFO) << itr->first;
      //   auto tmp_vec = itr->second;
      //   for (auto iter = tmp_vec.begin(); iter != tmp_vec.end(); iter++)
      //     LOG(INFO) << *iter;
      // }
    }
  }
  if (errors)
    return -1;

  return array_len;
}
template class ArithmeticExecutor<D32>;
template class ArithmeticExecutor<D16>;

} // namespace primihub
