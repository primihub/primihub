from primihub.client.ph_grpc.src.primihub.protos import common_pb2
from primihub.client.ph_grpc.worker import WorkerClient
from copy import deepcopy
from cloudpickle import dumps
import json
import uuid

class Client:
    def __init__(self, node, cert, node_config) -> None:
        self.client = WorkerClient(node, cert)
        self.node_config = node_config
    
    
    def submit(self, func_map, para_map):
        #submit the task from the task_list

        task_id = uuid.uuid1().hex
        print(f'The task_id is {task_id}')

        for role, func in func_map.items():
            func_map[role] = dumps(func)

        #updata the params
        cp_param =  common_pb2.Params()
        example_party = "" #
        for party, task_parameter in para_map.items():
            process = task_parameter['process'] #only one time is enough
            example_party = party #only one time is enough
            #set params
            cp_param.param_map[party].var_type = 2
            cp_param.param_map[party].is_array = False
            cp_param.param_map[party].value_string = json.dumps(task_parameter).encode()
            

        cp_task_info = common_pb2.TaskContext()
        cp_task_infotask_id = task_id
        cp_task_info.job_id = task_id
        cp_task_info.request_id = task_id



        #update the datasets
        party_datasets = {}
        for party, dataset in para_map[example_party]['data'].items():
            Dataset = common_pb2.Dataset()
            for k, val in dataset.items():
                Dataset.data[k] = val
            party_datasets[party] = Dataset

        #update the party_access_info
        party_access_info = {}
        for party, config in self.node_config.items():
            if isinstance(config, str):
                continue
            Node = common_pb2.Node()
            Node.ip = config['ip'].encode()
            Node.port = int(config['port'])
            Node.use_tls = config['use_tls']
            party_access_info[party] = Node
        task = self.client.set_task_map(common_pb2.TaskType.ACTOR_TASK, # Task type.
                           process,                    # Name. example: {'Xgb_train'}
                           common_pb2.Language.PYTHON,     # Language.
                           cp_param,                         # Params. example {'guest': {'iter':10},'host': {'iter':10} }
                           func_map,                    # Code. example code {'guest': "c = 1; print c ",'host': "a = 1; print a"}
                           None,                           # Node map. not used at the moment
                           None,                           # Input dataset.
                           cp_task_info,
                           None,
                           party_datasets,
                           party_access_info)                           # Channel map.
        
        print(f"================data_seb_pb is : {party_datasets}")


        request = self.client.push_task_request(b'1',  # Intended_worker_id.
                                        task,  # Task.
                                        11,    # Sequence_number.
                                        22,    # Client_processed_up_to.
                                        b"")   # Submit_client_id.

        reply = self.client.submit_task(request)
        return task_id

    def get_status(self, task_id):
        self.client.get_status(task_id)
