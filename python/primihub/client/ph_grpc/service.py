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


class ServiceClient(GRPCClient):
    """primihub gRPC service client


    :param str node: Address of the node.
    :param str cert: Path of the local cert file path.

    :return: A primihub gRPC service client.
    """
    pass

    def set_context_request_data(self, client_id: str,
                         client_ip: str,
                         client_port: int):
        self.request_data = {
            "client_id": client_id,
            "client_ip": client_ip,
            "client_port": client_port
        }

        return self.request_data

    def get_node_context(self, request_data):
        """gRPC get node context

        :returns: gRPC reply
        :rtype: :obj:`server_pb2.NodeContext`
        """
        with self.channel:
            stub = service_pb2_grpc.NodeContextServiceStub(self.channel)
            request = service_pb2.ClientContext(**request_data)
            # print("request: ", request)
            reply = stub.GetNodeContext(request)

            # print("-*-" * 30)
            print("reply : %s" % (reply))
            # print("-*-" * 30)
            return reply
