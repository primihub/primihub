"""
 Copyright 2022 Primihub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 """
import traceback
from cloudpickle import loads
import json
from primihub.context import Context
from primihub.utils.logger_util import logger
from primihub.client.ph_grpc.src.primihub.protos import worker_pb2
class Executor:
    '''
    Excute the py file. Note the Context is passed
    from c++ level. 
    '''
    def __init__(self):
        pass

    @staticmethod
    def execute_py():
        PushTaskRequest = worker_pb2.PushTaskRequest()
        PushTaskRequest.ParseFromString(Context.message)
        task = PushTaskRequest.task


        print("Below are the code run on the server")
        #get the name first
        task_name = task.name
        party_name = task.party_name
        print(f"party_name is : {party_name}")
        
        #process the parameters
        task_params = task.params.param_map[party_name].value_string
        task_parameter = json.loads(task_params.decode())
        print(f"task_parameter: {task_parameter}")
        
        task_info = task.task_info
        print(f"task_info is : {task_info}")
        party_datasets = task.party_datasets
        print(f"party_datasets: {party_datasets}")
        party_access_info = task.party_access_info
        print(f"party_access_info : {party_access_info}s")

        #execute the function
        role_name = task_parameter['role']
        my_code = task.code[role_name]
        loads(my_code)(task_parameter, party_access_info)
