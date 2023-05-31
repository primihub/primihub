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

#include "src/primihub/node/nodelet.h"

namespace primihub {

// Load config file and load default datasets
void Nodelet::loadConifg(const std::string &config_file_path,
                        unsigned int dht_get_value_timeout) {
  dataset_service_->restoreDatasetFromLocalStorage();
  dataset_service_->loadDefaultDatasets(config_file_path);
  dataset_service_->setMetaSearchTimeout(dht_get_value_timeout);
}

} // namespace primihub
