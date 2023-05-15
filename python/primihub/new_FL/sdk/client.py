from primihub.client.ph_grpc.src.primihub.protos import common_pb2
from primihub.client.ph_grpc.worker1 import WorkerClient
import json
import uuid


class Client:

    def __init__(self, json_file, var_type=2, is_array=False):
        # json_file contains three components:
        self.party_info = json_file['party_info']
        self.component_params = json_file['component_params']
        self.role_params = self.component_params['role_params']
        self.var_type = var_type
        self.is_array = is_array
        self.task_id = uuid.uuid1().hex

    def prepare_for_worker(self):

        Dataset = common_pb2.Dataset()

        # consturct 'cp_param'
        cp_param = common_pb2.Params()
        for tmp_party, tmp_param in self.role_params.items():

            cp_param.param_map[tmp_party].var_type = self.var_type
            cp_param.param_map[tmp_party].is_array = self.is_array
            # cp_param.param_map[tmp_party].value_string = json.dumps(
            #     tmp_param).encode()
            cp_param.param_map[tmp_party].value_string = json.dumps(
                self.component_params).encode()

        # consturct 'dataset'
        party_datasets = {}
        for tmp_party, tmp_param in self.role_params.items():
            tmp_data = common_pb2.Dataset()
            if 'data_set' is tmp_param:
                tmp_data.data['data_set'] = tmp_param['data_set']
            else:
                tmp_data.data['data_set'] = ""
            party_datasets[tmp_party] = tmp_data

        # construct 'cp_task_info'
        cp_task_info = common_pb2.TaskContext()
        cp_task_info.task_id = self.task_id
        cp_task_info.job_id = self.task_id
        cp_task_info.request_id = self.task_id

        # construct 'party_access_info'
        party_access_info = {}
        for tmp_party, tmp_info in self.party_info.items():
            tmp_node = common_pb2.Node()
            if isinstance(tmp_node, dict):
                tmp_node.ip = tmp_info['ip'].encode()
                tmp_node.port = int(tmp_info['port'])
                tmp_node.use_tls = tmp_info['use_tls']
                party_access_info[tmp_party] = tmp_node

        self.current_worker = WorkerClient(
            node=self.party_info['task_manager'],
            cert=None,
            task_name=self.component_params['common_params']['task_name'],
            language=0,
            params=cp_param,
            task_info=cp_task_info,
            party_datasets=party_datasets,
            party_access_info=party_access_info)

    def submit(self):
        self.prepare_for_worker()
        reply = self.current_worker.submit_task(request=None)

    def get_status(self, task_id):
        self.client.get_status(task_id)
