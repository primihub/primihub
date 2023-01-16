#include "src/primihub/algorithm/missing_val_processing.h"

#include <arrow/api.h>
#include <arrow/array.h>
#include <arrow/array/array_binary.h>
#include <arrow/csv/api.h>
#include <arrow/csv/writer.h>
#include <arrow/filesystem/localfs.h>
#include <arrow/io/api.h>
#include <arrow/io/file.h>
#include <arrow/pretty_print.h>
#include <arrow/result.h>
#include <arrow/type.h>
#include <glog/logging.h>
#include <parquet/arrow/reader.h>
#include <parquet/arrow/writer.h>
#include <parquet/exception.h>
#include <parquet/stream_reader.h>
#include <rapidjson/document.h>

#include <iostream>

#include "src/primihub/data_store/csv/csv_driver.h"
#include "src/primihub/data_store/dataset.h"
#include "src/primihub/data_store/driver.h"
#include "src/primihub/data_store/factory.h"

using arrow::Array;
using arrow::DoubleArray;
using arrow::Int64Array;
using arrow::StringArray;
using arrow::Table;

using namespace rapidjson;

namespace primihub {
void MissingProcess::_spiltStr(string str, const string &split,
                               std::vector<string> &strlist) {
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

int MissingProcess::_strToInt64(const std::string &str, int64_t &i64_val) {
  try {
    VLOG(5) << "Convert string '" << str << "' into int64 value.";
    size_t conv_length = 0;
    i64_val = stoll(str, &conv_length);
    if (conv_length != str.length()) {
      throw std::invalid_argument("Invalid stoll argument");
    }
  } catch (std::invalid_argument const &ex) {
    LOG(ERROR) << "Can't convert string " << str
               << " to int64 value, invalid numberic string.";
    return -1;
  } catch (std::out_of_range const &ex) {
    LOG(ERROR) << "Can't convert string " << str
               << " to int64 value, value in string out of range.";
    return -2;
  }

  return 0;
}

int MissingProcess::_strToDouble(const std::string &str, double &d_val) {
  try {
    VLOG(5) << "Convert string '" << str << "' into double value.";
    size_t conv_length = 0;
    d_val = std::stod(str, &conv_length);
    if (conv_length != str.length()) {
      throw std::invalid_argument("Invalid stod argument");
    }
  } catch (std::invalid_argument &ex) {
    LOG(ERROR) << "Can't convert string " << str
               << " to double value, invalid numberic string.";
    return -1;
  } catch (std::out_of_range &ex) {
    LOG(ERROR) << "Can't convert string " << str
               << " to double value, value in string out of range.";
    return -2;
  }

  return 0;
}

int MissingProcess::_avoidStringArray(std::shared_ptr<arrow::Array> array) {
  auto result = array->View(::arrow::utf8());
  if (!result.ok()) {
    LOG(WARNING) << "StringArray is bad, " << result.status() << ".";
    return 1;
  }

  return 0;
}

void MissingProcess::_buildNewColumn(std::vector<std::string> &col_val,
                                     std::shared_ptr<arrow::Array> &array) {
  arrow::MemoryPool *pool = arrow::default_memory_pool();
  arrow::StringBuilder builder(pool);
  for (auto i = 0; i < col_val.size(); i++)
    builder.Append(col_val[i]);

  builder.Finish(&array);
}

void MissingProcess::_buildNewColumn(std::shared_ptr<arrow::Table> table,
                                     int col_index, const std::string &replace,
                                     NestedVectorI32 &abnormal_index,
                                     bool need_double,
                                     std::shared_ptr<arrow::Array> &new_array) {
  int chunk_num = table->column(col_index)->chunks().size();

  std::vector<std::vector<std::string>> new_col_val;
  new_col_val.resize(chunk_num);

  for (int k = 0; k < chunk_num; k++) {
    auto array = std::static_pointer_cast<StringArray>(
        table->column(col_index)->chunk(k));

    // Copy value in array into vector, the null value will be replaced
    // by average value.
    for (int j = 0; j < array->length(); j++)
      if (array->IsNull(j))
        new_col_val[k].emplace_back(replace);
      else
        new_col_val[k].emplace_back(array->GetString(j));

    // Replace abnormal value with average value.
    for (size_t j = 0; j < abnormal_index[k].size(); j++) {
      auto index = abnormal_index[k][j];
      new_col_val[k][index] = replace;
    }
  }

  // Combine all chunk's value.
  for (int k = 1; k < chunk_num; k++) {
    auto chunk_vals = new_col_val[k];
    new_col_val[0].insert(new_col_val[0].end(), chunk_vals.begin(),
                          chunk_vals.end());
  }

  if (VLOG_IS_ON(5)) {
    VLOG(5) << "After rebuild, value in this column is:";
    for (auto &val : new_col_val[0])
      VLOG(5) << val;
  }

  if (need_double) {
    arrow::MemoryPool *pool = arrow::default_memory_pool();
    arrow::DoubleBuilder builder(pool);
    for (auto i = 0; i < new_col_val[0].size(); i++)
      builder.Append(std::stod(new_col_val[0][i]));
    builder.Finish(&new_array);
  } else {
    arrow::MemoryPool *pool = arrow::default_memory_pool();
    arrow::Int64Builder builder(pool);
    for (auto i = 0; i < new_col_val[0].size(); i++)
      builder.Append(std::stoll(new_col_val[0][i]));
    builder.Finish(&new_array);
  }
}

void MissingProcess::_buildNewColumn(std::shared_ptr<arrow::Table> table,
                                     int col_index, const std::string &replace,
                                     std::vector<int> both_index,
                                     bool need_double,
                                     std::shared_ptr<arrow::Array> &new_array) {
  int chunk_num = table->column(col_index)->chunks().size();

  std::vector<std::vector<double>> new_double_col_val;
  std::vector<std::vector<int64_t>> new_int_col_val;
  if (need_double) {
    new_double_col_val.resize(chunk_num);

    for (int k = 0; k < chunk_num; k++) {
      auto array = std::static_pointer_cast<DoubleArray>(
          table->column(col_index)->chunk(k));

      // Copy value in array into vector
      for (int j = 0; j < array->length(); j++)
        new_double_col_val[k].emplace_back(array->Value(j));
    }
    // Combine all chunk's value.
    for (int k = 1; k < chunk_num; k++) {
      auto chunk_vals = new_double_col_val[k];
      new_double_col_val[0].insert(new_double_col_val[0].end(),
                                   chunk_vals.begin(), chunk_vals.end());
    }
    // Replace abnormal and null value with average value.
    for (size_t j = 0; j < both_index.size(); j++) {
      auto index = both_index[j];
      new_double_col_val[0][index] = std::stod(replace);
    }
  } else {
    new_int_col_val.resize(chunk_num);

    for (int k = 0; k < chunk_num; k++) {
      auto array = std::static_pointer_cast<Int64Array>(
          table->column(col_index)->chunk(k));

      // Copy value in array into vector
      for (int j = 0; j < array->length(); j++)
        new_int_col_val[k].emplace_back(array->Value(j));
    }
    // Combine all chunk's value.
    for (int k = 1; k < chunk_num; k++) {
      auto chunk_vals = new_int_col_val[k];
      new_int_col_val[0].insert(new_int_col_val[0].end(), chunk_vals.begin(),
                                chunk_vals.end());
    }
    // Replace abnormal and null value with average value.
    for (size_t j = 0; j < both_index.size(); j++) {
      auto index = both_index[j];
      new_int_col_val[0][index] = std::stoll(replace);
    }
  }

  if (VLOG_IS_ON(5)) {
    VLOG(5) << "After rebuild, value in this column is:";
    if (need_double)
      for (auto &val : new_double_col_val[0])
        VLOG(5) << val;
    else
      for (auto &val : new_int_col_val[0])
        VLOG(5) << val;
  }

  if (need_double) {
    arrow::MemoryPool *pool = arrow::default_memory_pool();
    arrow::DoubleBuilder builder(pool);
    for (auto i = 0; i < new_double_col_val[0].size(); i++)
      builder.Append(new_double_col_val[0][i]);
    builder.Finish(&new_array);
  } else {
    arrow::MemoryPool *pool = arrow::default_memory_pool();
    arrow::Int64Builder builder(pool);
    for (auto i = 0; i < new_int_col_val[0].size(); i++)
      builder.Append(new_int_col_val[0][i]);
    builder.Finish(&new_array);
  }
}

MissingProcess::MissingProcess(PartyConfig &config,
                               std::shared_ptr<DatasetService> dataset_service,
                               std::unique_ptr<LinkContext> &link_context)
    : AlgorithmBase(dataset_service, link_context) {
  this->algorithm_name_ = "missing_val_processing";

  std::map<std::string, rpc::Node> &node_map = config.node_map;
  LOG(INFO) << node_map.size();
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

    next_ip_ = node.ip();
    next_port_ = port;

    prev_ip_ = node.vm(0).prev().ip();
    prev_port_ = node.vm(0).prev().port();
  } else {
    rpc::Node &node = party_id_node_map[2];

    next_ip_ = node.vm(0).next().ip();
    next_port_ = node.vm(0).next().port();

    prev_ip_ = node.vm(0).prev().ip();
    prev_port_ = node.vm(0).prev().port();
  }

  node_id_ = config.node_id;
}

int MissingProcess::loadParams(primihub::rpc::Task &task) {
  auto param_map = task.params().param_map();

  // File path.
  data_file_path_ = param_map["Data_File"].value_string();
  //
  use_db = false;
  if (data_file_path_.find("sqlite") == 0) {
    use_db = true;
    conn_info_ = data_file_path_;
    std::vector<std::string> conn_info;
    _spiltStr(data_file_path_, "#", conn_info);
    data_file_path_ = conn_info[1];
    table_name = conn_info[2];
  }
  // Column dtype.
  std::string json_str = param_map["ColumnInfo"].value_string();

  Document doc;
  doc.Parse(json_str.c_str());

  bool found = false;
  std::string local_dataset;
  for (Value::ConstMemberIterator iter = doc.MemberBegin();
       iter != doc.MemberEnd(); iter++) {
    std::string ds_name = iter->name.GetString();
    std::string ds_node = param_map[ds_name].value_string();
    if (ds_node == this->node_id_) {
      local_dataset = iter->name.GetString();
      found = true;
    }
  }

  if (!found) {
    // TODO: This request every node has dataset to handle, but sometimes only
    // two node has dataset to handle, fix it later.
    std::stringstream ss;
    ss << "Can't not find dataset belong to " << this->node_id_ << ".";
    LOG(ERROR) << ss.str();
    throw std::runtime_error(ss.str());
  }

  Document doc_ds;
  auto doc_iter = doc.FindMember(local_dataset.c_str());
  doc_ds.Swap(doc_iter->value);

  Value &vals = doc_ds["columns"];
  for (Value::ConstMemberIterator iter = vals.MemberBegin();
       iter != vals.MemberEnd(); iter++) {
    std::string col_name = iter->name.GetString();
    uint32_t col_dtype = iter->value.GetInt();
    col_and_dtype_.insert(std::make_pair(col_name, col_dtype));
    LOG(INFO) << "Type of column " << iter->name.GetString() << " is "
              << iter->value.GetInt() << ".";
  }

  new_dataset_id_ = doc_ds["newDataSetId"].GetString();
  LOG(INFO) << "New id of new dataset is " << new_dataset_id_ << ".";
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
  // mpc_op_exec_->set_task_info(platform_type_, job_id_, task_id_);

  return 0;
}

int MissingProcess::loadDataset() {
  int ret;
  if (use_db) {
    ret = _LoadDatasetFromDB(conn_info_);
  } else {
    ret = _LoadDatasetFromCSV(data_file_path_);
  }
  // file reading error or file empty
  if (ret <= 0) {
    LOG(ERROR) << "Load dataset failed.";
    return -1;
  }
  return 0;
}

int MissingProcess::initPartyComm(void) {
  LOG(INFO) << "Begin to init party comm.";
  mpc_op_exec_->setup(next_ip_, prev_ip_, next_port_, prev_port_);
  LOG(INFO) << "Finish to init party comm.";
  return 0;
}

int MissingProcess::execute() {
  try {
    int cols_0, cols_1, cols_2;
    for (uint64_t i = 0; i < 3; i++) {
      if (party_id_ == i) {
        cols_0 = col_and_dtype_.size();
        mpc_op_exec_->mNext.asyncSendCopy(cols_0);
        mpc_op_exec_->mPrev.asyncSendCopy(cols_0);
      } else {
        if (party_id_ == (i + 1) % 3)
          mpc_op_exec_->mPrev.recv(cols_2);
        else if (party_id_ == (i + 2) % 3)
          mpc_op_exec_->mNext.recv(cols_1);
        else
          throw std::runtime_error("Message recv logic error.");
      }
    }

    if (cols_0 != cols_1 || cols_0 != cols_2 || cols_1 != cols_2) {
      LOG(ERROR)
          << "The taget data columns of the three parties are inconsistent!";
      return -1;
    }

    int *arr_dtype0 = new int[cols_0];
    int *arr_dtype1 = new int[cols_0];
    int *arr_dtype2 = new int[cols_0];

    int tmp_index = 0;
    for (auto itr = col_and_dtype_.begin(); itr != col_and_dtype_.end();
         itr++) {
      arr_dtype0[tmp_index++] = itr->second;
    }

    for (uint64_t i = 0; i < 3; i++) {
      if (party_id_ == i) {
        mpc_op_exec_->mNext.asyncSendCopy(arr_dtype0, cols_0);
        mpc_op_exec_->mPrev.asyncSendCopy(arr_dtype0, cols_0);
      } else {
        if (party_id_ == (i + 1) % 3)
          mpc_op_exec_->mPrev.recv(arr_dtype1, cols_0);
        else if (party_id_ == (i + 2) % 3)
          mpc_op_exec_->mNext.recv(arr_dtype2, cols_0);
        else
          throw std::runtime_error("Message recv logic error.");
      }
    }

    for (uint64_t i = 0; i < cols_0; i++) {
      if ((arr_dtype0[i] != arr_dtype1[i]) ||
          (arr_dtype0[i] != arr_dtype2[i]) ||
          (arr_dtype1[i] != arr_dtype2[i])) {
        LOG(ERROR)
            << "The data column types of the three parties are inconsistent!";
        return -1;
      }
    }

    delete arr_dtype0;
    delete arr_dtype1;
    delete arr_dtype2;

    for (auto iter = col_and_dtype_.begin(); iter != col_and_dtype_.end();
         iter++) {
      auto t = std::find(local_col_names.begin(), local_col_names.end(),
                         iter->first);

      double double_sum = 0;
      uint32_t double_count = 0;

      int64_t int_sum = 0;
      uint32_t int_count = 0;

      int null_num = 0;
      int abnormal_num = 0;

      // For each column type of which maybe double or int64, read every row as
      // a string then try to convert string into int64 value or double value,
      // this value should be a abnormal value when convert failed.
      if (t != local_col_names.end()) {
        LOG(INFO) << "Begin to process column " << iter->first << ":";
        std::vector<std::vector<uint32_t>> abnormal_index;
        int col_index = std::distance(local_col_names.begin(), t);

        if (iter->second == 1 || iter->second == 3) {
          null_num = 0;
          int_count = 0;
          int_sum = 0;
          abnormal_num = 0;

          int chunk_num = table->column(0)->chunks().size();
          LOG(INFO) << "Column " << iter->first << " has " << chunk_num
                    << " chunk(s), expect type is int64.";

          abnormal_index.resize(chunk_num);
          if (use_db) {
            for (int k = 0; k < chunk_num; k++) {
              auto value_array = std::static_pointer_cast<Int64Array>(
                  table->column(col_index)->chunk(k));
              int64_t i64_val = 0;
              for (int64_t j = 0; j < value_array->length(); j++) {
                i64_val = value_array->Value(j);
                int_sum += i64_val;
              }
            }
            int_count = table->num_rows() - db_both_index[iter->first].size();
          } else {
            for (int k = 0; k < chunk_num; k++) {
              auto str_array = static_pointer_cast<StringArray>(
                  table->column(col_index)->chunk(k));

              // Detect string that can't convert into int64_t value.
              int ret = 0;
              int64_t i64_val = 0;
              for (int64_t j = 0; j < str_array->length(); j++) {
                if (str_array->IsNull(j)) {
                  LOG(WARNING) << "Find missing value in column " << iter->first
                               << ", chunk " << k << ", index " << j << ".";
                  continue;
                }

                ret = _strToInt64(str_array->GetString(j), i64_val);
                if (ret != 0) {
                  abnormal_num++;
                  abnormal_index[k].emplace_back(j);
                  LOG(WARNING)
                      << "Find abnormal value '" << str_array->GetString(j)
                      << "' in column " << iter->first << ", chunk " << k
                      << ", index " << j << ".";
                } else {
                  int_sum += i64_val;
                }
              }

              null_num +=
                  table->column(col_index)->chunk(k)->data()->GetNullCount();
            }
            int_count = table->num_rows() - null_num - abnormal_num;
          }
        } else if (iter->second == 2) {
          null_num = 0;
          double_count = 0;
          double_sum = 0;
          abnormal_num = 0;

          int chunk_num = table->column(col_index)->chunks().size();
          LOG(INFO) << "Column " << iter->first << " has " << chunk_num
                    << " chunk(s), expect type is double.";

          abnormal_index.resize(chunk_num);
          if (use_db) {
            for (int k = 0; k < chunk_num; k++) {
              auto value_array = std::static_pointer_cast<DoubleArray>(
                  table->column(col_index)->chunk(k));
              double d_val = 0;
              for (int64_t j = 0; j < value_array->length(); j++) {
                d_val = value_array->Value(j);
                double_sum += d_val;
              }
            }
            double_count =
                table->num_rows() - db_both_index[iter->first].size();
          } else {
            for (int k = 0; k < chunk_num; k++) {
              auto str_array = std::static_pointer_cast<StringArray>(
                  table->column(col_index)->chunk(k));
              // Detect string that can't convert into double value.
              double d_val = 0;
              int ret = 0;
              // LOG_INFO()<< str_array->length();
              for (int64_t j = 0; j < str_array->length(); j++) {
                // LOG(INFO) << str_array->GetString(j);
                if (str_array->IsNull(j)) {
                  LOG(WARNING) << "Find missing value in column " << iter->first
                               << ", chunk " << k << ", index " << j << ".";
                  continue;
                }
                ret = _strToDouble(str_array->GetString(j), d_val);
                if (ret) {
                  abnormal_num++;
                  abnormal_index[k].emplace_back(j);
                  LOG(WARNING)
                      << "Find abnormal value '" << str_array->GetString(j)
                      << "' in column " << iter->first << ", chunk " << k
                      << ", index " << j << ".";

                } else {
                  double_sum += d_val;
                }
              }

              // Get count of position that is empty in this column.
              null_num +=
                  table->column(col_index)->chunk(k)->data()->GetNullCount();
            }
            double_count = table->num_rows() - null_num - abnormal_num;
          }
        }

        if (iter->second == 1 || iter->second == 3) {
          i64Matrix m(2, 1);
          m(0, 0) = int_sum;
          m(1, 0) = int_count;

          LOG(INFO) << "Local column: sum " << int_sum << ", count "
                    << int_count << ".";

          si64Matrix sh_m[3];
          for (uint8_t i = 0; i < 3; i++) {
            if (i == party_id_) {
              sh_m[i].resize(2, 1);
              mpc_op_exec_->createShares(m, sh_m[i]);
            } else {
              sh_m[i].resize(2, 1);
              mpc_op_exec_->createShares(sh_m[i]);
            }
          }

          si64Matrix sh_sum(2, 1);
          sh_sum = sh_m[0];
          for (uint8_t i = 1; i < 3; i++)
            sh_sum = sh_sum + sh_m[i];

          LOG(INFO) << "Run MPC sum to get sum of all party.";

          i64Matrix plain_sum(2, 1);
          plain_sum = mpc_op_exec_->revealAll(sh_sum);

          LOG(INFO) << "Sum of column in all party is " << plain_sum(0, 0)
                    << ", sum of count in all party is " << plain_sum(1, 0)
                    << ".";

          LOG(INFO) << "Build new array to save column value, missing and "
                       "abnormal value will be replaced by average value.";

          // Update value in position that have null or abormal value with
          // average value.
          int64_t col_avg = plain_sum(0, 0) / plain_sum(1, 0);
          std::shared_ptr<arrow::Array> new_array;
          if (use_db) {
            _buildNewColumn(table, col_index, std::to_string(col_avg),
                            db_both_index[iter->first], false, new_array);
          } else {
            _buildNewColumn(table, col_index, std::to_string(col_avg),
                            abnormal_index, false, new_array);
          }
          std::shared_ptr<arrow::ChunkedArray> chunk_array =
              std::make_shared<arrow::ChunkedArray>(new_array);
          std::shared_ptr<arrow::Field> field =
              std::make_shared<arrow::Field>(iter->first, arrow::int64());

          LOG(INFO) << "Replace column " << iter->first
                    << " with new array in table.";

          LOG(INFO) << "col_index:" << col_index;
          LOG(INFO) << "name:" << field->name();
          LOG(INFO) << "type:" << field->type();
          LOG(INFO) << "table->type:" << table->field(col_index)->type();

          auto result = table->SetColumn(col_index, field, chunk_array);
          if (!result.ok()) {
            std::stringstream ss;
            ss << "Replace content of column " << iter->first << " failed, "
               << result.status();
            LOG(ERROR) << ss.str();
            throw std::runtime_error(ss.str());
          }

          table = result.ValueOrDie();
          LOG(INFO) << "Finish.";
        } else if (iter->second == 2) {
          eMatrix<double> m(2, 1);
          m(0, 0) = double_sum;
          m(1, 0) = double_count;

          LOG(INFO) << "Local column: sum " << double_sum << ", count "
                    << double_count << ".";

          sf64Matrix<D16> sh_m[3];
          for (uint8_t i = 0; i < 3; i++) {
            if (i == party_id_) {
              sh_m[i].resize(2, 1);
              mpc_op_exec_->createShares(m, sh_m[i]);
            } else {
              sh_m[i].resize(2, 1);
              mpc_op_exec_->createShares(sh_m[i]);
            }
          }

          sf64Matrix<D16> sh_sum;
          sh_sum = sh_m[0];
          for (int j = 1; j < 3; j++)
            sh_sum = sh_sum + sh_m[j];

          LOG(INFO) << "Run MPC sum to get sum of all party.";

          eMatrix<double> plain_sum(2, 0);
          plain_sum = mpc_op_exec_->revealAll(sh_sum);

          LOG(INFO) << "Sum of column in all party is " << plain_sum(0, 0)
                    << ", sum of count in all party is " << plain_sum(1, 0)
                    << ".";

          LOG(INFO) << "Build new array to save column value, missing and "
                       "abnormal value will be replaced by average value.";

          // Update value in position that have null or abormal value with
          // average value.
          double col_avg = plain_sum(0, 0) / plain_sum(1, 0);

          std::shared_ptr<arrow::Array> new_array;
          if (use_db) {
            _buildNewColumn(table, col_index, std::to_string(col_avg),
                            db_both_index[iter->first], true, new_array);
          } else {
            _buildNewColumn(table, col_index, std::to_string(col_avg),
                            abnormal_index, true, new_array);
          }
          std::shared_ptr<arrow::ChunkedArray> chunk_array =
              std::make_shared<arrow::ChunkedArray>(new_array);
          std::shared_ptr<arrow::Field> field =
              std::make_shared<arrow::Field>(iter->first, arrow::float64());

          LOG(INFO) << "Replace column " << iter->first
                    << " with new array in table.";

          auto result = table->SetColumn(col_index, field, chunk_array);
          if (!result.ok()) {
            std::stringstream ss;
            ss << "Replace content of column " << iter->first << " failed, "
               << result.status();
            LOG(ERROR) << ss.str();
            throw std::runtime_error(ss.str());
          }

          table = result.ValueOrDie();
          LOG(INFO) << "Finish.";
        }
      } else {
        LOG(ERROR) << "Can't find value of column " << iter->first << ".";
      }
    }
  } catch (std::exception &e) {
    LOG(ERROR) << "In party " << party_id_ << ":\n" << e.what() << ".";
    return -1;
  }

  return 0;
}

int MissingProcess::finishPartyComm(void) {
  si64 tmp_share0, tmp_share1, tmp_share2;
  if (party_id_ == 0)
    mpc_op_exec_->createShares(1, tmp_share0);
  else
    mpc_op_exec_->createShares(tmp_share0);
  mpc_op_exec_->fini();
  delete mpc_op_exec_;
  return 0;
}

int MissingProcess::saveModel(void) {
  std::vector<std::string> str_vec;
  std::string delimiter = ".";
  int pos = data_file_path_.rfind(delimiter);

  std::string new_path = data_file_path_.substr(0, pos) + "_missing.csv";

  std::shared_ptr<DataDriver> driver =
      DataDirverFactory::getDriver("CSV", dataset_service_->getNodeletAddr());

  auto cursor = driver->initCursor(new_path);

  VLOG(5) << "A brief view of new table:\n" << table->ToString() << ".";

  auto dataset = std::make_shared<primihub::Dataset>(table, driver);
  int ret = cursor->write(dataset);
  if (ret != 0) {
    LOG(ERROR) << "Save result to file " << new_path << " failed.";
    return -1;
  }

  service::DatasetMeta meta(dataset, new_dataset_id_,
                            service::DatasetVisbility::PUBLIC);
  dataset_service_->regDataset(meta);

  return 0;
}

int MissingProcess::_LoadDatasetFromCSV(std::string &dataset_id) {
  auto driver = this->dataset_service_->getDriver(dataset_id);
  auto access_info =
      dynamic_cast<CSVAccessInfo *>(driver->dataSetAccessInfo().get());
  if (access_info == nullptr) {
    LOG(ERROR) << "get csv access info for dataset: " << dataset_id
               << " failed";
    return -1;
  }
  auto &filename = access_info->file_path_;
  arrow::io::IOContext io_context = arrow::io::default_io_context();
  arrow::fs::LocalFileSystem local_fs(
      arrow::fs::LocalFileSystemOptions::Defaults());
  auto file_stream = local_fs.OpenInputStream(filename);
  if (!file_stream.ok()) {
    LOG(ERROR) << "Open file " << filename << " failed.";
    return -1;
  }

  std::shared_ptr<arrow::io::InputStream> input = file_stream.ValueOrDie();

  auto read_options = arrow::csv::ReadOptions::Defaults();
  auto parse_options = arrow::csv::ParseOptions::Defaults();
  auto convert_options = arrow::csv::ConvertOptions::Defaults();

  parse_options.ignore_empty_lines = false;

  // Force all column's type to string.
  std::unordered_map<std::string, std::shared_ptr<arrow::DataType>> expect_type;

  std::vector<std::string> target_column;
  for (auto &pair : col_and_dtype_) {
    if (pair.second == 1 || pair.second == 2)
      target_column.emplace_back(pair.first);
  }

  for (auto &col : target_column) {
    expect_type[col] = ::arrow::utf8();
    VLOG(5) << "Force type of column " << col << " be string.";
  }

  convert_options.column_types = expect_type;
  convert_options.strings_can_be_null = true;

  auto maybe_reader = arrow::csv::TableReader::Make(
      io_context, input, read_options, parse_options, convert_options);
  if (!maybe_reader.ok()) {
    LOG(ERROR) << "Init csv reader failed.";
    return -2;
  }

  std::shared_ptr<arrow::csv::TableReader> reader = *maybe_reader;
  auto maybe_table = reader->Read();
  if (!maybe_table.ok()) {
    LOG(ERROR) << "Read file " << filename << "'s content failed.";
    return -3;
  }

  table = *maybe_table;

  bool errors = false;
  int num_col = table->num_columns();
  // std::vector<std::string> col_names = table->ColumnNames();

  local_col_names = table->ColumnNames();

  // 'array' include values in a column of csv file.
  int chunk_num = table->column(num_col - 1)->chunks().size();
  int array_len = 0;
  for (int k = 0; k < chunk_num; k++) {
    auto array = std::static_pointer_cast<DoubleArray>(
        table->column(num_col - 1)->chunk(k));
    array_len += array->length();
  }

  // Force the same value count in every column.
  for (int i = 0; i < num_col; i++) {
    int chunk_num = table->column(i)->chunks().size();
    std::vector<double> tmp_data;
    int tmp_len = 0;

    for (int k = 0; k < chunk_num; k++) {
      auto array =
          std::static_pointer_cast<DoubleArray>(table->column(i)->chunk(k));
      tmp_len += array->length();
      for (int64_t j = 0; j < array->length(); j++) {
        tmp_data.push_back(array->Value(j));
      }
    }
    if (tmp_len != array_len) {
      LOG(ERROR) << "Column " << local_col_names[i] << " has " << tmp_len
                 << " value, but other column has " << array_len << " value.";
      errors = true;
      break;
    }

    if (errors)
      return -1;

    return array_len;
  }
}

int MissingProcess::_LoadDatasetFromDB(std::string &source) {
  std::shared_ptr<DataDriver> driver = DataDirverFactory::getDriver(
      "sqlite", dataset_service_->getNodeletAddr());

  // auto cursor = driver->initCursor(new_path);
  auto cursor = driver->read(source);
  std::shared_ptr<SQLiteCursor> sql_cursor =
      std::dynamic_pointer_cast<SQLiteCursor>(cursor);
  table = sql_cursor->read_from_abnormal(col_and_dtype_, db_both_index);
  if (table == NULL) {
    return -1;
  }
  bool errors = false;
  int num_col = table->num_columns();

  local_col_names = table->ColumnNames();
  for (auto i = 0; i < local_col_names.size(); i++)
    LOG(INFO) << local_col_names[i];
  // 'array' include values in a column of csv file.
  int chunk_num = table->column(num_col - 1)->chunks().size();

  int array_len = 0;
  for (int k = 0; k < chunk_num; k++) {
    auto array = std::static_pointer_cast<DoubleArray>(
        table->column(num_col - 1)->chunk(k));
    array_len += array->length();
  }

  // Force the same value count in every column.
  for (int i = 0; i < num_col; i++) {
    int chunk_num = table->column(i)->chunks().size();
    std::vector<double> tmp_data;
    int tmp_len = 0;

    for (int k = 0; k < chunk_num; k++) {
      auto array =
          std::static_pointer_cast<DoubleArray>(table->column(i)->chunk(k));
      tmp_len += array->length();
      for (int64_t j = 0; j < array->length(); j++) {
        tmp_data.push_back(array->Value(j));
      }
    }
    if (tmp_len != array_len) {
      LOG(ERROR) << "Column " << local_col_names[i] << " has " << tmp_len
                 << " value, but other column has " << array_len << " value.";
      errors = true;
      break;
    }

    if (errors)
      return -1;

    return array_len;
  }
}

int MissingProcess::set_task_info(std::string platform_type, std::string job_id,
                                  std::string task_id) {
  platform_type_ = platform_type;
  job_id_ = job_id;
  task_id_ = task_id;
  return 0;
}
} // namespace primihub
