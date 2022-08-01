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
import os
import traceback
# from dill import loads
from cloudpickle import loads
from primihub.context import Context
from multiprocessing import Process

shared_globals = dict()
shared_globals['context'] = Context


def _handle_timeout():
    raise TimeoutError('function timeout')


def timeout(interval, callback=None):
    def decorator(func):
        def wrapper(*args, **kwargs):
            import gevent  # noqa
            from gevent import monkey  # noqa
            monkey.patch_all()

            try:
                gevent.with_timeout(interval, func, *args, **kwargs)
            except gevent.timeout.Timeout as e:
                callback() if callback else None
        return wrapper
    return decorator


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
    @timeout(60 * 60, _handle_timeout)  # TODO TIMEOUT 60 * 60
    def execute_py(dumps_func):
        print("execute oy code.")
        func_name = loads(dumps_func).__name__
        print("func name: ", func_name)
        func_params = Context.get_func_params_map().get(func_name, None)
        func = loads(dumps_func)
        print("func params: ", func_params)
        if not func_params:
            try:
                print("start execute")
                func()
                print("end execute")
            except Exception as e:
                print("Exception: ", str(e))
                traceback.print_exc()
            finally:
                ...
        else:
            try:
                print("start execute with params")
                func(*func_params)
                print("end execute with params")
            except Exception as e:
                print("Exception: ", str(e))
                traceback.print_exc()
            finally:
                ...

    @staticmethod
    def execute_test():
        print("This is a tset function.")
