#ifndef SGX_DCAP_SERVER_HTTP_SERVER_H_
#define SGX_DCAP_SERVER_HTTP_SERVER_H_
#include <string>
void server_start(const std::string& port, const std::string& privatekey_path);

#endif  // SGX_DCAP_SERVER_HTTP_SERVER_H_
