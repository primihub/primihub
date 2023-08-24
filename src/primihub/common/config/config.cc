/*
 * Copyright (c) 2022 by PrimiHub
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "src/primihub/common/config/config.h"

#if defined(__GNUC__) && (__GNUC__ < 8) && !defined(__clang__)
#include <experimental/filesystem>
#else
#include <filesystem>
#endif
#include <fstream>


#if defined(__GNUC__) && (__GNUC__ < 8) && !defined(__clang__)
namespace fs = std::experimental::filesystem;
#else
namespace fs = std::filesystem;
#endif

namespace primihub::file_utility {
retcode checkFileValidation(const std::string& config_file) {
  fs::path file(config_file);
  if (!fs::exists(file)) {
    LOG(ERROR) << "File " << file.string() << " does not exist";
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode getFileContents(const std::string& fpath, std::string* contents) {
  // auto ret = checkFileValidation(fpath);
  // if (ret != retcode::SUCCESS) {
  //     return retcode::FAIL;
  // }
  std::ifstream f_in_stream(fpath);
  std::string contents_((std::istreambuf_iterator<char>(f_in_stream)),
                      std::istreambuf_iterator<char>());
  *contents = std::move(contents_);
  return retcode::SUCCESS;
}
}  // namespace primihub::file_utility

namespace primihub::common {
retcode CertificateConfig::initCertificateConfig() {
  auto ret = primihub::file_utility::getFileContents(this->root_ca_path_,
                                                     &this->root_ca_content_);
  if (ret != retcode::SUCCESS) {
    return retcode::FAIL;
  }
  ret = primihub::file_utility::getFileContents(this->key_path_,
                                                &this->key_content_);
  if (ret != retcode::SUCCESS) {
    return retcode::FAIL;
  }
  ret = primihub::file_utility::getFileContents(this->cert_path_,
                                                &this->cert_content_);
  if (ret != retcode::SUCCESS) {
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}
}  // namespace primihub::common
