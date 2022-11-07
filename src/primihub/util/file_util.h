// Copyright [2021] <primihub.com>
#ifndef SRC_primihub_UTIL_FILE_UTIL_H_
#define SRC_primihub_UTIL_FILE_UTIL_H_

#include <string>
#include <sstream>
#include <vector>
#include <map>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utility>
#include <fstream>

namespace primihub {

std::vector<std::string> GetFiles(const std::string& path);
int ValidateDir(const std::string &file_path);
std::string getFileContents(const std::string& fpath);
}
#endif  // SRC_primihub_UTIL_FILE_UTIL_H_
