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

def test_mpc_local():
    '''
    : 多方安全计算任务
    : --task_lang: language is proto
    : --task_type: 0-actor task
    : --task_code: logistic_regression
    : --params: batchsize 数据大小，
    :           numlters  迭代次数，
    :           trainData 训练数据集
    :           testData  测试数据集
    '''
    path = config.primihub_path 
    os.chdir(path)
    command = "./bazel-bin/cli --task_lang=proto --task_type=0 --task_code='logistic_regression' --params='BatchSize:INT32:0:128,NumIters:INT32:0:100,TrainData:STRING:0:train_party_0;train_party_1;train_party_2,TestData:STRING:0:test_party_0;test_party_1;test_party_2'"
    assert(os.system(command)) == 0 
    print("Cli command has been sent")
    
    '''测试生成文件'''
    assert(os.path.isfile('/tmp/100_200_party_0_lr.csv')) == 1
    assert(os.path.isfile('/tmp/100_200_party_1_lr.csv')) == 1
    assert(os.path.isfile('/tmp/100_200_party_2_lr.csv')) == 1
    print("Output file has been generated")
    
#    kill_node.direct_kill()
#    print("Nodes have been  killed")
