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
    
    
    def submit(self, tasks_list):
        #submit the task from the task_list

        task_id = uuid.uuid1().hex
        print(f'The task_id is {task_id}')

        if not isinstance(tasks_list[0], list):
            tasks_list = [tasks_list]

        func_map = dict()
        cp_param =  common_pb2.Params()
        party_datasets = dict()
        for func, parameter in tasks_list:
            func_map[parameter['role']] = dumps(func) 
            party_datasets[parameter['role']] = parameter['data']
            process = parameter['process'] #only one time is enough


            #set params
            cp_param.param_map['role'].var_type = 2
            cp_param.param_map['role'].is_array = False
            cp_param.param_map['role'].value_string = json.dumps(parameter)


        cp_task_info = common_pb2.TaskContext()
        common_pb2.TaskContext.task_id = task_id
        

        party_access_info = deepcopy(self.node_config)
        del[party_access_info['task_manager']]
        task = self.client.set_task_map(common_pb2.TaskType.ACTOR_TASK, # Task type.
                           process,                    # Name. example: {'Xgb_train'}
                           common_pb2.Language.PYTHON,     # Language.
                           cp_param,                         # Params. example {'guest': {'iter':10},'host': {'iter':10} }
                           func_map,                    # Code. example code {'guest': "c = 1; print c ",'host': "a = 1; print a"}
                           None,                           # Node map. not used at the moment
                           None,                           # Input dataset.
                           cp_task_info,
                           None,  
                           None,
                           party_datasets,
                           party_access_info)                           # Channel map.




        request = self.client.push_task_request(b'1',  # Intended_worker_id.
                                        task,  # Task.
                                        11,    # Sequence_number.
                                        22,    # Client_processed_up_to.
                                        b"")   # Submit_client_id.

        reply = self.client.submit_task(request)
        return task_id

    def get_status(self, task_id):
        self.client.get_status(task_id)
