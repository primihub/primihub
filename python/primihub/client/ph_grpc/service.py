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
from ...utils.protobuf_to_dict import protobuf_to_dict
from .connect import GRPCConnect
import grpc
from abc import ABC
import sys
from os import path

here = path.abspath(path.join(path.dirname(__file__), "."))
sys.path.append(here)


from src.primihub.protos import service_pb2, service_pb2_grpc  # noqa

NODE_EVENT_TYPE_NODE_CONTEXT = 0
NODE_EVENT_TYPE_TASK_STATUS = 1
NODE_EVENT_TYPE_TASK_RESULT = 2

NODE_EVENT_TYPE = {
    NODE_EVENT_TYPE_NODE_CONTEXT: "NODE_EVENT_TYPE_NODE_CONTEXT",
    NODE_EVENT_TYPE_TASK_STATUS: "NODE_EVENT_TYPE_TASK_STATUS",
    NODE_EVENT_TYPE_TASK_RESULT: "NODE_EVENT_TYPE_TASK_RESULT"
}


class NodeServiceClient(GRPCConnect):
    """primihub gRPC node context service client.

    :return: A primihub gRPC node context service client.
    """

    def __init__(self, node, cert) -> None:
        """Constructor
        """
        super(NodeServiceClient, self).__init__(node, cert)
        self.channel = grpc.aio.insecure_channel(self.node)
        # self.stub = service_pb2_grpc.NodeContextServiceStub(self.channel)
        self.stub = service_pb2_grpc.NodeServiceStub(self.channel)

    @staticmethod
    def client_context(client_id: str, client_ip: str, client_port: int):
        request = service_pb2.ClientContext(
            **{"client_id": client_id, "client_ip": client_ip, "client_port": client_port})
        # print(request)
        return request

    async def async_get_node_event(self, request: service_pb2.ClientContext) -> None:
        print("async_get_node_event")
        async with self.channel:
            print("with chennel", self.channel)
            # Read from an async generator

            # request = service_pb2.ClientContext(
            #         client_id="client01",
            #         # client_ip="127.0.0.1",
            #         # client_port=50051
            #     )

            from primihub.client.ph_grpc.event import listener
            async for response in self.stub.SubscribeNodeEvent(request):
                print("NodeService client received from async generator: " +
                      str(response.event_type))
                # try:
                #     response_dict = protobuf_to_dict(response)
                # except Exception as e:
                #     print("Error", str(e))
                # print("NodeService client received from async generator: " + response_dict)
                print("event type: ", response.event_type)

                # note
                # 1. if NODE_CONTEXT fire
                # 2. elif TASK_STATUS or TASK_RESULT  + taskid fire event to Task object

                if response.event_type == 0:  # NODE_CONTEXT
                    # TODO set node connected event
                    topic = f"/0/"
                    listener.fire(name=topic, data=response)  # fire
                elif response.event_type == 1:  # TASK_STATUS
                    # event_type = NODE_EVENT_TYPE[response.event_type]
                    task_id = response.task_status.task_context.task_id
                    topic = f"/1/{task_id}"
                    listener.fire(name=topic, data=response)  # fire
                elif response.event_type == 2:  # TASK_RESULT
                    # event_type = NODE_EVENT_TYPE[response.event_type]
                    task_id = response.task_result.task_context.task_id
                    topic = f"/2/{task_id}"
                    listener.fire(name=topic, data=response)  # fire


class DataServiceClient(GRPCConnect):
    """primihub gRPC data service client.

    :return: A primihub gRPC data service client.
    """

    def __init__(self, node, cert) -> None:
        """Constructor
        """
        super(DataServiceClient, self).__init__(node, cert)
        self.channel = grpc.insecure_channel(self.node)
        self.stub = service_pb2_grpc.DataServiceStub(self.channel)

    @staticmethod
    def new_data_request(driver: str, path: str, fid: str):
        return service_pb2.NewDatasetRequest(driver, path, fid)

    def new_dataset(self, request):
        """gRPC new dataset

        :returns: gRPC reply
        :rtype: :obj:`server_pb2.NewDatasetResponse`
        """
        with self.channel:
            reply = self.stub.NewDataset(request)
            print("reply : %s" % reply)
            return reply
