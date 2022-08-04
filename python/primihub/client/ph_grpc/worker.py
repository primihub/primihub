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

from .connect import GRPCClient, GRPCConnect
from src.primihub.protos import common_pb2, worker_pb2, worker_pb2_grpc  # noqa


class WorkerClient(GRPCClient):
    """primihub gRPC worker client

    :return: A primihub gRPC worker client.
    """

    def __init__(self, connect: GRPCConnect) -> None:
        """Constructor
        """
        super().__init__(connect)
        self.request_data = None
        self.stub = worker_pb2_grpc.VMNodeStub(self.channel)

        # self.channel = connect.channel

    def set_task_map(self,
                     task_type: common_pb2.TaskType = 0,
                     name: str = "",
                     language: common_pb2.Language = 0,
                     params: common_pb2.Params = None,
                     code: bytes = None,
                     node_map: common_pb2.Task.NodeMapEntry = None,
                     input_datasets: str = None,
                     job_id: bytes = None,
                     task_id: bytes = None,
                     channel_map: str = None,  # TODO
                     ):
        """set task map

        :param task_type: {}
        :param name: str
        :param language: {}
        :param params: `dict`
        :param code: bytes
        :param node_map: `dict`
        :param input_datasets:
        :param job_id: bytes
        :param task_id: bytes

        :return: `dict`
        """

        task_map = {
            "type": task_type,
            "name": name,
            "language": language,
            "code": code
        }
        if name:
            task_map["name"] = name

        if params:
            task_map["params"] = params

        if node_map:
            task_map["node_map"] = node_map

        if input_datasets:
            task_map["inout_datasets"] = input_datasets

        print(type(job_id), job_id)
        if job_id:
            task_map["job_id"] = job_id
        else:
            job_id = uuid.uuid1().hex
            task_map["job_id"] = bytes(str(job_id), "utf-8")

        if task_id:
            task_map["task_id"] = task_id
        else:
            task_id = uuid.uuid1().hex
            task_map["task_id"] = bytes(str(task_id), "utf-8")

        return task_map

    @staticmethod
    def push_task_request(intended_worker_id=b'1',
                          task=None,
                          sequence_number=11,
                          client_processed_up_to=22):
        request_data = {
            "intended_worker_id": intended_worker_id,
            "task": task,
            "sequence_number": sequence_number,
            "client_processed_up_to": client_processed_up_to
        }

        request = worker_pb2.PushTaskRequest(**request_data)
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

    # def execute(self, request) -> None:
    #     pass

    # def remote_execute(self, *args):
    #     """Send local functions and parameters to the remote side

    #     :param args: `list` [`tuple`] (`function`, `args`)
    #     """
    #     for arg in args:
    #         # TODO
    #         print(arg)
