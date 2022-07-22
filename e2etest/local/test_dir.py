import os
from . import config
def test_change_dir():
    '''测试修改当前工作目录'''
    path = config.simple_bootstrap_node_path
    os.chdir(path)
    workDir = os.getcwd()
    print(workDir)
    assert(workDir) == path

def test_change_workdir():
    path = config.primihub_path  
    os.chdir(path)
    workDir = os.getcwd()
    print(workDir)
    assert(workDir) == path