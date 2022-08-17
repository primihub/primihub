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
from primihub.client.ph_grpc.event import listener
from primihub.client.ph_grpc.grpc_client import GrpcClient
from primihub.client.tiny_listener import Event

NODE_EVENT_TYPE_NODE_CONTEXT = 0
NODE_EVENT_TYPE_TASK_STATUS = 1
NODE_EVENT_TYPE_TASK_RESULT = 2

NODE_EVENT_TYPE = {
    NODE_EVENT_TYPE_NODE_CONTEXT: "NODE_EVENT_TYPE_NODE_CONTEXT",
    NODE_EVENT_TYPE_TASK_STATUS: "NODE_EVENT_TYPE_TASK_STATUS",
    NODE_EVENT_TYPE_TASK_RESULT: "NODE_EVENT_TYPE_TASK_RESULT"
}


class Task(object):

    def __init__(self, task_id):
        self.task_id = task_id

    @listener.on_event("/task/{task_id}/%s" % NODE_EVENT_TYPE[NODE_EVENT_TYPE_NODE_CONTEXT])
    async def handler_node_context(self, event: Event):
        print("handler_node_context", event.params, event.data)
        # TODO
        # node_context = event.data.get("node_context")
        # notify_channel = node_context.get("notify_channel")
        # node = notify_channel["connect_str"]
        # cert = notify_channel["key"]
        # grpc_client = GrpcClient(node, cert)

    @listener.on_event("/task/{task_id}/%s" % NODE_EVENT_TYPE[NODE_EVENT_TYPE_TASK_STATUS])
    async def handler_task_status(self, event: Event):
        print("handler_task_status", event.params, event.data)
        # TODO
        # event data
        # {'event_type': 1,
        #      'task_status': {'task_context': {'task_id': '1',
        #                                       'job_id': 'task test status'
        #                                       }
        #                   'status': '' // ? TODO
        #                      }
        #      }
        node = ""  # TODO
        cert = ""  # TODO
        client_id = ""
        client_ip = ""
        client_port = 12345
        # get node event from other nodes
        grpc_client = GrpcClient(node, cert)
        task_id = event.data["task_status"]["task_context"]["task_id"]
        await grpc_client.get_task_node_event(task_id=task_id, client_id=client_id, client_ip=client_ip,
                                              client_port=client_port)

    @listener.on_event("/task/{task_id}/%s" % NODE_EVENT_TYPE[NODE_EVENT_TYPE_TASK_RESULT])
    async def handler_task_result(self, event: Event):
        print("handler_task_result", event.params, event.data)
        # TODO
        # event data
        # {'event_type': 2,
        #  'task_result': {'task_context': {'task_id': '1',
        #                                   'job_id': 'task test result'},
        #                   'result_dataset_url': '' // ? TODO
        #                  }
        #  }

    def get_status(self):
        pass

    def get_result(self):
        pass
