# E2E端到端测试

## 目录结构

```
e2etest |--------- local|---- pre_test.py 启动节点
        |                                 |---- test_mpc.py
        |                                 |---- test_fl.py
        |                                 |---- test_psi.py
        |                                 |---- test_pir.py 
        |                                 |---- test_all.py 
        |                                 |---- config.py   配置路径，请首先修改！！！
        |                                 |---- kill_node.py 
        |                                 |---- portlisten.py 

```
## local测试环境
1.环境： ubuntu，远程连接时可使用，依赖项如下：
  ```
    - name: Install go
      run: |
        sudo apt install golang
        sudo rm -rf /usr/bin/go 
        sudo rm -rf /usr/local/go 
        wget https://dl.google.com/go/go1.18.3.linux-amd64.tar.gz 
        sudo tar -xzf go1.18.3.linux-amd64.tar.gz -C /usr/local  
        sudo ln -s /usr/local/go/bin/go /usr/bin/go
        
    - name: Install startNode
      run: |
        git clone https://github.com/primihub/simple-bootstrap-node.git && cd simple-bootstrap-node 
        go mod tidy
  ```

  
  > 启动节点已经安装的请忽略上述安装go和startNode

  ```  
    - name: Install pytest
      run: sudo apt-get install python3-pip && pip3 install pytest     

2.运行方式
        cd e2etest
        cd local
        1) 检测全部功能
          pytest
        2) 检测单个功能
          为了释放资源，取消指定功能的最后kill_node两行注释
          pytest -s test_mpc.py

3.注意问题
        1) 注意配置/e2etest/local_action/config中的startNode的路径
        2) 这种方式隐式启动节点，节点的输出在node?.log中
        3) 尽量在root下运行
        4) 在全部检测时，kill_node只需在任意功能最后执行一次，可以节省测试时间

>pytest的常见参数
>-s ：显示标准输出，例如print()的语句；
>-v ：显示详细报告；
>-q ：显示简洁报告；

