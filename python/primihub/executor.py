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


def _run_in_process(target, *args, **kwargs):
    """Runs target in process and returns its exitcode after 10s (None if still alive)."""
    process = multiprocessing.Process(target=target, args=args, kwargs=kwargs)
    process.daemon = True
    try:
        process.start()
        # Do not need to wait much, 10s should be more than enough.
        process.join(timeout=10)
        return process.exitcode
    finally:
        if process.is_alive():
            print("process is alive...")
            ...
            # process.terminate()


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
    @timeout(60 * 60, _handle_timeout)  # TODO TIMEOUT 60 * 60
    def execute_py(dumps_func):
        logger.info("execute py code.")
        func_name = loads(dumps_func).__name__
        logger.debug("func name: ", func_name)
        func_params = Context.get_func_params_map().get(func_name, None)
        func = loads(dumps_func)
        logger.debug("func params: ", func_params)
        if not func_params:
            try:
                logger.debug("start execute")
                # func()
                exitcode = _run_in_process(target=func)
                logger.info("exitcode is: %s" % exitcode)
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
                exitcode = _run_in_process(target=func, args=func_params)
                logger.info("exitcode is: %s" % exitcode)
                logger.debug("end execute with params")
            except Exception as e:
                logger.error("Exception: ", str(e))
                traceback.print_exc()
            finally:
                Context.clean_content()


    @staticmethod
    def execute_test():
        print("This is a tset function.")
