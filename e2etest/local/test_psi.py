import pytest
import os
import pre_test
import config

'''
运行启动节点和node节点。
如果已经运行了可以注释掉，也可以选择关闭命令行窗口重新运行。
'''
# pre_test.pre_start_node()
# pre_test.pre_node()

def test_psi_local():
    path = config.primihub_path 
    os.chdir(path)
    command = "./bazel-bin/cli --task_type=3 --params='clientData:STRING:0:psi_client_data,serverData:STRING:0:psi_server_data, clientIndex:INT32:0:0,serverIndex:INT32:0:1,psiType:INT32:0:0,outputFullFilename:STRING:0:data/result/psi_result.csv' --input_datasets='clientData,serverData'"
    assert(os.system(command)) == 0 
    '''测试生成文件'''
    assert(os.path.isfile('data/result/psi_result.csv')) == 1

def test_psi_difference():
    ''' 测试psi任务,求差集 '''
    path = config.primihub_path 
    os.chdir(path)
    command = "./bazel-bin/cli --task_type=3 --params='clientData:STRING:0:psi_client_data,serverData:STRING:0:psi_server_data, clientIndex:INT32:0:0,serverIndex:INT32:0:1,psiType:INT32:0:1,outputFullFilename:STRING:0:data/result/psi_result_difference.csv' --input_datasets='clientData,serverData'"
    assert(os.system(command)) == 0 
    '''测试生成文件'''
    assert(os.path.isfile('data/result/psi_result_difference.csv')) == 1