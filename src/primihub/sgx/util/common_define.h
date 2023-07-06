#ifndef SGX_UTIL_COMMON_DEFINE_H_
#define SGX_UTIL_COMMON_DEFINE_H_
#include <string>
namespace sgx {

enum retcode {
  SUCCESS = 0,
  FAIL,
};

struct Node {
  Node() = default;
  Node(const std::string &ip, const uint32_t port, bool use_tls)
      : ip_(ip), port_(port), use_tls_(use_tls), role_("default") {}
  Node(const std::string &ip, const uint32_t port, bool use_tls, const std::string &role)
      : ip_(ip), port_(port), use_tls_(use_tls), role_(role) {}
  Node(Node &&) = default;
  Node(const Node &) = default;
  Node &operator=(const Node &) = default;
  Node &operator=(Node &&) = default;
  std::string to_string() const {
    std::string sep = ":";
    std::string node_info = ip_;
    node_info.append(sep).append(std::to_string(port_))
        .append(sep).append(role_).append(sep).append(use_tls_ ? "1" : "0");
    return node_info;
  }
  inline std::string ip() const { return ip_; }
  inline uint32_t port() const { return port_; }
  inline bool use_tls() const { return use_tls_; }
  inline std::string addr() const { return ip_ + ":" + std::to_string(port_); }
  mutable std::string ip_;
  mutable uint32_t port_;
  mutable bool use_tls_{false};
  std::string role_{"default"};
};

}  // namespace sgx

#endif  // SGX_UTIL_COMMON_DEFINE_H_
