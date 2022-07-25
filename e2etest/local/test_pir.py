import pytest
import os
from . import pre_test
from . import config

'''
运行启动节点和node节点。
如果已经运行了可以注释掉，也可以选择关闭命令行窗口重新运行。
'''
# pre_test.pre_start_node()
# pre_test.pre_node()

def test_pir_local():
    path = config.primihub_path   
    os.chdir(path)
    command = "./bazel-bin/cli --task_type=2 --params='queryIndeies:STRING:0:5,serverData:STRING:0:pir_server_data,outputFullFilename:STRING:0:data/result/pir_result.csv' --input_datasets='serverData'"
    assert(os.system(command)) == 0 
    '''测试生成文件'''
    assert(os.path.isfile('data/result/pir_result.csv')) == 1