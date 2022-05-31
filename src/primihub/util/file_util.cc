#include "src/primihub/util/file_util.h"

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
  DIR* open_dir = opendir(path.c_str());
  if (open_dir == NULL) {
    return {};
  }
  dirent* p = nullptr;
  closedir(open_dir);
  return res;
}

}
