#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# context.py
# @Author :  ()
# @Link   :
# @Date   : 6/23/2022, 12:36:28 PM

from ph_grpc import worker_pb2
from ph_grpc import worker_pb2_grpc
from ph_grpc.common_pb2 import Task
from pydantic import bytes, int

from python.primihub.client.handlers.cli import Cli


class CliContext(object):
    """Cli Context

    Args:
        object (_type_): _description_
    """

    # CLI_TYPE = {
    #    "xgb" XGBCli,
    #    "pir" PRICli,
    #    "psr" PSRCli,
    # }
    def __init__(self, cli: Cli) -> None:
        self.cli = cli

    # @property
    # def task_map(self):
    #     """The task map property - the getter.
    #     """
    #     return self._task_map

    # @property.setter
    # def task_map(self, value: dict):
    #     """The setter of the task_map

    #     Args:
    #         value (dict): The task map.
    #     """
    #     # code = f = open(r"disxgb.py", "rb")
    #     # task_map = {
    #     #     "type": 0,
    #     #     "language": common_pb2.Language.PYTHON,
    #     #     "code": f.read()
    #     # }

    #     self._task_map = value

    def task_request(self, intended_worker_id: bytes, task: Task, sequence_number: int, client_processed_up_to: int):
        request = worker_pb2.PushTaskRequest(
            intended_worker_id=intended_worker_id,
            task=self.cli.task,
            sequence_number=sequence_number,
            client_processed_up_to=client_processed_up_to
        )
        return request

    def submit(self, request) -> worker_pb2.PushTaskReply:
        with self.cli.channel:
            stub = worker_pb2_grpc.VMNodeStub(self._channel)
            request = self.task_request()
            reply = stub.SubmitTask(request)
            return reply

    def execute(self, request) -> None:
        pass

    def remote_execute(self):
        pass
