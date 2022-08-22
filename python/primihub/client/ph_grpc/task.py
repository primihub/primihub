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
from primihub.client.ph_grpc.service import NodeServiceClient, NODE_EVENT_TYPE, NODE_EVENT_TYPE_TASK_STATUS, \
    NODE_EVENT_TYPE_TASK_RESULT
from primihub.utils.async_util import fire_coroutine_threadsafe


class Task(object):

    def __init__(self, task_id, primihub_client):
        self.task_id = task_id
        self.task_status = "PENDING"  # PENDING, RUNNING, SUCCESS, FAILED
        self.node_grpc_connections = []
        # from primihub.client import primihub_cli as cli
        self.cli = primihub_client
        self.loop = self.cli.loop

    def get_status(self):
        pass

    def get_result(self):
        pass

    def get_node_event(self, node, cert):
        # # get node event from other nodes
        worker_node_grpc_client = NodeServiceClient(node, cert)
        self.node_grpc_connections.append(worker_node_grpc_client)
        notify_request = worker_node_grpc_client.client_context(
            self.cli.client_id, self.cli.client_ip, self.cli.client_port)

        fire_coroutine_threadsafe(
            worker_node_grpc_client.async_get_node_event(notify_request), self.loop)
