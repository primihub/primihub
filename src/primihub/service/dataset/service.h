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

#ifndef SRC_PRIMIHUB_SERVICE_DATASET_SERVICE_H_
#define SRC_PRIMIHUB_SERVICE_DATASET_SERVICE_H_
#include <glog/logging.h>
#include <arrow/buffer.h>
#include <arrow/filesystem/filesystem.h>
#include <arrow/record_batch.h>
#include <arrow/table.h>

#include <shared_mutex>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "src/primihub/data_store/dataset.h"
#include "src/primihub/data_store/driver.h"
#include "src/primihub/data_store/factory.h"
#include "src/primihub/service/dataset/model.h"
#include "src/primihub/service/dataset/meta_service/interface.h"

namespace primihub::service {

class DatasetService  {
 public:
  DatasetService(std::unique_ptr<DatasetMetaService> meta_service_);
  ~DatasetService() = default;

  // create dataset from DataDriver reader
  std::shared_ptr<primihub::Dataset>
  newDataset(std::shared_ptr<primihub::DataDriver> driver,
              const std::string &description,
              const std::string& dataset_access_info,
              DatasetMeta* meta /*output*/);

  // create dataset from arrow data and write data using driver
  void writeDataset(const std::shared_ptr<primihub::Dataset> &dataset,
                    const std::string &description,
                    const std::string& dataset_access_info,
                    DatasetMeta& meta /*output*/);

  void regDataset(const DatasetMeta& meta);

  retcode readDataset(const DatasetId &id, ReadDatasetHandler handler);

  retcode findDataset(const DatasetId &id, FoundMetaHandler handler);

  retcode deleteDataset(const DatasetId &id);
  void loadDefaultDatasets(const std::string &config_file_path);
  void restoreDatasetFromLocalStorage(void);
  void setMetaSearchTimeout(unsigned int timeout);
  std::string getNodeletAddr(void);
  /**
   * return server access info
  */
  const std::string& DatasetLocation() const {return nodelet_addr_;}
  DatasetMetaService* MetaService() {return meta_service_.get();}

  // void listDataset() {}

  void updateDataset(const std::string &id, const std::string& description,
                      const std::string &schema_url) {}
  retcode registerDriver(const std::string& dataset_id,
                        std::shared_ptr<DataDriver> driver);
  std::shared_ptr<DataDriver> getDriver(const std::string& dataset_id,
                                        bool is_access_info = false);
  retcode unRegisterDriver(const std::string& dataset_id);
  // local config file
  std::unique_ptr<DataSetAccessInfo>
  createAccessInfo(const std::string& driver_type, const YAML::Node& meta_info);
  // NewDataset rpc interface json format
  std::unique_ptr<DataSetAccessInfo>
  createAccessInfo(const std::string& driver_type, const std::string& meta_info);
  std::unique_ptr<DataSetAccessInfo>
  createAccessInfo(const std::string& driver_type, const DatasetMetaInfo& meta_info);

 private:
  /**
   * parameter initialization
  */
  retcode Init();

 private:
  std::unique_ptr<DatasetMetaService> meta_service_{nullptr};
  std::string nodelet_addr_;
  // use cache or not, maybe support in future
  std::shared_mutex driver_mtx_;
  std::unordered_map<std::string, std::shared_ptr<DataDriver>> driver_manager_;
};

}  // namespace primihub::service
#endif  // SRC_PRIMIHUB_SERVICE_DATASET_SERVICE_H_
