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

#include "src/primihub/util/file_util.h"
#include <unistd.h>
#include <glog/logging.h>
#include <cstdio>
#include <fstream>
#include "src/primihub/common/config/server_config.h"

namespace primihub {
retcode ReadFileContents(const std::string& fpath, std::string* contents) {
  bool exist_flag = primihub::FileExists(fpath);
  if (!exist_flag) {
    LOG(ERROR) << "file is not exist, path: " << fpath;
    return retcode::FAIL;
  }
  std::ifstream f_in_stream(fpath);
  std::string contents_((std::istreambuf_iterator<char>(f_in_stream)),
                      std::istreambuf_iterator<char>());
  if (contents_.empty()) {
    LOG(ERROR) << "file is empty, please check, path: " << fpath;
    return retcode::FAIL;
  }
  *contents = std::move(contents_);
  return retcode::SUCCESS;
}

std::vector<std::string> GetFiles(const std::string& path) {
  if (path.empty()) {
    return {};
  }

  std::vector<std::string> res;
  struct stat s;
  std::string name;
  stat(path.c_str(), &s);
  if (!S_ISDIR(s.st_mode)) {
    return {};
  }
  DIR* open_dir = ::opendir(path.c_str());
  if (open_dir == NULL) {
    return {};
  }
  // dirent* p = nullptr;
  ::closedir(open_dir);
  return res;
}

int ValidateDir(const std::string &file_path) {
  if (file_path.empty()) {
    return -1;
  }
  int pos = file_path.find_last_of('/');
  if (pos > 0) {
    std::string path = file_path.substr(0, pos);
    if (::access(path.c_str(), 0) == -1) {
      std::string cmd = "mkdir -p " + path;
      int ret = system(cmd.c_str());
      if (ret) {
        return -1;
      }
    }
  }
  return 0;
}

bool FileExists(const std::string& file_path) {
  if (file_path.empty()) {
    return false;
  }
  if (::access(file_path.c_str(), F_OK) == 0) {
    return true;
  } else {
    return false;
  }
}
bool RemoveFile(const std::string& file_path) {
  if (::remove(file_path.c_str()) == 0) {
    return true;
  } else {
    LOG(ERROR) << "remove file: " << file_path << " failed";
    return false;
  }
}

std::string CompletePath(const std::string& default_storage_path,
                         const std::string& file_path) {
  if (file_path.empty()) {
    LOG(ERROR) << "file name is empty";
    return std::string("");
  }
  // if file path start with "/", it is considered as absolute path
  // or relative path
  if (file_path[0] == '/') {
    return file_path;
  } else {
    if (default_storage_path.empty()) {
      return file_path;
    }
    return default_storage_path + "/" + file_path;
  }
}

std::string CompletePath(const std::string& file_path) {
  auto& ins = ServerConfig::getInstance();
  auto& storage_path = ins.StoragePath();
  return CompletePath(storage_path, file_path);
}

}  // namespace primihub
