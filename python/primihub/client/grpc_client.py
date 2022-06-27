#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Created by lizhiwei at 2022/6/27
import sys
from os import path

sys.path.append(path.abspath(path.join(path.dirname(__file__), "ph_grpc")))

# from src.primihub.protos import common_pb2
from src.primihub.protos import common_pb2, worker_pb2, worker_pb2_grpc

import grpc


class GRPCClient(object):
    """primihub gRPC client

    Args:
        object (_type_): _description_
    """

    def __init__(self, node: str, cert: str) -> None:
        """Constructor

        Keyword arguments:
        argument --  str: The grpc server address. cert: local cert path.
        Return: return_description
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
        """
        set task map
        :param task_type:
        :param name:
        :param language:
        :param params:
        :param code:
        :param node_map:
        :param input_datasets:
        :param job_id:
        :param task_id:
        :return:
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
        """
        gRPC submit
        :param request:
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

    def execute(self, request) -> None:
        pass

    def remote_execute(self):
        pass
