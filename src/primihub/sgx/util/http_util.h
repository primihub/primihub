#ifndef SGX_UTIL_HTTP_UTIL_H_
#define SGX_UTIL_HTTP_UTIL_H_

#include <curl/curl.h>
#include <json/value.h>
#include <string>

class HttpRequest {
 public:
  HttpRequest();
  virtual ~HttpRequest();
  const std::string request(const std::string &url);
  const std::string get_field(const std::string &key);
  const std::string get_header_field(const std::string &key);
  bool parse_header(const std::string &);
  void set_req_field(const std::string &key, const std::string &value);
  const std::string get_case_insensitive_field(const std::string &field_name);

 protected:
  CURL *curl {nullptr};
  Json::Value root;
  Json::Value header_root;
  Json::Value req_root;
  static std::size_t write_callback(char *in, std::size_t size,
                                    std::size_t nmemb, std::string *out);
};

#endif  // SGX_UTIL_HTTP_UTIL_H_
