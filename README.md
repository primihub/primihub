# PrimiHub

![build workflow](https://github.com/primihub/primihub/actions/workflows/main.yml/badge.svg?branch=master)
[![Contributors](https://img.shields.io/github/contributors/primihub/primihub.svg)](https://github.com/linuxsuren/github-go/graphs/contributors)
[![Gitter](https://badges.gitter.im/primihub/community.svg)](https://gitter.im/primihub/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)
[![GitHub release](https://img.shields.io/github/release/primihub/primihub.svg?label=release)](https://github.com/linuxsuren/github-go/releases/latest)
[![Docker Pulls](https://img.shields.io/docker/pulls/primihub/primihub-node.svg)](https://hub.docker.com/r/primihub/primihub-node/tags)

> 中文 | [English](README_EN.md)

## 简介

PrimiHub是一个支持多方计算(MPC)、联邦学习(FL)、隐私求交(PSI)、隐私查询(PIR)特性的平台，支持数据源接入、数据消费、接入应用、语法、语义、安全协议多方面的扩展。 具体请见 PrimiHub [核心特性](https://docs.primihub.com/docs/category/%E5%88%9B%E5%BB%BA%E4%BB%BB%E5%8A%A1)。

## 运行一个MPC案例部署图

![Depolyment](doc/tutorial-depolyment.jpg)

## 在物理机上启动服务
### 启动MetaSerivce服务
需要自己安装JDK8环境

```bash
wget https://primihub.oss-cn-beijing.aliyuncs.com/tools/meta_service.tar.gz
tar -zxvf meta_service.tar.gz
cd meta_service
bash run.sh
```
默认启动三个 meta service 服务，每个参与方有自己的 meta service 服务

服务的日志分别存储在 meta_log(0/1/2)文件中

通过命令 ``ps -ef | grep fusion-simple.jar``检查服务是否正常运行 
```
ps -ef | grep fusion-simple.jar
root     298757       1 99 13:33 pts/8    00:00:10 java -jar fusion-simple.jar --server.port=7877 --grpc.server.port=7977 --db.path=/home/cuibo/meta_service/storage/node0 --collaborate=http://127.0.0.1:7878/,http://127.0.0.1:7879/
root     298758       1 99 13:33 pts/8    00:00:10 java -jar fusion-simple.jar --server.port=7878 --grpc.server.port=7978 --db.path=/home/cuibo/meta_service/storage/node1 --collaborate=http://127.0.0.1:7877/,http://127.0.0.1:7879/
root     298759       1 99 13:33 pts/8    00:00:10 java -jar fusion-simple.jar --server.port=7879 --grpc.server.port=7979 --db.path=/home/cuibo/meta_service/storage/node2 --collaborate=http://127.0.0.1:7878/,http://127.0.0.1:7877/
```
### 启动PrimiHub

获取PrimiHub的二进制有两种选择
- 直接从github获取最新发布的[二进制文件](https://github.com/primihub/primihub/releases)
- 通过[源码编译](https://docs.primihub.com/docs/advance-usage/start/build)

！！注意： 发布的二进制文件是基于ubuntu20.04系统编译，在其他系统可能出现不兼容的情况

运行服务

切换到bazel-bin的同级目录

！！注意：如果是通过源码编译的，请手动将start_server.sh中定义的PYTHONPATH环境变量注释

检查 PrimiHub 中配置的meta_service服务的地址和端口是否与启动的端口一致，配置在 config/nodeX.yaml，使用默认配置则不用修改

```
meta_service:
  mode: "grpc"
  ip: "127.0.0.1"
  port: 7977
  use_tls: false
```

启动 PrimiHub Node
```
./start_server.sh
```

服务的日志分别存储在log_node0, log_node1, log_node2文件中

通过命令 ``ps -ef | grep bin/node``检查服务是否正常运行

```shell
root    4172627       1  0 10:03 pts/6    00:00:00 ./bazel-bin/node --node_id=node0 --service_port=50050 --config=./config/node0.yaml
root    4172628       1  0 10:03 pts/6    00:00:00 ./bazel-bin/node --node_id=node1 --service_port=50051 --config=./config/node1.yaml
root    4172629       1  0 10:03 pts/6    00:00:00 ./bazel-bin/node --node_id=node2 --service_port=50052 --config=./config/node2.yaml
```

## 使用 docker-compose 启动，参考 [这里](https://docs.primihub.com/docs/advance-usage/start/quick-start)

## 使用PrimiHub运行任务，参考 [这里](https://docs.primihub.com/docs/advance-usage/create-tasks/mpc-task)

## 进阶使用

想了解如何从原生应用启动以及关于如何利用PrimiHub的特性，实现更多应用，见 [进阶使用](https://docs.primihub.com/docs/developer-docs/core-concept/model)。

## 开发者

如果你想参与PrimiHub项目，可以在[Issue](https://github.com/primihub/primihub/issues) 页面开启一个新的话题，比如文档、创意、Bug等。
同时可以了解我们的社区治理结构 [PrimiHub社区治理委员会](https://docs.primihub.com/docs/developer-docs/primihub-community)，我们是一个开放共建的开源项目，欢迎参与到我们的项目中。

## 社区

* 微信助手:

![wechat_helper](./doc/wechat.jpeg)
