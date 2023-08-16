"""
Copyright 2022 PrimiHub

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
import sys
import socket
import uuid
import threading
from concurrent.futures import ThreadPoolExecutor

from primihub.client.ph_grpc.task import Task
from primihub.client.ph_grpc.grpc_client import GrpcClient
from primihub.client.ph_grpc.service import NodeServiceClient
from primihub.client.visitor import Visitor
from primihub.utils.async_util import fire_coroutine_threadsafe
from primihub.dataset.dataset_cli import DatasetClientFactory
from primihub.utils.logger_util import logger

import primihub as ph


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


class PrimiHubClient(object):
    """PrimiHub Python SDK Client."""
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
            PrimiHubClient.__first_init = True

        self.visitor = Visitor()
        self.client_id = "client:" + \
                         uuid.uuid3(uuid.NAMESPACE_DNS, 'python.org').hex
        self.client_ip = get_host_ip()  # TODO
        self.client_port = 10050  # TODO default

        # NOTE All submit task will be done in one loop
        self.loop = asyncio.get_event_loop()
        executor_opts = {'max_workers': None}  # type: Dict[str, Any]
        if sys.version_info[:2] >= (3, 6):
            executor_opts['thread_name_prefix'] = 'SyncWorker'
        self.executor = ThreadPoolExecutor(**executor_opts)
        self.loop.set_default_executor(self.executor)

        def exception_handler(loop, context):
            logger.error(context)
            logger.error("An error occurred and the client was about to exit.")
            loop.stop()

        self.loop.set_exception_handler(exception_handler)

        # Storage
        self.tasks_map = {}  # PrimiHub task map task_id: Task
        self._pending_task = []  # pending asyncio task

        self.notify_channel_connected = False

    def init(self, config):
        """Client Initialization.

        config: `dict` [`str`]
        The keys are names of `node` and `cert`.

        >>> from primihub.client import primihub_cli as cli
        >>> cli.init(config={"node": "node_address", "cert": "cert_file_path"})
        """
        logger.info("*** cli init ***")
        logger.debug(config)
        self.node = node = config.get("node", None)
        notify_node = config.get("node", None).split(":")[0] + ":6666"
        self.cert = cert = config.get("cert", None)

        # grpc clients: Command/Notify/Dataset
        self.grpc_client = GrpcClient(node, cert)
        self.notify_grpc_client = None   #NodeServiceClient(notify_node, cert)
        # NOTE create when recevie NodeContext in NodeService
        self.dataset_client = DatasetClientFactory.create("flight", node, cert)

        self.code = self.visitor.visit_file()  # get client code str
        """
        TODO 创建与Node之间的NodeService async grpc通道：
        1. 发送ClientContext给Node，接收NodeContext
        2. 收到NodeContext后，根据配置的数据通道和通知通道建立连接（暂时用当前默认的这个grpc连接）
        """
        logger.info("-------create task: async notify-----------")
        # listener.on_event("/0/")(self.node_event_handler(event=None))  # regist event handler

        notify_request = self.notify_grpc_client.client_context(
            self.client_id, self.client_ip, self.client_port)

        try:
            fire_coroutine_threadsafe(
                self.notify_grpc_client.async_get_node_event(notify_request), self.loop)
        except Exception as e:
            logger.error(str(e))

    def start(self):
        """Client Start.
        """
        logger.info("*** cli start ***")
        try:
            self.loop.run_forever()
        except (KeyboardInterrupt, SystemExit) as e:
            logger.debug(str(e))
            self.exit()
        except Exception as e:
            logger.debug(str(e))
            self.exit()

    def stop(self):
        """Client Stop
        """
        logger.info("*** cli stop ***")
        self.loop.stop()

    def exit(self):
        """Client exit
        """
        self.stop()
        # sys.exit()

    async def submit_task(self, job_id, client_id, *args):
        """Send local functions and parameters to the remote side

        :param job_id
        :param client_id
        :param args: `list` [`tuple`] (`function`, `args`)
        """
        task = Task(task_id="task:" + uuid.uuid5(uuid.NAMESPACE_DNS,
                                                 'python.org').hex, primihub_client=self)
        task_id = task.task_id
        self.tasks_map[task_id] = task

        n = 0
        while self.notify_channel_connected is False:
            n += 1
            logger.debug(
                "----- waiting notify channel connected {}".format("▒" * int(n)))
            logger.debug(n)
            await asyncio.sleep(n)  # 50ms

        func_params_map = ph.context.Context.func_params_map
        logger.debug(func_params_map)
        for arg in args:
            func = arg[0]
            params = arg[1:]
            func_params_map[func.__name__] = params

        self.code = self.visitor.trans_remote_execute(self.code)
        logger.debug("╔═" + "=" * 60 + "═╗")
        logger.debug(self.code)
        logger.debug("╚═" + "=" * 60 + "═╝")
        ph_context_str = "ph.context.Context.func_params_map = %s" % func_params_map
        self.code += "\n"
        self.code += ph_context_str
        logger.info("-*-" * 25)
        logger.info("Have a cup of coffee, it will take a lot of time here.")
        logger.info("-*-" * 25)
        try:
            logger.debug("grpc submit task.")
            # self.grpc_client.submit_task(code=self.code, job_id=job_id, task_id=task_id, submit_client_id=client_id)
            thread = threading.Thread(target=self.grpc_client.submit_task, args=(self.code, job_id, task_id, client_id))
            thread.start()
        except asyncio.CancelledError:
            logger.error("cancel the future.")
        except asyncio.TimeoutError:
            logger.error("timeout error")
        except Exception as e:
            logger.error(str(e))
            self.exit()
        else:
            logger.info("grpc submit end.")

    def async_remote_execute(self, *args) -> None:
        job_id = "job:" + uuid.uuid1().hex
        # task_id = uuid.uuid1().hex
        client_id = self.client_id
        logger.debug("------- create task: async submit task -----------")
        logger.debug("job_id: {}, type: {}".format(job_id, type(job_id)))
        logger.debug("client_id: {}, type: {}".format(
            client_id, type(client_id)))

        logger.debug("------- async run submit task -----------")
        try:
            fire_coroutine_threadsafe(self.submit_task(
                job_id, client_id, *args), self.loop)
        except Exception as e:
            logger.debug(str(e))

        self.start()
