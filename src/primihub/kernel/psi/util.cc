/*
 Copyright 2022 PrimiHub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */
#include "src/primihub/kernel/psi/util.h"
#include <glog/logging.h>
#include <string>
#include <set>
#include <future>
#include <thread>
#include <utility>
#include <algorithm>
#include <map>
#include "src/primihub/util/file_util.h"
#include "src/primihub/util/util.h"

namespace primihub::psi {

bool PsiCommonUtil::isNumeric32Type(const arrow::Type::type& type_id) {
  static std::set<arrow::Type::type>
  numeric_type_id_set = {
    arrow::Type::UINT8,
    arrow::Type::INT8,
    arrow::Type::UINT16,
    arrow::Type::INT16,
    arrow::Type::UINT32,
    arrow::Type::INT32,
  };
  if (numeric_type_id_set.find(type_id) != numeric_type_id_set.end()) {
    return true;
  } else {
    return false;
  }
}
bool PsiCommonUtil::isNumeric64Type(const arrow::Type::type& type_id) {
  static std::set<arrow::Type::type> numeric_type_id_set = {
    arrow::Type::UINT64,
    arrow::Type::INT64,
  };
  if (numeric_type_id_set.find(type_id) != numeric_type_id_set.end()) {
    return true;
  } else {
    return false;
  }
}

bool PsiCommonUtil::isNumeric(const arrow::Type::type& type_id) {
  return isNumeric64Type(type_id) || isNumeric32Type(type_id);
}

bool PsiCommonUtil::isString(const arrow::Type::type& type_id) {
    bool is_string = (type_id == arrow::Type::STRING ||
                      type_id == arrow::Type::BINARY ||
                      type_id == arrow::Type::FIXED_SIZE_BINARY);
    return is_string;
}

bool PsiCommonUtil::IsValidDataType(const arrow::Type::type& type_id) {
  if (isNumeric(type_id) || isString(type_id)) {
    return true;
  } else {
    return false;
  }
}

bool PsiCommonUtil::validationDataColum(const std::vector<int>& data_cols,
                                        int table_max_colums) {
  if (data_cols.size() != table_max_colums) {
    LOG(ERROR) << "data col size does not match, " << " "
        << "expected: " << data_cols.size() << " "
        << " actually: " << table_max_colums;
    return false;
  }
  return true;
}

retcode PsiCommonUtil::LoadDatasetFromTable(
    std::shared_ptr<arrow::Table> table,
    const std::vector<int>& col_index,
    std::vector<std::string>* col_data,
    std::vector<std::string>* col_name) {
  // load data
  SCopedTimer timer;
  int num_cols = table->num_columns();
  int64_t num_rows = table->num_rows();

  col_data->resize(num_rows);
  if (num_cols == 0) {
    LOG(ERROR) << "no column selected";
    return retcode::FAIL;
  }

  // get data from first col
  auto col_ptr = table->column(0);
  int chunk_size = col_ptr->num_chunks();
  if (chunk_size == 1) {
    ExtractDataFromArray(table, col_data, col_name);
  } else {
    ExtractDataFromTrunkArray(table, col_data, col_name);
  }
  auto time_cost = timer.timeElapse();
  VLOG(5) << "LoadDatasetFromTable time cost: " << time_cost;
  VLOG(0) << "data records loaded number: " << col_data->size();
  return retcode::SUCCESS;
}

retcode PsiCommonUtil::LoadDatasetFromTable(
    std::shared_ptr<arrow::Table> table,
    const std::vector<int>& col_index,
    std::vector<std::string>& col_array) {
  // int data_col = col_index[0];
  int64_t num_rows = table->num_rows();
  col_array.resize(num_rows);
  int num_cols = table->num_columns();
  SCopedTimer timer;
  for (int col_i = 0; col_i < num_cols; col_i++) {
    auto col_ptr = table->column(col_i);
    int chunk_size = col_ptr->num_chunks();
    auto type_id = col_ptr->type()->id();
    bool is_numeric = this->isNumeric(type_id);
    bool is_string = this->isString(type_id);
    size_t index = 0;
    if (is_numeric) {
      VLOG(5) << "convert numeric to string";
      if (this->isNumeric64Type(type_id)) {
        for (int i = 0; i < chunk_size; i++) {
          auto array = std::static_pointer_cast<
                  arrow::NumericArray<arrow::Int64Type>>(col_ptr->chunk(i));
          for (int64_t j = 0; j < array->length(); j++) {
            std::string value = std::to_string(array->Value(j));
            col_array[index].append(value);
            index++;
          }
        }
      } else if (this->isNumeric32Type(type_id)) {
        for (int i = 0; i < chunk_size; i++) {
          auto array = std::static_pointer_cast<
                  arrow::NumericArray<arrow::Int32Type>>(col_ptr->chunk(i));
          for (int64_t j = 0; j < array->length(); j++) {
            std::string value = std::to_string(array->Value(j));
            col_array[index].append(value);
            index++;
          }
        }
      } else {
        LOG(ERROR) << "unknown type: " << static_cast<int>(type_id);
        return retcode::FAIL;
      }
    } else if (is_string) {
      for (int i = 0; i < chunk_size; i++) {
        auto array =
            std::static_pointer_cast<arrow::StringArray>(col_ptr->chunk(i));
        for (int64_t j = 0; j < array->length(); j++) {
          col_array[index].append(array->GetString(j));
          index++;
        }
      }
    } else {
      LOG(ERROR) << "unsupported data type for Psi: type: " << type_id;
      return retcode::FAIL;
    }
  }
  return retcode::SUCCESS;
}

retcode PsiCommonUtil::LoadDatasetInternal(
    std::shared_ptr<DataDriver>& driver,
    const std::vector<int>& data_cols,
    std::vector<std::string>& col_array) {
  auto cursor = driver->GetCursor(data_cols);
  if (cursor == nullptr) {
    LOG(ERROR) << "get cursor for dataset failed";
    return retcode::FAIL;
  }
  auto ds = cursor->read();
  if (ds == nullptr) {
    LOG(ERROR) << "get data failed";
    return retcode::FAIL;
  }
  auto& table = std::get<std::shared_ptr<arrow::Table>>(ds->data);
  int col_count = table->num_columns();
  bool all_colum_valid = validationDataColum(data_cols, col_count);
  if (!all_colum_valid) {
    return retcode::FAIL;
  }
  return LoadDatasetFromTable(table, data_cols, col_array);
}

retcode PsiCommonUtil::LoadDatasetInternal(
    std::shared_ptr<DataDriver>& driver,
    const std::vector<int>& col_index,
    std::vector<std::string>* col_data,
    std::vector<std::string>* col_names,
    const std::string& data_set_id,
    const std::string& trace_id) {
  auto cursor = driver->GetCursor(col_index);
  if (cursor == nullptr) {
    LOG(ERROR) << "get cursor for dataset failed";
    return retcode::FAIL;
  }
  cursor->SetDatasetId(data_set_id);
  cursor->SetTraceId(trace_id);
  auto& schema = driver->dataSetAccessInfo()->Schema();
  // construct new schema and check data type for each selected columns,
  // float type is not allowed
  std::decay_t<decltype(schema)> new_schema{};
  for (const auto index : col_index) {
    auto filed = schema[index];
    auto& type = std::get<1>(filed);
    if (!IsValidDataType(static_cast<arrow::Type::type>(type))) {
      auto arrow_schema = driver->dataSetAccessInfo()->ArrowSchema();
      auto data_type = arrow_schema->field(index);
      LOG(ERROR) << "data type: " << data_type->ToString()
                 << " is not supported";
      return retcode::FAIL;
    }
    type = arrow::Type::type::STRING;
    new_schema.push_back(std::move(filed));
  }
  auto ds = cursor->read(new_schema);
  if (ds == nullptr) {
    LOG(ERROR) << "get data failed";
    return retcode::FAIL;
  }
  auto& table = std::get<std::shared_ptr<arrow::Table>>(ds->data);
  int col_count = table->num_columns();
  bool all_colum_valid = validationDataColum(col_index, col_count);
  if (!all_colum_valid) {
    return retcode::FAIL;
  }
  return LoadDatasetFromTable(table, col_index, col_data, col_names);
}

retcode PsiCommonUtil::LoadDatasetInternal(
    const std::string& driver_name,
    const std::string& conn_str,
    const std::vector<int>& data_cols,
    std::vector<std::string>& col_array) {
  std::string nodeaddr("test address");
  // std::shared_ptr<DataDriver>
  auto driver = DataDirverFactory::getDriver(driver_name, nodeaddr);
  auto cursor = driver->read(conn_str);
  auto ds = cursor->read();
  if (ds == nullptr) {
      return retcode::FAIL;
  }
  auto& table = std::get<std::shared_ptr<arrow::Table>>(ds->data);
  int col_count = table->num_columns();
  bool all_colum_valid = validationDataColum(data_cols, col_count);
  if (!all_colum_valid) {
      return retcode::FAIL;
  }
  return LoadDatasetFromTable(table, data_cols, col_array);
}

retcode PsiCommonUtil::SaveDataToCSVFile(
    const std::vector<std::string>& data,
    const std::string& file_path,
    const std::vector<std::string>& col_names) {
  std::vector<std::shared_ptr<arrow::Array>> arrow_array;
  if (col_names.size() == 1) {
    arrow::StringBuilder builder;
    builder.AppendValues(data);
    std::shared_ptr<arrow::Array> array;
    builder.Finish(&array);
    arrow_array.push_back(std::move(array));
  } else {
    std::vector<std::vector<std::string>> result_data;
    result_data.resize(col_names.size());
    for (auto& item : result_data) {
      item.resize(data.size());
    }
    for (size_t i = 0; i < data.size(); i++) {
      std::vector<std::string> vec;
      str_split(data[i], &vec, DATA_RECORD_SEP);
      if (vec.size() != col_names.size()) {
        LOG(ERROR) << "data column does not match, expected: "
                   << col_names.size()
                   << " but get: " << vec.size();
        return retcode::FAIL;
      }
      for (int j = 0; j < vec.size(); j ++) {
        result_data[j][i] = std::move(vec[j]);
      }
    }
    for (auto& col_data : result_data) {
      arrow::StringBuilder builder;
      builder.AppendValues(col_data);
      std::shared_ptr<arrow::Array> array;
      builder.Finish(&array);
      arrow_array.push_back(std::move(array));
    }
  }

  std::vector<std::shared_ptr<arrow::Field>> schema_vector;
  for (const auto& col_name : col_names) {
    schema_vector.push_back(arrow::field(col_name, arrow::int64()));
  }
  auto schema = std::make_shared<arrow::Schema>(schema_vector);
  // std::shared_ptr<arrow::Table>
  auto table = arrow::Table::Make(schema, arrow_array);
  auto driver = DataDirverFactory::getDriver("CSV", "test address");
  auto csv_driver = std::dynamic_pointer_cast<CSVDriver>(driver);
  if (ValidateDir(file_path)) {
    LOG(ERROR) << "can't access file path: " << file_path;
    return retcode::FAIL;
  }
  int ret = csv_driver->write(table, file_path);
  if (ret != 0) {
    LOG(ERROR) << "Save PSI result to file " << file_path << " failed.";
    return retcode::FAIL;
  }
  LOG(INFO) << "Save PSI result to " << file_path << ".";
  return retcode::SUCCESS;
}

retcode PsiCommonUtil::saveDataToCSVFile(
    const std::vector<std::string>& data,
    const std::string& file_path, const std::string& col_title) {
  arrow::MemoryPool *pool = arrow::default_memory_pool();
  arrow::StringBuilder builder(pool);
  builder.AppendValues(data);
  std::shared_ptr<arrow::Array> array;
  builder.Finish(&array);
  std::vector<std::shared_ptr<arrow::Field>> schema_vector = {
      arrow::field(col_title, arrow::int64())};
  auto schema = std::make_shared<arrow::Schema>(schema_vector);
  std::shared_ptr<arrow::Table> table = arrow::Table::Make(schema, {array});
  auto driver = DataDirverFactory::getDriver("CSV", "test address");
  auto csv_driver = std::dynamic_pointer_cast<CSVDriver>(driver);
  if (ValidateDir(file_path)) {
    LOG(ERROR) << "can't access file path: " << file_path;
    return retcode::FAIL;
  }
  int ret = csv_driver->write(table, file_path);
  if (ret != 0) {
    LOG(ERROR) << "Save PSI result to file " << file_path << " failed.";
    return retcode::FAIL;
  }
  LOG(INFO) << "Save PSI result to " << file_path << ".";
  return retcode::SUCCESS;
}

retcode PsiCommonUtil::ExtractDataFromTrunkArray(
    std::shared_ptr<arrow::Table>& table,
    std::vector<std::string>* col_data,
    std::vector<std::string>* col_name) {
  int64_t num_rows = table->num_rows();
  int num_cols = table->num_columns();
  auto col_ptr = table->column(0);
  int chunk_size = col_ptr->num_chunks();
  auto field_ptr = table->field(0);
  col_name->push_back(field_ptr->name());
  VLOG(7) << " num_rows: " << num_rows
          << " chunk_size: " << chunk_size;
  // get cpu core info
  size_t cpu_core_num = std::thread::hardware_concurrency();
  int32_t use_core_num = cpu_core_num / 2 - 1;
  if (use_core_num > 10) {
    use_core_num = 10;
  }
  int32_t thread_num = std::max(use_core_num, 1);
  if (chunk_size <= thread_num) {
    thread_num = chunk_size;
  }
  // key: trucnk index, value: element number in trunk
  std::map<int32_t, int64_t> trunk_index_map;
  int64_t start_index = 0;
  for (int j = 0; j < chunk_size; j++) {
    auto array =
        std::static_pointer_cast<arrow::StringArray>(col_ptr->chunk(j));
    trunk_index_map[j] = array->length();
  }
  int32_t chunk_per_thread = chunk_size / thread_num;
  if (chunk_per_thread * thread_num < chunk_size) {
    thread_num++;
  }
  VLOG(7) << "chunk_per_thread: " << chunk_per_thread
          << " thread_num: " << thread_num;
  // process trunk data parallel
  std::vector<std::future<void>> futs;
  for (int i = 0; i < thread_num; i++) {
    int32_t chunk_s = i * chunk_per_thread;
    int32_t chunk_e = (i == thread_num - 1) ? chunk_size :
                                              (i+1) * chunk_per_thread;
    VLOG(7) << "chunk_s: " << chunk_s << " chunk_e: " << chunk_e;
    futs.push_back(std::async(
        std::launch::async,
        [&, chunk_s, chunk_e]() {
          if (chunk_e < chunk_s) {
            return;
          }
          int64_t index = 0;
          for (int32_t i = 0; i < chunk_s; i++) {
            index += trunk_index_map[i];
          }
          VLOG(7) << "start data index: " << index;
          auto& col_data_ref = *(col_data);
          for (int j = chunk_s; j < chunk_e; j++) {
            auto array =
                std::static_pointer_cast<arrow::StringArray>(col_ptr->chunk(j));
            for (int64_t k = 0; k < array->length(); k++) {
              col_data_ref[index] = array->GetString(k);
              index++;
            }
          }
        }));
  }
  for (auto&& fut : futs) {
    fut.get();
  }

  // get rest data
  for (int i = 1; i < num_cols; i++) {
    auto field_ptr = table->field(i);
    col_name->push_back(field_ptr->name());
    auto col_ptr = table->column(i);
    int chunk_size = col_ptr->num_chunks();
    if (chunk_size != trunk_index_map.size()) {
      LOG(ERROR) << "trunk size does not match";
      return retcode::FAIL;
    }
    std::vector<std::future<void>> futs;
    for (int i = 0; i < thread_num; i++) {
      int32_t chunk_s = i * chunk_per_thread;
      int32_t chunk_e = (i == thread_num - 1) ? chunk_size :
                                                (i+1) * chunk_per_thread;
      VLOG(7) << "chunk_s: " << chunk_s << " chunk_e: " << chunk_e;
      futs.push_back(std::async(
          std::launch::async,
          [&, chunk_s, chunk_e]() {
            int64_t index = 0;
            for (int32_t i = 0; i < chunk_s; i++) {
              index += trunk_index_map[i];
            }
            VLOG(7) << "start data index: " << index;
            auto& col_data_ref = *(col_data);
            for (int j = chunk_s; j < chunk_e; j++) {
              auto array =
                  std::static_pointer_cast<arrow::StringArray>(
                      col_ptr->chunk(j));
              for (int64_t k = 0; k < array->length(); k++) {
                col_data_ref[index].append(DATA_RECORD_SEP)
                                   .append(array->GetString(k));
                index++;
              }
            }
          }));
    }
    for (auto&& fut : futs) {
      fut.get();
    }
  }
}

retcode PsiCommonUtil::ExtractDataFromArray(
    std::shared_ptr<arrow::Table>& table,
    std::vector<std::string>* col_data,
    std::vector<std::string>* col_name) {
  int64_t num_rows = table->num_rows();
  int num_cols = table->num_columns();
  auto col_ptr = table->column(0);
  int chunk_size = col_ptr->num_chunks();
  if (chunk_size != 1) {
    LOG(ERROR) << "using ExtractDataFromTrunkArray instead";
    return retcode::FAIL;
  }
  auto field_ptr = table->field(0);
  col_name->push_back(field_ptr->name());
  VLOG(7) << " num_rows: " << num_rows
          << " chunk_size: " << chunk_size;
  // get cpu core info
  size_t cpu_core_num = std::thread::hardware_concurrency();
  int32_t use_core_num = cpu_core_num / 2 - 1;
  int32_t thread_num = std::max(use_core_num, 1);

  auto array = std::static_pointer_cast<arrow::StringArray>(col_ptr->chunk(0));
  int64_t element_number = array->length();
  if (element_number < 100000 || element_number <= thread_num) {
    thread_num = 1;
  }
  int32_t element_per_thread = element_number / thread_num;
  if (element_per_thread * thread_num < element_number) {
    thread_num++;
  }
  VLOG(7) << "element_per_thread: " << element_per_thread;
  // process trunk data parallel
  std::vector<std::future<void>> futs;
  for (int i = 0; i < thread_num; i++) {
    int64_t i_start = i * element_per_thread;
    int64_t i_end = std::min<int64_t>(((i+1) * element_per_thread),
                                      element_number);
    VLOG(7) << "start index: " << i_start << " end index: " << i_end;
    futs.push_back(std::async(
        std::launch::async,
        [&, i_start, i_end]() {
          VLOG(7) << "start data index: " << i_start;
          auto& col_data_ref = *(col_data);
          for (int64_t j = i_start; j < i_end; j++) {
            col_data_ref[j] = array->GetString(j);
          }
        }));
  }
  for (auto&& fut : futs) {
    fut.get();
  }

  // get rest data
  for (int i = 1; i < num_cols; i++) {
    auto field_ptr = table->field(i);
    col_name->push_back(field_ptr->name());
    auto col_ptr = table->column(i);
    int chunk_size = col_ptr->num_chunks();
    if (chunk_size != 1) {
      LOG(ERROR) << "trunk size does not match, expected 1,"
                 << "but get " << chunk_size;
      return retcode::FAIL;
    }
    auto array =
        std::static_pointer_cast<arrow::StringArray>(col_ptr->chunk(0));
    std::vector<std::future<void>> futs;
    for (int i = 0; i < thread_num; i++) {
      int64_t i_start = i * element_per_thread;
      int64_t i_end = std::min<int64_t>(((i+1) * element_per_thread),
                                        element_number);
      VLOG(7) << "start index: " << i_start << " end index: " << i_end;
      futs.push_back(std::async(
          std::launch::async,
          [&, i_start, i_end]() {
            VLOG(7) << "start data index: " << i_start;
            auto& col_data_ref = *(col_data);
            for (int64_t k = i_start; k < i_end; k++) {
              col_data_ref[k].append(DATA_RECORD_SEP)
                             .append(array->GetString(k));
            }
          }));
    }
    for (auto&& fut : futs) {
      fut.get();
    }
  }
}
}  // namespace primihub::psi
