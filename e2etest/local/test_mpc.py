import pytest
import os
import pre_test
import config
'''
运行启动节点和node节点。
如果已经运行了可以注释掉，也可以选择关闭命令行窗口重新运行。
'''
pre_test.pre_start_node()
pre_test.pre_node()

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
    '''测试生成文件'''
    assert(os.path.isfile('/tmp/100_200_party_0_lr.csv')) == 1
    assert(os.path.isfile('/tmp/100_200_party_1_lr.csv')) == 1
    assert(os.path.isfile('/tmp/100_200_party_2_lr.csv')) == 1
    
