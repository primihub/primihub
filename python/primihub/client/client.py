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
from primihub.client.grpc_client import GRPCClient


class PrimihubClient(object):
    """Primihub Python SDK Client."""
    __instance = None
    __first_init = False

    def __new__(cls):
        if not cls.__instance:
            cls.__instance = object.__new__(cls)
        return cls.__instance

        # vistitor = Visitor()
        # code = vistitor.visit_file()
        # grpc_client = GRPCClient(node=NODE, cert=CERT)
        # grpc_client.set_task_map(code=code.encode('utf-8'))
        # res = grpc_client.submit()
        # print("res: ", res)
        # return object.__new__(cls)

    def __init__(self, *args, **kwargs) -> None:
        if not self.__first_init:
            PrimihubClient.__first_init = True

    def init(self, **config):
        """Client Initialization.

        config: `dict` [`str`]
        The keys are names of `node` and `cert`.


        # >>> from primihub import cli
        >>> from primihub.client import primihub_cli as cli
        >>> cli.init(config={"node": "node_address", "cert": "cert_file_path"})
        """
        node = config.get("node", None)
        cert = config.get("cert", None)

        grpc_cli = GRPCClient(node=node, cert=cert)
        # grpc_client.set_task_map(code=code.encode('utf-8'))
        return grpc_cli


# alias
primihub_cli = PrimihubClient()
