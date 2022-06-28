/*
 Copyright 2022 Primihub

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

#ifndef SRC_PRIMIHUB_SERVICE_DATASET_LOCALKV_STORAGE_LEVELDB_H_
#define SRC_PRIMIHUB_SERVICE_DATASET_LOCALKV_STORAGE_LEVELDB_H_

#include <string>
#include <leveldb/db.h>
#include "src/primihub/service/dataset/storage_backend.h"

namespace primihub::service {
    class StorageBackendLevelDB : public StorageBackend {
      public:
        StorageBackendLevelDB(std::string path);
        ~StorageBackendLevelDB() override = default;

        outcome::result<void> putValue(Key key, Value value) override;

        outcome::result<Value> getValue(const Key &key) const override;

        outcome::result<void> erase(const Key &key) override;

        outcome::result<std::vector<std::pair<Key, Value>>> getAll() const override;

      private:
        std::string path_;
        leveldb::DB *db_;

    }; // class StorageBackendLevelDB
}  // namespace primihub::service

#endif  // SRC_PRIMIHUB_SERVICE_DATASET_LOCALKV_STORAGE_LEVELDB_H_
