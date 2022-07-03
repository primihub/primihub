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
import traceback
from dill import loads
from primihub.context import Context

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

    @staticmethod
    def execute_with_params(dumps_func=None, dumps_params=None):
        func_name = loads(dumps_func).__name__
        # print("func: ", loads(dumps_func))
        print("func name: ", func_name)
        # print("func params: ", loads(dumps_params))
        if not dumps_params:
            func_params = Context.get_func_params_map().get(func_name, None)
        else:
            func_params = loads(dumps_params)
        func = loads(dumps_func)
        print("func params: ", func_params)
        if not func_params:
            try:
                print("start execute")
                func()
                print("end execute")
            except Exception as e:
                print(e)
                exc_type, exc_value, exc_traceback_obj = sys.exc_info()
                print(exc_type, exc_value)
                traceback.print_tb(exc_traceback_obj)
        else:
            try:
                print("start execute with params")
                func(*func_params)
                print("end execute with params")
            except Exception as e:
                print(e)
                exc_type, exc_value, exc_traceback_obj = sys.exc_info()
                print(exc_type, exc_value)
                traceback.print_tb(exc_traceback_obj)
