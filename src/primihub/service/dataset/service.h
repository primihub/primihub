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

#ifndef SRC_PRIMIHUB_SERVICE_DATASET_SERVICE_H_
#define SRC_PRIMIHUB_SERVICE_DATASET_SERVICE_H_

#include <mutex>
#include <string>

#include "src/primihub/data_store/dataset.h"
#include "src/primihub/data_store/driver.h"
#include "src/primihub/p2p/node_stub.h"
#include "src/primihub/service/dataset/model.h"
#include "src/primihub/service/dataset/storage_backend.h"
#include "src/primihub/service/outcome.hpp"

namespace primihub::service {
  using DatasetWithParamTag = std::pair<std::string, std::string>;
  using DatasetMetaWithParamTag =
    std::pair<std::shared_ptr<primihub::service::DatasetMeta>, std::string>;
using FoundMetaHandler =
    std::function<void(std::shared_ptr<primihub::service::DatasetMeta> &)>;
using ReadDatasetHandler =
    std::function<void(std::shared_ptr<primihub::Dataset> &)>;
using FoundMetaListHandler = std::function<void(
    std::vector<DatasetMetaWithParamTag> &)>;



class DatasetMetaService;
class DatasetService {
  public:
    explicit DatasetService(std::shared_ptr<primihub::p2p::NodeStub> stub,
                            std::shared_ptr<StorageBackend> localkv) {
        metaService_ = std::make_shared<DatasetMetaService>(stub, localkv);
    }
    ~DatasetService() {}

    std::shared_ptr<primihub::Dataset>
    newDataset(std::shared_ptr<primihub::DataDriver> driver,
               const std::string &description,
               DatasetMeta &meta); // output

    void regDataset(DatasetMeta &meta);

    outcome::result<void> readDataset(const DatasetId &id,
                                      ReadDatasetHandler handler);

    outcome::result<void> findDataset(const DatasetId &id,
                                      FoundMetaHandler handler);

    int deleteDataset(const DatasetId &id);
    void loadDefaultDatasets(const std::string &config_file_path);
    void restoreDatasetFromLocalStorage(const std::string &nodelet_addr);
    void setMetaSearchTimeout(unsigned int timeout);
    // void
    // findPeerListFromDatasets(const std::vector<std::string> &dataset_namae_list,
    //                          FoundMetaListHandler handler);

    // TODO
    void listDataset() {}

    void updateDataset(const std::string &id, const std::string &description,
                       const std::string &schema_url) {}
    // DatasetSchema &getDatasetSchema(const std::string &id) const {}

    //  private:
    std::shared_ptr<DatasetMetaService> metaService_;
};

class DatasetMetaService {
  public:
    DatasetMetaService(std::shared_ptr<primihub::p2p::NodeStub> p2pStub,
                       std::shared_ptr<StorageBackend> localKv);
    ~DatasetMetaService() {}

    void putMeta(DatasetMeta &meta);
    outcome::result<void> getAllLocalMetas(std::vector<DatasetMeta> & metas);
    outcome::result<void> getMeta(const DatasetId &id,
                                  FoundMetaHandler handler);
    outcome::result<void>
    findPeerListFromDatasets(const std::vector<DatasetWithParamTag> &datasets_with_tag,
                             FoundMetaListHandler handler);
    void setMetaSearchTimeout(unsigned int timeout) {
        meta_search_timeout_ = timeout;
    }
  private:
    std::shared_ptr<StorageBackend> localKv_;
    std::shared_ptr<primihub::p2p::NodeStub> p2pStub_;
    std::map<std::string, DatasetMetaWithParamTag> meta_map_; // key: dataset_id
    std::mutex meta_map_mutex_;
    unsigned int  meta_search_timeout_ = 60; // seconds
};

} // namespace primihub::service

#endif // SRC_PRIMIHUB_SERVICE_DATASET_SERVICE_H_
