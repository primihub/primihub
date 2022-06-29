  
  
  #ifndef SRC_PRIMIHUB_DATA_STORE_LOCALKV_STORGE_DEFAULT_H_
  #define SRC_PRIMIHUB_DATA_STORE_LOCALKV_STORGE_DEFAULT_H_
  
  #include <unordered_map>
  #include "src/primihub/service/dataset/storage_backend.h"

  namespace primihub::service {
  
  class StorageBackendDefault : public StorageBackend {
   public:
    StorageBackendDefault() = default;
    ~StorageBackendDefault() override = default;

    outcome::result<void> putValue(Key key, Value value) override;

    outcome::result<Value> getValue(const Key &key) const override;

    outcome::result<void> erase(const Key &key) override;

    outcome::result<std::vector<std::pair<Key, Value>>> getAll() const override;

   private:
    std::unordered_map<Key, Value> values_;
    
  };


  } // namespace primihub::service

  #endif  // SRC_PRIMIHUB_DATA_STORE_LOCALKV_STORGE_DEFAULT_H_
