# E2E端到端测试

## 目录结构

```
e2etest |--------- local|---- pre_test.py 启动节点
        |               |---- test_mpc.py
        |               |---- test_fl.py
        |               |---- test_psi.py
        |               |---- test_pir.py 
        |               |---- config.py   配置路径，请首先修改！！！
        |               |---- test_dir.py 测试路径

```
## 测试环境

1. 环境: wsl2 + ubuntu, 本地编译, 依赖项配置参考如下:
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

    - name: Install gnome-terminal
      run: |
        sudo apt-get install libgirepository1.0-dev
        sudo apt-get install python-cairo
        sudo apt-get install libcairo2
        sudo python3 -m pip install -U pycairo
        sudo python3 -m pip install --ignore-installed PyGObject
        sudo apt-get install gnome-terminal
        
   ```  
   >或者参考 [这里](  https://blog.csdn.net/qq_44026881/article/details/125317821?spm=1001.2101.3001.6650.3&utm_medium=distribute.pc_relevant.none-task-blog-2%7Edefault%7ECTRLIST%7Edefault-3-125317821-blog-122414615.pc_relevant_multi_platform_whitelistv2&depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7ECTRLIST%7Edefault-3-125317821-blog-122414615.pc_relevant_multi_platform_whitelistv2&utm_relevant_index=5)


2. 运行方式
  ```bash
        cd e2etest
        cd local
        # 1. 检测全部功能
        pytest
        # 2. 检测单个功能
        pytest -s test_mpc.py
  ```

3. 注意问题

* 注意配置/e2etest/local/config中的bootstrap node的路径
* 如果test_mpc.py失败，请注意node2节点的配置。
* 如果test_fl.py失败，请注意是否在primihub/python目录下执行了 python3.9 setup.py install.

>pytest的常见参数
>-s ：显示标准输出，例如print()的语句；
>-v ：显示详细报告；
>-q ：显示简洁报告；

