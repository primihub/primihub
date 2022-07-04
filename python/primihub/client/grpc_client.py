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
import sys
from os import path

import grpc

sys.path.append(path.abspath(path.join(path.dirname(__file__), "ph_grpc")))  # noqa
from src.primihub.protos import common_pb2, worker_pb2, worker_pb2_grpc  # noqa


class GRPCClient(object):
    """primihub gRPC client


    :param str node: Address of the node.
    :param str cert: Path of the local cert file path.

    :return: A primihub gRPC client.
    """

    def __init__(self, node: str, cert: str) -> None:
        """Constructor
        """
        self.task_map = {}
        if node is not None:
            self.channel = grpc.insecure_channel(node)

        if cert is not None:
            # TODO
            pass

    def set_task_map(self,
                     task_type: common_pb2.TaskType = 0,
                     name: str = "",
                     language: common_pb2.Language = 0,
                     params: common_pb2.Params = None,
                     code: bytes = None,
                     node_map: common_pb2.Task.NodeMapEntry = None,
                     input_datasets: str = None,
                     job_id: bytes = None,
                     task_id: bytes = None
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

        if job_id:
            task_map["job_id"] = job_id
        else:
            # todo set job id
            pass

        if task_id:
            task_map["task_id"] = task_id
        else:
            # todo set task id
            pass

        self.task_map = task_map
        return task_map

    # def task_request(self,
    #                  intended_worker_id: bytes = b'1',
    #                  task: Task = None,
    #                  sequence_number: int = 11,
    #                  client_processed_up_to: int = 22):
    #     request = worker_pb2.PushTaskRequest(
    #         intended_worker_id=intended_worker_id,
    #         task=task,
    #         sequence_number=sequence_number,
    #         client_processed_up_to=client_processed_up_to
    #     )
    #     return request

    def submit(self) -> worker_pb2.PushTaskReply:
        """gRPC submit

        :return:
        """
        with self.channel:
            stub = worker_pb2_grpc.VMNodeStub(self.channel)
            request = worker_pb2.PushTaskRequest(
                intended_worker_id=b'1',
                task=self.task_map,
                sequence_number=11,
                client_processed_up_to=22
            )
            reply = stub.SubmitTask(request)
            print("return: ", reply)
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
