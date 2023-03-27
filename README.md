# PrimiHub
![build workflow](https://github.com/primihub/primihub/actions/workflows/main.yml/badge.svg?branch=master)
[![Contributors](https://img.shields.io/github/contributors/primihub/primihub.svg)](https://github.com/linuxsuren/github-go/graphs/contributors)
[![Gitter](https://badges.gitter.im/primihub/community.svg)](https://gitter.im/primihub/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)
[![GitHub release](https://img.shields.io/github/release/primihub/primihub.svg?label=release)](https://github.com/linuxsuren/github-go/releases/latest)
[![Docker Pulls](https://img.shields.io/docker/pulls/primihub/primihub-node.svg)](https://hub.docker.com/r/primihub/primihub-node/tags)

> English | [中文](README_CN.md)

## Feature
PrimiHub is a platform that supports Multi-Party Computing(MPC), Federated Learning, Private set intersection (PSI), and Private Information Retrieval (PIR) features, and supports extensions of data source access, data consumption, access application, syntax, semantic and security protocols. For details, see PrimiHub [core feature](https://docs.primihub.com/docs/category/%E5%88%9B%E5%BB%BA%E4%BB%BB%E5%8A%A1).

## Quick Start
Run an Multi-Party Computing application in 5 minutes
## Run an MPC LR case
![Depolyment](doc/tutorial-depolyment.jpg)

## Quick start without docker
### Build Application Server
two options you have to choose, Download the latest release version (released binary or source code)<br/>
1) download binary (skip build step)[latest release](https://github.com/primihub/primihub/releases)<br/>
2) download redis<br/>
  [x86_64](https://primihub.oss-cn-beijing.aliyuncs.com/tools/redis_x86_64.tar.gz)<br/>
  [aarch64](https://primihub.oss-cn-beijing.aliyuncs.com/tools/redis_aarch64.tar.gz)<br/>
3) build with source code [build](https://docs.primihub.com/docs/advance-usage/start/build)<br/>

### Start Server
```bash
change to redis dir
./run_redis.sh
change dir which parallel with bazel-bin
GLOG_logtostderr=1 GLOG_v=7 ./bazel-bin/node --node_id=node0 --service_port=50050 --config=./config/node0.yaml &> log_node0 &
GLOG_logtostderr=1 GLOG_v=7 ./bazel-bin/node --node_id=node1 --service_port=50051 --config=./config/node1.yaml &> log_node1 &
GLOG_logtostderr=1 GLOG_v=7 ./bazel-bin/node --node_id=node2 --service_port=50052 --config=./config/node2.yaml &> log_node2 &
```
the server log will be record into log_node0, log_node1, log_node2 seperately<br/>
if all server run success, using cmd ps -ef |grep bin/node, you will see the following process<br/>
```shell
root       4915       1  0 3月13 ?        00:08:49 ./redis-server 127.0.0.1:7379
root    4172627       1  0 10:03 pts/6    00:00:00 ./bazel-bin/node --node_id=node0 --service_port=50050 --config=./config/node0.yaml
root    4172628       1  0 10:03 pts/6    00:00:00 ./bazel-bin/node --node_id=node1 --service_port=50051 --config=./config/node1.yaml
root    4172629       1  0 10:03 pts/6    00:00:00 ./bazel-bin/node --node_id=node2 --service_port=50052 --config=./config/node2.yaml
```
### Run logistic regression based on MPC
choose one of server which is used to submit task
run the follwing cmd
```shell
./bazel-bin/cli --server="${SERVER_IP}:${SERVER_PORT}" --task_config_file="example/mpc_lr_task_conf.json"
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
    The container includes: one simple bootstrap node, one redis, three nodes
```shell
docker-compose up -d
```
or, you could specific the container register and version, such as:
```shell
echo -e "REGISTRY=registry.cn-beijing.aliyuncs.com\nTAG=1.5.0" >> .env && docker-compose up -d
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
redis                   "docker-entrypoint.s…"   redis                   running             0.0.0.0:6379->6379/tcp
simple_bootstrap_node   "/app/simple-bootstr…"   simple_bootstrap_node   running             0.0.0.0:4001->4001/tcp
```

### Create an MPC task

***Let three nodes jointly perform a logistic regression task of multi-party secure computation (MPC)***

```shell
docker run --network=host -it primihub/primihub-node:latest ./primihub-cli --server=127.0.0.1:8050
```

> 💡 The node response the task
>
> You can request computing tasks from any node in the computing cluster
>

> 💡 Available task parameters
>
> The following parameters can be specified through primihub-cli:
>  1. Which node is requested to start the task.
>  2. Which shared datasets are used.
>  3. What kind of private computing tasks to do.

In this example, primihub-cli will use the default parameters to request an ABY3 tripartite logistic regression test task from ***node 0***. For the parameters that can be specified by cli, please refer to ***[Create task](https://docs.primihub.com/docs/category/%E5%88%9B%E5%BB%BA%E4%BB%BB%E5%8A%A1)***

## Advanced use
To learn how to start from native applications and how to use PrimiHub features to implement more applications, see [Advanced Usage](https://docs.primihub.com/docs/developer-docs/core-concept/model/)

## Developer
* For how to build, see [Build](https://docs.primihub.com/docs/advance-usage/start/build)

## [Roadmap](https://docs.primihub.com/docs/developer-docs/roadmap/)


## How to contribute
If you want to contribute to this project, feel free to create an issue at our [Issue](https://github.com/primihub/primihub/issues) page (e.g., documentation, new idea and proposal).<br/>
Also, you can learn about our community [PrimiHub Open Source Community Governance](https://docs.primihub.com/docs/developer-docs/primihub-community)<br/>
This is an active open source project for everyone, and we are always open to everyone who want to use this system or contribute to it.<br/>
## Contributors
<a href="https://github.com/primihub/primihub/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=primihub/primihub" />
</a>
Made with [contrib.rocks](https://contrib.rocks).<br/>
## Community
* Slack: [primihub.slack.com](https://join.slack.com/t/primihub/shared_invite/zt-1iftyi7x0-n_HqllTgPfoEcgqw5UzoYw)
* Wechat Official Account:

![wechat_helper](./doc/wechat.jpeg)
