<p align="center">
  <img src="doc/header.jpeg" alt="Header">
  <br>

  <p align="center"><strong>由密码学专家团队打造的开源隐私计算平台</strong></p>

  <p align="center">
    <a href="https://github.com/primihub/primihub/releases"><img src="https://img.shields.io/github/v/release/primihub/primihub?style=flat-square" alt="GitHub Release"></a>
    <a href="https://github.com/primihub/primihub/actions/workflows/main.yml"><img src="https://img.shields.io/github/actions/workflow/status/primihub/primihub/main.yml?logo=github&style=flat-square" alt="Build Status"></a>
    <a href="https://hub.docker.com/r/primihub/primihub-node"><img src="https://img.shields.io/docker/pulls/primihub/primihub-node?style=flat-square" alt="Docker Pulls"></a>
  </p>

</p>

中文 | [英文](README_EN.md)

隐私计算
-------

数据流动起来才可以创造更大的价值，随着数字经济持续高速增长，**数据的互联互通需求越来越旺盛**，大到政府机关的机密数据、公司核心商业数据、小到个人信息。近两年，我国也相继出台了 **《数据安全法》** 和 **《个人信息保护法》**。因此，**如何让数据安全地流通起来，是一个必须要解决的问题**。

隐私计算技术作为**连接数据流通和隐私保护法规的纽带**，实现了 **“数据可用不可见”**。即**在保护数据本身不对外泄露的前提下实现数据分析计算的技术集合**。隐私计算作为数据流通的**重要创新前沿技术**，已经广泛应用于金融、医疗、通信、政务等多个行业。

PrimiHub
-------

如果你对隐私计算感兴趣，想近距离体验下隐私计算的魅力，不妨试试 PrimiHub！一款**由密码学专家团队打造的开源隐私计算平台**，它安全可靠、开箱即用、自主研发、功能丰富。

特性
---

* **开源**：完全开源、免费
* **安装简单**：支持 Docker 一键部署
* **开箱即用**：拥有 [Web界面](https://github.com/primihub/primihub-platform)、[命令行](https://docs.primihub.com/docs/category/%E5%88%9B%E5%BB%BA%E4%BB%BB%E5%8A%A1) 和 [Python SDK](https://docs.primihub.com/docs/category/python-sdk-client) 多种使用方式
* **功能丰富**：支持隐匿查询、隐私求交、联合统计、数据资源管理等功能
* **灵活配置**：支持自定义扩展语法、语义、安全协议等
* **自主研发**：基于安全多方计算、联邦学习、同态加密、可信计算等隐私计算技术

快速开始
-------

推荐使用 Docker 部署 PrimiHub，开启你的隐私计算之旅。

```
# 第一步：下载
git clone https://github.com/primihub/primihub.git
# 第二步：启动容器
cd primihub && docker-compose up -d
# 第三步：进入容器
docker exec -it primihub-node0 bash
# 第四步：执行隐私求交计算
./primihub-cli --task_config_file="example/psi_ecdh_task_conf.json"
I20230616 13:40:10.683375    28 cli.cc:524] all node has finished
I20230616 13:40:10.683745    28 cli.cc:598] SubmitTask time cost(ms): 1419
# 查看结果
cat data/result/psi_result.csv
"intersection_row"
X3
...
```

<p align="center"><img src="doc/kt.gif" width=700 alt="PSI"></p>

<p align="center"><em>隐私求交例子 <a href="https://docs.primihub.com/docs/quick-start-platform/">在线尝试</a>・<a href="https://docs.primihub.com/docs/advance-usage/create-tasks/psi-task/">命令行</a></em></p>

除此之外，PrimiHub 还提供了多种适合**不同人群**的使用方式：

* [在线体验](https://docs.primihub.com/docs/quick-start-platform/)
* [Docker](https://docs.primihub.com/docs/advance-usage/start/quick-start)
* [可执行文件](https://docs.primihub.com/docs/advance-usage/start/start-nodes)
* [自行编译](https://docs.primihub.com/docs/advance-usage/start/build)

问题 / 帮助 / Bug
------------

如果您在使用过程中遇到任何问题，需要我们的帮助可以 [点击](https://github.com/primihub/primihub/issues/new/choose) 反馈问题。

欢迎添加我们的微信助手，加入「PrimiHub 开源社区」微信群。“零距离”接触**项目核心开发、密码学专家、隐私计算行业大咖**，获得更及时的回复和隐私计算的第一手资讯。

<p align="center">
  <img src="doc/wechat.jpeg" alt="Header">
</p>

许可证
-----

此代码在 Apache 2.0 下发布，参见 [LICENSE](https://github.com/primihub/primihub/blob/develop/LICENSE) 文件。
