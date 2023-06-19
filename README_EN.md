<p align="center">
  <img src="doc/header_en.jpeg" alt="Header">
  <br>

  <p align="center"><strong>An open source privacy-preserving computation platform built by a team of cryptography experts</strong></p>

  <p align="center">
    <a href="https://github.com/primihub/primihub/releases"><img src="https://img.shields.io/github/v/release/primihub/primihub?style=flat-square" alt="GitHub Release"></a>
    <a href="https://github.com/primihub/primihub/actions/workflows/main.yml"><img src="https://img.shields.io/github/actions/workflow/status/primihub/primihub/main.yml?logo=github&style=flat-square" alt="Build Status"></a>
    <a href="https://hub.docker.com/r/primihub/primihub-node"><img src="https://img.shields.io/docker/pulls/primihub/primihub-node?style=flat-square" alt="Docker Pulls"></a>
  </p>

  <p align="center">
    English | <a href='README.md'>中文</a>
  </p>
</p>

Privacy-Preserving Computation
-------

Data circulation can create greater value. With the continuous rapid growth of the digital economy, **the demand for data interconnection is increasingly strong**, ranging from confidential data of government agencies, core data of business companies, to personal information. In the past two years, our country has also promulgated **"Data Security Law"** and **"Personal Information Protection Law"**. Therefore, **how to make data circulate privately is a problem that must be solved**.

Privacy-preserving computation, as a link between **Data Circulation and Privacy Protection Regulations**, enables **"data available but not visible"**. That is, **a collection of technologies that enable data analysis and computation while protecting the data itself from external leakage**. As an **important innovative cutting-edge technology** for data circulation, privacy-preserving computation has been widely used in many industries such as finance, healthcare, communication, and government.

PrimiHub
-------

If you are interested in privacy-preserving computation and want to experience the charm of privacy computing up close, try PrimiHub! An **open source privacy-preserving computation platform built by a team of cryptography experts**. It is secure, reliable, out-of-the-box, independent-developed, and feature-rich.

Characteristics
---

* **Open source**: Completely open-source and free
* **Easy to install**: Support Docker one-click deployment
* **Out-of-the-box**: With [Web UI](https://github.com/primihub/primihub-platform), [Command Line](https://docs.primihub.com/docs/category/%E5%88%9B%E5%BB%BA%E4%BB%BB%E5%8A%A1), and [Python SDK](https://docs.primihub.com/docs/category/python-sdk-client) to use
* **Feature-rich**: Support PIR, PSI, joint statistics, data resource management, etc.
* **Flexible configuration**: Support customized syntax, semantics, security protocols, etc.
* **Independent development**: Based on secure multi-party computation, federated learning, homomorphic encryption, trusted computing, etc.

Quick start
-------

It is recommended to use Docker to deploy PrimiHub and start your journey to privacy-preserving computation.

```
# Step 1: Download
git clone https://github.com/primihub/primihub.git
# Step 2: Start container
cd primihub && docker-compose up -d
# Step 3: Enter the container
docker exec -it primihub-node0 bash
# Step 4: Execute PSI task
./primihub-cli --task_config_file="example/psi_ecdh_task_conf.json"
I20230616 13:40:10.683375 28 cli.cc:524] all node has finished
I20230616 13:40:10.683745 28 cli.cc:598] SubmitTask time cost(ms): 1419
# View results
cat data/result/psi_result.csv
"intersection_row"
X3
...
```

<p align="center"><img src="doc/kt.gif" width=700 alt="PSI"></p>

<p align="center"><em>PSI example <a href="https://docs.primihub.com/docs/quick-start-platform/">Try online</a>・<a href ="https://docs.primihub.com/docs/advance-usage/create-tasks/psi-task/">Command line</a></em></p>

In addition, PrimiHub provides a variety of uses for **different populations**:

* [Online](https://docs.primihub.com/docs/quick-start-platform/)
* [Docker](https://docs.primihub.com/docs/advance-usage/start/quick-start)
* [Binary](https://docs.primihub.com/docs/advance-usage/start/start-nodes)
* [Source](https://docs.primihub.com/docs/advance-usage/start/build)

Question / Help / Bug
------------

If you encounter any problems while using PrimiHub and need our help, please [click](https://github.com/primihub/primihub/issues/new/choose) to report the problem.

Feel free to add our WeChat assistant and join the "PrimiHub Open Source Community" WeChat group. Get direct contact with **project core developers, cryptography experts, and privacy industry experts** to get more timely responses and first-hand information about privacy-preserving computation.

<p align="center">
  <img src="doc/wechat.jpeg" alt="Header">
</p>

License
-----

This code is released under Apache 2.0, as found in the [LICENSE](https://github.com/primihub/primihub/blob/develop/LICENSE) file.
