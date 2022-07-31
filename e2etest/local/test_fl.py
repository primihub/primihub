import pytest
import os
from . import pre_test
from . import config
from . import kill_node
from . import port_listen

def test_start_node():
    print("Starting startnode...")
    pre_test.pre_start_node()
    assert(port_listen.judge_port_listen(4001)) == 0
    print("Start_node has been started successfully")

def test_node():
    print("String node....")
    pre_test.pre_node()
    assert(port_listen.judge_port_listen(50050)) == 0
    assert(port_listen.judge_port_listen(50051)) == 0
    assert(port_listen.judge_port_listen(50052)) == 0
    print("Node has been started successfully")

def test_fl_local():
    path = config.primihub_path  
    os.chdir(path)
    command = "./bazel-bin/cli --task_lang=python --server=127.0.0.1:50050  --task_code='./python/primihub/examples/disxgb_en.py' --params='predictFileName:STRING:0:/data/result/prediction.csv,indicatorFileName:STRING:0:/data/result/indicator.csv'"
    assert(os.system(command)) == 0 
    print("Cli command has been sent")

    '''测试生成文件'''
    assert(os.path.isfile('/data/result/prediction.csv')) == 1
    assert(os.path.isfile('/data/result/indicator.csv')) == 1
    print("Output file has been generated")

#    kill_node.direct_kill()
#    print("Nodes have been  killed")