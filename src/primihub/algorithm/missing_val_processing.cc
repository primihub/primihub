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

#include <float.h>
#include <iostream>
#include <limits>

// #include "src/primihub/common/type/fixed_point.h"
#include "src/primihub/data_store/csv/csv_driver.h"
#include "src/primihub/data_store/dataset.h"
#include "src/primihub/data_store/driver.h"
#include "src/primihub/data_store/factory.h"
#include "src/primihub/util/network/message_interface.h"
#include "src/primihub/util/file_util.h"

using arrow::Array;
using arrow::DoubleArray;
using arrow::Int64Array;
using arrow::StringArray;
using arrow::Table;

using namespace rapidjson;

namespace primihub {
void MissingProcess::_spiltStr(std::string str, const std::string &split,
                               std::vector<std::string> &strlist) {
  strlist.clear();
  if (str == "")
    return;
  std::string strs = str + split;
  size_t pos = strs.find(split);
  int steps = split.size();

  while (pos != strs.npos) {
    std::string temp = strs.substr(0, pos);
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
               << " to int64 value, invalid numeric string.";
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
    LOG(WARNING) << "Can't convert string " << str
                 << " to double value, invalid numeric string.";
    return -1;
  } catch (std::out_of_range &ex) {
    LOG(WARNING) << "Can't convert string " << str
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
  for (size_t i = 0; i < col_val.size(); i++) {
    builder.Append(col_val[i]);
  }

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

  if (need_double) {
    arrow::MemoryPool *pool = arrow::default_memory_pool();
    arrow::DoubleBuilder builder(pool);
    for (size_t i = 0; i < new_col_val[0].size(); i++)
      builder.Append(std::stod(new_col_val[0][i]));
    builder.Finish(&new_array);
  } else {
    arrow::MemoryPool *pool = arrow::default_memory_pool();
    arrow::Int64Builder builder(pool);
    for (size_t i = 0; i < new_col_val[0].size(); i++)
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
    for (size_t i = 0; i < new_double_col_val[0].size(); i++)
      builder.Append(new_double_col_val[0][i]);
    builder.Finish(&new_array);
  } else {
    arrow::MemoryPool *pool = arrow::default_memory_pool();
    arrow::Int64Builder builder(pool);
    for (size_t i = 0; i < new_int_col_val[0].size(); i++)
      builder.Append(new_int_col_val[0][i]);
    builder.Finish(&new_array);
  }
}

MissingProcess::MissingProcess(PartyConfig &config,
                               std::shared_ptr<DatasetService> dataset_service)
                               : AlgorithmBase(config, dataset_service) {
  this->algorithm_name_ = "missing_val_processing";
  this->set_party_name(config.party_name());
  this->set_party_id(config.party_id());

// #ifdef MPC_SOCKET_CHANNEL
//   std::map<std::string, rpc::Node> &node_map = config.node_map;

//   std::map<uint16_t, rpc::Node> party_id_node_map;
//   for (auto iter = node_map.begin(); iter != node_map.end(); iter++) {
//     rpc::Node &node = iter->second;
//     uint16_t party_id = static_cast<uint16_t>(node.vm(0).party_id());
//     party_id_node_map[party_id] = node;
//   }

//   auto iter = node_map.find(config.node_id);  // node_id
//   if (iter == node_map.end()) {
//     std::stringstream ss;
//     ss << "Can't find " << config.node_id << " in node_map.";
//     throw std::runtime_error(ss.str());
//   }

//   party_id_ = iter->second.vm(0).party_id();
//   LOG(INFO) << "Note party id of this node is " << party_id_ << ".";

//   if (party_id_ == 0) {
//     rpc::Node &node = party_id_node_map[0];

//     next_ip_ = node.ip();
//     next_port_ = node.vm(0).next().port();

//     prev_ip_ = node.ip();
//     prev_port_ = node.vm(0).prev().port();
//   } else if (party_id_ == 1) {
//     rpc::Node &node = party_id_node_map[1];

//     // A local server addr.
//     uint16_t port = node.vm(0).next().port();

//     next_ip_ = node.ip();
//     next_port_ = port;

//     prev_ip_ = node.vm(0).prev().ip();
//     prev_port_ = node.vm(0).prev().port();
//   } else {
//     rpc::Node &node = party_id_node_map[2];

//     next_ip_ = node.vm(0).next().ip();
//     next_port_ = node.vm(0).next().port();

//     prev_ip_ = node.vm(0).prev().ip();
//     prev_port_ = node.vm(0).prev().port();
//   }

//   node_id_ = config.node_id;
// #endif
//   party_config_.Init(config);
  party_id_ = party_config_.SelfPartyId();
}

int MissingProcess::loadParams(primihub::rpc::Task &task) {
  AlgorithmBase::loadParams(task);
  LOG(INFO) << "party_name: " << this->party_name_;
  auto ret = this->ExtractProxyNode(task, &this->proxy_node_);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "extract proxy node failed";
    return -1;
  }
  auto party_datasets = task.party_datasets();
  auto it = party_datasets.find(this->party_name());
  if (it == party_datasets.end()) {
    LOG(ERROR) << "no data set found for party name: " << this->party_name();
    return -1;
  }
  const auto &dataset = it->second.data();
  auto iter = dataset.find("Data_File");
  if (iter == dataset.end()) {
    LOG(ERROR) << "no dataset found for dataset name Data_File";
    return -1;
  }
  // File path.
  if (it->second.dataset_detail()) {
    this->is_dataset_detail_ = true;
    auto& param_map = task.params().param_map();
    auto p_it = param_map.find("Data_File");
    if (p_it != param_map.end()) {
      data_file_path_ = p_it->second.value_string();
      this->dataset_id_ = iter->second;
    } else {
      LOG(ERROR) << "no dataset id is found";
      return -1;
    }
  } else {
    data_file_path_ = iter->second;
    this->dataset_id_ = iter->second;
  }

  auto param_map = task.params().param_map();
  replace_type_ = param_map["Replace_Type"].value_string();
  if (replace_type_ == "")
    replace_type_ = "MAX";

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

  // bool found = false;
  std::string local_dataset = data_file_path_;

  Document doc_ds;
  auto doc_iter = doc.FindMember(local_dataset.c_str());

  doc_ds.Swap(doc_iter->value);

  Value &vals = doc_ds["columns"];
  for (auto iter = vals.MemberBegin(); iter != vals.MemberEnd(); iter++) {
    std::string col_name = iter->name.GetString();
    uint32_t col_dtype = iter->value.GetInt();
    col_and_dtype_.insert(std::make_pair(col_name, col_dtype));
    LOG(INFO) << "Type of column " << iter->name.GetString() << " is "
              << iter->value.GetInt() << ".";
  }
  new_dataset_id_ = doc_ds["newDataSetId"].GetString();
  if (doc_ds.HasMember("outputFilePath") && doc_ds["outputFilePath"].IsString()) {
    new_dataset_path_ = doc_ds["outputFilePath"].GetString();
  } else {
    new_dataset_path_ = "./" + new_dataset_id_ + ".csv";
    LOG(WARNING) << "using default path: " << new_dataset_path_;
  }
  new_dataset_path_ = CompletePath(new_dataset_path_);

  LOG(INFO) << "New id of new dataset is " << new_dataset_id_ << ". "
            << "new dataset path: " << new_dataset_path_;
  return 0;
}

int MissingProcess::loadDataset() {
  int ret;
  if (use_db) {
    ret = _LoadDatasetFromDB(conn_info_);
  } else {
    ret = _LoadDatasetFromCSV(this->dataset_id_);
  }
  // file reading error or file empty
  if (ret <= 0) {
    LOG(ERROR) << "Load dataset failed.";
    return -1;
  }
  return 0;
}

retcode MissingProcess::InitEngine() {
  std::string next_name = "fake_next";
  std::string prev_name = "fake_prev";
  mpc_op_exec_ = std::make_unique<MPCOperator>(
      this->party_id(), next_name.c_str(), prev_name.c_str());
  mpc_op_exec_->setup(this->CommPkgPtr());
  return retcode::SUCCESS;
}

int MissingProcess::execute() {

  try {
    int cols_0, cols_1, cols_2;
    for (uint64_t i = 0; i < 3; i++) {
      if (party_id_ == i) {
        cols_0 = col_and_dtype_.size();
        mpc_op_exec_->mNext().asyncSendCopy(cols_0);
        mpc_op_exec_->mPrev().asyncSendCopy(cols_0);
      } else {
        if (party_id_ == (i + 1) % 3) {
          mpc_op_exec_->mPrev().recv(cols_2);
        } else if (party_id_ == (i + 2) % 3) {
          mpc_op_exec_->mNext().recv(cols_1);
        } else {
          std::stringstream ss;
          ss << "Abnormal party id value " << party_id_ << ".";

          LOG(ERROR) << ss.str();
          throw std::runtime_error(ss.str());
        }
      }
    }

    if (cols_0 != cols_1 || cols_0 != cols_2 || cols_1 != cols_2) {
      LOG(ERROR)
          << "The target data columns of the three parties are inconsistent!";
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
        mpc_op_exec_->mNext().asyncSendCopy(arr_dtype0, cols_0);
        mpc_op_exec_->mPrev().asyncSendCopy(arr_dtype0, cols_0);
      } else {
        if (party_id_ == (i + 1) % 3) {
          mpc_op_exec_->mPrev().recv(arr_dtype1, cols_0);
        } else if (party_id_ == (i + 2) % 3) {
          mpc_op_exec_->mNext().recv(arr_dtype2, cols_0);
        } else {
          std::stringstream ss;
          ss << "Abnormal party id value " << party_id_ << ".";

          LOG(ERROR) << ss.str();
          throw std::runtime_error(ss.str());
        }
      }
    }

    for (int i = 0; i < cols_0; i++) {
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

      int64_t int_max = LONG_MIN;
      int64_t int_min = LONG_MAX;
      double double_min = DBL_MAX;
      double double_max = DBL_MIN;
      // 0:avge 1:max 2:min
      int process_type = 0;
      int col_index = -1;
      double double_col_max = 0;
      int64_t int_col_max = 0;
      double double_col_min = 0;
      int64_t int_col_min = 0;

      // For each column type of which maybe double or int64, read every row as
      // a string then try to convert string into int64 value or double value,
      // this value should be a abnormal value when convert failed.
      if (t != local_col_names.end()) {
        LOG(INFO) << "Begin to process column " << iter->first << ":";
        std::vector<std::vector<uint32_t>> abnormal_index;
        col_index = std::distance(local_col_names.begin(), t);

        // Integer Long
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
                int_max = i64_val > int_max ? i64_val : int_max;
                int_min = i64_val < int_min ? i64_val : int_min;
              }
            }
            int_count = table->num_rows() - db_both_index[iter->first].size();
          } else {
            for (int k = 0; k < chunk_num; k++) {
              auto str_array = std::static_pointer_cast<StringArray>(
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

                // convert fails

                if (ret != 0) {
                  abnormal_num++;
                  abnormal_index[k].emplace_back(j);
                  LOG(WARNING)
                      << "Find abnormal value '" << str_array->GetString(j)
                      << "' in column " << iter->first << ", chunk " << k
                      << ", index " << j << ".";
                } else {

                  int_max = i64_val > int_max ? i64_val : int_max;
                  int_min = i64_val < int_min ? i64_val : int_min;

                  int_sum += i64_val;
                }
              }

              null_num +=
                  table->column(col_index)->chunk(k)->data()->GetNullCount();
            }
            int_count = table->num_rows() - null_num - abnormal_num;

          } // double

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
                double_max = d_val > double_max ? d_val : double_max;
                double_min = d_val < double_min ? d_val : double_min;
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

              for (int64_t j = 0; j < str_array->length(); j++) {

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

                  double_max = d_val > double_max ? d_val : double_max;
                  double_min = d_val < double_min ? d_val : double_min;

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

        // MPC
        //.........................................................................................
        const Decimal myD = D16;
        // 1:max 2:min 3:avg
        // enum replace { MAX, MIN, AVG };
        // replace replace_type = AVG;

        if (replace_type_ == "MAX") {
          if (iter->second == 1 || iter->second == 3) {
            i64Matrix m(1, 1);

            std::vector<bool> mpc_res;

            m(0, 0) = int_max;
            LOG(INFO) << "The max of party" << party_id_
                      << " column is: " << int_max << ".";

            sbMatrix sh_res;
            // first compare:p0-p1
            if (party_id_ != 2) {
              mpc_op_exec_->MPC_Compare(m, sh_res);
            } else {
              mpc_op_exec_->MPC_Compare(sh_res);
            }

            i64Matrix tmp;
            tmp.resize(sh_res.rows(), sh_res.i64Cols());
            mpc_op_exec_->enc.revealAll(mpc_op_exec_->runtime, sh_res, tmp)
                .get();
            for (i64 i = 0; i < tmp.rows(); i++) {
              mpc_res.emplace_back(static_cast<bool>(tmp(i, 0)));
            }
            LOG(INFO) << "Second: The revealed sh_res is " << tmp << ".";
            LOG(INFO) << "Second: The mpc_res is " << mpc_res[0] << ".";
            // second compare
            // 0:p0 is greater
            // 1:p1 is greater
            // 2:p2
            int flag = 0;
            sbMatrix sh_res2;
            for (size_t i = 0; i < mpc_res.size(); i++) {
              // p0<p1
              if (mpc_res[i]) {
                flag = 1;
                // p1-p2
                if (party_id_ != 0) {
                  mpc_op_exec_->MPC_Compare(m, sh_res2);
                } else {
                  mpc_op_exec_->MPC_Compare(sh_res2);
                }
              } else {
                flag = 0;
                // p0-p2
                if (party_id_ != 1) {
                  mpc_op_exec_->MPC_Compare(m, sh_res2);
                } else {
                  mpc_op_exec_->MPC_Compare(sh_res2);
                }
              }
            }

            std::vector<bool> mpc_res2;
            i64Matrix tmp2;
            tmp2.resize(sh_res2.rows(), sh_res2.i64Cols());
            mpc_op_exec_->enc.revealAll(mpc_op_exec_->runtime, sh_res2, tmp2)
                .get();
            for (i64 i = 0; i < tmp2.rows(); i++) {
              mpc_res2.emplace_back(static_cast<bool>(tmp2(i, 0)));
            }

            LOG(INFO) << "Second: The revealed sh_res2 is " << tmp2 << ".";
            LOG(INFO) << "Second: The mpc_res2 is " << mpc_res2[0] << ".";

            si64Matrix sh_max;
            sh_max.resize(m.rows(), m.cols());
            for (size_t i = 0; i < mpc_res2.size(); i++) {
              // max is p2
              if (mpc_res2[i]) {
                flag = 2;
                if (party_id_ == 2) {
                  mpc_op_exec_->createShares(m, sh_max);
                } else {
                  mpc_op_exec_->createShares(sh_max);
                }
              } else {
                // max is p0 or p1
                if (flag == 0) {
                  if (party_id_ == 0) {
                    mpc_op_exec_->createShares(m, sh_max);
                  } else {
                    mpc_op_exec_->createShares(sh_max);
                  }
                } else if (flag == 1) {
                  if (party_id_ == 1) {
                    mpc_op_exec_->createShares(m, sh_max);
                  } else {
                    mpc_op_exec_->createShares(sh_max);
                  }
                }
              }
              i64Matrix plain_max(1, 1);
              plain_max = mpc_op_exec_->revealAll(sh_max);
              LOG(WARNING) << "The max value is " << plain_max << " in party "
                           << flag << ".";
              int_col_max = plain_max(0, 0);
            }

            replaceValue(iter, table, col_index, int_col_max, abnormal_index,
                         use_db, false);
          } else if (iter->second == 2) {
            f64Matrix<myD> m(1, 1);

            std::vector<bool> mpc_res;

            m(0, 0) = double_max;
            LOG(INFO) << "The max of party" << party_id_
                      << " column is: " << double_max << ".";

            sbMatrix sh_res;
            // first compare:p0-p1
            if (party_id_ != 2) {
              mpc_op_exec_->MPC_Compare(m, sh_res);
            } else {
              mpc_op_exec_->MPC_Compare(sh_res);
            }
            LOG(INFO) << "The first compare is completely. ";

            i64Matrix tmp;
            tmp.resize(sh_res.rows(), sh_res.i64Cols());
            mpc_op_exec_->enc.revealAll(mpc_op_exec_->runtime, sh_res, tmp)
                .get();
            for (i64 i = 0; i < tmp.rows(); i++) {
              mpc_res.emplace_back(static_cast<bool>(tmp(i, 0)));
            }

            LOG(INFO) << "First: the revealed sh_res is " << tmp << ".";
            LOG(INFO) << "First: The mpc_res is " << mpc_res[0] << ".";

            // second compare
            // 0:p0 is greater
            // 1:p1 is greater
            // 2:p2
            int flag = 0;
            sbMatrix sh_res2;
            for (size_t i = 0; i < mpc_res.size(); i++) {
              // p0<p1
              if (mpc_res[i]) {
                flag = 1;
                // p1-p2
                if (party_id_ != 0) {
                  mpc_op_exec_->MPC_Compare(m, sh_res2);
                } else {
                  mpc_op_exec_->MPC_Compare(sh_res2);
                }
              }
              // p0>=p1
              else {
                flag = 0;
                // p0-p2
                if (party_id_ != 1) {
                  mpc_op_exec_->MPC_Compare(m, sh_res2);
                } else {
                  mpc_op_exec_->MPC_Compare(sh_res2);
                }
              }
            }

            std::vector<bool> mpc_res2;
            i64Matrix tmp2;
            tmp2.resize(sh_res2.rows(), sh_res2.i64Cols());
            mpc_op_exec_->enc.revealAll(mpc_op_exec_->runtime, sh_res2, tmp2)
                .get();
            for (i64 i = 0; i < tmp2.rows(); i++) {
              mpc_res2.emplace_back(static_cast<bool>(tmp2(i, 0)));
            }

            LOG(INFO) << "Second: The revealed sh_res is " << tmp2 << ".";
            LOG(INFO) << "Second: The mpc_res2 is " << mpc_res2[0] << ".";

            sf64Matrix<myD> sh_max;
            sh_max.resize(m.rows(), m.cols());
            for (size_t i = 0; i < mpc_res2.size(); i++) {
              // max is p2
              if (mpc_res2[i]) {
                flag = 2;
                if (party_id_ == 2) {
                  mpc_op_exec_->createShares(m, sh_max);
                } else {
                  mpc_op_exec_->createShares(sh_max);
                }
              } else {
                // max is p0 or p1
                if (flag == 0) {
                  if (party_id_ == 0) {
                    mpc_op_exec_->createShares(m, sh_max);
                  } else {
                    mpc_op_exec_->createShares(sh_max);
                  }
                } else if (flag == 1) {
                  if (party_id_ == 1) {
                    mpc_op_exec_->createShares(m, sh_max);
                  } else {
                    mpc_op_exec_->createShares(sh_max);
                  }
                }
              }
              eMatrix<double> plain_max(1, 1);
              plain_max = mpc_op_exec_->revealAll(sh_max);
              LOG(WARNING) << "The max value is " << plain_max << " in party "
                           << flag << ".";
              double_col_max = plain_max(0, 0);
            }

            replaceValue(iter, table, col_index, double_col_max, abnormal_index,
                         use_db, true);
          } else {
            LOG(ERROR) << "Can't find value of column " << iter->first << ".";
          }
        } else if (replace_type_ == "MIN") {
          if (iter->second == 1 || iter->second == 3) {
            i64Matrix m(1, 1);

            std::vector<bool> mpc_res;

            m(0, 0) = int_min;
            LOG(INFO) << "The min of party" << party_id_
                      << " column is: " << int_min << ".";

            sbMatrix sh_res;
            // first compare:p0-p1
            if (party_id_ != 2) {
              mpc_op_exec_->MPC_Compare(m, sh_res);
            } else {
              mpc_op_exec_->MPC_Compare(sh_res);
            }

            i64Matrix tmp;
            tmp.resize(sh_res.rows(), sh_res.i64Cols());
            mpc_op_exec_->enc.revealAll(mpc_op_exec_->runtime, sh_res, tmp)
                .get();
            for (i64 i = 0; i < tmp.rows(); i++) {
              mpc_res.emplace_back(static_cast<bool>(tmp(i, 0)));
            }
            LOG(INFO) << "Second: The revealed sh_res is " << tmp << ".";
            LOG(INFO) << "Second: The mpc_res is " << mpc_res[0] << ".";
            // second compare
            // 0:p0 is greater
            // 1:p1 is greater
            // 2:p2
            int flag = 0;
            sbMatrix sh_res2;
            for (size_t i = 0; i < mpc_res.size(); i++) {
              // p0<p1
              if (!mpc_res[i]) {
                flag = 1;
                // p1-p2
                if (party_id_ != 0) {
                  mpc_op_exec_->MPC_Compare(m, sh_res2);
                } else {
                  mpc_op_exec_->MPC_Compare(sh_res2);
                }
              } else {
                flag = 0;
                // p0-p2
                if (party_id_ != 1) {
                  mpc_op_exec_->MPC_Compare(m, sh_res2);
                } else {
                  mpc_op_exec_->MPC_Compare(sh_res2);
                }
              }
            }

            std::vector<bool> mpc_res2;
            i64Matrix tmp2;
            tmp2.resize(sh_res2.rows(), sh_res2.i64Cols());
            mpc_op_exec_->enc.revealAll(mpc_op_exec_->runtime, sh_res2, tmp2)
                .get();
            for (i64 i = 0; i < tmp2.rows(); i++) {
              mpc_res2.emplace_back(static_cast<bool>(tmp2(i, 0)));
            }

            LOG(INFO) << "Second: The revealed sh_res2 is " << tmp2 << ".";
            LOG(INFO) << "Second: The mpc_res2 is " << mpc_res2[0] << ".";

            si64Matrix sh_min;
            sh_min.resize(m.rows(), m.cols());

            for (size_t i = 0; i < mpc_res2.size(); i++) {
              // min is p2
              if (!mpc_res2[i]) {
                flag = 2;
                if (party_id_ == 2) {
                  mpc_op_exec_->createShares(m, sh_min);
                } else {
                  mpc_op_exec_->createShares(sh_min);
                }
              } else {
                // min is p0 or p1
                if (flag == 0) {
                  if (party_id_ == 0) {
                    mpc_op_exec_->createShares(m, sh_min);
                  } else {
                    mpc_op_exec_->createShares(sh_min);
                  }
                } else if (flag == 1) {
                  if (party_id_ == 1) {
                    mpc_op_exec_->createShares(m, sh_min);
                  } else {
                    mpc_op_exec_->createShares(sh_min);
                  }
                }
              }
              i64Matrix plain_min(1, 1);
              plain_min = mpc_op_exec_->revealAll(sh_min);
              LOG(WARNING) << "The min value is " << plain_min << " in party "
                           << flag << ".";
              int_col_min = plain_min(0, 0);
            }

            replaceValue(iter, table, col_index, int_col_min, abnormal_index,
                         use_db, false);
          } else if (iter->second == 2) {
            f64Matrix<myD> m(1, 1);

            std::vector<bool> mpc_res;

            m(0, 0) = double_min;

            LOG(INFO) << "The min of party" << party_id_
                      << " column is: " << double_min << ".";

            sbMatrix sh_res;
            // first compare:p0-p1
            if (party_id_ != 2) {
              mpc_op_exec_->MPC_Compare(m, sh_res);
            } else {
              mpc_op_exec_->MPC_Compare(sh_res);
            }
            LOG(INFO) << "The first compare is completely. ";

            i64Matrix tmp;
            tmp.resize(sh_res.rows(), sh_res.i64Cols());
            mpc_op_exec_->enc.revealAll(mpc_op_exec_->runtime, sh_res, tmp)
                .get();
            for (i64 i = 0; i < tmp.rows(); i++) {
              mpc_res.emplace_back(static_cast<bool>(tmp(i, 0)));
            }

            LOG(INFO) << "First: the revealed sh_res is " << tmp << ".";
            LOG(INFO) << "First: The mpc_res is " << mpc_res[0] << ".";

            // second compare
            // 0:p0 is greater
            // 1:p1 is greater
            // 2:p2
            int flag = 0;
            sbMatrix sh_res2;
            for (size_t i = 0; i < mpc_res.size(); i++) {
              // p0>=p1
              if (!mpc_res[i]) {
                flag = 1;
                // p1-p2
                if (party_id_ != 0) {
                  mpc_op_exec_->MPC_Compare(m, sh_res2);
                } else {
                  mpc_op_exec_->MPC_Compare(sh_res2);
                }
              }
              // p0<p1
              else {
                flag = 0;
                // p0-p2
                if (party_id_ != 1) {
                  mpc_op_exec_->MPC_Compare(m, sh_res2);
                } else {
                  mpc_op_exec_->MPC_Compare(sh_res2);
                }
              }
            }

            std::vector<bool> mpc_res2;
            i64Matrix tmp2;
            tmp2.resize(sh_res2.rows(), sh_res2.i64Cols());
            mpc_op_exec_->enc.revealAll(mpc_op_exec_->runtime, sh_res2, tmp2)
                .get();
            for (i64 i = 0; i < tmp2.rows(); i++) {
              mpc_res2.emplace_back(static_cast<bool>(tmp2(i, 0)));
            }

            sf64Matrix<myD> sh_min;
            sh_min.resize(m.rows(), m.cols());
            for (size_t i = 0; i < mpc_res2.size(); i++) {
              // min is p2
              if (!mpc_res2[i]) {
                flag = 2;
                if (party_id_ == 2) {
                  mpc_op_exec_->createShares(m, sh_min);
                } else {
                  mpc_op_exec_->createShares(sh_min);
                }
              } else {
                // min is p0 or p1
                if (flag == 0) {
                  if (party_id_ == 0) {
                    mpc_op_exec_->createShares(m, sh_min);
                  } else {
                    mpc_op_exec_->createShares(sh_min);
                  }
                } else if (flag == 1) {
                  if (party_id_ == 1) {
                    mpc_op_exec_->createShares(m, sh_min);
                  } else {
                    mpc_op_exec_->createShares(sh_min);
                  }
                }
              }
              eMatrix<double> plain_min(1, 1);
              plain_min = mpc_op_exec_->revealAll(sh_min);
              LOG(WARNING) << "The min value is " << plain_min << " in party "
                           << flag << ".";
              double_col_min = plain_min(0, 0);
            }

            replaceValue(iter, table, col_index, double_col_min, abnormal_index,
                         use_db, true);
          } else {
            LOG(ERROR) << "Can't find value of column " << iter->first << ".";
          }
        } else if (replace_type_ == "AVG") {
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
            replaceValue(iter, table, col_index, col_avg, abnormal_index,
                         use_db, false);
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

            replaceValue(iter, table, col_index, col_avg, abnormal_index,
                         use_db, true);
          } else {
            LOG(ERROR) << "Can't find value of column " << iter->first << ".";
          }
        }
      }
    }
  } catch (std::exception &e) {
    LOG(ERROR) << "In party " << party_id_ << ":\n" << e.what();
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
  AlgorithmBase::finishPartyComm();
  return 0;
}

int MissingProcess::saveModel(void) {
  std::vector<std::string> str_vec;
  auto &new_path = new_dataset_path_;
  auto driver =
      DataDirverFactory::getDriver("CSV", dataset_service_->getNodeletAddr());
  if (driver == nullptr) {
    LOG(ERROR) << "get driver failed, data path: " << new_path;
    return -1;
  }
  auto cursor = driver->initCursor(new_path);
  if (cursor == nullptr) {
    LOG(ERROR) << "init cursor failed, data path: " << new_path;
    return -1;
  }
  VLOG(5) << "A brief view of new table: " << table->ToString() << ".";

  auto dataset = std::make_shared<primihub::Dataset>(table, driver);
  int ret = cursor->write(dataset);
  if (ret != 0) {
    LOG(ERROR) << "Save result to file " << new_path << " failed.";
    return -1;
  }

  service::DatasetMeta meta(dataset, new_dataset_id_,
                            service::DatasetVisbility::PUBLIC, new_path);
  dataset_service_->regDataset(meta);

  return 0;
}

int MissingProcess::_LoadDatasetFromCSV(std::string &dataset_id) {
  auto driver = this->dataset_service_->getDriver(dataset_id,
                                                  this->is_dataset_detail_);
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
    if (pair.second == 1 || pair.second == 2 || pair.second == 3)
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


  }
  return errors ? -1 : array_len;
}

int MissingProcess::_LoadDatasetFromDB(std::string &source) {
  std::shared_ptr<DataDriver> driver = DataDirverFactory::getDriver(
      "sqlite", dataset_service_->getNodeletAddr());

  // auto cursor = driver->initCursor(new_path);
  auto cursor = driver->read(source);
  auto sql_cursor = dynamic_cast<SQLiteCursor *>(cursor.get());
  if (sql_cursor == nullptr) {
    return -1;
  }
  table = sql_cursor->read_from_abnormal(col_and_dtype_, db_both_index);
  if (table == NULL) {
    return -1;
  }
  bool errors = false;
  int num_col = table->num_columns();

  local_col_names = table->ColumnNames();
  for (size_t i = 0; i < local_col_names.size(); i++)
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

  }
  return errors ? -1 : array_len;
}

int MissingProcess::set_task_info(std::string platform_type, std::string job_id,
                                  std::string task_id) {
  platform_type_ = platform_type;
  job_id_ = job_id;
  task_id_ = task_id;
  return 0;
}
} // namespace primihub
