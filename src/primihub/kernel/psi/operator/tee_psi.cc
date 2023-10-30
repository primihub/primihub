// "Copyright [2023] <PrimiHub>"
#include "src/primihub/kernel/psi/operator/tee_psi.h"
#include "src/primihub/util/hash.h"
#include "src/primihub/util/file_util.h"

namespace primihub::psi {
retcode TeePsiOperator::OnExecute(const std::vector<std::string>& input,
                                   std::vector<std::string>* result) {
//
  auto ret{retcode::SUCCESS};
  if (RoleValidation::IsTeeCompute(this->PartyName()) ) {
    ret = ExecuteAsCompute();
  } else {
    if (input.empty()) {
      LOG(ERROR) << "no data is set for tee psi";
      return retcode::FAIL;
    }
    ret = ExecuteAsDataProvider(input, result);
  }
  CleanTempFile();
  VLOG(0) << "end of OnExecute for tee psi";
  return retcode::SUCCESS;
}
// method for data provider
retcode TeePsiOperator::ExecuteAsDataProvider(
    const std::vector<std::string>& input,
    std::vector<std::string>* result) {
//
  // send cipher data to compute party
  Node compute_node;
  auto ret = GetComputeNode(&compute_node);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "GetComputeNode for party: " << PartyName() << " failed";
    return retcode::FAIL;
  }
  sgx::Node remote_node(
      compute_node.ip(), compute_node.port(), compute_node.use_tls());
  auto succ_flag = TeeExecutor()->build_ratls_channel(DataIdPrefix(),
                                                      remote_node);
  if (!succ_flag) {
    LOG(ERROR) << "build_ratls_channel error!";
    return retcode::FAIL;
  }
  LOG(INFO) << "build_ratls_channel success";
  auto key_id = this->GetKey(remote_node.ip());

  std::unordered_map<std::string, std::string> hashed_data(input.size());
  ret = HashData(input, &hashed_data);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "Hash data failed";
    return retcode::FAIL;
  }
  // plain data
  std::string plain_data;
  auto it = std::begin(hashed_data);
  plain_data.reserve(hashed_data.size() * it->first.size());
  for (const auto& [hash_item, orig_item]  : hashed_data) {
    plain_data.append(hash_item);
  }
  //
  std::vector<char> cipher_data;
  ret = EncryptDataWithShareKey(key_id, plain_data, &cipher_data);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "EncryptDataWithShareKey failed";
    return retcode::FAIL;
  }
  // TODO(XXXX)
  Node self_node;
  GetNodeByName(PartyName(), &self_node);
  std::string node_id = self_node.id();
  std::string id = GenerateDataId(DataIdPrefix(), node_id);
  std::string_view send_buf{cipher_data.data(), cipher_data.size()};
  ret = this->GetLinkContext()->Send(id, compute_node, send_buf);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "send EncryptData to "
               << compute_node.to_string() << " failed";
  }
  if (!IsResultReceiver()) {
    return retcode::SUCCESS;
  }
  auto result_id = GenerateResultId();
  std::string cipher_result_buff;
  ret = this->GetLinkContext()->Recv(result_id, &cipher_result_buff);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "recv result failed";
    return retcode::FAIL;
  } else {
    VLOG(2) << "recv result data len:"
              << cipher_result_buff.size() << " bytes successed";
  }
  // decrypt data to plain
  std::vector<char> plain_result;
  ret = DecryptDataWithShareKey(cipher_result_buff, &plain_result);
  if (ret != retcode::SUCCESS) {
    LOG(WARNING) << "DecryptDataWithShareKey failed for: " << PartyName();
    return retcode::SUCCESS;
  }
  if (plain_result.empty()) {
    LOG(WARNING) << "intersection result is empty";
    return retcode::SUCCESS;
  }
  std::string_view palin_result_sv{plain_result.data(), plain_result.size()};
  ret = GetIntersection(palin_result_sv, hashed_data, result);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "GetIntersection failed for: " << PartyName();
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode TeePsiOperator::GetComputeNode(Node* compute_node) {
  return GetNodeByName(PARTY_TEE_COMPUTE, compute_node);
}

retcode TeePsiOperator::HashData(const std::vector<std::string>& input,
    std::unordered_map<std::string, std::string>* hashed_data) {
  std::unordered_set<std::string> dup_filter{input.size()};
  primihub::Hash h_alg;
  if (!h_alg.Init()) {
    LOG(ERROR) << "unknown hash alg:";
    return retcode::FAIL;
  }

  for (const auto& item : input) {
    if (dup_filter.find(item) != dup_filter.end()) {
      continue;
    }
    dup_filter.insert(item);
    hashed_data->insert(std::pair{h_alg.HashToString(item), item});
  }
  return retcode::SUCCESS;
}

retcode TeePsiOperator::EncryptDataWithShareKey(const std::string& key_id,
    const std::string& plain_data,
    std::vector<char>* cipher_data) {
  auto cipher_data_size = TeeExecutor()->get_encrypted_size(plain_data.size());
  // auto encrypted_buf = std::make_unique<char[]>(encrypted_data_size);
  cipher_data->resize(cipher_data_size);
  char* encrypted_buf_ptr = cipher_data->data();
  bool success_flag = TeeExecutor()->encrypt_data_with_sharekey(
      key_id, plain_data.data(), plain_data.size(),
      encrypted_buf_ptr, cipher_data_size);
  if (!success_flag) {
    LOG(ERROR) << "encrypt data with key:" << key_id << " failed!";
    return retcode::FAIL;
  }
  LOG(INFO) << "encrypt data with key:" << key_id << " successed!";
  return retcode::SUCCESS;
}

retcode TeePsiOperator::DecryptDataWithShareKey(const std::string& cipher_data,
                                                std::vector<char>* plain_data) {
  plain_data->clear();
  size_t plain_result_len = 0;
  bool success_flag = TeeExecutor()->get_plain_size(cipher_data.data(),
                                                    cipher_data.size(),
                                                    &plain_result_len);
  if (!success_flag) {
    LOG(ERROR) << "get_plain_size failed:";
    return retcode::FAIL;
  }
  LOG(INFO) << "plain_result_len: " << plain_result_len;
  if (plain_result_len == 0) {
    LOG(WARNING) << "no result need to decrypt";
    return retcode::SUCCESS;
  }
  plain_data->resize(plain_result_len);
  char* plain_result_ptr = plain_data->data();
  Node compute_node;
  GetComputeNode(&compute_node);
  sgx::Node remote_node(
      compute_node.ip(), compute_node.port(), compute_node.use_tls());
  auto key_id = GetKey(remote_node.ip());
  success_flag = TeeExecutor()->decrypt_data_with_sharekey(key_id,
      cipher_data.data(), cipher_data.size(),
      plain_result_ptr, &plain_result_len);

  if (!success_flag) {
    LOG(ERROR) << "encrypt data with key:" << key_id << " failed!";
    retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode TeePsiOperator::GetIntersection(std::string_view result_buf,
    std::unordered_map<std::string, std::string>& hashed_data,
    std::vector<std::string>* result) {
  auto plain_result_buf = result_buf.data();
  size_t plain_result_len = result_buf.size();
  size_t hash_size = hashed_data.begin()->first.size();
  for (size_t i = 0; i < plain_result_len / hash_size; i++) {
    std::string temp{plain_result_buf + i * hash_size, hash_size};
    auto it = hashed_data.find(temp);
    if (it != hashed_data.end()) {
      auto& orig_item = it->second;
      result->push_back(orig_item);
    }
  }
  return retcode::SUCCESS;
}

bool TeePsiOperator::IsResultReceiver() {
  if (RoleValidation::IsClient(PartyName())) {
    return true;
  }
  return false;
}

// method for TEE compute
retcode TeePsiOperator::ExecuteAsCompute() {
  VLOG(0) << "ExecuteAsCompute";
  auto ret = ReceiveDataFromDataProvider();
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "ReceiveDataFromDataProvider encountes error";
    return retcode::FAIL;
  }
  ret = DoCompute(options_.code);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "execute psi failed";
    return retcode::FAIL;
  }
  ret = SendEncryptedResult();
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "SendEncryptedResult encountes error";
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode TeePsiOperator::ReceiveDataFromDataProvider() {
  std::vector<Node> data_provider;
  auto ret = GetDataProviderNode(&data_provider);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "get data provider list failed";
    return retcode::FAIL;
  }
  for (const auto& node : data_provider) {
    std::string recv_buff;
    std::string node_id = node.id();
    auto id = this->GenerateDataId(DataIdPrefix(), node_id);
    auto ret = this->GetLinkContext()->Recv(id, &recv_buff);
    if (ret != retcode::SUCCESS) {
      LOG(ERROR) << "recv data from peer: [" << node_id
                << "] failed";
      return retcode::FAIL;
    } else {
      LOG(INFO) << "recv data from peer: [" << node_id
                << " data len:" << recv_buff.size() << " bytes"
                << "] successed";
    }
    std::string key_id = this->GetKey(node.id());
    std::string data_file = key_id;
    this->SetDataFile(data_file);
    auto ret_val = TeeExecutor()->decrypt_data_with_sharekey_inside(key_id,
        recv_buff.data(), recv_buff.size(), data_file);
    if (!ret_val) {
      LOG(ERROR) << "decrypt_data_with_sharekey_inside into file "
                 << data_file << " error!";
      return retcode::FAIL;
    }
  }
  return retcode::SUCCESS;
}

retcode TeePsiOperator::GetDataProviderNode(
    std::vector<Node>* data_provider_list) {
  auto party_info = options_.party_info;
  for (const auto& [party_name, node] : party_info) {
    if (party_name == PartyName()) {
      continue;
    }
    data_provider_list->push_back(node);
  }
  return retcode::SUCCESS;
}

retcode TeePsiOperator::DoCompute(const std::string& code) {
  std::string result_name = GenerateResultId();
  this->SetDataFile(result_name);
  bool succ_flag = TeeExecutor()->do_compute(this->DataFiles(),
                                             code,
                                             this->ExtraInfo(),
                                             &result_size_);
  if (!succ_flag) {
    LOG(ERROR) << "compute :" << code << " failed!";
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode TeePsiOperator::SendEncryptedResult() {
  Node result_receiver;
  auto ret = GetResultReceiver(&result_receiver);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "GetResultReceiver";
    return retcode::FAIL;
  }
  std::string result_key_id = this->GetKey(result_receiver.id());
  auto cipher_size = TeeExecutor()->get_encrypted_size(this->result_size_);
  LOG(INFO) << "cipher_size:" << cipher_size;
  auto cipher_buf = std::make_unique<char[]>(cipher_size);
  std::string result_name = GenerateResultId();
  auto succ_flag =
      TeeExecutor()->encrypt_data_with_sharekey_inside(result_key_id,
          result_name, result_size_, cipher_buf.get(), cipher_size);
  if (!succ_flag) {
    LOG(ERROR) << "encrypt_data_with_sharekey_inside for result: "
               << result_name << " error!";
    return retcode::FAIL;
  }

  auto result_id = this->GenerateResultId();    // for receiver to receive
  std::string_view cipher_data_sv(cipher_buf.get(), cipher_size);
  ret = this->GetLinkContext()->Send(result_id,
                                     result_receiver, cipher_data_sv);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "send data to receiver: [" << result_receiver.to_string()
               << "] failed";
    return retcode::FAIL;
  }
  LOG(INFO) << "send data to receiver: [" << result_receiver.to_string()
            << "] successed";
  return retcode::SUCCESS;
}

retcode TeePsiOperator::GetResultReceiver(Node* receiver) {
  return GetNodeByName(PARTY_CLIENT, receiver);
}

retcode TeePsiOperator::CleanTempFile() {
  VLOG(5) << "begin to clean temp data";
  for (const auto& file_name : files_) {
    if (primihub::FileExists(file_name)) {
      primihub::RemoveFile(file_name);
      VLOG(5) << "remove file: " << file_name << " success";
    }
  }
}

}  // namespace primihub::psi
