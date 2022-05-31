  
  #include "src/primihub/service/error.hpp"

  #include "src/primihub/service/dataset/localkv/storage_default.h"
  


  namespace primihub::service {
  
  outcome::result<void> StorageBackendDefault::putValue(Key key, Value value) {
    values_.emplace(std::move(key), std::move(value));
    return outcome::success();
  }

  outcome::result<Value> StorageBackendDefault::getValue(const Key &key) const {
    if (auto it = values_.find(key); it != values_.end()) {
      return it->second;
    }
    return Error::VALUE_NOT_FOUND;
  }

  outcome::result<void> StorageBackendDefault::erase(const Key &key) {
    values_.erase(key);
    return outcome::success();
  }


  } // namespace primihub::service
