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


#include "src/primihub/util/util.h"
#include "src/primihub/util/file_util.h"
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <glog/logging.h>

namespace primihub {

void str_split(const std::string& str, std::vector<std::string>* v,
               char delimiter) {
  std::stringstream ss(str);

  while (ss.good()) {
      std::string substr;
      getline(ss, substr, delimiter);
      v->push_back(substr);
  }
}

void peer_to_list(const std::vector<std::string>& peer,
                  std::vector<Node>* list) {
  list->clear();
  for (auto iter : peer) {
    DLOG(INFO) << "split param list:" << iter;
    std::vector<std::string> v;
    str_split(iter, &v);
    Node node;
    node.set_node_id(v[0]);
    node.set_ip(v[1]);
    node.set_port(std::stoi(v[2]));
    // node.set_data_port(std::stoi(v[3])); // FIXME (chenhongbo):? why comment ?
    list->push_back(node);
  }
}



void sort_peers(std::vector<std::string>* peers) {
  std::string str_temp;

  for (size_t i = 0; i < peers->size(); i++) {
    for (size_t j = i + 1; j < peers->size(); j++) {
      if ((*peers)[i].compare((*peers)[j]) > 0) {
        str_temp = (*peers)[i];
        (*peers)[i] = (*peers)[j];
        (*peers)[j] = str_temp;
      }
    }
  }
}
std::shared_ptr<grpc::Channel> buildChannel(const std::string& dest_node_address,
                                            const std::string& current_node_id, bool use_tls) {
  std::shared_ptr<grpc::ChannelCredentials> creds{nullptr};
  if (use_tls) {
    std::string root_ca_file_path = "data/cert/ca.crt";
    std::string cert_file_path = "data/cert/" + current_node_id + ".crt";
    std::string key_file_path = "data/cert/" + current_node_id + ".key";
    VLOG(5) << "root_ca_file_path: " << root_ca_file_path << " "
      << "cert_file_path: " << cert_file_path << " "
      << "key_file_path: " << key_file_path;
    std::string key = getFileContents(key_file_path);
    std::string cert = getFileContents(cert_file_path);
    std::string root_ca = getFileContents(root_ca_file_path);
    grpc::SslCredentialsOptions ssl_opts;
    ssl_opts.pem_root_certs = root_ca;
    ssl_opts.pem_private_key = key;
    ssl_opts.pem_cert_chain = cert;
    creds = grpc::SslCredentials(ssl_opts);
  } else {
    creds = grpc::InsecureChannelCredentials();
  }
  return grpc::CreateChannel(dest_node_address, creds);  // maybe using customized method
}
}  // namespace primihub
