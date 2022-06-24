  
  #include "src/primihub/service/error.hpp"

 
  #include "src/primihub/service/dataset/localkv/storage_leveldb.h"
  


  namespace primihub::service {
  
  outcome::result<void> StorageBackendLevelDB::putValue(Key key, Value value) {
   // TODO
    return outcome::success();
  }

  outcome::result<Value> StorageBackendLevelDB::getValue(const Key &key) const {
   // TODO
    return Error::VALUE_NOT_FOUND;
  }

  outcome::result<void> StorageBackendLevelDB::erase(const Key &key) {
    // TODO
    return outcome::success();
  }


  } // namespace primihub::service
