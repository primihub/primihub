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


#include "src/primihub/service/dataset/localkv/storage_leveldb.h"
#include "src/primihub/service/dataset/util.hpp"
#include "src/primihub/service/error.hpp"

namespace primihub::service {

StorageBackendLevelDB::StorageBackendLevelDB(std::string path) : path_(path) {
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, path_, &db_);
    if (!status.ok()) {
        throw Error::INTERNAL_ERROR;
    }
}

outcome::result<void> StorageBackendLevelDB::putValue(Key key, Value value) {
    // Kade::ContentId to leveldb::Slice
    auto key_str = Key2Str(key);
    leveldb::Slice key_slice = key_str;
    leveldb::Status status =
        db_->Put(leveldb::WriteOptions(), key_slice, value);

    return outcome::success();
}

outcome::result<Value> StorageBackendLevelDB::getValue(const Key &key) const {
    std::string value;
    auto key_str = Key2Str(key);
    leveldb::Slice key_slice = key_str;
    leveldb::Status status =
        db_->Get(leveldb::ReadOptions(), key_slice, &value);
    if (!status.ok()) {
        return Error::VALUE_NOT_FOUND;
    }
    return outcome::success(value);
}

outcome::result<void> StorageBackendLevelDB::erase(const Key &key) {
    auto key_str = Key2Str(key);
    leveldb::Slice key_slice = key_str;
    leveldb::Status status = db_->Delete(leveldb::WriteOptions(), key_slice);
    if (!status.ok()) {
        return Error::INTERNAL_ERROR;
    }
    return outcome::success();
}

outcome::result<std::vector<std::pair<Key, Value>>>
StorageBackendLevelDB::getAll() const {
    std::vector<std::pair<Key, Value>> result;
    leveldb::Iterator *it = db_->NewIterator(leveldb::ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        auto key = Str2Key(it->key().ToString());
        auto value = it->value().ToString();
        result.emplace_back(key, value);
    }
    delete it;
    return outcome::success(result);
}

} // namespace primihub::service
