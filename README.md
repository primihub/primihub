# PrimiHub 
![build workflow](https://github.com/primihub/primihub/actions/workflows/main.yml/badge.svg?branch=master)
[![Contributors](https://img.shields.io/github/contributors/primihub/primihub.svg)](https://github.com/linuxsuren/github-go/graphs/contributors)
[![Gitter](https://badges.gitter.im/primihub/community.svg)](https://gitter.im/primihub/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)
[![GitHub release](https://img.shields.io/github/release/primihub/primihub.svg?label=release)](https://github.com/linuxsuren/github-go/releases/latest)
[![Docker Pulls](https://img.shields.io/docker/pulls/primihub/primihub-node.svg)](https://hub.docker.com/r/primihub/primihub-node/tags)

> English | [中文](README_CN.md)

## Feature
PrimiHub is a platform that supports Multi-Party Computing(MPC), Federated Learning, Private set intersection (PSI), and Private Information Retrieval (PIR) features, and supports extensions of data source access, data consumption, access application, syntax, semantic and security protocols. For details, see PrimiHub [core feature](http://docs.primihub.com/docs/category/%E6%A0%B8%E5%BF%83%E7%89%B9%E6%80%A7).

## Quick start

Run an Multi-Party Computing application in 5 minutes

Install [docker](https://docs.docker.com/install/overview/) and [docker-compose](https://docs.docker.com/compose/install/)

Download the `docker-compose` file:

```shell
curl https://get.primihub.com/release/1.3.9/docker-compose.yml -s -o docker-compose.yml
```

## Run an MPC case
![Depolyment](doc/tutorial-depolyment.jpg)

### Start the test nodes
   
Start three docker containers using docker-compose.
    The container includes: one simple bootstrap node, three nodes
    
```shell
docker-compose up -d
```
or, you could specific the container register and version, such as:
```shell
REGISTRY=registry.cn-beijing.aliyuncs.com TAG=1.4.0 docker-compose up -d
```

Check out the running docker **container**

```shell
docker-compose ps
```

```shell
CONTAINER ID   IMAGE                                COMMAND                  CREATED          STATUS          PORTS                                                                         NAMES
cf875c1280be   primihub/primihub-node:latest        "/bin/bash -c './pri…"   11 minutes ago   Up 11 minutes   0.0.0.0:12120-12121->12120-12121/tcp, 0.0.0.0:8052->50050/tcp                 node2_primihub
6a822ff5c6f7   primihub/primihub-node:latest        "/bin/bash -c './pri…"   11 minutes ago   Up 11 minutes   0.0.0.0:10120->12120/tcp, 0.0.0.0:10121->12121/tcp, 0.0.0.0:8050->50050/tcp   node0_primihub
11d55ce06ff0   primihub/primihub-node:latest        "/bin/bash -c './pri…"   11 minutes ago   Up 11 minutes   0.0.0.0:11120->12120/tcp, 0.0.0.0:11121->12121/tcp, 0.0.0.0:8051->50050/tcp   node1_primihub
68befa6ab2a5   primihub/simple-bootstrap-node:1.0   "/app/simple-bootstr…"   11 minutes ago   Up 11 minutes   0.0.0.0:4001->4001/tcp                                                        simple_bootstrap_node
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
To learn how to start from native applications and how to use PrimiHub features to implement more applications, see [Advanced Usage](http://docs.primihub.com/docs/category/%E8%BF%9B%E9%98%B6%E4%BD%BF%E7%94%A8)

## Developer
* For how to build, see [Build](http://docs.primihub.com/docs/developer-docs/build)

## [Roadmap](https://docs.primihub.com/en/docs/roadmap)


## How to contribute
If you want to contribute to this project, feel free to create an issue at our [Issue](https://github.com/primihub/primihub/issues) page (e.g., documentation, new idea and proposal).

Also, you can learn about our community [PrimiHub Open Source Community Governance](http://docs.primihub.com/docs/primihub-community)

This is an active open source project for everyone, and we are always open to everyone who want to use this system or contribute to it.

## Contributors
<a href="https://github.com/primihub/primihub/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=primihub/primihub" />
</a>

Made with [contrib.rocks](https://contrib.rocks).

## Community
* Slack: [primihub.slack.com](https://join.slack.com/t/primihub/shared_invite/zt-1iftyi7x0-n_HqllTgPfoEcgqw5UzoYw)
* Wechat Official Account:

![wechat_helper](./doc/wechat.jpeg)
