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
import random

from primihub.client.ph_grpc.service import NodeServiceClient
from primihub.utils.async_util import fire_coroutine_threadsafe
from primihub.utils.logger_util import logger
import asyncio

lock = asyncio.Lock()


class Task(object):

    def __init__(self, task_id, primihub_client):
        self.task_id = task_id
        self.task_status = "PENDING"  # PENDING, RUNNING, SUCCESS, FAILED
        self.node_grpc_connections = []
        self.cli = primihub_client
        self.loop = self.cli.loop
        self.nodes = []

    async def set_task_status(self, status):
        async with lock:
            self.task_status = status
            if self.task_status == "SUCCESS" or self.task_status == "FAILED":
                logger.debug("The task is over and the node exits.")
                self.nodes.pop()
                logger.debug("Number of current survival nodes: {}".format(len(self.nodes)))
                if len(self.nodes) == 0:
                    self.exit()

    def exit(self):
        try:
            self.cli.tasks_map.pop(self.task_id, None)
        except Exception as e:
            logger.error(str(e))
        logger.debug(self.cli.tasks_map)
        if not self.cli.tasks_map:
            logger.info("All tasks are over and the client is about to exit.")
            self.cli.exit()

    def get_task_status(self):
        return self.task_status

    def get_result(self):
        pass

    def get_node_event(self, node, cert):
        # # get node event from other nodes
        logger.debug("get node event from: {}".format(node))
        worker_node_grpc_client = None # NodeServiceClient(node, cert)
        self.node_grpc_connections.append(worker_node_grpc_client)
        logger.debug(
            "!!!!!!! ------ node_grpc_connections ------node: {}".format(worker_node_grpc_client.node))
        logger.debug(self.node_grpc_connections)
        notify_request = worker_node_grpc_client.client_context(
            self.cli.client_id, self.cli.client_ip, random.randint(10000, 10050))
        # self.cli.client_id, self.cli.client_ip, self.cli.client_port)

        fire_coroutine_threadsafe(
            worker_node_grpc_client.async_get_node_event(notify_request), self.loop)

        # get task status event: task:886313e13b8a53729b900c9aee199e5d SUCCESS , clientId:client:6fa459eaee8a3ca4894edb77e160355z
