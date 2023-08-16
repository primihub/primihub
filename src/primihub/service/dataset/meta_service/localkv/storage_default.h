// Copyright 2022 PrimiHub
#ifndef SRC_PRIMIHUB_SERVICE_DATASET_LOCALKV_STORAGE_DEFAULT_H_
#define SRC_PRIMIHUB_SERVICE_DATASET_LOCALKV_STORAGE_DEFAULT_H_

#include <unordered_map>
#include <vector>
#include <map>
#include <shared_mutex>
#include "src/primihub/service/dataset/meta_service/localkv/storage_backend.h"

namespace primihub::service {
class StorageBackendDefault : public StorageBackend {
 public:
  StorageBackendDefault() = default;
  ~StorageBackendDefault() override = default;
  retcode PutValue(const Key& key, const Value& value) override;
  retcode GetValue(const Key& key, Value* result) override;
  retcode GetValue(const std::vector<Key>& keys,
                    std::map<Key, Value>* result) override;
  retcode Erase(const Key& key) override;
  retcode GetAll(std::map<Key, Value>* results) override;

 private:
  std::shared_mutex value_guard_;
  std::unordered_map<Key, Value> values_;
};

}  // namespace primihub::service

#endif  // SRC_PRIMIHUB_SERVICE_DATASET_LOCALKV_STORAGE_DEFAULT_H_
