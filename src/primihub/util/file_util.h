/*
* Copyright (c) 2023 by PrimiHub
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

#ifndef SRC_PRIMIHUB_UTIL_FILE_UTIL_H_
#define SRC_PRIMIHUB_UTIL_FILE_UTIL_H_

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <utility>
#include "src/primihub/common/common.h"

namespace primihub {
retcode ReadFileContents(const std::string& fpath, std::string* contents);
std::vector<std::string> GetFiles(const std::string& path);
int ValidateDir(const std::string &file_path);
bool FileExists(const std::string& file_path);
bool RemoveFile(const std::string& file_path);
int64_t FileSize(const std::string& file_path);
/**
 * complete path using provided path
 * if file_path is relative path, concat default_storage_path to file path
 * or using file_path
*/
std::string CompletePath(const std::string& default_storage_path,
                         const std::string& file_path);
/**
 * complete path using storage path configured in config file
 * if file_path is relative path, concat default_storage_path to file path
 * or using file_path
*/
std::string CompletePath(const std::string& file_path);
}

#endif  // SRC_PRIMIHUB_UTIL_FILE_UTIL_H_
