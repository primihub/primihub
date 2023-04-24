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
// MACRO or CONSTANT defination
#ifdef __linux__
#define SET_THREAD_NAME(name)  prctl(PR_SET_NAME, name)
#else
#define SET_THREAD_NAME(name)
#endif

// macro defination
static const char* SCHEDULER_NODE = "SCHEDULER_NODE";
static int WAIT_TASK_WORKER_READY_TIMEOUT_MS = 5*1000;
static int CACHED_TASK_STATUS_TIMEOUT_S = 5;
static int SCHEDULE_WORKER_TIMEOUT_S = 20;
// common type defination
using u64 = uint64_t;
using i64 = int64_t;
using u32 = uint32_t;
using i32 = int32_t;
using u16 = uint16_t;
using i16 = int16_t;
using u8 = uint8_t;
using i8 = int8_t;

enum class Channel_Status {
    Normal,
    Closing,
    Closed,
    Canceling
};

enum class Errc_Status {
    success = 0
};

enum class retcode {
    SUCCESS = 0,
    FAIL,
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
        if (v.size() != 5) {
            return retcode::FAIL;
        }
        id_ = v[0];
        ip_ = v[1];
        port_ = std::stoi(v[2]);
        use_tls_ = (v[3] == "1" ? true : false);
        role_ = v[4];
        return retcode::SUCCESS;
    }
    std::string ip() const {return ip_;}
    uint32_t port() const {return port_;}
    bool use_tls() const {return use_tls_;}
    std::string id() const {return id_;}
    std::string role() const {return role_;}

    std::string id_{"defalut"};
    std::string ip_;
    uint32_t port_;
    bool use_tls_{false};
    std::string role_{"default"};
};

struct DatasetMetaInfo {
  std::string id;
  std::string driver_type;
  std::string access_info;
  std::vector<std::tuple<std::string, int>> schema;
};

}  // namespace primihub
#endif  // SRC_PRIMIHUB_COMMON_COMMON_H_
