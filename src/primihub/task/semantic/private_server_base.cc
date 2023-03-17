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



#include "src/primihub/task/semantic/private_server_base.h"
#include "src/primihub/data_store/factory.h"
#include <fstream>

using arrow::Array;
using arrow::StringArray;
using arrow::Table;

namespace primihub::task {

ServerTaskBase::ServerTaskBase(const Params *params,
                               std::shared_ptr<DatasetService> dataset_service)
: dataset_service_(dataset_service) {
    setTaskParam(params);
}

Params* ServerTaskBase::getTaskParam() {
    return &params_;
}

void ServerTaskBase::setTaskParam(const Params *params) {
    params_.CopyFrom(*params);
}

int ServerTaskBase::loadDatasetFromSQLite(const std::string& conn_str, int data_col,
		                  std::vector<std::string>& col_array, int64_t max_num) {
    std::string nodeaddr("localhost"); // TODO
    // std::shared_ptr<DataDriver>
    auto driver = DataDirverFactory::getDriver("SQLITE", nodeaddr);
    if (driver == nullptr) {
        LOG(ERROR) << "create sqlite db driver failed";
        return -1;
    }
    // std::shared_ptr<Cursor> &cursor
    auto cursor = driver->read(conn_str);
    // std::shared_ptr<Dataset>
    auto ds = cursor->read();
    if (ds == nullptr) {
        return -1;
    }
    auto table = std::get<std::shared_ptr<Table>>(ds->data);
    int num_col = table->num_columns();
    if (num_col < data_col) {
        LOG(ERROR) << "psi dataset colunum number is smaller than data_col";
        return -1;
    }
    auto array = std::static_pointer_cast<StringArray>(table->column(data_col)->chunk(0));
    for (int64_t i = 0; i < array->length(); i++) {
        if (max_num > 0 && max_num == i) {
            break;
        }
        col_array.push_back(array->GetString(i));
    }
    VLOG(5) << "psi server loaded data records: " << col_array.size();
    return array->length();
}

int ServerTaskBase::loadDatasetFromCSV(const std::string& filename, int data_col,
                                       std::vector<std::string> &col_array,
                                       int64_t max_num) {
    std::string nodeaddr("test address"); // TODO
    std::shared_ptr<DataDriver> driver =
        DataDirverFactory::getDriver("CSV", nodeaddr);
    auto cursor = driver->read(filename);
    auto ds = cursor->read();
    std::shared_ptr<Table> table = std::get<std::shared_ptr<Table>>(ds->data);

    int num_col = table->num_columns();
    if (num_col < data_col) {
        LOG(ERROR) << "psi dataset colunum number is smaller than data_col";
        return -1;
    }
    int64_t num_rows = table->num_rows();
    int64_t num_records = max_num > 0 ? max_num : num_rows;
    col_array.reserve(num_records);
    auto col_ptr = table->column(data_col);
    int chunk_size = col_ptr->num_chunks();
    for (int i = 0; i < chunk_size; i++) {
        auto array = std::static_pointer_cast<StringArray>(col_ptr->chunk(i));
        for (size_t j = 0; j < array->length(); j++) {
            col_array.push_back(array->GetString(j));
            if (max_num > 0 && max_num == col_array.size()) {
                return col_array.size();
            }
        }
    }
    return col_array.size();
}

int ServerTaskBase::loadDatasetFromTXT(std::string &filename,
                                       std::vector <std::string> &col_array) {
    LOG(INFO) << "loading file ...";
    std::ifstream infile;
    infile.open(filename);

    col_array.clear();
    std::string tmp;
    std::getline(infile, tmp); // ignore the first line
    while (std::getline(infile, tmp)) {
        col_array.push_back(tmp);
    }

    infile.close();
    return col_array.size();
}

} // namespace primihub::task
