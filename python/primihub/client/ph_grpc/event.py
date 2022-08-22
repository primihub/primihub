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

from primihub.client.tiny_listener import Listener
from primihub.client.ph_grpc.task import Task
from primihub.client.tiny_listener import Event

listener = Listener()
listener.run()


@listener.on_event("/0/")
def node_event_handler(event: Event):
    # self.notify_channel_connected = True
    # self.notify_channel_connected = True
    print("...node_event_handler: %s" % event)


@listener.on_event("/1/{task_id}")
async def handler_task_status(event: Event):
    task_id = event.params["task_id"]
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
    # node = ""  # TODO
    # cert = ""  # TODO
    nodes = [
        {
            "client_id": "127.0.0.1",
            "client_ip": 6667,
            "client_port": 12345
        },
        {
            "client_id": "127.0.0.1",
            "client_ip": 6668,
            "client_port": 12345

        }
    ]
    for node in nodes:
        connect_str = node['client_id'] + ":" + str(node["client_ip"])
        cert = ""  # TODO

        from primihub.client import primihub_cli as cli
        task = Task(task_id=task_id, primihub_client=cli)
        task.get_node_event(node=connect_str, cert=cert)


@listener.on_event("/2/{task_id}")
async def handler_task_result(event: Event):
    int("handler_task_result", event.params, event.data)

    ...
    # event data
    # {'event_type': 2,
    #  'task_result': {'task_context': {'task_id': '1',
    #                                   'job_id': 'task test result'},
    #                   'result_dataset_url': '' // ? TODO
    #                  }
    #  }
