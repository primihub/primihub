# PrimiHub
![build workflow](https://github.com/primihub/primihub/actions/workflows/main.yml/badge.svg?branch=master)
[![Contributors](https://img.shields.io/github/contributors/primihub/primihub.svg)](https://github.com/linuxsuren/github-go/graphs/contributors)
[![Gitter](https://badges.gitter.im/primihub/community.svg)](https://gitter.im/primihub/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)
[![GitHub release](https://img.shields.io/github/release/primihub/primihub.svg?label=release)](https://github.com/linuxsuren/github-go/releases/latest)
[![Docker Pulls](https://img.shields.io/docker/pulls/primihub/primihub-node.svg)](https://hub.docker.com/r/primihub/primihub-node/tags)

> 中文 | [English](README.md)

## 特性
PrimiHub是一个支持多方计算、联邦学习、隐私求交(PSI)、隐私查询(PIR)特性的平台，支持数据源接入、数据消费、接入应用、语法、语义、安全协议多方面的扩展。 具体请见 PrimiHub [核心特性](https://docs.primihub.com/docs/category/%E5%88%9B%E5%BB%BA%E4%BB%BB%E5%8A%A1)。
## 快速开始
5分钟运行起来一个MPC应用

## 运行一个MPC案例部署图
![Depolyment](doc/tutorial-depolyment.jpg)

启动服务有两种方式，直接在物理机启动服务，基于docker容器获取服务
## 在物理机上启动服务
启动redis服务作为数据集meta信息的存储服务

获取应用的二进制有两种选择<br/>
1）直接从github获取发布的二进制文件[最新发布](https://github.com/primihub/primihub/releases)<br/>
2) 下载预配置redis<br/>
  [x86_64](https://primihub.oss-cn-beijing.aliyuncs.com/tools/redis_x86_64.tar.gz)<br/>
  [aarch64](https://primihub.oss-cn-beijing.aliyuncs.com/tools/redis_aarch64.tar.gz)<br/>
3) 通过源码编译[编译步骤](https://docs.primihub.com/docs/developer-docs/build)<br/>
运行服务
```bash
进入 redis 目录
执行 ./run_redis.sh 启动redis
切换到与bazel-bin平行的目录
GLOG_logtostderr=1 GLOG_v=7 ./bazel-bin/node --node_id=node0 --service_port=50050 --config=./config/node0.yaml &> log_node0 &
GLOG_logtostderr=1 GLOG_v=7 ./bazel-bin/node --node_id=node1 --service_port=50051 --config=./config/node1.yaml &> log_node1 &
GLOG_logtostderr=1 GLOG_v=7 ./bazel-bin/node --node_id=node2 --service_port=50052 --config=./config/node2.yaml &> log_node2 &
```

服务的日志分别存储在log_node0, log_node1, log_node2文件中，便于以后查看<br/>
如果服务正常运行，通过linux命令 ps -ef |grep bin/node, 你会获取到一下服务信息<br/>
```shell
root       4915       1  0 3月13 ?        00:08:49 ./redis-server 127.0.0.1:6379
root    4172627       1  0 10:03 pts/6    00:00:00 ./bazel-bin/node --node_id=node0 --service_port=50050 --config=./config/node0.yaml
root    4172628       1  0 10:03 pts/6    00:00:00 ./bazel-bin/node --node_id=node1 --service_port=50051 --config=./config/node1.yaml
root    4172629       1  0 10:03 pts/6    00:00:00 ./bazel-bin/node --node_id=node2 --service_port=50052 --config=./config/node2.yaml
```

## 基于docker容器 启动服务

安装 [docker](https://docs.docker.com/install/overview/) 和 [docker-compose](https://docs.docker.com/compose/install/)<br/>
下载代码并进到代码根目录<br/>
```shell
git clone https://github.com/primihub/primihub.git
cd primihub
```

***启动测试用的节点***

使用docker-compose 启动容器。<br/>
容器包括：启动点、redis、三个节点<br/>
```bash
docker-compose up -d
```
或者，您也可以通过环境变量指定镜像服务地址以及版本号，例如：<br/>
```shell
echo -e "REGISTRY=registry.cn-beijing.aliyuncs.com\nTAG=1.5.0" >> .env && docker-compose up -d
```
查看运行起来的 docker 容器：<br/>
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

### 创建一个MPC任务

***让三个节点共同执行一个多方安全计算（MPC）的逻辑回归任务***

```shell
docker run --network=host -it primihub/primihub-node:latest ./primihub-cli --server=127.0.0.1:8050
```

> 💡 请求任务的节点
>
> 你可以向计算集群中任意一个节点请求计算任务
>

> 💡 可用的任务参数
>
> 通过primihub-cli可以指定以下参数
>  1. 请求哪个节点启动任务
>  2. 使用哪些共享数据集
>  3. 做什么样的隐私计算任务

在这个例子中primihub-cli会使用默认参数向 ***node 0*** 请求一个ABY3的三方逻辑回归测试任务，关于cli可以指定的参数请见 ***[创建任务]([http://docs.primihub.com/docs/advance-usage/create-tasks](https://docs.primihub.com/docs/category/%E5%88%9B%E5%BB%BA%E4%BB%BB%E5%8A%A1))***。

## 进阶使用
想了解如何从原生应用启动以及关于如何利用PrimiHub的特性，实现更多应用，见 [进阶使用](https://docs.primihub.com/docs/core-concept/model)。

## 开发者
* 关于如何编译，请见[编译](http://docs.primihub.com/docs/developer-docs/build)
* 关于如何贡献代码，请见 [PrimiHub开源社区治理](http://docs.primihub.com/docs/primihub-community)

## [路线图](https://docs.primihub.com/docs/roadmap/)

## 如何贡献
如果你想参与PrimiHub项目，可以在[Issue](https://github.com/primihub/primihub/issues) 页面随意开启一个新的话题，比如文档、创意、Bug等。<br/>
同时可以了解我们的社区治理结构 [PrimiHub社区治理委员会](http://docs.primihub.com/docs/primihub-community)<br/>
我们是一个开放共建的开源项目，欢迎参与到我们的项目中。<br/>
## 贡献者
<a href="https://github.com/primihub/primihub/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=primihub/primihub" />
</a>

[contrib.rocks](https://contrib.rocks)

## 社区
* Slack: [primihub.slack.com](https://join.slack.com/t/primihub/shared_invite/zt-1iftyi7x0-n_HqllTgPfoEcgqw5UzoYw)
* 微信助手:

![wechat_helper](./doc/wechat.jpeg)
