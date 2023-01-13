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

#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_PSI_TASK_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_PSI_TASK_H_
#include "src/primihub/common/defines.h"
#include "arrow/api.h"
#include "src/primihub/data_store/factory.h"
namespace primihub::task {
class PsiCommonOperator {
 public:
     bool isNumeric(const arrow::Type::type& type_id);
     bool isString(const arrow::Type::type& type_id);
     bool validationDataColum(const std::vector<int>& data_cols, int table_max_colums);
     retcode LoadDatasetFromTable(std::shared_ptr<arrow::Table> table,
                                const std::vector<int>& col_index,
                                std::vector<std::string>& col_array);
     retcode LoadDatasetFromCSV(const std::string& filename,
                                   const std::vector<int>& data_col,
                                   std::vector<std::string>& col_array);
     retcode LoadDatasetFromSQLite(const std::string &conn_str,
                                   const std::vector<int>& data_col,
                                   std::vector<std::string>& col_array);
     retcode LoadDatasetInternal(std::shared_ptr<DataDriver>& driver,
                                 const std::vector<int>& data_col,
                                 std::vector<std::string>& col_array);
     retcode LoadDatasetInternal(const std::string& driver_name,
                                   const std::string& conn_str,
                                   const std::vector<int>& data_cols,
                                   std::vector <std::string>& col_array);
     retcode saveDataToCSVFile(const std::vector<std::string>& data,
                              const std::string& file_path,
                              const std::string& col_title);

};
}  // namespace primihub::task
#endif  // SRC_PRIMIHUB_TASK_SEMANTIC_PSI_TASK_H_