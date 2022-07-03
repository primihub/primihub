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

#include "src/primihub/cli/cli.h"
#include <fstream> // std::ifstream
#include <string>

using primihub::rpc::ParamValue;
using primihub::rpc::string_array;
using primihub::rpc::TaskType;

ABSL_FLAG(std::string, server, "127.0.0.1:50050", "server address");
ABSL_FLAG(int, task_type, TaskType::ACTOR_TASK,
          "task type, 0-ACTOR_TASK, 1-PSI_TASK 2-PIR_TASK");
ABSL_FLAG(std::vector<std::string>, params,
          std::vector<std::string>(
              {"BatchSize:INT32:0:128", "NumIters:INT32:0:1",
               "TrainData:STRING:0:train_party_0;train_party_1;train_party_2",
               "TestData:STRING:0:test_party_0;test_party_1;test_party_2",
	       "outputFullFilename:STRING:0:./test.csv"}),
          "task params, format is <name, type, is array, value>");
ABSL_FLAG(std::vector<std::string>, input_datasets,
          std::vector<std::string>({"TrainData", "TestData"}),
          "input datasets name list");
ABSL_FLAG(std::string, job_id, "100", "job id");    // TODO: auto generate
ABSL_FLAG(std::string, task_id, "200", "task id");  // TODO: auto generate

ABSL_FLAG(std::string, task_lang, "proto", "task language, proto or python");
ABSL_FLAG(std::string, task_code, "logistic_regression", "task code");


namespace primihub {

void fill_param(const std::vector<std::string> &params,
                google::protobuf::Map<std::string, ParamValue> *param_map) {
    for (std::string param : params) {
        std::vector<std::string> v;
        std::stringstream ss(param);
        std::string substr;
        for (int i = 0; i < 3; i++) {
            getline(ss, substr, ':');
            v.push_back(substr);
        }
        getline(ss, substr);
        v.push_back(substr);

        ParamValue pv;
        if (v[1] == "INT32") {
            pv.set_var_type(VarType::INT32);
            pv.set_value_int32(std::stoi(v[3]));
        } else if (v[1] == "STRING") {
            pv.set_var_type(VarType::STRING);
            pv.set_value_string(v[3]);
        } else if (v[1] == "BYTE") {
            // TODO
        }
        pv.set_is_array(v[2] == "1" ? true : false);
        (*param_map)[v[0]] = pv;
    }
}

int SDKClient::SubmitTask() {
    PushTaskRequest pushTaskRequest;
    PushTaskReply pushTaskReply;
    grpc::ClientContext context;

    pushTaskRequest.set_intended_worker_id("1");
    pushTaskRequest.mutable_task()->set_type(
        (enum TaskType)absl::GetFlag(FLAGS_task_type));

    // Setup task params
    pushTaskRequest.mutable_task()->set_name("demoTask");

    auto task_lang = absl::GetFlag(FLAGS_task_lang);
    if (task_lang == "proto") {
        pushTaskRequest.mutable_task()->set_language(Language::PROTO);
    } else if (task_lang == "python") {
        pushTaskRequest.mutable_task()->set_language(Language::PYTHON);
    } else {
        std::cerr << "task language not supported" << std::endl;
        return -1;
    }
    
    google::protobuf::Map<std::string, ParamValue> *map =
        pushTaskRequest.mutable_task()->mutable_params()->mutable_param_map();
    const std::vector<std::string> params = absl::GetFlag(FLAGS_params);
    fill_param(params, map);
    // if given task code file, read it and set task code
    auto task_code = absl::GetFlag(FLAGS_task_code);
    if (task_code.find('/') != std::string::npos ||
        task_code.find('\\') != std::string::npos) {
        // read file
        std::ifstream ifs(task_code);
        if (!ifs.is_open()) {
            std::cerr << "open file failed: " << task_code << std::endl;
            return -1;
        }
        std::stringstream buffer;
        buffer << ifs.rdbuf();
        std::cout << buffer.str() << std::endl;
        pushTaskRequest.mutable_task()->set_code(buffer.str());
    } else {
        // read code from command line
        pushTaskRequest.mutable_task()->set_code(task_code);
    }

    // Setup input datasets
    auto input_datasets = absl::GetFlag(FLAGS_input_datasets);
    for (int i = 0; i < input_datasets.size(); i++) {
        pushTaskRequest.mutable_task()->add_input_datasets(input_datasets[i]);
    }
    // TODO Generate job id and task id
    pushTaskRequest.mutable_task()->set_job_id(absl::GetFlag(FLAGS_job_id));
    pushTaskRequest.mutable_task()->set_task_id(absl::GetFlag(FLAGS_task_id));
    pushTaskRequest.set_sequence_number(11);
    pushTaskRequest.set_client_processed_up_to(22);

    LOG(INFO) << " SubmitTask...";

    grpc::Status status =
        stub_->SubmitTask(&context, pushTaskRequest, &pushTaskReply);
    if (status.ok()) {
        LOG(INFO) << "SubmitTask rpc succeeded.";
        if (pushTaskReply.ret_code() == 0) {
            LOG(INFO) << "job_id: " << pushTaskReply.job_id() << " success";
        } else if (pushTaskReply.ret_code() == 1) {
            LOG(INFO) << "job_id: " << pushTaskReply.job_id() << " doing";
        } else {
            LOG(INFO) << "job_id: " << pushTaskReply.job_id() << " error";
        }
    } else {
        LOG(INFO) << "ERROR: " << status.error_message();
        LOG(INFO) << "SubmitTask rpc failed.";
        return -1;
    }
    return 0;
}

} // namespace primihub

int main(int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);
    // Set Color logging on
    FLAGS_colorlogtostderr = true;
    // Set stderr logging on
    FLAGS_alsologtostderr = true;
    // Set log output directory
    FLAGS_log_dir = "./log/";
    // Set log max size (MB)
    FLAGS_max_log_size = 10;
    // Set stop logging if disk full on
    FLAGS_stop_logging_if_full_disk = true;

    absl::ParseCommandLine(argc, argv);

    std::vector<std::string> peers;
    peers.push_back(absl::GetFlag(FLAGS_server));

    for (auto peer : peers) {
        LOG(INFO) << "SDK SubmitTask to: " << peer;
        primihub::SDKClient client(
            grpc::CreateChannel(peer, grpc::InsecureChannelCredentials()));
        if (!client.SubmitTask()) {
            break;
        }
    }

    return 0;
}
