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

    def __init__(self, node, cert) -> None:
        """Constructor
        """
        super(WorkerClient, self).__init__(node, cert)
        self.channel = grpc.insecure_channel(self.node)
        self.request_data = None
        self.stub = worker_pb2_grpc.VMNodeStub(self.channel)

    def set_task_map(self,
                     task_type: common_pb2.TaskType = 0,
                     name: str = "",
                     language: common_pb2.Language = 0,
                     params: common_pb2.Params = None,
                     code: dict = None,
                     node_map: dict = None,
                     input_datasets: str = None,
                     task_info: common_pb2.TaskContext = None,
                     role: str = None,  # TODO
                     rank: int = None,
                     party_datasets: dict = None,
                     party_access_info: dict = None
                     ):
        """set task map
        """

        task_map = {
            "type": task_type,
            "name": name,
            "language": language,
            "params": params,
            "code": code,
            "node_map" :node_map,
            "input_datasets" : input_datasets, 
            "task_info" : task_info,
            "role" : role,
            "rank": rank,
            "party_datasets" : party_datasets,
            "party_access_info" : party_access_info
        }
        

        return task_map

    @staticmethod
    def push_task_request(intended_worker_id=b'1',
                          task=None,
                          sequence_number=11,
                          client_processed_up_to=22,
                          submit_client_id=b""
                          ):
        request_data = {
            "intended_worker_id": intended_worker_id,
            "task": task,
            "sequence_number": sequence_number,
            "client_processed_up_to": client_processed_up_to,
            "submit_client_id": submit_client_id
        }
        request = worker_pb2.PushTaskRequest(**request_data)
        print(f"########################The request is {request}")
        return request

    def submit_task(self, request: worker_pb2.PushTaskRequest) -> worker_pb2.PushTaskReply:
        """gRPC submit task

        :returns: gRPC reply
        :rtype: :obj:`worker_pb2.PushTaskReply`
        """
        # print(type(request_data), request_data)
        with self.channel:
            reply = self.stub.SubmitTask(request)
            print("return code: %s, job id: %s" % (reply.ret_code, reply.job_id))  # noqa
            return reply

