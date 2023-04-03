from dev_example_guest import Dev_example_guest
from dev_example_host import Dev_example_host
import os,sys
sys.path.append("..")
from client import Client
from copy import deepcopy
class Dev_example:
    def __init__(self, node_config, num_iter = 10):
        
        #choose the task_manager to submit the task. 
        self.client = Client(node_config['task_manager'], None, node_config)
        self.node_config = node_config #maybe not used at this moment
        self.param = dict()
        self.param['num_iter'] = num_iter

    def train(self, data):
        #init the task_list
        task_list = []

        #set commom paramerter
        task_parameter = dict()
        task_parameter['param'] = self.param
        task_parameter['data'] = data
        task_parameter['process'] = 'train'
        

        #add task for guest
        tmp_parameter = deepcopy(task_parameter)
        tmp_parameter['role'] = 'guest'
        task_list.append([Dev_example_guest.run, tmp_parameter])
        #add task for host
        tmp_parameter = deepcopy(task_parameter)
        tmp_parameter['role'] = 'host'
        task_list.append([Dev_example_host.run, tmp_parameter])

        #submit the task and get the task ID
        task_ids = self.client.submit(task_list)

        #query the status of the task if needed
        #status = self.client.get_status(task_ids[0])
            


