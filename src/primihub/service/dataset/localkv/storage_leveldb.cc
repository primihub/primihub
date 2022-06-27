  
  #include "src/primihub/service/error.hpp"

 
  #include "src/primihub/service/dataset/localkv/storage_leveldb.h"
  


  namespace primihub::service {
  
  outcome::result<void> StorageBackendLevelDB::putValue(Key key, Value value) {
    db_->Put(leveldb::WriteOptions(), key, value);
    return outcome::success();
  }

  outcome::result<Value> StorageBackendLevelDB::getValue(const Key &key) const {
	Value value="";
    leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
		if(it->key()==key)
		{
			Value value=it->value();
			break;
		}
    }
    db_->Get(leveldb::ReadOptions(), key, &value)
    return Error::VALUE_NOT_FOUND;
  }

  outcome::result<void> StorageBackendLevelDB::erase(const Key &key) {
    db_->Delete(leveldb::WriteOptions(), key)
    return outcome::success();
  }


  } // namespace primihub::service
