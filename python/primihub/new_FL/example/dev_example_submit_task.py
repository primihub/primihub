from primihub.new_FL.example.dev_example_guest import Dev_example_guest
from primihub.new_FL.example.dev_example_host import Dev_example_host
import sys
sys.path.append("..")
from client import Client
from copy import deepcopy

def generate_para_map(task_parameter, data, roles):
    party2role = dict()
    tmp_party_list = []
    for role, party_list in roles.items():
            for party in party_list:
                party2role[party] = role
                tmp_party_list.append(party)
    
    #set the paramap
    para_map = {}
    for party in tmp_party_list:
        tmp_param = deepcopy(task_parameter)
        tmp_param['role'] = party2role[party]
        tmp_param['data'] = data[party]
        para_map[party] = tmp_param
    
    return para_map

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
        task_parameter['process'] = 'train'
        #generate the task map
        para_map = generate_para_map(task_parameter, data, self.roles)
        #set the func map
        func_map = {'guest': Dev_example_guest.run,
                    'host': Dev_example_host.run}

        
        #submit the task and get the task ID
        task_ids = self.client.submit(func_map,para_map)

        #query the status of the task if needed
        #status = self.client.get_status(task_ids[0])
            


