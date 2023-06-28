//  Copyright [2023] <Primihub>
#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_PIR_UTIL_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_PIR_UTIL_H_
// STD
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>
#include <map>

#include "src/primihub/common/common.h"

// APSI
#include "apsi/item.h"
#include "apsi/psi_params.h"
#include "apsi/util/db_encoding.h"

namespace primihub::pir::util {

class CSVReader {
 public:
  using UnlabeledData = std::vector<apsi::Item>;
  using LabeledData = std::vector<std::pair<apsi::Item, apsi::Label>>;
  using DBData = std::variant<UnlabeledData, LabeledData>;
  CSVReader() = default;

};  // class CSVReader
}  // namespace primihub::pir::util
#endif  // SRC_PRIMIHUB_TASK_SEMANTIC_PIR_UTIL_H_
