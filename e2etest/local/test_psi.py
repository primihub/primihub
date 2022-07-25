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

def test_psi_local():
    path = config.primihub_path 
    os.chdir(path)
    command = "./bazel-bin/cli --task_type=3 --params='clientData:STRING:0:psi_client_data,serverData:STRING:0:psi_server_data, clientIndex:INT32:0:0,serverIndex:INT32:0:1,psiType:INT32:0:0,outputFullFilename:STRING:0:data/result/psi_result.csv' --input_datasets='clientData,serverData'"
    assert(os.system(command)) == 0 
    print("Cli command has been sent")

    '''测试生成文件'''
    assert(os.path.isfile('data/result/psi_result.csv')) == 1
    print("Output file has been generated")

def test_psi_difference():
    ''' 测试psi任务,求差集 '''
    path = config.primihub_path 
    os.chdir(path)
    command = "./bazel-bin/cli --task_type=3 --params='clientData:STRING:0:psi_client_data,serverData:STRING:0:psi_server_data, clientIndex:INT32:0:0,serverIndex:INT32:0:1,psiType:INT32:0:1,outputFullFilename:STRING:0:data/result/psi_result_difference.csv' --input_datasets='clientData,serverData'"
    assert(os.system(command)) == 0 
    print("Cli command has been sent")

    '''测试生成文件'''
    assert(os.path.isfile('data/result/psi_result_difference.csv')) == 1
    print("Output file has been generated")

    kill_node.direct_kill()
    print("Nodes have been  killed")