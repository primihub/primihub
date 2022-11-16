#!/bin/bash
############# CA certificate ####################### 
# 生成.key  私钥文件
openssl genrsa -out ca.key 2048
# 生成.csr 证书签名请求文件
openssl req -new -key ca.key -out ca.csr \
  -subj "/C=GB/L=China/O=primihub/CN=primihubnode"
# 自签名生成.crt/.pem 证书文件
openssl req -new -x509 -days 3650 -key ca.key -out ca.crt \
   -subj "/C=GB/L=China/O=primihub/CN=primihubnode"


############server side certificate #################3
# 生成.key  私钥文件
openssl genrsa -out server.key 2048

# 生成.csr 证书签名请求文件
openssl req -new -key server.key -out server.csr \
  -subj "/C=GB/L=China/O=primihub/CN=primihubnode"

# 签名生成.crt/.pem 证书文件
openssl x509 -req -days 3650 \
   -in server.csr -out server.crt \
   -CA ca.crt -CAkey ca.key -CAcreateserial


############client side certificate #################3
# 生成.key  私钥文件
openssl genrsa -out client.key 2048

# 生成.csr 证书签名请求文件
openssl req -new -key client.key -out client.csr \
  -subj "/C=GB/L=China/O=primihub/CN=primihubnode_client"

# 签名生成.crt/.pem 证书文件
openssl x509 -req -days 3650 \
   -in client.csr -out client.crt \
   -CA ca.crt -CAkey ca.key -CAcreateserial

