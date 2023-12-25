// Copyright [2023] <primihub.com>
#ifndef SRC_PRIMIHUB_COMMON_COMMON_H_
#define SRC_PRIMIHUB_COMMON_COMMON_H_
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include <string>
#include <sstream>
#include <vector>
namespace primihub {
// MACRO or CONSTANT definition
#ifdef __linux__
#define SET_THREAD_NAME(name)  prctl(PR_SET_NAME, name)
#else
#define SET_THREAD_NAME(name)
#endif
#define TO_CHAR(ptr) reinterpret_cast<char*>(ptr)
#define TO_UCHAR(ptr) reinterpret_cast<unsigned char*>(ptr)

[[maybe_unused]] static uint16_t ABY3_TOTAL_PARTY_NUM = 3;
[[maybe_unused]] static uint64_t LIMITED_PACKAGE_SIZE = 3 * 1024 * 1024;  // 4M
// macro definition
[[maybe_unused]] static const char* ROLE_CLIENT = "CLIENT";
[[maybe_unused]] static const char* ROLE_SCHEDULER = "SCHEDULER";
[[maybe_unused]] static const char* SCHEDULER_NODE = "SCHEDULER_NODE";
[[maybe_unused]] static const char* PROXY_NODE = "PROXY_NODE";
[[maybe_unused]] static const char* AUX_COMPUTE_NODE = "AUXILIARY_COMPUTE_NODE";
[[maybe_unused]] static const char* PARTY_CLIENT = "CLIENT";
[[maybe_unused]] static const char* PARTY_SERVER = "SERVER";
[[maybe_unused]] static const char* PARTY_TEE_COMPUTE = "TEE_COMPUTE";
[[maybe_unused]] static const char* DEFAULT = "DEFAULT";
[[maybe_unused]] static const char* DATA_RECORD_SEP = "####";

[[maybe_unused]] static int WAIT_TASK_WORKER_READY_TIMEOUT_MS = 5*1000;
[[maybe_unused]] static int CACHED_TASK_STATUS_TIMEOUT_S = 5;
[[maybe_unused]] static int SCHEDULE_WORKER_TIMEOUT_S = 20;
[[maybe_unused]] static int CONTROL_CMD_TIMEOUT_S = 5;
[[maybe_unused]] static int GRPC_RETRY_MAX_TIMES = 3;
// common type definition
using u64 = uint64_t;
using i64 = int64_t;
using u32 = uint32_t;
using i32 = int32_t;
using u16 = uint16_t;
using i16 = int16_t;
using u8 = uint8_t;
using i8 = int8_t;

enum class retcode {
  SUCCESS = 0,
  FAIL,
};

/**
 * vibility for dataset
 * public: vibiliable for all party
 * private: vibiable only for the party who public
*/
enum class Visibility {
  PUBLIC = 0,
  PRIVATE = 1,
};

struct TaskInfo {
  std::string job_id;
  std::string task_id;
  std::string request_id;
  std::string sub_task_id;
};

enum class Role {
  CLIENT = 1,
  SERVER = 2,
};

class RoleValidation {
public:
  static bool IsClient(Role role) {
    return role == Role::CLIENT;
  }

  static bool IsServer(Role role) {
    return role == Role::SERVER;
  }

  static bool IsClient(const std::string& party_name) {
    return party_name == PARTY_CLIENT;
  }

  static bool IsServer(const std::string& party_name) {
    return party_name == PARTY_SERVER;
  }

  static bool IsTeeCompute(const std::string& party_name) {
    return party_name == PARTY_TEE_COMPUTE;
  }
  static bool IsAuxiliaryCompute(const std::string& party_name) {
    return party_name == AUX_COMPUTE_NODE;
  }
};


struct Node {
  Node() = default;
  Node(const std::string& id, const std::string& ip,
       const uint32_t port, bool use_tls)
       : id_(id), ip_(ip), port_(port), use_tls_(use_tls), role_("default") {}
  Node(const std::string& ip, const uint32_t port, bool use_tls)
       : ip_(ip), port_(port), use_tls_(use_tls), role_("default") {}
  Node(const std::string& ip, const uint32_t port, bool use_tls, const std::string& role)
       : ip_(ip), port_(port), use_tls_(use_tls), role_(role) {}
  Node(const std::string& id, const std::string& ip, const uint32_t port,
       bool use_tls, const std::string& role)
       : id_(id), ip_(ip), port_(port), use_tls_(use_tls), role_(role) {}
  Node(Node&&) = default;
  Node(const Node&) = default;
  Node& operator=(const Node&) = default;
  Node& operator=(Node&&) = default;
  bool operator==(const Node& item) {
    return (this->ip() == item.ip()) &&
        (this->port() == item.port()) &&
        (this->use_tls() == item.use_tls());
  }
  std::string to_string() const {
    std::string sep = ":";
    std::string node_info = id_;
    node_info.append(sep).append(ip_)
        .append(sep).append(std::to_string(port_))
        .append(sep).append(use_tls_ ? "1" : "0")
        .append(sep).append(role_);
    return node_info;
  }
  retcode fromString(const std::string& node_info) {
    char delimiter = ':';
    std::vector<std::string> v;
    std::stringstream ss(node_info);
    while (ss.good()) {
      std::string substr;
      getline(ss, substr, delimiter);
      v.push_back(substr);
    }
    id_ = v[0];
    ip_ = v[1];
    port_ = std::stoi(v[2]);
    use_tls_ = (v[3] == "1" ? true : false);
    if (v.size() == 5) {
      role_ = v[4];
    }
    return retcode::SUCCESS;
  }

  std::string ip() const {return ip_;}
  uint32_t port() const {return port_;}
  bool use_tls() const {return use_tls_;}
  std::string id() const {return id_;}
  std::string role() const {return role_;}

 public:
  std::string id_;
  std::string ip_;
  uint32_t port_{0};
  bool use_tls_{false};
  std::string role_;
};

struct DatasetMetaInfo {
  std::string id;
  std::string driver_type;
  std::string access_info;
  Node location;
  Visibility visibility;
  std::vector<std::tuple<std::string, int>> schema;
};

}  // namespace primihub
#endif  // SRC_PRIMIHUB_COMMON_COMMON_H_
