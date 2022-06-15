# Primihub 
![build workflow](https://github.com/primihub/primihub/actions/workflows/main.yml/badge.svg?branch=master)
[![Gitter](https://badges.gitter.im/primihub/community.svg)](https://gitter.im/primihub/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)

## Feature
Primihub is a platform that supports Multi-Party Computing(MPC), Federated Learning, Private set intersection (PSI), and Private Information Retrieval (PIR) features, and supports extensions of data source access, data consumption, access application, syntax, semantic and security protocols. For details, see Primihub [Core Feature](http://docs.primihub.com/docs/category/%E6%A0%B8%E5%BF%83%E7%89%B9%E6%80%A7)

## Quick start

Run an Multi-Party Computing application in 5 minutes


Install [docker](https://docs.docker.com/install/overview/) and [docker-compose](https://docs.docker.com/compose/install/)

Download the code and switch to the code root path

```
$ git clone https://github.com/primihub/primihub.git
$ cd primihub
```


## Run an MPC case
![Depolyment](doc/tutorial-depolyment.jpg)


### Start the test nodes
 

   
Start three docker containers using docker-compose.
    The container includes: one simple bootstrap node, three nodes

  ```bash
  $ docker-compose up
  ```

Check out the running docker **container**

```bash
$ docker ps
```
```
  CONTAINER ID   IMAGE                                COMMAND                  CREATED          STATUS          PORTS                                                                         NAMES
cf875c1280be   primihub-node:1.0.5                  "/bin/bash -c './pri…"   11 minutes ago   Up 11 minutes   0.0.0.0:12120-12121->12120-12121/tcp, 0.0.0.0:8052->50050/tcp                 node2_primihub
6a822ff5c6f7   primihub-node:1.0.5                  "/bin/bash -c './pri…"   11 minutes ago   Up 11 minutes   0.0.0.0:10120->12120/tcp, 0.0.0.0:10121->12121/tcp, 0.0.0.0:8050->50050/tcp   node0_primihub
11d55ce06ff0   primihub-node:1.0.5                  "/bin/bash -c './pri…"   11 minutes ago   Up 11 minutes   0.0.0.0:11120->12120/tcp, 0.0.0.0:11121->12121/tcp, 0.0.0.0:8051->50050/tcp   node1_primihub
68befa6ab2a5   primihub/simple-bootstrap-node:1.0   "/app/simple-bootstr…"   11 minutes ago   Up 11 minutes   0.0.0.0:4001->4001/tcp                                                        simple_bootstrap_node

```                                                   


### Create an MPC task

*** Let three nodes jointly perform a logistic regression task of multi-party secure computation (MPC) ***


```bash
$ docker run --network=host -it primihub/primihub-node:1.0.5 ./primihub-cli --server=127.0.0.1:8050
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
 
In this example, primihub-cli will use the default parameters to request an ABY3 tripartite logistic regression test task from *** node 0 ***. For the parameters that can be specified by cli, please refer to *** [Create task](http://docs.primihub.com/docs/advance-usage/create-tasks) ***

## Advanced use
   To learn how to start from native applications and how to use Primihub features to implement more applications, see [Advanced Usage](http://docs.primihub.com/docs/category/%E8%BF%9B%E9%98%B6%E4%BD%BF%E7%94%A8)

## Developer
  * For how to build, see [Build](http://docs.primihub.com/docs/developer-docs/build)
  * For how to contribute code, see [Primihub Open Source Community Governance](http://docs.primihub.com/docs/primihub-community)
