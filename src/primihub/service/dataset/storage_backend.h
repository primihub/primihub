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



#ifndef SRC_PRIMIHUB_DATA_STORE_STORGE_BACKEND_H_
#define SRC_PRIMIHUB_DATA_STORE_STORGE_BACKEND_H_


#include <string>
#include <libp2p/protocol/kademlia/content_id.hpp>

#include "src/primihub/service/outcome.hpp"

namespace kade = libp2p::protocol::kademlia;
namespace primihub::service {
  
  using Key = kade::ContentId;
  using Value = std::string;

  /**
   * Backend of key-value storage
   */

  class StorageBackend {
   public:
    virtual ~StorageBackend() = default;

    /// Adds @param value corresponding to given @param key.
    virtual outcome::result<void> putValue(Key key, Value value) = 0;

    /// Searches for the @return value corresponding to given @param key.
    virtual outcome::result<Value> getValue(const Key &key) const = 0;

    /// Removes value corresponded to given @param key.
    virtual outcome::result<void> erase(const Key &key) = 0;

    // Get all key and value pairs from the storage.
    virtual outcome::result<std::vector<std::pair<Key, Value>>> getAll() const = 0;

  };  // class StorageBackend


}  // namespace primihub::service

#endif  // SRC_PRIMIHUB_DATA_STORE_STORGE_BACKEND_H_
