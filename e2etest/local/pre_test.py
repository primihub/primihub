""""
 Copyright 2022 Primihub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 """

import os
import time
import config

def pre_start_node():
    '''开启一个命令行窗口，运行启动节点
    :time.sleep() : 节点首次启动需要时间较长，之后可以修改。
    '''
    path = config.simple_bootstrap_node_path
    os.chdir(path)
    os.system("sudo gnome-terminal -e 'go run main.go'")
    time.sleep(60)


def pre_node():
    '''开启三个命令行窗口，运行节点'''
    path = config.primihub_path  
    os.chdir(path)
    os.system("sudo gnome-terminal -e './bazel-bin/node --node_id=node0 --service_port=50050 --config=./config/node0.yaml'")
    os.system("sudo gnome-terminal -e './bazel-bin/node --node_id=node1 --service_port=50051 --config=./config/node1.yaml'")
    os.system("sudo gnome-terminal -e './bazel-bin/node --node_id=node2 --service_port=50052 --config=./config/node2.yaml'")
    time.sleep(30)
