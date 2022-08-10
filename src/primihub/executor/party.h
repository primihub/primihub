#ifndef SRC_PRIMIHUB_EXECUTOR_PARTY_H_
#define SRC_PRIMIHUB_EXECUTOR_PARTY_H_

#include <cstlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace primihub {
class PartyConfig {
public:
  enum ColDtype { INT64, FP64 };

  PartyConfig(std::string node_id) { node_id_ = node_id; }
  ~PartyConfig();

  int importColumnDtype(const std::string &col_name, const ColDtype &dtype);
  int importColumnOwner(const std::string &col_name,
                        const std::string &node_id);
  int getColumnDtype(const std::string &col_name, ColDtype &dtype);
  int getColumnLocality(bool &is_local);
  int resolveLocalColumn(void);
  void Clean(void);

  static std::string DtypeToString(const ColDtype &dtype);

private:
  std::string node_id_;
  std::map<std::string, std::string> col_owner_;
  std::map<std::string, ColDtype> col_dtype_;
  std::map<std::string, bool> local_col_;
};

class FeedDict {
public:
  FeedDict(const std::string &node_id, const PartyConfig *party_config,
           bool float_run) {
    node_id_ = node_id;
    party_config_ = party_config;
    float_run_ = float_run;
  }

  ~FeedDict();

  int importColumnValues(const std::string &col_name,
                         std::vector<int64_t> &int64_vec);
  int importColumnValues(const std::string &col_name,
                         std::vector<double> &fp64_vec);
  template <class T>
  int getColumnValues(const std::string &col_name, std::vector<T> &col_vec);

private:
  int checkLocalColumn(const std::string &col_name);

  bool float_run_;
  std::string node_id_;
  std::map<std::string, std::vector<double>> fp64_col_;
  std::map<std::string, std::vector<int64_t>> int64_col_;
  const PartyConfig *party_config_;
};
} // namespace primihub

#endif
