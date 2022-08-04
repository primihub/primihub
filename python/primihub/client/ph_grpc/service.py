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

from .connect import GRPCClient, GRPCConnect
from src.primihub.protos import service_pb2, service_pb2_grpc  # noqa


class NodeContextServiceClient(GRPCClient):
    """primihub gRPC node context service client.

    :return: A primihub gRPC node context service client.
    """

    def __init__(self, connect: GRPCConnect) -> None:
        """Constructor
        """
        super().__init__(connect)
        self.stub = service_pb2_grpc.NodeContextServiceStub(self.channel)

    @staticmethod
    def client_context(client_id: str, client_ip: str, client_port: int):
        request = service_pb2.ClientContext(**{"client_id": client_id, "client_ip": client_ip, "client_port": client_port})
        return request

    def get_node_context(self, request):
        """gRPC get node context

        :returns: gRPC reply
        :rtype: :obj:`server_pb2.NodeContext`
        """
        with self.channel:
            # request = self.client_context(**request_data)
            reply = self.stub.GetNodeContext(request)
            print("reply : %s" % reply)
            return reply


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

    def new_dataset(self, request_data):
        """gRPC new dataset

        :returns: gRPC reply
        :rtype: :obj:`server_pb2.NewDatasetResponse`
        """
        with self.channel:
            request = self.new_data_request(**request_data)
            reply = self.stub.GetNodeContext(request)
            print("reply : %s" % reply)
            return reply
