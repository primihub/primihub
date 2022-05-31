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

from primihub.context import Context
from dill import loads

shared_globals = dict()
shared_globals['context'] = Context

class Executor:
    def __init__(self):
        pass
    

    @staticmethod
    def execute(py_code: str):
        exec(py_code, shared_globals)

    @staticmethod
    def execute_role(role: str):
        try:
            loads(Context.nodes_context[role].dumps_func)()
        except Exception as e:
            print(e)
            raise e

    @staticmethod
    def execute1(dumps_func):
        try:
            print("start execute 1")
            t = loads(dumps_func)
            print(t)
            loads(dumps_func)()
            print("end execute 1")
        except Exception as e:
            print(e)