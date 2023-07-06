/*
 Copyright 2022 Primihub

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
#include "src/primihub/task/semantic/psi_task.h"
#include <glog/logging.h>
#include <string>
#include <set>
#include "src/primihub/util/file_util.h"

namespace primihub::task {


bool PsiCommonOperator::isNumeric32Type(const arrow::Type::type& type_id) {
  static std::set<arrow::Type::type> numeric_type_id_set = {
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
bool PsiCommonOperator::isNumeric64Type(const arrow::Type::type& type_id) {
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

bool PsiCommonOperator::isNumeric(const arrow::Type::type& type_id) {
  return isNumeric64Type(type_id) || isNumeric32Type(type_id);
}

bool PsiCommonOperator::isString(const arrow::Type::type& type_id) {
    bool is_string = (type_id == arrow::Type::STRING ||
                        type_id == arrow::Type::BINARY ||
                        type_id == arrow::Type::FIXED_SIZE_BINARY);
    return is_string;
}

bool PsiCommonOperator::validationDataColum(const std::vector<int>& data_cols, int table_max_colums) {
  if (data_cols.size() != table_max_colums) {
    LOG(ERROR) << "data col size does not match, " << " "
        << "expected: " << data_cols.size() << " "
        << " actually: " << table_max_colums;
    return false;
  }
  return true;
}
retcode PsiCommonOperator::LoadDatasetFromTable(std::shared_ptr<arrow::Table> table,
                                                const std::vector<int>& col_index,
                                                std::vector<std::string>* col_data,
                                                std::vector<std::string>* col_name) {
  // load data
  int num_cols = table->num_columns();
  int64_t num_rows = table->num_rows();
  col_data->reserve(num_rows);
  if (num_cols == 0) {
    LOG(ERROR) << "no colum selected";
    return retcode::FAIL;
  }

  // get data from first col
  auto col_ptr = table->column(0);
  int chunk_size = col_ptr->num_chunks();
  auto field_ptr = table->field(0);
  col_name->push_back(field_ptr->name());
  for (int j = 0; j < chunk_size; j++) {
    auto array = std::static_pointer_cast<arrow::StringArray>(col_ptr->chunk(j));
    for (int64_t k = 0; k < array->length(); k++) {
      col_data->push_back(array->GetString(k));
    }
  }
  // get rest data
  for (int i = 1; i < num_cols; i++) {
    auto field_ptr = table->field(i);
    col_name->push_back(field_ptr->name());
    auto col_ptr = table->column(i);
    int chunk_size = col_ptr->num_chunks();
    size_t index = 0;
    for (int j = 0; j < chunk_size; j++) {
      auto array = std::static_pointer_cast<arrow::StringArray>(col_ptr->chunk(j));
      for (int64_t k = 0; k < array->length(); k++) {
        (*col_data)[index].append(array->GetString(k));
        index++;
      }
    }
  }
  VLOG(0) << "data records loaded number: " << col_data->size();
  return retcode::SUCCESS;

}

retcode PsiCommonOperator::LoadDatasetFromTable(
        std::shared_ptr<arrow::Table> table,
        const std::vector<int>& col_index,
        std::vector<std::string>& col_array) {
//
    // int data_col = col_index[0];
    int64_t num_rows = table->num_rows();
    col_array.resize(num_rows);
    int num_cols = table->num_columns();
    for (int col_i = 0; col_i < num_cols; col_i++) {
        auto col_ptr = table->column(col_i);
        int chunk_size = col_ptr->num_chunks();
        auto type_id = col_ptr->type()->id();
        bool is_numeric = this->isNumeric(type_id);
        bool is_string = this->isString(type_id);
        size_t index = 0;
        if (is_numeric) {
          VLOG(5) << "covert numeric to string";
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
                auto array = std::static_pointer_cast<arrow::StringArray>(col_ptr->chunk(i));
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

retcode PsiCommonOperator::LoadDatasetFromCSV(
        const std::string& filename,
        const std::vector<int>& data_col,
        std::vector<std::string>& col_array) {
    return LoadDatasetInternal("CSV", filename, data_col, col_array);
}

retcode PsiCommonOperator::LoadDatasetFromSQLite(
        const std::string &conn_str,
        const std::vector<int>& data_col,
        std::vector <std::string>& col_array) {
    return LoadDatasetInternal("SQLITE", conn_str, data_col, col_array);
}
retcode PsiCommonOperator::LoadDatasetInternal(
        std::shared_ptr<DataDriver>& driver,
        const std::vector<int>& data_cols,
        std::vector<std::string>& col_array) {
//
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
  if(!all_colum_valid) {
    return retcode::FAIL;
  }
  return LoadDatasetFromTable(table, data_cols, col_array);
}

retcode PsiCommonOperator::LoadDatasetInternal(std::shared_ptr<DataDriver>& driver,
                                               const std::vector<int>& col_index,
                                               std::vector<std::string>* col_data,
                                               std::vector<std::string>* col_names) {
//
  auto cursor = driver->GetCursor(col_index);
  if (cursor == nullptr) {
    LOG(ERROR) << "get cursor for dataset failed";
    return retcode::FAIL;
  }
  auto& schema = driver->dataSetAccessInfo()->Schema();
  std::decay_t<decltype(schema)> new_schema{};
  for (const auto index : col_index) {
    new_schema.push_back(schema[index]);
  }
  auto ds = cursor->read(new_schema);
  if (ds == nullptr) {
    LOG(ERROR) << "get data failed";
    return retcode::FAIL;
  }
  auto& table = std::get<std::shared_ptr<arrow::Table>>(ds->data);
  int col_count = table->num_columns();
  bool all_colum_valid = validationDataColum(col_index, col_count);
  if(!all_colum_valid) {
    return retcode::FAIL;
  }
  return LoadDatasetFromTable(table, col_index, col_data, col_names);
}

retcode PsiCommonOperator::LoadDatasetInternal(
        const std::string& driver_name,
        const std::string& conn_str,
        const std::vector<int>& data_cols,
        std::vector<std::string>& col_array) {
    std::string nodeaddr("test address"); // TODO
    std::shared_ptr<DataDriver> driver =
        DataDirverFactory::getDriver(driver_name, nodeaddr);
    auto cursor = driver->read(conn_str);
    auto ds = cursor->read();
    if (ds == nullptr) {
        return retcode::FAIL;
    }
    auto& table = std::get<std::shared_ptr<arrow::Table>>(ds->data);
    int col_count = table->num_columns();
    bool all_colum_valid = validationDataColum(data_cols, col_count);
    if(!all_colum_valid) {
        return retcode::FAIL;
    }
    return LoadDatasetFromTable(table, data_cols, col_array);
}

retcode PsiCommonOperator::saveDataToCSVFile(const std::vector<std::string>& data,
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

}  // namespace primihub::task
