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
from importlib import import_module
from primihub.context import Context
from primihub.utils.logger_util import logger
from primihub.client.ph_grpc.src.primihub.protos import worker_pb2
import primihub

path = primihub.__file__
path = path[:-12]

global FUNC_MAP

with open(path + '/new_FL/model_map.json', 'r') as f:
    FUNC_MAP = json.load(f)


def execute_function(common_params, role_params, node_info, task_params):
    model = common_params['model']
    role = role_params['role']

    func_name = FUNC_MAP[model][role]['func']
    func_module = FUNC_MAP[model][role]['module']

    module_name = import_module(func_module)
    get_model_attr = getattr(module_name, func_name)
    initial_model = get_model_attr(common_params=common_params,
                                   role_params=role_params,
                                   node_info=node_info,
                                   other_params=task_params)
    initial_model.run()


def run(task_params):
    party_name = task_params.party_name
    component_params_str = task_params.params.param_map[party_name].value_string
    component_params_dict = json.loads(component_params_str.decode())

    # set commom parmas, role params and node_info
    common_params = component_params_dict['common_params']
    all_role_params = component_params_dict['role_params']
    roles = component_params_dict['roles']

    current_role_params = all_role_params[party_name]

    # set role for current party
    for key, val in roles.items():
        if party_name in val:
            current_role_params['role'] = key
        else:
            current_role_params['neighbors'] = val

    node_info = task_params.party_access_info

    execute_function(common_params, current_role_params, node_info, task_params)


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

        run(task)
