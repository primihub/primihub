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
import sys
import socket
import uuid
from concurrent.futures import ThreadPoolExecutor

from primihub import context, dataset
from primihub.client.ph_grpc.event import listener
from primihub.client.ph_grpc.task import Task
from primihub.client.ph_grpc.grpc_client import GrpcClient
from primihub.client.ph_grpc.service import NodeServiceClient, NODE_EVENT_TYPE, NODE_EVENT_TYPE_NODE_CONTEXT
from primihub.client.visitor import Visitor
from primihub.utils.async_util import fire_coroutine_threadsafe
from primihub.dataset.dataset_cli import DatasetClientFactory

import primihub as ph
notify_channel_connected = False


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
        self.client_id = str(uuid.uuid1().hex)  # TODO
        self.client_ip = get_host_ip()  # TODO
        self.client_port = 10050  # TODO default

        # NOTE All submit task will be done in one loop
        self.loop = asyncio.get_event_loop()
        executor_opts = {'max_workers': None}  # type: Dict[str, Any]
        if sys.version_info[:2] >= (3, 6):
            executor_opts['thread_name_prefix'] = 'SyncWorker'
        self.executor = ThreadPoolExecutor(**executor_opts)
        self.loop.set_default_executor(self.executor)

        # Storage
        self.tasks_map = {}  # Primihub task map task_id: Task
        self._pending_task = []  # pending asyncio task

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
        notify_node = config.get("node", None).split(":")[0] + ":6666"
        cert = config.get("cert", None)

        # grpc clients: Command/Notify/Dataset
        self.grpc_client = GrpcClient(node, cert)
        self.notify_grpc_client = NodeServiceClient(notify_node, cert)
        # NOTE create when recevie NodeContext in NodeService
        self.dataset_client = DatasetClientFactory.create(node, cert)

        self.code = self.visitor.visit_file()  # get client code str
        """
        TODO 创建与Node之间的NodeService async grpc通道：
        1. 发送ClientContext给Node，接收NodeContext
        2. 收到NodeContext后，根据配置的数据通道和通知通道建立连接（暂时用当前默认的这个grpc连接）
        """
        print("-------create task: async notify-----------")
        # listener.on_event("/0/")(self.node_event_handler(event=None))  # regist event handler

        notify_request = self.notify_grpc_client.client_context(
            self.client_id, self.client_ip, self.client_port)

        fire_coroutine_threadsafe(
            self.notify_grpc_client.async_get_node_event(notify_request), self.loop)

    def start(self):
        """Client Start.
        """
        print("*** cli start ***")
        try:
            self.loop.run_forever()
        except Exception as e:
            print(str(e))

            self.loop.stop()
        # TODO try exception

    async def submit_task(self, job_id, client_id, *args):
        """Send local functions and parameters to the remote side

        :param job_id
        :param task_id
        :param client_id
        :param args: `list` [`tuple`] (`function`, `args`)
        """
        task = Task(task_id=uuid.uuid1().hex, primihub_client=self)
        task_id = task.task_id
        self.tasks_map[task_id] = task

        # while self.notify_channel_connected is False:
        #     print("----- waiting notify channel connected...")
        #     await asyncio.sleep(5e-2)  # 50ms

        func_params_map = ph.context.Context.func_params_map
        print("/,./.,/.")
        print(func_params_map)
        for arg in args:
            func = arg[0]
            params = arg[1:]
            func_params_map[func.__name__] = params

        self.code = self.visitor.trans_remote_execute(self.code)
        print("* - * " * 20)
        print(self.code)
        print("* - * " * 20)
        ph_context_str = "ph.context.Context.func_params_map = %s" % func_params_map
        self.code += "\n"
        self.code += ph_context_str
        print("-*-" * 30)
        print("Have a cup of coffee, it will take a lot of time here.")
        print("-*-" * 30)
        res = await self.grpc_client.submit_task(self.code, job_id=job_id, task_id=task_id, submit_client_id=client_id)
        return res

    # TODO NodeEvent Handler

    # @listener.on_event("%s" % NODE_EVENT_TYPE[NODE_EVENT_TYPE_NODE_CONTEXT])
    def node_event_handler(self, event):
        # self.notify_channel_connected = True
        self.notify_channel_connected = True
        print("...node_event_handler: %s" % event)

    def async_remote_execute(self, *args) -> None:
        job_id = uuid.uuid1().hex
        # task_id = uuid.uuid1().hex
        client_id = self.client_id
        print("------- create task: async submit task -----------")
        print("job_id: {}, type: {}".format(job_id, type(job_id)))
        # print("task_id_id: {}, type: {}".format(task_id, type(task_id)))
        print("client_id: {}, type: {}".format(client_id, type(client_id)))

        print("------- async run submit task -----------")
        try:
            fire_coroutine_threadsafe(self.submit_task(
                job_id, client_id, *args), self.loop)
        except Exception as e:
            print(str(e))
        # self.loop.call_soon_threadsafe(self.submit_task_task)
        # self.loop.run_until_complete(asyncio.wait(tasks))

    async def get(self, ref_ret):
        print("........%s >>>>>>>", ref_ret)
        pass
