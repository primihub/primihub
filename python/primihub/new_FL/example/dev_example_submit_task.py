from dev_example_guest import Dev_example_guest
from dev_example_host import Dev_example_host
import sys
sys.path.append("..")
from client import Client
from copy import deepcopy
class Dev_example:
    def __init__(self, node_config, roles, num_iter = 10):
        
        #choose the task_manager to submit the task. 
        self.client = Client(node_config['task_manager'], None, node_config)
        self.node_config = node_config #maybe not used at this moment
        self.param = dict()
        self.param['num_iter'] = num_iter
        self.roles = roles

    def train(self, data):
        #init the task_list
        task_list = []

        #set commom paramerter
        task_parameter = dict()
        task_parameter['param'] = self.param
        task_parameter['data'] = data
        task_parameter['process'] = 'train'
        

        tmp_party_list = []
        #set the party and the role
        for role, party_list in self.roles.items():
            for party in party_list:
                task_parameter['party2role'][party] = role
                tmp_party_list.append(party)


        #set the func map
        func_map = {'guests': Dev_example_guest.run,
                    'host': Dev_example_host.run}
        
        #set the paramap
        para_map = {}
        for party in tmp_party_list:
            para_map[party] = task_parameter
        

        #submit the task and get the task ID
        task_ids = self.client.submit(func_map,para_map)

        #query the status of the task if needed
        #status = self.client.get_status(task_ids[0])
            


