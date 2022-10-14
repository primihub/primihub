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
import multiprocessing
import traceback
from cloudpickle import loads
from primihub.context import Context

from primihub.utils.logger_util import logger

shared_globals = dict()
shared_globals['context'] = Context


def _run_in_process(target, args=(), kwargs={}):
    """Runs target in process and returns its exitcode after 10s (None if still alive)."""
    process = multiprocessing.Process(target=target, args=args, kwargs=kwargs)
    process.daemon = True
    process.start()
    return process


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
            logger.error(str(e))
            raise e

    @staticmethod
    def execute_py(dumps_func):
        logger.info("execute py code.")
        func_name = loads(dumps_func).__name__
        logger.debug("func name: ", func_name)
        func_params = Context.get_func_params_map().get(func_name, None)
        func = loads(dumps_func)
        logger.debug("func params: ", func_params)
        logger.debug("params_map: ", Context.params_map)
        if not func_params:
            try:
                logger.debug("start execute")
                # func()
                process = _run_in_process(target=func)
                Context.clean_content()

                while process.exitcode is None:
                    process.join(timeout=5)
                    logger.debug("Wait for FL task to finish, pid is {}".format(process.pid))
                logger.debug("end execute")
            except Exception as e:
                logger.error("Exception: ", str(e))
                traceback.print_exc()
            finally:
                Context.clean_content()
        else:
            try:
                logger.debug("start execute with params")
                # func(*func_params)
                process = _run_in_process(target=func, args=func_params)
                Context.clean_content()

                while process.exitcode is None:
                    process.join(timeout=5)
                    logger.debug("Wait for FL task to finish, pid is {}".format(process.pid))
                logger.debug("end execute with params")
            except Exception as e:
                logger.error("Exception: ", str(e))
                traceback.print_exc()
            finally:
                Context.clean_content()

    @staticmethod
    def execute_test():
        print("This is a tset function.")