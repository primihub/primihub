  
#include "src/primihub/service/error.hpp" 
#include "src/primihub/service/dataset/localkv/storage_leveldb.h"
#include "src/primihub/service/dataset/util.hpp"

namespace primihub::service {
  
  StorageBackendLevelDB::StorageBackendLevelDB() {
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, "./localdb", &db_);
    if (!status.ok()) {
      throw Error::INTERNAL_ERROR;
    }
  }
  
  outcome::result<void> StorageBackendLevelDB::putValue(Key key, Value value) {
    // Kade::ContentId to leveldb::Slice
    auto key_str = Key2Str(key);
    leveldb::Slice key_slice = key_str;
    leveldb::Status status = db_->Put(leveldb::WriteOptions(), key_slice, value);

    return outcome::success();
  }

  outcome::result<Value> StorageBackendLevelDB::getValue(const Key &key) const {
    std::string value;
    auto key_str = Key2Str(key);
    leveldb::Slice key_slice = key_str;
    leveldb::Status status = db_->Get(leveldb::ReadOptions(), key_slice, &value);
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


} // namespace primihub::service
