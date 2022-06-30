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

#include <glog/logging.h>

#include <future>
#include <thread>
#include <chrono>

#include "src/primihub/service/dataset/service.h"
#include "src/primihub/data_store/factory.h"
#include "src/primihub/common/config/config.h"
#include "src/primihub/service/dataset/util.hpp"

using namespace std::chrono_literals;

namespace primihub::service {

    /**
     * @brief Construct a new Dataset object
     * 1. Read data using driver for get dataset & datameta
     * 2. Save datameta in local storage.
     * 3. Publish dataset meta on libp2p network.
     * 
     * @param driver [input]: Data driver
     * @param name [input]: Dataset name
     * @param description [input]: Dataset description
     * @param meta [output]: Dataset meta 
     * @return std::shared_ptr<primihub::Dataset> 
     */
    std::shared_ptr<primihub::Dataset> DatasetService::newDataset(
                                std::shared_ptr<primihub::DataDriver> driver, 
                                const std::string& description, // TODO put description in meta
                                DatasetMeta& meta) { 

        // Read data using driver for get dataset & datameta
        auto dataset = driver->getCursor()->read();
        DatasetMeta _meta(dataset, description, DatasetVisbility::PUBLIC);  // TODO(chenhongbo) visibility public for test now. 
        meta = _meta;

        // Save datameta in local storage.& Publish dataset meta on libp2p network.
        metaService_->putMeta(meta);

        return dataset;
    }

    /**
     * @brief Register dataset use meta
     * 
     * @param meta [input]: Dataset meta
     */ 
    void DatasetService::regDataset(DatasetMeta& meta) {
        metaService_->putMeta(meta);
    }

    /**
     * @brief Read dataset by dataset uri
     * TODO(chenhongbo) Only support local dataset now. !!!
     * @param id [input]: Dataset id
     * @param handler [input]: Read dataset handler
     * @return outcome::result<void>
     *  TODO async style or callback style ?
     */
    outcome::result<void> DatasetService::readDataset(const DatasetId& id, 
                                                      ReadDatasetHandler handler) {
        metaService_->getMeta(id, [&](std::shared_ptr<DatasetMeta> meta) {
            if (meta) {
                // Construct dataset from meta
                auto driver = DataDirverFactory::getDriver(meta->getDriverType(), "node test addr");  // TODO get node test addr from config
                auto cursor = driver->read(meta->getDataURL());  // TODO only support Local file path now.
                auto dataset = cursor->read();
                handler(dataset);
                return outcome::success();
            }
            
        });
        return outcome::success();
    }

    /**
     * @brief Find dataset by dataset uri
     * 
     */
    outcome::result<void> DatasetService::findDataset(const DatasetId& id,
                                        FoundMetaHandler handler) {
        metaService_->getMeta(id, handler);
        return outcome::success();
    }


    /** 
     * @brief TODO Delete dataset 
     * 1. Delete dataset meta from local storage.
     * 2. Delete dataset from libp2p network.
     * 
     * @param id [input]: Dataset id
     * @return int 
     */
    int DatasetService::deleteDataset(const DatasetId& id) {
        
        return 0;
    }

    /**
     * @brief Consture datasets from yaml datasets configurtion.
     * @param config_file_path [input]: Datasets configuration file path
     * 
     */
    void DatasetService::loadDefaultDatasets(const std::string&  config_file_path) {
        LOG(INFO) << "📃 Load default datasets from config: " << config_file_path;
        YAML::Node config = YAML::LoadFile(config_file_path);
        // strcat nodelet address
        std::string nodelet_addr = config["node"].as<std::string>() + ":"
        + config["location"].as<std::string>() + ":"
        + std::to_string(config["grpc_port"].as<uint64_t>());

        if (config["datasets"]) {
            for (auto dataset : config["datasets"]) {            
                auto driver = DataDirverFactory::getDriver(
                    std::move(dataset["model"].as<std::string>()), 
                    nodelet_addr);  

                auto source = dataset["source"].as<std::string>();
                [[maybe_unused]] auto cursor = driver->read(source);
                DatasetMeta meta;
                newDataset(driver, dataset["description"].as<std::string>(), meta);
            }
        }
    }

    // Load dataset from local meta storage.
    void DatasetService::restoreDatasetFromLocalStorage(const std::string &nodelet_addr) {
        LOG(INFO) << "💾 Restore dataset from local storage...";
        std::vector<DatasetMeta> metas;
        metaService_->getAllLocalMetas(metas);
        for (auto meta : metas) {
            // Update node let address.
            std::string node_id, node_ip, dataset_path;
            int node_port;
            std::string data_url = meta.getDataURL();
            DataURLToDetail(data_url, node_id, node_ip, node_port, dataset_path);
            meta.setDataURL(nodelet_addr + ":" +dataset_path);
            // Publish dataset meta on libp2p network.
            metaService_->putMeta(meta);
        }
    }

    void DatasetService::setMetaSearchTimeout(unsigned int timeout) {
       if (metaService_) {
          metaService_->setMetaSearchTimeout(timeout);
       }
    }


    // ======================== DatasetMetaService ====================================
    DatasetMetaService::DatasetMetaService(std::shared_ptr<primihub::p2p::NodeStub> p2pStub,
                                   std::shared_ptr<StorageBackend> localKv) {
        p2pStub_ = p2pStub;
        localKv_ = localKv;
    }

    // Get all local metas
    outcome::result<void> DatasetMetaService::getAllLocalMetas(std::vector<DatasetMeta> & metas) {
        // Get all k, v from local storage
        auto r = localKv_->getAll();
        auto rl = r.value();
        for (auto kv_pair : rl) {
            // Construct meta from k, v
            auto meta = DatasetMeta(kv_pair.second);
            metas.push_back(meta);
        }
        return outcome::success();
    }

    void DatasetMetaService::putMeta(DatasetMeta& meta) {
        std::string meta_str = meta.toJSON();
        LOG(INFO) << "<< Put meta: "<< meta_str;
         // Save datameta in local storage.
        localKv_->putValue(meta.id,  meta_str);

        // Publish dataset meta on libp2p network.
        p2pStub_->putDHTValue(meta.id, meta_str);
    }


    outcome::result<void> DatasetMetaService::getMeta(const DatasetId& id, 
                                    FoundMetaHandler handler) {
        // Try get meta from local storage or p2p network
        auto res = localKv_->getValue(id);
        if (res.has_value()) {
             // construct dataset meta from json meta.
            auto meta = std::make_shared<DatasetMeta>(res.value());
            handler(meta);
            return outcome::success();
        }

        // Try get dataset from p2p network      
        p2pStub_->getDHTValue(id, 
        [&](libp2p::outcome::result<libp2p::protocol::kademlia::Value> result) {
            if (result.has_value()) {
                try {
                    auto r = result.value();
                    std::stringstream rs;
                    std::copy(r.begin(), r.end(), std::ostream_iterator<uint8_t>(rs, ""));
                    std::cout<<rs.str()<<std::endl;
                    auto _meta = std::make_shared<DatasetMeta>(std::move(rs.str()));
                    handler(_meta);
                } catch (std::exception& e) {
                    LOG(ERROR) << "<< Get meta failed: " << e.what();
                }
            }
        });
        return outcome::success();
    }


      outcome::result<void> DatasetMetaService::findPeerListFromDatasets(
                                                        const std::vector<DatasetWithParamTag>& datasets_with_tag, 
                                                        FoundMetaListHandler handler) {
        std::vector<DatasetMetaWithParamTag> meta_list;
        std::lock_guard<std::mutex> lock(meta_map_mutex_);
        meta_map_.clear();
        auto t_start = std::chrono::high_resolution_clock::now();
        bool is_timeout = false;
        for (;;) {
            // TODO timeout guard
            auto t_now = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(t_now - t_start).count() > this->meta_search_timeout_) {
                LOG(ERROR) << " 🔍 ⏱️  Timeout while searching meta list.";
                is_timeout = true;
                break;
            }

            for (auto dataset_item : datasets_with_tag) {
                auto dataset_name = std::get<0>(dataset_item);
                auto dataset_tag = std::get<1>(dataset_item);
                DatasetId id(dataset_name);
                // Try get meta from local storage or p2p network
                auto res = localKv_->getValue(id);
                if (res.has_value()) {
                    // construct dataset meta from json meta.
                    auto _meta = std::make_shared<DatasetMeta>(res.value());
                    // meta_list.push_back(std::move(_meta));
                    LOG(INFO) << "Found local meta: " << res.value();
                    auto k = _meta->getDescription();
                    meta_map_.insert({k, std::make_pair(_meta, dataset_tag)});

                } else {
                // Find in DHT
                p2pStub_->getDHTValue(id, 
                    [&, dataset_tag](libp2p::outcome::result<libp2p::protocol::kademlia::Value> result) {
                        if (result.has_value()) {
                            try {
                                std::vector<std::shared_ptr<DatasetMeta>> meta_list;
                                auto r = result.value();
                                std::stringstream rs;
                                std::copy(r.begin(), r.end(), std::ostream_iterator<uint8_t>(rs, ""));
                                LOG(INFO) << "Fount remote meta: " << rs.str();
                                auto _meta = std::make_shared<DatasetMeta>(std::move(rs.str()));
                                auto k = _meta->getDescription();
                                meta_map_.insert({k, std::make_pair(_meta, dataset_tag)});
                            } catch (std::exception& e) {
                                LOG(ERROR) << "<< Get meta failed: " << e.what();
                            }
                        }
                    });
                }
            }
            if (meta_map_.size() < datasets_with_tag.size()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            } else {
                break;
            }
        }
        if (is_timeout) {
            return outcome::success();
        }
        for (auto& meta : meta_map_) {
            meta_list.push_back(std::make_pair(meta.second.first, meta.second.second));
        }
        handler(meta_list);
        return outcome::success();
     }


} // namespace primihub::service
