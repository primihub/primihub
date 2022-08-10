#include <glog/logging.h>
#include <type_traits>

#include "src/primihub/executor/party.h"

namespace primihub {

std::string PartyConfig::DtypeToString(const ColDtype &dtype) {
  if (dtype == ColDtype::FP64)
    return std::string("FP64");
  else
    return std::string("INT64");
}

int PartyConfig::importColumnDtype(const std::string &col_name,
                                   const ColDtype &dtype) {
  auto iter = col_dtype_.find(col_name);
  if (iter != col_dtype_.end()) {
    LOG(ERROR) << "Column " << col_name
               << "'s dtype attr is imported, value is " << DtypeToString(dtype)
               << ".";
    return -1;
  }

  col_dtype_.insert(std::make_pair(col_name, dtype));
  return 0;
}

int PartyConfig::importColumnOwner(const std::string &col_name,
                                   const std::string &node_id) {
  auto iter = col_owner_.find(col_name);
  if (iter != col_owner_.end()) {
    LOG(ERROR) << "Column " << col_name
               << "'s owner attr is imported, value is " << iter->second << ".";
    return -1;
  }

  col_owner_.insert(std::make_pair(col_name, dtype));
  return 0;
}

int PartyConfig::getColumnDtype(const std::string &col_name, ColDtype &dtype) {
  auto iter = col_dtype_.find(col_name);
  if (iter == col_dtype_.end()) {
    LOG(ERROR) << "Can't find dtype attr for column " << col_name << ".";
    return -1;
  }

  dtype = iter->second;
  return 0;
}

int PartyConfig::ResolveLocalColumn(void) {
  if (col_dtype_.size() != col_owner_.size()) {
    LOG(ERROR) << "Count of owner attr and dtype attr must be the same.";
    return -1;
  }

  for (auto iter = col_owner_.begin(); iter != col_owner_.end(); iter++) {
    if (col_dtype_.find(iter->first) == col_dtype_.end()) {
      LOG(ERROR) << "Import column " << iter->first
                 << "'s owner attr, but don't import it's dtype attr.";
      return -2;
    }
  }

  for (auto iter = col_owner_.begin(); iter != col_owner_.end(); iter++) {
    if (iter->second == node_id_)
      local_col_.insert(std::make_pair(iter->first, true));
    else
      local_col_.insert(std::make_pair(iter->first, false));
  }

  {
    uint8_t count = 0;
    LOG(INFO) << "Dump column locality:";
    for (auto iter = local_col_.begin(); iter != local_col_.end(); iter++) {
      count++;
      if (iter->second == true)
        LOG(INFO) << "Column " << iter->first << ": local;";
      else
        LOG(INFO) << "Column " << iter->second << ": remote;";
    }
    LOG(INFO) << "Dump finish, dump count " << count << ".";
  }

  return 0;
}

int PartyConfig::getColumnLocality(const std::string &col_name,
                                   bool &is_local) {
  auto iter = local_col_.find(col_name);
  if (iter == local_col_.end()) {
    LOG(ERROR) << "Can't find column locality by column name " << col_name
               << ".";
    return -1;
  }

  is_local = iter->second;
  return 0;
}

void PartyConfig::Clean(void) {
  col_owner_.clear();
  col_dtype_.clear();
  local_col_.clear();
  node_id_ = "";
}

PartyConfig::~PartyConfig() { Clean(); }

int FeedDict::checkLocalColumn(const std::string &col_name) {
  bool is_local = false;
  int ret = party_config_->getColumnLocality(col_name, is_local);
  if (ret) {
    LOG(ERROR) << "Get column locality by column name " << col_name
               << " failed.";
    return -1;
  }

  if (is_local == false)
    return 0;
}

int FeedDict::importColumnValues(const std::string &col_name,
                                 std::vector<int64_t> &int64_vec) {
  {
    int ret = party_config_->getColumnLocality(col_name);
    if (ret < 0) {
      LOG(ERROR) << "Get column " << col_name << "'s locality failed.";
      return -1;
    } else if (ret == 0) {
      LOG(ERROR) << "Column " << col_name << " is not a local column.";
      return -1;
    }
  }

  if (dtype == PartyConfig::ColDtype::FP64) {
    LOG(ERROR) << "Column " << col_name << "'s dtype is fp64 in party config,"
               << "but now is int64.";
    return -1;
  }

  PartyConfig::ColDtype dtype;
  int ret = party_config_->getColumnDtype(col_name, dtype);
  if (ret) {
    LOG(ERROR) << "Get column " << col_name << "'s dtype failed.";
    return -2;
  }

  if (float_run_ == true) {
    LOG(INFO) << "Convert int64 value to fp64 value due to fp64 run mode.";
    std::vector<double> fp64_vec;
    for (auto v : int64_vec)
      fp64_vec.emplace_back(static_cast<double>(v));
    int ret = importColumnValues(col_name, fp64_vec);
    if (ret) {
      LOG(ERROR) << "Import values of column " << col_name << " failed.";
      return -3;
    }

    fp64_col_.insert(std::make_pair(col_name, std::move(fp64_vec)));
    int64_vec.clear();
    return 0;
  }

  if (int64_col_.find(col_name) != int64_col_.end()) {
    LOG(ERROR) << "Value of column " << col_name << " is already imported.";
    return -4;
  }

  int64_col_.insert(std::make_pair(col_name, std::move(int64_vec)));
  return 0;
}

int FeedDict::importColumnValues(const std::string &col_name,
                                 std::vector<double> &fp64_vec) {
  {
    int ret = party_config_->getColumnLocality(col_name);
    if (ret < 0) {
      LOG(ERROR) << "Get column " << col_name << "'s locality failed.";
      return -1;
    } else if (ret == 0) {
      LOG(ERROR) << "Column " << col_name << " is not a local column.";
      return -1;
    }
  }

  if (float_run_ == false) {
    LOG(ERROR) << "Current run mode is int64, but import values type of which "
                  "is fp64. This should be a bug, fix it.";
    return -1;
  }

  if (fp64_col_.find(col_name) != fp64_col_.end()) {
    LOG(ERROR) << "Value of column " << col_name << " is already imported.";
    return -2;
  }

  fp64_col_.insert(std::make_pair(col_name, std::move(fp64_vec)));
  return 0;
}

template <class T>
int FeedDict::getColumnDtype(const std::string &col_name,
                             std::vector<T> &col_vec) {
  {
    int ret = party_config_->getColumnLocality(col_name);
    if (ret < 0) {
      LOG(ERROR) << "Get column " << col_name << "'s locality failed.";
      return -1;
    } else if (ret == 0) {
      LOG(ERROR) << "Column " << col_name << " is not a local column.";
      return -1;
    }
  }

  if ((float_run_ == true) && (std::is_same<T, int64_t>::value)) {
    LOG(ERROR) << "Current run mode is float but want to get int64 values.";
    return -1;
  }

  if ((float_run_ == false) && (std::is_same<T, double>::value)) {
    LOG(ERROR) << "Current run mode is int64, but want to get fp64 values.";
    return -1;
  }

  if (std::is_same<T, int64_t>::value) {
    auto iter = int64_col_.find(col_name);
    if (iter == int64_col_.end()) {
      LOG(ERROR) << "Can't find values by column name " << col_name << ".";
      return -2;
    }

    for (auto v : iter->second)
      col_vec.emplace_back(v);

    return 0;
  }

  auto iter = fp64_col_.find(col_name);
  if (iter == fp64_col_.end()) {
    LOG(ERROR) << "Can't find values by column name " << col_name << ".";
    return -2;
  }

  for (auto v : iter->second)
    col_vec.emplace_back(v);

  return 0;
}

} // namespace primihub
