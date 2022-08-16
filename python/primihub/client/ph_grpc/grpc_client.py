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
import random

from primihub.client.ph_grpc.service import NodeServiceClient
from primihub.client.ph_grpc.worker import WorkerClient
from primihub.client.tiny_listener import Listener, Event

from primihub.utils.protobuf_to_dict import protobuf_to_dict

# NodeEventType

NODE_EVENT_TYPE_NODE_CONTEXT = 0
NODE_EVENT_TYPE_TASK_STATUS = 1
NODE_EVENT_TYPE_TASK_RESULT = 2

NODE_EVENT_TYPE = {
    NODE_EVENT_TYPE_NODE_CONTEXT: "NODE_EVENT_TYPE_NODE_CONTEXT",
    NODE_EVENT_TYPE_TASK_STATUS: "NODE_EVENT_TYPE_TASK_STATUS",
    NODE_EVENT_TYPE_TASK_RESULT: "NODE_EVENT_TYPE_TASK_RESULT"
}
listener = Listener()
listener.run()


@listener.on_event(NODE_EVENT_TYPE[NODE_EVENT_TYPE_NODE_CONTEXT])
async def handler_node_context(event: Event):
    print("handler_node_context", event.params, event.data)
    # TODO
    node_context = event.data.get("node_context")
    notify_channel = node_context.get("notify_channel")
    node = notify_channel["connect_str"]
    cert = notify_channel["key"]
    grpc_client = GrpcClient(node, cert)
    # TODO ip, port from `connect_str`
    client_id = ""
    client_ip = ""
    client_port = 12345
    # get node event from other nodes
    await grpc_client.get_node_event(client_id=client_id, client_ip=client_ip, client_port=client_port)


@listener.on_event(NODE_EVENT_TYPE[NODE_EVENT_TYPE_TASK_STATUS])
async def handler_task_status(event: Event):
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


@listener.on_event(NODE_EVENT_TYPE[NODE_EVENT_TYPE_TASK_RESULT])
async def handler_task_result(event: Event):
    print("handler_task_result", event.params, event.data)
    # TODO
    # event data
    # {'event_type': 2,
    #  'task_result': {'task_context': {'task_id': '1',
    #                                   'job_id': 'task test result'},
    #                   'result_dataset_url': '' // ? TODO
    #                  }
    #  }


class GrpcClient(object):
    """primihub grpc client"""

    def __init__(self, node, cert):
        self.node = node
        self.cert = cert
        self.listener = listener

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
        client = WorkerClient(self.node, self.cert)
        task_map = client.set_task_map(code=code.encode('utf-8'),
                                       job_id=bytes(str(job_id), "utf-8"),
                                       task_id=bytes(str(task_id), "utf-8"))

        request = client.push_task_request(intended_worker_id=b'1',
                                           task=task_map,
                                           sequence_number=random.randint(0, 9999),
                                           client_processed_up_to=random.randint(0, 9999))
        res = client.submit_task(request)
        return res

    async def get_node_event(self, client_id: str, client_ip: str, client_port: int):
        client = NodeServiceClient(self.node, self.cert)
        request = client.client_context(client_id=client_id,
                                        client_ip=client_ip,
                                        client_port=client_port)
        async with client.channel:
            # Read from an async generator
            async for response in client.stub.SubscribeNodeEvent(request):
                print("NodeService client received from async generator: " + response.event_type)
                response_dict = protobuf_to_dict(response)
                event_type = NODE_EVENT_TYPE[response.event_type]
                listener.fire(name=event_type, data=response_dict)
