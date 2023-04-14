
#include "gtest/gtest.h"


#include <arrow/type.h>
#include <libp2p/protocol/kademlia/common.hpp>
#include <libp2p/outcome/outcome.hpp>

#include "src/primihub/service/dataset/service.h"
#include "src/primihub/service/dataset/localkv/storage_default.h"
#include "src/primihub/data_store/csv/csv_driver.h"
#include "src/primihub/p2p/node_stub.h"

namespace primihub::service {




void getVHandler(libp2p::outcome::result<libp2p::protocol::kademlia::Value> result) {
     if (result.has_error()) {
         std::cerr << " ! getVHandler failed: " << result.error().message()
                 << std::endl;
         return;
     } else {
      auto r = result.value();
      std::stringstream rs;
      std::copy(r.begin(), r.end(), std::ostream_iterator<uint8_t>(rs, ""));
      std::cout << "🚩🚩 "<< rs.str() <<std::endl;
     }

}
#ifdef TEST_LIBP2P
TEST(DatasetServiceTest, DatasetServiceTest_TestP2PNodeStub) {
    TestP2PNodeStub stub;

    stub.hostStart("/ip4/127.0.0.1/tcp/8888");

    while (1)
    {
      // stub.putValue("test", "test value");
      sleep(1);
      stub.getValue("test", getVHandler);
    }

}
#endif

// TODO need TestClass for DatasetService warped with only one p2p node stub

TEST(DatasetServiceTest, DatasetServiceTest_newDataset) {
  std::vector<std::string> bootstrap = {
      "/ip4/127.0.0.1/tcp/4001/ipfs/QmaoqnKLY74VdY4P4Ujqyja3eZHy9xX7Q3tQaLWLcvuqHp",
  };
  auto stub = std::make_shared<primihub::p2p::NodeStub>(bootstrap);
  stub->start("/ip4/127.0.0.1/tcp/8888");

  // Wait for stub server start
  // sleep(10);
  auto metaservice = std::make_shared<DatasetMetaService>(stub,
        std::make_shared<StorageBackendDefault>());
  DatasetService service(metaservice, "test");
  auto driver = std::make_shared<primihub::CSVDriver>("test");
  std::string filePath("/Users/chb/Projects/primihub/matrix.csv");
  auto cursor = driver->read(filePath);

  DatasetMeta meta;
  service.newDataset(driver, "test", filePath, meta);
}


TEST(DatasetServiceTest, DatasetServiceTest_RegAndGetDataset) {
  // Initialize dataset service
  std::vector<std::string> bootstrap = {
      "/ip4/127.0.0.1/tcp/4001/ipfs/QmaoqnKLY74VdY4P4Ujqyja3eZHy9xX7Q3tQaLWLcvuqHp",
  };
   auto stub = std::make_shared<primihub::p2p::NodeStub>(bootstrap);
   stub->start("/ip4/127.0.0.1/tcp/8889");
   auto metaservice = std::make_shared<DatasetMetaService>(stub,
        std::make_shared<StorageBackendDefault>());
   DatasetService service(metaservice, "test");

  // Initialize csv data driver and get local dataset.
  auto driver = std::make_shared<primihub::CSVDriver>("test");
  std::string filePath("/Users/chb/Projects/primihub/matrix.csv");
  auto cursor = driver->read(filePath);
  auto dataset = cursor->read();

  // Initialize dataset meta from dataset
  DatasetMeta meta(dataset, "desc", DatasetVisbility::PUBLIC, filePath);

  // Register dataset in local KV storage
  service.regDataset(meta);

  // Read dataset from local KV storage
  // std::function<void(std::shared_ptr<primihub::Dataset>&)>
  auto read_data_handler = [&] (std::shared_ptr<primihub::Dataset>& retrive_dataset) {
      // Compare two datasets schema
      bool eq = std::get<0>(
            retrive_dataset->data)->schema()->Equals(
                  std::get<0>(dataset->data)->schema());
      ASSERT_EQ(eq, true);
  };

  auto res = service.readDataset(meta.id, read_data_handler);

}

// TODO Test findDataset for get datameta and node location.


} // namespace primihub::service
