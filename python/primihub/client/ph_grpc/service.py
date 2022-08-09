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
import grpc

from .connect import GRPCClient, GRPCConnect
from src.primihub.protos import service_pb2, service_pb2_grpc  # noqa


class NodeServiceClient(GRPCClient):
    """primihub gRPC node context service client.

    :return: A primihub gRPC node context service client.
    """

    def __init__(self, connect: GRPCConnect) -> None:
        """Constructor
        """
        super().__init__(connect)
        self.channel = connect.aio_channel
        self.stub = service_pb2_grpc.NodeContextServiceStub(self.channel)

    @staticmethod
    def client_context(client_id: str, client_ip: str, client_port: int):
        request = service_pb2.ClientContext(
            **{"client_id": client_id, "client_ip": client_ip, "client_port": client_port})
        return request

    async def get_node_event_async(self, request: service_pb2.ClientContext) -> None:
        async with self.channel:
            # Read from an async generator
            async for response in self.stub.SubscribeNodeEvent(request):
                print("NodeService client received from async generator: " +
                      response.event_type)
            # # Direct read from the stub
            # node_event_reply_stream = self.stub.SubscribeNodeEvent(request)
            # while True:
            #     response = await node_event_reply_stream.read()
            #     if response == grpc.aio.EOF:
            #         break
            #     print("NodeService client received from direct read: " +
            #           response.event_type)

    async def get_node_event_direct(self, request: service_pb2.ClientContext) -> None:
        async with self.channel:
            # # Read from an async generator
            # async for response in self.stub.SubscribeNodeEvent(request):
            #     print("NodeService client received from async generator: " +
            #           response.event_type)
            # Direct read from the stub
            node_event_reply_stream = self.stub.SubscribeNodeEvent(request)
            while True:
                response = await node_event_reply_stream.read()
                if response == grpc.aio.EOF:
                    break
                print("NodeService client received from direct read: " +
                      response.event_type)


class DataServiceClient(GRPCClient):
    """primihub gRPC data service client.

    :return: A primihub gRPC data service client.
    """

    def __init__(self, connect: GRPCConnect) -> None:
        """Constructor
        """
        super().__init__(connect)
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
