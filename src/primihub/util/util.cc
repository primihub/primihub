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
                  std::vector<primihub::rpc::Node>* list) {
  list->clear();
  for (auto iter : peer) {
    DLOG(INFO) << "split param list:" << iter;
    std::vector<std::string> v;
    str_split(iter, &v);
    primihub::rpc::Node node;
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

int getAvailablePort(uint32_t* port) {
    uint32_t tmp_port = 0;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
      return -1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = 0;        // system will allocate one available port when call bind if sin_port is 0
    int ret = bind(sock, (struct sockaddr *) &addr, sizeof addr);
    do {
      if (0 != ret) {     // 4. acquire available port by calling getsockname
        break;
      }
      struct sockaddr_in sockaddr;
      int len = sizeof(sockaddr);
      ret = getsockname(sock, (struct sockaddr *) &sockaddr, (socklen_t *)&len);
      if (0 != ret) {
        break;
      }
      tmp_port = ntohs(sockaddr.sin_port);  // the available socket port get here
      *port = tmp_port;
    } while (false);
    close(sock);
    VLOG(5) << "get available port: " << *port;
    return 0;
}

}  // namespace primihub
