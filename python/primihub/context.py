'''
Generally, to run a task there are several important parts for a task.
1. The task configure (format fix): for the task, i.e. the task type, ID, also the function
2. The node configure (format fix): mainly for the communication, the node configuration is used
    communication with other nodes and check the node info.
3. The task parameters (format is defined by user): The parameter of the special algorithm.
Note, the dataset is also included in this parameter.
'''

import functools
from primihub.utils.logger_util import logger
from primihub.client.ph_grpc.src.primihub.protos import worker_pb2

class ContextAll:
    '''
    All the parameter is included in this context.
    '''
    def __init__(self) -> None:
        '''
        Set the message from protobuf
        '''
        self.message = None
        self.datasets = []
        self.task_info = ""
        self.dataset_info = {}
        self.party_access_info = {}
        self.task_config = ""
        self.cert_config = {}

    def init_context(self):
        self.task_req = worker_pb2.PushTaskRequest()
        self.task_req.ParseFromString(self.message)
        self.task_config = self.task_req.task
        self.task_info = self.task_config.task_info

    def request_id(self):
        return self.task_info.request_id

    def party_name(self):
        return self.task_config.party_name

    def task_code(self):
        return self.task_config.code

def set_message(message):
    Context.message = message
    Context.init_context()

def set_cert_config(root_ca_path, key_path, cert_path):
    Context.cert_config = {
      "root_ca_path": root_ca_path,
      "key_path": key_path,
      "cert_path": cert_path
    }

def reg_dataset(func):

    @functools.wraps(func)
    def reg_dataset_decorator(dataset):
        print("Register dataset:", dataset)
        Context.datasets.append(dataset)
        return func(dataset)

    return reg_dataset_decorator

Context = ContextAll()



