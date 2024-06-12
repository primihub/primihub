/*
 Copyright 2022 PrimiHub

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
#include <limits.h>
#include <libgen.h>
#if __linux__
#include <unistd.h>
#elif __APPLE__
#include <mach-o/dyld.h>
#endif
#include <glog/logging.h>

#include <iomanip>
#include <algorithm>
#include <string>
#include <cctype>

namespace primihub {

void str_split(const std::string& str,
               std::vector<std::string>* v, char delimiter) {
  std::stringstream ss(str);
  while (ss.good()) {
    std::string substr;
    getline(ss, substr, delimiter);
    v->push_back(substr);
  }
}

void str_split(const std::string& str,
               std::vector<std::string>* v,
               const std::string& pattern) {
  std::string::size_type pos1, pos2;
  pos2 = str.find(pattern);
  pos1 = 0;
  while (std::string::npos != pos2) {
    v->push_back(str.substr(pos1, pos2 - pos1));
    pos1 = pos2 + pattern.size();
    pos2 = str.find(pattern, pos1);
  }
  if (pos1 != str.length()) {
    v->push_back(str.substr(pos1));
  }
}

void peer_to_list(const std::vector<std::string>& peer,
                  std::vector<primihub::rpc::Node>* list) {
  list->clear();
  for (auto iter : peer) {
    VLOG(3) << "split param list:" << iter;
    primihub::rpc::Node node;
    parseTopbNode(iter, &node);
    list->push_back(node);
  }
}

void sort_peers(std::vector<std::string>* peers) {
  if (peers->empty() || peers->size() == 1) {
    return;
  }
  auto& peers_ref = *peers;
  std::stable_sort(
    std::begin(peers_ref),
    std::end(peers_ref),
    [](const std::string& first, const std::string& second) -> bool {
      if (first.compare(second) < 0) {
          return true;
      } else {
          return false;
      }
    });
}

retcode pbNode2Node(const rpc::Node& pb_node, Node* node) {
  node->id_ = pb_node.node_id();
  node->ip_ = pb_node.ip();
  node->port_ = pb_node.port();
  node->role_ = pb_node.role();
  node->use_tls_ = pb_node.use_tls();
  return retcode::SUCCESS;
}

retcode node2PbNode(const Node& node, primihub::rpc::Node* pb_node) {
  pb_node->set_node_id(node.id());
  pb_node->set_ip(node.ip());
  pb_node->set_port(node.port());
  pb_node->set_role(node.role());
  pb_node->set_use_tls(node.use_tls());
  return retcode::SUCCESS;
}

retcode parseToNode(const std::string& node_info, Node* node) {
  std::vector<std::string> addr_info;
  str_split(node_info, &addr_info, ':');
  VLOG(5) << "nodelet_attr: " << node_info;
  if (addr_info.size() < 4) {
    return retcode::FAIL;
  }
  node->id_ = addr_info[0];
  node->ip_ = addr_info[1];
  node->port_ = std::stoi(addr_info[2]);
  node->use_tls_ = (addr_info[3] == "1") ? true : false;
  return retcode::SUCCESS;
}

retcode parseTopbNode(const std::string& node_info, rpc::Node* node) {
  std::vector<std::string> addr_info;
  str_split(node_info, &addr_info, ':');
  VLOG(5) << "nodelet_attr: " << node_info;
  if (addr_info.size() < 4) {
    return retcode::FAIL;
  }
  node->set_node_id(addr_info[0]);
  node->set_ip(addr_info[1]);
  node->set_port(std::stoi(addr_info[2]));
  node->set_use_tls((addr_info[3] == "1") ? true : false);
  return retcode::SUCCESS;
}

// void sort_peers(std::vector<std::string>* peers) {
//   std::string str_temp;

//   for (size_t i = 0; i < peers->size(); i++) {
//     for (size_t j = i + 1; j < peers->size(); j++) {
//       if ((*peers)[i].compare((*peers)[j]) > 0) {
//         str_temp = (*peers)[i];
//         (*peers)[i] = (*peers)[j];
//         (*peers)[j] = str_temp;
//       }
//     }
//   }
// }

int getAvailablePort(uint32_t* port) {
  uint32_t tmp_port = 0;
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    return -1;
  }
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = 0;        // system will allocate one available port
                            // when call bind if sin_port is 0
  int ret = bind(sock, (struct sockaddr *) &addr, sizeof addr);
  do {
    if (0 != ret) {     // 4. acquire available port by calling getsockname
      break;
    }
    struct sockaddr_in sockaddr;
    int len = sizeof(sockaddr);
    ret = getsockname(sock,
                      reinterpret_cast<struct sockaddr*>(&sockaddr),
                      reinterpret_cast<socklen_t*>(&len));
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

std::string strToUpper(const std::string& str) {
  std::string upper_str = str;
  // to upper
  std::transform(upper_str.begin(),
                 upper_str.end(), upper_str.begin(), ::toupper);
  return upper_str;
}

std::string strToLower(const std::string& str) {
  std::string lower_str = str;
  // to lower
  std::transform(lower_str.begin(),
                 lower_str.end(), lower_str.begin(), ::tolower);
  return lower_str;
}

std::string getCurrentProcessPath() {
  char path[PATH_MAX];
  memset(path, 0, PATH_MAX);
  [[maybe_unused]] uint32_t size = PATH_MAX;
#if __linux__
  int n = readlink("/proc/self/exe", path, PATH_MAX);
  if (n >= PATH_MAX) {
    n = PATH_MAX - 1;
  }
  path[n] = '\0';
#elif __APPLE__
  _NSGetExecutablePath(path, &size);
  path[size-1] = '\0';
#endif
  return path;
}

std::string getCurrentProcessDir() {
  std::string path = getCurrentProcessPath();
  char* dir_path = dirname(const_cast<char*>(path.c_str()));
  return std::string(dir_path);
}

std::string buf_to_hex_string(const uint8_t* pdata, size_t size) {
  std::stringstream ss;
  ss << std::hex << std::setfill('0');
  for (size_t i = 0; i < size; i++) {
    ss << std::setw(2) << static_cast<unsigned>(pdata[i]);
  }
  return ss.str();
}

void TrimLeft(std::string& str) {
  if (str.empty()) {
    return;
  }
  str.erase(str.begin(),
    std::find_if(str.begin(), str.end(), [](unsigned char ch) {
      return !std::isspace(ch);}));
}

void TrimRight(std::string& str) {
  if (str.empty()) {
    return;
  }
  str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
      return !std::isspace(ch);}).base(), str.end());
}

void TrimAll(std::string& str) {
  if (str.empty()) {
    return;
  }
  str.erase(remove_if(str.begin(), str.end(), [](unsigned char ch) {
      return std::isspace(ch);}), str.end());
}
}  // namespace primihub
