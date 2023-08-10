#include "src/primihub/util/file_util.h"

#include <unistd.h>
#include <glog/logging.h>
#include <cstdio>

namespace primihub {

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
}  // namespace primihub
