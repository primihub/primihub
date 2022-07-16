import functools
import os
from typing import Callable

# from dill import dumps
from cloudpickle import dumps


class NodeContext:
    def __init__(self, role, protocol, datasets, func=None, next_peer=None):
        self.role = role
        self.protocol = protocol
        self.datasets = datasets
        self.func = func
        self.next_peer = next_peer
        print("func type: ", type(func))

        self.dumps_func = None
        if isinstance(func, Callable):
            # pickle dumps func
            self.dumps_func = dumps(func)
        elif type(func) == str:
            self.dumps_func = func

        if self.dumps_func:
            print("dumps func:", self.dumps_func)


class TaskContext:
    """ key: role, value:NodeContext """
    nodes_context = dict()
    dataset_service = None
    datasets = []
    # dataset meta information
    dataset_map = dict()
    predict_file_path = "result/xgb_prediction.csv"
    indicator_file_path = "result/xgb_indicator.json"
    func_params_map = dict()

    def __init__(self) -> None:
        pass

    def get_protocol(self):
        """Get current task support protocol.
           NOTE: Only one protocol is supported in one task now.
            Maybe support mix protocol in one task in the future.
        Returns:
            string: protocol string
        """
        protocol = None
        try:
            protocol = list(self.nodes_context.values())[0].protocol
        except IndexError:
            protocol = None

        return protocol

    def get_roles(self):
        return list(self.nodes_context.keys())

    def get_datasets(self):
        return self.datasets

    def get_func_params_map(self):
        return self.func_params_map

    def get_predict_file_path(self):
        output_dir = os.path.dirname(self.predict_file_path)
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)
        return self.predict_file_path

    def get_indicator_file_path(self):
        output_dir = os.path.dirname(self.indicator_file_path)
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)
        return self.indicator_file_path


Context = TaskContext()


def set_node_context(role, protocol, datasets,  next_peer):
    print("========set node context: ", role, protocol, datasets,  next_peer)
    Context.nodes_context[role] = NodeContext(role, protocol, datasets, None, next_peer)  # noqa
    # TODO set dataset map, key dataset name, value dataset meta information


def set_task_context_func_params(func_name, func_params):
    Context.params_map[func_name] = func_params


def set_task_context_dataset_map(k, v):
    Context.dataset_map[k] = v


def set_task_context_predict_file(f):
    Context.predict_file_path = f


def set_task_context_indicator_file(f):
    Context.indicator_file_path = f


# For test
def set_text(role, protocol, datasets, dumps_func):
    print("========", role, protocol, datasets, dumps_func)


# Register dataset decorator
def reg_dataset(func):
    @functools.wraps(func)
    def reg_dataset_decorator(dataset):
        print("Register dataset:", dataset)
        Context.datasets.append(dataset)
        return func(dataset)

    return reg_dataset_decorator


# Register task decorator
def function(protocol, role, datasets, next_peer):
    def function_decorator(func):
        print("Register task:", func.__name__)
        Context.nodes_context[role] = NodeContext(
            role, protocol, datasets, func, next_peer)

        print(">>>>> next peer in {}'s node context is {}.".format(role, Context.nodes_context[role].next_peer)) 
        print(">>>>> dataset in {}'s node context is {}.".format(role, Context.nodes_context[role].datasets)) 
        print(">>>>> role in {}'s node context is {}.".format(role, Context.nodes_context[role].role)) 

        @functools.wraps(func)
        def wapper(*args, **kwargs):
            return func(*args, **kwargs)

        return wapper

    return function_decorator
