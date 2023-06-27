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

retcode throw_if_file_invalid(const std::string& file_name);
class CSVReader {
 public:
  using UnlabeledData = std::vector<apsi::Item>;
  using LabeledData = std::vector<std::pair<apsi::Item, apsi::Label>>;
  using DBData = std::variant<UnlabeledData, LabeledData>;

  CSVReader() = default;

  explicit CSVReader(const std::string& file_name);

  std::pair<DBData, std::vector<std::string>> read(std::istream& stream);

  std::pair<DBData, std::vector<std::string>> read();
  std::pair<DBData, std::vector<std::string>> read(std::atomic<bool>* stop_flag);

 protected:
    auto pre_process(std::istream& stream) -> std::pair<std::map<std::string, std::string>, bool>;
    std::pair<bool, bool> process_line(const std::string& line,
                                      std::string& item,
                                      std::string& label);
 private:
    std::string file_name_;
    std::pair<bool, bool> process_line(const std::string &line,
                                      std::string &orig_item,
                                      apsi::Item &item,
                                      apsi::Label &label) const;
};  // class CSVReader
}  // namespace primihub::pir::util
#endif  // SRC_PRIMIHUB_TASK_SEMANTIC_PIR_UTIL_H_
