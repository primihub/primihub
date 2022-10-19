#include "src/primihub/algorithm/arithmetic.h"
#include "src/primihub/data_store/csv/csv_driver.h"
#include "src/primihub/data_store/factory.h"
#include <arrow/api.h>
#include <arrow/array.h>
#include <arrow/result.h>
MissingProcess::MissingProcess(PartyConfig &config,
                               std::shared_ptr<DatasetService> dataset_service)
    : AlgorithmBase(dataset_service) {
  this->algorithm_name_ = "missing_val_processing";

  std::map<std::string, Node> &node_map = config.node_map;
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

int MissingProcess::loadParams(primihub::rpc::Task &task) {
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

    res_name_ = param_map["ResFileName"].value_string();
  } catch (std::exception &e) {
    LOG(ERROR) << "Failed to load params: " << e.what();
    return -1;
  }
  return 0;
}
int MissingProcess::loadDataset() {
  int ret = _LoadDatasetFromCSV(data_file_path_);
  // file reading error or file empty
  if (ret <= 0) {
    LOG(ERROR) << "Load dataset for train failed.";
    return -1;
  }
}
int MissingProcess::initPartyComm(void) {
  mpc_op_exec_->setup(next_ip_, prev_ip_, next_port_, prev_port_);
  return 0;
}

int MissingProcess::execute() {
  try {
    arrow::Result<std::shared_ptr<Table>> res_table;
    std::shared_ptr<arrow::Table> new_table;
    std::shared_ptr<arrow::Array> doublearray;
    for (auto itr = correct_col_names.begin(); itr != correct_col_names.end();
         itr++) {
      std::vector<std::string>::iterator t =
          std::find(local_col_names.begin(), local_col_names.end(), *itr);
      double col_sum = 0;
      if (t != local_col_names.end()) {
        int tmp_index = std::distance(local_col_names.begin(), t);
        auto array = std::static_pointer_cast<DoubleArray>(
            table->column(tmp_index)->chunk(0));
        for (int64_t j = 0; j < array->length(); j++) {
          col_sum += array->Value(j);
        }
        col_sum = col_sum / array->length();
      }
      //进行MPC计算，之后得到一个三方和，将和除以3，
      ///把计算结果写入至本列中的null_index中，

      sf64<D16> &sharedFixedInt;

      mpc_op_exec_->CreateShare(col_sum, sharedFixedInt);
      double new_sum = mpc_op_exec_->revealAll(sharedFixedInt);
      new_sum = new_sum / 3;
      ///
      //找到原数据列，并构造新数据列csv_array
      int tmp_index = std::distance(local_col_names.begin(), t);
      auto csv_array = std::static_pointer_cast<DoubleArray>(
          table->column(tmp_index)->chunk(0));
      LOG(INFO) << csv_array->length();
      std::string data_str;
      std::vector<std::string> tmp1;
      std::vector<int> null_index;

      data_str = csv_array->ToString();
      LOG(INFO) << data_str;
      spiltStr(data_str, ",", tmp1);
      for (auto itr = tmp1.begin(); itr != tmp1.end(); itr++) {
        LOG(INFO) << *itr;
      }
      for (int i = 0; i < tmp1.size(); i++) {
        if (tmp1[i].find("null") != tmp1[i].npos) {
          null_index.push_back(i);
        }
      }
      std::vector<double> new_col;
      for (int64_t i = 0; i < csv_array->length(); i++) {
        new_col.push_back(csv_array->Value(i));
      }
      for (auto itr = null_index.begin(); itr != null_index.end(); itr++) {
        new_col[*itr] = new_sum;
        LOG(INFO) << *itr;
      }
      arrow::DoubleBuilder double_builder;
      double_builder.AppendValues(new_col);
      double_builder.Finish(&doublearray);

      arrow::ChunkedArray *new_array = new arrow::ChunkedArray(doublearray);

      auto ptr_Array = std::shared_ptr<arrow::ChunkedArray>(new_array);
      res_table = table->SetColumn(
          tmp_index, arrow::field(*itr, arrow::float64()), ptr_Array);
      table = res_table.ValueUnsafe();
      std::vector<std::string> col_names1 = table->ColumnNames();
      for (auto itr = col_names1.begin(); itr != col_names1.end(); itr++) {
        LOG(INFO) << *itr;
      }
      auto array = std::static_pointer_cast<DoubleArray>(
          table->column(tmp_index)->chunk(0));
      for (int64_t j = 0; j < array->length(); j++) {
        LOG(INFO) << j << ":" << array->Value(j);
      }
    }
  } catch (std::exception &e) {
    LOG(ERROR) << "In party " << party_id_ << ":\n" << e.what() << ".";
  }
  return 0;
}

int MissingProcess::finishPartyComm(void) {
  mpc_op_exec_->fini();
  delete mpc_op_exec_;
  return 0;
}

int ArithmeticExecutor::saveModel(void) {
  std::shared_ptr<arrow::io::FileOutputStream> outfile;
  PARQUET_ASSIGN_OR_THROW(outfile,
                          arrow::io::FileOutputStream::Open(res_name_));
  // The last argument to the function call is the size of the RowGroup in
  // the parquet file. Normally you would choose this to be rather large but
  // for the example, we use a small value to have multiple RowGroups.
  PARQUET_THROW_NOT_OK(parquet::arrow::WriteTable(
      table, arrow::default_memory_pool(), outfile, 3));
  return 0;
}

int MissingProcess::_LoadDatasetFromCSV(std::string &filename) {
  std::cout << "Reading data.parquet at once" << std::endl;
  std::shared_ptr<arrow::io::ReadableFile> infile;
  PARQUET_ASSIGN_OR_THROW(infile, arrow::io::ReadableFile::Open(
                                      filename, arrow::default_memory_pool()));
  std::unique_ptr<parquet::arrow::FileReader> reader;
  PARQUET_THROW_NOT_OK(
      parquet::arrow::OpenFile(infile, arrow::default_memory_pool(), &reader));
  PARQUET_THROW_NOT_OK(reader->ReadTable(&table));
  std::cout << "Loaded " << table->num_rows() << " rows in "
            << table->num_columns() << " columns." << std::endl;
  local_col_names = table->ColumnNames();
  bool errors = false;
  int num_col = table->num_columns();

  // 'array' include values in a column of csv file.
  auto array = std::static_pointer_cast<DoubleArray>(
      table->column(num_col - 1)->chunk(0));
  int64_t array_len = array->length();
  LOG(INFO) << "Label column '" << local_col_names[num_col - 1] << "' has "
            << array_len << " values.";

  // Force the same value count in every column.
  for (int i = 0; i < num_col; i++) {
    auto array =
        std::static_pointer_cast<DoubleArray>(table->column(i)->chunk(0));
    std::vector<double> tmp_data;
    for (int64_t j = 0; j < array->length(); j++) {
      tmp_data.push_back(array->Value(j));
      LOG(INFO) << array->Value(j);
    }
    if (array->length() != array_len) {
      LOG(ERROR) << "Column " << local_col_names[i] << " has "
                 << array->length() << " value, but other column has "
                 << array_len << " value.";
      errors = true;
      break;
    }
    col_and_val_double.insert(
        pair<string, std::vector<double>>(local_col_names[i], tmp_data));
    for (auto itr = col_and_val_double.begin(); itr != col_and_val_double.end();
         itr++) {
      LOG(INFO) << itr->first;
      auto tmp_vec = itr->second;
      for (auto iter = tmp_vec.begin(); iter != tmp_vec.end(); iter++)
        LOG(INFO) << *iter;
    }
  }
  if (errors)
    return -1;
  return array->length();
}
