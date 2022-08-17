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
from primihub.client.ph_grpc.grpc_client import GrpcClient
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
        self.grpc_client = None
        self.node_event_stream = None
        self.code = None
        if not self.__first_init:
            PrimihubClient.__first_init = True

        self.visitor = Visitor()
        self.client_id = str(uuid.uuid1())  # TODO
        self.client_ip = get_host_ip()  # TODO
        self.client_port = 10050  # TODO default
        self.loop = asyncio.get_event_loop()

    def init(self, config):
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
        self.grpc_client = GrpcClient(node, cert)  # grpc client connect
        self.code = self.visitor.visit_file()  # get client code str

    async def submit_task(self, job_id, task_id, *args):
        """Send local functions and parameters to the remote side

        :param job_id
        :param task_id
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
        res = await self.grpc_client.submit_task(self.code, job_id=job_id, task_id=task_id)
        return res

    async def get_node_event(self):
        await self.grpc_client.get_node_event(self.client_id, self.client_ip, self.client_port)

    async def get_task_node_event(self, task_id):
        await self.grpc_client.get_node_event(task_id, self.client_id, self.client_ip, self.client_port)

    async def remote_execute(self, *args):
        job_id = uuid.uuid1().hex
        task_id = uuid.uuid1().hex
        tasks = [
            # asyncio.ensure_future(self.get_node_event()),  # get node event
            asyncio.ensure_future(self.submit_task(job_id, task_id, *args)),  # submit_task
            asyncio.ensure_future(self.get_task_node_event(task_id))  # get node event
        ]
        self.loop.run_until_complete(asyncio.wait(tasks))

    async def get(self, ref_ret):
        print("........%s >>>>>>>", ref_ret)
        pass
