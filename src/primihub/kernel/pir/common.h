//  "Copyright [2023] <PrimiHub>"
#ifndef SRC_PRIMIHUB_KERNEL_PIR_COMMON_H_
#define SRC_PRIMIHUB_KERNEL_PIR_COMMON_H_
#include <unordered_map>
#include <vector>
#include <string>

namespace primihub::pir {
using PirDataType = std::unordered_map<std::string, std::vector<std::string>>;
enum class PirType {
  ID_PIR = 0,
  KEY_PIR,
};
}  // namespace primihub::pir
#endif  // SRC_PRIMIHUB_KERNEL_PIR_COMMON_H_
