// "Copyright [2023] <Primihub>"
#include "src/primihub/kernel/psi/operator/ecdh_psi.h"

#include <utility>
#include <set>

#include "private_set_intersection/cpp/psi_client.h"
#include "private_set_intersection/cpp/psi_server.h"

#include "src/primihub/common/common.h"
#include "src/primihub/common/value_check_util.h"
#include "src/primihub/util/util.h"
#include "src/primihub/util/endian_util.h"

namespace primihub::psi {
retcode EcdhPsiOperator::OnExecute(const std::vector<std::string>& input,
                                   std::vector<std::string>* result) {
  if (input.empty()) {
    LOG(ERROR) << "no data is set for ecdh psi";
    return retcode::FAIL;
  }
  if (RoleValidation::IsClient(PartyName())) {
    return ExecuteAsClient(input, result);
  } else if (RoleValidation::IsServer(this->PartyName())) {
    return ExecuteAsServer(input);
  } else {
    LOG(ERROR) << "invalid party name: " << this->PartyName() << " "
               << " expected party name: [" << PARTY_CLIENT << ","
               << PARTY_SERVER <<"]";
    return retcode::FAIL;
  }
}

retcode EcdhPsiOperator::ExecuteAsClient(const std::vector<std::string>& input,
    std::vector<std::string>* result) {
  CHECK_TASK_STOPPED(retcode::FAIL);
  SCopedTimer timer;
  rpc::TaskRequest request;
  std::string init_param_str;
  VLOG(5) << "begin to build init param";
  BuildInitParam(input.size(), &init_param_str);
  VLOG(5) << "build init param finished";
  auto init_param_ts = timer.timeElapse();
  auto init_param_time_cost = init_param_ts;
  VLOG(5) << "buildInitParam time cost(ms): " << init_param_time_cost << " "
          << "content length: " << init_param_str.size();
  auto ret = SendInitParam(init_param_str);
  CHECK_RETCODE(ret);
  VLOG(5) << "client begin to prepare psi request";
  // prepare psi data
  auto ts = timer.timeElapse();
  auto client = openminded_psi::PsiClient::CreateWithNewKey(
      reveal_intersection_).value();
  // psi_proto::Request
  auto client_request = client->CreateRequest(input).value();
  // psi_proto::Response server_response;
  auto build_req_ts = timer.timeElapse();
  auto build_req_time_cost = build_req_ts - ts;
  VLOG(5) << "client build request time cost(ms): " << build_req_time_cost;
  // send psi data to server
  rpc::PsiResponse task_response;
  ret = SendPSIRequestAndWaitResponse(std::move(client_request),
                                      &task_response);
  CHECK_RETCODE(ret);
  auto _start = timer.timeElapse();
  ret = this->GetIntersection(input, client, task_response, result);
  CHECK_RETCODE_WITH_ERROR_MSG(ret, "Node psi client get insection failed.");
  auto _end =  timer.timeElapse();
  auto get_intersection_time_cost = _end - _start;
  VLOG(5) << "get intersection time cost(ms): " << get_intersection_time_cost;
  return retcode::SUCCESS;
}

retcode EcdhPsiOperator::GetIntersection(
    const std::vector<std::string> origin_data,
    const std::unique_ptr<openminded_psi::PsiClient>& client,
    rpc::PsiResponse& response,
    std::vector<std::string>* result) {
  CHECK_TASK_STOPPED(retcode::FAIL);
  SCopedTimer timer;
  psi_proto::Response entrpy_response;
  size_t num_response_elements = response.encrypted_elements().size();
  for (size_t i = 0; i < num_response_elements; i++) {
    entrpy_response.add_encrypted_elements(
        response.encrypted_elements()[i]);
  }
  psi_proto::ServerSetup server_setup;
  server_setup.set_bits(response.server_setup().bits());
  if (response.server_setup().data_structure_case() ==
      psi_proto::ServerSetup::DataStructureCase::kGcs) {
    auto *ptr_gcs = server_setup.mutable_gcs();
    ptr_gcs->set_div(response.server_setup().gcs().div());
    ptr_gcs->set_hash_range(response.server_setup().gcs().hash_range());
  } else if (response.server_setup().data_structure_case() ==
              psi_proto::ServerSetup::DataStructureCase::kBloomFilter) {
    auto *ptr_bloom_filter = server_setup.mutable_bloom_filter();
    ptr_bloom_filter->set_num_hash_functions(
        response.server_setup().bloom_filter().num_hash_functions());
  } else {
    LOG(ERROR) << "Node psi client get intersection error!";
    return retcode::FAIL;
  }
  auto build_resp_time_cost = timer.timeElapse();
  VLOG(5) << "build_response_time_cost(ms): " << build_resp_time_cost;

  std::vector<int64_t> intersection =
      std::move(client->GetIntersection(server_setup, entrpy_response)).value();
  auto get_intersection_ts = timer.timeElapse();
  auto get_intersection_time_cost = get_intersection_ts - build_resp_time_cost;
  VLOG(5) << "get_intersection_time_cost: " << get_intersection_time_cost;
  size_t num_intersection = intersection.size();
  for (size_t i = 0; i < num_intersection; i++) {
    result->push_back(origin_data[intersection[i]]);
  }
  return retcode::SUCCESS;
}

retcode EcdhPsiOperator::BuildInitParam(int64_t element_size,
                                        std::string* init_param) {
  CHECK_TASK_STOPPED(retcode::FAIL);
  rpc::Params init_params;
  auto param_map = init_params.mutable_param_map();
  // dataset size
  rpc::ParamValue pv_datasize;
  pv_datasize.set_var_type(rpc::VarType::INT64);
  pv_datasize.set_value_int64(element_size);
  pv_datasize.set_is_array(false);
  (*param_map)["dataset_size"] = pv_datasize;
  // reveal intersection
  rpc::ParamValue pv_intersection;
  pv_intersection.set_var_type(rpc::VarType::INT32);
  pv_intersection.set_value_int32(this->reveal_intersection_ ? 1 : 0);
  pv_intersection.set_is_array(false);
  (*param_map)["reveal_intersection"] = pv_intersection;
  bool success = init_params.SerializeToString(init_param);
  if (!success) {
    LOG(ERROR) << "serialize init param failed";
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode EcdhPsiOperator::SendInitParam(const std::string& init_param) {
  return this->GetLinkContext()->Send(this->key_, this->peer_node_, init_param);
}

retcode EcdhPsiOperator::SendPSIRequestAndWaitResponse(
    psi_proto::Request&& request,
    rpc::PsiResponse* response) {
  CHECK_TASK_STOPPED(retcode::FAIL);
  SCopedTimer timer;
  // send process
  {
    psi_proto::Request client_request = std::move(request);
    std::string psi_req_str;
    client_request.SerializeToString(&psi_req_str);
    VLOG(5) << "begin to send psi request to server";
    auto ret = this->GetLinkContext()->Send(this->key_,
                                            this->peer_node_, psi_req_str);
    if (ret != retcode::SUCCESS) {
      LOG(ERROR) << "send psi request to ["
                 << this->peer_node_.to_string() << "] failed";
      return retcode::FAIL;
    }
    VLOG(5) << "send psi request to server success";
  }

  VLOG(5) << "begin to recv psi response from server";
  std::string recv_data_str;
  auto ret = this->GetLinkContext()->Recv(this->key_,
                                          this->ProxyServerNode(),
                                          &recv_data_str);
  if (ret != retcode::SUCCESS || recv_data_str.empty()) {
    LOG(ERROR) << "recv data is empty";
    return retcode::FAIL;
  }
  VLOG(5) << "successfully recv psi response from server";
  ret = ParsePsiResponseFromeString(recv_data_str, response);
  VLOG(5) << "end of merge task response";
  return ret;
}

retcode EcdhPsiOperator::ParsePsiResponseFromeString(
    const std::string& response_str,
    rpc::PsiResponse* response) {
  CHECK_TASK_STOPPED(retcode::FAIL);
  psi_proto::Response psi_response;
  psi_proto::ServerSetup setup_info;
  char* recv_buf = const_cast<char*>(response_str.c_str());
  uint64_t* be_psi_res_len = reinterpret_cast<uint64_t*>(recv_buf);
  uint64_t psi_res_len = ntohll(*be_psi_res_len);
  recv_buf += sizeof(uint64_t);
  psi_response.ParseFromArray(recv_buf, psi_res_len);
  recv_buf += psi_res_len;
  uint64_t* be_setup_info_len = reinterpret_cast<uint64_t*>(recv_buf);
  uint64_t setup_info_len = ntohll(*be_setup_info_len);
  recv_buf += sizeof(uint64_t);
  setup_info.ParseFromArray(recv_buf, setup_info_len);
  VLOG(5) << "begin to merge task response";

  auto server_setup = response->mutable_server_setup();
  server_setup->set_bits(setup_info.bits());
  if (setup_info.data_structure_case() ==
      psi_proto::ServerSetup::DataStructureCase::kGcs) {
    auto ptr_gcs = server_setup->mutable_gcs();
    ptr_gcs->set_div(setup_info.gcs().div());
    ptr_gcs->set_hash_range(setup_info.gcs().hash_range());
  } else if (setup_info.data_structure_case() ==
              psi_proto::ServerSetup::DataStructureCase::kBloomFilter) {
    auto ptr_bloom_filter = server_setup->mutable_bloom_filter();
    ptr_bloom_filter->set_num_hash_functions(
        setup_info.bloom_filter().num_hash_functions());
  } else {
    LOG(ERROR) << "Node psi client get intersection setup info error!";
    return retcode::FAIL;
  }
  auto elements_size = psi_response.encrypted_elements_size();
  auto encrypted_elements_ptr = response->mutable_encrypted_elements();
  encrypted_elements_ptr->Reserve(elements_size);
  for (auto& encrypted_element : *(psi_response.mutable_encrypted_elements())) {
    response->add_encrypted_elements(std::move(encrypted_element));
  }
  return retcode::SUCCESS;
}

// server method
retcode EcdhPsiOperator::ExecuteAsServer(
    const std::vector<std::string>& input) {
  CHECK_TASK_STOPPED(retcode::FAIL);
  SCopedTimer timer;
  size_t num_client_elements{0};
  bool reveal_intersection_flag{false};
  auto ret = RecvInitParam(&num_client_elements, &reveal_intersection_flag);
  CHECK_RETCODE(ret);
  // prepare for local computation
  VLOG(5) << "sever begin to SetupMessage";
  std::unique_ptr<openminded_psi::PsiServer> server =
      std::move(openminded_psi::PsiServer::CreateWithNewKey(
          reveal_intersection_flag)).value();
  // std::int64_t num_client_elements =
  //     static_cast<std::int64_t>(psi_request.encrypted_elements().size());
  psi_proto::ServerSetup server_setup =
      std::move(server->CreateSetupMessage(fpr_,
                                           num_client_elements,
                                           input)).value();
  VLOG(5) << "sever end of SetupMessage";
  // recv request from client
  VLOG(5) << "server begin to init reauest according to recv data from client";
  psi_proto::Request psi_request;
  ret = InitRequest(&psi_request);
  CHECK_RETCODE(ret);
  auto init_req_ts = timer.timeElapse();
  auto init_req_time_cost = init_req_ts;
  VLOG(5) << "init_req_time_cost(ms): " << init_req_time_cost;
  psi_proto::Response server_response =
      std::move(server->ProcessRequest(psi_request)).value();
  VLOG(5) << "server end of process request, begin to build response";
  PreparePSIResponse(std::move(server_response), std::move(server_setup));
  VLOG(5) << "end of send psi response to client";
  return retcode::SUCCESS;
}

retcode EcdhPsiOperator::InitRequest(psi_proto::Request* psi_request) {
  CHECK_TASK_STOPPED(retcode::FAIL);
  std::string request_str;
  auto ret = this->GetLinkContext()->Recv(this->key_,
                                          this->ProxyServerNode(),
                                          &request_str);
  CHECK_RETCODE_WITH_ERROR_MSG(ret, "receive request from client failed");
  psi_proto::Request recv_psi_req;
  recv_psi_req.ParseFromString(request_str);
  *psi_request = std::move(recv_psi_req);
  return retcode::SUCCESS;
}

retcode EcdhPsiOperator::PreparePSIResponse(psi_proto::Response&& psi_response,
    psi_proto::ServerSetup&& setup_info) {
  CHECK_TASK_STOPPED(retcode::FAIL);
  VLOG(5) << "preparePSIResponse";
  // prepare response to server
  auto server_response = std::move(psi_response);
  auto server_setup = std::move(setup_info);
  std::string response_str;
  server_response.SerializeToString(&response_str);
  std::string setup_info_str;
  server_setup.SerializeToString(&setup_info_str);
  uint64_t response_len = response_str.size();
  uint64_t setup_info_len = setup_info_str.size();
  std::string psi_res_str;
  psi_res_str.reserve(response_len+setup_info_len+2*sizeof(uint64_t));
  uint64_t be_response_len = htonll(response_len);
  psi_res_str.append(TO_CHAR(&be_response_len), sizeof(uint64_t));
  psi_res_str.append(response_str);
  uint64_t be_setup_info_len = htonll(setup_info_len);
  psi_res_str.append(TO_CHAR(&be_setup_info_len), sizeof(uint64_t));
  psi_res_str.append(setup_info_str);
  VLOG(5) << "preparePSIResponse data length: " << psi_res_str.size();
  // pushDataToSendQueue(this->key, std::move(psi_res_str));
  VLOG(5) << "begin to send psi response to client";
  auto ret = this->GetLinkContext()->Send(this->key_,
                                          this->peer_node_, psi_res_str);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "send psi response data to ["
                << this->peer_node_.to_string() << "] failed";
    return retcode::FAIL;
  }
  VLOG(5) << "successfully send psi response to client";
  return retcode::SUCCESS;
}

retcode EcdhPsiOperator::RecvInitParam(size_t* client_dataset_size,
                                       bool* reveal_intersection) {
  CHECK_TASK_STOPPED(retcode::FAIL);
  VLOG(5) << "begin to recvInitParam ";
  std::string init_param_str;
  auto ret = this->GetLinkContext()->Recv(this->key_,
                                          this->ProxyServerNode(),
                                          &init_param_str);
  CHECK_RETCODE_WITH_ERROR_MSG(ret, "receive init param from client failed");
  auto& client_dataset_size_ = *client_dataset_size;
  auto& reveal_flag = *reveal_intersection;
  rpc::Params init_params;
  init_params.ParseFromString(init_param_str);
  const auto& parm_map = init_params.param_map();
  auto it = parm_map.find("dataset_size");
  if (it == parm_map.end()) {
    CHECK_RETCODE_WITH_ERROR_MSG(retcode::FAIL, "parse dataset_size is empty");
  }
  client_dataset_size_ = it->second.value_int64();
  VLOG(5) << "client_dataset_size_: " << client_dataset_size_;
  it = parm_map.find("reveal_intersection");
  if (it == parm_map.end()) {
    CHECK_RETCODE_WITH_ERROR_MSG(retcode::FAIL,
                                 "parse reveal_intersection is empty");
  }
  reveal_flag = it->second.value_int32() > 0;
  VLOG(5) << "reveal_intersection_: " << reveal_flag;
  VLOG(5) << "end of recvInitParam ";
  return retcode::SUCCESS;
}
}  // namespace primihub::psi
