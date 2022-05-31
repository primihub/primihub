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

}  // namespace primihub
