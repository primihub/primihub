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


#include "src/primihub/service/dataset/meta_service/localkv/storage_leveldb.h"

namespace primihub::service {

StorageBackendLevelDB::StorageBackendLevelDB(const std::string& path) : path_(path) {
  // leveldb::Options options;
  // options.create_if_missing = true;
  // leveldb::Status status = leveldb::DB::Open(options, path_, &db_);
  // if (!status.ok()) {
  //     throw Error::INTERNAL_ERROR;
  // }
}

StorageBackendLevelDB::~StorageBackendLevelDB()  {
    std::cout << "Closing leveldb..." << std::endl;
    delete db_;
}


// outcome::result<void> StorageBackendLevelDB::putValue(Key key, Value value) {
//     // Kade::ContentId to leveldb::Slice
//     auto key_str = Key2Str(key);
//     leveldb::Slice key_slice = key_str;
//     try {
//         // leveldb::Status to outcome::result<void>
//         auto status = db_->Put(leveldb::WriteOptions(), key_slice, value);
//         if (!status.ok()) {
//             return Error::INTERNAL_ERROR;
//         }
//     } catch (const std::exception &e) {
//         std::cout << "StorageBackendLevelDB::putValue() exception: " << e.what() << std::endl;
//         return Error::INTERNAL_ERROR;
//     }

//     return outcome::success();
// }

// outcome::result<Value> StorageBackendLevelDB::getValue(const Key &key) const {
//     std::string value;
//     auto key_str = Key2Str(key);
//     leveldb::Slice key_slice = key_str;
//     try {
//         auto status = db_->Get(leveldb::ReadOptions(), key_slice, &value);
//         if (!status.ok()) {
//             return Error::VALUE_NOT_FOUND;
//         }
//     } catch (const std::exception &e) {
//         std::cout << "StorageBackendLevelDB::getValue() exception: " << e.what() << std::endl;
//         return Error::INTERNAL_ERROR;
//     }
//     return outcome::success(value);
// }

// outcome::result<void> StorageBackendLevelDB::erase(const Key &key) {
//     auto key_str = Key2Str(key);
//     leveldb::Slice key_slice = key_str;
//     leveldb::Status status = db_->Delete(leveldb::WriteOptions(), key_slice);
//     if (!status.ok()) {
//         return Error::INTERNAL_ERROR;
//     }
//     return outcome::success();
// }

// outcome::result<std::vector<std::pair<Key, Value>>>
// StorageBackendLevelDB::getAll() const {
//     std::vector<std::pair<Key, Value>> result;
//     leveldb::Iterator *it = db_->NewIterator(leveldb::ReadOptions());
//     for (it->SeekToFirst(); it->Valid(); it->Next()) {
//         auto key = Str2Key(it->key().ToString());
//         auto value = it->value().ToString();
//         result.emplace_back(key, value);
//     }
//     delete it;
//     return outcome::success(result);
// }

retcode StorageBackendLevelDB::PutValue(const Key& key, const Value& value) {
  return retcode::SUCCESS;
}

retcode StorageBackendLevelDB::GetValue(const Key& key, Value* result) {
  return retcode::SUCCESS;
}

retcode StorageBackendLevelDB::GetValue(const std::vector<Key>& keys,
                                        std::map<Key, Value>* result) {
  return retcode::SUCCESS;
}

retcode StorageBackendLevelDB::Erase(const Key& key) {
  return retcode::SUCCESS;
}

retcode StorageBackendLevelDB::GetAll(std::map<Key, Value>* results) {
  return retcode::SUCCESS;
}

} // namespace primihub::service
