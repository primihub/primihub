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
#include <glog/logging.h>

#include <memory>
#include <mutex>
#include <string>
#include <shared_mutex>

#include <arrow/buffer.h>
#include <arrow/filesystem/filesystem.h>
#include <arrow/flight/internal.h>
#include <arrow/flight/server.h>
#include <arrow/flight/test_util.h>
#include <arrow/record_batch.h>
#include <arrow/table.h>

#include "src/primihub/data_store/dataset.h"
#include "src/primihub/data_store/driver.h"
#include "src/primihub/data_store/factory.h"
#include "src/primihub/p2p/node_stub.h"
#include "src/primihub/service/dataset/model.h"
#include "src/primihub/service/dataset/localkv/storage_backend.h"
#include "src/primihub/service/outcome.hpp"

namespace primihub::service {

using DatasetWithParamTag = std::pair<std::string, std::string>;
using DatasetMetaWithParamTag =
    std::pair<std::shared_ptr<primihub::service::DatasetMeta>, std::string>;
using FoundMetaHandler =
    std::function<void(std::shared_ptr<primihub::service::DatasetMeta> &)>;
using ReadDatasetHandler =
    std::function<void(std::shared_ptr<primihub::Dataset> &)>;
using FoundMetaListHandler =
    std::function<void(std::vector<DatasetMetaWithParamTag> &)>;

using arrow::Buffer;
using arrow::RecordBatchReader;
using arrow::Schema;
using arrow::flight::FlightDataStream;
using arrow::flight::FlightDescriptor;
using arrow::flight::FlightEndpoint;
using arrow::flight::FlightInfo;
using arrow::flight::FlightMessageReader;
using arrow::flight::FlightMetadataWriter;
using arrow::flight::FlightServerBase;
using arrow::flight::Location;
using arrow::flight::NumberingStream;
using arrow::flight::RecordBatchStream;
using arrow::flight::ServerCallContext;
using arrow::flight::Ticket;
using arrow::flight::FlightPayload;
using arrow::flight::internal::SchemaToString;
using arrow::fs::FileSystem;

using primihub::DataDirverFactory;

class DatasetMetaService {
 public:
    DatasetMetaService(std::shared_ptr<primihub::p2p::NodeStub> p2pStub,
                       std::shared_ptr<StorageBackend> localKv);
    ~DatasetMetaService() {}

    virtual void putMeta(DatasetMeta& meta);
    virtual outcome::result<void> getMeta(const DatasetId &id,
                                  FoundMetaHandler handler);
    virtual outcome::result<void> findPeerListFromDatasets(
        const std::vector<DatasetWithParamTag> &datasets_with_tag,
        FoundMetaListHandler handler);

    std::shared_ptr<DatasetMeta> getLocalMeta(const DatasetId &id);
    outcome::result<void> getAllLocalMetas(std::vector<DatasetMeta> &metas);

    void setMetaSearchTimeout(unsigned int timeout) {
        meta_search_timeout_ = timeout;
    }
 private:
    std::shared_ptr<StorageBackend> localKv_;
    std::shared_ptr<primihub::p2p::NodeStub> p2pStub_;
    // std::map<std::string, DatasetMetaWithParamTag> meta_map_; // key: dataset_id
    unsigned int meta_search_timeout_ = 60; // seconds
};

class RedisDatasetMetaService : public DatasetMetaService {
 public:
    RedisDatasetMetaService(
        const std::string& redis_addr, const std::string& redis_passwd,
        std::shared_ptr<primihub::service::StorageBackend> local_db,
        std::shared_ptr<primihub::p2p::NodeStub> dummy);
    void putMeta(DatasetMeta &meta) override;
    outcome::result<void> getMeta(const DatasetId &ID, FoundMetaHandler handler) override;
    outcome::result<void> findPeerListFromDatasets(
        const std::vector<DatasetWithParamTag> &datasets_with_tag,
        FoundMetaListHandler handler) override;

 private:
    std::string redis_addr_;
    std::string redis_passwd_;
    std::shared_ptr<StorageBackend> local_db_{nullptr};
};

class DatasetService  {
 public:
    explicit DatasetService(std::shared_ptr<DatasetMetaService> meta_service,
                            const std::string &nodelet_addr);
    ~DatasetService() {}

    //////////////////// Arrow Flight Server End
    ////////////////////////////////////
    // create dataset from DataDriver reader
    std::shared_ptr<primihub::Dataset>
    newDataset(std::shared_ptr<primihub::DataDriver> driver,
               const std::string &description,
               const std::string& dataset_access_info,
               DatasetMeta &meta /*output*/);

    // create dataset from arrow data and write data using driver
    void writeDataset(const std::shared_ptr<primihub::Dataset> &dataset,
                      const std::string &description,
                      const std::string& dataset_access_info,
                      DatasetMeta &meta /*output*/);

    void regDataset(DatasetMeta &meta);

    outcome::result<void> readDataset(const DatasetId &id,
                                      ReadDatasetHandler handler);

    outcome::result<void> findDataset(const DatasetId &id,
                                      FoundMetaHandler handler);

    int deleteDataset(const DatasetId &id);
    void loadDefaultDatasets(const std::string &config_file_path);
    void restoreDatasetFromLocalStorage(void);
    void setMetaSearchTimeout(unsigned int timeout);
    std::string getNodeletAddr(void);
    std::shared_ptr<DatasetMetaService>& metaService() {
        return metaService_;
    }
    // void
    // findPeerListFromDatasets(const std::vector<std::string>
    // &dataset_namae_list,
    //                          FoundMetaListHandler handler);

    // TODO
    void listDataset() {}

    void updateDataset(const std::string &id, const std::string &description,
                       const std::string &schema_url) {}
    // DatasetSchema &getDatasetSchema(const std::string &id) const {}
    retcode registerDriver(const std::string& dataset_id, std::shared_ptr<DataDriver>);
    std::shared_ptr<DataDriver> getDriver(const std::string& dataset_id);
    retcode unRegisterDriver(const std::string& dataset_id);
    // local config file
    std::unique_ptr<DataSetAccessInfo>
    createAccessInfo(const std::string driver_type, const YAML::Node& meta_info);
    // NewDataset rpc interface json format
    std::unique_ptr<DataSetAccessInfo>
    createAccessInfo(const std::string driver_type, const std::string& meta_info);
    std::unique_ptr<DataSetAccessInfo>
    createAccessInfo(const std::string driver_type, const DatasetMetaInfo& meta_info);

 private:
    //  private:
    std::shared_ptr<DatasetMetaService> metaService_;
    std::string nodelet_addr_;
    std::shared_mutex driver_mtx_;
    std::unordered_map<std::string, std::shared_ptr<DataDriver>> driver_manager_;
};

/////////////////////////// Arrow Flight Server////////////////////////////////////
struct IntegrationDataset {
    std::shared_ptr<arrow::Schema> schema;
    std::vector<std::shared_ptr<arrow::RecordBatch>> chunks;
};

class FlightIntegrationServer : public arrow::flight::FlightServerBase {

  public:
    // ----------------------------------------------------------------------
    // A FlightDataStream that numbers the record batches
    /// \brief A basic implementation of FlightDataStream that will provide
    /// a sequence of FlightData messages to be written to a gRPC stream
    class ARROW_FLIGHT_EXPORT NumberingStream : public FlightDataStream {
      public:
        explicit NumberingStream(std::unique_ptr<FlightDataStream> stream)
        : counter_(0), stream_(std::move(stream)) {}

        std::shared_ptr<Schema> schema() override  { return stream_->schema(); }
        arrow::Status GetSchemaPayload(FlightPayload *payload) override {
              return stream_->GetSchemaPayload(payload);
        }

        arrow::Status Next(FlightPayload *payload) override {
            RETURN_NOT_OK(stream_->Next(payload));
            if (payload && payload->ipc_message.type == arrow::ipc::MessageType::RECORD_BATCH) {
                payload->app_metadata = Buffer::FromString(std::to_string(counter_));
                counter_++;
            }
            return arrow::Status::OK();
        }

      private:
        int counter_;
        std::shared_ptr<FlightDataStream> stream_;
    };

  public:
    FlightIntegrationServer(std::shared_ptr<DatasetService> service)
        : dataset_service_(service) {}

    arrow::Status GetFlightInfo(const arrow::flight::ServerCallContext &context,
                                const FlightDescriptor &request,
                                std::unique_ptr<FlightInfo> *info) override {
        if (request.type == FlightDescriptor::PATH) {
            if (request.path.size() == 0) {
                return arrow::Status::Invalid(
                    "Invalid path"); // path is dataurl
            }
            DatasetId id(request.path[0]);
            std::shared_ptr<DatasetMeta> meta =
                dataset_service_->metaService()->getLocalMeta(id);
            if (meta == nullptr) {
                return arrow::Status::KeyError(
                    "Could not find flight.",
                    request.path[0]); // path is dataset description
            }

            arrow::flight::Location server_location;
            RETURN_NOT_OK(arrow::flight::Location::ForGrpcTcp(
                "127.0.0.1", port(), &server_location));
            arrow::flight::FlightEndpoint endpoint1(
                {{request.path[0]}, {server_location}});

            FlightInfo::Data flight_data;
            flight_data.schema = meta->toJSON();
            flight_data.descriptor = request;
            flight_data.endpoints = {endpoint1};
            flight_data.total_records = meta->getTotalRecords();
            flight_data.total_bytes = -1;
            FlightInfo value(flight_data);

            *info = std::unique_ptr<FlightInfo>(
                new arrow::flight::FlightInfo(value));
            return arrow::Status::OK();
        } else {
            return arrow::Status::NotImplemented(request.type);
        }
    }

    // NOTE Only allow get dataset from local driver
    arrow::Status
    DoGet(const arrow::flight::ServerCallContext &context,
          const arrow::flight::Ticket &request,
          std::unique_ptr<FlightDataStream> *data_stream) override;

    arrow::Status DoPut(const ServerCallContext &context,
                        std::unique_ptr<FlightMessageReader> reader,
                        std::unique_ptr<FlightMetadataWriter> writer) override {
        LOG(INFO) << "====DoPut 1====";
        const FlightDescriptor &descriptor = reader->descriptor();

        if (descriptor.type !=
            arrow::flight::FlightDescriptor::DescriptorType::PATH) {
            return arrow::Status::Invalid("Must specify a path");
        } else if (descriptor.path.size() < 1) {
            return arrow::Status::Invalid("Must specify a path");
        }

        std::string key = descriptor.path[0];
        LOG(INFO) << "DoPut dataest key: " << key;
        IntegrationDataset dataset;
        ARROW_ASSIGN_OR_RAISE(dataset.schema, reader->GetSchema());
        arrow::flight::FlightStreamChunk chunk;
        while (true) {
            RETURN_NOT_OK(reader->Next(&chunk));
            if (chunk.data == nullptr)
                break;
            RETURN_NOT_OK(chunk.data->ValidateFull());
            dataset.chunks.push_back(chunk.data);
            if (chunk.app_metadata) {
                RETURN_NOT_OK(writer->WriteMetadata(
                    *chunk.app_metadata)); // TODO metadata include dataset meta
            }
        }
        LOG(INFO) << "====DoPut 2====";
        // Register dataset and  write dataset by driver
        // TODO stream writer
        auto driver = primihub::DataDirverFactory::getDriver(
            "CSV", dataset_service_->getNodeletAddr());
        driver->initCursor(key+".csv");
        auto ph_dataset =
            std::make_shared<primihub::Dataset>(dataset.chunks, driver);
        LOG(INFO) << "====DoPut 3====";
        DatasetMeta meta;
        std::string access_info{""};
        dataset_service_->writeDataset(
            ph_dataset, key /*NOTE from upload description*/, access_info, meta);
        LOG(INFO) << "====DoPut 4====";
        auto metadata_buf = arrow::Buffer::FromString(meta.toJSON());
        RETURN_NOT_OK(writer->WriteMetadata(*metadata_buf));
        LOG(INFO) << "====DoPut 5====";
        return arrow::Status::OK();
    }
    // TODO temporary solution for integration test
    // std::unordered_map<std::string, IntegrationDataset> uploaded_chunks;
    std::shared_ptr<DatasetService> dataset_service_;
}; // class FlightIntegrationServer

} // namespace primihub::service

#endif // SRC_PRIMIHUB_SERVICE_DATASET_SERVICE_H_
