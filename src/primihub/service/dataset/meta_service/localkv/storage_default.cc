// Copyright 2022 PrimiHub
#include "src/primihub/service/dataset/meta_service/localkv/storage_default.h"
#include <utility>

namespace primihub::service {

retcode StorageBackendDefault::PutValue(const Key& key, const Value& value) {
  std::lock_guard<std::shared_mutex> lck(this->value_guard_);
  values_.emplace(key, value);
  return retcode::SUCCESS;
}

retcode StorageBackendDefault::GetValue(const Key& key, Value* result) {
  std::shared_lock<std::shared_mutex> lck(this->value_guard_);
  if (auto it = values_.find(key); it != values_.end()) {
    *result = it->second;
    return retcode::SUCCESS;
  }
  LOG(ERROR) << "key: " << key << " is not found";
  return retcode::FAIL;;
}

retcode StorageBackendDefault::GetValue(const std::vector<Key>& keys,
                                        std::map<Key, Value>* result) {
  auto& result_container = *result;
  for (const auto& key : keys) {
    Value value;
    auto ret = GetValue(key, &value);
    if (ret != retcode::SUCCESS) {
      return retcode::FAIL;
    }
  }
  return retcode::SUCCESS;
}

retcode StorageBackendDefault::Erase(const Key& key) {
  std::lock_guard<std::shared_mutex> lck(this->value_guard_);
  auto it = values_.find(key);
  if (it != values_.end()) {
    values_.erase(key);
  }
  return retcode::SUCCESS;
}

retcode StorageBackendDefault::GetAll(std::map<Key, Value>* results) {
  std::shared_lock<std::shared_mutex> lck(this->value_guard_);
  for (auto& [key, value] : values_) {
    results->insert({key, value});
  }
  return retcode::SUCCESS;
}

}  // namespace primihub::service
