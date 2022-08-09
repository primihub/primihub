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
import asyncio
import random

from primihub import context, dataset
from primihub.client.ph_grpc.connect import GRPCConnect
from primihub.client.ph_grpc.service import NodeServiceClient
from primihub.client.ph_grpc.worker import WorkerClient
from primihub.client.visitor import Visitor
import primihub as ph
import socket
import uuid


def get_host_ip():
    """get host ip address

    :return: ip
    """
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(('8.8.8.8', 80))
        ip = s.getsockname()[0]
    finally:
        s.close()

    return ip


class PrimihubClient(object):
    """Primihub Python SDK Client."""
    __instance = None
    __first_init = False

    def __new__(cls):
        if not cls.__instance:
            cls.__instance = object.__new__(cls)
        return cls.__instance

    def __init__(self, *args, **kwargs) -> None:
        self.connect = None
        self.node_event_stream = None
        self.code = None
        if not self.__first_init:
            PrimihubClient.__first_init = True

        self.visitor = Visitor()
        self.client_id = str(uuid.uuid1())  # TODO
        self.client_ip = get_host_ip()  # TODO
        self.client_port = 10050  # TODO default

    async def init(self, config):
        """Client Initialization.

        config: `dict` [`str`]
        The keys are names of `node` and `cert`.

        >>> from primihub.client import primihub_cli as cli
        >>> cli.init(config={"node": "node_address", "cert": "cert_file_path"})
        """
        print("*** cli init ***")
        print(config)
        node = config.get("node", None)
        cert = config.get("cert", None)
        # connect
        # self.connect = GRPCConnect(node=node, cert=cert)
        # get client code str
        self.code = self.visitor.visit_file()
        # get node context
        # self.node_context = self.get_node_context() # TODO

        self.node_event_stream = await self.get_node_event_stream()
        return self.connect

    async def get_node_event_stream(self):
        client = NodeServiceClient(self.connect)
        request = client.client_context(client_id=self.client_id,
                                        client_ip=self.client_ip,
                                        client_port=self.client_port)
        stream = client.get_node_event_stream(request)
        return stream

    # def get_node_context(self):
    #     """get node context
    #
    #     :return: _description_
    #     :rtype: _type_
    #     """
    #     client = NodeServiceClient(self.connect)
    #     request = client.client_context(client_id=self.client_id,
    #                                     client_ip=self.client_ip,
    #                                     client_port=self.client_port)
    #     res = client.get_node_context(request)
    #     return res

    async def submit_task(self, code: str, job_id: str, task_id: str):
        """submit task

        :param code: code str
        :type code: str
        :param job_id: job id
        :type job_id: str
        :param task_id: task id
        :type task_id: str
        :return: _description_
        :rtype: _type_
        """
        client = WorkerClient(self.connect)
        task_map = client.set_task_map(code=code.encode('utf-8'),
                                       job_id=bytes(str(job_id), "utf-8"),
                                       task_id=bytes(str(task_id), "utf-8"))

        request = client.push_task_request(intended_worker_id=b'1',
                                           task=task_map,
                                           sequence_number=random.randint(0, 9999),
                                           client_processed_up_to=random.randint(0, 9999))
        res = client.submit_task(request)
        return res

    async def _remote_execute(self, *args):
        """Send local functions and parameters to the remote side

        :param args: `list` [`tuple`] (`function`, `args`)
        """
        func_params_map = ph.context.Context.func_params_map
        for arg in args:
            func = arg[0]
            params = arg[1:]
            func_params_map[func.__name__] = params

        self.code = self.visitor.trans_remote_execute(self.code)
        ph_context_str = "ph.context.Context.func_params_map = %s" % func_params_map
        self.code += "\n"
        self.code += ph_context_str
        print("-*-" * 30)
        print("Have a cup of coffee, it will take a lot of time here.")
        print("-*-" * 30)
        res = await self.submit_task(self.code, uuid.uuid1().hex, uuid.uuid1().hex)

        return res

    async def remote_execute(self, *args):
        complete, pending = await asyncio.wait([self.get_node_event_stream(), self._remote_execute(*args)])
        for i in complete:
            print("result: ", i.result())
        if pending:
            for p in pending:
                print(p)

    # async def get_node_event(self):
    #     async for response in node_event_reply_stream():


    # def get_status(self):
    #     """get task status
    #
    #     :return: _description_
    #     :rtype: _type_
    #     """
    #     client = NodeServiceClient(self.connect)
    #     request = client.client_context(client_id=self.client_id,
    #                                     client_ip=self.client_ip,
    #                                     client_port=self.client_port)
    #     res = client.get_task_status(request)
    #     return res

    # def get_result(self):
    #     """get task result
    #
    #     :return: _description_
    #     :rtype: _type_
    #     """
    #     client = NodeServiceClient(self.connect)
    #     request = client.client_context(client_id=self.client_id,
    #                                     client_ip=self.client_ip,
    #                                     client_port=self.client_port)
    #     res = client.get_task_result(request)
    #     return res
    #
    async def get(self, ref_ret):
        print("........%s >>>>>>>", ref_ret)
        pass
