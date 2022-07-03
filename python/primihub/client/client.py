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
from primihub.client.visitor import Visitor
from primihub.client.grpc_client import GRPCClient
import primihub as ph
import inspect
import ast


class PrimihubClient(object):
    """Primihub Python SDK Client."""
    __instance = None
    __first_init = False

    def __new__(cls):
        if not cls.__instance:
            cls.__instance = object.__new__(cls)
        return cls.__instance

    def __init__(self, *args, **kwargs) -> None:
        if not self.__first_init:
            PrimihubClient.__first_init = True

        self.vistitor = Visitor()

    def init(self, config):
        """Client Initialization.

        config: `dict` [`str`]
        The keys are names of `node` and `cert`.


        # >>> from primihub import cli
        >>> from primihub.client import primihub_cli as cli
        >>> cli.init(config={"node": "node_address", "cert": "cert_file_path"})
        """
        print("***")
        print(config)
        node = config.get("node", None)
        cert = config.get("cert", None)

        self.grpc_cli = GRPCClient(node=node, cert=cert)

        self.code = self.vistitor.visit_file()
        # print(self.code)

        return self.grpc_cli

    def submit_task(self, code):

        self.grpc_cli.set_task_map(code=code.encode('utf-8'))
        res = self.grpc_cli.submit()
        print("res: ", res)
        return res

    def remote_execute(self, *args):
        """Send local functions and parameters to the remote side

        :param args: `list` [`tuple`] (`function`, `args`)
        """
        func_params_map = ph.context.Context.func_params_map
        for arg in args:
            func = arg[0]
            params = arg[1:]
            func_params_map[func.__name__] = params

        print(func_params_map)

        self.code = self.vistitor.trans_remote_execute(self.code)
        ph_context_str = "ph.context.Context.func_params_map = %s" % func_params_map
        self.code += "\n"
        self.code += ph_context_str
        print(self.code)
        res = self.submit_task(self.code)
        print(res)
        return res


# alias
primihub_cli = PrimihubClient()
