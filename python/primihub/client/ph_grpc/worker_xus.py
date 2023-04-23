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
import uuid
import grpc

from .connect import GRPCConnect
from src.primihub.protos import common_pb2, worker_pb2, worker_pb2_grpc  # noqa


class WorkerClient(GRPCConnect):
    """primihub gRPC worker client

    :return: A primihub gRPC worker client.
    """

    def __init__(self,
                 node,
                 cert,
                 task_type: common_pb2.TaskType = 0,
                 name: str = "",
                 language: common_pb2.Language = 0,
                 params: common_pb2.Params = None,
                 code: dict = None,
                 node_map: dict = None,
                 input_datasets: str = None,
                 task_info: common_pb2.TaskContext = None,
                 party_name: str = None,
                 party_datasets: dict = None,
                 party_access_info: dict = None) -> None:
        """Constructor
        """
        super(WorkerClient, self).__init__(node, cert)
        self.channel = grpc.insecure_channel(self.node)
        self.request_data = None
        self.stub = worker_pb2_grpc.VMNodeStub(self.channel)
        self.task_type = task_type
        self.name = name
        self.language = language
        self.params = params
        self.code = code
        self.node_map = node_map
        self.input_datasets = input_datasets
        self.task_info = task_info
        self.party_name = party_name
        self.party_datasets = party_datasets
        self.party_access_info = party_access_info

    def set_request_map(self):
        request_map = {
            "type": self.task_type,
            "name": self.name,
            "language": self.language,
            "params": self.params,
            "code": self.code,
            "node_map": self.node_map,
            "input_datasets": self.input_datasets,
            "task_info": self.task_info,
            "party_name": self.party_name,
            "party_datasets": self.party_datasets,
            "party_access_info": self.party_access_info
        }
        self.request_map = request_map

    @staticmethod
    def push_task_request(intended_worker_id=b'1',
                          task=None,
                          sequence_number=11,
                          client_processed_up_to=22,
                          submit_client_id=b""):
        request_data = {
            "intended_worker_id": intended_worker_id,
            "task": task,
            "sequence_number": sequence_number,
            "client_processed_up_to": client_processed_up_to,
            "submit_client_id": submit_client_id
        }
        print(
            f"########################The request_data is {request_data}##################"
        )
        request = worker_pb2.PushTaskRequest(**request_data)

        return request

    def submit_task(
            self,
            request: worker_pb2.PushTaskRequest) -> worker_pb2.PushTaskReply:
        """gRPC submit task

        :returns: gRPC reply
        :rtype: :obj:`worker_pb2.PushTaskReply`
        """
        # print(type(request_data), request_data)
        self.set_request_map()
        request = WorkerClient.push_task_request(task=self.request_map)
        with self.channel:
            reply = self.stub.SubmitTask(request)
            print("return code: %s, job id: %s" %
                  (reply.ret_code, reply.task_info.job_id))  # noqa
            return reply
