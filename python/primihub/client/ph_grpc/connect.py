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
import sys
from os import path

here = path.abspath(path.join(path.dirname(__file__), "."))
sys.path.append(here)


class GRPCConnect(object):
    """primihub gRPC connect

    :param str node: Address of the node.
    :param str cert: Path of the local cert file path.

    :return: A primihub gRPC connect.
    """

    def __init__(self, node: str, cert: str) -> None:
        """Constructor
        """
        self.task_map = {}
        if node is not None:
            self.node = node
            # self.channel = grpc.insecure_channel(node)
            # self.aio_channel = grpc.aio.insecure_channel(node)
        else:
            raise

        if cert is not None:
            self.cert = cert
            # TODO
            pass
