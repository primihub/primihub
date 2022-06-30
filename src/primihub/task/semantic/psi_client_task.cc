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

#include "private_set_intersection/cpp/psi_client.h"

#include "src/primihub/task/semantic/psi_client_task.h"
#include "src/primihub/data_store/factory.h"


using namespace std;
using arrow::Table;
using arrow::StringArray;
using arrow::DoubleArray;
using arrow::Int64Builder;
using primihub::rpc::VarType;

namespace primihub::task {

int validateDir(string file_path) {
    int pos = file_path.find_last_of('/');
    string path;
    if (pos > 0) {
        path = file_path.substr(0, pos);
	if (access(path.c_str(), 0) == -1) {
	    string cmd = "mkdir -p " + path;
	    int ret = system(cmd.c_str());
	    if (ret)
	        return -1;
	}
    }
    return 0;
}

PSIClientTask::PSIClientTask(const std::string &node_id,
                             const std::string &job_id,
                             const std::string &task_id, 
                             const TaskParam *task_param, 
                             std::shared_ptr<DatasetService> dataset_service)
    : TaskBase(task_param, dataset_service), node_id_(node_id),
      job_id_(job_id), task_id_(task_id) {}

int PSIClientTask::_LoadParams(Task &task) {
    auto param_map = task.params().param_map();

    reveal_intersection_ = true;
    try {
        data_index_ = param_map["clientIndex"].value_int32();
        psi_type_ = param_map["psiType"].value_int32();
        dataset_path_ = param_map["clientData"].value_string();
        result_file_path_ = param_map["outputFullFilename"].value_string();

        server_index_ = param_map["serverIndex"];
        server_address_ = param_map["serverAddress"].value_string();
        server_dataset_ = param_map[server_address_].value_string();

    } catch (std::exception &e) {
        LOG(ERROR) << "Failed to load params: " << e.what();
        return -1;
    }
    return 0;
}

int PSIClientTask::_LoadDatasetFromCSV(std::string &filename,
                        int data_col,
                        std::vector <std::string> &col_array) {
    std::string nodeaddr("test address"); // TODO
    std::shared_ptr<DataDriver> driver =
        DataDirverFactory::getDriver("CSV", nodeaddr);
    std::shared_ptr<Cursor> &cursor = driver->read(filename);
    std::shared_ptr<Dataset> ds = cursor->read();
    std::shared_ptr<Table> table = std::get<std::shared_ptr<Table>>(ds->data);

    int num_col = table->num_columns();
    if (num_col < data_col) {
        LOG(ERROR) << "psi dataset colunum number is smaller than data_col";
        return -1;
    }

    auto array = std::static_pointer_cast<StringArray>(
        table->column(data_col)->chunk(0));
    for (int64_t i = 0; i < array->length(); i++) {
        col_array.push_back(array->GetString(i));
    }
    return 0;
}

int PSIClientTask::_LoadDataset(void) {
    int ret = _LoadDatasetFromCSV(dataset_path_, data_index_, elements_);
    if (ret) {
        LOG(ERROR) << "Load dataset for psi client failed.";
        return -1;
    }
    return 0;
}

int PSIClientTask::_GetIntsection(const std::unique_ptr<PsiClient> &client, 
                              ExecuteTaskResponse & taskResponse) {
    psi_proto::Response entrpy_response;
    const std::int64_t num_response_elements =
        static_cast<std::int64_t>(taskResponse.psi_response().encrypted_elements().size());
    for (std::int64_t i = 0; i < num_response_elements; i++) {
        entrpy_response.add_encrypted_elements(
            taskResponse.psi_response().encrypted_elements()[i]);
    }

    psi_proto::ServerSetup server_setup;
    server_setup.set_bits(taskResponse.psi_response().server_setup().bits());
    if (taskResponse.psi_response().server_setup().data_structure_case() ==
        psi_proto::ServerSetup::DataStructureCase::kGcs) {
        auto *ptr_gcs = server_setup.mutable_gcs();
        ptr_gcs->set_div(taskResponse.psi_response().server_setup().gcs().div());
        ptr_gcs->set_hash_range(taskResponse.psi_response().server_setup().gcs().hash_range());
    } else if (taskResponse.psi_response().server_setup().data_structure_case() ==
               psi_proto::ServerSetup::DataStructureCase::kBloomFilter) {
        auto *ptr_bloom_filter = server_setup.mutable_bloom_filter();
        ptr_bloom_filter->set_num_hash_functions(
            taskResponse.psi_response().server_setup().bloom_filter().num_hash_functions()
        );
    } else {
        LOG(ERROR) << "Node psi client get intersection error!";
        return -1;
    }

    std::vector <int64_t> intersection =
        std::move(client->GetIntersection(server_setup, entrpy_response)).value();

    const std::int64_t num_intersection =
        static_cast<std::int64_t>(intersection.size());

    if (psi_type_ == PsiType::DIFFERENCE) {
	map<int64_t, int> inter_map;
        std::int64_t num_elements = static_cast<std::int64_t>(elements_.size());
        for (std::int64_t i = 0; i < num_intersection; i++) {
            inter_map[intersection[i]] = 1;
        }
        for (std::int64_t i = 0; i < num_elements; i++) {
            if (inter_map.find(i) == inter_map.end()) {
                // outFile << i << std::endl;
                result_.push_back(i);
            }
        }
    } else {
        for (std::int64_t i = 0; i < num_intersection; i++) {
            // outFile << intersection[i] << std::endl;
            result_.push_back(intersection[i]);
        }
    }
    return 0;
}

int PSIClientTask::execute() {
    int ret = _LoadParams(task_param_);
    if (ret) {
        LOG(ERROR) << "Psi client load task params failed.";
        return ret;
    }

    ret = _LoadDataset();
    if (ret) {
        LOG(ERROR) << "Psi client load dataset failed.";
        return ret;
    }

    std::unique_ptr<PsiClient> client = 
        std::move(PsiClient::CreateWithNewKey(reveal_intersection_)).value();
    psi_proto::Request client_request = 
        std::move(client->CreateRequest(elements_)).value();
    psi_proto::Response server_response;

    ClientContext context;
    ExecuteTaskRequest taskRequest;
    ExecuteTaskResponse taskResponse;

    PsiRequest *ptr_request = taskRequest.mutable_psi_request();
    ptr_request->set_reveal_intersection(client_request.reveal_intersection());
    const std::int64_t num_elements =
        static_cast<std::int64_t>(client_request.encrypted_elements().size());
    for (std::int64_t i = 0; i < num_elements; i++) {
        ptr_request->add_encrypted_elements(client_request.encrypted_elements()[i]);
    }

    auto *ptr_params = taskRequest.mutable_params()->mutable_param_map();
    ParamValue pv;
    pv.set_var_type(VarType::STRING);
    pv.set_value_string(server_dataset_);
    (*ptr_params)["serverData"] = pv;
    (*ptr_params)["serverIndex"] = server_index_;

    std::unique_ptr<VMNode::Stub> stub = VMNode::NewStub(grpc::CreateChannel(
        server_address_, grpc::InsecureChannelCredentials()));

    Status status =
        stub->ExecuteTask(&context, taskRequest, &taskResponse);
    if (status.ok()) {
        if (taskResponse.psi_response().ret_code()) {
            LOG(ERROR) << "Node psi server process request error.";
            return -1;
        }

        int ret = _GetIntsection(client, taskResponse);
        if (ret) {
            LOG(ERROR) << "Node psi client get insection failed.";
            return -1;
        }

        ret = saveResult();
        if (ret) {
            LOG(ERROR) << "Save psi result failed.";
            return -1;
        }
    } else {
        LOG(ERROR) << "Node push psi server task rpc failed.";
        LOG(ERROR) << status.error_code() << ": " << status.error_message();
        return -1;
    }

    return 0;
}

int PSIClientTask::saveResult() {
    arrow::MemoryPool *pool = arrow::default_memory_pool();
    arrow::Int64Builder builder(pool);

    for (std::int64_t i = 0; i < result_.size(); i++) {
        builder.Append(result_[i]);
    }

    std::shared_ptr<arrow::Array> array;
    builder.Finish(&array);

    std::string col_title = 
        psi_type_ == PsiType::DIFFERENCE ? "difference_row" : "intersection_row";
    std::vector<std::shared_ptr<arrow::Field>> schema_vector = {
        arrow::field(col_title, arrow::int64())};
    auto schema = std::make_shared<arrow::Schema>(schema_vector);
    std::shared_ptr<arrow::Table> table = arrow::Table::Make(schema, {array});

    std::shared_ptr<DataDriver> driver =
        DataDirverFactory::getDriver("CSV", "psi result");
    std::shared_ptr<CSVDriver> csv_driver =
        std::dynamic_pointer_cast<CSVDriver>(driver);


    if (validateDir(result_file_path_)) {
        LOG(ERROR) << "can't access file path: "
                   << result_file_path_;
        return -1;
    }
    int ret = csv_driver->write(table, result_file_path_);

    if (ret != 0) {
        LOG(ERROR) << "Save PSI result to file " << result_file_path_ << " failed.";
        return -1;
    }

    LOG(INFO) << "Save PSI result to " << result_file_path_ << ".";
    return 0;
}

} // namespace primihub::task
