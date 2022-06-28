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
from primihub.client.visitor import Visitor

# todo
NODE = "localhost:50050"
CERT = ""


class PrimihubClient(object):

    def __new__(self):
        vistitor = Visitor()
        code = vistitor.visit_file()
        grpc_client = GRPCClient(node=NODE, cert=CERT)
        grpc_client.set_task_map(code=code.encode('utf-8'))
        res = grpc_client.submit()
        print("res: ", res)


PrimihubClient()
print(NODE, CERT)