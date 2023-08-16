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

#ifndef SRC_PRIMIHUB_SERVICE_DATASET_AUTH_H__
#define SRC_PRIMIHUB_SERVICE_DATASET_AUTH_H__

namespace primihub::service {
class DatasetAuthFilterInterface {
    // TODO
    virtual bool isWriteAuth(const std::string& datasetId, const std::string& owner) = 0;
    virtual bool isReadAuth(const std::string &datasetId, const std::string& owner) = 0;
};

} // namespace primihub::service

#endif // SRC_PRIMIHUB_SERVICE_DATASET_AUTH_H__
