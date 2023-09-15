#ifndef SGX_UTIL_UTIL_H_
#define SGX_UTIL_UTIL_H_

#include <vector>
#include <string>
#include <utility>
#include <map>
#include "base64.h"

typedef std::vector<std::pair<std::string, std::string>>  pair_string;
const std::string get_file_content(const std::string &);
const std::string generate_json(const pair_string &data);
bool exists_test (const std::string& name);
bool save_to_file(const std::string &data, const std::string file_name);
bool remove_file(const std::string &filename);
bool make_dir(const std::string &filename);
std::string complete_dir(const std::string &path);
const std::string generate_cert_path(const std::string &prefix);
const std::string generate_privkey_path(const std::string &prefix);


#define CHECK_RET_LOG(ret, err)                               \
  if (!(ret)) {                                               \
    LOG(ERROR) << "[FUNC: " << __FUNCTION__ << "] " << err; \
    return false;                                             \
  }

#endif  // SGX_UTIL_UTIL_H_
