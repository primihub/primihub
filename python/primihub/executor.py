"""
 Copyright 2022 PrimiHub

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
import json
from importlib import import_module
from primihub.context import Context
from primihub.utils.logger_util import logger
from primihub.client.ph_grpc.src.primihub.protos import worker_pb2


def run(task_params):
    party_name = task_params.party_name
    params_str = task_params.params.param_map["component_params"].value_string
    params_dict = json.loads(params_str.decode())

    # load commom_parmas, roles, node_info, task_info
    common_params = params_dict['common_params']
    roles = params_dict['roles']
    node_info = task_params.party_access_info
    task_info = task_params.task_info

    # set role_params for current party
    all_role_params = params_dict['role_params']
    if party_name in all_role_params.keys():
        role_params = all_role_params[party_name]
        role_params['data'] = eval(task_params.party_datasets[party_name].data['data_set'])
    else:
        role_params = {}

    role_params['self_name'] = party_name
    role_params['others_role'] = []
    for key, val in roles.items():
        if party_name in val:
            role_params['self_role'] = key
        else:
            role_params['others_role'].append(key)

    if len(role_params['others_role']) == 1:
        role_params['others_role'] = role_params['others_role'][0]

    # load model and run
    with open('python/primihub/FL/model_map.json', 'r') as f:
        model_map = json.load(f)

    model = common_params['model']
    logger.info(f'model: {model}')
    role = role_params['self_role']
    logger.info(f'role: {role}')
    model_path = model_map[model][role]
    cls_module, cls_name = model_path.rsplit(".", maxsplit=1)

    module_name = import_module(cls_module)
    get_model_attr = getattr(module_name, cls_name)
    model = get_model_attr(roles=roles,
                           common_params=common_params,
                           role_params=role_params,
                           node_info=node_info,
                           task_info=task_info)
    model.run()


class Executor:
    '''
    Execute the py file. Note the Context is passed
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
