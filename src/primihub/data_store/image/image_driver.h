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

#ifndef SRC_PRIMIHUB_DATA_STORE_IMAGE_IMAGE_DRIVER_H_
#define SRC_PRIMIHUB_DATA_STORE_IMAGE_IMAGE_DRIVER_H_

#include <memory>
#include <vector>
#include <string>

#include "src/primihub/data_store/dataset.h"
#include "src/primihub/data_store/driver.h"

namespace primihub {
class ImageDriver;
struct ImageAccessInfo : public DataSetAccessInfo {
  ImageAccessInfo() = default;
  ImageAccessInfo(const std::string& image_dir, const std::string& annotations_file)
      : image_dir_(image_dir), annotations_file_(annotations_file) {}
  std::string toString() override;
  retcode ParseFromJsonImpl(const nlohmann::json& access_info) override;
  retcode ParseFromYamlConfigImpl(const YAML::Node& meta_info) override;
  retcode ParseFromMetaInfoImpl(const DatasetMetaInfo& meta_info) override;

 public:
  std::string image_dir_;
  std::string annotations_file_;
};

class ImageCursor : public Cursor {
 public:
  explicit ImageCursor(std::shared_ptr<ImageDriver> driver);
  ~ImageCursor();
  std::shared_ptr<Dataset> readMeta() override;
  std::shared_ptr<Dataset> read() override;
  std::shared_ptr<Dataset> read(const std::shared_ptr<arrow::Schema>& data_schema) override;
  std::shared_ptr<Dataset> read(int64_t offset, int64_t limit) override;
  int write(std::shared_ptr<Dataset> dataset) override;
  void close() override;

 private:
  unsigned long long offset_{0};    // NOLINT
  std::shared_ptr<ImageDriver> driver_;
};

class ImageDriver : public DataDriver, public std::enable_shared_from_this<ImageDriver> {
 public:
  explicit ImageDriver(const std::string &nodelet_addr);
  ImageDriver(const std::string &nodelet_addr, std::unique_ptr<DataSetAccessInfo> access_info);
  ~ImageDriver() {}
  std::unique_ptr<Cursor> read() override;
  std::unique_ptr<Cursor> read(const std::string& access_info) override;
  std::unique_ptr<Cursor> initCursor(const std::string& access_ifno) override;
  std::unique_ptr<Cursor> initCursor();
  std::unique_ptr<Cursor> GetCursor() override;
  std::unique_ptr<Cursor> GetCursor(const std::vector<int>& col_index) override;
  std::string getDataURL() const override;

 protected:
  void setDriverType();
};

}  // namespace primihub

#endif  // SRC_PRIMIHUB_DATA_STORE_IMAGE_IMAGE_DRIVER_H_
