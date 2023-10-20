from primihub.client.ph_grpc.src.primihub.protos import common_pb2
from primihub.client.ph_grpc.worker import WorkerClient
import json
import uuid


class Client:

    def __init__(self, json_file, var_type=common_pb2.VarType.STRING, is_array=False):
        # json_file: party_info, component_params
        self.party_info = json_file['party_info']
        self.component_params = json_file['component_params']
        # component_params: common_params, role_params
        self.common_params = self.component_params['common_params']
        self.role_params = self.component_params['role_params']
        self.var_type = var_type
        self.is_array = is_array
        self.task_id = uuid.uuid1().hex

    def prepare_for_worker(self):
        # construct 'params'
        params = common_pb2.Params()
        params.param_map["component_params"].var_type = self.var_type
        params.param_map["component_params"].is_array = self.is_array
        params.param_map["component_params"].value_string = \
            json.dumps(self.component_params).encode()

        # construct 'party_datasets'
        party_datasets = {}
        for party_name, role_param in self.role_params.items():
            Dataset = common_pb2.Dataset()
            data_key = role_param.get('data_set')
            if data_key:
                Dataset.data['data_set'] = data_key
                party_datasets[party_name] = Dataset

        # construct 'task_info'
        task_info = common_pb2.TaskContext()
        task_info.task_id = self.task_id
        task_info.job_id = self.task_id
        task_info.request_id = self.task_id

        # construct 'party_access_info'
        party_access_info = {}
        for party_name, party_info in self.party_info.items():
            Node = common_pb2.Node()
            if isinstance(party_info, dict):
                Node.ip = party_info['ip'].encode()
                Node.port = int(party_info['port'])
                Node.use_tls = party_info['use_tls']
                party_access_info[party_name] = Node

        self.worker = WorkerClient(
            node=self.party_info['task_manager'],
            cert=None,
            task_name=self.common_params.get('task_name',''),
            language=common_pb2.Language.PYTHON,
            params=params,
            task_info=task_info,
            party_datasets=party_datasets,
            party_access_info=party_access_info)

    def submit(self):
        self.prepare_for_worker()
        self.worker.submit_task()
