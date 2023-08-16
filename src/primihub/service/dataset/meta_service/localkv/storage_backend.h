/*
 Copyright 2022 PrimiHub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#ifndef SRC_PRIMIHUB_SERVICE_DATASET_LOCALKV_STORAGE_BACKEND_H_
#define SRC_PRIMIHUB_SERVICE_DATASET_LOCALKV_STORAGE_BACKEND_H_

#include <string>
#include <vector>
#include <utility>
#include <map>
#include "src/primihub/common/common.h"
#include "src/primihub/service/dataset/util.hpp"

namespace primihub::service {

/**
 * Backend of key-value storage
 */
class StorageBackend {
 public:
  virtual ~StorageBackend() = default;

  /**
   * Adds @param value corresponding to given @param key.
  */
  virtual retcode PutValue(const Key& key, const Value& value) = 0;

  /**
   * Search for the @return value corresponding to given @param key.
  */
  virtual retcode GetValue(const Key& key, Value* result) = 0;

  /**
   * Search for the @return value corresponding to given @param keys.
  */
  virtual retcode GetValue(const std::vector<Key>& keys,
                            std::map<Key, Value>* result) = 0;

  /**
   * Removes value corresponded to given @param key.
  */
  virtual retcode Erase(const Key& key) = 0;

  /**
   * Get all key and value pairs from the storage.
  */
  virtual retcode GetAll(std::map<Key, Value>* results) = 0;
};  // class StorageBackend

}  // namespace primihub::service

#endif  // SRC_PRIMIHUB_SERVICE_DATASET_LOCALKV_STORAGE_BACKEND_H_
