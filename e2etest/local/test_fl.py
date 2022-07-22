import pytest
import os
from . import pre_test
from . import config
'''
运行启动节点和node节点。
如果已经运行了可以注释掉，也可以选择关闭命令行窗口重新运行。
'''
pre_test.pre_start_node()
pre_test.pre_node()

def test_fl_local():
    path = config.primihub_path  
    os.chdir(path)
    command = "./bazel-bin/cli --task_lang=python --server=127.0.0.1:50050  --task_code='./python/primihub/examples/disxgb_en.py' --params='predictFileName:STRING:0:/data/result/prediction.csv,indicatorFileName:STRING:0:/data/result/indicator.csv'"
    assert(os.system(command)) == 0 
    '''测试生成文件'''
    assert(os.path.isfile('/data/result/prediction.csv')) == 1
    assert(os.path.isfile('/data/result/indicator.csv')) == 1