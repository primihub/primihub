# PrimiHub

![build workflow](https://github.com/primihub/primihub/actions/workflows/main.yml/badge.svg?branch=master)
[![Contributors](https://img.shields.io/github/contributors/primihub/primihub.svg)](https://github.com/linuxsuren/github-go/graphs/contributors)
[![Gitter](https://badges.gitter.im/primihub/community.svg)](https://gitter.im/primihub/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)
[![GitHub release](https://img.shields.io/github/release/primihub/primihub.svg?label=release)](https://github.com/linuxsuren/github-go/releases/latest)
[![Docker Pulls](https://img.shields.io/docker/pulls/primihub/primihub-node.svg)](https://hub.docker.com/r/primihub/primihub-node/tags)

> English | [中文](README.md)

## Directions

PrimiHub is a platform that supports Multi-Party Computing (MPC), Federated Learning (FL), Private set intersection (PSI), and Private Information Retrieval (PIR). It also supports extensions of data source access, data consumption, access application, syntax, semantics, and security protocols. For more details, please refer to PrimiHub [core feature](https://docs.primihub.com/docs/category/%E5%88%9B%E5%BB%BA%E4%BB%BB%E5%8A%A1).

## Run an MPC LR case

![Depolyment](doc/tutorial-depolyment.jpg)

## Quick start without docker

### Build Application Server

two options you can choose, Download the latest release version (released binary or source code)<br/>

1) download binary (skip build step)[latest release](https://github.com/primihub/primihub/releases)<br/>
2) download meta service<br/>
  [meta service](https://primihub.oss-cn-beijing.aliyuncs.com/tools/meta_service.tar.gz)<br/>

3) build with source code [build](https://docs.primihub.com/docs/advance-usage/start/build)<br/>

Attention: the release binary is compiled on os ubuntu20.04

### Start Server

```bash
extract metaservice
tar -zxvf meta_service.tar.gz
change directory to meta service
execute scripte ./run.sh as start meta service（it depends on JRE8 env）, three meta service is started on default，it represents meta service for each party
check meta_log1/2/3 log whether the service is started success or not
or use command ps -ef |grep fusion-simple.jar
root     298757       1 99 13:33 pts/8    00:00:10 java -jar fusion-simple.jar --server.port=7877 --grpc.server.port=7977 --db.path=/home/cuibo/meta_service/storage/node0 --collaborate=http://127.0.0.1:7878/,http://127.0.0.1:7879/
root     298758       1 99 13:33 pts/8    00:00:10 java -jar fusion-simple.jar --server.port=7878 --grpc.server.port=7978 --db.path=/home/cuibo/meta_service/storage/node1 --collaborate=http://127.0.0.1:7877/,http://127.0.0.1:7879/
root     298759       1 99 13:33 pts/8    00:00:10 java -jar fusion-simple.jar --server.port=7879 --grpc.server.port=7979 --db.path=/home/cuibo/meta_service/storage/node2 --collaborate=http://127.0.0.1:7878/,http://127.0.0.1:7877/

change dir which parallel with bazel-bin
waring !!!!!! if server is build by bazel build, before run script start_server.sh, comment the definition of PYTHONPATH
check the meta service config is correct, it is configured in config/nodeX.yaml or config/primihub_nodeX.yaml
meta_service:
  mode: "grpc"
  ip: "127.0.0.1"
  port: 7977
  use_tls: false
./start_server.sh
```

the server log will be recorded in log_node0, log_node1, log_node2 seperately<br/>
if all servers run successfully, using cmd ``ps -ef | grep bin/node``, you will see the following process<br/>

```shell
root    4172627       1  0 10:03 pts/6    00:00:00 ./bazel-bin/node --node_id=node0 --service_port=50050 --config=./config/node0.yaml
root    4172628       1  0 10:03 pts/6    00:00:00 ./bazel-bin/node --node_id=node1 --service_port=50051 --config=./config/node1.yaml
root    4172629       1  0 10:03 pts/6    00:00:00 ./bazel-bin/node --node_id=node2 --service_port=50052 --config=./config/node2.yaml
```

### Run logistic regression based on MPC

choose one of the servers to submit a task
run the following command

```shell
./bazel-bin/cli --server="${SERVER_IP}:${SERVER_PORT}" --task_config_file="example/mpc_lr_task_conf.json"
or ./client_run.sh will execute all case
```

## Run Server with docker

Install [docker](https://docs.docker.com/install/overview/) and [docker-compose](https://docs.docker.com/compose/install/)<br/>
Download the code and switch to the code root path<br/>

```shell
git clone https://github.com/primihub/primihub.git
cd primihub
```

### Start Server

Start three docker containers using docker-compose.
    The container includes: one simple bootstrap node, one redis, and three nodes

```shell
docker-compose up -d
```

Check out the running docker **container**

```shell
docker-compose ps
```

```shell
NAME                    COMMAND                  SERVICE                 STATUS              PORTS
primihub-node0          "/bin/bash -c './pri…"   node0                   running             0.0.0.0:6666->6666/tcp, 0.0.0.0:8050->50050/tcp
primihub-node1          "/bin/bash -c './pri…"   node1                   running             0.0.0.0:6667->6667/tcp, 0.0.0.0:8051->50051/tcp
primihub-node2          "/bin/bash -c './pri…"   node2                   running             0.0.0.0:6668->6668/tcp, 0.0.0.0:8052->50052/tcp
```

## To create a task, refer to [here](https://docs.primihub.com/docs/advance-usage/create-tasks/mpc-task)

## Advanced use

To learn how to start from native applications and how to use PrimiHub features to implement more applications, see [Advanced Usage](https://docs.primihub.com/docs/developer-docs/core-concept/model/)

## Developer

* For how to build, see [Build](https://docs.primihub.com/docs/advance-usage/start/build)

## [Roadmap](https://docs.primihub.com/docs/developer-docs/roadmap/)

## How to contribute

If you want to contribute to this project, feel free to create an issue at our [Issue](https://github.com/primihub/primihub/issues) page (e.g., documentation, new idea, and proposal).<br/>
Also, you can learn about our community [PrimiHub Open Source Community Governance](https://docs.primihub.com/docs/developer-docs/primihub-community)<br/>
This is an active open-source project, and we are always open to everyone who wants to use this platform or contribute to it.<br/>

## Community

* Wechat Official Account:

![wechat_helper](./doc/wechat.jpeg)
